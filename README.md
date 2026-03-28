# KonEngine
A lightweight 2D game engine written in C++, built for old and low-end machines.
Made primarily for personal use and for friends — I wanted something simple and fast
that fits my workflow when making games, without the overhead of larger engines.
Heavily inspired by Raylib's simplicity and API style, with plans for a full Godot-style editor.

## Roadmap
See [ROADMAP.md](ROADMAP.md) — project is still in early stages.

## Features
- Simple Raylib-style API
- OpenGL 2D rendering (rectangles, circles, lines, textures)
- Texture loading + sprite sheet support
- Text rendering with custom fonts or built-in default font
- Camera system (pan, zoom, rotation)
- Collision detection (AABB, circle, circle vs rectangle, SAT)
- CollisionWorld with enter/exit signals and layer/mask filtering
- Node/scene system (Godot-style hierarchy, signals, parent pointers, world transform propagation)
- Sprite2D with pivot/origin support
- Animation system (sprite sheet + keyframe, 16 easing curves)
- Input system (keyboard, mouse, gamepad)
- Color system with presets (RED, GREEN, BLUE, WHITE...)
- Audio (sound effects + music streaming via miniaudio)
- Delta time + FPS capping + GetTime() + GetFPS()
- VSync toggle
- Cross-platform (Linux + Windows)
- `DebugMode(true)` -- FPS overlay, mouse crosshair, auto collider visualization
- **KonScript** -- scripting language that compiles to C++ and links against KonEngine
- **KonAnimator** -- standalone Qt animation editor
- **anim_compiler** -- CLI + Qt GUI tool, compiles `.anim` to `.konani`
- Test suite (`./build-test.sh`)

## Getting Started

### 1. Clone the repository
```bash
git clone --recurse-submodules https://github.com/AnoDelta/KonEngine.git
```

### 2. Install system dependencies

**Linux (Ubuntu/Debian):**
```bash
sudo apt-get install -y libgl1-mesa-dev libx11-dev libxrandr-dev libxi-dev \
  libwayland-dev wayland-protocols libxkbcommon-dev libxinerama-dev libxcursor-dev
```

**Linux (Fedora):**
```bash
sudo dnf install -y mesa-libGL-devel libX11-devel libXrandr-devel libXi-devel \
  wayland-devel wayland-protocols-devel libxkbcommon-devel libXinerama-devel libXcursor-devel
```

**Linux (Arch):**
```bash
sudo pacman -S mesa libx11 libxrandr libxi wayland wayland-protocols libxkbcommon libxinerama libxcursor
```

**Linux (Gentoo):**
```bash
emerge --ask x11-libs/libX11 x11-libs/libXrandr x11-libs/libXi media-libs/mesa \
  dev-libs/wayland dev-libs/wayland-protocols x11-libs/libxkbcommon
```

**Windows:** No extra dependencies needed.

### 3. Install Qt5 (only needed for KonAnimator and anim_compiler)

**Linux (Ubuntu/Debian):**
```bash
sudo apt-get install -y qtbase5-dev libqt5opengl5-dev
```

**Linux (Fedora):**
```bash
sudo dnf install -y qt5-qtbase-devel
```

**Linux (Arch):**
```bash
sudo pacman -S qt5-base
```

**Windows:** Download from https://www.qt.io/download-open-source

### 4. Build the engine

**Linux:**
```bash
./build.sh
```

**Windows:**
```bat
build.bat
```

### 5. Build KonAnimator and anim_compiler (optional)

```bash
./build-tools.sh
```

### 6. Using KonScript (optional)

KonScript is a scripting language that compiles to C++ and links against KonEngine.
It lives in `tools/KonScript/`.

**Build and install the compiler:**
```bash
cd tools/KonScript
./build.sh
./install.sh   # installs konscript and ksc to /usr/local/bin
```

**Write a game in KonScript:**
```ks
#include <engine>

node Player : Node2D {
    let mut speed: F64 = 200.0;

    func Update(dt: F64) {
        if KeyDown(Key.D) { x += speed * dt; }
        if KeyDown(Key.A) { x -= speed * dt; }
    }

    func OnCollisionEnter(other: Collider2D) {
        Print("Hit: %s\n", other.name);
    }
}

func main() {
    InitWindow(800, 600, "My Game");
    SetTargetFPS(60);

    let scene: Scene = Scene();
    let player: Player = scene.add(Player, "player");
    let col: Collider2D = player.add(Collider2D, "player");
    col.width = 32.0;
    col.height = 32.0;
    scene.scan();

    while !WindowShouldClose() {
        let dt: F64 = GetDeltaTime();
        ClearBackground(0.1, 0.1, 0.1);
        scene.update(dt);
        scene.draw();
        Present();
        PollEvents();
    }
}
```

**Compile and run:**
```bash
ksc main.ks            # compile and run
ksc main.ks --keep     # keep the generated .cpp
ksc --compile main.ks  # compile only
ksc --check main.ks    # typecheck only
```

**Use with CMake (for larger projects):**
```cmake
add_subdirectory(KonEngine)
add_executable(MyGame)
target_link_libraries(MyGame PRIVATE KonEngine)
konscript_sources(MyGame src/main.ks)
```

## Project structure
```
KonEngine/
  src/
    node/         -- Node, Node2D, Sprite2D, Collider2D, Scene
    collision/    -- CollisionWorld, SAT detection
    renderer/     -- OpenGL renderer
    window/       -- GLFW window wrapper
    input/        -- keyboard, mouse, gamepad
    font/         -- font loading and text rendering
    audio/        -- miniaudio wrapper
    time/         -- delta time, FPS, GetTime()
    camera/       -- Camera2D
    animation/    -- AnimationPlayer, keyframe tracks, easing curves
    math/         -- Vector2
  libs/
    glfw/         -- window + input (bundled)
    glm/          -- math (bundled)
  tools/
    KonScript/    -- scripting language compiler
    KonAnimator/  -- animation editor (Qt)
    KonPaktor/    -- asset packer
```
