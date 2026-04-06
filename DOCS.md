# KonEngine Documentation

> All examples below are shown in both **KonScript** and **C++**.

## Table of Contents

1. [Getting Started](#getting-started)
2. [Color System](#color-system)
3. [Window & Rendering](#window--rendering)
4. [Drawing Primitives](#drawing-primitives)
5. [Textures & Text](#textures--text)
6. [Input](#input)
7. [Camera](#camera)
8. [Audio](#audio)
9. [KonScript Language](#konscript-language)
10. [Node Types](#node-types)
11. [Physics & Collision](#physics--collision)
12. [Animation System](#animation-system)
13. [Asset Packing](#asset-packing-konpak)
14. [Cross-Compilation](#cross-compilation)
15. [Editor (KonEditor)](#editor-koneditor)

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

**KonScript:**
```ks
#include <engine>

func main() -> I32 {
    InitWindow(800, 600, "Hello World");
    SetTargetFPS(60);

    while !WindowShouldClose() {
        ClearBackground(BLACK);
        DrawRectangle(350, 250, 100, 100, CYAN);
        Present();
        PollEvents();
    }
    return 0;
}
```

```bash
konscript hello.ks && ./hello
```

**C++:**
```cpp
#include "KonEngine.hpp"

int main() {
    InitWindow(800, 600, "Hello World");
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        ClearBackground(BLACK);                           // Color preset
        // ClearBackground(0.0f, 0.0f, 0.0f);            // or r, g, b
        DrawRectangle(350, 250, 100, 100, CYAN);
        Present();
        PollEvents();
    }
}
```

---

## Color System

Colors use float components (0.0 - 1.0). Both presets and custom colors work everywhere.

**Presets:** `RED`, `GREEN`, `BLUE`, `WHITE`, `BLACK`, `YELLOW`, `CYAN`, `MAGENTA`, `ORANGE`, `GRAY`, `BLANK`

**Custom color (C++):** `Color(0.2f, 0.5f, 0.8f, 1.0f)` or `Color(0.2f, 0.5f, 0.8f)` (alpha defaults to 1.0)

All drawing functions accept both `(r, g, b, a)` floats and a `Color` struct:
```cpp
ClearBackground(BLACK);                    // Color preset
ClearBackground(0.1f, 0.1f, 0.15f);       // r, g, b floats
DrawRectangle(10, 10, 50, 50, RED);        // Color preset
DrawRectangle(10, 10, 50, 50, 1, 0, 0, 1); // r, g, b, a floats
```

---

## Window & Rendering

**KonScript:**
```ks
InitWindow(800, 600, "Game");           // fixed size
InitWindow(800, 600, "Game", true);     // resizable with letterbox scaling
SetTargetFPS(60);                       // 0 = uncapped
SetVsync(true);
DebugMode(true);                        // FPS overlay, collider outlines, grid
```

**C++:**
```cpp
InitWindow(800, 600, "Game", true);     // resizable with letterboxing
SetTargetFPS(60);
SetVsync(true);
DebugMode(true);

int w = GetWindowWidth();               // design resolution (letterbox-aware)
int h = GetWindowHeight();
float dt = GetDeltaTime();
int fps = GetFPS();
float t = GetTime();                    // seconds since start
```

Letterbox scaling: resizable windows maintain aspect ratio with black bars. Game coordinates always match the design resolution passed to `InitWindow`.

---

## Drawing Primitives

**KonScript:**
```ks
ClearBackground(BLACK);
DrawRectangle(10, 20, 100, 50, RED);
DrawCircle(200, 200, 30, YELLOW);
DrawLine(0, 0, 400, 300, GREEN);
```

**C++:**
```cpp
ClearBackground(BLACK);
DrawRectangle(10, 20, 100, 50, RED);
DrawRectangle(10, 20, 100, 50, 1.0f, 0.0f, 0.0f, 0.5f);  // semi-transparent
DrawCircle(200, 200, 30, YELLOW);
DrawLine(0, 0, 400, 300, GREEN);
```

---

## Textures & Text

**KonScript:**
```ks
let mut tex: Texture = LoadTexture("sprite.png");
DrawTexture(tex, 100, 100, 64, 64);

// Sprite sheet region (UV coords 0-1)
DrawTextureRec(tex, 100, 100, 64, 64, 0.0, 0.0, 0.5, 0.5);
```

**C++:**
```cpp
Texture tex = LoadTexture("sprite.png");
DrawTexture(tex, 100, 100, 64, 64);
DrawTexture(tex, 100, 100, 64, 64, CYAN);    // tinted

// Text rendering
DrawText("Hello!", 10, 10, WHITE);
DrawText("Big text", 10, 40, 32, YELLOW);     // fontSize 32
DrawTextF(10, 70, RED, "Score: %d", score);   // printf-style

// Custom font
Font myFont = LoadFont("custom.ttf", 24);
DrawText(myFont, "Custom!", 10, 100, GREEN);
UnloadFont(myFont);

UnloadTexture(tex);
```

---

## Input

**KonScript:**
```ks
if IsKeyDown(KEY_D) { x = x + speed * dt; }
if IsKeyPressed(KEY_SPACE) { jump(); }
if IsKeyReleased(KEY_E) { interact(); }
```

**C++:**
```cpp
// Keyboard
if (IsKeyDown(Key::D))        x += speed * dt;
if (IsKeyPressed(Key::Space)) jump();
if (IsKeyReleased(Key::Escape)) break;

// Mouse
float mx = GetMouseX(), my = GetMouseY();
float gmx = GetGameMouseX();              // letterbox-corrected
if (IsMouseButtonPressed(Mouse::Left))  shoot();
float scroll = GetMouseScroll();          // wheel delta

// World-space mouse (with camera)
float wmx = GetWorldMouseX(cam);
float wmy = GetWorldMouseY(cam);

// Gamepad
if (IsGamepadConnected(0)) {
    float lx = GetGamepadAxis(0, Gamepad::LeftX);
    if (IsGamepadButtonPressed(0, Gamepad::A)) jump();
}
```

---

## Camera

**KonScript:**
```ks
let mut cam: Camera2D = Camera2D(400, 300, 1.0, 0.0);
BeginCamera2D(cam);
    // everything here is in world space
    scene.draw();
EndCamera2D();
// everything here is in screen space (HUD)
```

**C++:**
```cpp
Camera2D cam(400, 300, 1.0f);

// Smooth follow
Camera2DFollow(cam, player->x, player->y, 0.4f, dt);

// Clamp to world bounds
Camera2DClamp(cam, 0, 0, worldW, worldH, GetWindowWidth(), GetWindowHeight());

// Screen shake
Camera2DShake(cam, shakeMagnitude);
shakeMagnitude *= 0.9f;  // decay

// Lerp between cameras
Camera2D result = Camera2DLerp(camA, camB, t);

BeginCamera2D(cam);
    scene.Draw();
EndCamera2D();
```

---

## Audio

**KonScript:**
```ks
let mut sfx: Sound = LoadSound("hit.wav");
PlaySound(sfx);

let mut bgm: Music = LoadMusic("song.mp3");
PlayMusic(bgm);
```

**C++:**
```cpp
Sound sfx = LoadSound("hit.wav");
SetSoundVolume(sfx, 0.5f);
PlaySound(sfx);

Music bgm = LoadMusic("theme.mp3");
SetMusicVolume(bgm, 0.8f);
SetMusicLooping(bgm, true);
PlayMusic(bgm);

// Call every frame to stream music
UpdateMusic(bgm);

SetMasterVolume(0.7f);

UnloadSound(sfx);
UnloadMusic(bgm);
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

**KonScript:**
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

**C++:**
```cpp
class Enemy : public Sprite2D {
public:
    Enemy(const std::string& name) : Sprite2D(name) {}
    void Ready() override {
        x = 500; y = 300;
        width = 64; height = 64;
        tint = RED;
    }
};

// Usage:
Texture tex = LoadTexture("enemy.png");
auto* enemy = scene.Add<Enemy>("enemy");
enemy->SetTexture(tex);
```

**Properties:** `x`, `y`, `width`, `height`, `scaleX`, `scaleY`, `rotation`, `originX`, `originY`, `tint`

### CameraNode2D

Camera node that auto-applies to `Scene::Draw()`.

**KonScript:**
```ks
node GameCamera : CameraNode2D {
    func Ready() { zoom = 1.5; current = true; }
}
```

**C++:**
```cpp
auto* cam = scene.Add<CameraNode2D>("cam");
cam->x = player->x; cam->y = player->y;
cam->zoom = 1.5f;
cam->current = true;  // Scene::Draw() uses this camera automatically
```

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

Body types: `StaticBody2D` (walls), `KinematicBody2D` (player-controlled), `RigidBody2D` (gravity-driven).

### StaticBody2D -- Immovable walls and platforms

**KonScript:**
```ks
node Wall : StaticBody2D {
    func Ready() {
        AddCollider(200, 32);
        x = 400; y = 568;
    }

    func Draw() {
        DrawRectangle(x - 100, y - 16, 200, 32, GRAY);
    }
}
```

**C++:**
```cpp
class Wall : public StaticBody2D {
public:
    float w, h;
    Wall(const std::string& name, float w = 200, float h = 32)
        : StaticBody2D(name), w(w), h(h) {}

    void Ready() override {
        AddCollider(w, h);         // auto-generates name
        StaticBody2D::Ready();
    }

    void Draw() override {
        DrawRectangle(DrawX(w), DrawY(h), w, h, GRAY);
    }
};

// Usage:
auto* floor = scene.Add<Wall>("floor", 800, 20);
floor->x = 400; floor->y = 580;
```

### KinematicBody2D -- Player with wall collision

**KonScript:**
```ks
node Player : KinematicBody2D {
    let mut speed: F64 = 200;

    func Ready() {
        AddCollider(32, 48);
        x = 400; y = 300;
    }

    func Update(dt: F64) {
        let mut dx: F64 = 0;
        let mut dy: F64 = 0;
        if IsKeyDown(KEY_D) { dx = speed * dt; }
        if IsKeyDown(KEY_A) { dx = -speed * dt; }
        if IsKeyDown(KEY_S) { dy = speed * dt; }
        if IsKeyDown(KEY_W) { dy = -speed * dt; }
        MoveAndCollide(dx, dy);
    }

    func OnCollisionEnter(other: Collider2D) { Print("Hit!"); }
    func OnCollisionExit(other: Collider2D)  { Print("Left"); }
}
```

**C++:**
```cpp
class Player : public KinematicBody2D {
public:
    float speed = 200.0f;
    Player(const std::string& name) : KinematicBody2D(name) {}

    void Ready() override {
        AddCollider(32, 48);  // no name needed, auto-generates
        KinematicBody2D::Ready();
    }

    void Update(float dt) override {
        float dx = 0, dy = 0;
        if (IsKeyDown(Key::D)) dx += speed * dt;
        if (IsKeyDown(Key::A)) dx -= speed * dt;
        if (IsKeyDown(Key::S)) dy += speed * dt;
        if (IsKeyDown(Key::W)) dy -= speed * dt;
        MoveAndCollide(dx, dy);   // auto-resolves against static bodies
    }

    void OnCollisionEnter(Collider2D* other) override { /* hit */ }
    void OnCollisionExit(Collider2D* other) override  { /* left */ }

    void Draw() override {
        DrawRectangle(DrawX(32), DrawY(48), 32, 48, CYAN);
    }
};
```

### RigidBody2D -- Gravity-driven physics objects

Physics runs automatically in `Update()`. Gravity, velocity, and collision with static bodies are all handled.

**KonScript:**
```ks
node Crate : RigidBody2D {
    func Ready() {
        AddCollider(32, 32);
        gravity = 600;
    }

    func Update(dt: F64) {
        if onFloor {
            // on the ground
        }
    }
}
```

**C++:**
```cpp
class Crate : public RigidBody2D {
public:
    Crate(const std::string& name) : RigidBody2D(name) {}

    void Ready() override {
        AddCollider(32, 32);
        gravity = 600.0f;
        RigidBody2D::Ready();
    }

    // Physics runs automatically in Update() -- just override for custom logic
    void Update(float dt) override {
        RigidBody2D::Update(dt);  // apply gravity, move, resolve collisions
        if (onFloor) { /* landed */ }
    }

    void Draw() override {
        DrawRectangle(DrawX(32), DrawY(32), 32, 32, ORANGE);
    }
};
```

**Properties:** `velocity` (Vector2), `gravity` (float, default 980), `onFloor` (bool)

### Collider2D

Collision shapes: `Rectangle` (default), `Circle`, `Custom` (polygon).

```cpp
auto* col = AddCollider(64, 64);          // rectangle
col->shape = ColliderShape::Circle;       // change to circle
col->radius = 32.0f;
col->layer = 1; col->mask = 2;           // layer/mask filtering
```

**Callbacks** bubble up from child colliders to parent nodes:
```cpp
void OnCollisionEnter(Collider2D* other) override { /* entered */ }
void OnCollisionExit(Collider2D* other) override  { /* exited */ }
```

### Scene setup

**C++:**
```cpp
Scene scene;
auto* player = scene.Add<Player>("player");
auto* floor  = scene.Add<Wall>("floor", 800, 20);
auto* crate  = scene.Add<Crate>("crate");

while (!WindowShouldClose()) {
    scene.Update(GetDeltaTime());  // updates all nodes + collision
    ClearBackground(BLACK);
    scene.Draw();
    Present(); PollEvents();
}
```

---

## Asset Packing (KonPak)

```bash
# Create encrypted pack
konpak create game.konpak assets/* --pass mypassword

# List contents
konpak list game.konpak --pass mypassword

# Extract
konpak extract game.konpak --out assets_out/ --pass mypassword
```

**KonScript (build with --pack):**
```ks
// Build: konscript game.ks --pack
// Cross-compile: konscript game.ks --target windows --pack

AssetManager.init("game.konpak", "mypassword");
let mut tex: Texture = LoadTexture("sprite.png");  // reads from pack
```

**C++ (CMake with KON_PACK_KEY):**
```cpp
// CMake: cmake -B build -DKON_PACK_KEY="mypassword"
// The key is baked at compile time -- no password in game code.

#include "KonEngine.hpp"
AssetManager::init("game.konpak");                 // uses baked key
Texture tex = LoadTexture("sprite.png");           // reads from pack
```

---

## Using C++ Directly

```cpp
#include "KonEngine.hpp"

class Player : public KinematicBody2D {
public:
    float speed = 200.0f;
    Player(const std::string& name) : KinematicBody2D(name) {}

    void Ready() override {
        AddCollider(32, 48);
        KinematicBody2D::Ready();
    }

    void Update(float dt) override {
        float dx = 0, dy = 0;
        if (IsKeyDown(Key::D)) dx += speed * dt;
        if (IsKeyDown(Key::A)) dx -= speed * dt;
        if (IsKeyDown(Key::W)) dy -= speed * dt;
        if (IsKeyDown(Key::S)) dy += speed * dt;
        MoveAndCollide(dx, dy);
    }

    void OnCollisionEnter(Collider2D* other) override { tint = RED; }
    void OnCollisionExit(Collider2D* other) override  { tint = CYAN; }

    void Draw() override {
        DrawRectangle(DrawX(32), DrawY(48), 32, 48, tint);
    }
};

class Wall : public StaticBody2D {
public:
    float w, h;
    Wall(const std::string& name, float w, float h) : StaticBody2D(name), w(w), h(h) {}
    void Ready() override { AddCollider(w, h); StaticBody2D::Ready(); }
    void Draw() override { DrawRectangle(DrawX(w), DrawY(h), w, h, GRAY); }
};

int main() {
    InitWindow(800, 600, "C++ Game", true);
    SetTargetFPS(60);
    DebugMode(true);

    Scene scene;
    auto* player = scene.Add<Player>("player");
    player->x = 400; player->y = 300; player->tint = CYAN;

    auto* floor = scene.Add<Wall>("floor", 600, 20);
    floor->x = 400; floor->y = 500;

    Camera2D cam(player->x, player->y, 1.0f);

    while (!WindowShouldClose()) {
        scene.Update(GetDeltaTime());
        Camera2DFollow(cam, player->x, player->y, 0.4f, GetDeltaTime());

        ClearBackground(BLACK);
        BeginCamera2D(cam);
            scene.Draw();
        EndCamera2D();
        DrawText("WASD to move", 10, 10, WHITE);
        Present(); PollEvents();
    }
}
```

See `examples/cpp_example/` for full CMake setup.

---

## Cross-Compilation

### KonScript games
```bash
# Compile game for Windows from Linux
konscript game.ks --target windows              # outputs game.exe
konscript game.ks --target windows --pack       # with KonPak support
```

### C++ engine library (for KonScript toolchain)
```bash
cd tools/KonScript
./build-engine-lib.sh                           # linux64
./build-engine-lib.sh --windows                 # also builds windows64
```

### C++ games directly (using CMake + MXE)
```bash
# Engine library cross-compile
./build-windows.sh                              # engine only
./build-windows.sh --pack                       # with KonPak
./build-windows.sh --pack-key=mypassword        # with baked key
./build-windows.sh --tools                      # with KonAnimator
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
