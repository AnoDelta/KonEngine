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

### Creating Animations in Code (C++)

```cpp
// Sprite sheet animation
Animation walkAnim("walk", true);  // name, loop
walkAnim.AddFrame(0, 0, 32, 32, 0.1f);   // srcX, srcY, srcW, srcH, duration
walkAnim.AddFrame(32, 0, 32, 32, 0.1f);
walkAnim.AddFrame(64, 0, 32, 32, 0.1f);
walkAnim.AddFrame(96, 0, 32, 32, 0.1f);

// Keyframe animation with easing
Animation bounce("bounce");
bounce.Track("y").AddKey(0.0f, 0.0f, Ease::EaseOut)
                 .AddKey(0.3f, -20.0f, Ease::EaseIn)
                 .AddKey(0.6f, 0.0f);
bounce.Track("scaleX").AddKey(0.0f, 1.0f, Ease::EaseOutElastic)
                      .AddKey(0.6f, 1.2f);
bounce.AutoDuration();  // sets duration from last keyframe

// Attach to a Sprite2D via AnimationPlayer
auto* anim = sprite->AddChild<AnimationPlayer>("anim");
anim->Add(walkAnim);
anim->Add(bounce);
anim->Play("walk");
```

### Easing Curves

| Curve | Description |
|-------|-------------|
| `Linear` | Constant speed |
| `EaseIn` / `EaseOut` / `EaseInOut` | Quadratic acceleration |
| `EaseInCubic` / `EaseOutCubic` / `EaseInOutCubic` | Cubic acceleration |
| `EaseInElastic` / `EaseOutElastic` / `EaseInOutElastic` | Spring-like bounce |
| `EaseInBounce` / `EaseOutBounce` / `EaseInOutBounce` | Ball-drop bounce |
| `EaseInBack` / `EaseOutBack` / `EaseInOutBack` | Slight overshoot |

The curve on a keyframe controls interpolation **from** that keyframe **to** the next. The last keyframe's curve has no effect.

### Animation Overlays

Animation values are applied as **non-destructive overlays** during rendering only. This means animations never modify the actual `x`, `y`, `scaleX`, `scaleY`, `rotation`, or `alpha` of a node. Gameplay code and animation can both modify properties without conflict.

| Track | Overlay | Effect |
|-------|---------|--------|
| `x`, `y` | `animOffsetX/Y` | Added to position |
| `scaleX`, `scaleY` | `animScaleX/Y` | Multiplied with scale |
| `rotation` | `animRotation` | Added to rotation |
| `alpha` | `animAlpha` | Multiplied with tint alpha |

### Compiling .anim Files

```bash
anim_compiler player.anim    # outputs player.konani
```

---

## Physics & Collision

### Overview

KonEngine uses a **CollisionWorld** that runs SAT (Separating Axis Theorem) collision detection each frame. Colliders are added as children of nodes. The world handles enter/exit signals and automatic depenetration.

### Collision Shapes

Colliders support three shapes: **Rectangle** (AABB), **Circle**, and **Custom** (convex polygon).

**KonScript:**
```ks
node Player : Node2D {
    func Ready() {
        # Rectangle collider
        let col: Collider2D = this.add(Collider2D, "hitbox");
        col.width  = 32.0;
        col.height = 48.0;

        # Circle collider
        let sensor: Collider2D = this.add(Collider2D, "range");
        sensor.shape  = ColliderShape.Circle;
        sensor.radius = 64.0;
    }
}
```

**C++:**
```cpp
class Player : public Node2D {
public:
    Player(const std::string& name = "Player") : Node2D(name) {}

    void Ready() override {
        // Rectangle collider
        auto* col = AddChild<Collider2D>("hitbox");
        col->width  = 32.0f;
        col->height = 48.0f;

        // Circle collider
        auto* sensor = AddChild<Collider2D>("range");
        sensor->shape  = ColliderShape::Circle;
        sensor->radius = 64.0f;
    }
};
```

### Collision Layers & Masks

Each collider has a `layer` (what it is) and a `mask` (what it collides with), both 32-bit bitfields. Two colliders only interact when `(a.layer & b.mask) || (b.layer & a.mask)` is true.

**KonScript:**
```ks
# Player on layer 1, collides with layer 2 (enemies)
col.layer = 1;
col.mask  = 2;

# Enemy on layer 2, collides with layer 1 (player)
enemyCol.layer = 2;
enemyCol.mask  = 1;
```

