#!/bin/bash
# Build KonScript — builds the self-hosted compiler (Stage 4)
#
# Pipeline:
#   1. Build Stage 0 from C++ (src/main.cpp)
#   2. Stage 0 compiles konscript.ks → Stage 1
#   3. Stage 1 compiles konscript.ks → Stage 2 (has CLI args)
#   4. Stage 2 compiles konscript.ks → Stage 3 (self-hosted)
#   5. Stage 3 compiles konscript.ks → Stage 4 (verified identical)
#   6. Install Stage 4 as 'konscript'
#
# The final binary is fully self-hosted and byte-identical to Stage 3.
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

# ── Find the toolchain ────────────────────────────────────────────────────
find_toolchain() {
    if [ -d "$SCRIPT_DIR/toolchain/llvm/bin" ]; then
        echo "$SCRIPT_DIR/toolchain"
        return
    fi
    if [ -n "${KONSCRIPT_TOOLCHAIN:-}" ] && [ -d "$KONSCRIPT_TOOLCHAIN/llvm/bin" ]; then
        echo "$KONSCRIPT_TOOLCHAIN"
        return
    fi
    echo ""
}

TOOLCHAIN_DIR=$(find_toolchain)

echo "==================================================="
echo " Building KonScript (self-hosted)"
echo "==================================================="
echo ""

# ── Stage 0: Build C++ compiler ──────────────────────────────────────────
echo "[1/5] Building Stage 0 (C++)..."
DEFINES=""
if [ -n "$TOOLCHAIN_DIR" ]; then
    DEFINES="-DKONSCRIPT_TOOLCHAIN_BUILTIN=\"$TOOLCHAIN_DIR\""
fi
# shellcheck disable=SC2086
g++ -std=c++20 -O2 -I include $DEFINES src/main.cpp -o konscript-stage0

# ── Stage 1: Stage 0 compiles konscript.ks ───────────────────────────────
echo "[2/5] Building Stage 1 (Stage 0 → konscript.ks)..."
./konscript-stage0 konscript.ks -o konscript-stage1 2>/dev/null

# ── Stage 2: Stage 1 compiles konscript.ks (gets CLI args) ───────────────
echo "[3/5] Building Stage 2 (Stage 1 → konscript.ks)..."
echo "konscript.ks" > .ks_input
./konscript-stage1 2>/dev/null
# Stage 1 outputs to default name 'konscript' (stem of konscript.ks)
mv konscript konscript-stage2

# ── Stage 3: Stage 2 compiles konscript.ks ───────────────────────────────
echo "[4/5] Building Stage 3 (Stage 2 → konscript.ks)..."
./konscript-stage2 konscript.ks -o konscript-stage3 2>/dev/null

# ── Stage 4: Stage 3 compiles konscript.ks (verify identical) ────────────
echo "[5/5] Building Stage 4 (Stage 3 → konscript.ks)..."
./konscript-stage3 konscript.ks -o konscript-stage4 2>/dev/null

# ── Verify ───────────────────────────────────────────────────────────────
MD5_3=$(md5sum konscript-stage3 | cut -d' ' -f1)
MD5_4=$(md5sum konscript-stage4 | cut -d' ' -f1)

if [ "$MD5_3" = "$MD5_4" ]; then
    echo ""
    echo "  ✓ Self-hosting verified: Stage 3 = Stage 4"
    echo "    MD5: $MD5_4"
else
    echo ""
    echo "  ✗ WARNING: Stage 3 ≠ Stage 4"
    echo "    Stage 3: $MD5_3"
    echo "    Stage 4: $MD5_4"
fi

# ── Install Stage 4 as the default binary ────────────────────────────────
cp konscript-stage4 konscript

echo ""
echo "==================================================="
echo " Done!"
echo "   Binary:  $(pwd)/konscript"
echo "   Stage:   4 (self-hosted, verified)"
echo "   Install: sudo ./install.sh"
echo "==================================================="
