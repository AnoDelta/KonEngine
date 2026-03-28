# KonEngine Documentation

> Version 0.8.0 — API is stable but subject to change before v1.0.

---

## Table of Contents

1. [Window & Application](#1-window--application)
2. [Rendering](#2-rendering)
3. [Text & Fonts](#3-text--fonts)
4. [Input](#4-input)
5. [Camera](#5-camera)
6. [Textures](#6-textures)
7. [Color](#7-color)
8. [Time](#8-time)
9. [Audio](#9-audio)
10. [Node System](#10-node-system)
11. [Scene](#11-scene)
12. [Node2D](#12-node2d)
13. [Sprite2D](#13-sprite2d)
14. [Collider2D & CollisionWorld](#14-collider2d--collisionworld)
15. [Animation](#15-animation)
16. [AnimationPlayer](#16-animationplayer)
17. [Easing Curves](#17-easing-curves)
18. [Debug Mode](#18-debug-mode)
19. [Math — Vector2](#19-math--vector2)
20. [Tools](#20-tools)

---

## 1. Window & Application

```cpp
#include "KonEngine.hpp"
```

All engine functions are available through this single header.

```cpp
void InitWindow(int width, int height, const std::string& title, bool canResize = false);
```
Creates the window and initializes OpenGL. Must be called before anything else.

```cpp
bool WindowShouldClose();
void Present();       // swap back buffer to screen — call once per frame
void PollEvents();    // process input and advance frame timer — call after Present()
void ClearBackground(float r, float g, float b);  // RGB, values 0.0–1.0

int  GetWindowWidth();
int  GetWindowHeight();
void SetVsync(bool enabled);
```

### Typical game loop

```cpp
InitWindow(800, 600, "My Game");
SetTargetFPS(60);

while (!WindowShouldClose()) {
    ClearBackground(0.1f, 0.1f, 0.1f);

    // update logic
    // draw calls

    Present();
    PollEvents();
}
```

---

## 2. Rendering

All draw calls use a **top-left origin** coordinate system. X increases right, Y increases down.

### Rectangles

```cpp
void DrawRectangle(float x, float y, float w, float h, float r, float g, float b, float a = 1.0f);
void DrawRectangle(float x, float y, float w, float h, Color color);
```
`x, y` is the top-left corner. Draws a filled rectangle.

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

`DrawTextureRec` draws a sub-region of a texture — useful for sprite sheets.
`srcX`, `srcY`, `srcWidth`, `srcHeight` are UV coordinates in the range 0.0–1.0.

---

## 3. Text & Fonts

KonEngine uses TrueType fonts baked into a GPU atlas via `stb_truetype`. A built-in
Inconsolata font is always available without loading anything.

```cpp
Font LoadFont(const char* path, int fontSize);
Font LoadDefaultFont(int fontSize);
Font& GetDefaultFont();
void UnloadFont(Font& font);
```

`LoadFont` loads a `.ttf` file from disk. If the file can't be opened, it falls back
to the default font silently. `GetDefaultFont()` returns the built-in Inconsolata at
size 20, loading it on first call.

```cpp
// With an explicit font
void DrawText(Font& font, const char* text, float x, float y, Color color);
void DrawText(Font& font, const char* text, float x, float y, int fontSize, Color color);

// Using the default font
void DrawText(const char* text, float x, float y, Color color);
void DrawText(const char* text, float x, float y, int fontSize, Color color);
```

```cpp
// Quick usage — default font, no setup needed
DrawText("Score: 100", 10.0f, 10.0f, WHITE);

// Custom font
Font myFont = LoadFont("assets/fonts/pixel.ttf", 16);
DrawText(myFont, "Hello!", 100.0f, 100.0f, YELLOW);
UnloadFont(myFont);
```

**Note:** The `fontSize` parameter in the `DrawText` overloads is currently unused —
size is fixed at load time. This will be improved in a future version.

---

## 4. Input

### Keyboard

```cpp
bool IsKeyDown(Key::Code key);      // held this frame
bool IsKeyPressed(Key::Code key);   // became pressed this frame
bool IsKeyReleased(Key::Code key);  // became released this frame
```

**Key codes** (use the `Key::` namespace):

| Group | Codes |
|---|---|
| Letters | `A`–`Z` |
| Numbers | `Num0`–`Num9` |
| Arrows | `Up`, `Down`, `Left`, `Right` |
| Special | `Space`, `Enter`, `Escape`, `Tab`, `Backspace` |
| Modifiers | `Shift`, `Ctrl`, `Alt` |
| Function | `F1`–`F12` |

```cpp
if (IsKeyDown(Key::D))       x += speed * dt;
if (IsKeyPressed(Key::Space)) Jump();
```

### Mouse

```cpp
bool  IsMouseButtonDown(Mouse::Button button);
bool  IsMouseButtonPressed(Mouse::Button button);
bool  IsMouseButtonReleased(Mouse::Button button);

float GetMouseX();
float GetMouseY();
float GetMouseDeltaX();   // movement since last frame
float GetMouseDeltaY();
float GetMouseScroll();   // scroll wheel delta
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

**Buttons:** `A`, `B`, `X`, `Y`, `LeftBumper`, `RightBumper`, `Back`, `Start`,
`LeftThumb`, `RightThumb`, `DPadUp`, `DPadRight`, `DPadDown`, `DPadLeft`

**Axes:** `LeftX`, `LeftY`, `RightX`, `RightY`, `LeftTrigger`, `RightTrigger`

```cpp
float moveX = GetGamepadAxis(0, Gamepad::LeftX);
x += moveX * speed * dt;
```

---

## 5. Camera

```cpp
Camera2D cam(float targetX, float targetY, float zoom, float rotation);
```

Creates a 2D camera centered on `(targetX, targetY)`. `zoom` defaults to `1.0f`,
`rotation` is in degrees.

```cpp
void BeginCamera2D(const Camera2D& cam);
void EndCamera2D();
```

Everything drawn between these two calls is transformed by the camera. Anything
drawn outside is in screen space and ignores the camera.

```cpp
Camera2D cam(playerX, playerY, 2.0f, 0.0f);

BeginCamera2D(cam);
    scene.Draw();        // world space — affected by camera
EndCamera2D();

DrawText("HUD", 10, 10, WHITE);  // screen space — not affected
```

The camera centers on `(targetX, targetY)` in world space. Moving the camera's
`x`/`y` pans the view; `zoom > 1` zooms in.

```cpp
// Smooth camera follow
cam.x += (player->x - cam.x) * 5.0f * dt;
cam.y += (player->y - cam.y) * 5.0f * dt;
```

---

## 6. Textures

```cpp
Texture LoadTexture(const char* path);
void    UnloadTexture(Texture& texture);
```

Supported formats: PNG, JPG, BMP, TGA. Always call `UnloadTexture` when done.

```cpp
struct Texture {
    unsigned int id;     // OpenGL texture ID
    int          width;
    int          height;
};
```

```cpp
Texture sheet = LoadTexture("sprites/player.png");
DrawTexture(sheet, 100.0f, 200.0f, 64.0f, 64.0f);
UnloadTexture(sheet);
```

---

## 7. Color

```cpp
struct Color { float r, g, b, a; };
```

All components are in the range 0.0–1.0.

**Presets:**

| Name | | Name | |
|---|---|---|---|
| `RED` | | `CYAN` | |
| `GREEN` | | `MAGENTA` | |
| `BLUE` | | `ORANGE` | |
| `WHITE` | | `PURPLE` | |
| `BLACK` | | `GRAY` | |
| `YELLOW` | | `DARKGRAY` | |
| `TRANSPARENT` | (a=0) | | |

```cpp
DrawRectangle(x, y, w, h, RED);
DrawCircle(cx, cy, r, Color{0.5f, 0.2f, 0.8f, 1.0f});
```

---

## 8. Time

```cpp
void  SetTargetFPS(int fps);
float GetDeltaTime();    // seconds since last frame
int   GetFPS();
float GetTime();         // total elapsed time in seconds
void  SetTimeScale(float scale);  // 1.0 = normal, 0.5 = half speed
```

Always multiply movement by `GetDeltaTime()` to keep things frame-rate independent:

```cpp
x += speed * GetDeltaTime();
```

---

## 9. Audio

```cpp
void PlaySound(const std::string& path);
void StopSound(const std::string& path);

void PlayMusic(const std::string& path, bool loop = true);
void StopMusic();
void PauseMusic();
void ResumeMusic();
void SetMusicVolume(float volume);   // 0.0–1.0
void SetSoundVolume(float volume);   // 0.0–1.0
```

Supported formats: `.wav`, `.ogg`, `.mp3`

Sound effects are loaded and played immediately. Music streams from disk.

```cpp
PlaySound("sfx/jump.wav");
PlayMusic("music/level1.ogg", true);
SetMusicVolume(0.6f);
```

---

## 10. Node System

The node system is the foundation of KonEngine's scene architecture. Every object
in a scene is a `Node` or a subclass of it.

### Node (base class)

```cpp
class Node {
public:
    std::string name;
    bool        active = true;
    Node*       parent = nullptr;

    template<typename T, typename... Args>
    T* AddChild(const std::string& name, Args&&... args);

    Node* GetNode(const std::string& name);            // search full subtree
    void  ForEachDescendant(std::function<void(Node*)> fn);
    void  RemoveChild(const std::string& name);

    virtual void Update(float dt) {}
    virtual void Draw() {}
    virtual void Ready() {}
};
```

`Ready()` is called once when the node is first added to the scene. `Update()` and
`Draw()` are called every frame by the scene.

### Signals

Nodes support a lightweight signal/slot system:

```cpp
// Connect a listener (lambda, function pointer, or std::function)
node->Connect("signal_name", [&]() {
    std::cout << "fired!\n";
});

// Emit with no arguments
node->Emit("signal_name");
```

`Collider2D` signals pass the other collider as an argument:

```cpp
collider->Connect("on_collision_enter", [](Collider2D* other) {
    std::cout << "Hit: " << other->name << "\n";
});
```

`AnimationPlayer` signals:

```cpp
anim->Connect("animation_finished", [&]() {
    anim->Play("idle");
});
```

### Custom node subclasses (C++)

```cpp
class Player : public Node2D {
public:
    float speed = 200.0f;
    int   health = 3;

    void Ready() override {
        x = 100.0f; y = 300.0f;
    }

    void Update(float dt) override {
        if (IsKeyDown(Key::D)) x += speed * dt;
        if (IsKeyDown(Key::A)) x -= speed * dt;
    }

    void Draw() override {
        DrawRectangle(x - 16, y - 24, 32.0f, 48.0f, BLUE);
    }
};
```

---

## 11. Scene

```cpp
class Scene {
public:
    CollisionWorld collisionWorld;

    template<typename T, typename... Args>
    T* Add(const std::string& name, Args&&... args);

    void   Remove(const std::string& name);
    Node*  GetNode(const std::string& name);

    void Update(float dt);
    void Draw();
};
```

`Scene::Add` registers `Collider2D` nodes with the `CollisionWorld` automatically.
`Scene::Update` runs collision detection before calling `Update` on all nodes.
`Scene::Draw` calls `Draw` on all active nodes, and draws collider outlines
when `DebugMode(true)` is active.

```cpp
Scene scene;

auto* player = scene.Add<Player>("player");
auto* enemy  = scene.Add<Enemy>("enemy");

while (!WindowShouldClose()) {
    ClearBackground(0.1f, 0.1f, 0.1f);
    scene.Update(GetDeltaTime());
    scene.Draw();
    Present();
    PollEvents();
}
```

---

## 12. Node2D

Extends `Node` with a 2D transform.

```cpp
class Node2D : public Node {
public:
    float x = 0, y = 0;
    float scaleX = 1.0f, scaleY = 1.0f;
    float rotation = 0.0f;          // degrees
    float originX  = 0.5f;          // pivot: 0 = left, 0.5 = center, 1 = right
    float originY  = 0.5f;
    float alpha    = 1.0f;          // opacity, 0.0–1.0

    void  Move(float dx, float dy);

    float DrawX(float width)  const;  // top-left X after applying origin
    float DrawY(float height) const;  // top-left Y after applying origin
};
```

Children inherit their parent's transform — a child at `(0, 0)` will appear at
the parent's position.

---

## 13. Sprite2D

```cpp
class Sprite2D : public Node2D {
public:
    Texture texture  = {0, 0, 0};
    float   width    = 64.0f;
    float   height   = 64.0f;
    Color   tint     = WHITE;

    bool  useSourceRect = false;
    float srcX = 0, srcY = 0, srcWidth = 64, srcHeight = 64;

    void SetTexture(Texture& tex);
};
```

`SetTexture` sets the texture and updates `width`/`height` to match the texture's
pixel size, unless `useSourceRect` is already enabled.

`useSourceRect`, `srcX/srcY/srcWidth/srcHeight` are managed automatically by
`AnimationPlayer` — you don't need to set them manually when using animation.

```cpp
auto* sprite = scene.Add<Sprite2D>("player");
sprite->x = 400; sprite->y = 300;

Texture sheet = LoadTexture("sprites/player.png");
sprite->SetTexture(sheet);
```

---

## 14. Collider2D & CollisionWorld

### Collider2D

```cpp
class Collider2D : public Node2D {
public:
    ColliderShape shape  = ColliderShape::Rectangle;
    float width   = 32.0f;
    float height  = 32.0f;
    float radius  = 16.0f;                   // used when shape == Circle
    std::vector<glm::vec2> points;           // used when shape == Custom (SAT)

    uint32_t layer = 1;
    uint32_t mask  = 1;

    bool  debugDraw  = false;
    Color debugColor = {0.0f, 1.0f, 0.0f, 0.8f};
};
```

**Shapes:** `ColliderShape::Rectangle`, `ColliderShape::Circle`, `ColliderShape::Custom`

**Layer / mask filtering** (Godot-style): a collision only fires if
`(a.layer & b.mask) || (b.layer & a.mask)`.

**Signals:**

| Signal | Argument | When |
|---|---|---|
| `on_collision_enter` | `Collider2D* other` | First frame of overlap |
| `on_collision_exit` | `Collider2D* other` | First frame after overlap ends |

```cpp
auto* col = scene.Add<Collider2D>("playerCol");
col->x = 400; col->y = 300;
col->width = 32; col->height = 48;

col->Connect("on_collision_enter", [](Collider2D* other) {
    if (other->name == "enemy") { /* take damage */ }
});
```

### CollisionWorld

`Scene` manages a `CollisionWorld` automatically. You can also use one manually:

```cpp
CollisionWorld world;
world.Add(&colA);
world.Add(&colB);
world.Update();   // detects overlaps and fires signals

// One-shot overlap test with no signals
bool hit = CollisionWorld::Overlaps(&colA, &colB);
```

---

## 15. Animation

### The .anim format

`.anim` is a human-readable text format compiled to `.konani` binary at build time.
Lines starting with `#` are comments.

```
# spritesheet path — loaded automatically in KonAnimator
spritesheet sprites/player.png

anim idle loop
    display 32 32 1.0
    frame 0 0 32 32 0.12
    frame 32 0 32 32 0.12
    frame 64 0 32 32 0.12
end

anim jump
    display 32 32 1.0
    frame 96 0 32 32 0.08
    frame 128 0 32 32 0.08
    track scaleY 0.0  1.0 easeout
    track scaleY 0.15 1.4 easeout
    track scaleY 0.35 1.0 easein
end
```

**Syntax:**

| Token | Format | Description |
|---|---|---|
| `spritesheet` | `spritesheet <path>` | Path to the image (KonAnimator only) |
| `anim` | `anim <name> [loop]` | Start a clip; add `loop` to repeat |
| `display` | `display <w> <h> <scale>` | Display size in pixels and scale |
| `frame` | `frame <srcX> <srcY> <srcW> <srcH> <duration>` | One sprite sheet frame |
| `track` | `track <prop> <time> <value> [curve]` | Keyframe on a property |
| `end` | `end` | Close the current clip |

**Animatable properties:** `x`, `y`, `scaleX`, `scaleY`, `rotation`, `alpha`

### Compiling

```bash
# CLI
./anim_compiler player.anim

# GUI (Qt)
./anim_compiler

# Or use KonAnimator for the full visual editor
```

---

## 16. AnimationPlayer

```cpp
class AnimationPlayer : public Node {
public:
    Sprite2D* target = nullptr;  // set automatically from parent Sprite2D
    Node2D*   node   = nullptr;  // set automatically from parent Node2D
    float     speed  = 1.0f;

    AnimationPlayer& Add(const Animation& anim);
    bool LoadFromFile(const std::string& path);  // loads .konani binary

    void Play(const std::string& name);
    void Pause();
    void Resume();
    void Stop();

    bool               IsPlaying()  const;
    bool               IsFinished() const;
    const std::string& GetCurrent() const;
    int                GetCurrentFrame() const;
    float              GetElapsed() const;
};
```

When `Play()` is called, `AnimationPlayer` automatically:
- Finds the parent `Sprite2D` or `Node2D` if `target`/`node` are not set
- Enables `useSourceRect` on the target sprite
- Sets `width`/`height` from the clip's `display` values

**Signal:** `animation_finished` — emitted when a non-looping animation completes.

### Typical setup

```cpp
auto* sprite = scene.Add<Sprite2D>("player");
sprite->SetTexture(LoadTexture("sprites/player.png"));

auto* anim = sprite->AddChild<AnimationPlayer>("anim");
anim->LoadFromFile("sprites/player.konani");
anim->Play("idle");

// Transition on landing
anim->Connect("animation_finished", [&]() {
    anim->Play("idle");
});
```

### Switching animations

```cpp
if (IsKeyPressed(Key::Space) && grounded) {
    anim->Play("jump");
    grounded = false;
}

if (grounded && !anim->IsPlaying()) {
    anim->Play("idle");
}
```

---

## 17. Easing Curves

Used in `.anim` keyframe tracks. All curves take `t ∈ [0, 1]`.

| Name | Feel |
|---|---|
| `linear` | Constant speed |
| `easein` | Starts slow, ends fast |
| `easeout` | Starts fast, ends slow |
| `easeinout` | Slow → fast → slow |
| `easeincubic` / `easeoutcubic` / `easeinoutcubic` | Stronger cubic versions |
| `easeinelastic` / `easeoutelastic` / `easeinoutelastic` | Springy overshoot |
| `easeinbounce` / `easeoutbounce` / `easeinoutbounce` | Bouncing |
| `easeinback` / `easeoutback` / `easeinoutback` | Slight pull-back then settle |

**Quick guide:** use `easeout` for things sliding into place, `easeinoutback` for UI
popups, `linear` for continuous loops like a spinning object.

You can also apply curves directly in C++:

```cpp
float t = Curves::Apply(Ease::EaseOutBack, rawT);
```

---

## 18. Debug Mode

```cpp
void DebugMode(bool enabled);
bool IsDebugMode();
```

Call before or after `InitWindow`. When enabled:

- Red border drawn around the window edge
- Red crosshair follows the mouse cursor
- FPS, mouse position, and delta time printed to stdout every second
- All `Collider2D` nodes in the active scene have their outlines drawn
  automatically, regardless of their individual `debugDraw` flag

```cpp
int main() {
    DebugMode(true);
    InitWindow(800, 600, "My Game");
    // ...
}
```

Disable before shipping:
```cpp
DebugMode(false);
```

---

## 19. Math — Vector2

```cpp
struct Vector2 {
    float x, y;

    Vector2(float x = 0, float y = 0);
    Vector2(const glm::vec2& v);
    operator glm::vec2() const;

    // Arithmetic: +  -  *  /  +=  -=  *=  /=  unary -
    // Comparison:  ==  !=

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
Vector2 vel = Vector2::Right() * 300.0f;
vel = vel.Rotated(angle);
pos += vel * GetDeltaTime();
```

---

## 20. Tools

KonEngine ships with companion tools for animation and asset management.
Each has its own documentation.

| Tool | Binary | Purpose | Docs |
|---|---|---|---|
| KonAnimator | `build/tools/KonAnimator/KonAnimator` | Visual animation editor | [tools/KonAnimator/DOCS.md](tools/KonAnimator/DOCS.md) |
| anim_compiler | `build/anim_compiler` | Compile `.anim` → `.konani` (CLI + GUI) | — |
| KonPaktor | `tools/KonPaktor/build/KonPaktor` | Manage `.konpak` archives (GUI) | [tools/KonPaktor/DOCS.md](tools/KonPaktor/DOCS.md) |
| konpak | `tools/KonPaktor/build/konpak` | Manage `.konpak` archives (CLI) | [tools/KonPaktor/DOCS.md](tools/KonPaktor/DOCS.md) |
| KonScript | `build/konscript` / `ksc` | Scripting language compiler | [tools/KonScript/DOCS.md](tools/KonScript/DOCS.md) |

Build all tools at once:

```bash
./build-tools.sh
```

---

*KonEngine is MIT licensed. Free to use in personal, commercial, and open source projects.*
