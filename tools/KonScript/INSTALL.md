# KonScript Installation Guide

> Quick setup for Linux and Windows. From zero to compiling KonScript programs in under 15 minutes.

---

## Table of Contents

1. [Quick Start (Linux)](#quick-start-linux)
2. [Quick Start (Windows)](#quick-start-windows)
3. [Full Installation (Linux)](#full-installation-linux)
4. [Full Installation (Windows)](#full-installation-windows)
5. [Cross-Compilation](#cross-compilation)
6. [Verifying Your Installation](#verifying-your-installation)
7. [Troubleshooting](#troubleshooting)

---

## Quick Start (Linux)

### Prerequisites

```bash
# Ubuntu / Debian
sudo apt install g++ llvm-17 lld-17 cmake wget make

# Fedora
sudo dnf install gcc-c++ llvm lld cmake wget make

# Arch
sudo pacman -S gcc llvm lld cmake wget make
```

### Build & Install

```bash
git clone https://github.com/AnoDelta/KonEngine.git
cd KonEngine/tools/KonScript

# 1. Build the self-hosted compiler (5-stage bootstrap, installs Stage 4)
./build.sh

# 2. Install system-wide
sudo ./install.sh

# Optional: bundle the LLVM toolchain for Stage 0 builds
./bundle-toolchain.sh

# Optional: build the engine library for game development
./build-engine-lib.sh
```

### Test It

```bash
# Create a test file
cat > hello.ks << 'EOF'
func main() -> I32 {
    Print("Hello from KonScript!");
    return 0;
}
EOF

# Compile and run
konscript hello.ks
./hello
```

---

## Quick Start (Windows)

### Prerequisites

Install one of:
- **Visual Studio 2019+** with "Desktop development with C++" workload
- **MSYS2** with MinGW-w64: `pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-cmake`
- **LLVM/Clang** from https://releases.llvm.org

### Build & Install

Open a terminal (Developer Command Prompt for MSVC, or MSYS2/Git Bash):

```cmd
git clone https://github.com/AnoDelta/KonEngine.git
cd KonEngine\tools\KonScript

REM Build the compiler
build.bat

REM For native compilation, you also need LLVM tools.
REM Option A: Install LLVM and add to PATH
REM Option B: Use C++ transpilation mode (no LLVM needed):
konscript --cpp hello.ks -o hello.cpp
cl /EHsc /std:c++17 hello.cpp /Fe:hello.exe
```

### Test It

```cmd
echo func main() -^> I32 { Print("Hello!"); return 0; } > hello.ks
konscript --cpp hello.ks -o hello.cpp
cl /EHsc /std:c++17 hello.cpp /Fe:hello.exe
hello.exe
```

---

## Full Installation (Linux)

### Step 1: Install System Dependencies

KonScript needs a C++17 compiler and LLVM 15+ to build. The bundled toolchain
makes the compiled `konscript` binary fully self-contained afterwards.

```bash
# Required
sudo apt install g++ cmake make wget

# Required for native compilation (pick one LLVM version)
sudo apt install llvm-17 lld-17 clang-17
# OR: sudo apt install llvm-16 lld-16 clang-16
# OR: sudo apt install llvm-15 lld-15 clang-15

# Optional: for Windows cross-compilation
# (bundle-toolchain.sh downloads llvm-mingw automatically)
```

### Step 2: Build the Compiler

```bash
cd KonEngine/tools/KonScript
./build.sh
```

This compiles `src/main.cpp` into the `konscript` binary using your system C++ compiler.
If `./toolchain/` already exists, the path is baked into the binary so it works from anywhere.

**Output:** `./konscript` (~2MB binary)

### Step 3: Bundle the Toolchain

```bash
./bundle-toolchain.sh
```

This creates a self-contained `./toolchain/` directory (~1GB) containing:

| Component | Purpose | Location |
|-----------|---------|----------|
| `llc` | LLVM compiler (IR → object code) | `toolchain/llvm/bin/` |
| `ld.lld` | ELF linker | `toolchain/llvm/bin/` |
| `clang++` | C++ compiler (for engine builds) | `toolchain/llvm/bin/` |
| `llvm-ar` | Archive tool | `toolchain/llvm/bin/` |
| musl libc | Static C library | `toolchain/sysroot/linux64/lib/` |
| CRT objects | Startup code (crt1.o, crti.o, crtn.o) | `toolchain/sysroot/linux64/lib/` |

**Options:**
```bash
./bundle-toolchain.sh --prefix=/opt/konscript/toolchain  # Custom location
./bundle-toolchain.sh --musl-version=1.2.5               # Specific musl version
```

### Step 4: Build the Engine Library

```bash
./build-engine-lib.sh
```

Pre-compiles KonEngine into a static library so KonScript game files can link against it.

**Output:**
- `toolchain/engine/linux64/libKonEngine.a` — Engine static library
- `toolchain/engine/linux64/libglfw3.a` — GLFW window library
- `toolchain/engine/linux64/include/` — Engine headers

### Step 5: Install (Optional)

```bash
sudo ./install.sh                    # Installs to /usr/local/bin/
./install.sh --prefix=~/.local       # Install to home directory
```

Installs `konscript`, `ksc` (frontend runner), and `_ks_runtime.c` to the prefix.

### Step 6: Rebuild with Baked Path

After bundling the toolchain, rebuild the compiler so the toolchain path is baked in:

```bash
./build.sh   # Detects ./toolchain/ and bakes the absolute path
```

Now `konscript` works from any directory without needing `KONSCRIPT_TOOLCHAIN` set.

---

## Full Installation (Windows)

### Option A: Native Windows Build (MSVC)

1. Install **Visual Studio 2019+** with "Desktop development with C++"
2. Open **Developer Command Prompt for VS**:

```cmd
cd KonEngine\tools\KonScript
build.bat --msvc
```

3. Use C++ transpilation mode (no LLVM needed):

```cmd
konscript --cpp game.ks -o game.cpp
cl /EHsc /std:c++17 /O2 game.cpp /Fe:game.exe
```

### Option B: Native Windows Build (MinGW)

1. Install **MSYS2** from https://www.msys2.org
2. Open MSYS2 MinGW64 terminal:

```bash
pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-cmake
cd KonEngine/tools/KonScript
build.bat --mingw
```

3. Compile KonScript files:

```bash
konscript --cpp game.ks -o game.cpp
g++ -std=c++17 -O2 game.cpp -o game.exe
```

### Option C: Cross-Compile from Linux

Build Windows executables without Windows. See [Cross-Compilation](#cross-compilation).

---

## Cross-Compilation

### Linux → Windows

```bash
# Bundle the Windows toolchain (one-time, downloads llvm-mingw ~500MB)
./bundle-toolchain.sh   # Automatically sets up Windows cross-compile

# Cross-compile any .ks file to a Windows .exe
konscript --target windows64 game.ks
# Output: game.exe (runs on Windows x86_64)
```

### Linux → WebAssembly

```bash
konscript --target wasm32 game.ks
# Output: game.wasm
```

### Custom LLVM Targets

```bash
konscript --target custom:aarch64-linux-gnu game.ks
```

---

## Verifying Your Installation

### Check Compiler

```bash
konscript --help
```

### Run the Test Suite

```bash
cd KonEngine/tools/KonScript
./run_tests.sh
```

Expected output: `11/11 tests passed`

### Compile a Standalone Program

```bash
cat > test.ks << 'EOF'
func fib(n: I32) -> I32 {
    if n <= 1 { return n; }
    return fib(n - 1) + fib(n - 2);
}

func main() -> I32 {
    Print("fib(10) = ", fib(10));
    return 0;
}
EOF

konscript test.ks
./test
# Output: fib(10) = 55
```

### Compile an Engine Game

```bash
cat > game.ks << 'EOF'
#include <engine>

node Player : Node2D {
    func Ready() {
        Print("Player ready!");
        x = 100.0;
        y = 100.0;
    }

    func Update(dt: F64) {
        x = x + 60.0 * dt;
    }

    func Draw() {
        // Drawing handled by engine
    }
}

func main() -> I32 {
    let mut scene: Scene = Scene();
    let player: Player = Player();
    scene.add(player);
    scene.run();
    return 0;
}
EOF

konscript game.ks
./game
```

---

## Troubleshooting

### "llc not found" or "toolchain not found"

The bundled toolchain is missing or not found. Either:
- Run `./bundle-toolchain.sh` to create it
- Set the environment variable: `export KONSCRIPT_TOOLCHAIN=/path/to/toolchain`
- Rebuild the compiler after bundling: `./build.sh` (bakes the path)

### "ld.lld: error: undefined symbol"

Missing runtime or engine library. Make sure you ran:
```bash
./build-engine-lib.sh   # For engine games
```

For standalone programs (no `#include <engine>`), ensure `_ks_runtime.c` is accessible.

### "clang++: command not found" during bundle

Install LLVM:
```bash
sudo apt install clang-17 llvm-17 lld-17
```

### Windows: "cl is not recognized"

Open a **Developer Command Prompt for VS** instead of a regular terminal.
Or add MSVC tools to PATH.

### Permission denied on install.sh

```bash
sudo ./install.sh
# OR install to user directory:
./install.sh --prefix=~/.local
```

### Cross-compile fails for Windows target

Ensure llvm-mingw was bundled:
```bash
ls toolchain/llvm-mingw/bin/
# Should contain x86_64-w64-mingw32-clang++
```

If missing, re-run `./bundle-toolchain.sh`.

---

## Directory Structure After Installation

```
KonEngine/tools/KonScript/
├── konscript              # Compiler binary
├── _ks_runtime.c          # Runtime (linked into every native binary)
├── build.sh               # Build script (Linux)
├── build.bat              # Build script (Windows)
├── bundle-toolchain.sh    # Toolchain bundler
├── build-engine-lib.sh    # Engine library builder
├── install.sh             # System installer
├── DOCS.md                # Language reference
├── INSTALL.md             # This file
├── toolchain/             # Self-contained build tools (~1GB)
│   ├── llvm/bin/          # llc, ld.lld, clang++, llvm-ar
│   ├── sysroot/
│   │   ├── linux64/lib/   # musl libc, CRT objects
│   │   └── windows64/lib/ # MinGW CRT (if bundled)
│   ├── llvm-mingw/        # Windows cross-compiler (if bundled)
│   └── engine/
│       ├── linux64/       # libKonEngine.a, libglfw3.a, headers
│       └── windows64/     # Windows engine lib (if bundled)
├── src/                   # Compiler source code
├── include/               # Compiler headers (parser, codegen, etc.)
├── tests/                 # Test suite
└── konscript.ks           # Self-hosted compiler (Stage 1)
```

---

## Environment Variables

| Variable | Purpose | Default |
|----------|---------|---------|
| `KONSCRIPT_TOOLCHAIN` | Override toolchain location | Auto-detected (baked at build time) |

Most users never need to set any environment variables. The toolchain path is baked into the binary during `./build.sh`.
