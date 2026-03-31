#!/usr/bin/env bash
# ---------------------------------------------------------------------------
# build-engine-lib.sh
#
# Pre-builds libKonEngine.a for linux64 (and optionally windows64)
# and copies it into the KonScript toolchain so the editor can link
# games without any external build tools.
#
# Run from tools/KonScript/ after bundle-toolchain.sh has been run:
#   ./build-engine-lib.sh
#   ./build-engine-lib.sh --windows  (also cross-compile for windows64)
# ---------------------------------------------------------------------------
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ENGINE_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"   # KonEngine root
PREFIX="${SCRIPT_DIR}/toolchain"
CLANG="${PREFIX}/llvm/bin/clang++"

GREEN='\033[0;32m'; YELLOW='\033[1;33m'; RED='\033[0;31m'; NC='\033[0m'
ok()   { echo -e "  ${GREEN}[ok]${NC}  $*"; }
info() { echo -e "  [..]  $*"; }
warn() { echo -e "  ${YELLOW}[warn]${NC} $*"; }
fail() { echo -e "  ${RED}[FAIL]${NC} $*"; exit 1; }

DO_WINDOWS=0
for arg in "$@"; do
    case "$arg" in --windows) DO_WINDOWS=1 ;; esac
done

echo ""
echo "======================================================"
echo "  KonEngine Library Builder"
echo "======================================================"
echo "  Engine root : ${ENGINE_ROOT}"
echo "  Toolchain   : ${PREFIX}"
echo ""

# ── Check clang++ ──────────────────────────────────────────────────────────
if [ ! -f "${CLANG}" ]; then
    # Try system clang
    CLANG=$(command -v clang++ 2>/dev/null || command -v clang++-18 || command -v clang++-17 || echo "")
    [ -z "$CLANG" ] && fail "clang++ not found. Run bundle-toolchain.sh first or install LLVM."
    warn "Using system clang++: ${CLANG}"
else
    ok "clang++ → ${CLANG}"
fi

AR="${PREFIX}/llvm/bin/llvm-ar"
[ ! -f "$AR" ] && AR=$(command -v llvm-ar 2>/dev/null || command -v ar)
ok "ar → ${AR}"

# ── Collect engine sources ─────────────────────────────────────────────────
ENGINE_SRCS=()
for dir in window renderer/opengl time input font audio collision animation; do
    while IFS= read -r -d '' f; do
        [[ "$f" == *"anim_compiler"* ]] && continue
        [[ "$f" == *"imgui"* ]] && continue
        ENGINE_SRCS+=("$f")
    done < <(find "${ENGINE_ROOT}/src/${dir}" -name "*.cpp" -print0 2>/dev/null || true)
done
# asset_manager and glad
ENGINE_SRCS+=("${ENGINE_ROOT}/src/asset_manager.cpp")
ENGINE_SRCS+=("${ENGINE_ROOT}/src/glad/src/glad.c")

ok "Found ${#ENGINE_SRCS[@]} engine source files"

# ── Linux64 output dir (needed by GLFW build below) ───────────────────────
OUT_DIR="${PREFIX}/engine/linux64"
BUILD_DIR="${PREFIX}/_build/engine_linux64"
mkdir -p "${OUT_DIR}" "${BUILD_DIR}"

# ── Build libglfw3.a ──────────────────────────────────────────────────────
GLFW_DIR="${ENGINE_ROOT}/libs/glfw"
GLFW_OUT="${OUT_DIR}/libglfw3.a"
if [ -f "$GLFW_OUT" ]; then
    ok "libglfw3.a already built, skipping"
