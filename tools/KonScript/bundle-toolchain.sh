#!/usr/bin/env bash
# ---------------------------------------------------------------------------
# bundle-toolchain.sh
#
# Sets up the self-contained toolchain that KonEditor uses to build games
# without any external compiler or cmake on the end user's machine.
#
# What it does:
#   1. Downloads and builds musl libc (static, no glibc dependency)
#   2. Copies llc, ld.lld (and optionally lld-link, wasm-ld) from your
#      installed LLVM into toolchain/llvm/bin/
#   3. Writes toolchain/sysroot/linux64/lib/{crt1.o,crti.o,crtn.o,libc.a,libm.a}
#
# After running this, KonEditor can build games on ANY Linux box that has
# the toolchain/ folder — no gcc, no cmake, no system libs needed.
#
# Run from the repo root or from tools/KonScript/:
#   ./bundle-toolchain.sh
#   ./bundle-toolchain.sh --prefix=/opt/KonEditor/toolchain
#   ./bundle-toolchain.sh --musl-version=1.2.5
#
# Dependencies (on the DEV machine only, not on end-user machines):
#   - LLVM/clang (llc, ld.lld)  — already needed to build KonScript
#   - wget or curl
#   - make, tar
#   - A C compiler (gcc/clang) to compile musl itself once
# ---------------------------------------------------------------------------

set -euo pipefail

# ── Defaults ─────────────────────────────────────────────────────────────────
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
MUSL_VERSION="1.2.5"
PREFIX="${SCRIPT_DIR}/toolchain"

# Parse args
for arg in "$@"; do
    case "$arg" in
        --prefix=*)      PREFIX="${arg#--prefix=}" ;;
        --musl-version=*) MUSL_VERSION="${arg#--musl-version=}" ;;
        --help|-h)
            echo "Usage: $0 [--prefix=<dir>] [--musl-version=<ver>]"
            echo "  --prefix=DIR         Where to write the toolchain (default: ./toolchain)"
            echo "  --musl-version=VER   musl version to build (default: 1.2.5)"
            exit 0 ;;
    esac
done

LLVM_DIR="${PREFIX}/llvm"
SYSROOT="${PREFIX}/sysroot/linux64"
MUSL_SRC="${PREFIX}/_build/musl-${MUSL_VERSION}"
MUSL_INSTALL="${PREFIX}/_build/musl-install"
MUSL_TARBALL="musl-${MUSL_VERSION}.tar.gz"
MUSL_URL="https://musl.libc.org/releases/${MUSL_TARBALL}"

# ── Colours ───────────────────────────────────────────────────────────────────
RED='\033[0;31m'; GREEN='\033[0;32m'; YELLOW='\033[1;33m'; NC='\033[0m'
ok()   { echo -e "${GREEN}[ok]${NC}  $*"; }
info() { echo -e "${YELLOW}[..]${NC}  $*"; }
fail() { echo -e "${RED}[!!]${NC}  $*"; exit 1; }

echo ""
echo "======================================================"
echo "  KonEditor Toolchain Builder"
echo "======================================================"
echo "  Prefix     : ${PREFIX}"
echo "  musl       : ${MUSL_VERSION}"
echo "======================================================"
echo ""

# ── Step 1: find LLVM tools ───────────────────────────────────────────────────
info "Looking for LLVM tools..."

find_llvm_tool() {
    local name="$1"
    # Try versioned first, then unversioned
    for candidate in \
        "${name}-17" "${name}-16" "${name}-15" \
        "/usr/lib/llvm-17/bin/${name}" \
        "/usr/lib/llvm-16/bin/${name}" \
        "/usr/lib/llvm-15/bin/${name}" \
        "${name}"
    do
        if command -v "$candidate" &>/dev/null; then
            echo "$(command -v "$candidate")"
            return 0
        fi
    done
    return 1
}

