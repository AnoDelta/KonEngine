#!/bin/bash
set -e
mkdir -p build && cd build
cmake .. -DKON_BUILD_TOOLS=ON -DCMAKE_BUILD_TYPE=Release
make -j$(nproc) KonAnimator anim_compiler
echo ""
echo "Done:"
echo "  build/tools/KonAnimator/KonAnimator"
echo "  build/anim_compiler"
