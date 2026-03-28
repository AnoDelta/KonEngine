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
- `.anim` text format + `anim_compiler` tool → `.konani` binary
- `anim_compiler` Qt GUI tool (cross-platform)

### v0.8.0 -- KonAnimator & Polish
- Standalone Qt-based animation editor (KonAnimator)
- Visual spritesheet frame editor (click+drag to define frames)
- Live OpenGL preview with zoom, pan, fullscreen
- Keyframe track editor with timeline
- Direct `.anim` save/load and one-click `.konani` compile
- `DebugMode(true)` -- FPS, mouse crosshair, auto collider outlines
- Test suite with headless + visual tests (`./build-test.sh`)
- Cross-platform builds (Linux + Windows, cross-compile from Linux via MXE)
- GitHub Actions CI/CD with automatic release packaging

---

## Upcoming

### v0.9.0 -- Asset Pipeline & KonScript

This version has two priorities: locking in the asset security pipeline and
maturing the KonScript language to a point where real game logic can be written
entirely in it.

#### Asset Pipeline (KonPaktor)
- AES-256 asset encryption (implemented via `konpak.hpp` / KonPaktor)
- `.konpak` file bundler — compress + encrypt all assets into one archive
- Asset manager — transparent load from `.konpak` or loose files
- Compile-time key baking (`KON_PACK_KEY` CMake define)
- CMake Release/Debug workflow: loose files in dev, packed on release

#### KonScript Language
- Stabilize the full lexer → parser → typechecker → codegen pipeline
- Complete node lifecycle bindings (`Ready`, `Update`, `Draw`, `OnCollisionEnter`, `OnCollisionExit`)
- Full type system: `I8`–`I64`, `U8`–`U64`, `F32`, `F64`, `Bool`, `str`, `String`, `Vec2`
- Nullable types (`T?`), null coalescing (`??`), safe access (`?.`), force unwrap (`!`)
- Arrays (`[T]`, `[T; N]`) and tuples (`(T, T)`)
- Enums with optional payloads
- Structs as value types
- `pub` visibility modifier for exported symbols
- `spawn` keyword for node instantiation
- `konscript_sources()` CMake helper — compile `.ks` files at build time
- `ksc` frontend runner — compile + build + run in one command
- Debug modes: `--lex`, `--parse`, `--check` flags
- KonScript DOCS written and shipped with the engine

#### Stretch Goals for v0.9.0
- `wait` keyword for simple coroutine-style delays inside nodes
- String interpolation
- Better error messages with source locations and suggestions

---

### v0.10.0 -- Editor MVP
- Viewport panel
- Hierarchy + properties panels
- Scene open/save
- Asset browser

### v0.11.0 -- Editor Scripting
- Built-in code editor with KonScript support
- Project management (new/open project)
- In-editor compile + run games

### v1.0.0 -- Stable Release
- Polish
- Full documentation for engine, KonScript, KonAnimator, and KonPaktor
- Ready for serious use

---

## Far Future
- 3D rendering
- Networking

## No Guarantees
This is a personal project built for personal use and for friends.
Features get added when I need them. Nothing here is a promise.
