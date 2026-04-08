# KonScript Language Reference

> Version 0.1.0 — Compiles to C++ and links against KonEngine.

KonScript is a statically typed scripting language designed for KonEngine.
It compiles to C++ via `konscript` and is fully interoperable with the engine.

---

## Table of Contents

1. [Getting Started](#1-getting-started)
2. [Types](#2-types)
3. [Variables](#3-variables)
4. [Constants](#4-constants)
5. [Functions](#5-functions)
6. [Control Flow](#6-control-flow)
7. [Loops](#7-loops)
8. [Nodes](#8-nodes)
9. [Scene](#9-scene)
10. [Operators](#10-operators)
11. [Enums](#11-enums)
12. [Structs](#12-structs)
13. [Arrays & Tuples](#13-arrays--tuples)
14. [Nullable Types](#14-nullable-types)
15. [Comments](#15-comments)
16. [Engine Functions](#16-engine-functions)
17. [CMake Integration](#17-cmake-integration)
18. [Known Limitations](#18-known-limitations)

---

## 1. Getting Started

**Install the compiler:**
```bash
cd tools/KonScript
./build.sh
./install.sh   # installs konscript and ksc to /usr/local/bin
```

**Write a game:**
```ks
#include <engine>

node Player : Node2D {
    let mut speed: F64 = 200.0;

    func Update(dt: F64) {
        if KeyDown(Key.D) { x += speed * dt; }
        if KeyDown(Key.A) { x -= speed * dt; }
    }
}

func main() {
    InitWindow(800, 600, "My Game");
    SetTargetFPS(60);

    let scene: Scene = Scene();
    let player: Player = scene.add(Player, "player");

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

**Compile and run:**
```bash
ksc main.ks            # compile and run
ksc main.ks --keep     # keep the generated .cpp
ksc --compile main.ks  # compile only, no run
ksc --check main.ks    # typecheck only
ksc --parse main.ks    # dump AST
ksc --lex main.ks      # dump tokens
```

---

## 2. Types

| KonScript | C++ | Notes |
|---|---|---|
| `I8` | `int8_t` | |
| `I16` | `int16_t` | |
| `I32` | `int32_t` | default integer |
| `I64` | `int64_t` | |
| `U8` | `uint8_t` | |
| `U16` | `uint16_t` | |
| `U32` | `uint32_t` | |
| `U64` | `uint64_t` | |
| `F32` | `float` | |
| `F64` | `double` | default float |
| `Bool` | `bool` | |
| `str` | `const char*` | string literals |
| `String` | `std::string` | |
| `Vec2` | `Vector2` | |
| `[T]` | `std::vector<T>` | dynamic array |
| `[T; N]` | `std::array<T, N>` | fixed array |
| `(T, T)` | `std::tuple<T, T>` | tuple |
| `T?` | `std::optional<T>` | nullable |
| `Node2D` | `Node2D*` | engine node types are always pointers |
| `Collider2D` | `Collider2D*` | |
| `Scene` | `Scene` | value type, not a pointer |
| `Sound` | `Sound` | value type |
| `Music` | `Music` | value type |
| `Font` | `Font` | value type |
| `Rectangle` | `Rectangle` | collision shape |
| `Circle` | `Circle` | collision shape |
| `Camera2D` | `Camera2D` | camera struct |
| `TileGrid` | `TileGrid` | tile helper |

Node types declared with `node` are also treated as pointers when used in `let` declarations.

---

## 3. Variables

```ks
let x: I32 = 10;          # immutable
let mut y: F64 = 3.14;    # mutable
```

Variables must be initialized. Immutable variables cannot be reassigned.

Node pointer types are always mutable regardless of `mut`:
```ks
let player: Player = scene.add(Player, "player");  # always mutable
```

---

## 4. Constants

```ks
const MAX_SPEED: F64 = 500.0;
const TILE_SIZE: I32 = 32;
```

Top-level constants compile to `constexpr`. Constants inside functions compile to `const`.

---

## 5. Functions & Lambdas

```ks
func Add(a: I32, b: I32) -> I32 {
    return a + b;
}

func Greet(name: str) {
    Print("Hello, %s!\n", name);
}
```

The entry point is always `func main()`:
```ks
func main() {
    InitWindow(800, 600, "My Game");
    # ...
}
```

**Public functions** (exported from a node):
```ks
node Player : Node2D {
    pub func GetHealth() -> I32 { return health; }
}
```

**Lambdas / closures:**

KonScript supports C++ style lambdas with `[]` or `[&]` captures. Both compile to `[&]` in C++ (captures are always by reference).

```ks
# C++ style — [] or [&] both work
let triple = [](x: I32) -> I32 { return x * 3; };
let quad = [&](x: I32) -> I32 { return x * 4; };

# No-parameter lambdas (common for callbacks)
UI.OnClick("btn", []() { Print("clicked!"); });

# With captures — lambdas can read and modify surrounding variables
let mut score: I32 = 0;
UI.OnClick("score", [&]() {
    score = score + 10;
    Print("Score: ", score);
});

# Timer callbacks
Timer.Create("tick", 1.0, true, [&]() {
    score = score + 1;
    Print("Tick! Score: ", score);
});
```

---

## 6. Control Flow

### if / else

```ks
if x > 0 {
    Print("positive\n");
} else if x < 0 {
    Print("negative\n");
} else {
    Print("zero\n");
}
```

### switch

```ks
switch state {
    case 0: { Print("idle\n"); }
    case 1: { Print("running\n"); }
    default: { Print("unknown\n"); }
}
```

---

## 7. Loops

### while

```ks
while !WindowShouldClose() && running {
    # game loop
}
```

### loop (infinite)

```ks
loop {
    if done { break; }
}
```

### for-in (range, exclusive)

```ks
for i: I32 in 0..10 {
    Print("%d\n", i);  # 0 through 9
}
```

### for-in (range, inclusive)

```ks
for i: I32 in 1..=3 {
    Print("%d\n", i);  # 1, 2, 3
}
```

### for-in (collection)

```ks
for item: I32 in myArray {
    Print("%d\n", item);
}
```

### for (C-style)

```ks
for i: I32 = 0; i < 10; i++ {
    Print("%d\n", i);
}
```

### break / continue

```ks
for i: I32 in 0..100 {
    if i == 50 { break; }
    if i % 2 == 0 { continue; }
    Print("%d\n", i);
}
```

---

## 8. Nodes

Nodes are the primary way to define game objects. They inherit from a KonEngine node type.

```ks
node Player : Node2D {
    # fields
    let mut health: I32    = 3;
    let mut speed:  F64    = 200.0;
    let mut grounded: Bool = false;

    # lifecycle methods
    func Ready() {
        x = 100.0;
        y = 400.0;
    }

    func Update(dt: F64) {
        if KeyDown(Key.D) { x += speed * dt; }
        if KeyDown(Key.A) { x -= speed * dt; }
    }

    func Draw() {
        DrawRectangle(x - 16.0, y - 24.0, 32.0, 48.0, 0.2, 0.5, 1.0, 1.0);
    }

    func OnCollisionEnter(other: Collider2D) {
        if other.name == "enemy" {
            health -= 1;
            Print("Hit! HP: %d\n", health);
        }
    }

    func OnCollisionExit(other: Collider2D) {
        Print("Stopped touching: %s\n", other.name);
    }

    pub func GetHealth() -> I32 { return health; }
}
```

### Lifecycle methods

| Method | When called | Use for |
|---|---|---|
| `Ready()` | Once, when added to scene | Setup, load resources |
| `Update(dt: F64)` | Every frame | Movement, input, logic |
| `Draw()` | Every frame after Update | Render sprites, shapes, text |
| `OnCollisionEnter(other: Collider2D)` | When a collider starts touching | Damage, triggers |
| `OnCollisionExit(other: Collider2D)` | When they stop touching | Reset states |
| `OnDestroy()` | When the node is removed | Unload textures, stop music |

### Custom methods

Nodes can have any number of custom methods beyond lifecycle methods:

```ks
node Enemy : Node2D {
    let mut hp: I32 = 50;

    func TakeDamage(amount: I32) {
        hp = hp - amount;
    }

    func IsAlive() -> Bool {
        return hp > 0;
    }

    func Heal(amount: I32) {
        hp = hp + amount;
        if hp > 100 { hp = 100; }
    }
}

# In main:
let enemy: Enemy = scene.add(Enemy, "goblin");
enemy.TakeDamage(20);
Print("Alive: ", enemy.IsAlive());
```

### Asset fields (Texture, Music, Sound)

Nodes can store `Texture`, `Music`, and `Sound` as fields. They load when the node is created.

```ks
node Player : Node2D {
    let mut sprite: Texture = LoadTexture("player.png");
    let mut bgm: Music = LoadMusic("theme.mp3");
    let mut jump: Sound = LoadSound("jump.wav");

    func Ready() {
        PlayMusic(bgm);
    }

    func Update(dt: F64) {
        UpdateMusic(bgm);
        if KeyPressed(Key.Space) { PlaySound(jump); }
    }

    func Draw() {
        DrawTexture(sprite, x, y, 32.0, 32.0);
    }

    func OnDestroy() {
        UnloadTexture(sprite);
        UnloadMusic(bgm);
        UnloadSound(jump);
    }
}
```

> `LoadTexture`/`LoadMusic`/`LoadSound` are global — backed by a singleton AssetManager. Any node, anywhere, can call them. No need to pass references.

### Inherited Node2D fields

Inside a `node : Node2D` body these are available directly:

```ks
x, y          # position (F64)
scaleX, scaleY
rotation
originX, originY  # pivot (0.5 = center)
active        # Bool
name          # Str
```

### Adding child nodes inside a node

Use `this.add()` to add children from within a node method:

```ks
node Player : Node2D {
    func Ready() {
        let col: Collider2D = this.add(Collider2D, "hitbox");
        col.width  = 32.0;
        col.height = 48.0;
    }
}
```

### Multi-file projects

Split nodes into separate `.ks` files and include them:

**player.ks:**
```ks
node Player : Node2D {
    let mut sprite: Texture = LoadTexture("player.png");
    let mut speed: F64 = 200.0;

    func Update(dt: F64) {
        if KeyDown(Key.D) { x = x + speed * dt; }
    }

    func Draw() {
        DrawTexture(sprite, x, y, 32.0, 32.0);
    }
}
```

**main.ks:**
```ks
#include <engine>
#include "player.ks"

func main() {
    InitWindow(800, 600, "Game");
    SetTargetFPS(60);
    let scene: Scene = Scene();
    let player: Player = scene.add(Player, "p1");
    player.x = 400.0;

    while !WindowShouldClose() {
        scene.update(GetDeltaTime());
        ClearBackground(0.1, 0.1, 0.1);
        scene.draw();
        Present();
        PollEvents();
    }
}
```

Compile from your project directory:
```bash
cd my_game
konscript main.ks
./main
```

---

## 9. Scene

```ks
let scene: Scene = Scene();
```

### Adding nodes

```ks
let player: Player    = scene.add(Player, "player");
let enemy:  Enemy     = scene.add(Enemy, "enemy");
```

`scene.add()` calls `Ready()` on the node after adding it.

### Adding colliders

Colliders are added as children of nodes:
```ks
let col: Collider2D = player.add(Collider2D, "hitbox");
col.width  = 32.0;
col.height = 48.0;
```

After adding colliders outside of `Ready()`, call `scene.scan()` to register them:
```ks
scene.scan();
```

### Scene methods

```ks
scene.update(dt);         # update all nodes + run collision
scene.draw();             # draw all nodes
scene.remove("name");     # remove a node by name
scene.get("name");        # get a node by name
scene.scan();             # register late-added colliders
```

---

## 10. Operators

### Arithmetic
```ks
x + y   x - y   x * y   x / y   x % y
x += y  x -= y  x *= y  x /= y
x++     x--
```

### Comparison
```ks
x == y   x != y
x < y    x > y
x <= y   x >= y
```

### Logic
```ks
x && y   x || y   !x
```

### Ternary
```ks
x ? y : z        # y if x is true, else z
```

### Cast
```ks
let f: F64 = 10 as F64;
```

### Ternary conditional
```ks
let val: Str = condition ? "yes" : "no";
let max: I32 = a > b ? a : b;

# Right-associative chaining
let tier: Str = score > 90 ? "A" : score > 80 ? "B" : "C";
```

### Null coalescing
```ks
let val: I32 = maybeNull ?? 0;
```

### Safe member access
```ks
let name: str = node?.name ?? "unknown";
```

### Force unwrap
```ks
let val: I32 = maybeVal!;  # panics if null
```

---

## 11. Enums

Simple enums:
```ks
enum State {
    Idle,
    Running,
    Jumping,
}

let s: State = State.Idle;
```

Enums with payloads (sum types):
```ks
enum Event {
    Damage(I32),
    Heal(I32),
    Die,
}
```

---

## 12. Structs

```ks
struct Point {
    let x: F64;
    let y: F64;
}

let p: Point = Point { x: 10.0, y: 20.0 };
```

---

## 13. Arrays & Tuples

### Dynamic array
```ks
let items: [I32] = [1, 2, 3];
```

### Fixed array
```ks
let grid: [I32; 4] = [0, 0, 0, 0];
```

### Tuple
```ks
let pos: (F64, F64) = (10.0, 20.0);
```

---

## 14. Nullable Types

```ks
let maybe: I32? = None;
maybe = 42;

if maybe != None {
    Print("has value\n");
}

let val: I32 = maybe ?? 0;   # default if null
let val2: I32 = maybe!;      # force unwrap (unsafe)
```

---

## 15. Comments

```ks
# This is a line comment
# Everything after # is ignored unless it is #include
```

---

## 16. Engine Functions

These are available when `#include <engine>` is at the top of your file.

### Window
```ks
InitWindow(width: I32, height: I32, title: str);
InitWindow(width: I32, height: I32, title: str, resizable: Bool);
WindowShouldClose() -> Bool
Present()
PollEvents()
ClearBackground(r: F64, g: F64, b: F64)
SetTargetFPS(fps: I32)
SetVsync(enabled: Bool)
GetWindowWidth() -> I32
GetWindowHeight() -> I32
GetDesignWidth() -> I32
GetDesignHeight() -> I32
GetLetterboxScale() -> F64
GetLetterboxOffsetX() -> F64
GetLetterboxOffsetY() -> F64
GetGameMouseX() -> F64
GetGameMouseY() -> F64
```

### Time
```ks
GetDeltaTime() -> F64
GetTime()      -> F64
GetFPS()       -> I32
```

### Input — Keyboard
```ks
KeyDown(key: Key)     -> Bool
KeyPressed(key: Key)  -> Bool
KeyReleased(key: Key) -> Bool
```

**Key constants:** `Key.A`–`Key.Z`, `Key.Num0`–`Key.Num9`, `Key.Space`, `Key.Enter`,
`Key.Esc`, `Key.Tab`, `Key.Backspace`, `Key.Shift`, `Key.Ctrl`, `Key.Alt`,
`Key.Up`, `Key.Down`, `Key.Left`, `Key.Right`, `Key.F1`–`Key.F12`

### Input — Mouse
```ks
MouseDown(btn: Mouse)     -> Bool
MousePressed(btn: Mouse)  -> Bool
MouseReleased(btn: Mouse) -> Bool
GetMouseX() -> F64
GetMouseY() -> F64
GetMouseDeltaX() -> F64
GetMouseDeltaY() -> F64
GetMouseScroll() -> F64
```

**Mouse constants:** `Mouse.Left`, `Mouse.Right`, `Mouse.Middle`

### Input — Gamepad
```ks
GamepadConnected(player: I32)                        -> Bool
GamepadDown(player: I32, btn: Gamepad.Button)        -> Bool
GamepadPressed(player: I32, btn: Gamepad.Button)     -> Bool
GamepadReleased(player: I32, btn: Gamepad.Button)    -> Bool
GamepadAxis(player: I32, axis: Gamepad.Axis)         -> F64
```

**Gamepad buttons:** `Gamepad.A`, `Gamepad.B`, `Gamepad.X`, `Gamepad.Y`,
`Gamepad.LeftBumper`, `Gamepad.RightBumper`, `Gamepad.Back`, `Gamepad.Start`,
`Gamepad.LeftThumb`, `Gamepad.RightThumb`,
`Gamepad.DPadUp`, `Gamepad.DPadRight`, `Gamepad.DPadDown`, `Gamepad.DPadLeft`

**Gamepad axes:** `Gamepad.LeftX`, `Gamepad.LeftY`, `Gamepad.RightX`, `Gamepad.RightY`,
`Gamepad.LeftTrigger`, `Gamepad.RightTrigger`

### Rendering
```ks
DrawRectangle(x: F64, y: F64, w: F64, h: F64, r: F64, g: F64, b: F64, a: F64)
DrawCircle(x: F64, y: F64, radius: F64, r: F64, g: F64, b: F64, a: F64)
DrawLine(x1: F64, y1: F64, x2: F64, y2: F64, r: F64, g: F64, b: F64, a: F64)
DrawText(text: str, x: F64, y: F64, size: I32, color: Color)
DrawTexture(tex: Texture, x: F64, y: F64, w: F64, h: F64)
DrawTextureRec(tex: Texture, x: F64, y: F64, w: F64, h: F64,
               srcX: F64, srcY: F64, srcW: F64, srcH: F64)
```

### Textures
```ks
LoadTexture(path: str)    -> Texture
UnloadTexture(tex: Texture)
```

### Fonts
```ks
LoadFont(path: str, size: I32) -> Font
UnloadFont(font: Font)
```

### Audio — Sounds
```ks
LoadSound(path: str)     -> Sound
UnloadSound(snd: Sound)
PlaySound(snd: Sound)
StopSound(snd: Sound)
PauseSound(snd: Sound)
ResumeSound(snd: Sound)
IsSoundPlaying(snd: Sound)  -> Bool
SetSoundVolume(snd: Sound, volume: F64)
```

### Audio — Music
```ks
LoadMusic(path: str)     -> Music
UnloadMusic(mus: Music)
PlayMusic(mus: Music)
StopMusic(mus: Music)
PauseMusic(mus: Music)
ResumeMusic(mus: Music)
UpdateMusic(mus: Music)
IsMusicPlaying(mus: Music)  -> Bool
SetMusicVolume(mus: Music, volume: F64)
SetMusicLooping(mus: Music, loop: Bool)
SetMasterVolume(volume: F64)
```

### Debug
```ks
DebugMode(enabled: Bool)
IsDebugMode() -> Bool
```

### Camera
```ks
Camera2D(x: F64, y: F64, zoom: F64, rotation: F64) -> Camera2D
BeginCamera2D(cam: Camera2D)
EndCamera2D()
Camera2DFollow(cam: Camera2D, targetX: F64, targetY: F64, speed: F64, dt: F64)
Camera2DClamp(cam: Camera2D, worldX: F64, worldY: F64, worldW: F64, worldH: F64,
              viewW: F64, viewH: F64)
Camera2DShake(cam: Camera2D, magnitude: F64)
Camera2DLerp(from: Camera2D, to: Camera2D, t: F64) -> Camera2D
```

### Collision Detection
```ks
Rectangle(x: F64, y: F64, w: F64, h: F64) -> Rectangle
Circle(x: F64, y: F64, radius: F64) -> Circle
CheckCollisionRecs(a: Rectangle, b: Rectangle) -> Bool
CheckCollisionCircles(a: Circle, b: Circle) -> Bool
CheckCollisionCircleRec(c: Circle, r: Rectangle) -> Bool
```

### Random
```ks
Random.Seed()
Random.Seed(seed: I32)
Random.Range(min: I32, max: I32) -> I32
Random.RangeF(min: F64, max: F64) -> F64
Random.Value() -> F64
Random.Bool(probability: F64) -> Bool
```

### Color
```ks
Color(r: F64, g: F64, b: F64, a: F64) -> Color
```

**Presets:** `RED`, `GREEN`, `BLUE`, `WHITE`, `BLACK`, `YELLOW`, `CYAN`, `MAGENTA`, `ORANGE`, `GRAY`, `BLANK`

### Vec2
```ks
Vec2(x: F64, y: F64) -> Vec2

# Methods (called on a Vec2 value):
v.Length()        -> F64
v.LengthSq()     -> F64
v.Normalized()    -> Vec2
v.Dot(other)      -> F64
v.Distance(other) -> F64
v.DistanceSq(other) -> F64
v.Rotated(angle)  -> Vec2
v.Reflected(normal) -> Vec2
```

### TileGrid
```ks
TileGrid(tileW: I32, tileH: I32) -> TileGrid
grid.WorldToTile(worldX: F64, worldY: F64)  -> TileCoord
grid.TileToWorld(tileX: I32, tileY: I32)    -> WorldPos
grid.Snap(worldX: F64, worldY: F64)         -> WorldPos
grid.TileCenter(tileX: I32, tileY: I32)     -> WorldPos
grid.DrawGrid(originX: F64, originY: F64, cols: I32, rows: I32)
```

### Timer

Frame-rate independent timers for gameplay events. Call `Timer.UpdateAll(dt)` every frame.

```ks
# API
Timer.Create(id: Str, duration: F64, repeating: Bool, callback: [&]())
Timer.UpdateAll(dt: F64)        # MUST call every frame
Timer.Pause(id: Str)
Timer.Resume(id: Str)
Timer.Reset(id: Str)
Timer.Remove(id: Str)
Timer.RemoveAll()
Timer.Exists(id: Str)     -> Bool
Timer.Finished(id: Str)   -> Bool
Timer.Remaining(id: Str)  -> F64
```

**Example:**
```ks
#include <engine>

func main() {
    InitWindow(400, 300, "Timer Demo", false);
    SetTargetFPS(60);

    let mut count: I32 = 0;

    # Repeating timer: increments counter every second
    Timer.Create("counter", 1.0, true, [&]() {
        count = count + 1;
        Print("Count: ", count);
    });

    # One-shot timer: prints a message after 5 seconds
    Timer.Create("alert", 5.0, false, [&]() {
        Print("5 seconds have passed!");
    });

    while !WindowShouldClose() {
        let dt: F64 = GetDeltaTime();
        Timer.UpdateAll(dt);  # tick all timers

        if KeyPressed(Key.P) {
            Timer.Pause("counter");
            Print("Paused");
        }
        if KeyPressed(Key.R) {
            Timer.Resume("counter");
            Print("Resumed");
        }

        ClearBackground(0.1, 0.1, 0.15);
        DrawText("Count: " + ToString(count), 10.0, 10.0, 24, WHITE);

        if !Timer.Finished("alert") {
            let rem: F64 = Timer.Remaining("alert");
            DrawText("Alert in: " + ToString(rem), 10.0, 50.0, 16, GRAY);
        } else {
            DrawText("Alert fired!", 10.0, 50.0, 16, YELLOW);
        }

        Present();
        PollEvents();
    }
}
```

### UI
```ks
UI.AddButton(id: Str, text: Str, x: F64, y: F64)
UI.AddLabel(id: Str, text: Str, x: F64, y: F64, fontSize: I32, color: Color)
UI.AddPanel(id: Str, x: F64, y: F64, w: F64, h: F64)
UI.AddImage(id: Str, tex: Texture, x: F64, y: F64, w: F64, h: F64)
UI.PanelAddChild(panelId: Str, childId: Str)
UI.OnClick(id: Str, callback: func())
UI.AddTextBox(id: Str, text: Str, x: F64, y: F64, w: F64, h: F64, typewriter: Bool, charsPerSec: F64)
UI.Connect(id: Str, signal: Str, callback: func())
UI.Update()
UI.Draw()
UI.WantsInput() -> Bool
UI.Remove(id: Str)
UI.Clear()
MeasureTextWidth(text: Str, fontSize: I32) -> F64
```

**Signals:** `"clicked"`, `"hovered"`, `"unhovered"`

### Signals
```ks
# Inside a node — connect to a signal
Connect("signal_name", func() {
    Print("signal fired!");
});

# Inside a node — emit a signal
Emit("player_dead");
```

Collision signals are automatic when using `Collider2D` children:
```ks
node Player : Node2D {
    func OnCollisionEnter(other: Collider2D) {
        Print("hit: ", other.name);
    }
    func OnCollisionExit(other: Collider2D) {
        Print("left: ", other.name);
    }
}
```

### Output
```ks
Print(fmt: str, ...)   # printf-style format string
ToString(val) -> str
```

---

## 17. CMake Integration

For larger projects, use `konscript_sources()` in your CMakeLists.txt:

```cmake
cmake_minimum_required(VERSION 3.16)
project(MyGame LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)

add_subdirectory(KonEngine)

add_executable(MyGame)
target_link_libraries(MyGame PRIVATE KonEngine)
konscript_sources(MyGame src/main.ks src/player.ks)
```

`konscript_sources` compiles each `.ks` file to a `.ks.cpp` and adds it to the target automatically.

---

## 18. Known Limitations

- **No pointer type annotations** — don't write `Player*` in `let` declarations. Node types are automatically pointers in generated C++.
- **`Print` uses printf format strings** — `%d` for integers, `%f` for floats, `%s` for strings. `std::string` values need `.c_str()` which isn't yet available in KonScript — use `str` literals instead.
- **`wait` and `spawn`** — compile to stub comments. Coroutine/async support requires a VM scheduler not yet implemented.
- **`switch` on enums** — enum variant qualification in case values isn't yet handled by the typechecker.
- **No modules** — all code in a project shares one namespace. Use unique names to avoid conflicts.
