#!/bin/bash
# build-windows.sh -- cross-compile KonEngine for Windows from Linux using MXE
#
# Usage:
#   ./build-windows.sh              # engine library only
#   ./build-windows.sh --tools      # engine + KonAnimator + anim_compiler
#
# One-time MXE setup (~30-60 min):
#   git clone https://github.com/mxe/mxe.git ~/mxe
#   cd ~/mxe
#   make -j$(nproc) MXE_TARGETS=x86_64-w64-mingw32.static qt5

set -e

BUILD_TOOLS=OFF
for arg in "$@"; do
    if [ "$arg" = "--tools" ]; then
        BUILD_TOOLS=ON
    fi
done

# ---- Locate MXE ----
MXE_ROOT=""
for candidate in /usr/lib/mxe /opt/mxe "$HOME/mxe"; do
    if [ -f "$candidate/usr/bin/x86_64-w64-mingw32.static-gcc" ]; then
        MXE_ROOT="$candidate"
        break
    fi
done

if [ -z "$MXE_ROOT" ]; then
    echo ""
    echo "  ERROR: MXE not found."
    echo ""
    echo "  Install MXE (one-time, ~30-60 min):"
    echo "    git clone https://github.com/mxe/mxe.git ~/mxe"
    echo "    cd ~/mxe"
    if [ "$BUILD_TOOLS" = "ON" ]; then
        echo "    make -j\$(nproc) MXE_TARGETS=x86_64-w64-mingw32.static qt5"
    else
        echo "    make -j\$(nproc) MXE_TARGETS=x86_64-w64-mingw32.static"
    fi
    echo ""
    exit 1
fi

echo "MXE root   : $MXE_ROOT"
echo "Build tools: $BUILD_TOOLS"
echo ""

BUILD_DIR="build-windows"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

cmake .. \
    -DCMAKE_TOOLCHAIN_FILE=../windows-toolchain.cmake \
    -DMXE_ROOT="$MXE_ROOT" \
    -DCMAKE_BUILD_TYPE=Release \
    -DKON_BUILD_TOOLS="$BUILD_TOOLS"

if [ "$BUILD_TOOLS" = "ON" ]; then
    make -j"$(nproc)" KonAnimator anim_compiler
    echo ""
    echo "==================================================="
    echo " Done!"
    echo "   Engine  : $BUILD_DIR/libKonEngine.a"
    echo "   Animator: $BUILD_DIR/tools/KonAnimator/KonAnimator.exe"
    echo "   Compiler: $BUILD_DIR/anim_compiler.exe"
    echo " All exes are standalone -- no DLLs needed."
    echo "==================================================="
else
    make -j"$(nproc)"
    echo ""
    echo "==================================================="
    echo " Done!  ->  $BUILD_DIR/libKonEngine.a"
    echo "==================================================="
fi
