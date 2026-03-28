#!/bin/bash
# build-windows.sh  — cross-compile KonAnimator for Windows from Linux
# Produces a standalone KonAnimator.exe (no DLLs needed on target machine)
#
# Requirements (install once):
#   git clone https://github.com/mxe/mxe.git ~/mxe
#   cd ~/mxe && make -j$(nproc) MXE_TARGETS=x86_64-w64-mingw32.static \
#       qt5 qtbase   # ~30-60 min first time

set -e

# ---- Locate MXE ----
MXE_ROOT=""
for candidate in /usr/lib/mxe /opt/mxe "$HOME/mxe" "${MXE_ROOT:-}"; do
    if [ -f "$candidate/usr/bin/x86_64-w64-mingw32.static-gcc" ]; then
        MXE_ROOT="$candidate"
        break
    fi
done

if [ -z "$MXE_ROOT" ]; then
    echo ""
    echo "  ERROR: MXE not found."
    echo ""
    echo "  Install MXE with Qt5 support (one-time, ~30-60 min):"
    echo "    git clone https://github.com/mxe/mxe.git ~/mxe"
    echo "    cd ~/mxe"
    echo "    make -j\$(nproc) MXE_TARGETS=x86_64-w64-mingw32.static qt5"
    echo ""
    echo "  Then re-run this script."
    echo ""
    exit 1
fi

echo "Using MXE at: $MXE_ROOT"

BUILD_DIR="build-windows"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

cmake .. \
    -DCMAKE_TOOLCHAIN_FILE=../windows-cross-toolchain.cmake \
    -DMXE_ROOT="$MXE_ROOT" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=../dist-windows

make -j"$(nproc)"

echo ""
echo "==================================================="
echo " Done!  ->  $BUILD_DIR/KonAnimator.exe"
echo " Single self-contained exe, no DLLs required."
echo "==================================================="
