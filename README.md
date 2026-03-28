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
- Input system (keyboard, mouse, gamepad)
- Color system with presets (RED, GREEN, BLUE, WHITE...)
- Audio (sound effects + music streaming via miniaudio)
- Delta time + FPS capping
- VSync toggle
- Cross-platform (Linux + Windows)
- Node/scene system (Godot-style hierarchy, signals, parent pointers)
- Sprite2D with pivot/origin support
- Animation system (sprite sheet + keyframe, 16 easing curves)
- `DebugMode(true)` -- FPS overlay, mouse crosshair, auto collider visualization
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

**Linux (Arch/Gentoo):**
```bash
# Arch
sudo pacman -S mesa libx11 libxrandr libxi wayland wayland-protocols libxkbcommon libxinerama libxcursor
# Gentoo
emerge --ask x11-libs/libX11 x11-libs/libXrandr x11-libs/libXi media-libs/mesa
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

Or build individually:
```bash
cmake --build build --target KonAnimator
cmake --build build --target anim_compiler
```

### 6. Run the test suite (optional)
```bash
./build-test.sh
```

### 7. Install (optional)
Installing makes KonEngine available system-wide.

**Linux:**
```bash
./install.sh
```

**Windows:**
```bat
install.bat
```

## Using KonEngine in your project

### With CMake (recommended)
After installing, add this to your `CMakeLists.txt`:
```cmake
find_package(KonEngine REQUIRED)
target_link_libraries(YourGame PRIVATE KonEngine)
```

### Without installing (manual)
Copy or submodule the repo into your project:
```cmake
add_subdirectory(KonEngine)
target_link_libraries(YourGame PRIVATE KonEngine)
```

## Usage

### Basic template
```cpp
#include "KonEngine.hpp"

int main() {
    InitWindow(800, 600, "My Game");
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        ClearBackground(0.1f, 0.1f, 0.1f);

        // Your game code here

        Present();
        PollEvents();
    }

    return 0;
}
```

### Node/scene system
```cpp
#include "KonEngine.hpp"

int main() {
    InitWindow(800, 600, "My Game");
    SetTargetFPS(60);

    Scene scene;

    auto* player = scene.Add<Sprite2D>("player");
    player->x = 400;
    player->y = 300;

    Texture sheet = LoadTexture("player.png");
    player->SetTexture(sheet);

    while (!WindowShouldClose()) {
        ClearBackground(0.1f, 0.1f, 0.1f);
        scene.Update(GetDeltaTime());
        scene.Draw();
        Present();
        PollEvents();
    }

    return 0;
}
```

### Animation
```cpp
#include "KonEngine.hpp"

int main() {
    InitWindow(800, 600, "My Game");
    SetTargetFPS(60);

    Scene scene;

    auto* player = scene.Add<Sprite2D>("player");
    player->x = 400;
    player->y = 300;

    Texture sheet = LoadTexture("player.png");
    player->SetTexture(sheet);

    // AnimationPlayer auto-detects the parent Sprite2D -- no manual setup needed
    auto* anim = player->AddChild<AnimationPlayer>("anim");
    anim->LoadFromFile("player.konani");
    anim->Play("idle");

    while (!WindowShouldClose()) {
        ClearBackground(0.1f, 0.1f, 0.1f);
        scene.Update(GetDeltaTime());
        scene.Draw();
        Present();
        PollEvents();
    }

    return 0;
}
```

### Collision with signals
```cpp
#include "KonEngine.hpp"

int main() {
    InitWindow(800, 600, "My Game");
    SetTargetFPS(60);

    Scene scene;

    auto* player = scene.Add<Collider2D>("player");
    player->x = 100; player->y = 300;
    player->width = 32; player->height = 64;

    auto* wall = scene.Add<Collider2D>("wall");
    wall->x = 400; wall->y = 300;
    wall->width = 32; wall->height = 200;

    player->Connect("on_collision_enter", [](Collider2D* other) {
        std::cout << "Hit: " << other->name << "\n";
    });
    player->Connect("on_collision_exit", [](Collider2D* other) {
        std::cout << "Left: " << other->name << "\n";
    });

    Camera2D cam(400, 300, 1.0f, 0.0f);

    while (!WindowShouldClose()) {
        ClearBackground(0.1f, 0.1f, 0.1f);

        if (IsKeyDown(Key::Right)) player->x += 200.0f * GetDeltaTime();
        if (IsKeyDown(Key::Left))  player->x -= 200.0f * GetDeltaTime();

        BeginCamera2D(cam);
        scene.Update(GetDeltaTime());
        scene.Draw();
        EndCamera2D();

        Present();
        PollEvents();
    }

    return 0;
}
```

### Debug mode
```cpp
DebugMode(true); // call before or after InitWindow
```

Enables a red border around the window, mouse crosshair, FPS printed to stdout
every second, and automatic collider outline drawing for every `Collider2D` in
the active scene.

## KonAnimator

KonAnimator is the visual animation editor for KonEngine. Open a spritesheet,
draw frame rectangles, set durations, add keyframe tracks, preview live, and
export directly to `.konani`.

```bash
./build/tools/KonAnimator/KonAnimator
```

**Controls in the preview panel:**
- Drag -- pan
- Scroll -- zoom
- Double-click -- reset view
- F -- fullscreen

## Anim Compiler

The `anim_compiler` tool compiles `.anim` text files into `.konani` binaries.

**GUI mode:**
```bash
./build/anim_compiler
```

**CLI mode:**
```bash
./build/anim_compiler player.anim             # outputs player.konani
./build/anim_compiler player.anim out.konani
```

---

### Writing .anim files

```
anim idle loop
  display 32 32 1.0        # display width, height, scale
  frame 0 0 32 32 0.15     # srcX srcY srcW srcH duration
  frame 32 0 32 32 0.15
  frame 64 0 32 32 0.15
end

anim pop_in
  track scaleX 0.0 0.0 easeinoutback
  track scaleX 0.4 1.0 easeinoutback
  track scaleY 0.0 0.0 easeinoutback
  track scaleY 0.4 1.0 easeinoutback
  track alpha  0.0 0.0 easeout
  track alpha  0.3 1.0 easeout
end
```

**Animatable properties:** `x`, `y`, `scaleX`, `scaleY`, `rotation`, `alpha`

**Easing curves:**

| Curve | Feel |
|---|---|
| `linear` | Constant speed |
| `easein` / `easeout` / `easeinout` | Smooth acceleration |
| `easeincubic` / `easeoutcubic` / `easeinoutcubic` | Stronger |
| `easeinelastic` / `easeoutelastic` / `easeinoutelastic` | Springy |
| `easeinbounce` / `easeoutbounce` / `easeinoutbounce` | Bouncy |
| `easeinback` / `easeoutback` / `easeinoutback` | Slight overshoot |

Lines starting with `#` are comments.

## License
MIT -- free to use in commercial and open source projects.
