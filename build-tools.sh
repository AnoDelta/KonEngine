#!/bin/bash
# Build all KonEngine tools: KonAnimator, anim_compiler, KonPaktor, konpak
set -e

# ---- Engine tools (KonAnimator + anim_compiler) ----
echo "Building KonAnimator and anim_compiler..."
mkdir -p build
cmake -B build -DKON_BUILD_TOOLS=ON -DCMAKE_BUILD_TYPE=Release
make -j$(nproc) -C build KonAnimator anim_compiler

# ---- KonPaktor + konpak CLI ----
echo ""
echo "Building KonPaktor and konpak..."
cd tools/KonPaktor
mkdir -p build
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target KonPaktor
cmake --build build --target konpak
cd ../..

echo ""
echo "==================================================="
echo " Done!"
echo "   KonAnimator : build/tools/KonAnimator/KonAnimator"
echo "   anim_compiler: build/anim_compiler"
echo "   KonPaktor   : tools/KonPaktor/build/KonPaktor"
echo "   konpak      : tools/KonPaktor/build/konpak"
echo "==================================================="