LLC=$(find_llvm_tool "llc")     || fail "llc not found. Install LLVM: sudo apt install llvm"
LLD=$(find_llvm_tool "ld.lld")  || fail "ld.lld not found. Install lld: sudo apt install lld"

ok "llc    → ${LLC}"
ok "ld.lld → ${LLD}"

# Optional cross-linkers
LLD_LINK=$(find_llvm_tool "lld-link" 2>/dev/null) || LLD_LINK=""
WASM_LD=$(find_llvm_tool "wasm-ld"   2>/dev/null) || WASM_LD=""
[ -n "$LLD_LINK" ] && ok "lld-link → ${LLD_LINK}" || info "lld-link not found (Windows cross-compile won't work)"
[ -n "$WASM_LD"  ] && ok "wasm-ld  → ${WASM_LD}"  || info "wasm-ld not found  (WASM target won't work)"

# ── Step 2: create directory layout ──────────────────────────────────────────
info "Creating toolchain directory layout..."
mkdir -p "${LLVM_DIR}/bin"
mkdir -p "${SYSROOT}/lib"
mkdir -p "${PREFIX}/_build"

# Copy LLVM tools into toolchain/llvm/bin/
cp -f "${LLC}"  "${LLVM_DIR}/bin/llc"
cp -f "${LLD}"  "${LLVM_DIR}/bin/ld.lld"
[ -n "$LLD_LINK" ] && cp -f "${LLD_LINK}" "${LLVM_DIR}/bin/lld-link"
[ -n "$WASM_LD"  ] && cp -f "${WASM_LD}"  "${LLVM_DIR}/bin/wasm-ld"

ok "LLVM tools copied to ${LLVM_DIR}/bin/"

# ── Step 3: build musl ────────────────────────────────────────────────────────
if [ -f "${SYSROOT}/lib/libc.a" ]; then
    ok "musl already built, skipping (delete ${SYSROOT}/lib/libc.a to rebuild)"
else
    info "Downloading musl ${MUSL_VERSION}..."

    cd "${PREFIX}/_build"
    if [ ! -f "${MUSL_TARBALL}" ]; then
        if command -v wget &>/dev/null; then
            wget -q --show-progress "${MUSL_URL}" -O "${MUSL_TARBALL}"
        elif command -v curl &>/dev/null; then
            curl -L --progress-bar "${MUSL_URL}" -o "${MUSL_TARBALL}"
        else
            fail "Neither wget nor curl found. Install one to download musl."
        fi
    fi
    ok "Downloaded ${MUSL_TARBALL}"

    info "Extracting musl..."
    tar xf "${MUSL_TARBALL}"
    ok "Extracted to ${MUSL_SRC}"

    info "Configuring musl (target: x86_64-linux-musl)..."
    cd "${MUSL_SRC}"

    # Find a C compiler explicitly — musl configure can't always find one on Gentoo
    find_cc() {
        for candidate in gcc clang cc \
            /usr/lib/llvm/21/bin/clang \
            /usr/lib/llvm/20/bin/clang \
            /usr/lib/llvm/17/bin/clang \
            $(ls /usr/bin/gcc-* 2>/dev/null | sort -V | tail -1) \
            $(ls /usr/bin/x86_64-pc-linux-gnu-gcc-* 2>/dev/null | sort -V | tail -1)
        do
            if command -v "$candidate" &>/dev/null; then
                echo "$candidate"; return 0
            fi
        done
        return 1
    }

    MUSL_CC=$(find_cc) || fail "No C compiler found. Install gcc or clang: sudo emerge sys-devel/gcc"
    ok "Using C compiler: ${MUSL_CC} ($(${MUSL_CC} --version 2>&1 | head -1))"

    CC="${MUSL_CC}" ./configure \
        --prefix="${MUSL_INSTALL}" \
        --target=x86_64 \
        --disable-shared \
        --enable-static \
        --syslibdir="${MUSL_INSTALL}/lib" \
        CFLAGS="-O2" \
        > "${PREFIX}/_build/musl-configure.log" 2>&1
    ok "musl configured"

    info "Building musl (this takes ~30 seconds)..."
    make -j"$(nproc)" > "${PREFIX}/_build/musl-build.log" 2>&1
    ok "musl built"

    info "Installing musl to ${MUSL_INSTALL}..."
    make install > "${PREFIX}/_build/musl-install.log" 2>&1
    ok "musl installed"

    # Copy the pieces KonBuild needs into the sysroot
    info "Copying musl files into sysroot..."
    cp -f "${MUSL_INSTALL}/lib/crt1.o"  "${SYSROOT}/lib/"
    cp -f "${MUSL_INSTALL}/lib/crti.o"  "${SYSROOT}/lib/"
    cp -f "${MUSL_INSTALL}/lib/crtn.o"  "${SYSROOT}/lib/"
    cp -f "${MUSL_INSTALL}/lib/libc.a"  "${SYSROOT}/lib/"

    # libm is bundled inside musl's libc.a — create a symlink so KonBuild finds it
    # (Some musl builds produce a separate libm.a, others don't)
    if [ -f "${MUSL_INSTALL}/lib/libm.a" ]; then
        cp -f "${MUSL_INSTALL}/lib/libm.a" "${SYSROOT}/lib/"
    else
        # Create a thin archive that just re-exports libc.a
        # (musl includes math in libc.a)
        cd "${SYSROOT}/lib"
        echo "/* libm.a — math is bundled in libc.a for musl */" > libm_stub.c
        llvm-ar rc libm.a || cp libc.a libm.a
        cd "${SCRIPT_DIR}"
    fi

    ok "musl sysroot ready at ${SYSROOT}/lib/"
