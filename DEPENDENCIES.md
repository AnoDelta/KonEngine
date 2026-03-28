# Dependencies

## Bundled (no install needed)
| Library | Purpose | Location |
|---|---|---|
| GLFW 3.4 | Window creation, input | `libs/glfw` |
| GLM 1.0 | Math (vectors, matrices) | `libs/glm` |
| GLAD | OpenGL function loader | `src/glad` |
| stb_image | Texture loading (PNG, JPG, BMP, TGA) | `src/stb` |
| miniaudio | Audio playback and streaming | `src/miniaudio` |

## System (must install)

### Linux
| Package | Ubuntu/Debian | Fedora | Arch | Gentoo |
|---|---|---|---|---|
| OpenGL | `libgl1-mesa-dev` | `mesa-libGL-devel` | `mesa` | `media-libs/mesa` |
| X11 | `libx11-dev` | `libX11-devel` | `libx11` | `x11-libs/libX11` |
| Xrandr | `libxrandr-dev` | `libXrandr-devel` | `libxrandr` | `x11-libs/libXrandr` |
| Xi | `libxi-dev` | `libXi-devel` | `libxi` | `x11-libs/libXi` |
| Wayland | `libwayland-dev` | `wayland-devel` | `wayland` | `dev-libs/wayland` |
| Wayland protocols | `wayland-protocols` | `wayland-protocols-devel` | `wayland-protocols` | `dev-libs/wayland-protocols` |
| xkbcommon | `libxkbcommon-dev` | `libxkbcommon-devel` | `libxkbcommon` | `x11-libs/libxkbcommon` |
| Xinerama | `libxinerama-dev` | `libXinerama-devel` | `libxinerama` | `x11-libs/libXinerama` |
| Xcursor | `libxcursor-dev` | `libXcursor-devel` | `libxcursor` | `x11-libs/libXcursor` |

### Windows
No extra dependencies — everything is bundled or comes with Windows (`opengl32`, `gdi32`, `winmm`).

## Optional (tools only)

### Qt6 (for KonAnimator and anim_compiler)
| Distro | Command |
|---|---|
| Ubuntu/Debian | `sudo apt-get install -y qt6-base-dev` |
| Fedora | `sudo dnf install -y qt6-qtbase-devel` |
| Arch | `sudo pacman -S qt6-base` |
| Windows | https://www.qt.io/download-open-source |

> Qt5 also works if Qt6 is unavailable. Replace `qt6` with `qt5` / `qt5-base` / `qtbase5-dev` as appropriate for your distro.

### KonScript compiler (for .ks scripting)
No extra system dependencies — builds with just CMake and a C++17 compiler.
```bash
cd tools/KonScript && ./build.sh && ./install.sh
```
