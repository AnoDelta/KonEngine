# KonScript

A statically typed, self-hosting programming language that compiles to C++. Designed for game development with KonEngine, systems programming, and general-purpose use.

## Features

- **Self-hosting** — the compiler compiles itself (byte-identical across stages)
- **C++ codegen** — generates readable C++ that links with any C/C++ library
- **FFI** — `extern "C"` to call any C library (SDL2, Qt, POSIX, etc.)
- **Inline assembly** — `asm("nop")` for systems/OS development
- **Engine integration** — first-class KonEngine node/scene system
- **Cross-platform** — Linux, Windows (cross-compile), WebAssembly
- **CLI compiler** — `-I`, `-L`, `-l`, `-o`, `--cpp`, `--no-stdlib` flags

## Quick Start

```bash
# Build the compiler
./build.sh

# Compile a program
./konscript hello.ks -o hello
./hello
```

### Hello World

```
func main() -> I32 {
    Print("Hello, world!");
    return 0;
}
```

## Installation

### Linux
```bash
git clone https://github.com/AnoDelta/KonEngine.git
cd KonEngine/tools/KonScript
./build.sh                    # Build Stage 0 (C++ compiler)
./konscript-stage0 konscript.ks -o konscript  # Build self-hosted compiler
sudo cp konscript /usr/local/bin/
```

### From CMake
```bash
cd tools/KonScript
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
sudo cmake --install build
```

## CLI Usage

```
konscript [options] <file.ks>

Options:
  -o <file>        Output file name
  -I<dir>          Add include search directory
  -L<dir>          Add library search directory
  -l<lib>          Link library
  --cpp            Output C++ source only
  --no-stdlib      Don't link runtime (for OS dev)
  --help, -h       Show help
```

### Examples

```bash
# Basic compilation
konscript game.ks                          # → ./game
konscript game.ks -o mygame                # → ./mygame

# C++ output only
konscript --cpp game.ks -o game.cpp        # → game.cpp

# Link external libraries
konscript app.ks -I/usr/include/SDL2 -lSDL2
konscript gui.ks -I/usr/include/qt6 -lQt6Widgets -lQt6Core

# OS development (no standard library)
konscript kernel.ks --no-stdlib

# Self-compile
konscript konscript.ks -o konscript2       # Produces identical binary
```

## Language Overview

### Types
```
I8, I16, I32, I64          // Signed integers
U8, U16, U32, U64          // Unsigned integers
F32, F64                    // Floats
Bool                        // true / false
Str                         // String
[T]                         // Array
HashMap<K, V>               // Hash map
Result<T>                   // Error handling
T?                          // Nullable
```

### Functions
```
func add(a: I32, b: I32) -> I32 {
    return a + b;
}
```

### Structs
```
struct Vec2 {
    x: F64,
    y: F64,
}
```

### Calling C Libraries (FFI)
```
extern "C" func printf(fmt: Str, ...) -> I32;
extern "C" func malloc(size: U64) -> I64;
extern "C" func free(ptr: I64);

func main() -> I32 {
    printf("Hello from C! %d\n", 42);
    return 0;
}
```

### Inline Assembly
```
func halt() {
    asm("cli; hlt");
}
```

### Engine Games
```
#include <engine>

node Player : Node2D {
    let mut speed: F64 = 200.0;

    func Ready() {
        x = 100.0;
        y = 100.0;
    }

    func Update(dt: F64) {
        if KeyDown("d") { x = x + speed * dt; }
        if KeyDown("a") { x = x - speed * dt; }
    }

    func Draw() {
        DrawRect(x, y, 32.0, 32.0, Color::Blue);
    }
}
```

## Use Cases

### Game Development
First-class KonEngine integration with nodes, scenes, sprites, collisions, audio, and input.

### Systems Programming / OS Development
With `extern "C"`, inline `asm`, `--no-stdlib`, and pointer types, KonScript can target bare-metal environments. The compiler generates C++ which can be compiled with any cross-compiler.

