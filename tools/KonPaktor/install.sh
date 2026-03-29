#!/bin/bash
# Install KonPaktor and konpak to /usr/local/bin
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD="$SCRIPT_DIR/tools/KonPaktor/build"

if [ ! -f "$BUILD/KonPaktor" ]; then
    echo "Error: KonPaktor not built. Run ./build-tools.sh first."
    exit 1
fi
if [ ! -f "$BUILD/konpak" ]; then
    echo "Error: konpak not built. Run ./build-tools.sh first."
    exit 1
fi

echo "Installing KonPaktor and konpak to /usr/local/bin..."
sudo install -m 755 "$BUILD/KonPaktor" /usr/local/bin/KonPaktor
sudo install -m 755 "$BUILD/konpak"    /usr/local/bin/konpak

echo ""
echo "Done!"
echo "  KonPaktor -> /usr/local/bin/KonPaktor"
echo "  konpak    -> /usr/local/bin/konpak"