else
    info "Building libglfw3.a..."
    GLFW_BUILD="${PREFIX}/_build/glfw_linux64"
    mkdir -p "$GLFW_BUILD"
    GLFW_SRCS=()
    # X11 only — Wayland needs generated protocol headers from wayland-scanner
    for f in context.c init.c input.c monitor.c platform.c vulkan.c window.c \
              egl_context.c osmesa_context.c null_init.c null_monitor.c \
              null_window.c null_joystick.c posix_module.c posix_time.c \
              posix_thread.c posix_poll.c x11_init.c x11_monitor.c x11_window.c \
              xkb_unicode.c glx_context.c linux_joystick.c; do
        [ -f "$GLFW_DIR/src/$f" ] && GLFW_SRCS+=("$GLFW_DIR/src/$f")
    done
    GLFW_OBJS=()
    GLFW_FLAGS=(-I"$GLFW_DIR/include" -I"$GLFW_DIR/src"
                -D_GLFW_X11
                -O2 --target=x86_64-pc-linux-gnu)
    CLANG_C="${CLANG%++}"; [ -x "$CLANG_C" ] || CLANG_C="$CLANG"
    for src in "${GLFW_SRCS[@]}"; do
        base=$(basename "$src" .c).o
        obj="$GLFW_BUILD/$base"
        "$CLANG_C" "${GLFW_FLAGS[@]}" -x c -c "$src" -o "$obj" 2>/dev/null && GLFW_OBJS+=("$obj")
    done
    [ ${#GLFW_OBJS[@]} -gt 0 ] && "${AR}" rcs "$GLFW_OUT" "${GLFW_OBJS[@]}" && \
        ok "libglfw3.a → $GLFW_OUT" || warn "libglfw3.a build incomplete"
fi

# ── Compile flags ──────────────────────────────────────────────────────────
INCLUDES=(
    -I"${ENGINE_ROOT}/src"
    -I"${ENGINE_ROOT}/src/glad/include"
    -I"${ENGINE_ROOT}/src/stb"
    -I"${ENGINE_ROOT}/libs/glfw/include"
    -I"${ENGINE_ROOT}/libs/glm"
)

CXXFLAGS=(-std=c++17 -O2 -fPIC -DNDEBUG "${INCLUDES[@]}")
CFLAGS=(-O2 -fPIC -DNDEBUG "${INCLUDES[@]}")

# ── Linux64 ───────────────────────────────────────────────────────────────
info "Building libKonEngine.a (linux64)..."
OBJS=()
for src in "${ENGINE_SRCS[@]}"; do
    [ -f "$src" ] || continue
    base=$(basename "$src" | sed 's/\.[^.]*$//').o
    obj="${BUILD_DIR}/${base}"
    if [[ "$src" == *.c ]]; then
        CLANG_C="${CLANG%++}"; [ -x "$CLANG_C" ] || CLANG_C="$CLANG"
        "$CLANG_C" "${CFLAGS[@]}" --target=x86_64-pc-linux-gnu -x c -c "$src" -o "$obj"
    else
        "${CLANG}" "${CXXFLAGS[@]}" --target=x86_64-pc-linux-gnu -c "$src" -o "$obj"
    fi
    OBJS+=("$obj")
done

"${AR}" rcs "${OUT_DIR}/libKonEngine.a" "${OBJS[@]}"
ok "libKonEngine.a → ${OUT_DIR}/libKonEngine.a"

# Copy ALL engine headers preserving directory structure
mkdir -p "${OUT_DIR}/include"
cp "${ENGINE_ROOT}/src/KonEngine.hpp" "${OUT_DIR}/include/"
# Copy every subdirectory that has .hpp files
find "${ENGINE_ROOT}/src" -name "*.hpp" | while read -r hpp; do
    rel="${hpp#${ENGINE_ROOT}/src/}"
    dst="${OUT_DIR}/include/${rel}"
    mkdir -p "$(dirname "$dst")"
    cp "$hpp" "$dst"
done
# Also copy glad and stb includes
mkdir -p "${OUT_DIR}/include/glad/include/glad" "${OUT_DIR}/include/KHR"
cp -r "${ENGINE_ROOT}/src/glad/include/glad/"  "${OUT_DIR}/include/glad/include/glad/"  2>/dev/null || true
cp -r "${ENGINE_ROOT}/src/glad/include/KHR/"   "${OUT_DIR}/include/KHR/"                2>/dev/null || true
ok "Headers → ${OUT_DIR}/include/"

# ── Windows64 (optional) ──────────────────────────────────────────────────
if [ "$DO_WINDOWS" = "1" ]; then
    OUT_DIR_WIN="${PREFIX}/engine/windows64"
    BUILD_DIR_WIN="${PREFIX}/_build/engine_windows64"
    mkdir -p "${OUT_DIR_WIN}" "${BUILD_DIR_WIN}"

    info "Building libKonEngine.a (windows64 cross-compile)..."
    WIN_INCLUDES=("${INCLUDES[@]}" -I"${ENGINE_ROOT}/libs/glfw/include")
    OBJS_WIN=()
    for src in "${ENGINE_SRCS[@]}"; do
        [ -f "$src" ] || continue
        # Skip Linux-specific sources
        [[ "$src" == *"/wl_"* ]] && continue
        [[ "$src" == *"/x11_"* ]] && continue
        [[ "$src" == *"/glx_"* ]] && continue
        [[ "$src" == *"/posix_"* ]] && continue
        [[ "$src" == *"/linux_"* ]] && continue
        base=$(basename "$src" | sed 's/\.[^.]*$//').o
        obj="${BUILD_DIR_WIN}/${base}"
        if [[ "$src" == *.c ]]; then
            "${CLANG}" "${CFLAGS[@]}" "${WIN_INCLUDES[@]}" \
                --target=x86_64-pc-windows-gnu -c "$src" -o "$obj" 2>/dev/null || true
        else
            "${CLANG}" "${CXXFLAGS[@]}" "${WIN_INCLUDES[@]}" \
                --target=x86_64-pc-windows-gnu -c "$src" -o "$obj" 2>/dev/null || true
        fi
        [ -f "$obj" ] && OBJS_WIN+=("$obj")
    done

    [ ${#OBJS_WIN[@]} -gt 0 ] && \
        "${AR}" rcs "${OUT_DIR_WIN}/libKonEngine.a" "${OBJS_WIN[@]}" && \
        ok "libKonEngine.a (windows64) → ${OUT_DIR_WIN}/libKonEngine.a" || \
        warn "Windows engine lib build incomplete"
fi

echo ""
echo "======================================================"
echo -e "  ${GREEN}Engine library ready!${NC}"
echo "======================================================"
echo ""
echo "  Now editor can build games with:"
echo "    konscript game.ks  →  ./game  (no cmake, no g++)"
echo ""
