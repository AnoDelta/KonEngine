#!/bin/bash
# Install KonAnimator and anim_compiler to /usr/local/bin
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD="$SCRIPT_DIR/build"

if [ ! -f "$BUILD/KonAnimator" ]; then
    echo "Error: KonAnimator not built. Run ./build-tools.sh first."
    exit 1
fi
if [ ! -f "$BUILD/anim_compiler" ]; then
    echo "Error: anim_compiler not built. Run ./build-tools.sh first."
    exit 1
fi

echo "Installing KonAnimator and anim_compiler to /usr/local/bin..."
sudo install -m 755 "$BUILD/KonAnimator"    /usr/local/bin/KonAnimator
sudo install -m 755 "$BUILD/anim_compiler"  /usr/local/bin/anim_compiler

echo ""
echo "Done!"
echo "  KonAnimator  -> /usr/local/bin/KonAnimator"
echo "  anim_compiler -> /usr/local/bin/anim_compiler"
