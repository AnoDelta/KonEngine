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
    CLANG=$(command -v clang++ 2>/dev/null || command -v clang++-21 || command -v clang++-20 || command -v clang++-19 || command -v clang++-18 || echo "")
    [ -z "$CLANG" ] && fail "clang++ not found. Run bundle-toolchain.sh first or install LLVM."
    warn "Using system clang++: ${CLANG}"
else
    # Bundled clang++ lacks system headers — always prefer system clang++ for Linux builds
    SYS=$(command -v clang++ 2>/dev/null || command -v clang++-21 || command -v clang++-20 || command -v clang++-19 || command -v clang++-18 || echo "")
    [ -n "$SYS" ] && CLANG="$SYS"
    ok "clang++ → ${CLANG}"
fi

AR="${PREFIX}/llvm/bin/llvm-ar"
[ ! -f "$AR" ] && AR=$(command -v llvm-ar 2>/dev/null || command -v ar)
ok "ar → ${AR}"

# ── Collect engine sources ─────────────────────────────────────────────────
ENGINE_SRCS=()
for dir in window renderer/opengl time input font audio collision animation node ui; do
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

CXXFLAGS=(-std=c++17 -O2 -fPIC -DNDEBUG -DKON_USE_PACK "${INCLUDES[@]}")
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
# Copy GLM
if [ -d "${ENGINE_ROOT}/libs/glm/glm" ]; then
    mkdir -p "${OUT_DIR}/include/glm"
    cp -r "${ENGINE_ROOT}/libs/glm/glm/." "${OUT_DIR}/include/glm/"
fi
# GLFW headers
if [ -d "${ENGINE_ROOT}/libs/glfw/include/GLFW" ]; then
    mkdir -p "${OUT_DIR}/include/GLFW"
    cp -r "${ENGINE_ROOT}/libs/glfw/include/GLFW/." "${OUT_DIR}/include/GLFW/"
