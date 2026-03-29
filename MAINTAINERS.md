# Maintainer Guide

This document is for people contributing to or maintaining KonEngine.

---

## Table of Contents

1. [Project Structure](#1-project-structure)
2. [Building](#2-building)
3. [Testing](#3-testing)
4. [Release Process](#4-release-process)
5. [Versioning Policy](#5-versioning-policy)
6. [Adding Engine Features](#6-adding-engine-features)
7. [Adding KonScript Builtins](#7-adding-konscript-builtins)
8. [CI/CD](#8-cicd)
9. [Known Constraints](#9-known-constraints)

---

## 1. Project Structure

```
KonEngine/
├── src/                        Engine source
│   ├── KonEngine.hpp           Single include for users
│   ├── node/                   Node system (Node, Node2D, Sprite2D, Collider2D, Scene)
│   ├── collision/              CollisionWorld, SAT
│   ├── animation/              AnimationPlayer, curves
│   ├── renderer/               OpenGL renderer
│   ├── window/                 Window, input, camera
│   ├── audio/                  miniaudio wrapper
│   ├── font/                   stb_truetype wrapper
│   ├── time/                   Delta time, FPS, GetTime
│   └── math/                   Vector2
├── tests/
│   └── test_main.cpp           Headless + visual test suite
├── tools/
│   ├── KonAnimator/            Qt animation editor
│   ├── KonPaktor/              Qt asset packer + CLI
│   └── KonScript/              Scripting language compiler
│       ├── include/
│       │   ├── lexer.hpp
│       │   ├── parser.hpp
│       │   ├── typechecker.hpp
│       │   └── codegen.hpp
│       └── src/main.cpp        ksc compiler entry point
├── libs/                       Bundled submodules (glfw, glm)
├── .github/workflows/
│   ├── cmake-single-platform.yml   CI on every push/PR
│   └── release.yml                 Release packaging on tag push
├── build.sh / build.bat        Quick build scripts
└── build-tools.sh              Builds KonAnimator + anim_compiler
```

---

## 2. Building

### Engine only
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

### Engine + tools (KonAnimator, anim_compiler)
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release -DKON_BUILD_TOOLS=ON
cmake --build build
```

### KonPaktor (separate CMake project)
```bash
cd tools/KonPaktor
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

### KonScript compiler
```bash
cd tools/KonScript
./build.sh
./install.sh   # installs ksc and konscript to /usr/local/bin
```

### Windows cross-compile from Linux (MXE)
```bash
./build-windows.sh
```

Requires MXE at `/home/delta/mxe` with `x86_64-w64-mingw32.shared` target.

---

## 3. Testing

```bash
./build-test.sh
```

This builds and runs `tests/test_main.cpp` which has two phases:

**Headless tests** — run automatically, print pass/fail, exit non-zero on failure:
- Easing curves
- Keyframe track sampling
- Animation clip logic
- Node tree (add, remove, traverse)
- Node2D (DrawX/DrawY, Move)
- Signals
- AnimationPlayer (headless, no window)
- CollisionWorld (AABB, circle, SAT, layer/mask, enter/exit)
- DebugMode toggle

**Visual tests** — open a window, requires manual verification:
- Rendering primitives
- Camera zoom/pan
- Collision with signals (WASD to move, see color change on overlap)
- Animation playback

### Before any release

Run the full test suite on:
- Linux (native)
- Windows (Wine or native)
- At least one of Fedora/Arch (via container or CI)

Do not tag a release if any headless test fails. Visual tests require a human
to verify the checklist printed to stdout.

---

## 4. Release Process

1. **Feature freeze** — no new features after this point, only bug fixes
2. **Full test pass** on all platforms (see above)
3. **Update version strings:**
   - `ROADMAP.md` — move items from Upcoming to Done, update version numbers
   - `DOCS.md` — update the version header at the top
   - `tools/KonAnimator/DOCS.md` — update version header
   - Any `> Version X.X.X` lines in docs
4. **Update `ROADMAP.md`** with the full changelog for the new version
5. **Commit everything:**
   ```bash
   git add .
   git commit -m "chore: prepare v0.8.2"
   ```
6. **Tag and push:**
   ```bash
   git tag v0.8.2
   git push origin main --tags
   ```
7. **CI does the rest** — GitHub Actions builds and publishes the release artifacts automatically

### What CI produces per release

| Artifact | Contents |
|---|---|
| `KonEngine-linux-vX.X.X.zip` | Static library + all headers |
| `KonEngine-windows-vX.X.X.zip` | Static library + all headers (MSVC) |
| `KonAnimator-linux-vX.X.X.zip` | KonAnimator + anim_compiler binaries |
| `KonAnimator-windows-vX.X.X.zip` | KonAnimator + anim_compiler + Qt DLLs |
| `KonPaktor-linux-vX.X.X.zip` | KonPaktor GUI + konpak CLI |
| `KonPaktor-windows-vX.X.X.zip` | KonPaktor + konpak + Qt DLLs + zlib1.dll |

---

## 5. Versioning Policy

```
v MAJOR . MINOR . PATCH
```

- **PATCH** (0.8.x) — bug fixes, no new public API, no breaking changes
- **MINOR** (0.x.0) — new features, backwards-compatible API additions
- **MAJOR** (x.0.0) — breaking API changes (reserved for v1.0.0 stable)

**Nothing gets a version number until it passes the full test suite.**
CI failing on main is a blocker — fix it before merging anything else.

Pre-1.0, the API is allowed to change between minor versions but changes
should be documented in ROADMAP.md under the relevant version.

---

## 6. Adding Engine Features

### Adding a new engine function

1. Declare it in `src/window/window.hpp` (or the relevant header)
2. Implement it in the corresponding `.cpp`
3. Include the header in `src/KonEngine.hpp` if it's a new subsystem
4. Add a test in `tests/test_main.cpp`
5. Document it in `DOCS.md`
6. If it needs a KonScript binding, see section 7

### Adding a new node type

1. Create `src/node/yournodetype.hpp`
2. Inherit from `Node`, `Node2D`, or `Sprite2D`
3. Include in `src/KonEngine.hpp`
4. Register with CollisionWorld in `scene.hpp` if it's collidable
5. Add tests
6. Document in `DOCS.md`

### Node system rules

- `Node::Ready()` is called automatically by `Scene::Add()` and `Node::AddChild()`
- `Node::UpdateChildren()` and `DrawChildren()` are virtual — `Node2D` overrides
  both to bake world transforms temporarily before each child's Update/Draw
- `CollisionWorld::Update()` runs **after** all node updates in `Scene::Update()`
  so colliders have world-space positions when SAT runs
- `Collider2D::Emit()` bubbles up to `parent->OnCollisionEnter/Exit()` automatically

---

## 7. Adding KonScript Builtins

KonScript has three places to update when adding a new engine function:

### 1. Typechecker — `tools/KonScript/include/typechecker.hpp`

In `registerBuiltins()`, add:
```cpp
reg("GetWorldMouseX", {Camera2D}, F64);
```

### 2. Codegen — `tools/KonScript/include/codegen.hpp`

In the `builtins` map in `genIdent()`:
```cpp
{"GetWorldMouseX", "GetWorldMouseX"},
```

If it takes a non-trivial argument (like a Camera2D), make sure the call
expression passes through `genCall()` correctly — most functions do automatically.

### 3. Method mappings (for node methods)

In `genNodeMethod()`, the `builtinMethods` vector maps KonScript lifecycle
names to C++ virtual override signatures. Add new lifecycle methods here if
needed.

### After changes to KonScript

```bash
cd tools/KonScript
./build.sh
./install.sh
```

Then rebuild the test game to verify the generated C++ compiles.

---

## 8. CI/CD

### On every push/PR to `main`

`.github/workflows/cmake-single-platform.yml` builds on:
- Ubuntu (native)
- Fedora (container)
- Arch Linux (container)
- Windows (windows-latest runner)

All four must pass before a PR merges.

### On tag push (`v*`)

`.github/workflows/release.yml` builds and uploads all release artifacts.
See section 4 for what gets produced.

### If CI fails

- Fedora/Arch container jobs sometimes fail on dependency names changing — check
  package names first before assuming it's a code bug
- Windows jobs can fail if Qt5 paths change — `jurplel/install-qt-action` pins
  the version so this should be rare
- `zlib1.dll` must be copied manually in the KonPaktor Windows job — `windeployqt`
  does not handle vcpkg deps

---

## 9. Known Constraints

- **No networking** — out of scope due to security complexity
- **No 3D** — planned for far future, not a current priority
- **Qt5 only** — Qt6 would require significant changes to KonAnimator/KonPaktor
- **MXE cross-compile** — Windows builds from Linux require MXE at a fixed path.
  The `GLFW_PLATFORM_X11` hint must NOT be set for Windows builds (`#if !defined(_WIN32)`)
- **KonScript `this.add()`** — creates children via `Node::AddChild()` which now
  calls `Ready()`. Any children added this way are NOT auto-registered with
  CollisionWorld — call `scene.Scan()` after dynamic additions
- **Collider origin** — `Collider2D` defaults to `originX/Y = 0.5` (center pivot).
  World position is computed by walking the parent chain. Scale sign flip
  (e.g. `scaleX = -2` for facing left) is absorbed by `fabs()` in size
  calculations so the hitbox doesn't move when the sprite flips
