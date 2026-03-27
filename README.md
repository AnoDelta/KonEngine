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
- Collision detection (AABB, circle, circle vs rectangle)
- Input system (keyboard, mouse, gamepad)
- Color system with presets (RED, GREEN, BLUE, WHITE...)
- Audio (sound effects + music streaming via miniaudio)
- Delta time + FPS capping
- VSync toggle
- Cross-platform (Linux + Windows)

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
sudo pacman -S mesa libx11 libxrandr libxi \
  wayland wayland-protocols libxkbcommon libxinerama libxcursor
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

### 4. Install (optional)
Installing makes KonEngine available system-wide so you can use it in your own projects without copying files manually.

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
Copy the `src/` folder into your project and add it as a subdirectory in CMake:
```cmake
add_subdirectory(KonEngine)
target_link_libraries(YourGame PRIVATE KonEngine)
```

## Usage

```cpp
#include <KonEngine>

int main() {
    InitWindow(800, 600, "My Game");
    SetTargetFPS(60);

    Camera2D cam(400, 300, 1.0f, 0.0f);

    Rectangle player(100, 100, 64, 64);
    Rectangle wall(300, 200, 128, 128);

    while (!WindowShouldClose()) {
        ClearBackground(0.1f, 0.1f, 0.1f);

        if (IsKeyDown(Key::Right)) player.x += 3;
        if (IsKeyDown(Key::Left))  player.x -= 3;

        if (CheckCollisionRecs(player, wall))
            DrawText("Collision!", 10, 10, RED);

        BeginCamera2D(cam);
            DrawRectangle(player.x, player.y, player.width, player.height, GREEN);
            DrawRectangle(wall.x, wall.y, wall.width, wall.height, RED);
        EndCamera2D();

        Present();
        PollEvents();
    }

    return 0;
}
```

## Template

```cpp
#include <KonEngine>

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

## License
MIT — free to use in commercial and open source projects.
