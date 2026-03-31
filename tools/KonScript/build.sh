#!/bin/bash
# Build KonScript — compiles the konscript backend binary
# Bakes the toolchain path into the binary so it works from anywhere
# without needing KONSCRIPT_TOOLCHAIN to be set.
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

# ── Find the toolchain ────────────────────────────────────────────────────
# Check common locations in order
find_toolchain() {
    # 1. Already built alongside us
    if [ -d "$SCRIPT_DIR/toolchain/llvm/bin" ]; then
        echo "$SCRIPT_DIR/toolchain"
        return
    fi
    # 2. KONSCRIPT_TOOLCHAIN env var
    if [ -n "${KONSCRIPT_TOOLCHAIN:-}" ] && [ -d "$KONSCRIPT_TOOLCHAIN/llvm/bin" ]; then
        echo "$KONSCRIPT_TOOLCHAIN"
        return
    fi
    # 3. Not found
    echo ""
}

TOOLCHAIN_DIR=$(find_toolchain)

# ── Build ─────────────────────────────────────────────────────────────────
echo "Building konscript..."

DEFINES=""
if [ -n "$TOOLCHAIN_DIR" ]; then
    # Bake the absolute toolchain path into the binary
    DEFINES="-DKONSCRIPT_TOOLCHAIN_BUILTIN=\"$TOOLCHAIN_DIR\""
    echo "  Toolchain: $TOOLCHAIN_DIR (baked in)"
else
    echo "  Toolchain: not found — binary will require KONSCRIPT_TOOLCHAIN env var"
    echo "  Run ./bundle-toolchain.sh to build it, then rebuild konscript"
fi

# shellcheck disable=SC2086
g++ -std=c++17 -O2 -I include $DEFINES src/main.cpp -o konscript

echo ""
echo "==================================================="
echo " Done!"
echo "   Backend : $(pwd)/konscript"
echo "   Frontend: ksc (install with ./install.sh)"
if [ -n "$TOOLCHAIN_DIR" ]; then
echo ""
echo "   Toolchain baked in — works from any directory"
fi
echo "==================================================="
