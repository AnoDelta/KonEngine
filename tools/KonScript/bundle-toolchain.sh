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
WIN_SYSROOT="${PREFIX}/sysroot/windows64"
MUSL_SRC="${PREFIX}/_build/musl-${MUSL_VERSION}"
MUSL_INSTALL="${PREFIX}/_build/musl-install"
MUSL_TARBALL="musl-${MUSL_VERSION}.tar.gz"
MUSL_URL="https://musl.libc.org/releases/${MUSL_TARBALL}"

# ── Colours ───────────────────────────────────────────────────────────────────
RED='\033[0;31m'; GREEN='\033[0;32m'; YELLOW='\033[1;33m'; NC='\033[0m'
ok()   { echo -e "${GREEN}[ok]${NC}  $*"; }
info() { echo -e "${YELLOW}[..]${NC}  $*"; }
warn() { echo -e "${RED}[warn]${NC} $*"; }
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

    # IMPORTANT: musl's Makefile uses % pattern rules which break if the build
    # path contains spaces. Build in /tmp which is guaranteed space-free, then
    # copy the output back into the sysroot.
    MUSL_TMPBUILD="/tmp/koneditor-musl-build"
    MUSL_TMPINSTALL="/tmp/koneditor-musl-install"
    rm -rf "$MUSL_TMPBUILD" "$MUSL_TMPINSTALL"
    mkdir -p "$MUSL_TMPBUILD" "$MUSL_TMPINSTALL"

    info "Copying musl source to /tmp (avoids spaces-in-path issue)..."
    cp -r "${MUSL_SRC}/." "$MUSL_TMPBUILD/"
    cd "$MUSL_TMPBUILD"

    # Clean any previous attempt
    make clean > /dev/null 2>&1 || true
    rm -f config.mak

    CC="${MUSL_CC}" AR=ar RANLIB=ranlib ./configure \
        --prefix="$MUSL_TMPINSTALL" \
        --target=x86_64 \
        --disable-shared \
        --enable-static \
        --syslibdir="$MUSL_TMPINSTALL/lib" \
        CFLAGS="-O2" \
        > "${PREFIX}/_build/musl-configure.log" 2>&1
    ok "musl configured"

    info "Building musl (this takes ~30 seconds)..."
    make -j"$(nproc)" > "${PREFIX}/_build/musl-build.log" 2>&1
    ok "musl built"

    info "Installing musl..."
    make install > "${PREFIX}/_build/musl-install.log" 2>&1
    ok "musl installed"

    # Copy the pieces KonBuild needs into the sysroot
    info "Copying musl files into sysroot..."
    for f in crt1.o Scrt1.o crti.o crtn.o libc.a; do
        [ -f "$MUSL_TMPINSTALL/lib/$f" ] && cp -f "$MUSL_TMPINSTALL/lib/$f" "${SYSROOT}/lib/"
    done

    # libm — musl includes math in libc.a; create a copy named libm.a
    if [ -f "$MUSL_TMPINSTALL/lib/libm.a" ]; then
        cp -f "$MUSL_TMPINSTALL/lib/libm.a" "${SYSROOT}/lib/"
    else
        cp -f "${SYSROOT}/lib/libc.a" "${SYSROOT}/lib/libm.a"
    fi

    ok "musl sysroot ready at ${SYSROOT}/lib/"
fi

# ── Step 5: Windows cross-compile sysroot (MinGW-w64 CRT) ────────────────────
# Downloads a pre-built MinGW-w64 toolchain to get the CRT objects and import
# libs needed to link Windows binaries from Linux via lld-link.
WIN_CRT="${WIN_SYSROOT}/lib/crt2.o"
if [ -f "${WIN_CRT}" ]; then
    ok "Windows sysroot already present — skipping"
