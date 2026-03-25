# Dependencies

## Linux
- `glfw` — window creation and input
- `GL` — OpenGL
- `dl` — dynamic linking
- `pthread` — threading
- `X11`, `Xrandr`, `Xi`, `Xinerama`, `Xcursor` — Linux windowing system
- `wayland`, `wayland-protocols`, `xkbcommon` — Wayland support

## Windows
- `opengl32` — comes with Windows
- `gdi32` — comes with Windows
- `winmm` — comes with Windows

## Bundled (no install needed)
- `GLAD` — OpenGL function loader (in `src/glad/`)
- `GLFW` — bundled as submodule in `libs/glfw/`
- `GLM` — math library, bundled as submodule in `libs/glm/`
