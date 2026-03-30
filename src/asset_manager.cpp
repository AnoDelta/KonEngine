// asset_manager.cpp -- the ONLY translation unit that includes konpak.hpp.
// Keeping konpak.hpp here means windows.h, zlib.h, and openssl headers
// are isolated to one .obj file and don't poison audio.cpp, font.cpp, etc.

#include "asset_manager.hpp"
#include <iostream>
#include <fstream>
#include <sys/stat.h>
#include <sys/types.h>
#ifdef _WIN32
#  define WIN32_LEAN_AND_MEAN
#  define NOGDI
#  define NOUSER
#  ifndef NOMINMAX
#    define NOMINMAX
#  endif
#  include <windows.h>
#  undef DrawText
#  undef Rectangle
#  undef PlaySound
#  undef LoadBitmap
#  undef CreateFont
#  undef CopyFile
#  undef DeleteFile
#  undef MoveFile
#  undef remove
#endif

#ifdef KON_USE_PACK
#  include "konpak.hpp"
#endif

// -----------------------------------------------------------------------
// Singleton
// -----------------------------------------------------------------------
AssetManager& AssetManager::get() {
    static AssetManager instance;
    return instance;
}

// -----------------------------------------------------------------------
// Init
// -----------------------------------------------------------------------
void AssetManager::init(const std::string& pathOrRoot) {
    auto& am = get();
    am.m_root    = pathOrRoot;
#ifdef _WIN32
    // On Windows use %TEMP% or a fallback
    const char* tmp = getenv("TEMP");
    if (!tmp) tmp = getenv("TMP");
    if (!tmp) tmp = "C:\\Temp";
    am.m_tempDir = std::string(tmp) + "\\konpak_";
#else
    am.m_tempDir = "/tmp/konpak_";
#endif

    bool isPack = pathOrRoot.size() > 7 &&
                  pathOrRoot.substr(pathOrRoot.size() - 7) == ".konpak";

    if (isPack) {
#ifdef KON_USE_PACK
        am.m_pack = std::make_unique<KonPak::Pack>();
        try {
            am.m_pack->openWithBuiltinKey(pathOrRoot);
            am.m_packMode = true;
            std::cout << "[AssetManager] Loaded pack: " << pathOrRoot
                      << " (" << am.m_pack->index.size() << " files)\n";
        } catch (const std::exception& e) {
            std::cerr << "[AssetManager] Failed to open pack: " << e.what()
                      << "\n[AssetManager] Falling back to loose files\n";
            am.m_packMode = false;
            am.m_root     = "./";
        }
#else
        std::cerr << "[AssetManager] Pack requested but KON_USE_PACK not defined.\n"
                  << "  Compile with KON_PACK_SUPPORT=ON to enable.\n";
        am.m_packMode = false;
        am.m_root     = "./";
#endif
    } else {
        am.m_packMode = false;
        if (!am.m_root.empty() && am.m_root.back() != '/')
            am.m_root += '/';
    }
}

// -----------------------------------------------------------------------
// readFile
// -----------------------------------------------------------------------
std::vector<uint8_t> AssetManager::readFile(const std::string& assetPath) {
    auto& am = get();

#ifdef KON_USE_PACK
    if (am.m_packMode && am.m_pack) {
        std::string trimmed = assetPath;
        if (trimmed.size() > 2 && trimmed[0] == '.' && trimmed[1] == '/')
            trimmed = trimmed.substr(2);

        const auto* ie = am.m_pack->findIndex(assetPath);
        if (!ie) ie = am.m_pack->findIndex(trimmed);
        if (ie) {
            try { return am.m_pack->getData(ie->path); }
            catch (const std::exception& e) {
                std::cerr << "[AssetManager] getData failed: " << e.what() << "\n";
            }
        }
        std::cerr << "[AssetManager] Not found in pack: " << assetPath << "\n";
        return {};
    }
#endif

    std::string fullPath = am.m_root + assetPath;
    std::ifstream f(fullPath, std::ios::binary);
    if (!f.is_open()) {
        f.open(assetPath, std::ios::binary);
        if (!f.is_open()) {
            std::cerr << "[AssetManager] File not found: " << fullPath << "\n";
            return {};
        }
    }
    return std::vector<uint8_t>(
        std::istreambuf_iterator<char>(f),
        std::istreambuf_iterator<char>()
    );
}

// -----------------------------------------------------------------------
// resolvePath
// -----------------------------------------------------------------------
std::string AssetManager::resolvePath(const std::string& assetPath) {
    auto& am = get();

#ifdef KON_USE_PACK
    if (am.m_packMode && am.m_pack) {
        auto it = am.m_tempPaths.find(assetPath);
        if (it != am.m_tempPaths.end()) return it->second;

        auto data = readFile(assetPath);
        if (data.empty()) return assetPath;

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
bool AssetManager::isPackMode()          { return get().m_packMode; }
const std::string& AssetManager::root()  { return get().m_root; }

void AssetManager::shutdown() {
    auto& am = get();
#ifdef _WIN32
    for (auto& [p, t] : am.m_tempPaths) ::DeleteFileA(t.c_str());
#else
    for (auto& [p, t] : am.m_tempPaths) ::remove(t.c_str());
#endif
    am.m_tempPaths.clear();
    am.m_packMode = false;
    am.m_root.clear();
#ifdef KON_USE_PACK
    am.m_pack.reset();
#endif
}

// -----------------------------------------------------------------------
// Private helpers
// -----------------------------------------------------------------------
std::string AssetManager::sanitize(const std::string& path) {
    std::string s = path;
    for (char& c : s) if (c == '/' || c == '\\') c = '_';
    return s;
}

void AssetManager::ensureDir(const std::string& filePath) {
    size_t pos = filePath.rfind('/');
    if (pos == std::string::npos) return;
    std::string dir = filePath.substr(0, pos);
#ifdef _WIN32
    ::CreateDirectoryA(dir.c_str(), nullptr);
#else
    ::mkdir(dir.c_str(), 0700);
#endif
}
