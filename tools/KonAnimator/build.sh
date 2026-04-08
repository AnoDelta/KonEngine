#!/bin/bash
set -e
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
echo ""
echo "Done:"
echo "  build/KonAnimator"
[ -f "anim_compiler" ] && echo "  build/anim_compiler"
