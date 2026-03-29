#pragma once
// asset_manager.hpp — Transparent asset loading for KonEngine.
//
// In development:  loads files directly from disk (fast iteration).
// In release:      loads from a .konpak archive (encrypted, compressed).
//
// Usage (dev):
//   AssetManager::init("assets/");          // loose files root
//
// Usage (release — key baked at compile time via KON_PACK_KEY):
//   AssetManager::init("game.konpak");       // loads and decrypts on startup
//
// Then in game code — same API either way:
//   Texture t = LoadTexture("sprites/player.png");
//   Sound   s = LoadSound("audio/jump.wav");
//   Music   m = LoadMusic("audio/music.ogg");
//
// All engine load functions (LoadTexture, LoadSound, LoadMusic, LoadFont)
// go through AssetManager automatically when a pack is loaded.
//
// The key is NEVER exposed at runtime — it's a compile-time string constant
// burned into the binary. Users can't extract the pack without it.

#include <string>
#include <vector>
#include <unordered_map>
#include <stdexcept>
#include <fstream>
#include <cstdint>
#include <memory>
#include <iostream>
#include <sys/stat.h>
#include <sys/types.h>

// Only include konpak if it's available (engine release build)
#ifdef KON_PACK_KEY
#  ifndef KON_USE_PACK
#    define KON_USE_PACK
#  endif
#endif

// TODO: will change to just konpak.hpp soon
#ifdef KON_USE_PACK
#  include "konpak.hpp"
#endif

class AssetManager {
public:
    // -----------------------------------------------------------------------
    // Init — call once at startup before loading any assets.
    //
    //   AssetManager::init("assets/");        -- dev mode, loose files
    //   AssetManager::init("game.konpak");    -- release mode, pack file
    //
    // In release mode the pack is opened with the KON_PACK_KEY baked at
    // compile time. If KON_PACK_KEY is not defined, pack mode is unavailable.
    // -----------------------------------------------------------------------
    static void init(const std::string& pathOrRoot) {
        auto& am = get();
        am.m_root = pathOrRoot;

        // Detect pack mode: ends with .konpak
        bool isPack = pathOrRoot.size() > 7 &&
                      pathOrRoot.substr(pathOrRoot.size() - 7) == ".konpak";

        if (isPack) {
#ifdef KON_USE_PACK
            am.m_pack = std::make_unique<KonPak::Pack>();
            try {
                am.m_pack->openWithBuiltinKey(pathOrRoot);
                am.m_packMode = true;
                std::cout << "[AssetManager] Loaded pack: " << pathOrRoot
                          << " (" << am.m_pack->entries.size() << " files)\n";
            } catch (const std::exception& e) {
                std::cerr << "[AssetManager] Failed to open pack: "
                          << e.what() << "\n";
                std::cerr << "[AssetManager] Falling back to loose files\n";
                am.m_packMode = false;
                am.m_root     = "./";
            }
#else
            std::cerr << "[AssetManager] Pack requested but KON_USE_PACK not defined.\n"
                      << "  Compile with -DKON_PACK_KEY=... to enable.\n";
            am.m_packMode = false;
            am.m_root     = "./";
#endif
        } else {
            am.m_packMode = false;
            // Ensure root ends with /
            if (!am.m_root.empty() && am.m_root.back() != '/')
                am.m_root += '/';
        }
    }

    // -----------------------------------------------------------------------
    // readFile — primary API. Returns raw bytes of a file by asset path.
    //   "sprites/player.png"
    //   "audio/jump.wav"
    // Returns empty vector if not found.
    // -----------------------------------------------------------------------
    static std::vector<uint8_t> readFile(const std::string& assetPath) {
        auto& am = get();

#ifdef KON_USE_PACK
        if (am.m_packMode && am.m_pack) {
            const auto* entry = am.m_pack->find(assetPath);
            if (entry) return entry->data;
            // Try without leading ./
            std::string trimmed = assetPath;
            if (trimmed.size() > 2 && trimmed[0] == '.' && trimmed[1] == '/')
                trimmed = trimmed.substr(2);
            entry = am.m_pack->find(trimmed);
            if (entry) return entry->data;
            std::cerr << "[AssetManager] Not found in pack: " << assetPath << "\n";
            return {};
        }
#endif
        // Loose file mode
        std::string fullPath = am.m_root + assetPath;
        std::ifstream f(fullPath, std::ios::binary);
        if (!f.is_open()) {
            // Also try without root prefix (absolute paths)
            f.open(assetPath, std::ios::binary);
            if (!f.is_open()) {
                std::cerr << "[AssetManager] File not found: " << fullPath << "\n";
                return {};
            }
            fullPath = assetPath;
        }
        return std::vector<uint8_t>(
            std::istreambuf_iterator<char>(f),
            std::istreambuf_iterator<char>()
        );
    }

    // -----------------------------------------------------------------------
    // resolvePath — returns the full disk path for a loose-file asset.
    // Use this when an API needs a filename (e.g. OpenAL, stb_image from file).
    // In pack mode, writes the asset to a temp file and returns that path.
    // -----------------------------------------------------------------------
    static std::string resolvePath(const std::string& assetPath) {
        auto& am = get();

#ifdef KON_USE_PACK
        if (am.m_packMode && am.m_pack) {
            // Check if we already extracted this file to temp
            auto it = am.m_tempPaths.find(assetPath);
            if (it != am.m_tempPaths.end()) return it->second;

            // Extract to temp
            auto data = readFile(assetPath);
            if (data.empty()) return assetPath; // fallback

            // Build a temp path
            std::string tmpPath = am.m_tempDir + sanitize(assetPath);
            ensureDir(tmpPath);
            std::ofstream f(tmpPath, std::ios::binary);
            if (f.is_open()) {
                f.write(reinterpret_cast<const char*>(data.data()), data.size());
                am.m_tempPaths[assetPath] = tmpPath;
                return tmpPath;
            }
            return assetPath;
        }
#endif
        return am.m_root + assetPath;
    }

    // -----------------------------------------------------------------------
    // Accessors
    // -----------------------------------------------------------------------
    static bool isPackMode() { return get().m_packMode; }
    static const std::string& root()   { return get().m_root; }

    // Cleanup temp files on shutdown
    static void shutdown() {
        auto& am = get();
#ifdef _WIN32
        for (auto& [path, tmp] : am.m_tempPaths) ::DeleteFileA(tmp.c_str());
#else
        for (auto& [path, tmp] : am.m_tempPaths) ::remove(tmp.c_str());
#endif
        am.m_tempPaths.clear();
        am.m_packMode = false;
        am.m_root.clear();
#ifdef KON_USE_PACK
        am.m_pack.reset();
#endif
    }

private:
    AssetManager() = default;
    static AssetManager& get() {
        static AssetManager instance;
        return instance;
    }

    bool        m_packMode = false;
    std::string m_root;
    std::string m_tempDir = "/tmp/konpak_";

#ifdef KON_USE_PACK
    std::unique_ptr<KonPak::Pack>  m_pack;
#endif
    std::unordered_map<std::string, std::string> m_tempPaths;

    static std::string sanitize(const std::string& path) {
        std::string s = path;
        for (char& c : s) if (c == '/' || c == '\\') c = '_';
        return s;
    }

    static void ensureDir(const std::string& filePath) {
        size_t pos = filePath.rfind('/');
        if (pos == std::string::npos) return;
        std::string dir = filePath.substr(0, pos);
        // simple mkdir
#ifdef _WIN32
        ::CreateDirectoryA(dir.c_str(), nullptr);
#else
        ::mkdir(dir.c_str(), 0700);
#endif
    }
};
