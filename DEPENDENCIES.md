# Dependencies

## Linux
- `GL` — OpenGL
- `dl` — dynamic linking
- `pthread` — threading
- `X11`, `Xrandr`, `Xi` — Linux windowing system

## Bundled as submodules (no install needed)
- `GLFW` — window creation and input (`libs/glfw`)
- `GLM` — math library (`libs/glm`)
- `GLAD` — OpenGL function loader (`src/glad`)
- `stb_image` — texture loading (`src/stb`)
- `miniaudio` — audio playback (`src/miniaudio`)

## Tools (optional — only needed to build KonAnimator and anim_compiler)
- `Qt5` — GUI framework for KonAnimator and anim_compiler
  - Linux: install via your package manager (see README for distro-specific commands)
  - Windows: download from https://www.qt.io/download-open-source

## Windows
- `opengl32` — comes with Windows
- `gdi32` — comes with Windows
- `winmm` — comes with Windows
- All other dependencies are bundled — no manual installs needed
