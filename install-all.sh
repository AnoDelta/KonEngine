#!/bin/bash
# Install all KonEngine tools
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

echo "========================================"
echo "  KonEngine Tools Installer"
echo "========================================"
echo ""

# Engine library
if [ -f "$SCRIPT_DIR/build/libKonEngine.a" ]; then
    echo "Installing engine library..."
    sudo cmake --install "$SCRIPT_DIR/build" --prefix /usr/local
    echo "  libKonEngine.a -> /usr/local/lib/"
    echo "  KonEngine.hpp  -> /usr/local/include/"
else
    echo "Skipping engine library (not built -- run ./build.sh)"
fi
echo ""

# KonAnimator + anim_compiler
if [ -f "$SCRIPT_DIR/build/KonAnimator" ]; then
    echo "Installing KonAnimator..."
    sudo install -m 755 "$SCRIPT_DIR/build/KonAnimator"   /usr/local/bin/KonAnimator
    sudo install -m 755 "$SCRIPT_DIR/build/anim_compiler" /usr/local/bin/anim_compiler
    echo "  KonAnimator   -> /usr/local/bin/"
    echo "  anim_compiler -> /usr/local/bin/"
else
    echo "Skipping KonAnimator (not built -- run ./build-tools.sh)"
fi
echo ""

# KonPaktor + konpak
if [ -f "$SCRIPT_DIR/tools/KonPaktor/build/KonPaktor" ]; then
    echo "Installing KonPaktor..."
    sudo install -m 755 "$SCRIPT_DIR/tools/KonPaktor/build/KonPaktor" /usr/local/bin/KonPaktor
    sudo install -m 755 "$SCRIPT_DIR/tools/KonPaktor/build/konpak"    /usr/local/bin/konpak
    echo "  KonPaktor -> /usr/local/bin/"
    echo "  konpak    -> /usr/local/bin/"
else
    echo "Skipping KonPaktor (not built -- run ./build-tools.sh)"
fi
echo ""

# KonScript
if [ -f "$SCRIPT_DIR/tools/KonScript/konscript" ]; then
    echo "Installing KonScript..."
    sudo install -m 755 "$SCRIPT_DIR/tools/KonScript/konscript" /usr/local/bin/konscript
    [ -f "$SCRIPT_DIR/tools/KonScript/ksc" ] && \
        sudo install -m 755 "$SCRIPT_DIR/tools/KonScript/ksc" /usr/local/bin/ksc
    echo "  konscript -> /usr/local/bin/"
    echo "  ksc       -> /usr/local/bin/"
else
    echo "Skipping KonScript (not built -- run: cd tools/KonScript && ./build.sh)"
fi

echo ""
echo "========================================"
echo "  Installation complete!"
echo "========================================"