fi
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

    # Prefer bundled llvm-mingw over system clang
    LLVM_MINGW_BIN="${PREFIX}/llvm-mingw/bin"
    WIN_CXX="${LLVM_MINGW_BIN}/x86_64-w64-mingw32-clang++"
    WIN_CC="${LLVM_MINGW_BIN}/x86_64-w64-mingw32-clang"
    WIN_AR="${LLVM_MINGW_BIN}/llvm-ar"
    if [ ! -f "$WIN_CXX" ]; then
        # Fall back to system mingw
        WIN_CXX=$(command -v x86_64-w64-mingw32-g++ 2>/dev/null || echo "$CLANG --target=x86_64-pc-windows-gnu")
        WIN_CC=$(command -v x86_64-w64-mingw32-gcc 2>/dev/null || echo "$CLANG_C --target=x86_64-pc-windows-gnu")
        WIN_AR="${AR}"
    fi

    WIN_INCLUDES=("${INCLUDES[@]}" -I"${ENGINE_ROOT}/libs/glfw/include")
    OBJS_WIN=()
    for src in "${ENGINE_SRCS[@]}"; do
        [ -f "$src" ] || continue
        [[ "$src" == *"anim_compiler"* ]] && continue
        base=$(basename "$src" | sed 's/\.[^.]*$//').o
        obj="${BUILD_DIR_WIN}/${base}"
        if [[ "$src" == *.c ]]; then
            "$WIN_CC" "${WIN_INCLUDES[@]}" -O2 -DNDEBUG -x c -c "$src" -o "$obj" 2>/dev/null || true
        else
            "$WIN_CXX" "${WIN_INCLUDES[@]}" -std=c++17 -O2 -DNDEBUG -DKON_USE_PACK -c "$src" -o "$obj" 2>/dev/null || true
        fi
        [ -f "$obj" ] && OBJS_WIN+=("$obj")
    done

    [ ${#OBJS_WIN[@]} -gt 0 ] && \
        "${WIN_AR}" rcs "${OUT_DIR_WIN}/libKonEngine.a" "${OBJS_WIN[@]}" && \
        ok "libKonEngine.a (windows64) → ${OUT_DIR_WIN}/libKonEngine.a" || \
        warn "Windows engine lib build incomplete"

    # Copy headers for windows64 (same headers as linux64)
    mkdir -p "${OUT_DIR_WIN}/include"
    cp "${ENGINE_ROOT}/src/KonEngine.hpp" "${OUT_DIR_WIN}/include/"
    find "${ENGINE_ROOT}/src" -name "*.hpp" | while read -r hpp; do
        rel="${hpp#${ENGINE_ROOT}/src/}"
        dst="${OUT_DIR_WIN}/include/${rel}"
        mkdir -p "$(dirname "$dst")"
        cp "$hpp" "$dst"
    done
    if [ -d "${ENGINE_ROOT}/libs/glm/glm" ]; then
        mkdir -p "${OUT_DIR_WIN}/include/glm"
        cp -r "${ENGINE_ROOT}/libs/glm/glm/." "${OUT_DIR_WIN}/include/glm/"
    fi
    if [ -d "${ENGINE_ROOT}/libs/glfw/include/GLFW" ]; then
        mkdir -p "${OUT_DIR_WIN}/include/GLFW"
        cp -r "${ENGINE_ROOT}/libs/glfw/include/GLFW/." "${OUT_DIR_WIN}/include/GLFW/"
    fi
    mkdir -p "${OUT_DIR_WIN}/include/glad/include/glad" "${OUT_DIR_WIN}/include/KHR"
    cp -r "${ENGINE_ROOT}/src/glad/include/glad/"  "${OUT_DIR_WIN}/include/glad/include/glad/"  2>/dev/null || true
    cp -r "${ENGINE_ROOT}/src/glad/include/KHR/"   "${OUT_DIR_WIN}/include/KHR/"                2>/dev/null || true
    ok "Headers (windows64) → ${OUT_DIR_WIN}/include/"

    # Build libglfw3.a for Windows
    GLFW_OUT_WIN="${OUT_DIR_WIN}/libglfw3.a"
    if [ -f "$GLFW_OUT_WIN" ]; then
        ok "libglfw3.a (windows64) already built, skipping"
    else
        info "Building libglfw3.a (windows64)..."
        GLFW_BUILD_WIN="${PREFIX}/_build/glfw_windows64"
        mkdir -p "$GLFW_BUILD_WIN"
        GLFW_WIN_SRCS=()
        for f in context.c init.c input.c monitor.c platform.c vulkan.c window.c \
                  egl_context.c osmesa_context.c null_init.c null_monitor.c \
                  null_window.c null_joystick.c win32_time.c win32_module.c \
                  win32_thread.c win32_init.c win32_joystick.c win32_monitor.c \
                  win32_window.c wgl_context.c; do
            [ -f "${GLFW_DIR}/src/$f" ] && GLFW_WIN_SRCS+=("${GLFW_DIR}/src/$f")
        done
        GLFW_WIN_CC="${WIN_CC}"
        GLFW_WIN_OBJS=()
        for src in "${GLFW_WIN_SRCS[@]}"; do
            base=$(basename "$src" .c).o
            obj="$GLFW_BUILD_WIN/$base"
            "$GLFW_WIN_CC" -I"$GLFW_DIR/include" -I"$GLFW_DIR/src" \
                -D_GLFW_WIN32 -O2 -x c -c "$src" -o "$obj" 2>/dev/null && GLFW_WIN_OBJS+=("$obj")
        done
        [ ${#GLFW_WIN_OBJS[@]} -gt 0 ] && \
            "${WIN_AR}" rcs "$GLFW_OUT_WIN" "${GLFW_WIN_OBJS[@]}" && \
            ok "libglfw3.a (windows64) → $GLFW_OUT_WIN" || \
            warn "Windows GLFW build incomplete"
    fi
fi

echo ""
echo "======================================================"
echo -e "  ${GREEN}Engine library ready!${NC}"
echo "======================================================"
echo ""
echo "  Now editor can build games with:"
echo "    konscript game.ks  →  ./game  (no cmake, no g++)"
echo ""