fi

# ── Step 4: write a README for the toolchain dir ─────────────────────────────
cat > "${PREFIX}/README.md" << 'READMEEOF'
# KonEditor Toolchain

This directory is managed by `bundle-toolchain.sh` and contains
everything KonEditor needs to compile and link KonScript games
without any external toolchain on the end user's machine.

## Layout

```
toolchain/
  llvm/
    bin/llc          — LLVM compiler (produces .o from .ll)
    bin/ld.lld       — ELF linker    (Linux)
    bin/lld-link     — COFF linker   (Windows cross-compile, optional)
    bin/wasm-ld      — WASM linker   (optional)
  sysroot/
    linux64/
      lib/crt1.o     — musl C runtime startup
      lib/crti.o
      lib/crtn.o
      lib/libc.a     — musl libc (static)
      lib/libm.a     — musl libm (static)
```

## Rebuilding

From the repo root:
```
./tools/KonScript/bundle-toolchain.sh
```

To install to a custom prefix:
```
./tools/KonScript/bundle-toolchain.sh --prefix=/opt/KonEditor/toolchain
```
READMEEOF

# ── Done ─────────────────────────────────────────────────────────────────────
echo ""
echo "======================================================"
echo -e "  ${GREEN}Toolchain ready!${NC}"
echo "======================================================"
echo ""
echo "  Set in KonEditor:"
echo "    toolchain dir : ${PREFIX}"
echo ""
echo "  Test it manually (run from tools/KonScript/):"
echo "    konscript --llvm hello.ks"
echo "    ${LLVM_DIR}/bin/llc -filetype=obj -relocation-model=pic \\"
echo "        hello.ll -o hello.o --mtriple=x86_64-pc-linux-gnu"
echo "    ${LLVM_DIR}/bin/ld.lld -static-pie \\"
echo "        ${SYSROOT}/lib/crt1.o ${SYSROOT}/lib/crti.o \\"
echo "        hello.o \\"
echo "        ${SYSROOT}/lib/libc.a ${SYSROOT}/lib/libm.a \\"
echo "        ${SYSROOT}/lib/crtn.o \\"
echo "        -o hello"
echo "    ./hello"
echo ""
