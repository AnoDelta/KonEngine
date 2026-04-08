# KonEngine

A lightweight 2D game engine written in C++17, built for old and low-end machines.
Simple, fast, and designed for making games without the overhead of larger engines.

Inspired by Raylib's simplicity and API style, with a Godot-style node system,
a custom scripting language (KonScript), and a full visual editor (KonEditor).

---

## Features

**Core Engine**
- Simple Raylib-style API -- `InitWindow`, `DrawRectangle`, `PlaySound`, done
- OpenGL 2D renderer -- rectangles, circles, lines, textures, sprite sheets
- Batched rendering -- quads, lines, and glyphs batched for performance
- Text rendering -- custom TTF fonts or built-in Inconsolata, cached at any size
- Camera system -- pan, zoom, rotation, smooth follow, clamping, screen shake
- Letterbox scaling -- design-resolution coordinates with automatic black bars
- Input -- keyboard, mouse, gamepad (multi-player), delta tracking
- Collision detection -- AABB, circle, SAT (convex polygons), layer/mask filtering
- CollisionWorld with enter/exit signals and automatic depenetration
- Physics nodes -- StaticBody2D (walls), KinematicBody2D (players), RigidBody2D (physics objects)
- Animation -- sprite sheet frame-by-frame + keyframe tracks with 16 easing curves
- Audio -- sound effects + music streaming via miniaudio (.wav, .ogg, .mp3)
- Node/scene system -- Godot-style hierarchy with parent pointers and virtual lifecycle
- Signal system -- lightweight callbacks for decoupled node communication
- UI system -- screen-space Button, Label, Panel with click detection and input blocking
- Tilemap system -- tile data storage, tileset rendering, coordinate conversion, click detection
- Isometric grid -- diamond-shaped tile support for isometric games
- `DebugMode(true)` -- FPS overlay, mouse crosshair, world grid, auto collider outlines
- Timer system -- frame-rate independent timers with lambda callbacks, pause/resume/reset
- Color presets, Vector2 math, random number utilities, tile grid helpers
- Delta time, FPS cap, VSync toggle
- Cross-platform -- Linux and Windows

**KonScript**
- Statically-typed scripting language that compiles to C++
- Nodes, structs, classes, enums, generics, interfaces, closures
- Ternary operator, f-strings, nullable types, null coalescing
- Full engine API: rendering, input (keyboard + mouse + gamepad), audio, physics, camera, random, UI, tilemap
- CMake integration with `konscript_sources()`

**Tools**
- **KonEditor** -- Qt-based visual game editor with scene tree, viewport, inspector, and build system
- **KonAnimator** -- standalone Qt animation editor with timeline, live preview, and spritesheet support
- **anim_compiler** -- CLI/GUI tool that compiles `.anim` text files to `.konani` binary
- **KonPaktor / konpak** -- AES-256 encrypted asset packing and `.konpak` archive tool

---

## Quick Start

### 1. Clone

```bash
git clone --recurse-submodules https://github.com/AnoDelta/KonEngine.git
cd KonEngine
```

### 2. Install system dependencies

**Ubuntu/Debian:**
```bash
sudo apt-get install -y libgl1-mesa-dev libx11-dev libxrandr-dev libxi-dev \
  libwayland-dev wayland-protocols libxkbcommon-dev libxinerama-dev libxcursor-dev
```

**Fedora:**
```bash
sudo dnf install -y mesa-libGL-devel libX11-devel libXrandr-devel libXi-devel \
  wayland-devel wayland-protocols-devel libxkbcommon-devel libXinerama-devel libXcursor-devel
```

**Arch:**
```bash
sudo pacman -S mesa libx11 libxrandr libxi wayland wayland-protocols \
  libxkbcommon libxinerama libxcursor
```

**Windows:** No extra dependencies needed.

### 3. Build

```bash
make            # or: cmake -B build && cmake --build build -j$(nproc)
sudo make install   # installs to /usr/local
```

### 4. Write your first game

**In C++:**
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

**In KonScript:**
```ks
#include <engine>

node Player : Node2D {
    let mut speed: F64 = 200.0;

    func Update(dt: F64) {
        if KeyDown(Key.D) { x += speed * dt; }
        if KeyDown(Key.A) { x -= speed * dt; }
        if KeyDown(Key.W) { y -= speed * dt; }
        if KeyDown(Key.S) { y += speed * dt; }
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
        DrawText("WASD to move", 10, 10, WHITE);
        Present();
        PollEvents();
    }
}
```

Compile and run KonScript:
```bash
cd tools/KonScript && ./build.sh && ./install.sh
ksc game.ks
```

### 5. Link the engine from CMake

