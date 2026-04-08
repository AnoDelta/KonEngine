#!/bin/bash
# Install KonEngine library, headers, and optionally tools
#
# Usage:
#   ./install.sh                      -- engine library + headers
#   ./install.sh --tools              -- engine + all tools
#   ./install.sh --prefix=/opt/kon    -- custom install prefix
#
# After install:
#   /usr/local/lib/libKonEngine.a     -- static library
#   /usr/local/include/KonEngine.hpp  -- main header
#   /usr/local/include/color/         -- color system
#   /usr/local/include/window/        -- windowing
#   /usr/local/include/renderer/      -- rendering (OpenGL)
#   /usr/local/include/node/          -- scene graph
#   /usr/local/include/...            -- all subsystem headers
#
# Build a game:
#   g++ -std=c++17 game.cpp -I/usr/local/include \
#       -L/usr/local/lib -lKonEngine -lglfw -lGL -ldl -lpthread -lX11 -lXrandr -lXi -lm
#
# Or use KonScript:
#   konscript game.ks -o game

set -e
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

INSTALL_TOOLS=OFF
INSTALL_PREFIX="/usr/local"
for arg in "$@"; do
    if [ "$arg" = "--tools" ]; then INSTALL_TOOLS=ON; fi
    if [[ "$arg" == --prefix=* ]]; then INSTALL_PREFIX="${arg#--prefix=}"; fi
done

# ---- Build + install engine ----
echo "Building KonEngine..."
cmake -B build \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="$INSTALL_PREFIX" \
    -DKON_BUILD_TOOLS="$INSTALL_TOOLS"
cmake --build build -j$(nproc)
sudo cmake --install build

echo ""
echo "KonEngine installed to $INSTALL_PREFIX"
echo "  Library: $INSTALL_PREFIX/lib/libKonEngine.a"
echo "  Headers: $INSTALL_PREFIX/include/KonEngine.hpp"

# ---- Install tools ----
if [ "$INSTALL_TOOLS" = "ON" ]; then
    echo ""
    echo "Installing tools..."

    if [ -f build/anim_compiler ]; then
        sudo install -m 755 build/anim_compiler "$INSTALL_PREFIX/bin/anim_compiler"
    fi
    if [ -f build/tools/KonAnimator/KonAnimator ]; then
        sudo install -m 755 build/tools/KonAnimator/KonAnimator "$INSTALL_PREFIX/bin/KonAnimator"
    fi

    # KonPaktor
    if [ -d tools/KonPaktor ]; then
        cd tools/KonPaktor
        cmake -B build -DCMAKE_BUILD_TYPE=Release \
            -DCMAKE_INSTALL_PREFIX="$INSTALL_PREFIX"
        cmake --build build --target KonPaktor --target konpak 2>/dev/null || true
        sudo cmake --install build 2>/dev/null || true
        cd "$SCRIPT_DIR"
    fi

    echo ""
    echo "Tools installed to $INSTALL_PREFIX/bin/"
fi

echo ""
echo "Done!"
