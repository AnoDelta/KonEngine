@echo off
if not exist build mkdir build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release
echo.
echo Done: build\Release\KonAnimator.exe
