# KonPaktor Documentation

> Version 0.1.0

KonPaktor is the archive tool for KonEngine. It packs game assets into encrypted,
compressed `.konpak` files for distribution. It ships as two components:

- **`konpak`** — command line tool
- **KonPaktor** — Qt GUI file manager

---

## Table of Contents

1. [The .konpak Format](#1-the-konpak-format)
2. [CLI Tool — konpak](#2-cli-tool--konpak)
3. [GUI — KonPaktor](#3-gui--konpaktor)
4. [Engine Integration](#4-engine-integration)
5. [Compile-Time Key](#5-compile-time-key)
6. [Recommended Workflow](#6-recommended-workflow)
7. [Building](#7-building)

---

## 1. The .konpak Format

`.konpak` is a binary archive format. Files are compressed then encrypted.

```
[HEADER]
  magic         4 bytes    "KPAK"
  version       1 byte     0x01
  flags         1 byte     0x01 = encrypted, 0x02 = compressed
  salt          16 bytes   random, used for PBKDF2 key derivation
  iv            16 bytes   AES initialization vector

[INDEX SIZE]   4 bytes     size of the encrypted index block

[INDEX]        encrypted block containing:
  entry_count  uint32
  per entry:
    path_len   uint16
    path       N bytes     e.g. "sprites/player.png"
    size_raw   uint64      original uncompressed size
    size_packed uint64     size on disk (compressed + encrypted)
    offset     uint64      byte offset from start of DATA section

[DATA]
  ... per-entry compressed+encrypted blobs ...
```

**Security:** files are compressed with zlib deflate, then encrypted with AES-256-CBC.
The key is derived from the password using PBKDF2-SHA256 with 100,000 iterations and a
random 16-byte salt. The index is also encrypted so file paths are not visible without
the password.

**Modifying a pack** requires a full rebuild — the entire archive is decrypted,
modified in memory, then re-encrypted and written. This is fine for game-sized packs.

---

## 2. CLI Tool -- konpak

### Create a new pack

```bash
konpak create output.konpak file1.png file2.wav --pass mypassword
```

Files are stored with their filename as the pack path. Use `--as` to override:

```bash
konpak create game.konpak player.png --as sprites/player.png --pass mypassword
```

If `--pass` is omitted the tool prompts interactively (no echo).

### Add or replace a file

```bash
konpak add game.konpak newsprite.png --as sprites/new.png --pass mypassword
```

If the archive does not exist it is created. If the pack path already exists it is replaced.

### Remove a file

```bash
konpak remove game.konpak sprites/old.png --pass mypassword
```

### List contents

```bash
konpak list game.konpak --pass mypassword
```

Output:
```
game.konpak  (3 file(s))

  sprites/player.png   [4096 bytes raw, 2341 packed]
  sprites/enemy.png    [2048 bytes raw, 1203 packed]
  audio/jump.wav       [44100 bytes raw, 38291 packed]
```

### Extract files

```bash
# Extract everything to ./extracted/
konpak extract game.konpak --out extracted/ --pass mypassword

# Extract a single file
konpak extract game.konpak sprites/player.png --out extracted/ --pass mypassword
```

### Help

```bash
konpak help
```

---

## 3. GUI -- KonPaktor

### Opening a pack

**File > Open Pack** or drag a `.konpak` file onto the window.
You will be prompted for the password.

### Creating a new pack

**File > New Pack** — sets a password and creates an empty pack in memory.
Add files then save with **File > Save** or **Ctrl+S**.

### Adding files

- **Toolbar: Add Files** or **Pack > Add Files**
- **Drag and drop** files directly onto the window

Each file shows a path dialog so you can set where it lives inside the pack
(e.g. `sprites/player.png`).

### Preview panel

Clicking a file in the list shows a preview on the right:

- **Images** (`.png`, `.jpg`, `.bmp`, `.tga`) — thumbnail with dimensions
- **Audio** (`.wav`, `.ogg`, `.mp3`, `.flac`) — Play/Stop buttons
- **`.konani`** — info panel with size and hint to open in KonAnimator
- **Other** — raw size and type info

### Opening in KonAnimator

Right-click any image or `.konani` file → **Open in KonAnimator**.

KonPaktor extracts the file to a temp directory and launches KonAnimator with it.
After editing, use **Pack > Add Files** to add the updated file back.

KonPaktor looks for KonAnimator in these locations automatically:
- `../KonAnimator/build/KonAnimator`
- `../../KonAnimator/build/KonAnimator`
- `KonAnimator` (in PATH)
- `/usr/local/bin/KonAnimator`

### Extracting files

- **Right-click > Extract selected** — extract selected files to a directory
- **Toolbar: Extract All** — extract everything with a progress dialog
- **Double-click** — extract to temp and open with the OS default application

### Removing files

Select one or more files and press **Delete** or right-click > **Remove from pack**.
Confirm the prompt. Save the pack to apply changes.

---

## 4. Engine Integration

The recommended pattern is to keep loose files during development and pack on release.
The engine does not need to know about `.konpak` at all — just extract the pack at
startup and use normal file paths.

Add `konpak.hpp` to your game project (from `tools/KonPaktor/src/konpak.hpp`).

```cpp
#include "konpak.hpp"
#include <filesystem>
namespace fs = std::filesystem;

void UnpackAssets(const std::string& packFile = "game.konpak",
                  const std::string& cacheDir = "assets_cache") {
#ifdef KON_PACK_KEY
    if (fs::exists(cacheDir)) return; // already extracted this session
    if (!fs::exists(packFile)) return; // no pack -- use loose files

    KonPak::Pack pack;
    pack.openWithBuiltinKey(packFile);
    pack.extractAllTo(cacheDir);
#endif
}

std::string Asset(const std::string& path,
                  const std::string& cacheDir = "assets_cache") {
#ifdef KON_PACK_KEY
    return cacheDir + "/" + path;
#else
    return path; // debug: use loose files directly
#endif
}
```

Then in `main()`:
```cpp
UnpackAssets();
InitWindow(800, 600, "My Game");

Texture sheet = LoadTexture(Asset("sprites/player.png").c_str());
```

In debug builds `Asset()` returns the path unchanged. In release builds it
redirects to the cache directory where the pack was extracted.

---

## 5. Compile-Time Key

To bake the password into the release binary so the player is never prompted,
define `KON_PACK_KEY` at compile time.

**In your game's CMakeLists.txt:**
```cmake
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_definitions(MyGame PRIVATE KON_PACK_KEY="mygamekey123")
endif()
```

Then pack your assets with the same password:
```bash
konpak create game.konpak assets/ --pass mygamekey123
```

The `openWithBuiltinKey()` method reads the `KON_PACK_KEY` define automatically:
```cpp
pack.openWithBuiltinKey("game.konpak"); // uses KON_PACK_KEY at compile time
```

**Security note:** the key is stored in the binary and can be extracted by a
determined person with a debugger or hex editor. This stops casual asset browsing
and automated rippers but is not cryptographically secure against a motivated
attacker. For most indie games this is more than sufficient.

---

## 6. Recommended Workflow

### Development
Work with loose files. No packing needed.
```
assets/
  sprites/player.png
  sprites/player.konani
  audio/jump.wav
```

```cpp
// Debug build -- loose files
Texture t = LoadTexture("assets/sprites/player.png");
```

### Release
1. Pack all assets:
```bash
konpak create game.konpak assets/ --pass mygamekey123
```

2. Build game with key baked in:
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
# game CMakeLists.txt adds -DKON_PACK_KEY="mygamekey123" in Release
cmake --build build
```

3. Ship `game.exe` + `game.konpak`. The game extracts assets at startup automatically.

### Updating assets after packing
```bash
# Update a single file
konpak add game.konpak new_sprite.png --as sprites/new.png --pass mygamekey123

# Or open in KonPaktor GUI, drag in the new file, save
```

---

## 7. Building

### Linux
```bash
# From KonEngine root
./build-tools.sh

# Or from tools/KonPaktor directly
cd tools/KonPaktor
./build.sh
```

**Dependencies:**
```bash
# Ubuntu/Debian
sudo apt-get install qtbase5-dev libqt5multimedia5-dev libssl-dev zlib1g-dev

# Fedora
sudo dnf install qt5-qtbase-devel qt5-qtmultimedia-devel openssl-devel zlib-devel

# Arch
sudo pacman -S qt5-base qt5-multimedia openssl zlib

# Gentoo
emerge --ask dev-qt/qtbase:5 dev-qt/qtmultimedia:5 dev-libs/openssl sys-libs/zlib
```

### Windows
```bat
build-tools.bat
```

Requires Qt5, OpenSSL, and zlib via vcpkg:
```bat
vcpkg install qt5 openssl zlib
```
