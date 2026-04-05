#!/bin/bash
# Install KonEditor as a desktop application
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD="$SCRIPT_DIR/build"

if [ ! -f "$BUILD/KonEditor" ]; then
    echo "Error: KonEditor not built. Run ./build.sh first."
    exit 1
fi

echo "Installing KonEditor..."
sudo install -m 755 "$BUILD/KonEditor" /usr/local/bin/KonEditor

# Desktop entry
if [ -d /usr/share/applications ]; then
    sudo install -m 644 "$SCRIPT_DIR/koneditor.desktop" /usr/share/applications/koneditor.desktop
fi

# Icon
if [ -d /usr/share/icons/hicolor/scalable/apps ]; then
    sudo install -m 644 "$SCRIPT_DIR/koneditor.svg" /usr/share/icons/hicolor/scalable/apps/koneditor.svg
elif [ -d /usr/share/pixmaps ]; then
    sudo install -m 644 "$SCRIPT_DIR/koneditor.svg" /usr/share/pixmaps/koneditor.svg
fi

# Update icon cache if available
command -v gtk-update-icon-cache >/dev/null 2>&1 && \
    sudo gtk-update-icon-cache -f /usr/share/icons/hicolor/ 2>/dev/null || true
command -v update-desktop-database >/dev/null 2>&1 && \
    sudo update-desktop-database /usr/share/applications/ 2>/dev/null || true

echo ""
echo "Done!"
echo "  KonEditor  -> /usr/local/bin/KonEditor"
echo "  Desktop    -> /usr/share/applications/koneditor.desktop"
