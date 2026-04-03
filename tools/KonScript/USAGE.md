# KonScript Usage Guide

> A complete guide to the KonScript programming language — features, use cases, and benefits.

KonScript is a statically typed language designed for game development with KonEngine.
It compiles to native binaries via LLVM or transpiles to C++, offering the performance
of C++ with a clean, modern syntax inspired by Rust and Swift.

---

## Table of Contents

1. [Why KonScript](#why-konscript)
2. [Compiler Usage](#compiler-usage)
3. [Language Basics](#language-basics)
4. [Types](#types)
5. [Variables and Constants](#variables-and-constants)
6. [Functions](#functions)
7. [Control Flow](#control-flow)
8. [Loops](#loops)
9. [Strings](#strings)
10. [Arrays](#arrays)
11. [Hash Maps](#hash-maps)
12. [Structs](#structs)
13. [Enums](#enums)
14. [Classes](#classes)
15. [Nullable Types](#nullable-types)
16. [Result Types and Error Handling](#result-types-and-error-handling)
17. [Closures and Function Types](#closures-and-function-types)
18. [Generics](#generics)
19. [Interfaces](#interfaces)
20. [Coroutines](#coroutines)
21. [Ownership and References](#ownership-and-references)
22. [Extern and FFI](#extern-and-ffi)
23. [Inline Assembly](#inline-assembly)
24. [Bitwise Operations](#bitwise-operations)
25. [Game Development with KonEngine](#game-development-with-konengine)
26. [Engine API Reference](#engine-api-reference)
27. [Multi-File Projects](#multi-file-projects)
28. [Use Cases](#use-cases)
29. [Comparison with Other Languages](#comparison-with-other-languages)

---

## Why KonScript

**Performance.** Compiles to native code via LLVM. No garbage collector, no VM overhead.
Standalone programs are fully static — zero runtime dependencies.

**Safety.** Nullable types prevent null pointer crashes. Ownership tracking catches
use-after-move at compile time. The borrow checker prevents aliased mutation.
Strict mode rejects unused variables, unreachable code, and missing return paths.

**Simplicity.** Clean syntax with no header files, no forward declarations, and no
semicolons required. Type inference reduces boilerplate. The compiler is a single binary
with the toolchain bundled — nothing to install.

**Game-ready.** First-class integration with KonEngine: nodes, scenes, sprites,
collisions, audio, input — all accessible with zero boilerplate.

**Cross-platform.** Compile for Linux, Windows, and WebAssembly from a single machine.
No platform-specific code needed.

---

## Compiler Usage

### Basic Compilation

```bash
# Compile to native binary (default)
konscript game.ks              # → ./game

# Compile with a specific output name
konscript game.ks -o mygame    # → ./mygame

# Transpile to C++ (no LLVM needed)
konscript --cpp game.ks -o game.cpp

# Emit LLVM IR (for debugging)
konscript --ir game.ks

# Cross-compile for Windows
konscript --target windows64 game.ks   # → ./game.exe

# Cross-compile for WebAssembly
konscript --target wasm32 game.ks      # → ./game.wasm
```

### Compilation Modes

| Flag | Output | Use Case |
|------|--------|----------|
| *(none)* | Native binary | Production builds |
| `--cpp` | C++ source file | Integration with existing C++ projects |
| `--ir` | LLVM IR to stdout | Debugging codegen |
| `--llvm` | LLVM IR to file | Manual optimization |
| `--target linux64` | Linux ELF binary | Default |
| `--target windows64` | Windows PE .exe | Cross-compile from Linux |
| `--target wasm32` | WebAssembly .wasm | Web deployment |

---

## Language Basics

### Hello World

```
func main() -> I32 {
    Print("Hello, world!")
    return 0
}
```

Every program needs a `main` function. Semicolons are optional. `Print` outputs to stdout.

### Comments

```
// Single-line comment

/* 
   Multi-line
   comment 
*/
```

---

## Types

### Primitive Types

| KonScript | Size | Description |
|-----------|------|-------------|
| `I8` | 1 byte | Signed 8-bit integer |
| `I16` | 2 bytes | Signed 16-bit integer |
| `I32` | 4 bytes | Signed 32-bit integer (default) |
| `I64` | 8 bytes | Signed 64-bit integer |
| `U8` | 1 byte | Unsigned 8-bit integer |
| `U16` | 2 bytes | Unsigned 16-bit integer |
| `U32` | 4 bytes | Unsigned 32-bit integer |
| `U64` | 8 bytes | Unsigned 64-bit integer |
| `F32` | 4 bytes | 32-bit float |
| `F64` | 8 bytes | 64-bit float (double) |
| `Bool` | 1 byte | `true` or `false` |
| `Str` | pointer | UTF-8 string |

### Composite Types

| Type | Syntax | Example |
|------|--------|---------|
| Array | `[T]` | `[I32]`, `[Str]` |
| Tuple | `(T1, T2)` | `(I32, Str)` |
| Nullable | `T?` | `I32?`, `Str?` |
| Result | `Result<T>` | `Result<Str>` |
| HashMap | `HashMap<K, V>` | `HashMap<Str, I32>` |
| Function | `Fn(T) -> R` | `Fn(I32) -> Bool` |
| Pointer | `*T`, `*mut T` | `*I32`, `*void` |
| Reference | `&T`, `&mut T` | `&I32`, `&mut Str` |

### Type Casting

```
let x: I32 = 42
let y: F64 = x as F64       // 42.0
let z: I32 = 3.14 as I32    // 3
```

---

## Variables and Constants

### Variables

```
let name: Str = "KonScript"     // immutable
let mut score: I32 = 0          // mutable

score = 100                     // OK — score is mutable
// name = "other"               // ERROR — name is immutable
```

Variables must be initialized at declaration. Type annotations are required.

### Constants

```
const MAX_HP: I32 = 100
const PI: F64 = 3.14159
const GAME_TITLE: Str = "My Game"
```

Constants are compile-time values. They must be initialized with a literal.

---

## Functions

### Basic Functions

```
func add(a: I32, b: I32) -> I32 {
    return a + b
}

func greet(name: Str) {
    Print(f"Hello, {name}!")
}
```

### Tuple Returns

```
func divide(a: I32, b: I32) -> (I32, I32) {
    return (a / b, a % b)
}
```

### Recursion

```
func factorial(n: I32) -> I32 {
    if n <= 1 { return 1 }
    return n * factorial(n - 1)
}
```

### Public Functions (in Nodes)

```
node Player : Node2D {
    pub func takeDamage(amount: I32) {
        // accessible from other nodes
    }
}
```

---

## Control Flow

### If / Else

```
if health <= 0 {
    Print("Game Over")
} else if health < 20 {
    Print("Low health!")
} else {
    Print("All good")
}
```

### Switch

```
enum Direction { North, South, East, West }

switch dir {
    case Direction::North { Print("Going north") }
    case Direction::South { Print("Going south") }
    default { Print("Going somewhere") }
}
```

---

## Loops

### While Loop

```
let mut i: I32 = 0
while i < 10 {
    Print(i)
    i += 1
}
```

### Infinite Loop

```
loop {
    if done { break }
}
```

### For-In Range

```
// Exclusive range: 0, 1, 2, ..., 9
for i: I32 in 0..10 {
    Print(i)
}

// Inclusive range: 1, 2, 3, ..., 10
for i: I32 in 1..=10 {
    Print(i)
}
```

### For-In Collection

```
let names: [Str] = ["Alice", "Bob", "Charlie"]
for name in names {
    Print(name)
}
```

### C-Style For Loop

```
for i: I32 = 0; i < 100; i++ {
    Print(i)
}
```

### Labelled Loops

```
'outer: for i: I32 in 0..10 {
    for j: I32 in 0..10 {
        if i * j > 25 { break 'outer }
    }
}
```

---

## Strings

### String Literals and F-Strings

```
let name: Str = "KonScript"
let greeting: Str = f"Hello, {name}!"
let math: Str = f"2 + 2 = {2 + 2}"
```

### String Methods

```
let s: Str = "  Hello, World!  "

s.len()                    // 17
s.trim()                   // "Hello, World!"
s.upper()                  // "  HELLO, WORLD!  "
s.lower()                  // "  hello, world!  "
s.contains("World")        // true
s.starts("  Hello")        // true
s.ends("!  ")              // true
s.substr(2, 5)             // "Hello"
s.replace("World", "KS")  // "  Hello, KS!  "
s.split(",")               // ["  Hello", " World!  "]
s.isEmpty()                // false
s.charAt(2)                // "H"
s.toCharCode()             // 32 (space)
Str.fromCharCode(65)       // "A"
s.toInt()                  // 0 (leading spaces handled)
```

---

## Arrays

### Creation and Access

```
let mut arr: [I32] = [1, 2, 3, 4, 5]

arr[0]              // 1
arr[4]              // 5
arr.len()           // 5
```

### Methods

```
arr.push(6)         // add to end → [1, 2, 3, 4, 5, 6]
arr.pop()           // remove from end → 6
arr.has(3)          // true
arr.isEmpty()       // false
arr.clear()         // remove all elements
arr.clone()         // deep copy (ownership-safe)
```

### Iteration

```
for item in arr {
    Print(item)
}
```

---

## Hash Maps

```
let mut scores: HashMap<Str, I32> = HashMap()

scores.set("Alice", 100)
scores.set("Bob", 85)

scores.get("Alice")        // 100 (returns I32?)
scores.has("Charlie")      // false
scores.len()               // 2
scores.remove("Bob")       // removes entry
```

The `.get()` method returns a nullable type since the key may not exist:

```
let val: I32? = scores.get("Alice")
let safe: I32 = val ?? 0              // default if null
```

---

## Structs

Structs are value types — they're copied on assignment.

```
struct Vec2 {
    x: F64,
    y: F64,
}

struct Player {
    name: Str,
    position: Vec2,
    health: I32,
}

let p: Player = Player {
    name: "Hero",
    position: Vec2 { x: 0.0, y: 0.0 },
    health: 100,
}

Print(p.name)           // "Hero"
Print(p.position.x)     // 0.0
```

---

## Enums

### Simple Enums

```
enum Color { Red, Green, Blue }

let c: Color = Color::Red
```

### Enums with Payloads (Tagged Unions)

```
enum Shape {
    Circle(F64),               // radius
    Rectangle(F64, F64),       // width, height
}

func area(s: Shape) -> F64 {
    switch s {
        case Shape::Circle(r) { return 3.14159 * r * r }
        case Shape::Rectangle(w, h) { return w * h }
    }
    return 0.0
}

let s: Shape = Shape::Circle(5.0)
Print(area(s))    // 78.5398
```

---

## Classes

Classes are reference types — they're passed by pointer. Use classes when you need
mutable state and methods.

```
class Stack {
    let mut items: [I32] = [0]
    let mut size: I32 = 0

    func push(mut self, val: I32) {
        items.push(val)
        size += 1
    }

    func pop(mut self) -> I32? {
        if size == 0 { return null }
        size -= 1
        return items.pop()
    }

    func peek(self) -> I32? {
        if size == 0 { return null }
        return items[size - 1]
    }
}

let mut s: Stack = Stack()
s.push(10)
s.push(20)
Print(s.pop())    // 20
```

---

## Nullable Types

Any type can be made nullable with `?`. This prevents null pointer crashes at compile time.

```
let mut name: Str? = "Alice"
name = null                    // OK — it's nullable

// Must handle null before using the value:
let safe: Str = name ?? "Unknown"           // null coalescing
let forced: Str = name!                     // force unwrap (crashes if null)

// Safe member access:
let len: I32? = name?.len()                 // returns null if name is null
```

### Pattern

```
func findUser(id: I32) -> Str? {
    if id == 1 { return "Alice" }
    return null
}

let user: Str = findUser(42) ?? "Guest"
```

---

## Result Types and Error Handling

`Result<T>` represents an operation that can succeed or fail.

```
let result: Result<Str> = File.read("config.txt")

if result.ok {
    Print(result.value)
} else {
    Print(f"Error: {result.error}")
}
```

### File I/O (returns Result)

```
// Reading
let r: Result<Str> = File.read("data.txt")

// Writing
let w: Result<Str> = File.write("out.txt", "hello")

// Appending
let a: Result<Str> = File.append("log.txt", "entry\n")

// Checking existence
let exists: Bool = File.exists("config.txt")

// Deleting
let d: Result<Str> = File.delete("temp.txt")

// Reading lines as array
let lines: [Str] = File.lines("data.txt")
```

---

## Closures and Function Types

### Anonymous Functions (Closures)

```
let double: Fn(I32) -> I32 = func(x: I32) -> I32 {
    return x * 2
}

Print(double(21))    // 42
```

### Higher-Order Functions

```
func apply(arr: [I32], f: Fn(I32) -> I32) -> [I32] {
    let mut result: [I32] = [0]
    result.clear()
    for item in arr {
        result.push(f(item))
    }
    return result
}

let nums: [I32] = [1, 2, 3, 4]
let doubled: [I32] = apply(nums, func(x: I32) -> I32 { return x * 2 })
```

### Closures Capture Variables

```
let multiplier: I32 = 3
let mul: Fn(I32) -> I32 = func(x: I32) -> I32 {
    return x * multiplier    // captures 'multiplier' from outer scope
}
```

---

## Generics

### Generic Functions

```
func max<T>(a: T, b: T) -> T {
    if a > b { return a }
    return b
}

Print(max(3, 7))            // 7
Print(max(1.5, 2.3))        // 2.3
```

### Generic Structs

```
struct Pair<A, B> {
    first: A,
    second: B,
}

let p: Pair<Str, I32> = Pair { first: "age", second: 25 }
```

### Generic Classes

```
class Stack<T> {
    let mut items: [T] = [0]

    func push(mut self, val: T) {
        items.push(val)
    }

    func pop(mut self) -> T? {
        if items.isEmpty() { return null }
        return items.pop()
    }
}

let mut s: Stack<Str> = Stack()
s.push("hello")
```

---

## Interfaces

Interfaces define shared behavior across types (like Rust traits).

```
interface Drawable {
    func draw(self)
    func area(self) -> F64
}

class Circle implements Drawable {
    let radius: F64 = 0.0

    func draw(self) {
        Print(f"Drawing circle r={radius}")
    }

    func area(self) -> F64 {
        return 3.14159 * radius * radius
    }
}
```

### Mutable Self

```
interface Counter {
    func increment(mut self)
    func count(self) -> I32
}
```

---

## Coroutines

Cooperative coroutines for async-style game logic.

```
func patrol() {
    Print("Walking left")
    wait 2.0                // suspend for 2 seconds
    Print("Walking right")
    wait 2.0
}

// In your game update:
spawn patrol()

// In the engine update loop:
node Enemy : Node2D {
    func Update(dt: F64) {
        _ks_sched.update(dt)     // tick the scheduler
    }
}
```

---

## Ownership and References

KonScript tracks ownership to prevent memory bugs at compile time.

### Move Semantics

Non-copy types (Str, arrays, structs) are **moved** on assignment:

```
let a: [I32] = [1, 2, 3]
let b: [I32] = a              // 'a' is moved to 'b'
// Print(a)                    // ERROR: use of moved value 'a'
```

To keep both variables, use `.clone()`:

```
let a: [I32] = [1, 2, 3]
let b: [I32] = a.clone()      // deep copy
Print(a)                       // OK — 'a' still valid
```

### Copy Types

Primitives (I32, F64, Bool, etc.) are always copied:

```
let a: I32 = 42
let b: I32 = a                 // copy, not move
Print(a)                       // OK
```

### References

Borrow a value without taking ownership:

```
func print_len(arr: &[I32]) {
    Print(arr.len())
}

let data: [I32] = [1, 2, 3]
print_len(&data)               // borrow — data still valid
```

### Mutable References

```
func add_item(arr: &mut [I32], val: I32) {
    arr.push(val)
}

let mut data: [I32] = [1, 2, 3]
add_item(&mut data, 4)
```

### Borrow Rules

- Multiple shared references (`&T`) are allowed simultaneously
- Only one mutable reference (`&mut T`) at a time
- No shared references while a mutable reference exists

---

## Extern and FFI

Call C functions directly from KonScript:

```
extern "C" func puts(s: Str) -> I32;
extern "C" func printf(fmt: Str, ...) -> I32;
extern "C" func malloc(size: U64) -> *void;
extern "C" func free(ptr: *void);

func main() -> I32 {
    puts("Hello from C!")
    let ptr: *void = malloc(1024)
    free(ptr)
    return 0
}
```

### Linking with C Libraries

```
extern "C" func SDL_Init(flags: U32) -> I32;
extern "C" func SDL_CreateWindow(title: Str, x: I32, y: I32, w: I32, h: I32, flags: U32) -> *void;
```

---

## Inline Assembly

For systems programming and OS development:

```
func halt() {
    asm("cli; hlt")
}

func read_port(port: U16) -> U8 {
    let mut val: U8 = 0
    asm("inb %1, %0" : "=a"(val) : "Nd"(port))
    return val
}
```

---

## Bitwise Operations

```
let a: I32 = 0xFF
let b: I32 = 0x0F

a & b          // AND   → 0x0F
a | b          // OR    → 0xFF
a ^ b          // XOR   → 0xF0
~a             // NOT   → inverted bits
a << 4         // shift left  → 0xFF0
a >> 4         // shift right → 0x0F

// Compound assignment
let mut x: I32 = 0xFF
x &= 0x0F     // x = x & 0x0F
x |= 0xF0     // x = x | 0xF0
x ^= 0xFF     // x = x ^ 0xFF
```

---

## Game Development with KonEngine

### Minimal Game

```
#include <engine>

node Player : Node2D {
    let mut speed: F64 = 200.0

    func Ready() {
        x = 400.0
        y = 300.0
    }

    func Update(dt: F64) {
        if KeyDown("d") { x = x + speed * dt }
        if KeyDown("a") { x = x - speed * dt }
        if KeyDown("w") { y = y - speed * dt }
        if KeyDown("s") { y = y + speed * dt }
    }

    func Draw() {
        DrawRect(x, y, 32.0, 32.0, Color::Blue)
    }
}

func main() -> I32 {
    InitWindow(800, 600, "My Game")
    SetVSync(true)

    let mut scene: Scene = Scene()
    let player: Player = Player()
    scene.add(player)

    while !WindowShouldClose() {
        let dt: F64 = GetDeltaTime()
        ClearBackground(Color::Black)
        scene.update(dt)
        scene.draw()
        Present()
    }
    return 0
}
```

### Node Lifecycle

Every node has these methods called automatically:

| Method | When Called | Purpose |
|--------|-----------|---------|
| `Ready()` | Once, when added to scene | Initialize state |
| `Update(dt: F64)` | Every frame | Game logic |
| `Draw()` | Every frame, after Update | Rendering |
| `OnCollisionEnter(other: Collider2D)` | On collision start | React to collisions |
| `OnCollisionExit(other: Collider2D)` | On collision end | Cleanup after collision |

### Inherited Node Properties

All nodes inheriting from `Node2D` have:

| Property | Type | Description |
|----------|------|-------------|
| `x`, `y` | F64 | Position |
| `scaleX`, `scaleY` | F64 | Scale |
| `rotation` | F64 | Rotation in radians |
| `originX`, `originY` | F64 | Pivot point |
| `alpha` | F64 | Opacity (0.0 - 1.0) |
| `width`, `height` | F64 | Size |
| `z` | F64 | Z-order (draw depth) |
| `active` | Bool | Is node active |
| `visible` | Bool | Is node visible |
| `name` | Str | Node name |

### Node Types

| Type | Extras |
|------|--------|
| `Node2D` | Base — position, scale, rotation |
| `Sprite2D` | `flipH`, `flipV` |
| `AnimatedSprite2D` | Sprite sheet animation |
| `Collider2D` | `radius`, `solid`, `staticBody`, `layer`, `mask` |
| `CameraNode2D` | `zoom`, `rotation`, `current` |
| `AnimationPlayer` | Keyframe animation playback |

---

## Engine API Reference

### Window

```
InitWindow(width: I32, height: I32, title: Str)
WindowShouldClose() -> Bool
GetWindowWidth() -> I32
GetWindowHeight() -> I32
SetVSync(on: Bool)
```

### Time

```
GetDeltaTime() -> F64
GetFPS() -> I32
GetTime() -> F64
```

### Rendering

```
ClearBackground(color: Color)
Present()
DrawRect(x: F64, y: F64, w: F64, h: F64, color: Color)
DrawCircle(x: F64, y: F64, radius: F64, color: Color)
DrawLine(x1: F64, y1: F64, x2: F64, y2: F64, color: Color)
DrawText(text: Str, x: F64, y: F64, size: F64, color: Color)
```

### Input — Keyboard

```
KeyDown(key: Str) -> Bool         // held this frame
KeyPressed(key: Str) -> Bool      // just pressed
KeyReleased(key: Str) -> Bool     // just released
```

Keys: `"a"`-`"z"`, `"0"`-`"9"`, `"space"`, `"enter"`, `"escape"`, `"up"`, `"down"`, `"left"`, `"right"`, `"shift"`, `"ctrl"`, `"tab"`

### Input — Mouse

```
MouseX() -> F64
MouseY() -> F64
MouseDown(button: I32) -> Bool    // 0=left, 1=right, 2=middle
MousePressed(button: I32) -> Bool
MouseReleased(button: I32) -> Bool
```

### Input — Gamepad

```
GamepadConnected(id: I32) -> Bool
GamepadAxis(id: I32, axis: I32) -> F64
GamepadButton(id: I32, button: I32) -> Bool
```

### Audio

```
PlaySound(path: Str)
PlayMusic(path: Str)
StopMusic()
SetMusicVolume(vol: F64)
SetSoundVolume(vol: F64)
```

### Debug

```
DebugMode(on: Bool)
IsDebugMode() -> Bool
```

### Output

```
Print(args...)                  // print to stdout
ToString(val) -> Str            // convert any value to string
```

### Color Presets

`Color::Red`, `Color::Green`, `Color::Blue`, `Color::White`, `Color::Black`,
`Color::Yellow`, `Color::Cyan`, `Color::Magenta`, `Color::Orange`, `Color::Purple`,
`Color::Gray`, `Color::DarkGray`, `Color::LightGray`, `Color::Pink`, `Color::Brown`,
`Color::DarkGreen`, `Color::DarkBlue`, `Color::SkyBlue`, `Color::Gold`

---

## Multi-File Projects

### Using #include

```
// player.ks
func create_player() -> Player {
    return Player()
}

// main.ks
#include "player.ks"

func main() -> I32 {
    let p: Player = create_player()
    return 0
}
```

### CMake Integration

```cmake
# CMakeLists.txt
konscript_sources(
    SOURCES game.ks player.ks enemy.ks
    OUTPUT  game
)
```

---

## Use Cases

### Game Development
KonScript's primary purpose. The node/scene system, engine API, and rendering
primitives make it simple to build 2D games.

### Systems Programming
With `extern "C"`, inline assembly, raw pointers, and freestanding compilation,
KonScript can target bare-metal environments for OS development.

### Standalone Tools
KonScript compiles to fully static binaries with no dependencies.
File I/O, hashmaps, strings, and arrays provide enough stdlib for CLI tools and utilities.

### Scripting
The C++ transpilation mode allows embedding KonScript in existing C++ projects.
Functions compile to regular C++ functions callable from any codebase.

---

## Comparison with Other Languages

| Feature | KonScript | C++ | Rust | GDScript |
|---------|-----------|-----|------|----------|
| **Compilation** | Native (LLVM) | Native | Native | Interpreted |
| **Type System** | Static | Static | Static | Dynamic |
| **Null Safety** | T? with ?? | No | Option<T> | No |
| **Ownership** | Move + Borrow | Manual | Move + Borrow | GC |
| **Generics** | Yes | Templates | Yes | No |
| **Closures** | Yes | Yes | Yes | Yes |
| **Game Engine** | Built-in | DIY | DIY | Godot |
| **FFI** | extern "C" | Native | extern | GDExtension |
| **Inline ASM** | Yes | Yes | Yes | No |
| **Binary Size** | ~100KB+ | ~100KB+ | ~200KB+ | N/A |
| **Startup Time** | Instant | Instant | Instant | ~1s |
| **Learning Curve** | Low | High | High | Low |
