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
9. [UI System](#ui-system)
10. [Cross-Compilation](#cross-compilation)
11. [Editor (KonEditor)](#editor-koneditor)

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
        if KeyDown(Key.D) { x = x + speed * dt; }
        if KeyDown(Key.A) { x = x - speed * dt; }
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
        if KeyDown(Key.D) { dx = speed * dt; }
        if KeyDown(Key.A) { dx = -speed * dt; }
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

### Standalone Collision Checks

For quick collision tests outside the node system, use these functions with `Rectangle` and `Circle` structs.

**KonScript:**
```ks
let a: Rectangle = Rectangle(10.0, 10.0, 50.0, 50.0);  # x, y, w, h
let b: Rectangle = Rectangle(40.0, 40.0, 50.0, 50.0);

if CheckCollisionRecs(a, b) {
    Print("Rectangles overlap!");
}

let c1: Circle = Circle(100.0, 100.0, 25.0);   # x, y, radius
let c2: Circle = Circle(120.0, 110.0, 20.0);

if CheckCollisionCircles(c1, c2) {
    Print("Circles overlap!");
}

if CheckCollisionCircleRec(c1, a) {
    Print("Circle hits rectangle!");
}
```

**C++:**
```cpp
Rectangle a(10, 10, 50, 50);
Rectangle b(40, 40, 50, 50);

if (CheckCollisionRecs(a, b)) { /* overlap */ }

Circle c1(100, 100, 25);
Circle c2(120, 110, 20);

if (CheckCollisionCircles(c1, c2)) { /* overlap */ }
if (CheckCollisionCircleRec(c1, a)) { /* overlap */ }
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

### Camera Utilities

These free functions operate on the `Camera2D` struct directly.

**KonScript:**
```ks
let mut cam: Camera2D = Camera2D(400.0, 300.0, 1.0, 0.0);

# Smooth follow — speed 0.1 (slow) to 0.9 (fast)
Camera2DFollow(cam, player.x, player.y, 0.5, dt);

# Clamp to world bounds (never show outside the map)
Camera2DClamp(cam, 0.0, 0.0, worldW, worldH,
              GetWindowWidth() as F64, GetWindowHeight() as F64);

# Screen shake — decay magnitude each frame
Camera2DShake(cam, shakeMag);
shakeMag *= 0.9;

# Interpolate between two cameras
let blended: Camera2D = Camera2DLerp(camA, camB, 0.5);

# Manual camera control (without CameraNode2D)
BeginCamera2D(cam);
# ... draw world here ...
EndCamera2D();
```

**C++:**
```cpp
Camera2D cam(400, 300, 1.0f, 0.0f);

Camera2DFollow(cam, player->x, player->y, 0.5f, dt);
Camera2DClamp(cam, 0, 0, worldWidth, worldHeight,
              GetWindowWidth(), GetWindowHeight());

static float shakeMag = 0.0f;
if (hitTaken) shakeMag = 8.0f;
Camera2DShake(cam, shakeMag);
shakeMag *= 0.9f;

Camera2D blended = Camera2DLerp(camA, camB, 0.5f);

BeginCamera2D(cam);
// ... draw world here ...
EndCamera2D();
```

---

## Audio System

### Sound Effects

Short one-shot sounds loaded fully into memory.

**KonScript:**
```ks
let snd: Sound = LoadSound("assets/jump.wav");
PlaySound(snd);
StopSound(snd);
PauseSound(snd);
ResumeSound(snd);
SetSoundVolume(snd, 0.8);

if IsSoundPlaying(snd) { Print("playing"); }

UnloadSound(snd);
```

**C++:**
```cpp
Sound jump = LoadSound("assets/jump.wav");
PlaySound(jump);
StopSound(jump);
PauseSound(jump);
ResumeSound(jump);
SetSoundVolume(jump, 0.8f);

if (IsSoundPlaying(jump)) { /* ... */ }

UnloadSound(jump);
```

### Music Streaming

Long tracks streamed from disk. Call `UpdateMusic()` each frame to keep the stream fed.

**KonScript:**
```ks
let bgm: Music = LoadMusic("assets/bgm.ogg");
PlayMusic(bgm);
SetMusicVolume(bgm, 0.5);
SetMusicLooping(bgm, true);

# In game loop:
UpdateMusic(bgm);

# Query state
if IsMusicPlaying(bgm) { Print("music on"); }

# Controls
PauseMusic(bgm);
ResumeMusic(bgm);
StopMusic(bgm);
UnloadMusic(bgm);
```

**C++:**
```cpp
Music bgm = LoadMusic("assets/bgm.ogg");
bgm.looping = true;
PlayMusic(bgm);
SetMusicVolume(bgm, 0.5f);

// In game loop — required each frame
UpdateMusic(bgm);

if (IsMusicPlaying(bgm)) { /* ... */ }

PauseMusic(bgm);
ResumeMusic(bgm);
StopMusic(bgm);
SetMusicLooping(bgm, false);
UnloadMusic(bgm);
```

### Master Volume

```ks
SetMasterVolume(0.7);   # KonScript
```
```cpp
SetMasterVolume(0.7f);  // C++ — affects all sounds and music
```

**Supported formats:** `.wav`, `.ogg`, `.mp3` (via miniaudio)

---

## Input System

### Input States

Every input source (keyboard, mouse, gamepad) has three states:

| State | Meaning | Use for |
|-------|---------|---------|
| **Down** | Held right now | Movement, continuous actions |
| **Pressed** | Just pressed this frame | Jump, attack, menu select |
| **Released** | Just let go this frame | Release charge, stop sprint |

### Keyboard

**KonScript:**
```ks
# Movement (held every frame)
if KeyDown(Key.D) { x += speed * dt; }
if KeyDown(Key.A) { x -= speed * dt; }
if KeyDown(Key.W) { y -= speed * dt; }
if KeyDown(Key.S) { y += speed * dt; }

# One-shot actions (fires once on press)
if KeyPressed(Key.Space) { jump(); }
if KeyPressed(Key.Enter) { confirm(); }
if KeyPressed(Key.Esc)   { pauseMenu(); }

# Release detection
if KeyReleased(Key.Shift) { stopSprint(); }
```

**C++:**
```cpp
if (IsKeyDown(Key::D)) x += speed * dt;
if (IsKeyDown(Key::A)) x -= speed * dt;
if (IsKeyDown(Key::W)) y -= speed * dt;
if (IsKeyDown(Key::S)) y += speed * dt;

if (IsKeyPressed(Key::Space)) jump();
if (IsKeyPressed(Key::Enter)) confirm();
if (IsKeyPressed(Key::Escape)) pauseMenu();

if (IsKeyReleased(Key::Shift)) stopSprint();
```

**Key constants:**

| Category | Keys |
|----------|------|
| Letters | `A` - `Z` |
| Numbers | `Num0` - `Num9` |
| Arrows | `Up`, `Down`, `Left`, `Right` |
| Actions | `Space`, `Enter`, `Escape` (or `Esc`), `Tab`, `Backspace` |
| Modifiers | `Shift`, `Ctrl`, `Alt` |
| Function | `F1` - `F12` |

Access as `Key.Space` in KonScript, `Key::Space` in C++.

### Mouse

**KonScript:**
```ks
# Click detection
if MouseDown(Mouse.Left)     { shoot(); }       # held (auto-fire)
if MousePressed(Mouse.Left)  { singleShot(); }  # just clicked
if MouseReleased(Mouse.Left) { releaseCharge(); }
if MousePressed(Mouse.Right) { aim(); }

# Screen-space mouse position (design resolution coordinates)
let mx: F64 = GetMouseX();
let my: F64 = GetMouseY();

# Per-frame movement delta
let dx: F64 = GetMouseDeltaX();
let dy: F64 = GetMouseDeltaY();

# Scroll wheel (positive = up, negative = down)
let scroll: F64 = GetMouseScroll();
```

**C++:**
```cpp
if (IsMouseButtonDown(Mouse::Left))     shoot();
if (IsMouseButtonPressed(Mouse::Left))  singleShot();
if (IsMouseButtonReleased(Mouse::Left)) releaseCharge();

float mx = GetMouseX(), my = GetMouseY();
float dx = GetMouseDeltaX(), dy = GetMouseDeltaY();
float scroll = GetMouseScroll();
```

**Mouse buttons:** `Mouse.Left`, `Mouse.Right`, `Mouse.Middle` (KonScript) / `Mouse::Left`, etc. (C++)

### World-Space Mouse (Clicking Game Objects)

`GetMouseX/Y()` returns screen coordinates. To click on things in the game world (affected by camera pan/zoom), use `GetWorldMouseX/Y(camera)`:

**KonScript:**
```ks
let cam: Camera2D = Camera2D(400.0, 300.0, 1.0, 0.0);

# Screen mouse — for UI, HUD, menus
let screenX: F64 = GetMouseX();
let screenY: F64 = GetMouseY();

# World mouse — for clicking game objects, tiles, enemies
let worldX: F64 = GetWorldMouseX(cam);
let worldY: F64 = GetWorldMouseY(cam);

# Example: click to place a block at the mouse's world position
if MousePressed(Mouse.Left) {
    spawnBlock(worldX, worldY);
}

# Example: check if clicking near the player
let dist: F64 = Distance(worldX, worldY, player.x, player.y);
if dist < 32.0 && MousePressed(Mouse.Left) {
    Print("Clicked on player!");
}
```

**C++:**
```cpp
Camera2D cam(400, 300, 1.0f, 0.0f);

// Screen-space (for UI)
float screenX = GetMouseX();
float screenY = GetMouseY();

// World-space (for game objects)
float worldX = GetWorldMouseX(cam);
float worldY = GetWorldMouseY(cam);

// Click to place at world position
if (IsMouseButtonPressed(Mouse::Left)) {
    spawnBlock(worldX, worldY);
}
```

**When to use which:**

| Function | Coordinate space | Use for |
|----------|-----------------|---------|
| `GetMouseX/Y()` | Screen (design resolution) | UI buttons, HUD, menus |
| `GetGameMouseX/Y()` | Screen (letterbox-corrected) | Same as above, with letterbox |
| `GetWorldMouseX/Y(cam)` | World (camera-transformed) | Clicking game objects, tiles, placing items |

### Gamepad

Supports multiple gamepads for local multiplayer (player index 0-3).

**KonScript:**
```ks
if GamepadConnected(0) {
    # Buttons
    if GamepadPressed(0, Gamepad.A) { jump(); }
    if GamepadDown(0, Gamepad.RightBumper) { sprint(); }

    # Analog sticks (-1.0 to 1.0)
    let moveX: F64 = GamepadAxis(0, Gamepad.LeftX);
    let moveY: F64 = GamepadAxis(0, Gamepad.LeftY);

    # Triggers (0.0 to 1.0)
    let brake: F64 = GamepadAxis(0, Gamepad.LeftTrigger);
    let gas: F64   = GamepadAxis(0, Gamepad.RightTrigger);
}
```

**C++:**
```cpp
if (IsGamepadConnected(0)) {
    if (IsGamepadButtonPressed(0, Gamepad::A)) jump();
    if (IsGamepadButtonDown(0, Gamepad::RightBumper)) sprint();

    float moveX = GetGamepadAxis(0, Gamepad::LeftX);
    float moveY = GetGamepadAxis(0, Gamepad::LeftY);
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

**KonScript:**
```ks
let font: Font = LoadFont("assets/myfont.ttf", 24);
# Use with DrawText by passing the font object
UnloadFont(font);
```

**C++:**
```cpp
Font myFont = LoadFont("assets/myfont.ttf", 24);
DrawText(myFont, "Custom font", 10, 10, GREEN);
DrawTextF(myFont, 10, 40, BLUE, "HP: %d/%d", hp, maxHp);

// Cached loading — same file+size returns the same Font
Font& cached = GetCachedFont("assets/myfont.ttf", 32);
```

---

## Tilemap System

### Tilemap (Data + Rendering)

`Tilemap` stores a 2D grid of tile IDs and renders them using a tileset spritesheet. Tile ID 0 = empty.

**C++:**
```cpp
// Create a 20x15 tilemap with 32x32 pixel tiles
Tilemap map(20, 15, 32, 32);
map.originX = 0;
map.originY = 0;

// Load tileset spritesheet
map.SetTileset(LoadTexture("tiles.png"));

// Place tiles (ID 1 = first tile in sheet, 2 = second, etc.)
map.Set(5, 3, 1);    // grass
map.Set(6, 3, 2);    // stone
map.Fill(1);          // fill entire map with tile 1

// Render the tilemap
map.Draw();

// Draw debug grid overlay
map.DrawGrid();

// Draw a single tile from the tileset at any position
map.DrawTileAt(3, worldX, worldY);
```

### Tile Click Detection

```cpp
// Get which tile the player clicked (using world-space mouse)
float wmx = GetWorldMouseX(cam);
float wmy = GetWorldMouseY(cam);

TileCoord clicked = map.GetTileAt(wmx, wmy);
if (clicked.x >= 0) {
    int tileId = map.Get(clicked.x, clicked.y);
    printf("Clicked tile (%d,%d) = ID %d\n", clicked.x, clicked.y, tileId);

    // Place a tile where clicked
    map.Set(clicked.x, clicked.y, 5);
}

// Or get tile ID directly
int id = map.GetTileIdAt(wmx, wmy);
```

### Coordinate Conversion

```cpp
// Tile -> world position (top-left corner)
WorldPos wp = map.TileToWorld(5, 3);

// Tile -> world position (center)
WorldPos center = map.TileCenter(5, 3);

// World -> tile coordinates
TileCoord tc = map.WorldToTile(worldX, worldY);

// Bounds check
if (map.InBounds(tc.x, tc.y)) { /* valid tile */ }
```

### TileGrid (Lightweight Grid Overlay)

For simple grid snapping and visualization without tile data storage:

```cpp
TileGrid grid(32, 32);
grid.DrawGrid(0, 0, 25, 19);
TileCoord tc = grid.WorldToTile(mouseX, mouseY);
WorldPos snapped = grid.Snap(mouseX, mouseY);
```

### Isometric Tilemap (Full Example)

`IsoTilemap` combines isometric coordinate math with tile data storage and rendering. Tiles are drawn at correct diamond-projected positions automatically.

**C++ -- complete isometric game setup:**
```cpp
#include "KonEngine.hpp"

int main() {
    InitWindow(800, 600, "Isometric Demo", true);
    SetTargetFPS(60);

    // 1. Create isometric tilemap (10x10 grid, 64x32 pixel tiles)
    IsoTilemap map(10, 10, 64, 32);
    map.originX = 400;  // center the map on screen
    map.originY = 50;

    // 2. Load tileset spritesheet
    //    Each tile in the spritesheet is 64x32 pixels.
    //    Tile ID 1 = first tile (top-left), ID 2 = next, etc.
    map.SetTileset(LoadTexture("iso_tiles.png"));

    // 3. Fill the map with grass (tile ID 1)
    map.Fill(1);

    // 4. Place some different tiles
    map.Set(3, 3, 2);  // stone
    map.Set(4, 3, 2);
    map.Set(5, 5, 3);  // water

    Camera2D cam(400, 300, 1.0f, 0);

    while (!WindowShouldClose()) {
        float dt = GetDeltaTime();

        // 5. Click detection -- which tile did we click?
        float wmx = GetWorldMouseX(cam);
        float wmy = GetWorldMouseY(cam);
        TileCoord hover = map.GetTileAt(wmx, wmy);

        if (IsMouseButtonPressed(Mouse::Left) && map.InBounds(hover.x, hover.y)) {
            map.Set(hover.x, hover.y, 2);  // place stone where clicked
        }

        // 6. Render
        ClearBackground(0.15f, 0.15f, 0.2f);
        BeginCamera2D(cam);
            map.Draw();                    // draws all tiles at correct iso positions
            map.DrawGrid(GRAY);            // debug overlay
        EndCamera2D();

        // HUD
        if (map.InBounds(hover.x, hover.y)) {
            DrawTextF(10, 10, WHITE, "Tile: (%d, %d) ID: %d",
                      hover.x, hover.y, map.Get(hover.x, hover.y));
        }

        Present();
        PollEvents();
    }
}
```

**KonScript:**
```ks
#include <engine>

func main() {
    InitWindow(800, 600, "Isometric Demo", true);
    SetTargetFPS(60);

    let mut map: IsoTilemap = IsoTilemap(10, 10, 64, 32);
    map.originX = 400.0;
    map.originY = 50.0;
    map.SetTileset(LoadTexture("iso_tiles.png"));
    map.Fill(1);

    let cam: Camera2D = Camera2D(400.0, 300.0, 1.0, 0.0);

    while !WindowShouldClose() {
        let wmx: F64 = GetWorldMouseX(cam);
        let wmy: F64 = GetWorldMouseY(cam);
        let hover: TileCoord = map.GetTileAt(wmx, wmy);

        if MousePressed(Mouse.Left) && map.InBounds(hover.x, hover.y) {
            map.Set(hover.x, hover.y, 2);
        }

        ClearBackground(0.15, 0.15, 0.2);
        BeginCamera2D(cam);
        map.Draw();
        map.DrawGrid();
        EndCamera2D();

        Present();
        PollEvents();
    }
}
```

**How the tileset spritesheet works:**

```
iso_tiles.png (192 x 32 for 3 tiles at 64x32 each):
┌──────────┬──────────┬──────────┐
│  Tile 1  │  Tile 2  │  Tile 3  │
│  (grass) │  (stone) │  (water) │
│  64x32   │  64x32   │  64x32   │
└──────────┴──────────┴──────────┘
   ID 1       ID 2       ID 3
```

Each tile sprite should be a diamond shape drawn within a `tileW x tileH` rectangle. The transparent areas around the diamond let tiles overlap correctly.

### Isometric Grid (Coordinate Helper Only)

If you just need isometric math without tile data storage:

```cpp
IsometricGrid iso(64, 32);

WorldPos pos = iso.TileToScreen(3, 5, originX, originY);  // tile -> screen
TileCoord tc = iso.ScreenToTile(mouseX, mouseY, originX, originY);  // click -> tile
iso.DrawGrid(originX, originY, 10, 10);  // draw diamond grid
iso.DrawGridHighlight(originX, originY, 10, 10, tc.x, tc.y);  // highlight hovered
```

### Saving and Loading Tilemaps

The engine doesn't include a built-in save format, but saving tile data is straightforward since it's just a grid of integers:

**C++:**
```cpp
#include <fstream>

// Save tilemap to a simple binary file
void SaveTilemap(const Tilemap& map, const char* path) {
    std::ofstream f(path, std::ios::binary);
    f.write((char*)&map.cols, sizeof(int));
    f.write((char*)&map.rows, sizeof(int));
    f.write((char*)&map.tileW, sizeof(int));
    f.write((char*)&map.tileH, sizeof(int));
    for (int y = 0; y < map.rows; y++)
        for (int x = 0; x < map.cols; x++) {
            int id = map.Get(x, y);
            f.write((char*)&id, sizeof(int));
        }
}

// Load tilemap from binary file
Tilemap LoadTilemap(const char* path) {
    std::ifstream f(path, std::ios::binary);
    int cols, rows, tw, th;
    f.read((char*)&cols, sizeof(int));
    f.read((char*)&rows, sizeof(int));
    f.read((char*)&tw, sizeof(int));
    f.read((char*)&th, sizeof(int));
    Tilemap map(cols, rows, tw, th);
    for (int y = 0; y < rows; y++)
        for (int x = 0; x < cols; x++) {
            int id;
            f.read((char*)&id, sizeof(int));
            map.Set(x, y, id);
        }
    return map;
}

// Works the same way for IsoTilemap
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

Custom colors:
```ks
let myColor: Color = Color(0.2, 0.8, 1.0, 1.0);   # KonScript
```
```cpp
Color myColor(0.2f, 0.8f, 1.0f, 1.0f);             // C++
```

---

## Vector2 & Random

### Vector2

**KonScript:**
```ks
let pos: Vec2 = Vec2(100.0, 200.0);
let vel: Vec2 = Vec2(1.0, 0.0);

# Vec2 methods work directly
let len: F64   = vel.Length();
let dir: Vec2  = vel.Normalized();
let d: F64     = pos.Distance(other);
let dot: F64   = vel.Dot(other);
let r: Vec2    = vel.Reflected(wallNormal);
let rot: Vec2  = vel.Rotated(0.785);  # radians (pi/4)
```

**C++:**
```cpp
Vector2 pos(100, 200);
Vector2 vel(1, 0);

float len   = vel.Length();
Vector2 dir = vel.Normalized();
float d     = pos.Distance(other);
float dot   = vel.Dot(other);
Vector2 r   = vel.Reflected(wallNormal);
Vector2 rot = vel.Rotated(3.14159f / 4);

// Interpolation
Vector2 mid = Vector2::Lerp(a, b, 0.5f);

// Presets
Vector2 zero  = Vector2::Zero();   // (0, 0)
Vector2 up    = Vector2::Up();     // (0, -1)
Vector2 right = Vector2::Right();  // (1, 0)
```

**Vec2 methods:** `Length()`, `LengthSq()`, `Normalized()`, `Dot(other)`, `Distance(other)`, `DistanceSq(other)`, `Rotated(angle)`, `Reflected(normal)`

**Static methods (C++):** `Vector2::Lerp(a, b, t)`, `Vector2::Zero()`, `Vector2::One()`, `Vector2::Up()`, `Vector2::Down()`, `Vector2::Left()`, `Vector2::Right()`

### Random

**KonScript:**
```ks
Random.Seed();                     # random seed from clock
Random.Seed(42);                   # deterministic seed

let r: I32  = Random.Range(1, 6);       # 1-6 inclusive
let f: F64  = Random.RangeF(0.5, 1.5);  # float range
let v: F64  = Random.Value();            # 0.0 - 1.0
let b: Bool = Random.Bool(0.3);          # 30% chance true
```

**C++:**
```cpp
Random::Seed();
Random::Seed(42);

int r   = Random::Range(1, 6);
float f = Random::RangeF(0.5f, 1.5f);
float v = Random::Value();
bool b  = Random::Bool(0.3f);

// Random element from a vector (C++ only)
std::vector<std::string> names = {"Alice", "Bob", "Carol"};
std::string pick = Random::From(names);
```

| Function | Description |
|----------|-------------|
| `Random.Seed()` / `Random.Seed(n)` | Seed the RNG (call once at startup) |
| `Random.Range(min, max)` | Random integer in [min, max] inclusive |
| `Random.RangeF(min, max)` | Random float in [min, max] |
| `Random.Value()` | Random float 0.0 to 1.0 |
| `Random.Bool(probability)` | Random bool (0.0 = never, 1.0 = always) |
| `Random.From(vec)` | Random element from a vector (C++ only) |

---

## Timers

Frame-rate independent timers for gameplay events. Uses delta time so behavior is consistent regardless of FPS.

**KonScript:**
```ks
# One-shot timer: fires once after 2 seconds
Timer.Create("explode", 2.0, false, [&]() {
    Print("BOOM!");
});

# Repeating timer: fires every 0.5 seconds
Timer.Create("spawn", 0.5, true, [&]() {
    Print("Enemy spawned!");
});

# In game loop — MUST call this each frame
Timer.UpdateAll(GetDeltaTime());

# Controls
Timer.Pause("spawn");
Timer.Resume("spawn");
Timer.Reset("spawn");       # restart from 0
Timer.Remove("spawn");      # delete timer
Timer.RemoveAll();           # delete all timers

# Queries
let exists: Bool = Timer.Exists("spawn");
let done: Bool = Timer.Finished("explode");
let remaining: F64 = Timer.Remaining("explode");
```

**C++:**
```cpp
// One-shot
TimerCreate("explode", 2.0f, false, []{ printf("BOOM!\n"); });

// Repeating
TimerCreate("spawn", 0.5f, true, [&]{ spawnEnemy(); });

// In game loop
TimerUpdateAll(GetDeltaTime());

// Controls
TimerPause("spawn");
TimerResume("spawn");
TimerReset("spawn");
TimerRemove("spawn");

// Queries
bool exists = TimerExists("spawn");
float remaining = TimerRemaining("explode");
```

### Full Timer Example (KonScript)

```ks
#include <engine>

func main() {
    InitWindow(600, 400, "Timer Demo", false);
    SetTargetFPS(60);

    let mut score: I32 = 0;
    let mut message: Str = "Waiting...";

    # Repeating timer: adds score every second
    Timer.Create("score_tick", 1.0, true, [&]() {
        score = score + 10;
    });

    # One-shot timer: shows a message after 3 seconds
    Timer.Create("welcome", 3.0, false, [&]() {
        message = "Welcome to the game!";
    });

    # Repeating timer: blink effect every 0.5s
    let mut blink: Bool = true;
    Timer.Create("blink", 0.5, true, [&]() {
        blink = !blink;
    });

    while !WindowShouldClose() {
        let dt: F64 = GetDeltaTime();

        # IMPORTANT: must call every frame to tick all timers
        Timer.UpdateAll(dt);

        # Pause/resume with spacebar
        if KeyPressed(Key.Space) {
            if Timer.Exists("score_tick") {
                Timer.Pause("score_tick");
            }
        }

        ClearBackground(0.05, 0.05, 0.1);
        DrawText(message, 10.0, 10.0, 20, WHITE);

        if blink {
            DrawText("Score: " + ToString(score), 10.0, 40.0, 24, YELLOW);
        }

        # Show remaining time on welcome timer
        if !Timer.Finished("welcome") {
            let rem: F64 = Timer.Remaining("welcome");
            DrawText("Welcome in: " + ToString(rem) + "s", 10.0, 80.0, 16, GRAY);
        }

        Present();
        PollEvents();
    }
}
```

| Function | Description |
|----------|-------------|
| `Timer.Create(id, duration, repeating, callback)` | Create a timer with a lambda callback |
| `Timer.UpdateAll(dt)` | Tick all timers (call each frame with `GetDeltaTime()`) |
| `Timer.Pause(id)` / `Resume(id)` | Pause/resume a timer |
| `Timer.Reset(id)` | Restart timer from 0 |
| `Timer.Remove(id)` / `RemoveAll()` | Delete timers |
| `Timer.Exists(id)` | Check if timer exists |
| `Timer.Finished(id)` | True if one-shot timer has fired |
| `Timer.Remaining(id)` | Seconds left until next fire |

> **Note:** Timer callbacks use C++-style lambda syntax: `[&]() { ... }`. The `[&]` capture lets the lambda access and modify variables from the surrounding scope.

---

## Text Boxes (Dialogue)

`UITextBox` provides word-wrapped text with an optional typewriter effect — characters appear one at a time, perfect for dialogue boxes.

**KonScript:**
```ks
# Simple text box (instant, word-wrapped)
UI.AddTextBox("desc", "This is a long description that will automatically wrap words to fit within the box width.", 50.0, 400.0, 700.0, 120.0);

# Dialogue box with typewriter effect
UI.AddTextBox("dialogue", "Hello, adventurer! Welcome to the village. The elder would like to speak with you about a quest...", 50.0, 450.0, 700.0, 100.0, true, 40.0);

# Connect to finished signal
UI.Connect("dialogue", "finished", func() {
    Print("Dialogue finished typing!");
});

# In game loop
UI.Update();
UI.Draw();

# Skip typewriter (e.g. on button press)
if KeyPressed(Key.Space) {
    # Get the element and call Skip — or just set a new text
}
```

**C++:**
```cpp
// Simple wrapped text
UIAddTextBox("desc", "Long text here...", 50, 400, 700, 120);

// Typewriter dialogue box (40 chars/sec)
auto* dlg = UIAddTextBox("dialogue",
    "Hello, adventurer! Welcome to the village...",
    50, 450, 700, 100, true, 40.0f);

// Connect to finished signal
UIConnect("dialogue", "finished", []{ printf("Done typing!\n"); });

// Skip typewriter on keypress
if (IsKeyPressed(Key::Space) && dlg->IsTyping()) {
    dlg->Skip();
}

// Change text (resets typewriter)
dlg->SetText("New dialogue line here...");

// Style
dlg->backgroundColor = Color(0.0f, 0.0f, 0.0f, 0.85f);
dlg->textColor = WHITE;
dlg->fontSize = 20;
dlg->paddingX = 16;
dlg->paddingY = 12;
```

---

## Signals

Nodes have a lightweight signal system for decoupled communication. Connect callbacks to named signals, then emit them from anywhere.

**KonScript:**
```ks
node Player : Node2D {
    let mut hp: I32 = 100;

    func Ready() {
        # Connect to your own signal
        Connect("player_dead", func() {
            Print("I died!");
        });
    }

    func Update(dt: F64) {
        if hp <= 0 {
            Emit("player_dead");
        }
    }

    # Collision signals fire automatically from Collider2D children
    func OnCollisionEnter(other: Collider2D) {
        Print("Hit: ", other.name);
    }

    func OnCollisionExit(other: Collider2D) {
        Print("Left: ", other.name);
    }
}
```

**C++:**
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

// Collision signals are automatic on parent nodes of Collider2D children
void OnCollisionEnter(Collider2D* other) override {
    printf("Hit: %s\n", other->name.c_str());
}
void OnCollisionExit(Collider2D* other) override {
    printf("Left: %s\n", other->name.c_str());
}
```

**Built-in collision signals:**
- `OnCollisionEnter(other)` -- called when a child collider first overlaps another
- `OnCollisionExit(other)` -- called when they stop overlapping
- These are virtual methods on Node, not string signals -- override them directly

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

# Query design resolution and scaling info
let dw: I32  = GetDesignWidth();
let dh: I32  = GetDesignHeight();
let scale: F64 = GetLetterboxScale();

# Get mouse position in game (design-resolution) coordinates
let gmx: F64 = GetGameMouseX();
let gmy: F64 = GetGameMouseY();

SetVsync(true);
```

**C++:**
```cpp
InitWindow(800, 600, "My Game", true);

int dw = GetDesignWidth();
int dh = GetDesignHeight();
float scale = GetLetterboxScale();
float offX  = GetLetterboxOffsetX();
float offY  = GetLetterboxOffsetY();

// Mouse in game coordinates
float gmx = GetGameMouseX();
float gmy = GetGameMouseY();

SetVsync(true);
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

## UI System

Screen-space UI elements that stay fixed on screen regardless of camera. Buttons, labels, and panels for menus and HUDs.

### Creating Elements

**KonScript:**
```ks
# Button — auto-sizes from text, has hover/pressed visual states
UI.AddButton("play", "Play Game", 300.0, 200.0);
UI.AddButton("quit", "Quit", 300.0, 260.0);

# Label — static text
UI.AddLabel("title", "My Game", 280.0, 100.0, 32, WHITE);

# Image — display a texture
let tex: Texture = LoadTexture("logo.png");
UI.AddImage("logo", tex, 350.0, 30.0, 100.0, 100.0);

# Panel — colored container
UI.AddPanel("menu", 250.0, 80.0, 300.0, 240.0);
UI.PanelAddChild("menu", "title");
UI.PanelAddChild("menu", "play");
UI.PanelAddChild("menu", "quit");
# Child positions become relative to the panel's top-left
```

**C++:**
```cpp
UIAddButton("play", "Play Game", 300, 200);
UIAddButton("quit", "Quit", 300, 260);
UIAddLabel("title", "My Game", 280, 100, 32, WHITE);

Texture logo = LoadTexture("logo.png");
UIAddImage("logo", logo, 350, 30, 100, 100);  // auto-sizes from texture if w/h omitted

auto* panel = UIAddPanel("menu", 250, 80, 300, 240);
UIPanelAddChild("menu", "title");
UIPanelAddChild("menu", "play");
UIPanelAddChild("menu", "quit");
```

### Handling Clicks (Callbacks & Signals)

Two ways to handle button clicks: direct callbacks or signals.

**KonScript:**
```ks
# Direct callback (simple)
UI.OnClick("play", func() {
    Print("Play clicked!");
});

# Signal system (flexible — supports multiple listeners)
UI.Connect("play", "clicked", func() {
    Print("Play clicked via signal!");
});

UI.Connect("play", "hovered", func() {
    Print("Mouse entered play button");
});

UI.Connect("play", "unhovered", func() {
    Print("Mouse left play button");
});
```

**C++:**
```cpp
// Direct callback
UIOnClick("play", []{ printf("Play!\n"); });

// Signal system
UIConnect("play", "clicked", []{ printf("Clicked via signal!\n"); });
UIConnect("play", "hovered", []{ printf("Hovered!\n"); });
UIConnect("play", "unhovered", []{ printf("Left!\n"); });
```

**Available signals:**

| Signal | Fired when |
|--------|-----------|
| `"clicked"` | Button is clicked |
| `"hovered"` | Mouse enters the button |
| `"unhovered"` | Mouse leaves the button |

### Button Images

Buttons can have an icon (displayed left of text) and/or a background texture.

**C++:**
```cpp
auto* btn = UIAddButton("shop", "Shop", 300, 200);

// Icon — small texture displayed left of the text
Texture shopIcon = LoadTexture("shop_icon.png");
btn->SetIcon(shopIcon, 20, 20);  // display size 20x20

// Background texture — replaces the solid color fill
Texture btnNormal = LoadTexture("btn_normal.png");
Texture btnHover  = LoadTexture("btn_hover.png");
Texture btnPress  = LoadTexture("btn_pressed.png");
btn->SetBackground(btnNormal, btnHover, btnPress);
```

Panels also support background textures:
```cpp
auto* panel = UIAddPanel("hud", 0, 0, 800, 60);
panel->background = LoadTexture("hud_bg.png");
```

### Game Loop Integration

Call `UIUpdate()` and `UIDrawAll()` **after** `scene.draw()` (after EndCamera2D). Check `UIWantsInput()` before processing world clicks.

**KonScript:**
```ks
while !WindowShouldClose() {
    let dt: F64 = GetDeltaTime();
    ClearBackground(0.1, 0.1, 0.15);
    scene.update(dt);
    scene.draw();

    UI.Update();    # hit-test mouse against UI elements
    UI.Draw();      # render UI in screen-space (not affected by camera)

    if !UI.WantsInput() && MousePressed(Mouse.Left) {
        # handle world clicks here — UI didn't consume the click
    }

    Present();
    PollEvents();
}
```

**C++:**
```cpp
while (!WindowShouldClose()) {
    float dt = GetDeltaTime();
    ClearBackground(0.1f, 0.1f, 0.15f);
    scene.Update(dt);
    scene.Draw();

    UIUpdate();
    UIDrawAll();

    if (!UIWantsInput() && IsMouseButtonPressed(Mouse::Left)) {
        // world clicks
    }

    Present(); PollEvents();
}
```

### Button Styling (C++)

Buttons have sensible defaults but can be customized:

```cpp
auto* btn = UIAddButton("play", "Play", 300, 200);
btn->normalColor  = Color(0.2f, 0.5f, 0.2f, 1.0f);  // green
btn->hoverColor   = Color(0.3f, 0.7f, 0.3f, 1.0f);
btn->pressedColor = Color(0.1f, 0.3f, 0.1f, 1.0f);
btn->textColor    = WHITE;
btn->borderColor  = Color(0.4f, 0.8f, 0.4f, 1.0f);
btn->fontSize     = 24;
btn->paddingX     = 20;  // auto-size padding
btn->paddingY     = 12;
```

### Managing Elements

```ks
UI.Remove("play");    # remove a single element
UI.Clear();           # remove all elements
```

### API Reference

| Function | Description |
|----------|-------------|
| `UI.AddButton(id, text, x, y)` | Create a button (auto-sizes from text) |
| `UI.AddLabel(id, text, x, y, fontSize?, color?)` | Create a text label |
| `UI.AddPanel(id, x, y, w, h)` | Create a panel container |
| `UI.AddImage(id, tex, x, y, w?, h?)` | Display a texture (auto-sizes from texture) |
| `UI.PanelAddChild(panelId, childId)` | Add element as panel child |
| `UI.OnClick(id, callback)` | Set button click handler (shortcut) |
| `UI.Connect(id, signal, callback)` | Connect to any UI signal |
| `UI.Update()` | Per-frame hit testing (call before Draw) |
| `UI.Draw()` | Render all visible elements |
| `UI.WantsInput()` | True if UI consumed mouse this frame |
| `UI.Remove(id)` | Remove element by ID |
| `UI.Clear()` | Remove all elements |

---

## Examples

The `examples/` directory contains ready-to-run KonScript demos:

| Example | What it tests |
|---------|---------------|
| `physics_test.ks` | KinematicBody2D, gravity, jumping, StaticBody2D walls/platforms, collision signals, floor detection |
| `tilemap_test.ks` | Tile placement/removal, grid overlay, camera pan/zoom, world-space click detection, tile selection |
| `platformer.ks` | Simple platformer with KinematicBody2D and MoveAndCollide |
| `hello_world.ks` | Basic window, drawing, and game loop |
| `pong.ks` | Classic pong with collision |
| `cpp_example/` | C++ example with CMake integration, camera follow, collision callbacks |

Run any KonScript example:
```bash
ksc examples/physics_test.ks
ksc examples/tilemap_test.ks
```

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
