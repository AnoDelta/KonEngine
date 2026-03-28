// -----------------------------------------------------------------------------
// KonPak compile-time key example
// -----------------------------------------------------------------------------
//
// In your game's CMakeLists.txt:
//
//   if(CMAKE_BUILD_TYPE STREQUAL "Release")
//       target_compile_definitions(MyGame PRIVATE KON_PACK_KEY="mygamekey123")
//   endif()
//
// Pack your assets before shipping:
//   ./konpak create game.konpak assets/ --pass mygamekey123
//
// Then at startup your game calls UnpackAssets() below.
// In debug builds, loose files are used — no pack needed.
// -----------------------------------------------------------------------------

#include "KonEngine.hpp"

// Only include konpak in release builds, or always if you want pack support in debug
#ifdef KON_PACK_KEY
#include "konpak.hpp"
#endif

#include <filesystem>
#include <iostream>
namespace fs = std::filesystem;

// Call once at startup before any asset loads.
// In release: extracts everything from game.konpak to a temp cache dir.
// In debug: does nothing, loose files are used as-is.
static void UnpackAssets(const std::string& packFile = "game.konpak",
                         const std::string& cacheDir = "assets_cache") {
#ifdef KON_PACK_KEY
    // Already extracted this session? Skip.
    if (fs::exists(cacheDir)) return;

    if (!fs::exists(packFile)) {
        std::cerr << "[KonPak] Warning: " << packFile << " not found. "
                  << "Using loose files.\n";
        return;
    }

    std::cout << "[KonPak] Extracting assets...\n";
    try {
        KonPak::Pack pack;
        pack.openWithBuiltinKey(packFile);
        pack.extractAllTo(cacheDir);
        std::cout << "[KonPak] Done. " << pack.entries.size()
                  << " file(s) extracted to " << cacheDir << "\n";
    } catch (std::exception& e) {
        std::cerr << "[KonPak] Failed to unpack: " << e.what() << "\n";
    }
#endif
}

// Helper: resolve an asset path.
// In release builds, redirects to the cache dir.
// In debug builds, returns the path as-is.
static std::string Asset(const std::string& path,
                         const std::string& cacheDir = "assets_cache") {
#ifdef KON_PACK_KEY
    return cacheDir + "/" + path;
#else
    return path;
#endif
}

// -----------------------------------------------------------------------------
// Example game
// -----------------------------------------------------------------------------
int main() {
    // Unpack once at startup (no-op in debug)
    UnpackAssets();

    InitWindow(800, 600, "My Game");
    SetTargetFPS(60);

    Scene scene;

    auto* player = scene.Add<Sprite2D>("player");
    player->x = 400;
    player->y = 300;

    // Asset() resolves to "assets_cache/sprites/player.png" in release
    // and "sprites/player.png" in debug -- same code, both work
    Texture sheet = LoadTexture(Asset("sprites/player.png").c_str());
    player->SetTexture(sheet);

    auto* anim = player->AddChild<AnimationPlayer>("anim");
    anim->LoadFromFile(Asset("sprites/player.konani").c_str());
    anim->Play("idle");

    while (!WindowShouldClose()) {
        ClearBackground(0.1f, 0.1f, 0.1f);
        scene.Update(GetDeltaTime());
        scene.Draw();
        Present();
        PollEvents();
    }

    UnloadTexture(sheet);
    return 0;
}
