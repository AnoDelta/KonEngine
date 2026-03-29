# KonEngine

A lightweight 2D game engine written in C++, built for old and low-end machines.
Made primarily for personal use and for friends — simple, fast, and fits the workflow
of making games without the overhead of larger engines.

Heavily inspired by Raylib's simplicity and API style, with a Godot-style node system
and plans for a full editor.

---

## Features

- Simple Raylib-style API — `InitWindow`, `DrawRectangle`, `PlaySound`, done
- OpenGL 2D renderer — rectangles, circles, lines, textures
- Sprite sheet support
- Text rendering — custom TTF fonts or built-in Inconsolata
- Camera system — pan, zoom, rotation
- `GetWorldMouseX/Y(cam)` — correct world-space mouse position
- Collision detection — AABB, circle, SAT (convex polygons)
- CollisionWorld with enter/exit signals and layer/mask filtering
- Input — keyboard, mouse, gamepad
- Color system with presets (`RED`, `WHITE`, `TRANSPARENT`...)
- Audio — sound effects + music streaming (miniaudio)
- Delta time + FPS cap + VSync toggle
- Cross-platform — Linux and Windows
- Node/scene system — Godot-style hierarchy, signals, parent pointers
- `Node::OnCollisionEnter/Exit` virtual callbacks on parent nodes
- Node2D with pivot/origin support
- Sprite2D with texture and tint
- Animation — sprite sheet frame-by-frame + keyframe tracks, 16 easing curves
- `DebugMode(true)` — FPS overlay, mouse crosshair, world-space grid, auto collider outlines
- **KonAnimator** — standalone Qt animation editor with working timeline scrubbing
- **anim_compiler** — CLI + Qt GUI tool, compiles `.anim` → `.konani`
- **KonPaktor / konpak** — AES-256 asset encryption and `.konpak` archive tool
- **KonScript** — statically-typed scripting language that compiles to C++
- Test suite (`./build-test.sh`)

---

## Getting Started

### 1. Clone

```bash
git clone --recurse-submodules https://github.com/AnoDelta/KonEngine.git
cd KonEngine
```

### 2. Install system dependencies

**Linux — Ubuntu/Debian:**
```bash
sudo apt-get install -y libgl1-mesa-dev libx11-dev libxrandr-dev libxi-dev \
  libwayland-dev wayland-protocols libxkbcommon-dev libxinerama-dev libxcursor-dev
```

**Linux — Fedora:**
```bash
sudo dnf install -y mesa-libGL-devel libX11-devel libXrandr-devel libXi-devel \
  wayland-devel wayland-protocols-devel libxkbcommon-devel libXinerama-devel libXcursor-devel
```

**Linux — Arch:**
```bash
sudo pacman -S mesa libx11 libxrandr libxi wayland wayland-protocols \
  libxkbcommon libxinerama libxcursor
```

**Linux — Gentoo:**
```bash
emerge --ask x11-libs/libX11 x11-libs/libXrandr x11-libs/libXi media-libs/mesa
```

**Windows:** No extra dependencies needed.

### 3. Build

**Linux:**
```bash
./build.sh
```

**Windows:**
```bat
build.bat
```

### 4. Link against the engine in your game

```cmake
cmake_minimum_required(VERSION 3.16)
project(MyGame)

find_package(KonEngine REQUIRED)

add_executable(MyGame src/main.cpp)
target_link_libraries(MyGame PRIVATE KonEngine)
```

Or without installing:
```cmake
add_subdirectory(KonEngine)
target_link_libraries(MyGame PRIVATE KonEngine)
```

### 5. Write your first game

```cpp
#include "KonEngine.hpp"

int main() {
    InitWindow(800, 600, "My Game");
    SetTargetFPS(60);

    float x = 400, y = 300;

    while (!WindowShouldClose()) {
        float dt = GetDeltaTime();
        if (IsKeyDown(Key::D)) x += 200.0f * dt;
        if (IsKeyDown(Key::A)) x -= 200.0f * dt;
        if (IsKeyDown(Key::W)) y -= 200.0f * dt;
        if (IsKeyDown(Key::S)) y += 200.0f * dt;

        ClearBackground(0.1f, 0.1f, 0.1f);
        DrawRectangle(x - 16, y - 24, 32, 48, BLUE);
        DrawText("WASD to move", 10, 10, WHITE);
        Present();
        PollEvents();
    }
}
```