**C++:**
```cpp
col->layer = 1;
col->mask  = 2;

enemyCol->layer = 2;
enemyCol->mask  = 1;
```

### Collision Callbacks

Define `OnCollisionEnter` and `OnCollisionExit` on the **parent node** of the collider. The engine routes collision signals up from child colliders.

**KonScript:**
```ks
node Player : Node2D {
    let mut hp: I32 = 100;

    func OnCollisionEnter(other: Collider2D) {
        if other.name == "enemy_hitbox" {
            hp -= 10;
            Print("Ouch! HP: %d\n", hp);
        }
    }

    func OnCollisionExit(other: Collider2D) {
        Print("No longer touching: %s\n", other.name);
    }
}
```

**C++:**
```cpp
class Player : public Node2D {
public:
    int hp = 100;
    Player(const std::string& name = "Player") : Node2D(name) {}

    void OnCollisionEnter(Collider2D* other) override {
        if (other->name == "enemy_hitbox") {
            hp -= 10;
            printf("Ouch! HP: %d\n", hp);
        }
    }

    void OnCollisionExit(Collider2D* other) override {
        printf("No longer touching: %s\n", other->name.c_str());
    }
};
```

### Physics Bodies

#### StaticBody2D — Immovable walls, floors, platforms

Child colliders are automatically marked `solid = true` and `staticBody = true`. They never move during depenetration.

**KonScript:**
```ks
node Floor : StaticBody2D {
    func Ready() {
        x = 0.0; y = 568.0;
        let col: Collider2D = this.add(Collider2D, "floor");
        col.width  = 800.0;
        col.height = 32.0;
    }
}
```

**C++:**
```cpp
class Floor : public StaticBody2D {
public:
    Floor(const std::string& name = "Floor") : StaticBody2D(name) {}
    void Ready() override {
        x = 0; y = 568;
        auto* col = AddChild<Collider2D>("floor");
        col->width  = 800;
        col->height = 32;
    }
};
```

#### KinematicBody2D — Player-controlled, slides along walls

Use `MoveAndCollide(dx, dy)` to move with automatic wall sliding. Returns the actual movement applied.

**KonScript:**
```ks
node Player : KinematicBody2D {
    let mut speed: F64 = 200.0;

    func Ready() {
        let col: Collider2D = this.add(Collider2D, "col");
        col.width = 32.0; col.height = 48.0;
    }

    func Update(dt: F64) {
        let mut dx: F64 = 0.0;
        let mut dy: F64 = 0.0;
        if KeyDown(Key.D) { dx =  speed * dt; }
        if KeyDown(Key.A) { dx = -speed * dt; }
        if KeyDown(Key.S) { dy =  speed * dt; }
        if KeyDown(Key.W) { dy = -speed * dt; }
        MoveAndCollide(dx, dy);
    }
}
```

**C++:**
```cpp
class Player : public KinematicBody2D {
public:
    float speed = 200.0f;
    Player(const std::string& name = "Player") : KinematicBody2D(name) {}

    void Ready() override {
        auto* col = AddChild<Collider2D>("col");
        col->width = 32; col->height = 48;
    }

    void Update(float dt) override {
        float dx = 0, dy = 0;
        if (IsKeyDown(Key::D)) dx =  speed * dt;
        if (IsKeyDown(Key::A)) dx = -speed * dt;
        if (IsKeyDown(Key::S)) dy =  speed * dt;
        if (IsKeyDown(Key::W)) dy = -speed * dt;
        MoveAndCollide(dx, dy);
    }
};
```

#### RigidBody2D — Gravity-driven physics objects

Automatically applies gravity and resolves collisions with statics. Detects floor contact via `onFloor`.

**KonScript:**
```ks
node Crate : RigidBody2D {
    func Ready() {
        let col: Collider2D = this.add(Collider2D, "col");
        col.width = 32.0; col.height = 32.0;
        gravity = 980.0;   # pixels/s² (default)
    }
}
```

**C++:**
```cpp
class Crate : public RigidBody2D {
public:
    Crate(const std::string& name = "Crate") : RigidBody2D(name) {}
    void Ready() override {
        auto* col = AddChild<Collider2D>("col");
        col->width = 32; col->height = 32;
        gravity = 980.0f;
    }
};
```

### Scene Registration