```cmake
cmake_minimum_required(VERSION 3.20)
project(MyGame)

add_subdirectory(KonEngine)
add_executable(MyGame src/main.cpp)
target_link_libraries(MyGame PRIVATE KonEngine)

# Optional: compile KonScript files
konscript_sources(MyGame src/main.ks)
```

---

## Project Structure

```
KonEngine/
  src/
    KonEngine.hpp          # Single include header
    window/                # Window creation, letterbox scaling, debug mode
    renderer/              # OpenGL 2D renderer (rectangles, circles, lines, textures)
    input/                 # Keyboard, mouse, gamepad input
    camera/                # Camera2D with follow, clamp, shake
    collision/             # CollisionWorld, SAT, AABB, circle overlap
    node/                  # Node, Node2D, Sprite2D, Collider2D, Scene, bodies
    animation/             # Animation clips, keyframe tracks, AnimationPlayer
    audio/                 # Sound and music via miniaudio
    font/                  # TTF font rendering with glyph caching
    ui/                    # Screen-space UI (Button, Label, Panel)
    color/                 # Color struct and presets
    math/                  # Vector2, Random
    tilemap/               # Tilemap, TileGrid, IsometricGrid
    time/                  # Delta time, FPS cap
    asset_manager.*        # AssetManager for loose files or .konpak packs
  tools/
    KonScript/             # KonScript compiler (lexer, parser, typechecker, codegen)
    KonEditor/             # Visual game editor (Qt)
    KonAnimator/           # Animation editor (Qt)
    KonPaktor/             # Asset packer GUI + CLI
  libs/
    glfw/                  # Window/input (submodule)
    glm/                   # Math library (submodule)
  examples/                # Example games in KonScript and C++
    hello_world.ks         # Basic window and drawing
    platformer.ks          # Platformer with KinematicBody2D
    physics_test.ks        # Physics: gravity, jumping, collision, platforms
    tilemap_test.ks        # Tilemap: tile placement, grid, camera, click detection
    ui_test.ks             # UI: buttons, labels, panels, click handlers, signals
    pong.ks                # Pong game
    cpp_example/           # C++ example with CMake
  tests/                   # Engine test suite
```

---

## Tools

| Tool | Description | Build |
|---|---|---|
| **KonScript** | Scripting language compiler | `cd tools/KonScript && ./build.sh && ./install.sh` |
| **KonEditor** | Visual game editor | `cd tools/KonEditor && ./build.sh` |
| **KonAnimator** | Animation editor | `make tools` or `./build-tools.sh` |
| **anim_compiler** | .anim to .konani compiler | Built with KonAnimator |
| **KonPaktor** | Asset packer (GUI) | `cd tools/KonPaktor && ./build.sh` |
| **konpak** | Asset packer (CLI) | Built with KonPaktor |

---

## Documentation

Start with **[DOCS.md](DOCS.md)** for a full guide to the engine, or **[KonScript DOCS](tools/KonScript/DOCS.md)** if you're using the scripting language.

| Document | Contents |
|---|---|
| [DOCS.md](DOCS.md) | Full engine guide: rendering, input, camera, physics, collision, audio, animation, UI, timers, tilemaps -- with KonScript and C++ examples |
| [tools/KonScript/DOCS.md](tools/KonScript/DOCS.md) | KonScript language reference: types, variables, functions, lambdas, nodes, structs, enums, control flow, and full engine API listing |
| [ROADMAP.md](ROADMAP.md) | What's done, what's next, release policy |
| [DEPENDENCIES.md](DEPENDENCIES.md) | All libraries and system dependencies |
| [MAINTAINERS.md](MAINTAINERS.md) | How to contribute, build, test, and release |
| [tools/KonAnimator/DOCS.md](tools/KonAnimator/DOCS.md) | KonAnimator usage |
| [tools/KonPaktor/DOCS.md](tools/KonPaktor/DOCS.md) | KonPaktor / konpak usage |
| [examples/](examples/) | Example games: hello world, pong, platformer, physics test, tilemap test, C++ |

---

## Dependencies

All bundled as submodules -- no manual installs needed beyond system GL/X11 libs.

| Library | Purpose |
|---|---|
| GLFW | Window creation and input |
| GLM | Math library |
| GLAD | OpenGL function loader |
| stb_image | Texture loading |
| stb_truetype | Font rendering |
| miniaudio | Audio playback and streaming |
| Qt5 | KonEditor, KonAnimator, KonPaktor (optional) |
| zlib | Compression for .konpak |
| OpenSSL / BCrypt | AES-256 encryption for .konpak |

---

## Cross-Compilation (Windows from Linux)

```bash
./build-windows.sh              # engine library
./build-windows.sh --tools      # engine + tools
```

Requires MXE. See [build-windows.sh](build-windows.sh) for setup instructions.

---

## Running Tests

```bash
make test
```

---

## License

MIT -- free to use in personal, commercial, and open source projects.
