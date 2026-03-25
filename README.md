# KonEngine

A lightweight game engine written in C++, built for old and low-end machines.

Made primarily for personal use and for friends — I wanted something simple and fast 
that fits my workflow when making games, without the overhead of larger engines.

Heavily inspired by Raylib's simplicity and API style (as of now).

## Features
- Simple Raylib-style API
- OpenGL rendering (rectangles, circles, lines)
- Built-in delta time and FPS capping
- Cross-platform (Linux, Windows)

## Building
### Dependencies
See [DEPENDENCIES.md](DEPENDENCIES.md)

### Linux
```bash
./setup.sh
./build.sh
```

### Windows
```bat
setup.bat
build.bat
```

## Usage
```cpp
#include "KonEngine.hpp"

int main() {
    InitWindow(900, 800, "My Game");
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        ClearBackground(0.2f, 0.0f, 0.0f);
        DrawRectangle(100, 100, 200, 200, 1.0f, 0.0f, 0.0f);
        Present();
        PollEvents();
    }
}
```

## License
MIT — free to use in commercial and open source projects.
