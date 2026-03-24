#!/bin/bash
mkdir -p build-windows
cd build-windows
cmake .. -DCMAKE_TOOLCHAIN_FILE=../windows-toolchain.cmake
cmake --build .
