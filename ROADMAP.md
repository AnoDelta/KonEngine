# Roadmap

KonEngine is still in early stages. Here's the full plan for where it's headed.

> **Release policy:** Version numbers are only bumped after thorough bug testing.
> Every feature branch goes through a full test pass before merging to main.
> Patch releases (x.x.N) fix bugs. Minor releases (x.N.0) add features.
> Nothing ships broken on purpose.

---

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

### v0.6.0 — Node & Scene System
- Base Node class with parent pointers and signals
- Node2D, Sprite2D nodes with pivot/origin support
- Scene tree (Godot-style hierarchy)
- Auto update/draw
- Collider2D integrated into scene tree
- CollisionWorld with SAT, enter/exit signals, layer/mask filtering

### v0.7.0 — Animator
- Sprite sheet animation (frame by frame)
- Keyframe animation for nodes (position, rotation, scale, alpha)
- AnimationPlayer node (auto-detects parent Sprite2D)
- 16 easing curves
- `.anim` text format + `anim_compiler` tool → `.konani` binary
- `anim_compiler` Qt GUI tool (cross-platform)

### v0.8.0 — KonAnimator & Polish
- Standalone Qt-based animation editor (KonAnimator)
- Visual spritesheet frame editor (click+drag to define frames)
- Live OpenGL preview with zoom, pan, fullscreen
- Keyframe track editor with timeline
- Direct `.anim` save/load and one-click `.konani` compile
- `DebugMode(true)` — FPS, mouse crosshair, auto collider outlines
- Test suite with headless + visual tests (`./build-test.sh`)
- Cross-platform builds (Linux + Windows)
- GitHub Actions CI/CD with automatic release packaging

### v0.8.1 — KonScript
- Statically-typed scripting language that compiles to C++
- Full lexer → parser → typechecker → codegen pipeline
- Node lifecycle bindings (`Ready`, `Update`, `Draw`, `OnCollisionEnter`, `OnCollisionExit`)
- Type system: `I8`–`I64`, `U8`–`U64`, `F32`, `F64`, `Bool`, `str`, `Vec2`
- Arrays, tuples, enums with payloads, structs
- `ksc` frontend runner — compile + build + run in one command
- Neovim syntax highlighting

### v0.8.2 — Bug Fix & Collision Overhaul
- Fixed `OnCollisionEnter`/`OnCollisionExit` not firing on parent nodes
- Fixed child collider world positions (parent transform chain correctly applied)
- Fixed `Node2D::UpdateChildren` to bake world transforms before collision
- Fixed collision running after updates (correct ordering in Scene)
- `Node::Ready()` and `Node::OnCollisionEnter/Exit` are now proper virtuals
- `AddChild` now calls `Ready()` on the new child automatically
- Debug grid in debug mode (endless, zoom-adaptive, lag-free)
- `GetWorldMouseX/Y(Camera2D)` for correct world-space mouse position
- KonScript codegen: non-trivial field inits moved to constructor body
- KonScript codegen: node pointer fields tracked for correct `->` emission
- KonAnimator: timeline click-to-seek working, scrubbing pauses playback
- KonAnimator: frame move/resize handles on spritesheet view
- KonAnimator: dockable panels, layout presets
- `GetFPS()` and `GetTime()` added to engine
- Windows build fix: X11 hint no longer forced in Win32 builds
- `zlib1.dll` now bundled in Windows KonPaktor release

---

## Upcoming

### v0.9.0 — Asset Pipeline & KonPaktor Integration
**Goal:** Assets are encrypted at build time. The running game can load them.
End users and modders cannot extract or inspect them. The key is baked into the
binary at compile time and never exposed at runtime.

- KonPaktor fully integrated into the engine's asset loading path
- `AssetManager` — transparent load from `.konpak` or loose files
- AES-256-CBC encryption, PBKDF2-SHA256 key derivation
- Compile-time key baking via `KON_PACK_KEY` CMake define
- Release workflow: loose files during dev, packed + encrypted in release builds
- Key is embedded in the binary at link time — not readable from the exe
- `.konpak` index is also encrypted so file names are hidden
- CMake helper: `konpak_assets()` to auto-pack on Release build

### v0.9.1 — KonScript Text Editor (Standalone)
Since neither Vim nor VSCode can do full KonScript highlighting and
language features, KonScript ships its own editor.

- Standalone Qt-based code editor for `.ks` files
- Syntax highlighting (keywords, types, engine builtins, lifecycle methods)
- Basic error underlining from the typechecker
- File open/save, recent files
- One-click compile + run via `ksc`
- Ships as a separate tool, usable without KonEngine for standalone KonScript use

### v0.10.0 — Editor MVP
- Viewport panel
- Hierarchy + properties panels
- Scene open/save
- Asset browser (reads from `.konpak` or loose files)
- Integrated KonScript editor tab
- WebAssembly export (play in browser without installing)

### v0.11.0 — Editor Scripting & VM Support
- In-editor compile + run (no external terminal needed)
- Project management (new/open project)
- VM/sandbox support for running games in isolation — no cross-compile needed,
  the VM handles platform differences
- Possible standalone player (ship a `.konpak` + a small runtime exe)

### v0.12.0 — KonScript Standalone
KonScript splits into its own project. It can be used completely independently
of KonEngine for scripting, automation, and daily tasks.

- KonScript language runtime usable standalone (no engine dependency)
- Standard library: file I/O, strings, math, basic networking (read-only, no server)
- Package manager for KonScript libraries
- KonScript editor ships standalone, not just as part of KonEngine

### v0.13.0 — KonPaktor Standalone
KonPaktor becomes its own distributable tool, not tied to KonEngine.

- KonPaktor usable for any encrypted asset archive, not just game assets
- Plugin system for custom file type previews
- Batch operations, scripted packing via konpak CLI
- Ships with its own installer separate from KonEngine

### v1.0.0 — Stable Release
- Full documentation (engine, KonScript, KonAnimator, KonPaktor, editor)
- Maintainer documentation
- Every public API is stable and versioned
- Full test coverage on all platforms
- Website with getting started guide, API reference, and tutorials
- Ready for serious use in shipping games

---

## Far Future
- 3D rendering
- Website support (WebAssembly target via Emscripten)

## No Networking
Networking introduces a large security attack surface that is out of scope for
this engine at this time. It may be revisited much later with proper security
review.

## No Guarantees
This is a personal project built for personal use and for friends.
Features get added when I need them. Nothing here is a promise.
