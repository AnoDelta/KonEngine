# KonScript Documentation

> Version 0.1.0 — Companion language for KonEngine

KonScript is a statically-typed scripting language that compiles to C++. It is designed
to write KonEngine game logic without touching C++ directly. The compiler (`konscript`)
takes `.ks` source files, runs them through a lexer → parser → typechecker → codegen
pipeline, and outputs `.cpp` files that you compile and link against the engine.

---

## Table of Contents

1. [Getting Started](#1-getting-started)
2. [Types](#2-types)
3. [Variables](#3-variables)
4. [Functions](#4-functions)
5. [Control Flow](#5-control-flow)
6. [Nodes](#6-nodes)
7. [Structs](#7-structs)
8. [Enums](#8-enums)
9. [Arrays & Tuples](#9-arrays--tuples)
10. [Nullable Types](#10-nullable-types)
11. [Operators](#11-operators)
12. [Built-in Engine Functions](#12-built-in-engine-functions)
13. [Compiler Modes](#13-compiler-modes)
14. [Using ksc](#14-using-ksc)
15. [CMake Integration](#15-cmake-integration)

---

## 1. Getting Started

### Install

```bash
cd tools/KonScript
./build.sh
./install.sh          # installs to /usr/local/bin
```

Or with a custom prefix:
```bash
./install.sh --prefix=/opt/konscript
```

This installs two binaries:
- `konscript` — the backend compiler (lex, parse, typecheck, codegen)
- `ksc` — the frontend runner (compile + build + run in one command)

### Hello World (standalone)

```ks
func main() {
    Print("Hello, world!\n");
}
```

```bash
ksc hello.ks
```

### Hello World (engine mode)

Include `<engine>` to enable KonEngine bindings. The compiler detects this and
switches to engine target mode, which maps node lifecycle methods to C++ overrides.

```ks
#include <engine>

node Game : Node {
    func Ready() {
        InitWindow(800, 600, "Hello KonScript");
        SetTargetFPS(60);
    }

    func Update(dt: F64) {
        ClearBackground(0.1, 0.1, 0.1);
        DrawText("Hello from KonScript!", 100.0, 100.0, 24, WHITE);
    }
}
```

---

## 2. Types

### Primitive Types

| Type | Description | C++ equivalent |
|---|---|---|
| `I8` | 8-bit signed integer | `int8_t` |
| `I16` | 16-bit signed integer | `int16_t` |
| `I32` | 32-bit signed integer | `int32_t` |
| `I64` | 64-bit signed integer | `int64_t` |
| `U8` | 8-bit unsigned integer | `uint8_t` |
| `U16` | 16-bit unsigned integer | `uint16_t` |
| `U32` | 32-bit unsigned integer | `uint32_t` |
| `U64` | 64-bit unsigned integer | `uint64_t` |
| `F32` | 32-bit float | `float` |
| `F64` | 64-bit float | `double` |
| `Bool` | boolean | `bool` |
| `str` | string literal | `const char*` |
| `String` | heap string | `std::string` |
| `Vec2` | 2D vector | `Vector2` |

### Type Modifiers

```ks
I32?          // nullable I32
[I32]         // dynamic array of I32
[I32; 8]      // fixed array of I32 with 8 elements
(F64, F64)    // tuple of two F64 values
```

### Casting

```ks
let x: I32 = 10;
let y: F64 = x as F64;
```

---

## 3. Variables

Variables are declared with `let`. They are immutable by default. Add `mut` to allow
reassignment.

```ks
let speed: F64 = 200.0;        // immutable
let mut health: I32 = 100;     // mutable

health -= 10;                  // OK
// speed = 100.0;              // error: not mutable
```

### Constants

Top-level constants are declared with `const`. They must have a type annotation and
a literal initializer.

```ks
const GRAVITY: F64 = 980.0;
const MAX_ENEMIES: I32 = 32;
```

### Increment / Decrement

```ks
i++;
i--;
```

---

## 4. Functions

```ks
func Add(a: I32, b: I32) -> I32 {
    return a + b;
}
```

Functions with no return value omit the `->` annotation:

```ks
func Greet(name: str) {
    Print("Hello, %s!\n", name);
}
```

### Visibility

Mark a function `pub` to export it (makes it `public` in generated C++):

```ks
pub func GetScore() -> I32 {
    return score;
}
```

### Recursive Functions

```ks
func Factorial(n: I32) -> I32 {
    if n <= 1 { return 1; }
    return n * Factorial(n - 1);
}
```

---

## 5. Control Flow

### If / Else

```ks
if health <= 0 {
    Die();
} else if health < 20 {
    Flash();
} else {
    Recover();
}
```

### While

```ks
let mut i: I32 = 0;
while i < 10 {
    Print("%d\n", i);
    i++;
}
```

### Loop (infinite)

```ks
loop {
    if ShouldStop() { break; }
    DoWork();
}
```

### For (C-style)

```ks
for i: I32 = 0; i < 10; i++ {
    Print("%d\n", i);
}
```

### For-In (range)

```ks
for item: I32 in 0..10 {   // exclusive: 0 to 9
    Print("%d\n", item);
}

for item: I32 in 0..=10 {  // inclusive: 0 to 10
    Print("%d\n", item);
}
```

### Switch

```ks
switch state {
    case Idle:
        Print("standing\n");
        break;
    case Running:
        x += speed * dt;
        break;
    default:
        break;
}
```

### Break / Continue

```ks
for i: I32 = 0; i < 100; i++ {
    if i == 50 { break; }
    if i % 2 == 0 { continue; }
    Print("%d\n", i);
}
```

---

## 6. Nodes

Nodes are the primary way to define game objects in KonScript. A `node` declaration
compiles to a C++ class inheriting from a KonEngine node type.

```ks
#include <engine>

node Player : Node2D {
    let speed: F64 = 200.0;
    let mut health: I32 = 100;

    func Ready() {
        x = 400.0;
        y = 300.0;
    }

    func Update(dt: F64) {
        if KeyDown(Key.D) { x += speed * dt; }
        if KeyDown(Key.A) { x -= speed * dt; }
        if KeyDown(Key.W) { y -= speed * dt; }
        if KeyDown(Key.S) { y += speed * dt; }
    }

    func Draw() {
        DrawRectangle(x, y, 32.0, 32.0, RED);
    }

    func OnCollisionEnter(other: Node2D) {
        if other.name == "Enemy" {
            health -= 10;
            Emit("player_hurt", health);
        }
    }

    func OnCollisionExit(other: Node2D) {
        // called when collision ends
    }

    pub func GetHealth() -> I32 {
        return health;
    }
}
```

### Lifecycle Methods

These method names are automatically mapped to KonEngine's virtual overrides:

| KonScript name | C++ override | When it runs |
|---|---|---|
| `Ready()` | `Ready()` | Once, when added to the scene |
| `Update(dt: F64)` | `Update(float dt)` | Every frame before draw |
| `Draw()` | `Draw()` | Every frame during render |
| `OnCollisionEnter(other: Node2D)` | `OnCollisionEnter(Collider2D*)` | When collision starts |
| `OnCollisionExit(other: Node2D)` | `OnCollisionExit(Collider2D*)` | When collision ends |

### Built-in Node2D Fields

These fields are available inside any node that inherits from `Node2D`:

| Field | Type | Description |
|---|---|---|
| `x` | `F64` | World X position |
| `y` | `F64` | World Y position |
| `scaleX` | `F64` | Horizontal scale |
| `scaleY` | `F64` | Vertical scale |
| `rotation` | `F64` | Rotation in degrees |
| `originX` | `F64` | Pivot X (0.0–1.0) |
| `originY` | `F64` | Pivot Y (0.0–1.0) |
| `alpha` | `F64` | Opacity (0.0–1.0) |
| `active` | `Bool` | Whether the node updates and draws |
| `name` | `str` | The node's name in the scene tree |

### Base Types

The `node` declaration's base type controls what C++ class it inherits from:

```ks
node MyNode : Node    { ... }    // base Node (no 2D transform)
node MyNode : Node2D  { ... }    // 2D transform, part of scene
node MyNode : Sprite2D{ ... }    // Sprite2D with texture support
```

### Signals

Use `Emit` inside a node to fire a signal to listeners:

```ks
Emit("player_hurt", health);
Emit("game_over");
```

---

## 7. Structs

Structs are value types — simple data containers with no methods.

```ks
struct Bullet {
    x: F64,
    y: F64,
    speed: F64,
    active: Bool,
}
```

### Struct Initialization

```ks
let b: Bullet = Bullet { x: 100.0, y: 200.0, speed: 400.0, active: true };
```

### Field Access

```ks
b.x += b.speed * dt;
```

---

## 8. Enums

```ks
enum State {
    Idle,
    Running,
    Jumping,
    Dead,
}
```

### Enums with Payloads

Variants can carry a value:

```ks
enum Pickup {
    Coin(I32),      // carries a point value
    Health(I32),    // carries heal amount
    Key,            // no payload
}
```

### Matching Enum Values

Use `switch` to branch on enum variants:

```ks
switch pickup {
    case Coin:
        score += 10;
        break;
    case Health:
        health += 25;
        break;
    case Key:
        hasKey = true;
        break;
    default:
        break;
}
```

---

## 9. Arrays & Tuples

### Dynamic Arrays

```ks
let items: [I32] = [1, 2, 3, 4, 5];
let first: I32 = items[0];
```

### Fixed Arrays

```ks
let grid: [I32; 16] = [0, 0, 0, 0,
                       0, 0, 0, 0,
                       0, 0, 0, 0,
                       0, 0, 0, 0];
```

### Tuples

```ks
func GetPosition() -> (F64, F64) {
    return (x, y);
}

let pos: (F64, F64) = GetPosition();
```

---

## 10. Nullable Types

Append `?` to any type to make it nullable. The value can be `null` or `None`.

```ks
let enemy: Node2D? = FindEnemy();
```

### Safe Access

Use `?.` to access a member only if the value is non-null:

```ks
let name: str? = enemy?.name;
```

### Null Coalescing

Use `??` to provide a fallback if the value is null:

```ks
let hp: I32 = enemy?.health ?? 0;
```

### Force Unwrap

Use `!` (postfix) to assert the value is non-null. Crashes if it is null.

```ks
let e: Node2D = enemy!;
```

---

## 11. Operators

### Arithmetic

| Operator | Meaning |
|---|---|
| `+` `-` `*` `/` `%` | standard math |
| `+=` `-=` `*=` `/=` | compound assignment |
| `++` `--` | increment/decrement (postfix) |
| `-x` | unary negation |

### Comparison

`==` `!=` `<` `>` `<=` `>=`

### Logic

`&&` (and), `||` (or), `!` (not)

### Ranges

```ks
0..10    // exclusive range: 0 to 9
0..=10   // inclusive range: 0 to 10
```

### Cast

```ks
x as F64
```

---

## 12. Built-in Engine Functions

These are available when `#include <engine>` is at the top of the file.

### Window

```ks
InitWindow(width: I32, height: I32, title: str)
WindowShouldClose() -> Bool
Present()
PollEvents()
ClearBackground(r: F64, g: F64, b: F64)
SetTargetFPS(fps: I32)
GetDeltaTime() -> F64
GetFPS() -> I32
GetTime() -> F64
SetTimeScale(scale: F64)
DebugMode(enabled: Bool)
IsDebugMode() -> Bool
GetWindowWidth() -> I32
GetWindowHeight() -> I32
SetVsync(enabled: Bool)
```

### Drawing

```ks
DrawRectangle(x: F64, y: F64, w: F64, h: F64, r: F64, g: F64, b: F64, a: F64)
DrawCircle(x: F64, y: F64, radius: F64, r: F64, g: F64, b: F64, a: F64)
DrawLine(x1: F64, y1: F64, x2: F64, y2: F64, r: F64, g: F64, b: F64, a: F64)
DrawText(text: str, x: F64, y: F64, size: F64, color)
```

### Input

```ks
// Keyboard
KeyDown(key: I32) -> Bool        // also accepts Key.X notation
KeyPressed(key: I32) -> Bool
KeyReleased(key: I32) -> Bool

// Mouse
MouseDown(button: I32) -> Bool
MousePressed(button: I32) -> Bool
MouseReleased(button: I32) -> Bool
GetMouseX() -> F64
GetMouseY() -> F64
GetMouseDeltaX() -> F64
GetMouseDeltaY() -> F64
GetMouseScroll() -> F64

// Gamepad
IsGamepadConnected(player: I32) -> Bool
GamepadButtonDown(player: I32, button: I32) -> Bool
GetGamepadAxis(player: I32, axis: I32) -> F64
```

#### Key Codes

Access via the `Key` namespace:

```ks
if KeyDown(Key.D) { ... }
if KeyPressed(Key.Space) { ... }
```

Available keys: `A`–`Z`, `Num0`–`Num9`, `Up`, `Down`, `Left`, `Right`,
`Space`, `Enter`, `Escape`, `Tab`, `Backspace`, `Shift`, `Ctrl`, `Alt`, `F1`–`F12`

#### Mouse Buttons

```ks
if MouseDown(Mouse.Left) { ... }
```

Available: `Mouse.Left`, `Mouse.Right`, `Mouse.Middle`

### Audio

```ks
PlaySound(path: str)
StopSound(path: str)
PlayMusic(path: str, loop: Bool)
StopMusic()
PauseMusic()
ResumeMusic()
SetMusicVolume(volume: F64)
SetSoundVolume(volume: F64)
```

### Output

```ks
Print(fmt: str, ...)    // printf-style
```

---

## 13. Compiler Modes

The `konscript` backend supports several debug/inspection modes:

```bash
konscript --lex   file.ks     # dump all tokens and exit
konscript --parse file.ks     # dump the AST and exit
konscript --check file.ks     # typecheck only, no codegen
konscript file.ks -o out.cpp  # compile to C++
```

This is useful for debugging the language or inspecting what the compiler sees.

---

## 14. Using ksc

`ksc` is the high-level frontend that wraps compile → build → run into one step.

```bash
ksc hello.ks              # compile, build, run, then clean up
ksc hello.ks --keep       # same but keep the .cpp and binary
ksc hello.ks --clean      # remove generated .cpp and binary
ksc --compile hello.ks    # compile to .cpp only, don't run
ksc --compile hello.ks -o out.cpp
ksc --lex   hello.ks      # pass-through to konscript --lex
ksc --parse hello.ks      # pass-through to konscript --parse
ksc --check hello.ks      # pass-through to konscript --check
```

### Environment Variables

| Variable | Default | Description |
|---|---|---|
| `CXX` | `g++` | C++ compiler to use |
| `CXXFLAGS` | `-std=c++17` | Compiler flags |

---

## 15. CMake Integration

For engine-mode `.ks` files (those that `#include <engine>`), use the
`konscript_sources()` helper in your game's `CMakeLists.txt`. It compiles each
`.ks` file to `.cpp` at build time and adds it to the target automatically.

```cmake
cmake_minimum_required(VERSION 3.16)
project(MyGame)

find_package(KonEngine REQUIRED)

add_executable(MyGame src/main.cpp)

# Compile KonScript files and add to target
konscript_sources(MyGame
    src/Player.ks
    src/Enemy.ks
    src/GameManager.ks
)

target_link_libraries(MyGame PRIVATE KonEngine)
```

The `.ks` → `.cpp` step runs automatically during `cmake --build`. No manual
`ksc` invocations needed.

---

## Full Example

```ks
#include <engine>

const GRAVITY: F64 = 980.0;
const JUMP_FORCE: F64 = 450.0;

node Player : Node2D {
    let speed: F64 = 220.0;
    let mut vy: F64 = 0.0;
    let mut grounded: Bool = false;
    let mut health: I32 = 3;

    func Ready() {
        x = 100.0;
        y = 400.0;
    }

    func Update(dt: F64) {
        // Horizontal movement
        if KeyDown(Key.D) { x += speed * dt; }
        if KeyDown(Key.A) { x -= speed * dt; }

        // Jump
        if KeyPressed(Key.Space) && grounded {
            vy = -JUMP_FORCE;
            grounded = false;
        }

        // Gravity
        vy += GRAVITY * dt;
        y  += vy * dt;

        // Floor clamp
        if y >= 400.0 {
            y       = 400.0;
            vy      = 0.0;
            grounded = true;
        }
    }

    func Draw() {
        DrawRectangle(x, y, 32.0, 48.0, 0.2, 0.6, 1.0, 1.0);
    }

    func OnCollisionEnter(other: Node2D) {
        if other.name == "Enemy" {
            health -= 1;
            Emit("player_hurt", health);
            if health <= 0 {
                Emit("game_over");
            }
        }
    }

    pub func GetHealth() -> I32 {
        return health;
    }
}
```

---

*KonScript is part of KonEngine and is MIT licensed.*
