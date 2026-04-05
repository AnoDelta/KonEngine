# KonEngine Documentation

## Table of Contents

1. [Getting Started](#getting-started)
2. [KonScript Language](#konscript-language)
3. [Node Types](#node-types)
4. [Animation System](#animation-system)
5. [Physics & Collision](#physics--collision)
6. [Asset Packing](#asset-packing-konpak)
7. [Window & Rendering](#window--rendering)
8. [Using C++ Directly](#using-c-directly)
9. [Cross-Compilation](#cross-compilation)
10. [Editor (KonEditor)](#editor-koneditor)

---

## Getting Started

### Install Dependencies (Linux)

```bash
# Ubuntu/Debian
sudo apt install build-essential cmake libgl-dev libx11-dev libxrandr-dev libxi-dev libxinerama-dev libxcursor-dev

# Gentoo
emerge -av dev-util/cmake media-libs/mesa x11-libs/libX11 x11-libs/libXrandr
```

### Build KonScript Compiler

```bash
cd tools/KonScript
./build.sh          # builds self-hosted compiler
sudo ./install.sh   # installs to /usr/local/bin/konscript
```

### Build Engine Library (for KonScript games)

```bash
cd tools/KonScript
./build-engine-lib.sh            # builds libKonEngine.a for linux64
./build-engine-lib.sh --windows  # also builds for windows64
```

### Hello World

```ks
#include <engine>

func main() -> I32 {
    InitWindow(800, 600, "Hello World");
    SetTargetFPS(60);

    while !WindowShouldClose() {
        ClearBackground(0.1, 0.1, 0.15);
        DrawRectangle(350, 250, 100, 100, 0.2, 0.8, 1.0, 1.0);
        Present();
        PollEvents();
    }
    return 0;
}
```

```bash
konscript hello.ks && ./hello
```

---

## KonScript Language

### Types

| Type | Description | Example |
|------|-------------|---------|
| `I32` | 32-bit integer | `let x: I32 = 42;` |
| `F64` | 64-bit float | `let y: F64 = 3.14;` |
| `Bool` | Boolean | `let b: Bool = true;` |
| `Str` | String | `let s: Str = "hello";` |
| `Vec2` | 2D vector | `let v: Vec2 = Vec2(1.0, 0.0);` |

### Variables

```ks
let x: I32 = 10;           // immutable
let mut y: F64 = 3.14;     // mutable
const PI: F64 = 3.14159;   // compile-time constant
```

### Functions

```ks
func add(a: I32, b: I32) -> I32 {
    return a + b;
}

func greet(name: Str) {
    Print("Hello, ", name, "!");
}
```

### Nodes

Nodes are the building blocks of games. They have a type, a parent class, and lifecycle methods.

```ks
node Player : Sprite2D {
    let mut speed: F64 = 200;

    func Ready() {
        x = 100;
        y = 300;
        width = 32;
        height = 48;
    }

    func Update(dt: F64) {
        if IsKeyDown(KEY_D) { x = x + speed * dt; }
        if IsKeyDown(KEY_A) { x = x - speed * dt; }
    }
}
```

### Scene Setup

```ks
func main() -> I32 {
    InitWindow(800, 600, "My Game");
    SetTargetFPS(60);

    let mut scene: Scene = Scene();
    let mut player: Player = scene.add(Player, "player");

    while !WindowShouldClose() {
        scene.update(GetDeltaTime());
        ClearBackground(0.1, 0.1, 0.15);
        scene.draw();
        Present();
        PollEvents();
    }
    return 0;
}
```

---

## Node Types

### Sprite2D

Textured 2D sprite with position, scale, rotation, and tint.

```ks
node Enemy : Sprite2D {
    func Ready() {
        x = 500; y = 300;
        width = 64; height = 64;
    }
}

// In main():
let mut tex: Texture = LoadTexture("enemy.png");
let mut enemy: Enemy = scene.add(Enemy, "enemy");
enemy.SetTexture(tex);
```

**Properties:** `x`, `y`, `width`, `height`, `scaleX`, `scaleY`, `rotation`, `originX`, `originY`, `tint`

### StaticBody2D

Immovable collision body for walls, floors, platforms.

```ks
node Wall : StaticBody2D {
    func Ready() {
        AddCollider(200, 32);  // width, height
        x = 400; y = 568;
    }
}
```

### KinematicBody2D

Player-controlled body with collision response. Use `MoveAndCollide(dx, dy)` to move without passing through walls.

```ks
node Player : KinematicBody2D {
    let mut col: Collider2D = this.add(Collider2D, "col");
    let mut speed: F64 = 200;

    func Ready() {
        col.width = 32; col.height = 48;
    }

    func Update(dt: F64) {
        let mut dx: F64 = 0;
        let mut dy: F64 = 0;
        if IsKeyDown(KEY_D) { dx = speed * dt; }
        if IsKeyDown(KEY_A) { dx = -speed * dt; }
        MoveAndCollide(dx, dy);
    }
}
```

### RigidBody2D

Physics body with velocity and gravity. Automatically resolves collisions with static bodies.

**Properties:** `velocity` (Vec2), `gravity` (float, default 980)

### Collider2D

Axis-aligned bounding box for collision detection.

```ks
let mut col: Collider2D = this.add(Collider2D, "col");
col.width = 32;
col.height = 32;
```

**Callbacks:**
```ks
func OnCollisionEnter(other: Collider2D) { Print("Hit!"); }
func OnCollisionExit(other: Collider2D)  { Print("Left"); }
```

### CameraNode2D

Camera with zoom for viewport control.

### AnimationPlayer

Plays keyframe animations. Tracks modify properties as rendering overlays:
- `x`, `y` -- added to position
- `scaleX`, `scaleY` -- multiplied to scale
- `rotation` -- added to rotation
- `alpha` -- multiplied to tint alpha

```ks
let mut anim: AnimationPlayer = this.add(AnimationPlayer, "anim");
anim.LoadFromFile("walk.konani");
anim.Play("walk");
```

---

## Animation System

### .anim File Format

```
spritesheet assets/player.png

anim idle loop
    display 32 32 1.0
    frame 0 0 32 32 0.2
    frame 32 0 32 32 0.2
    track scaleX 0.0 1.0 linear
    track scaleX 0.5 1.1 easein
    track scaleX 1.0 1.0 easeout
end
```

**Track format:** `track <property> <time> <value> <curve>`

**Curves:** `linear`, `easein`, `easeout`, `easeinout`, `easeincubic`, `easeoutcubic`, `easeinelastic`, `easeoutelastic`, `easeinbounce`, `easeoutbounce`, `easeinback`, `easeoutback`

The curve on a keyframe controls interpolation FROM that keyframe TO the next. The last keyframe's curve has no effect.

### Compiling

```bash
anim_compiler player.anim    # outputs player.konani
```

---

## Physics & Collision

1. Add `Collider2D` children to your nodes
2. Use `StaticBody2D` for walls (immovable)
3. Use `KinematicBody2D` for players (`MoveAndCollide`)
4. Use `RigidBody2D` for physics objects (auto velocity/gravity)

See `examples/platformer.ks` for a complete example.

---

## Asset Packing (KonPak)

```bash
# Create encrypted pack
konpak create game.konpak assets/* --pass mypassword

# Load in game
AssetManager.init("game.konpak", "mypassword");
let mut tex: Texture = LoadTexture("sprite.png");  // reads from pack

# Build with pack support
konscript game.ks --pack
```

---

## Window & Rendering

```ks
InitWindow(800, 600, "Game");          // fixed size
InitWindow(800, 600, "Game", true);    // resizable with letterboxing
SetTargetFPS(60);                      // 0 = uncapped
SetVsync(true);
DebugMode(true);                       // FPS overlay, collider outlines
```

Letterbox scaling: resizable windows maintain aspect ratio with black bars. Game coordinates stay the same.

---

## Using C++ Directly

```cpp
#include "KonEngine.hpp"

class Player : public Sprite2D {
public:
    Player(const std::string& name = "Player") : Sprite2D(name) {}
    void Update(float dt) override {
        if (IsKeyDown(Key::D)) x += 200 * dt;
    }
};

int main() {
    InitWindow(800, 600, "C++ Game");
    Scene scene;
    auto* p = scene.Add<Player>("p");
    while (!WindowShouldClose()) {
        scene.Update(GetDeltaTime());
        ClearBackground(0.1f, 0.1f, 0.15f);
        scene.Draw();
        Present(); PollEvents();
    }
}
```

See `examples/cpp_example/` for CMake setup.

---

## Cross-Compilation

```bash
# Build Windows engine
cd tools/KonScript && ./build-engine-lib.sh --windows

# Compile game for Windows
konscript game.ks --target windows  # outputs game.exe
```

---

## Editor (KonEditor)

- **Project mode**: Open `.konproj`, edit scenes with viewport/inspector
- **Monolithic mode**: Open single `.ks` files directly
- **Project Settings**: window size, FPS, VSync, resizable, debug, KonPak
- **Build**: `Ctrl+B` build, `Ctrl+R` run, target: linux64/windows64
- **Animation**: Double-click `.anim` files to open KonAnimator

```bash
cd tools/KonEditor && ./build.sh && sudo ./install.sh
```
