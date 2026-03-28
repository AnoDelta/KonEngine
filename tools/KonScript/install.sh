#!/bin/bash
# Install KonScript -- installs konscript backend and ksc frontend
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

# Build first if binary doesn't exist
if [ ! -f "konscript" ]; then
    echo "Building konscript first..."
    ./build.sh
fi

echo "Installing to $BIN_DIR..."

# Install backend
sudo install -m 755 konscript "$BIN_DIR/konscript"

# Install frontend
sudo install -m 755 ksc "$BIN_DIR/ksc"

echo ""
echo "==================================================="
echo " Installed!"
echo "   $BIN_DIR/konscript  (backend compiler)"
echo "   $BIN_DIR/ksc        (frontend runner)"
echo ""
echo " Usage:"
echo "   ksc hello.ks            -- compile and run"
echo "   ksc hello.ks --keep     -- keep generated files"
echo "   ksc --compile hello.ks  -- compile only"
echo "   ksc --check hello.ks    -- typecheck only"
echo "==================================================="
