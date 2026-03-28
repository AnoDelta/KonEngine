# KonEngine Documentation

> Version 0.8.0 — Work in progress. API is stable but subject to change before v1.0.

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

---

## 1. Window & Application

```cpp
#include "KonEngine.hpp"
```

All engine functions are available through this single header.

### Functions

```cpp
void InitWindow(int width, int height, const std::string& title, bool canResize = false);
```
Creates the window and initializes OpenGL. Must be called before anything else.

```cpp
bool WindowShouldClose();
```
Returns `true` when the user closes the window or presses the OS close button.

```cpp
void Present();
```
Swaps the back buffer to the screen. Call once at the end of every frame.

```cpp
void PollEvents();
```
Processes input events and advances the frame timer. Call once per frame after `Present()`.

```cpp
void ClearBackground(float r, float g, float b);
```
Clears the screen to the given RGB color (values 0.0–1.0). Call at the start of every frame.

```cpp
int GetWindowWidth();
int GetWindowHeight();
void SetVsync(bool enabled);
```

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
Draws a filled rectangle. `x`, `y` is the top-left corner.

### Circles

```cpp
void DrawCircle(float x, float y, float radius, float r, float g, float b, float a = 1.0f);
void DrawCircle(float x, float y, float radius, Color color);
```
Draws a filled circle. `x`, `y` is the center.

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
Space, Enter, Escape, Tab, Backspace
Shift, Ctrl, Alt
F1–F12
```

### Mouse

```cpp
bool IsMouseButtonDown(Mouse::Button button);
bool IsMouseButtonPressed(Mouse::Button button);
bool IsMouseButtonReleased(Mouse::Button button);

float GetMouseX();
float GetMouseY();
float GetMouseDeltaX();   // movement since last frame
float GetMouseDeltaY();
float GetMouseScroll();   // scroll wheel delta
```

**Mouse buttons:** `Mouse::Left`, `Mouse::Right`, `Mouse::Middle`

### Gamepad

```cpp
bool IsGamepadConnected(int player);
bool IsGamepadButtonDown(int player, Gamepad::Button button);
bool IsGamepadButtonPressed(int player, Gamepad::Button button);
bool IsGamepadButtonReleased(int player, Gamepad::Button button);
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
    scene.Draw();      // drawn in world space
EndCamera2D();

DrawRectangle(...);    // drawn in screen space, ignores camera
```

---

## 5. Textures

```cpp
Texture LoadTexture(const char* path);
void UnloadTexture(Texture& texture);
```

Loads a PNG, JPG, BMP, or TGA image from disk. Always call `UnloadTexture` when done.

```cpp
struct Texture {
    unsigned int id;    // OpenGL texture ID
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
void SetTargetFPS(int fps);
float GetDeltaTime();    // time since last frame in seconds
float GetFPS();
float GetTime();         // total elapsed time in seconds
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
};
```

### Adding children

```cpp
template<typename T, typename... Args>
T* AddChild(const std::string& name, Args&&... args);
```

```cpp
auto* sprite = scene.Add<Sprite2D>("player");
auto* anim   = sprite->AddChild<AnimationPlayer>("anim");
// anim->parent == sprite
```

`parent` is set automatically when `AddChild` is called.

### Traversal

```cpp
Node* GetNode(const std::string& name);  // searches entire subtree
void  ForEachDescendant(std::function<void(Node*)> fn);
void  RemoveChild(const std::string& name);
```

### Signals

Nodes can emit and receive typed signals:

```cpp
// Connect a listener
node->Connect("my_signal", [&]() {
    std::cout << "fired!\n";
});

// Emit
node->Emit("my_signal");
```

For `Collider2D`, signals pass a `Collider2D*` argument:
```cpp
collider->Connect("on_collision_enter", [](Collider2D* other) {
    std::cout << "Hit: " << other->name << "\n";
});
```

### Update and draw

```cpp
virtual void Update(float dt) {}
virtual void Draw() {}
```

Override these in your own node subclasses. `Scene::Update()` and `Scene::Draw()`
call these on all nodes and their children automatically.

### Custom nodes

```cpp
class Player : public Node2D {
public:
    float speed = 200.0f;