Colliders added during `Ready()` are registered with the `CollisionWorld` automatically. If you add colliders later (e.g., in `Update`), call `scene.scan()` to register them.

**KonScript:**
```ks
# Late collider addition
let newCol: Collider2D = player.add(Collider2D, "shield");
newCol.width = 48.0; newCol.height = 48.0;
scene.scan();  # register the new collider
```

**C++:**
```cpp
auto* newCol = player->AddChild<Collider2D>("shield");
newCol->width = 48; newCol->height = 48;
scene.Scan();
```

### Debug Visualization

Enable `DebugMode(true)` to see all collider outlines drawn automatically. Active collisions are highlighted in a different color.

---

## Camera System

### Basic Camera

**KonScript:**
```ks
#include <engine>

node Player : Node2D {
    func Ready() { x = 400.0; y = 300.0; }
    func Update(dt: F64) {
        if KeyDown(Key.D) { x += 200.0 * dt; }
        if KeyDown(Key.A) { x -= 200.0 * dt; }
    }
}

func main() {
    InitWindow(800, 600, "Camera Demo");
    SetTargetFPS(60);
    let scene: Scene = Scene();
    let player: Player = scene.add(Player, "player");

    # Add a CameraNode2D to the scene
    let cam: CameraNode2D = scene.add(CameraNode2D, "cam");
    cam.current = true;   # marks this as the active camera
    cam.zoom = 1.5;

    while !WindowShouldClose() {
        let dt: F64 = GetDeltaTime();
        ClearBackground(0.1, 0.1, 0.1);
        scene.update(dt);
        scene.draw();   # auto-applies the active CameraNode2D
        Present();
        PollEvents();
    }
}
```

**C++:**
```cpp
#include "KonEngine.hpp"

int main() {
    InitWindow(800, 600, "Camera Demo");
    SetTargetFPS(60);
    Scene scene;

    auto* player = scene.Add<Node2D>("player");
    player->x = 400; player->y = 300;

    auto* cam = scene.Add<CameraNode2D>("cam");
    cam->current = true;
    cam->zoom = 1.5f;

    while (!WindowShouldClose()) {
        float dt = GetDeltaTime();
        ClearBackground(0.1f, 0.1f, 0.1f);
        scene.Update(dt);
        scene.Draw();
        Present(); PollEvents();
    }
}
```

### Camera Utilities (C++ only)

These free functions operate on the `Camera2D` struct directly:

```cpp
Camera2D cam(400, 300, 1.0f, 0.0f);

// Smooth follow — speed 0.1 (slow) to 0.9 (fast)
Camera2DFollow(cam, player->x, player->y, 0.5f, dt);

// Clamp to world bounds (never show outside the map)
Camera2DClamp(cam, 0, 0, worldWidth, worldHeight,
              GetWindowWidth(), GetWindowHeight());

// Screen shake — decay magnitude each frame for effect
static float shakeMag = 0.0f;
if (hitTaken) shakeMag = 8.0f;
Camera2DShake(cam, shakeMag);
shakeMag *= 0.9f;

// Interpolate between two cameras
Camera2D blended = Camera2DLerp(camA, camB, 0.5f);
```

---

## Audio System

### Sound Effects

Short one-shot sounds loaded fully into memory.

**KonScript:**
```ks
PlaySound("assets/jump.wav");
StopSound("assets/jump.wav");
SetSoundVolume(0.8);
```

**C++:**
```cpp
Sound jump = LoadSound("assets/jump.wav");
PlaySound(jump);
StopSound(jump);
SetSoundVolume(jump, 0.8f);

// Check state
if (IsSoundPlaying(jump)) { /* ... */ }

// Pause / Resume
PauseSound(jump);
ResumeSound(jump);

// Cleanup
UnloadSound(jump);
```

### Music Streaming

Long tracks streamed from disk. Call `UpdateMusic()` each frame to keep the stream fed.

**KonScript:**
```ks
PlayMusic("assets/bgm.ogg");
SetMusicVolume(0.5);

# In game loop:
# Music updates are handled automatically in KonScript
```

**C++:**
```cpp
Music bgm = LoadMusic("assets/bgm.ogg");
bgm.looping = true;
PlayMusic(bgm);
SetMusicVolume(bgm, 0.5f);

// In game loop — required each frame
UpdateMusic(bgm);

// Controls
PauseMusic(bgm);
ResumeMusic(bgm);
StopMusic(bgm);
SetMusicLooping(bgm, false);

// Cleanup
UnloadMusic(bgm);
```

