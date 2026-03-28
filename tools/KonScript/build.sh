#!/bin/bash
# Build KonScript -- compiles the konscript backend binary
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

echo "Building konscript..."
g++ -std=c++17 -O2 -I include src/main.cpp -o konscript

echo ""
echo "==================================================="
echo " Done!"
echo "   Backend : $(pwd)/konscript"
echo "   Frontend: ksc (install with ./install.sh)"
echo "==================================================="