Or write it in KonScript:

```ks
#include <engine>

node Player : Node2D {
    let mut speed: F64 = 200.0;

    func Update(dt: F64) {
        if KeyDown(Key.D) { x += speed * dt; }
        if KeyDown(Key.A) { x -= speed * dt; }
    }

    func Draw() {
        DrawRectangle(x - 16.0, y - 24.0, 32.0, 48.0, BLUE);
    }
}

func main() {
    InitWindow(800, 600, "My Game");
    SetTargetFPS(60);
    let scene: Scene = Scene();
    let player: Player = scene.add(Player, "player");
    while !WindowShouldClose() {
        ClearBackground(0.1, 0.1, 0.1);
        scene.update(GetDeltaTime());
        scene.draw();
        Present();
        PollEvents();
    }
}
```

---

## Tools

### KonAnimator & anim_compiler

Visual animation editor and CLI compiler for `.anim` → `.konani` files.

Requires Qt5:
```bash
# Ubuntu/Debian
sudo apt-get install -y qtbase5-dev libqt5opengl5-dev
# Arch
sudo pacman -S qt5-base
# Gentoo
emerge --ask dev-qt/qtbase:5 dev-qt/qtopengl:5
```

Build:
```bash
./build-tools.sh
```

See [tools/KonAnimator/DOCS.md](tools/KonAnimator/DOCS.md) for full usage.

### KonPaktor / konpak

Asset encryption and bundling. Packs your game's assets into AES-256 encrypted
`.konpak` archives for distribution. End users cannot extract the contents.

```bash
# CLI
konpak create game.konpak assets/ --pass mykey

# Bake the key into your release binary so the engine can decrypt at runtime
# but the key is never exposed to end users
target_compile_definitions(MyGame PRIVATE KON_PACK_KEY="mykey")
```

See [tools/KonPaktor/DOCS.md](tools/KonPaktor/DOCS.md) for full usage.

### KonScript

A statically-typed scripting language that compiles `.ks` files to C++.

```bash
# Install
cd tools/KonScript && ./build.sh && ./install.sh

# Compile and run
ksc game.ks

# Neovim syntax highlighting
./tools/KonScript/install-nvim-konscript.sh
```

See [tools/KonScript/DOCS.md](tools/KonScript/DOCS.md) for full usage.

---

## Running Tests

```bash
./build-test.sh
```

Headless unit tests run automatically. A visual test window then opens for
manual verification of rendering, collision, and animation.

---

## Documentation

| Document | Contents |
|---|---|
| [DOCS.md](DOCS.md) | Full engine API reference |
| [ROADMAP.md](ROADMAP.md) | What's done, what's next, release policy |
| [DEPENDENCIES.md](DEPENDENCIES.md) | All libraries and system deps |
| [MAINTAINERS.md](MAINTAINERS.md) | How to contribute, build, test, and release |
| [tools/KonAnimator/DOCS.md](tools/KonAnimator/DOCS.md) | KonAnimator usage |
| [tools/KonPaktor/DOCS.md](tools/KonPaktor/DOCS.md) | KonPaktor / konpak usage |
| [tools/KonScript/DOCS.md](tools/KonScript/DOCS.md) | KonScript language reference |

---

## Dependencies

All bundled as submodules — no manual installs needed beyond system GL/X11 libs.

| Library | Purpose |
|---|---|
| GLFW | Window creation and input |
| GLM | Math library |
| GLAD | OpenGL function loader |
| stb_image | Texture loading |
| stb_truetype | Font rendering |
| miniaudio | Audio playback and streaming |
| Qt5 | KonAnimator and KonPaktor GUI (optional) |
| zlib | Compression for .konpak |
| OpenSSL / BCrypt | AES-256 encryption for .konpak |

---

## Release Policy

Version numbers are only bumped after thorough bug testing on all supported platforms.
See [ROADMAP.md](ROADMAP.md) for the full plan and [MAINTAINERS.md](MAINTAINERS.md)
for the release process.

---

## License

MIT — free to use in personal, commercial, and open source projects.
