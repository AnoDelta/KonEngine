#!/bin/bash
# Install KonAnimator and anim_compiler as desktop applications
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

echo "Installing KonAnimator and anim_compiler..."
sudo install -m 755 "$BUILD/KonAnimator"    /usr/local/bin/KonAnimator
sudo install -m 755 "$BUILD/anim_compiler"  /usr/local/bin/anim_compiler

# Desktop entry
if [ -d /usr/share/applications ]; then
    sudo install -m 644 "$SCRIPT_DIR/konanimator.desktop" /usr/share/applications/konanimator.desktop
fi

# Icon
if [ -d /usr/share/icons/hicolor/scalable/apps ]; then
    sudo install -m 644 "$SCRIPT_DIR/konanimator.svg" /usr/share/icons/hicolor/scalable/apps/konanimator.svg
elif [ -d /usr/share/pixmaps ]; then
    sudo install -m 644 "$SCRIPT_DIR/konanimator.svg" /usr/share/pixmaps/konanimator.svg
fi

command -v gtk-update-icon-cache >/dev/null 2>&1 && \
    sudo gtk-update-icon-cache -f /usr/share/icons/hicolor/ 2>/dev/null || true
command -v update-desktop-database >/dev/null 2>&1 && \
    sudo update-desktop-database /usr/share/applications/ 2>/dev/null || true

echo ""
echo "Done!"
echo "  KonAnimator   -> /usr/local/bin/KonAnimator"
echo "  anim_compiler -> /usr/local/bin/anim_compiler"
echo "  Desktop       -> /usr/share/applications/konanimator.desktop"
