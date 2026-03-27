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
- Node/scene system (Godot-style hierarchy, signals)
- Sprite2D with pivot/origin support
- Animation system (sprite sheet + keyframe, 16 easing curves)
- `anim_compiler` tool with GUI — compiles `.anim` → `.konani`

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

### Basic template
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

### Node/scene system
```cpp
#include <KonEngine>

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
#include <KonEngine>

int main() {
    InitWindow(800, 600, "My Game");
    SetTargetFPS(60);

    Scene scene;

    auto* player = scene.Add<Sprite2D>("player");
    player->x = 400;
    player->y = 300;

    Texture sheet = LoadTexture("player.png");
    player->SetTexture(sheet);

    auto* anim = player->AddChild<AnimationPlayer>("anim");
    anim->target = player;
    anim->node   = player;
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

### Collision
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

## Anim Compiler

The `anim_compiler` tool is how you create animations for KonEngine. You describe your
animation in a simple `.anim` text file, then compile it into a `.konani` binary that
the engine loads at runtime.

**Build the tool:**
```bash
cmake --build build --target=anim_compiler
```

**Run with no arguments to open the GUI:**
```bash
./anim_compiler
```

**Or use the command line:**
```bash
./anim_compiler player.anim        # outputs player.konani automatically
./anim_compiler player.anim out.konani
```

---

### Writing .anim files

An `.anim` file can contain as many animations as you want, one after another.
There are two types of animation: **sprite sheet** and **keyframe**.

---

#### Sprite sheet animations

Use this when your character has a sprite sheet — a single image where each frame
of the animation is laid out in a row.

```
anim idle loop
  frame 0 0 32 32 0.15
  frame 32 0 32 32 0.15
  frame 64 0 32 32 0.15
end
```

**Breaking it down:**

- `anim idle loop` — start an animation called `idle`. The word `loop` makes it repeat.
  Leave it out if you only want it to play once (e.g. a death animation).
- `frame 0 0 32 32 0.15` — one frame of the animation. The numbers are:
  - `0 0` — where on the sprite sheet this frame starts (X, Y in pixels from the top-left)
  - `32 32` — the size of the frame (width, height in pixels)
  - `0.15` — how long to show this frame (in seconds). `0.15` is about 6-7 frames per second.
- `end` — closes the animation block.

So for a sprite sheet where your frames are 32×32 pixels and laid out in a horizontal row,
the first frame starts at X=0, the second at X=32, the third at X=64, and so on.

---

#### Keyframe animations

Use this when you want to animate a property of a node over time — like making a sprite
pop into view, slide across the screen, or fade out.

```
anim pop_in
  track scaleX 0.0 0.0 easeinoutback
  track scaleX 0.4 1.0 easeinoutback
  track scaleY 0.0 0.0 easeinoutback
  track scaleY 0.4 1.0 easeinoutback
  track alpha  0.0 0.0 easeout
  track alpha  0.3 1.0 easeout
end
```

**Breaking it down:**

- `anim pop_in` — start an animation called `pop_in`. No `loop` so it plays once.
- `track scaleX 0.0 0.0 easeinoutback` — a keyframe on the `scaleX` track. The numbers are:
  - `scaleX` — which property to animate. Options: `x`, `y`, `scaleX`, `scaleY`, `rotation`, `alpha`
  - `0.0` — the time this keyframe is at (in seconds)
  - `0.0` — the value at this time (`scaleX` of 0 = invisible, 1 = normal size)
  - `easeinoutback` — the easing curve (how it moves from this keyframe to the next)
- You need at least two keyframes per track — a start and an end.

**What the example does:** at time 0 the sprite is scaled to 0 (invisible), and by time 0.4
seconds it scales up to full size with a slight overshoot (the `back` in `easeinoutback`).
The alpha does the same thing but finishes a bit earlier at 0.3 seconds.

---

#### Easing curves

The curve controls how a value moves from one keyframe to the next.

| Curve | What it feels like |
|---|---|
| `linear` | Constant speed, robotic |
| `easein` | Starts slow, ends fast |
| `easeout` | Starts fast, ends slow |
| `easeinout` | Slow → fast → slow, smooth |
| `easeincubic` / `easeoutcubic` / `easeinoutcubic` | Same as above but stronger |
| `easeinelastic` / `easeoutelastic` / `easeinoutelastic` | Springy, bounces past the target |
| `easeinbounce` / `easeoutbounce` / `easeinoutbounce` | Bounces like a ball hitting the floor |
| `easeinback` / `easeoutback` / `easeinoutback` | Slight overshoot past the target |

When in doubt, `easeinout` or `easeinoutback` look good for most UI animations.
`easeout` is good for things sliding into place. `linear` is good for looping things
like a spinning object.

---

#### Combining both types

One `.anim` file can have both sprite sheet and keyframe animations:

```
anim idle loop
  frame 0 0 32 32 0.12
  frame 32 0 32 32 0.12
  frame 64 0 32 32 0.12
  frame 96 0 32 32 0.12
end

anim die
  frame 128 0 32 32 0.1
  frame 160 0 32 32 0.1
  frame 192 0 32 32 0.2
  track alpha 0.0 1.0 linear
  track alpha 0.5 0.0 linear
end
```

Lines starting with `#` are comments and are ignored by the compiler.

## License
MIT — free to use in commercial and open source projects.
