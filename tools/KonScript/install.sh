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

# Install whatever build.sh just produced ('konscript') preferentially.
# konscript-stage0 is only a fallback if konscript doesn't exist yet.
# Never install konscript-stage1 — it's experimental until --verify passes.
if [ -f "konscript" ]; then
    INSTALL_BIN="konscript"
elif [ -f "konscript-stage0" ]; then
    INSTALL_BIN="konscript-stage0"
else
    echo "konscript binary not found. Building first..."
    ./build.sh
    INSTALL_BIN="konscript"
fi

echo "Installing to $BIN_DIR..."
echo "  Binary: $INSTALL_BIN → konscript"

sudo install -m 755 "$INSTALL_BIN" "$BIN_DIR/konscript"
sudo install -m 755 ksc "$BIN_DIR/ksc"
# Install the runtime C source so konscript can compile it from any directory
if [ -f "_ks_runtime.c" ]; then
    sudo install -m 644 _ks_runtime.c "$BIN_DIR/_ks_runtime.c"
fi

echo ""
echo "==================================================="
echo " Installed!"
echo "   $BIN_DIR/konscript  (backend compiler)"
echo "   $BIN_DIR/ksc        (frontend runner)"
echo ""
echo " Usage:"
echo "   konscript hello.ks          -- build native binary"
echo "   konscript --cpp hello.ks    -- transpile to C++"
echo "   konscript --llvm hello.ks   -- emit LLVM IR"
echo "   ksc hello.ks                -- compile and run"
echo "==================================================="
