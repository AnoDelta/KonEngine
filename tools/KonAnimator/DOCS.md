# KonAnimator Documentation

> Version 0.8.0

KonAnimator is the visual animation editor for KonEngine. It produces `.konani`
binary files that the engine loads at runtime via `AnimationPlayer`.

---

## Table of Contents

1. [Interface Overview](#1-interface-overview)
2. [Opening and Saving](#2-opening-and-saving)
3. [Working with Clips](#3-working-with-clips)
4. [Spritesheet Frames](#4-spritesheet-frames)
5. [Keyframe Tracks](#5-keyframe-tracks)
6. [Preview Panel](#6-preview-panel)
7. [Timeline](#7-timeline)
8. [Compiling to .konani](#8-compiling-to-konani)
9. [The .anim File Format](#9-the-anim-file-format)
10. [Easing Curves](#10-easing-curves)
11. [Building](#11-building)

---

## 1. Interface Overview

```
+------------------+---------------------------+------------------+
|   Left Panel     |      Center Panel         |   Right Panel    |
|                  |                           |                  |
|  Animations list |   Spritesheet view        |  Preview         |
|  Clip settings   |   (click+drag to define   |  (live playback) |
|  Frame list      |    frames)                |                  |
|  Frame props     |                           |  Keyframe tracks |
|                  |                           |  Track props     |
+------------------+---------------------------+------------------+
|                    Bottom Panel                                  |
|   Timeline  |  Play/Pause/Stop  |  Time display                 |
+------------------------------------------------------------------+
```

---

## 2. Opening and Saving

### New project
**File > New** (`Ctrl+N`) — clears the current project.

### Open
**File > Open** (`Ctrl+O`) — opens a `.anim` text file or a `.konani` binary file.

When opening a `.konani`:
- The binary is decompiled back into clips, frames, and keyframe tracks
- KonAnimator looks for a spritesheet with the same base name automatically
  (e.g. `player.konani` → tries `player.png`, `player.jpg`, etc.)
- The project is marked dirty — save as `.anim` to keep an editable copy

### Save
**File > Save** (`Ctrl+S`) — saves as `.anim` text format.
**File > Save As** (`Ctrl+Shift+S`) — save to a new path.

### Load spritesheet
**File > Load Spritesheet** — load a PNG/JPG/BMP/TGA image to use as the
spritesheet in the center panel and preview.

You can also store the spritesheet path in the `.anim` file with a `spritesheet` line
so it loads automatically next time:
```
spritesheet sprites/player.png
```

### Compile
**File > Compile .konani** (`Ctrl+B`) — saves the `.anim` and compiles it to a
`.konani` binary in the same directory. This is what the engine loads at runtime.

---

## 3. Working with Clips

A project can contain multiple animation clips (e.g. `idle`, `walk`, `jump`, `die`).

### Add a clip
Click **+** in the Animations panel or **Animation > Add Clip**.
Enter a name when prompted.

### Remove a clip
Select it and click **−** or **Animation > Remove Clip**.

### Clip settings

| Setting | Description |
|---|---|
| Name | The clip name — used in `anim->Play("name")` |
| Loop | Whether the animation repeats |
| Display W/H | The sprite display size in pixels |
| Scale | Display scale multiplier |

Display W/H/Scale are written to the `.konani` and read by `AnimationPlayer` to
automatically resize the parent `Sprite2D` when the animation plays.

---

## 4. Spritesheet Frames

Frames define which region of the spritesheet to show for each animation step.

### Adding frames by clicking
With a clip selected, click and drag on the spritesheet in the center panel to
define a frame rectangle. Release to add it to the frame list.

### Adding frames manually
**Animation > Add Frame (Manual)** or the **+ Manual** button.
Fill in Src X, Src Y, Src W, Src H, and duration in the frame properties panel.

### Frame properties

| Property | Description |
|---|---|
| Src X / Src Y | Top-left corner of the frame on the spritesheet (pixels) |
| Src W / Src H | Frame size (pixels) |
| Dur | How long to show this frame (seconds) |

### Removing frames
Select a frame in the list and click **−** or **Animation > Remove Frame**.

### Frame highlighting
The selected frame is highlighted in the spritesheet view with a colored border.

---

## 5. Keyframe Tracks

Keyframe tracks animate node properties over time. They work alongside or
independently of spritesheet frames.

### Adding a track
**Animation > Add Track** — choose a property:

| Property | What it animates |
|---|---|
| `x` | Horizontal position |
| `y` | Vertical position |
| `scaleX` | Horizontal scale |
| `scaleY` | Vertical scale |
| `rotation` | Rotation in degrees |
| `alpha` | Opacity (0.0 = invisible, 1.0 = opaque) |

### Adding keyframes
Move the timeline playhead to the desired time, then **Animation > Add Keyframe**.
The keyframe is added at the playhead position with value 0.

### Keyframe properties

| Property | Description |
|---|---|
| Time | When this keyframe occurs (seconds) |
| Value | The property value at this time |
| Curve | Easing curve from this keyframe to the next |

### Moving keyframes
Drag keyframes left/right in the timeline to change their time.

### Removing keyframes
Select a keyframe and **Animation > Remove Keyframe**.

---

## 6. Preview Panel

The preview panel shows a live rendering of the current clip using OpenGL.

### Controls
- **Drag** — pan the view
- **Scroll** — zoom in/out
- **Double-click** — reset view to fit
- **F** — toggle fullscreen preview

Playback is controlled by the buttons in the bottom panel.

---

## 7. Timeline

The timeline shows all keyframe tracks for the current clip.

- **Click** on the timeline to move the playhead
- **Drag keyframes** to change their time
- **Click a keyframe** to select it and edit its properties in the right panel

The playhead position is shown in the time display as `0.000s`.

---

## 8. Compiling to .konani

**File > Compile .konani** (`Ctrl+B`) compiles the current project to a binary
`.konani` file in the same directory as the `.anim` file.

The `.konani` contains:
- All clips with their name and loop flag
- Display W/H/Scale for each clip
- All frames (srcX, srcY, srcW, srcH, duration)
- All keyframe tracks with times, values, and easing curves

The spritesheet path is **not** stored in the `.konani` — it is loaded separately
by the game via `LoadTexture`.

### Using in the engine
```cpp
auto* player = scene.Add<Sprite2D>("player");
player->SetTexture(LoadTexture("sprites/player.png"));

auto* anim = player->AddChild<AnimationPlayer>("anim");
anim->LoadFromFile("sprites/player.konani");
anim->Play("idle");
```

---

## 9. The .anim File Format

`.anim` is a human-readable text format. Lines starting with `#` are comments.

```
# Optional: path to the spritesheet (loaded automatically on open)
spritesheet sprites/player.png

anim idle loop
    display 32 32 1.0
    frame 0 0 32 32 0.12
    frame 32 0 32 32 0.12
    frame 64 0 32 32 0.12
end

anim jump
    display 32 32 1.0
    frame 96 0 32 32 0.08
    frame 128 0 32 32 0.08
    track scaleY 0.0 1.0 easeout
    track scaleY 0.15 1.4 easeout
    track scaleY 0.35 1.0 easein
end

anim die
    display 32 32 1.0
    frame 160 0 32 32 0.1
    frame 192 0 32 32 0.15
    frame 224 0 32 32 0.3
    track alpha 0.0 1.0 linear
    track alpha 0.4 0.0 linear
end
```

### Syntax reference

| Token | Syntax | Description |
|---|---|---|
| `spritesheet` | `spritesheet <path>` | Path to the spritesheet image |
| `anim` | `anim <name> [loop]` | Start a clip. Add `loop` to repeat. |
| `display` | `display <w> <h> <scale>` | Display size and scale for the sprite |
| `frame` | `frame <x> <y> <w> <h> <dur>` | One sprite sheet frame |
| `track` | `track <prop> <time> <value> [curve]` | One keyframe on a property track |
| `end` | `end` | Close the current clip |

---

## 10. Easing Curves

| Name | Feel |
|---|---|
| `linear` | Constant speed |
| `easein` | Starts slow, ends fast |
| `easeout` | Starts fast, ends slow |
| `easeinout` | Slow-fast-slow |
| `easeincubic` / `easeoutcubic` / `easeinoutcubic` | Stronger versions |
| `easeinelastic` / `easeoutelastic` / `easeinoutelastic` | Springy overshoot |
| `easeinbounce` / `easeoutbounce` / `easeinoutbounce` | Bouncing |
| `easeinback` / `easeoutback` / `easeinoutback` | Slight overshoot then settle |

When in doubt: `easeout` for things sliding into place, `easeinoutback` for UI
popups, `linear` for looping things like a spinning object.

---

## 11. Building

### Linux
```bash
# From KonEngine root
./build-tools.sh

# Or individually
cmake -B build -DKON_BUILD_TOOLS=ON
cmake --build build --target KonAnimator
```

**Dependencies:**
```bash
# Ubuntu/Debian
sudo apt-get install qtbase5-dev libqt5opengl5-dev

# Gentoo
emerge --ask dev-qt/qtbase:5 dev-qt/qtopengl:5
```

### Windows
```bat
build-tools.bat
```
