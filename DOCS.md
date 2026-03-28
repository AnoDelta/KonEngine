# KonEngine Documentation

> Version 0.8.1 — Work in progress. API is stable but subject to change before v1.0.

---

## Table of Contents

1. [Window & Application](#1-window--application)
2. [Rendering](#2-rendering)
3. [Input](#3-input)
4. [Camera](#4-camera)
5. [Textures](#5-textures)
6. [Color](#6-color)
7. [Time](#7-time)
8. [Audio](#8-audio)
9. [Node System](#9-node-system)
10. [Scene](#10-scene)
11. [Node2D](#11-node2d)
12. [Sprite2D](#12-sprite2d)
13. [Collider2D & CollisionWorld](#13-collider2d--collisionworld)
14. [Animation](#14-animation)
15. [AnimationPlayer](#15-animationplayer)
16. [Easing Curves](#16-easing-curves)
17. [Debug Mode](#17-debug-mode)
18. [Math — Vector2](#18-math--vector2)
19. [Tools](#19-tools)

---

## 1. Window & Application

```cpp
#include "KonEngine.hpp"
```

All engine functions are available through this single header.

```cpp
void InitWindow(int width, int height, const std::string& title, bool canResize = false);
bool WindowShouldClose();
void Present();
void PollEvents();
void ClearBackground(float r, float g, float b);
int  GetWindowWidth();
int  GetWindowHeight();
void SetVsync(bool enabled);
```

`InitWindow` creates the window and initializes OpenGL. Must be called before anything else.
`Present` swaps the back buffer to the screen — call once at the end of every frame.
`PollEvents` processes input and advances the frame timer — call once per frame after `Present`.

### Typical game loop

```cpp
InitWindow(800, 600, "My Game");
SetTargetFPS(60);

while (!WindowShouldClose()) {
    ClearBackground(0.1f, 0.1f, 0.1f);

    // update + draw

    Present();
    PollEvents();
}
```

---

## 2. Rendering

All draw calls use a top-left origin coordinate system. X increases right, Y increases down.

### Rectangles

```cpp
void DrawRectangle(float x, float y, float w, float h, float r, float g, float b, float a = 1.0f);
void DrawRectangle(float x, float y, float w, float h, Color color);
```
`x, y` is the top-left corner.

### Circles

```cpp
void DrawCircle(float x, float y, float radius, float r, float g, float b, float a = 1.0f);
void DrawCircle(float x, float y, float radius, Color color);
```
`x, y` is the center.

### Lines

```cpp
void DrawLine(float x1, float y1, float x2, float y2, float r, float g, float b, float a = 1.0f);
void DrawLine(float x1, float y1, float x2, float y2, Color color);
```

### Textures

```cpp
void DrawTexture(Texture& texture, float x, float y, float width, float height);
void DrawTexture(Texture& texture, float x, float y, float width, float height, Color tint);

void DrawTextureRec(Texture& texture, float x, float y, float width, float height,
                    float srcX, float srcY, float srcWidth, float srcHeight);
void DrawTextureRec(Texture& texture, float x, float y, float width, float height,
                    float srcX, float srcY, float srcWidth, float srcHeight, Color tint);
```
`DrawTextureRec` draws a sub-region of a texture — used for sprite sheets.
`srcX/srcY/srcWidth/srcHeight` are in UV coordinates (0.0–1.0).

---

## 3. Input

### Keyboard

```cpp
bool IsKeyDown(Key::Code key);      // held this frame
bool IsKeyPressed(Key::Code key);   // pressed this frame only
bool IsKeyReleased(Key::Code key);  // released this frame only
```

**Key codes** (`Key::` namespace):
```
A–Z, Num0–Num9
Right, Left, Down, Up
Space, Enter, Esc, Tab, Backspace
Shift, Ctrl, Alt
F1–F12
```

### Mouse

```cpp
bool  IsMouseButtonDown(Mouse::Button button);
bool  IsMouseButtonPressed(Mouse::Button button);
bool  IsMouseButtonReleased(Mouse::Button button);
float GetMouseX();
float GetMouseY();
float GetMouseDeltaX();
float GetMouseDeltaY();
float GetMouseScroll();
```

**Mouse buttons:** `Mouse::Left`, `Mouse::Right`, `Mouse::Middle`

### Gamepad

```cpp
bool  IsGamepadConnected(int player);
bool  IsGamepadButtonDown(int player, Gamepad::Button button);
bool  IsGamepadButtonPressed(int player, Gamepad::Button button);
bool  IsGamepadButtonReleased(int player, Gamepad::Button button);
float GetGamepadAxis(int player, Gamepad::Axis axis);
```

**Gamepad buttons:** `A`, `B`, `X`, `Y`, `LeftBumper`, `RightBumper`, `Back`, `Start`,
`LeftThumb`, `RightThumb`, `DPadUp`, `DPadRight`, `DPadDown`, `DPadLeft`

**Gamepad axes:** `LeftX`, `LeftY`, `RightX`, `RightY`, `LeftTrigger`, `RightTrigger`

---

## 4. Camera

```cpp
Camera2D cam(float targetX, float targetY, float zoom, float rotation);
```

Creates a 2D camera centered on `(targetX, targetY)` with the given zoom and rotation.

```cpp
void BeginCamera2D(const Camera2D& cam);
void EndCamera2D();
```

Everything drawn between these two calls is transformed by the camera.

```cpp
BeginCamera2D(cam);
    scene.Draw();       // drawn in world space
EndCamera2D();
DrawRectangle(...);     // drawn in screen space, ignores camera
```

---

## 5. Textures

```cpp
Texture LoadTexture(const char* path);
void    UnloadTexture(Texture& texture);
```

Loads a PNG, JPG, BMP, or TGA image from disk. Always call `UnloadTexture` when done.

```cpp
struct Texture {
    unsigned int id;
    int width;
    int height;
};
```

---

## 6. Color

```cpp
struct Color { float r, g, b, a; };
```

**Presets:** `RED`, `GREEN`, `BLUE`, `WHITE`, `BLACK`, `YELLOW`, `CYAN`, `MAGENTA`,
`ORANGE`, `PURPLE`, `GRAY`, `DARKGRAY`, `TRANSPARENT`

---

## 7. Time

```cpp
void  SetTargetFPS(int fps);
float GetDeltaTime();  // seconds since last frame
float GetTime();       // total elapsed time in seconds
float GetFPS();        // current frames per second
```

Always use `GetDeltaTime()` for movement to keep things frame-rate independent:

```cpp
player->x += speed * GetDeltaTime();
```

---

## 8. Audio

```cpp
void PlaySound(const std::string& path);
void StopSound(const std::string& path);

void PlayMusic(const std::string& path, bool loop = true);
void StopMusic();
void PauseMusic();
void ResumeMusic();
void SetMusicVolume(float volume);  // 0.0 to 1.0
void SetSoundVolume(float volume);  // 0.0 to 1.0
```

Supported formats: `.wav`, `.ogg`, `.mp3`

---

## 9. Node System

The node system is the foundation of KonEngine's scene architecture.
Every object in a scene is a `Node` or a subclass of it.

### Node

```cpp
class Node {
public:
    std::string name;
    bool        active = true;
    Node*       parent = nullptr;

    virtual void Ready() {}
    virtual void Update(float dt) {}
    virtual void Draw() {}
    virtual void OnCollisionEnter(Collider2D* other) {}
    virtual void OnCollisionExit(Collider2D* other) {}
};
```

**Lifecycle methods** — override these in subclasses:

- `Ready()` — called once when the node is added to the scene via `scene.Add()`
- `Update(dt)` — called every frame
- `Draw()` — called every frame after Update
- `OnCollisionEnter(other)` — called when a child `Collider2D` first overlaps another collider
- `OnCollisionExit(other)` — called when they stop overlapping

Collision signals bubble up automatically from child `Collider2D` nodes to the parent node's `OnCollisionEnter/Exit`. The `other` parameter is the `Collider2D*` that was hit, not the parent node — use `other->name` to identify what was hit.

### Adding children

```cpp
template<typename T, typename... Args>
T* AddChild(const std::string& name, Args&&... args);
```

```cpp
auto* player = scene.Add<Player>("player");
auto* col    = player->AddChild<Collider2D>("hitbox");
// col->parent == player
```

`parent` is set automatically.

### Traversal

```cpp
Node* GetNode(const std::string& name);
void  ForEachDescendant(std::function<void(Node*)> fn);
void  RemoveChild(const std::string& name);
```

### Signals

```cpp
node->Connect("my_signal", [&]() { /* ... */ });
node->Emit("my_signal");
```

For `Collider2D`, signals pass a `Collider2D*`:
```cpp
collider->Connect("on_collision_enter", [](Collider2D* other) {
    // other->name, other->parent, etc.
});
```

### Custom nodes

```cpp
class Player : public Node2D {
public:
    float speed = 200.0f;

    void Ready() override {
        x = 100.0f;
        y = 400.0f;
    }

    void Update(float dt) override {
        if (IsKeyDown(Key::D)) x += speed * dt;
        if (IsKeyDown(Key::A)) x -= speed * dt;
    }

    void OnCollisionEnter(Collider2D* other) override {
        if (other->name == "enemy") TakeDamage();
    }
};
```

---

## 10. Scene

```cpp
class Scene {
public:
    CollisionWorld collisionWorld;

    template<typename T, typename... Args>
    T* Add(const std::string& name, Args&&... args);

    void ScanColliders();
    void Remove(const std::string& name);
    Node* GetNode(const std::string& name);

    void Update(float dt);
    void Draw();
};
```

`Scene::Add` creates the node, adds it to the scene, calls `Ready()`, then registers
any `Collider2D` descendants with the `CollisionWorld`.

If you add `Collider2D` children **after** `scene.Add()` returns, call
`scene.ScanColliders()` once to register them:

```cpp
auto* player = scene.Add<Player>("player");
auto* col    = player->AddChild<Collider2D>("hitbox");
col->width   = 32.0f;
scene.ScanColliders();
```

`Scene::Update` each frame:
1. Updates all nodes (moves parents)
2. Propagates world transforms down to all `Node2D` children
3. Runs collision checks — colliders now have correct world positions
4. Restores children to local space

`Scene::Draw` draws collider outlines automatically when `DebugMode(true)` is active.

---

## 11. Node2D

Extends `Node` with 2D transform properties.

```cpp
class Node2D : public Node {
public:
    float x = 0, y = 0;
    float scaleX = 1, scaleY = 1;
    float rotation = 0;
    float originX = 0.5f;  // pivot: 0 = left, 0.5 = center, 1 = right
    float originY = 0.5f;

    void  Move(float dx, float dy);
    float DrawX(float width)  const;  // top-left X accounting for origin
    float DrawY(float height) const;  // top-left Y accounting for origin
};
```

Child nodes are stored in **local space** relative to their parent. Before each
collision check, `Scene::Update` propagates parent world positions down to all
`Node2D` children, so `Collider2D` nodes always reflect the correct world position.

---

## 12. Sprite2D

```cpp
class Sprite2D : public Node2D {
public:
    Texture texture  = {0, 0, 0};
    float   width    = 64, height = 64;
    Color   tint     = WHITE;

    bool  useSourceRect = false;
    float srcX = 0, srcY = 0, srcWidth = 64, srcHeight = 64;

    void SetTexture(Texture& tex);
};
```

`SetTexture` sets the texture and updates `width`/`height` to match.
`useSourceRect` and `srcX/srcY/srcWidth/srcHeight` are set automatically by
`AnimationPlayer` — you don't need to set them manually when using animation.

---

## 13. Collider2D & CollisionWorld

### Collider2D

```cpp
class Collider2D : public Node2D {
public:
    ColliderShape shape  = ColliderShape::Rectangle;
    float width  = 32.0f;
    float height = 32.0f;
    float radius = 16.0f;
    std::vector<glm::vec2> points;  // used when shape == Custom

    uint32_t layer = 1;
    uint32_t mask  = 1;

    bool  debugDraw  = false;
    Color debugColor = { 0, 1, 0, 0.8f };
};
```

**Shapes:** `ColliderShape::Rectangle`, `ColliderShape::Circle`, `ColliderShape::Custom`

**Layer/mask filtering** — a collision only fires if `(a.layer & b.mask) || (b.layer & a.mask)`.

**Signals:**
- `on_collision_enter` — fired once when two colliders first overlap
- `on_collision_exit` — fired once when they stop overlapping

Collision signals also bubble up to the parent node's `OnCollisionEnter/Exit` automatically.

### Typical setup

```cpp
auto* player = scene.Add<Player>("player");
auto* col    = player->AddChild<Collider2D>("hitbox");
col->width   = 28.0f;
col->height  = 44.0f;
scene.ScanColliders();
```

### CollisionWorld

Managed automatically by `Scene`. Can also be used manually:

```cpp
CollisionWorld world;
world.Add(&colliderA);
world.Add(&colliderB);
world.Update();  // checks all pairs, fires signals

bool hit = CollisionWorld::Overlaps(&a, &b);  // direct test, no signals
```

---

## 14. Animation

```cpp
struct AnimationFrame {
    float srcX, srcY, srcWidth, srcHeight;
    float duration;  // seconds
};

struct KeyframeTrack {
    std::string name;  // "x", "y", "scaleX", "alpha", etc.
    KeyframeTrack& AddKey(float time, float value, Ease curve = Ease::Linear);
    float Sample(float time) const;
};

struct Animation {
    std::string name;
    float       duration = 0.0f;
    bool        loop     = false;

    Animation& AddFrame(float srcX, float srcY, float srcW, float srcH, float dur = 0.1f);
    KeyframeTrack& Track(const std::string& name);
    void AutoDuration();
};
```

Animations are stored in `.anim` text files and compiled to `.konani` binary by
`anim_compiler`. They can also be built in code:

```cpp
Animation run("run", true);
run.AddFrame(0,  0, 32, 32, 0.1f);
run.AddFrame(32, 0, 32, 32, 0.1f);

Animation jump("jump", false);
jump.Track("y").AddKey(0.0f, 400.0f).AddKey(0.3f, 300.0f, Ease::EaseOut)
               .AddKey(0.6f, 400.0f, Ease::EaseIn);
jump.AutoDuration();
```

---

## 15. AnimationPlayer

```cpp
class AnimationPlayer : public Node {
public:
    float speed = 1.0f;

    void Add(const Animation& anim);
    void Play(const std::string& name);
    void Stop();
    void Pause();
    void Resume();
    void SetLoop(const std::string& name, bool loop);

    bool        IsPlaying()  const;
    bool        IsFinished() const;
    std::string GetCurrent() const;
};
```

`AnimationPlayer` auto-detects a `Sprite2D` parent and drives its source rect for
sprite sheet animation. It also applies keyframe tracks to the parent node's properties.

```cpp
auto* sprite = scene.Add<Sprite2D>("player");
auto* anim   = sprite->AddChild<AnimationPlayer>("anim");

Animation run("run", true);
run.AddFrame(0, 0, 32, 32, 0.1f);
run.AddFrame(32, 0, 32, 32, 0.1f);
anim->Add(run);
anim->Play("run");
```

---

## 16. Easing Curves

Used in keyframe animation tracks. All curves take `t` in `[0, 1]`.

| Name | Feel |
|---|---|
| `Linear` | Constant speed |
| `EaseIn` | Starts slow, ends fast |
| `EaseOut` | Starts fast, ends slow |
| `EaseInOut` | Slow-fast-slow |
| `EaseInCubic` / `EaseOutCubic` / `EaseInOutCubic` | Stronger versions |
| `EaseInElastic` / `EaseOutElastic` / `EaseInOutElastic` | Springy overshoot |
| `EaseInBounce` / `EaseOutBounce` / `EaseInOutBounce` | Bouncing |
| `EaseInBack` / `EaseOutBack` / `EaseInOutBack` | Slight overshoot then settle |

```cpp
float t = Curves::Apply(Ease::EaseOutBack, rawT);
```

---

## 17. Debug Mode

```cpp
void DebugMode(bool enabled);
bool IsDebugMode();
```

When enabled:
- Red border drawn around the window
- Red crosshair drawn at the mouse cursor position
- FPS, mouse position, and delta time printed to stdout every second
- All `Collider2D` nodes in the active scene have their shapes drawn automatically

```cpp
DebugMode(true);
InitWindow(800, 600, "My Game");
```

---

## 18. Math — Vector2

```cpp
struct Vector2 {
    float x, y;

    Vector2(float x = 0, float y = 0);

    // Arithmetic: +, -, *, /, +=, -=, *=, /=, unary -
    // Comparison: ==, !=

    float   Length() const;
    float   LengthSq() const;
    Vector2 Normalized() const;
    float   Dot(const Vector2& other) const;
    float   Distance(const Vector2& other) const;
    float   DistanceSq(const Vector2& other) const;
    Vector2 Rotated(float angleRadians) const;
    Vector2 Reflected(const Vector2& normal) const;

    static Vector2 Lerp(const Vector2& a, const Vector2& b, float t);
    static Vector2 Zero();
    static Vector2 One();
    static Vector2 Up();
    static Vector2 Down();
    static Vector2 Left();
    static Vector2 Right();
};
```

```cpp
Vector2 vel = Vector2::Right() * 200.0f;
vel = vel.Rotated(angle);
pos += vel * GetDeltaTime();
```

---

## 19. Tools

KonEngine ships with companion tools.

### KonScript

A scripting language that compiles to C++ and links against KonEngine.
Write games without touching C++ directly.

See **[tools/KonScript/DOCS.md](tools/KonScript/DOCS.md)** for the full language reference.

```ks
#include <engine>

node Player : Node2D {
    let mut speed: F64 = 200.0;

    func Ready() {
        x = 100.0;
        y = 400.0;
    }

    func Update(dt: F64) {
        if KeyDown(Key.D) { x += speed * dt; }
        if KeyDown(Key.A) { x -= speed * dt; }
    }

    func OnCollisionEnter(other: Collider2D) {
        if other.name == "enemy" {
            Print("Hit!\n");
        }
    }
}

func main() {
    InitWindow(800, 600, "My Game");
    SetTargetFPS(60);

    let scene: Scene = Scene();
    let player: Player = scene.add(Player, "player");
    let col: Collider2D = player.add(Collider2D, "hitbox");
    col.width  = 32.0;
    col.height = 48.0;
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

**Usage:**
```bash
ksc main.ks            # compile and run
ksc main.ks --keep     # keep the generated .cpp
ksc --compile main.ks  # compile only
ksc --check main.ks    # typecheck only
```

**CMake integration:**
```cmake
add_subdirectory(KonEngine)
add_executable(MyGame)
target_link_libraries(MyGame PRIVATE KonEngine)
konscript_sources(MyGame src/main.ks)
```

See `tools/KonScript/DOCS.md` for the full KonScript language reference.

### KonAnimator

Standalone Qt animation editor. Visual spritesheet frame editor, keyframe timeline,
live OpenGL preview. Saves `.anim` files and compiles to `.konani`.

See **[tools/KonAnimator/DOCS.md](tools/KonAnimator/DOCS.md)** for the full reference.

```bash
./build-tools.sh  # builds KonAnimator and anim_compiler
```

### KonPaktor

Asset packer — bundles and encrypts game assets into `.konpak` files for distribution.

See **[tools/KonPaktor/DOCS.md](tools/KonPaktor/DOCS.md)** for the full reference.

```bash
konpak create game.konpak assets/ --pass mypassword
konpak list   game.konpak
konpak extract game.konpak --out ./out
```
