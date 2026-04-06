# Roadmap

KonEngine is still in early stages. Here's the full plan for where it's headed.

## Done

### v0.4.0
- OpenGL 2D renderer (rectangles, circles, lines)
- Texture loading + drawing + sprite sheet support
- Text rendering with custom and default font (Inconsolata)
- Input system (keyboard, mouse, gamepad)
- Color system with presets
- Audio (sound effects + music streaming via miniaudio)
- Delta time + FPS cap
- VSync toggle
- Cross-platform (Linux + Windows)

### v0.5.0
- Camera system (pan, zoom, rotation)
- Collision detection (AABB, circle, circle vs rectangle)

### v0.6.0 -- Node & Scene System
- Base Node class with parent pointers and signals
- Node2D, Sprite2D nodes with pivot/origin support
- Scene tree (Godot-style hierarchy)
- Auto update/draw
- Collider2D integrated into scene tree
- CollisionWorld with SAT, enter/exit signals, layer/mask filtering

### v0.7.0 -- Animator
- Sprite sheet animation (frame by frame)
- Keyframe animation for nodes (position, rotation, scale, alpha)
- AnimationPlayer node (auto-detects parent Sprite2D)
- 16 easing curves
- `.anim` text format + `anim_compiler` tool -> `.konani` binary
- `anim_compiler` Qt GUI tool (cross-platform)

### v0.8.0 -- KonAnimator & Polish
- Standalone Qt-based animation editor (KonAnimator)
- Visual spritesheet frame editor (click+drag to define frames)
- Live OpenGL preview with zoom, pan, fullscreen
- Keyframe track editor with timeline
- Direct `.anim` save/load and one-click `.konani` compile
- `DebugMode(true)` -- FPS overlay, mouse crosshair, auto collider visualization
- Test suite with headless + visual tests (`./build-test.sh`)
- Cross-platform builds (Linux + Windows, cross-compile from Linux via MXE)
- GitHub Actions CI/CD with automatic release packaging

### v0.9.0 -- Asset Pipeline + Scripting
- AES-256 asset encryption
- `.konpak` file bundler (pack + encrypt all assets into one file)
- Asset manager pattern (`konpak.hpp` + `UnpackAssets()`/`Asset()`)
- **KonPaktor** -- Qt GUI asset manager (browse, add, remove, preview pack contents)
- **konpak** -- CLI tool (`create`, `add`, `remove`, `list`, `extract`)
- Compile-time key baking (`KON_PACK_KEY`) for release builds
- **KonScript v0.1.0** -- custom scripting language that compiles to C++
  - Lexer, parser, type checker, C++ code generator
  - Types: I8/I16/I32/I64, U8-U64, F32/F64, Bool, str, String, Vec2
  - Arrays `[T]`, fixed arrays `[T; N]`, tuples `(T, T)`, nullable `T?`
  - Structs, enums with payloads, node declarations, classes
  - Full operator set, null coalescing `??`, safe member access `?.`, force unwrap `!`
  - Control flow: if/else, while, loop, for-in, for-C, switch/case
  - Functions, return types, multiple return via tuples
  - Engine integration: `node Player : Node2D` maps to KonEngine node types
  - Lifecycle hooks: `Ready`, `Update`, `Draw`, `OnCollisionEnter`, `OnCollisionExit`
  - `konscript` backend compiler + `ksc` frontend runner
  - `--lex`, `--parse`, `--check` debug modes
  - CMake integration (`konscript_sources()` function)
- Renderer performance overhaul (v0.9.0 patch)
  - Uniform location caching (fixed bug: locations were cached before link)
  - `UseProgram()` guard -- skips redundant shader switches
  - Colored quad batcher -- all `DrawRectangle` calls collected into one `glDrawArrays` per frame
  - Projection matrix uploaded once per frame, not per draw call
  - `DrawGlyph` no longer re-uploads projection on every glyph
- VSync off by default, uncapped FPS when no `SetTargetFPS()` is called
- MSVC compatibility fix in test suite (nested brace-init for `std::vector`)

### v0.9.1 -- API Polish + Performance
- `ClearBackground(Color)` overload — use color presets like `ClearBackground(BLACK)`
- Color overload threaded through entire API: IRenderer, OpenGLRenderer, Window, global functions
- KonScript typechecker updated to accept `ClearBackground(Color)` in addition to `(r, g, b)`
- **Physics usability fixes:**
  - `CollisionWorld*` auto-propagated to all nodes via `Scene::Add()`
  - `MoveAndCollide(dx, dy)` no longer requires passing `CollisionWorld&` manually
  - `RigidBody2D` auto-runs physics in `Update()` — no manual `PhysicsUpdate()` call needed
  - `RigidBody2D::onFloor` flag — true when touching a static body below
  - `AddCollider(w, h)` convenience overload — auto-generates collider name
  - `SweepResolve()` — fresh MTV checks against all statics (fixes stale contacts bug)
- **Rendering performance:**
  - Line batching — all `DrawLine` calls batched into one `glDrawArrays(GL_LINES)` per flush
  - Glyph batching — all `DrawGlyph` calls batched per atlas, single draw per font atlas
  - `FlushAll()` ensures correct draw order across batch types
  - Debug grid (hundreds of lines) now renders in 1-2 draw calls instead of hundreds
- **Collision performance:**
  - AABB broad-phase rejection before SAT narrow-phase
  - `SweepResolve` also uses broad-phase for fast kinematic/rigid body resolution
- **Cross-compilation fixes:**
  - `windows-toolchain.cmake` uses `MXE_ROOT` for correct MXE compiler paths
  - `build-windows.sh --pack` flag for KonPak support during cross-compile
  - `build-windows.sh --pack-key=KEY` for compile-time key baking
  - Fixed `konpak.hpp` preprocessor guard for `__MINGW64__` toolchains
  - MXE sysroot added to bcrypt include search paths
  - Fixed `konscript --target windows --pack`: now passes `-DKON_USE_PACK` to CMake and links `bcrypt`
  - `build-engine-lib.sh` Windows path now compiles with `-DKON_USE_PACK`
- Updated C++ example (`examples/cpp_example/`) showcasing:
  - Color presets, `ClearBackground(BLACK)`
  - KinematicBody2D with `MoveAndCollide`, StaticBody2D walls
  - Camera2D follow, zoom, world mouse coordinates
  - Collision enter/exit callbacks, text rendering, debug mode
- Comprehensive DOCS.md rewrite with dual KonScript + C++ examples for every feature

---

## Upcoming

### v0.10.0 -- Editor MVP
- Viewport panel
- Hierarchy + properties panels
- Scene open/save
- Asset browser

### v0.11.0 -- Editor Scripting
- Built-in code editor
- Project management (new/open project)
- In-editor compile + run games

### v1.0.0 -- Stable Release
- Polish
- Full documentation
- Ready for serious use

---

## Far Future
- KonScript LLVM native code backend (`IRGen` alongside existing C++ transpiler)
- LLVM bundled inside editor so end users never need a toolchain
- Pre-built export templates per platform, cross-compile via LLVM target triples
- KonScript self-hosting (compiler written in KonScript)
- 3D rendering
- Networking

## No Guarantees
This is a personal project built for personal use and for friends.
Features get added when I need them. Nothing here is a promise.
