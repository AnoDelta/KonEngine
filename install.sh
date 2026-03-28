#!/bin/bash
# Install KonEngine library and optionally tools
# Usage:
#   ./install.sh           -- engine library only
#   ./install.sh --tools   -- engine + KonAnimator + anim_compiler + KonPaktor + konpak

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
mkdir -p build
cmake -B build \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX="$INSTALL_PREFIX" \
    -DKON_BUILD_TOOLS="$INSTALL_TOOLS"
cmake --build build -j$(nproc)
sudo cmake --install build

echo ""
echo "KonEngine installed to $INSTALL_PREFIX"

# ---- Install tools ----
if [ "$INSTALL_TOOLS" = "ON" ]; then
    echo ""
    echo "Installing tools..."

    sudo cp build/anim_compiler "$INSTALL_PREFIX/bin/anim_compiler"
    sudo cp build/tools/KonAnimator/KonAnimator "$INSTALL_PREFIX/bin/KonAnimator"

    # KonPaktor
    cd tools/KonPaktor
    mkdir -p build
    cmake -B build -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX="$INSTALL_PREFIX"
    cmake --build build --target KonPaktor --target konpak
    sudo cmake --install build
    cd "$SCRIPT_DIR"

    echo ""
    echo "Tools installed:"
    echo "  $INSTALL_PREFIX/bin/KonAnimator"
    echo "  $INSTALL_PREFIX/bin/anim_compiler"
    echo "  $INSTALL_PREFIX/bin/KonPaktor"
    echo "  $INSTALL_PREFIX/bin/konpak"
fi

echo ""
echo "Done!"
