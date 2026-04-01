#!/usr/bin/env bash
# ---------------------------------------------------------------------------
# sync-headers.sh
#
# Quickly copies all engine .hpp files into the KonScript toolchain so
# konscript picks up the latest headers without a full rebuild.
#
# Run this whenever you change a .hpp in src/.
# Run build-engine-lib.sh instead when you change a .cpp (needs recompile).
#
# Usage:
#   ./sync-headers.sh              # sync linux64 only
#   ./sync-headers.sh --all        # sync linux64 + windows64
# ---------------------------------------------------------------------------
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ENGINE_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
TOOLCHAIN="${SCRIPT_DIR}/toolchain"

GREEN='\033[0;32m'; NC='\033[0m'
ok() { echo -e "  ${GREEN}[ok]${NC}  $*"; }

DO_ALL=0
for arg in "$@"; do
    case "$arg" in --all) DO_ALL=1 ;; esac
done

sync_platform() {
    local DEST="$1/include"
    mkdir -p "$DEST"

    # KonEngine.hpp
    cp "${ENGINE_ROOT}/src/KonEngine.hpp" "$DEST/"

    # All .hpp files under src/, preserving directory structure
    find "${ENGINE_ROOT}/src" -name "*.hpp" | while read -r hpp; do
        rel="${hpp#${ENGINE_ROOT}/src/}"
        dst="${DEST}/${rel}"
        mkdir -p "$(dirname "$dst")"
        cp "$hpp" "$dst"
    done

    # GLM
    if [ -d "${ENGINE_ROOT}/libs/glm/glm" ]; then
        mkdir -p "${DEST}/glm"
        cp -r "${ENGINE_ROOT}/libs/glm/glm/." "${DEST}/glm/"
    fi

    # GLFW
    if [ -d "${ENGINE_ROOT}/libs/glfw/include/GLFW" ]; then
        mkdir -p "${DEST}/GLFW"
        cp -r "${ENGINE_ROOT}/libs/glfw/include/GLFW/." "${DEST}/GLFW/"
    fi

    ok "Headers → $DEST"
}

echo ""
echo "=============================="
echo "  KonEngine Header Sync"
echo "=============================="

sync_platform "${TOOLCHAIN}/engine/linux64"

if [ "$DO_ALL" = "1" ] && [ -d "${TOOLCHAIN}/engine/windows64" ]; then
    sync_platform "${TOOLCHAIN}/engine/windows64"
fi

echo ""
echo -e "  ${GREEN}Done!${NC} Headers are up to date."
echo "  (If you changed a .cpp, run ./build-engine-lib.sh instead)"
echo ""
