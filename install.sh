#!/bin/bash
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

mkdir -p build
cd build
cmake ..
cmake --build .
sudo cmake --install .

echo "KonEngine installed successfully!"