### CLI Tools
Compile to static binaries with zero runtime dependencies. File I/O, strings, arrays, and hashmaps are built-in.

### Library Bindings
Use `extern "C"` and `-I`/`-l` flags to call any C library: SDL2, OpenGL, Vulkan, POSIX, Qt (C API), etc.

## Benefits

- **Simple syntax** — Rust-inspired but easier to learn
- **Fast compilation** — generates C++, compiled by g++/clang++
- **Zero runtime overhead** — no GC, no VM, native performance
- **Self-hosting** — compiler compiles itself, proving language stability
- **Interop** — call any C library with one line
- **Cross-platform** — same code compiles on Linux, Windows, WebAssembly
- **Game-ready** — KonEngine integration out of the box

## Downsides

- **Young language** — limited ecosystem, no package manager yet
- **C++ dependency** — needs g++ or clang++ installed to compile output
- **No REPL** — compile-only, no interactive mode
- **Limited error messages** — typechecker errors could be more descriptive
- **No generics in self-hosted** — Stage 0 supports generics, self-hosted doesn't yet
- **No debugger integration** — debug via Print statements or GDB on generated C++

## Architecture

```
source.ks → Lexer → Parser → Typechecker → C++ Codegen → g++/clang++ → binary
```

The compiler is written in KonScript itself (~4800 lines). It:
1. **Lexes** source into tokens
2. **Parses** into an AST (parallel arrays)
3. **Typechecks** with scope-based inference
4. **Generates C++** via modular codegen
5. **Compiles** with system C++ compiler
6. **Links** with runtime + user libraries

## Maintenance Guide

### Adding a New Engine API Function
Edit `konscript.ks`, function `cg_engine_func()`:
```
if name == "MyNewFunc" { return "CppEquivalent"; }
```

### Adding a New Node Lifecycle Method
Edit `konscript.ks`, function `cg_lifecycle_sig()`:
```
if method_name == "OnDamage" { return "void OnDamage(int amount) override"; }
```

### Adding a New Keyword
1. Add `const TK_MYKEYWORD: I32 = N;` in token constants
2. Add `if w == "mykeyword" { return TK_MYKEYWORD; }` in `keyword_kind()`
3. Handle in `parse_top_level()` or `parse_stmt()`
4. Add codegen in `cg_gen_stmt()` or `cg_gen_expr()`

### Adding a New String Method
Edit `cg_gen_expr()` method call section:
```
if method == "myMethod" { return "_ks_myMethod(" + obj + ")"; }
```
Add C wrapper to `_ks_runtime.c` and declare in codegen header.

## Editor Support

- **Neovim**: `editor/install.sh --nvim` (syntax, keybindings, F5 run)
- **VS Code**: `editor/install.sh --vscode` (TextMate grammar, language config)
- **LSP**: `editor/install.sh --lsp` (real-time diagnostics)
- **KonScriptIDE**: Standalone Qt editor with auto-complete and build integration

## Self-Hosting

The compiler compiles itself through infinite bootstrap stages:
```
Stage 0 (C++) → konscript.ks → Stage 1
Stage 1       → konscript.ks → Stage 2
Stage 2       → konscript.ks → Stage 3
Stages 2+ produce byte-identical binaries
```

## Splitting KonScript Into Its Own Repo

To make KonScript independent from KonEngine:

```bash
# 1. Create new repo on GitHub: AnoDelta/KonScript

# 2. Extract from KonEngine
cd KonEngine
git subtree split -P tools/KonScript -b konscript-split

# 3. Create new repo and push
mkdir /tmp/konscript-repo && cd /tmp/konscript-repo
git init && git pull /path/to/KonEngine konscript-split
git remote add origin https://github.com/AnoDelta/KonScript.git
git push -u origin main

# 4. In KonEngine, replace with submodule
cd KonEngine
git rm -r tools/KonScript
git submodule add https://github.com/AnoDelta/KonScript tools/KonScript
git commit -m "chore: replace KonScript with submodule"
```

## License

Part of KonEngine. See repository root for license information.
