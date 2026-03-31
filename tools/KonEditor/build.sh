#!/usr/bin/env bash
set -e
mkdir -p build
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
echo ""
echo "=== Done! ==="
echo "  ./build/KonEditor"