### Master Volume

```cpp
SetMasterVolume(0.7f);  // affects all sounds and music
```

**Supported formats:** `.wav`, `.ogg`, `.mp3` (via miniaudio)

---

## Input System

### Keyboard

Three states: **down** (held), **pressed** (just this frame), **released** (just let go).

**KonScript:**
```ks
if KeyDown(Key.D)      { x += speed * dt; }   # held
if KeyPressed(Key.Space) { jump(); }            # just pressed
if KeyReleased(Key.Shift) { stopSprint(); }     # just released
```

**C++:**
```cpp
if (IsKeyDown(Key::D))        x += speed * dt;
if (IsKeyPressed(Key::Space)) jump();
if (IsKeyReleased(Key::Shift)) stopSprint();
```

**Key constants:** `A`-`Z`, `Num0`-`Num9`, `Right`/`Left`/`Down`/`Up`, `Space`, `Enter`, `Escape`, `Tab`, `Backspace`, `Shift`, `Ctrl`, `Alt`, `F1`-`F12`

### Mouse

**KonScript:**
```ks
if MouseDown(Mouse.Left)    { shoot(); }
if MousePressed(Mouse.Right) { aim(); }

let mx: F64 = GetMouseX();
let my: F64 = GetMouseY();
let dx: F64 = GetMouseDeltaX();   # per-frame movement
let dy: F64 = GetMouseDeltaY();
let scroll: F64 = GetMouseScroll();
```

**C++:**
```cpp
if (IsMouseButtonDown(Mouse::Left))    shoot();
if (IsMouseButtonPressed(Mouse::Right)) aim();

float mx = GetMouseX(), my = GetMouseY();
float dx = GetMouseDeltaX(), dy = GetMouseDeltaY();
float scroll = GetMouseScroll();
```

### Gamepad (C++ only)

Supports multiple gamepads for local multiplayer.

```cpp
// Check connection (player index 0-3)
if (IsGamepadConnected(0)) {
    // Buttons
    if (IsGamepadButtonPressed(0, Gamepad::A)) jump();
    if (IsGamepadButtonDown(0, Gamepad::RightBumper)) sprint();

    // Analog sticks (-1.0 to 1.0)
    float moveX = GetGamepadAxis(0, Gamepad::LeftX);
    float moveY = GetGamepadAxis(0, Gamepad::LeftY);

    // Triggers (0.0 to 1.0)
    float brake = GetGamepadAxis(0, Gamepad::LeftTrigger);
    float gas   = GetGamepadAxis(0, Gamepad::RightTrigger);
}
```

**Gamepad buttons:** `A`, `B`, `X`, `Y`, `LeftBumper`, `RightBumper`, `Back`, `Start`, `LeftThumb`, `RightThumb`, `DPadUp`, `DPadRight`, `DPadDown`, `DPadLeft`

**Gamepad axes:** `LeftX`, `LeftY`, `RightX`, `RightY`, `LeftTrigger`, `RightTrigger`

---

## Text & Font Rendering

### Default Font

A built-in Inconsolata font is available at any size without loading a file.

**KonScript:**
```ks
DrawText("Hello World", 10.0, 10.0, 20, WHITE);
DrawText("Big text", 10.0, 40.0, 48, RED);
```

**C++:**
```cpp
// Default font, default size (20)
DrawText("Hello World", 10, 10, WHITE);

// Default font, custom size (cached automatically)
DrawText("Big text", 10, 40, 48, RED);

// Printf-style formatting
DrawTextF(10, 80, WHITE, "Score: %d", score);
DrawTextF(10, 100, 32, YELLOW, "FPS: %d", GetFPS());
```

### Custom Fonts

**C++:**
```cpp
Font myFont = LoadFont("assets/myfont.ttf", 24);
DrawText(myFont, "Custom font", 10, 10, GREEN);
DrawTextF(myFont, 10, 40, BLUE, "HP: %d/%d", hp, maxHp);

// Cached loading — same file+size returns the same Font
Font& cached = GetCachedFont("assets/myfont.ttf", 32);
```

---

## Tilemap Helpers

`TileGrid` provides coordinate conversion and debug visualization for tile-based games.

