#!/bin/bash
# Install KonScript — builds the self-hosted compiler and installs it system-wide
#
# The installed binary is a Stage 4 self-hosted compiler:
#   Stage 0 (C++) → Stage 1 → Stage 2 → Stage 3 → Stage 4 (verified identical)
#
# Usage:
#   ./install.sh              -- install to /usr/local
#   ./install.sh --prefix=/opt/konscript

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

PREFIX="/usr/local"
for arg in "$@"; do
    if [[ "$arg" == --prefix=* ]]; then
        PREFIX="${arg#--prefix=}"
    fi
done

BIN_DIR="$PREFIX/bin"

# Build if 'konscript' binary doesn't exist
if [ ! -f "konscript" ]; then
    echo "No konscript binary found. Building..."
    ./build.sh
fi

echo "Installing to $BIN_DIR..."

sudo install -m 755 konscript "$BIN_DIR/konscript"
sudo install -m 755 ksc "$BIN_DIR/ksc"
# Install the runtime C source so konscript can compile programs from any directory
if [ -f "_ks_runtime.c" ]; then
    sudo install -m 644 _ks_runtime.c "$BIN_DIR/_ks_runtime.c"
fi

echo ""
echo "==================================================="
echo " Installed!"
echo "   $BIN_DIR/konscript  (self-hosted compiler, Stage 4)"
echo "   $BIN_DIR/ksc        (compile-and-run frontend)"
echo "   $BIN_DIR/_ks_runtime.c"
echo ""
echo " Usage:"
echo "   konscript hello.ks                    -- compile to native binary"
echo "   konscript hello.ks -o myprog          -- custom output name"
echo "   konscript --cpp hello.ks -o hello.cpp -- emit C++ only"
echo "   konscript app.ks -lSDL2               -- link C library"
echo "   konscript --help                      -- show all flags"
echo "   ksc hello.ks                          -- compile and run"
echo "==================================================="
