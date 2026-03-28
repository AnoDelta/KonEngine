#!/bin/bash
# Build and run the KonEngine test suite
set -e

mkdir -p build
cmake -B build -DKON_BUILD_TESTS=ON -DCMAKE_BUILD_TYPE=Debug
cmake --build build --target KonEngineTest
echo ""
echo "Running tests..."
echo ""
./build/tests/KonEngineTest

