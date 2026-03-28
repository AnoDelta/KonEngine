#!/bin/bash
# Build KonPaktor from the KonEngine root directory
set -e

cd tools/KonPaktor
mkdir -p build
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target KonPaktor
cmake --build build --target konpak

echo ""
echo "==================================================="
echo " Done!"
echo "   GUI : tools/KonPaktor/build/KonPaktor"
echo "   CLI : tools/KonPaktor/build/konpak"
echo "==================================================="
