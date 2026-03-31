#!/usr/bin/env bash
# ---------------------------------------------------------------------------
# bootstrap.sh — KonScript self-hosting bootstrap
#
# Stage 0: build konscript from C++ source using g++ (always works)
# Stage 1: rebuild konscript from konscript.ks using the stage-0 binary
# Stage 2: rebuild konscript.ks using the stage-1 binary (proves self-hosting)
#
# Usage:
#   ./bootstrap.sh           # stage 0 only (same as build.sh)
#   ./bootstrap.sh --stage1  # stage 0 + stage 1 (rebuild with itself)
#   ./bootstrap.sh --verify  # stage 0 + 1 + 2 (full self-hosting check)
# ---------------------------------------------------------------------------
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

RED='\033[0;31m'; GREEN='\033[0;32m'; YELLOW='\033[1;33m'; CYAN='\033[0;36m'; NC='\033[0m'
ok()   { echo -e "${GREEN}[ok]${NC}  $*"; }
info() { echo -e "${CYAN}[..]${NC}  $*"; }
warn() { echo -e "${YELLOW}[!!]${NC}  $*"; }
fail() { echo -e "${RED}[!!]${NC}  $*"; exit 1; }

STAGE1=false
VERIFY=false
for arg in "$@"; do
    case "$arg" in
        --stage1) STAGE1=true ;;
        --verify) STAGE1=true; VERIFY=true ;;
        --help|-h)
            echo "Usage: $0 [--stage1] [--verify]"
            echo "  (no flags)  Build konscript from C++ source (stage 0)"
            echo "  --stage1    Also rebuild konscript from konscript.ks using stage-0 binary"
            echo "  --verify    Full self-hosting check: stage 0 → 1 → 2, compare outputs"
            exit 0 ;;
    esac
done

echo ""
echo "======================================================"
echo "  KonScript Bootstrap"
echo "======================================================"
echo ""

# ── Stage 0: C++ compiler ─────────────────────────────────────────────────
info "Stage 0: building konscript from C++ source..."
g++ -std=c++17 -O2 -I include src/main.cpp -o konscript-stage0
ok "konscript-stage0 built"

# Make stage 0 available as 'konscript' for stage 1
cp -f konscript-stage0 konscript

# ── Stage 1: self-hosted ──────────────────────────────────────────────────
if [ "$STAGE1" = true ]; then
    if [ ! -f "src/konscript.ks" ]; then
        fail "src/konscript.ks not found. The self-hosted compiler hasn't been written yet."
    fi

    info "Stage 1: rebuilding konscript from konscript.ks using stage-0 binary..."

    # Find toolchain
    if [ -d "toolchain/llvm/bin" ]; then
        LLC="toolchain/llvm/bin/llc"
        LLD="toolchain/llvm/bin/ld.lld"
        SYSROOT="toolchain/sysroot/linux64/lib"
    elif [ -n "${KONSCRIPT_TOOLCHAIN:-}" ]; then
        LLC="$KONSCRIPT_TOOLCHAIN/llvm/bin/llc"
        LLD="$KONSCRIPT_TOOLCHAIN/llvm/bin/ld.lld"
        SYSROOT="$KONSCRIPT_TOOLCHAIN/sysroot/linux64/lib"
    else
        fail "Toolchain not found. Run ./bundle-toolchain.sh first."
    fi

    # IRGen
    ./konscript-stage0 --llvm src/konscript.ks -o /tmp/konscript-stage1.ll
    ok "IRGen complete → /tmp/konscript-stage1.ll"

    # Compile
    "$LLC" -filetype=obj /tmp/konscript-stage1.ll \
        -o /tmp/konscript-stage1.o \
        --mtriple=x86_64-pc-linux-gnu
    ok "llc complete → /tmp/konscript-stage1.o"

    # Link
    "$LLD" -static \
        "$SYSROOT/crt1.o" \
        "$SYSROOT/crti.o" \
        /tmp/konscript-stage1.o \
        "$SYSROOT/libc.a" \
        "$SYSROOT/libm.a" \
        "$SYSROOT/crtn.o" \
        -o konscript-stage1
    ok "konscript-stage1 built"

    # ── Stage 2: verify self-hosting ──────────────────────────────────────
    if [ "$VERIFY" = true ]; then
        info "Stage 2: rebuilding konscript.ks using stage-1 binary..."

        ./konscript-stage1 --llvm src/konscript.ks -o /tmp/konscript-stage2.ll

        # Compare IR output — if stage 1 and stage 2 produce identical IR,
        # the compiler is correctly self-hosting
        if diff -q /tmp/konscript-stage1.ll /tmp/konscript-stage2.ll > /dev/null 2>&1; then
            ok "Stage 1 and Stage 2 produce identical IR — self-hosting verified!"
            # Only promote to canonical konscript when fully verified
            cp -f konscript-stage1 konscript
            ok "konscript is now the stage-1 self-hosted binary"
        else
            warn "IR output differs between stage 1 and stage 2."
            warn "Stage-1 is not yet fully self-hosting — keeping stage-0 as konscript."
            diff /tmp/konscript-stage1.ll /tmp/konscript-stage2.ll | head -40
            cp -f konscript-stage0 konscript
        fi
    else
        # stage1 built but not verified — keep stage-0 as the working compiler
        # until stage-1 is complete enough to actually compile .ks files correctly
        cp -f konscript-stage0 konscript
        warn "Stage-1 binary built but not promoted (use --verify to promote when ready)"
        warn "konscript is still the stage-0 C++ binary"
    fi
else
    ok "konscript is the stage-0 C++ binary (run with --stage1 to self-host)"
fi

echo ""
echo "======================================================"
echo -e "  ${GREEN}Done!${NC}"
echo "======================================================"
echo ""
echo "  konscript : $(pwd)/konscript"
echo ""
echo "  Install with: ./install.sh"
echo ""
if [ "$STAGE1" = false ] && [ -f "src/konscript.ks" ]; then
    echo "  Self-host: ./bootstrap.sh --stage1"
    echo ""
fi
