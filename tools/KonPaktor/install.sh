#!/bin/bash
# Install KonPaktor and konpak as desktop applications
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD="$SCRIPT_DIR/build"

if [ ! -f "$BUILD/KonPaktor" ]; then
    echo "Error: KonPaktor not built. Run ./build.sh first."
    exit 1
fi
if [ ! -f "$BUILD/konpak" ]; then
    echo "Error: konpak not built. Run ./build.sh first."
    exit 1
fi

echo "Installing KonPaktor and konpak..."
sudo install -m 755 "$BUILD/KonPaktor" /usr/local/bin/KonPaktor
sudo install -m 755 "$BUILD/konpak"    /usr/local/bin/konpak

# Desktop entry
if [ -d /usr/share/applications ]; then
    sudo install -m 644 "$SCRIPT_DIR/konpaktor.desktop" /usr/share/applications/konpaktor.desktop
fi

# Icon
if [ -d /usr/share/icons/hicolor/scalable/apps ]; then
    sudo install -m 644 "$SCRIPT_DIR/konpaktor.svg" /usr/share/icons/hicolor/scalable/apps/konpaktor.svg
elif [ -d /usr/share/pixmaps ]; then
    sudo install -m 644 "$SCRIPT_DIR/konpaktor.svg" /usr/share/pixmaps/konpaktor.svg
fi

command -v gtk-update-icon-cache >/dev/null 2>&1 && \
    sudo gtk-update-icon-cache -f /usr/share/icons/hicolor/ 2>/dev/null || true
command -v update-desktop-database >/dev/null 2>&1 && \
    sudo update-desktop-database /usr/share/applications/ 2>/dev/null || true

echo ""
echo "Done!"
echo "  KonPaktor -> /usr/local/bin/KonPaktor"
echo "  konpak    -> /usr/local/bin/konpak"
echo "  Desktop   -> /usr/share/applications/konpaktor.desktop"
