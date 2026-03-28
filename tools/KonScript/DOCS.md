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

## 5. Functions

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

| Method | When called |
|---|---|
| `Ready()` | Once, when the node is added to the scene |
| `Update(dt: F64)` | Every frame |
| `Draw()` | Every frame after Update |
| `OnCollisionEnter(other: Collider2D)` | When a child collider first overlaps another |
| `OnCollisionExit(other: Collider2D)` | When they stop overlapping |

### Inherited Node2D fields

Inside a `node : Node2D` body these are available directly:

```ks
x, y          # position (F64)
scaleX, scaleY
rotation
originX, originY  # pivot (0.5 = center)
active        # Bool
name          # str
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

### Cast
```ks
let f: F64 = 10 as F64;
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
WindowShouldClose() -> Bool
Present()
PollEvents()
ClearBackground(r: F64, g: F64, b: F64)
SetTargetFPS(fps: I32)
GetWindowWidth() -> I32
GetWindowHeight() -> I32
```

### Time
```ks
GetDeltaTime() -> F64
GetTime()      -> F64
GetFPS()       -> I32
```

### Input
```ks
KeyDown(key: Key)     -> Bool
KeyPressed(key: Key)  -> Bool
KeyReleased(key: Key) -> Bool

MouseDown(btn: Mouse)     -> Bool
MousePressed(btn: Mouse)  -> Bool
MouseReleased(btn: Mouse) -> Bool
GetMouseX() -> F64
GetMouseY() -> F64
GetMouseDeltaX() -> F64
GetMouseDeltaY() -> F64
GetMouseScroll() -> F64
```

**Key constants:** `Key.A`–`Key.Z`, `Key.Num0`–`Key.Num9`, `Key.Space`, `Key.Enter`,
`Key.Esc`, `Key.Tab`, `Key.Backspace`, `Key.Shift`, `Key.Ctrl`, `Key.Alt`,
`Key.Up`, `Key.Down`, `Key.Left`, `Key.Right`, `Key.F1`–`Key.F12`

**Mouse constants:** `Mouse.Left`, `Mouse.Right`, `Mouse.Middle`

### Rendering
```ks
DrawRectangle(x: F64, y: F64, w: F64, h: F64, r: F64, g: F64, b: F64, a: F64)
DrawCircle(x: F64, y: F64, radius: F64, r: F64, g: F64, b: F64, a: F64)
DrawLine(x1: F64, y1: F64, x2: F64, y2: F64, r: F64, g: F64, b: F64, a: F64)
DrawText(text: str, x: F64, y: F64, size: I32, color: Color)
```

### Audio
```ks
PlaySound(path: str)
StopSound(path: str)
PlayMusic(path: str)
StopMusic()
PauseMusic()
ResumeMusic()
SetMusicVolume(volume: F64)
SetSoundVolume(volume: F64)
```

### Debug
```ks
DebugMode(enabled: Bool)
IsDebugMode() -> Bool
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
