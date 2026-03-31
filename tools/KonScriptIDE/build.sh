#!/bin/bash
# Build KonScriptIDE — standalone KonScript text editor
# Usage:
#   ./build.sh              -- release build
#   ./build.sh --debug      -- debug build
#   ./build.sh --clean      -- clean and rebuild
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

BUILD_TYPE="Release"
CLEAN=false

for arg in "$@"; do
    case "$arg" in
        --debug) BUILD_TYPE="Debug" ;;
        --clean) CLEAN=true ;;
    esac
done

if [ "$CLEAN" = true ] && [ -d build ]; then
    echo "Cleaning..."
    rm -rf build
fi

mkdir -p build
cd build

echo "Configuring KonScriptIDE ($BUILD_TYPE)..."
cmake .. -DCMAKE_BUILD_TYPE="$BUILD_TYPE"

echo "Building..."
make -j"$(nproc)"

echo ""
echo "==================================================="
echo " Done!"
echo "   KonScriptIDE : $(pwd)/KonScriptIDE"
echo ""
echo " Run:"
echo "   ./build/KonScriptIDE"
echo "   ./build/KonScriptIDE path/to/file.ks"
echo "==================================================="