    void Update(float dt) override {
        if (IsKeyDown(Key::D)) x += speed * dt;
        if (IsKeyDown(Key::A)) x -= speed * dt;
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

    void Remove(const std::string& name);
    Node* GetNode(const std::string& name);

    void Update(float dt);
    void Draw();
};
```

`Scene::Add` registers `Collider2D` nodes with the `CollisionWorld` automatically.
`Scene::Update` runs collision checks before updating nodes.
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

    void Move(float dx, float dy);

    float DrawX(float width)  const;  // actual top-left X accounting for origin
    float DrawY(float height) const;  // actual top-left Y accounting for origin
};
```

Children inherit their parent's transform — a child at `(0, 0)` will appear at the
parent's position.

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

`SetTexture` sets the texture and updates `width`/`height` to match the texture size
(unless `useSourceRect` is already enabled, in which case they're left alone).

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
    float radius = 16.0f;   // used when shape == Circle
    std::vector<glm::vec2> points; // used when shape == Custom

    uint32_t layer = 1;
    uint32_t mask  = 1;

    bool  debugDraw  = false;
    Color debugColor = { 0, 1, 0, 0.8f };
};
```

**Shapes:** `ColliderShape::Rectangle`, `ColliderShape::Circle`, `ColliderShape::Custom`

**Layer/mask filtering** works like Godot:
a collision only fires if `(a.layer & b.mask) || (b.layer & a.mask)`.

**Signals:**
- `on_collision_enter` — fired once when two colliders first overlap
- `on_collision_exit` — fired once when they stop overlapping

### CollisionWorld

The `Scene` manages a `CollisionWorld` automatically. You can also use it manually:

```cpp
CollisionWorld world;
world.Add(&colliderA);
world.Add(&colliderB);
world.Update();  // checks all pairs, fires signals

// Direct overlap test (no signals)
bool overlapping = CollisionWorld::Overlaps(&a, &b);
```

---

## 14. Animation

### The .anim file format

```
# comment
anim <name> [loop]
  display <width> <height> <scale>
  frame <srcX> <srcY> <srcW> <srcH> <duration>
  track <property> <time> <value> [curve]
end
```

- `display` — the display size in pixels and a scale multiplier
- `frame` — one frame of a sprite sheet animation
- `track` — a keyframe on a property track

**Animatable properties:** `x`, `y`, `scaleX`, `scaleY`, `rotation`, `alpha`

**Multiple animations in one file:**
```
anim idle loop
  display 32 32 1.0
  frame 0 0 32 32 0.12
  frame 32 0 32 32 0.12
end

anim jump
  display 32 32 1.0
  frame 64 0 32 32 0.08
  frame 96 0 32 32 0.08
  track scaleY 0.0 1.0 easeout
  track scaleY 0.2 1.3 easeout
  track scaleY 0.4 1.0 easein
end
```

### Compiling

```bash
# CLI
./anim_compiler player.anim

# GUI
./anim_compiler
```

Or use **KonAnimator** for a full visual editor.

---

## 15. AnimationPlayer

```cpp
class AnimationPlayer : public Node {
public:
    Sprite2D* target = nullptr;  // auto-set from parent if null
    Node2D*   node   = nullptr;  // auto-set from parent if null
    float     speed  = 1.0f;

    AnimationPlayer& Add(const Animation& anim);
    bool LoadFromFile(const std::string& path);

    void Play(const std::string& animName);
    void Pause();
    void Resume();
    void Stop();

    bool IsPlaying()  const;
    bool IsFinished() const;
    const std::string& GetCurrent() const;
    int   GetCurrentFrame() const;
    float GetElapsed() const;
};
```

When `Play()` is called, `AnimationPlayer` automatically:
- Detects the parent `Sprite2D` if `target`/`node` aren't set
- Enables `useSourceRect` on the target
- Sets `width`/`height` from the clip's `display` values

**Signals:**
- `animation_finished` — emitted when a non-looping animation completes

```cpp
anim->Connect("animation_finished", [&]() {
    anim->Play("idle");
});
```

### Typical setup

```cpp
auto* sprite = scene.Add<Sprite2D>("player");
sprite->SetTexture(LoadTexture("player.png"));

auto* anim = sprite->AddChild<AnimationPlayer>("anim");
anim->LoadFromFile("player.konani");
anim->Play("idle");
```

---

## 16. Easing Curves

Used in keyframe animation tracks. All curves take `t` in `[0, 1]`.

| Name | Feel |
|---|---|
| `linear` | Constant speed |
| `easein` | Starts slow, ends fast (quadratic) |
| `easeout` | Starts fast, ends slow (quadratic) |
| `easeinout` | Slow-fast-slow (quadratic) |
| `easeincubic` / `easeoutcubic` / `easeinoutcubic` | Stronger versions |
| `easeinelastic` / `easeoutelastic` / `easeinoutelastic` | Springy overshoot |
| `easeinbounce` / `easeoutbounce` / `easeinoutbounce` | Bouncing |
| `easeinback` / `easeoutback` / `easeinoutback` | Slight overshoot then settle |

You can also use curves directly in C++:
```cpp
float t = Curves::Apply(Ease::EaseOutBack, rawT);
```

---

## 17. Debug Mode

```cpp
void DebugMode(bool enabled);
bool IsDebugMode();
```

When enabled (call before or after `InitWindow`):

- Red border drawn around the window
- Red crosshair drawn at the mouse cursor position
- FPS, mouse position, and delta time printed to stdout every second
- All `Collider2D` nodes in the active scene have their shapes drawn automatically,
  regardless of their individual `debugDraw` flag

```cpp
int main() {
    DebugMode(true);
    InitWindow(800, 600, "My Game");
    // ...
}
```

Disable it before shipping:
```cpp
DebugMode(false);
```

---

## 18. Math -- Vector2

```cpp
struct Vector2 {
    float x, y;

    Vector2(float x = 0, float y = 0);
    Vector2(const glm::vec2& v);
    operator glm::vec2() const;

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

*KonEngine is MIT licensed. Free to use in commercial and open source projects.*

---

## Tools

KonEngine ships with two companion tools. Each has its own documentation:

- **[KonAnimator](tools/KonAnimator/DOCS.md)** — visual animation editor, produces `.konani` files
- **[KonPaktor](tools/KonPaktor/DOCS.md)** — archive tool, packs assets into encrypted `.konpak` files

### Quick reference

| Tool | Binary | Purpose |
|---|---|---|
| KonAnimator | `build/tools/KonAnimator/KonAnimator` | Edit animations visually |
| anim_compiler | `build/anim_compiler` | Compile `.anim` to `.konani` (CLI) |
| KonPaktor | `tools/KonPaktor/build/KonPaktor` | Manage `.konpak` archives (GUI) |
| konpak | `tools/KonPaktor/build/konpak` | Manage `.konpak` archives (CLI) |

Build all tools at once:
```bash
./build-tools.sh
```
