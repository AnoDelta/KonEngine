#pragma once
// asset_manager.hpp -- declaration only.
// Implementation is in asset_manager.cpp which is the only TU that includes konpak.hpp.
// This keeps windows.h / zlib.h / openssl out of every translation unit.

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <cstdint>

// Forward-declare Pack so we don't need to include konpak.hpp here
namespace KonPak { struct Pack; }

class AssetManager {
public:
    // Call once at startup before loading any assets.
    //   AssetManager::init("assets/");     -- dev: loose files
    //   AssetManager::init("game.konpak"); -- release: encrypted pack
    static void init(const std::string& pathOrRoot);

    // Returns raw bytes of an asset. Works in both modes.
    static std::vector<uint8_t> readFile(const std::string& assetPath);

    // Returns a disk path suitable for passing to file-opening APIs.
    // In pack mode, extracts to /tmp on first call and caches.
    static std::string resolvePath(const std::string& assetPath);

    static bool isPackMode();
    static const std::string& root();
    static void shutdown();

private:
    AssetManager() = default;
    static AssetManager& get();

    bool        m_packMode = false;
    std::string m_root;
    std::string m_tempDir;

#ifdef KON_USE_PACK
    std::unique_ptr<KonPak::Pack> m_pack;
#endif
    std::unordered_map<std::string, std::string> m_tempPaths;

    static std::string sanitize(const std::string& path);
    static void        ensureDir(const std::string& filePath);
};
