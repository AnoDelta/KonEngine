#!/bin/bash
# Build KonAnimator and anim_compiler (Qt tools)
# Run this once from the KonEngine root directory.
set -e

mkdir -p build && cd build
cmake .. -DKON_BUILD_TOOLS=ON -DCMAKE_BUILD_TYPE=Release
make -j$(nproc) KonAnimator anim_compiler

echo ""
echo "Done:"
echo "  build/tools/KonAnimator/KonAnimator"
echo "  build/anim_compiler"
