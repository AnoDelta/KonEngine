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

### v0.6.0 — Node & Scene System
- Base Node class
- Node2D, Sprite2D nodes with pivot/origin support
- Scene tree (Godot-style hierarchy)
- Signals
- Auto update/draw
- Collider2D integrated into scene tree

### v0.7.0 — Animator
- Sprite sheet animation (frame by frame)
- Keyframe animation for nodes (position, rotation, scale, alpha)
- Animation player node
- `.anim` text format + `anim_compiler` tool → `.konani` binary
- `anim_compiler` Qt GUI tool (cross-platform)

### v0.8.0 — KonAnimator
- Standalone Qt-based animation editor
- Visual spritesheet frame editor (click+drag to define frames)
- Live OpenGL preview with zoom, pan, fullscreen
- Keyframe track editor with timeline
- Direct `.anim` save/load and one-click `.konani` compile
- Cross-platform (Linux + Windows, including cross-compile from Linux)

---

## Upcoming

### v0.9.0 — Asset Pipeline
- AES-256 asset encryption
- `.pak` file bundler (pack + encrypt all assets into one file)
- Asset manager (load from `.pak` or loose files)

### v0.10.0 — Editor MVP
- Viewport panel
- Hierarchy + properties panels
- Scene open/save
- Asset browser

### v0.11.0 — Editor Scripting
- Built-in code editor
- Project management (new/open project)
- In-editor compile + run games

### v1.0.0 — Stable Release
- Polish
- Full documentation
- Ready for serious use

---

## Far Future
- 3D rendering
- Networking

## No Guarantees
This is a personal project built for personal use and for friends.
Features get added when I need them. Nothing here is a promise.