else
    info "Setting up Windows cross-compile sysroot (MinGW-w64 CRT)..."
    mkdir -p "${WIN_SYSROOT}/lib"
    mkdir -p "${PREFIX}/_build"

    # Try to grab CRT objects from system mingw-w64 first (much faster)
    MINGW_DIRS=(
        /usr/x86_64-w64-mingw32/usr/lib
        /usr/lib/mingw64-toolchain/lib/gcc/x86_64-w64-mingw32/*/
        /usr/lib/mingw64-toolchain/lib
        /usr/lib/gcc/x86_64-w64-mingw32/*/
        /usr/x86_64-w64-mingw32/sys-root/mingw/lib
    )
    MINGW_FOUND=""
    for d in "${MINGW_DIRS[@]}"; do
        # shellcheck disable=SC2086
        for dd in $d; do
            if [ -f "$dd/crt2.o" ]; then
                MINGW_FOUND="$dd"
                break 2
            fi
        done
    done

    if [ -n "$MINGW_FOUND" ]; then
        info "Found system MinGW-w64 at $MINGW_FOUND"
        for f in crt2.o crtbegin.o crtend.o dllcrt2.o libmingwex.a libmsvcrt.a libkernel32.a libucrt.a libmingw32.a; do
            [ -f "$MINGW_FOUND/$f" ] && cp -f "$MINGW_FOUND/$f" "${WIN_SYSROOT}/lib/" && info "  Copied $f"
        done
        ok "Windows sysroot from system MinGW-w64"
    else
        # Download WinLibs MinGW-w64 (UCRT, POSIX, no installer needed)
        WINLIBS_URL="https://github.com/brechtsanders/winlibs_mingw/releases/download/13.2.0posix-11.0.1-ucrt-r5/winlibs-x86_64-posix-seh-gcc-13.2.0-llvm-17.0.6-mingw-w64ucrt-11.0.1-r5.tar.xz"
        WINLIBS_TAR="${PREFIX}/_build/mingw64.tar.xz"

        info "Downloading MinGW-w64 (this may take a while ~150MB)..."
        if command -v wget &>/dev/null; then
            wget -q --show-progress -O "$WINLIBS_TAR" "$WINLIBS_URL" || { warn "Download failed — Windows cross-compile will use system linker"; WINLIBS_TAR=""; }
        elif command -v curl &>/dev/null; then
            curl -L --progress-bar -o "$WINLIBS_TAR" "$WINLIBS_URL" || { warn "Download failed — Windows cross-compile will use system linker"; WINLIBS_TAR=""; }
        else
            warn "Neither wget nor curl found — skipping Windows sysroot"
            WINLIBS_TAR=""
        fi

        if [ -n "$WINLIBS_TAR" ] && [ -f "$WINLIBS_TAR" ]; then
            info "Extracting MinGW-w64..."
            MINGW_EXTRACT="${PREFIX}/_build/mingw64-extract"
            mkdir -p "$MINGW_EXTRACT"
            tar -xf "$WINLIBS_TAR" -C "$MINGW_EXTRACT" --strip-components=1 2>/dev/null || \
                tar -xf "$WINLIBS_TAR" -C "$MINGW_EXTRACT"

            # Find the lib directory
            MINGW_LIB=$(find "$MINGW_EXTRACT" -name "crt2.o" -exec dirname {} \; 2>/dev/null | head -1)
            if [ -n "$MINGW_LIB" ]; then
                for f in crt2.o crtbegin.o crtend.o dllcrt2.o libmingwex.a libmsvcrt.a libkernel32.a libucrt.a libmingw32.a libgcc.a libgcc_eh.a; do
                    [ -f "$MINGW_LIB/$f" ] && cp -f "$MINGW_LIB/$f" "${WIN_SYSROOT}/lib/" && info "  Copied $f"
                done
                ok "Windows sysroot ready at ${WIN_SYSROOT}/lib/"
            else
                warn "Could not find CRT objects in extracted archive — Windows cross-compile may fail"
            fi
            rm -f "$WINLIBS_TAR"
        fi
    fi
fi
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
