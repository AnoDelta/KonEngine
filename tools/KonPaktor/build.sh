#!/bin/bash
# Build KonPaktor (GUI) and konpak (CLI) from within tools/KonPaktor/
set -e

mkdir -p build
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target KonPaktor
cmake --build build --target konpak

echo ""
echo "==================================================="
echo " Done!"
echo "   GUI : build/KonPaktor"
echo "   CLI : build/konpak"
echo "==================================================="