**C++:**
```cpp
TileGrid grid;
grid.tileW = 32;
grid.tileH = 32;

// Convert world position to tile coordinates
TileCoord tc = grid.WorldToTile(mouseX, mouseY);

// Convert tile back to world position
WorldPos wp = grid.TileToWorld(tc.x, tc.y);

// Snap world position to nearest tile corner
WorldPos snapped = grid.Snap(mouseX, mouseY);

// Get the center of a tile
WorldPos center = grid.TileCenter(3, 5);

// Debug visualization
grid.DrawGrid(0, 0, 25, 19);  // 25x19 grid of 32px tiles
grid.DrawGridHighlight(0, 0, 25, 19, tc.x, tc.y,
                       {0.3f, 0.3f, 0.3f, 0.4f},  // grid color
                       {1.0f, 1.0f, 0.0f, 0.3f},   // fill color
                       {1.0f, 1.0f, 0.0f, 1.0f});   // border color
```

---

## Color Presets

Available in both KonScript and C++:

| Constant | RGBA |
|----------|------|
| `RED` | (1, 0, 0, 1) |
| `GREEN` | (0, 1, 0, 1) |
| `BLUE` | (0, 0, 1, 1) |
| `WHITE` | (1, 1, 1, 1) |
| `BLACK` | (0, 0, 0, 1) |
| `YELLOW` | (1, 1, 0, 1) |
| `CYAN` | (0, 1, 1, 1) |
| `MAGENTA` | (1, 0, 1, 1) |
| `ORANGE` | (1, 0.5, 0, 1) |
| `GRAY` | (0.5, 0.5, 0.5, 1) |
| `BLANK` | (0, 0, 0, 0) |

Custom colors in C++: `Color myColor(0.2f, 0.8f, 1.0f, 1.0f);`

---

## Vector2 & Random (C++)

### Vector2

```cpp
Vector2 pos(100, 200);
Vector2 vel(1, 0);

float len   = vel.Length();
Vector2 dir = vel.Normalized();
float d     = pos.Distance(other);
float dot   = vel.Dot(other);
Vector2 r   = vel.Reflected(wallNormal);
Vector2 rot = vel.Rotated(3.14159f / 4);  // radians

// Interpolation
Vector2 mid = Vector2::Lerp(a, b, 0.5f);

// Presets
Vector2 zero  = Vector2::Zero();   // (0, 0)
Vector2 up    = Vector2::Up();     // (0, -1)
Vector2 right = Vector2::Right();  // (1, 0)
```

### Random

```cpp
Random::Seed();                  // random seed
Random::Seed(42);                // deterministic seed

int r   = Random::Range(1, 6);          // 1-6 inclusive
float f = Random::RangeF(0.5f, 1.5f);   // float range
float v = Random::Value();               // 0.0 - 1.0
bool b  = Random::Bool(0.3f);           // 30% chance true

// Random element from a vector
std::vector<std::string> names = {"Alice", "Bob", "Carol"};
std::string pick = Random::From(names);
```

---

## Signals (C++)

Nodes have a lightweight signal system for decoupled communication.

```cpp
// Connect a callback to a signal
player->Connect("player_dead", [&]() {
    printf("Game Over!\n");
    gameOver = true;
});

// Emit from inside a node
void Update(float dt) override {
    if (hp <= 0) Emit("player_dead");
}
```

---

## Debug Mode

`DebugMode(true)` enables a suite of visual debugging tools:

- **FPS counter** — top-left overlay showing current FPS and delta time
- **Mouse crosshair** — lines tracking the cursor position
- **Viewport border** — red outline around the game area
- **Collider outlines** — all Collider2D shapes drawn as wireframes (green = idle, highlighted = colliding)

**KonScript:**
```ks
DebugMode(true);
if IsDebugMode() { Print("Debug is on\n"); }
```

**C++:**
```cpp
DebugMode(true);
if (IsDebugMode()) { printf("Debug is on\n"); }
```

---

## Letterbox Scaling

When `InitWindow` is called with `canResize = true`, the engine maintains your design resolution with automatic black bars. All game coordinates remain unchanged regardless of window size.

**KonScript:**
```ks
InitWindow(800, 600, "My Game", true);  # resizable with letterboxing
```

**C++:**
```cpp
InitWindow(800, 600, "My Game", true);
// Mouse input is automatically transformed to game coordinates
// DrawRectangle(0, 0, 800, 600, RED) always fills the game area
```

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
