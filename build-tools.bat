@echo off
:: Build KonAnimator and anim_compiler (Qt tools)
:: Run this once from the KonEngine root directory.
:: Edit QT_DIR below to match your Qt5 installation.

set QT_DIR=C:\Qt\5.15.2\msvc2019_64

if not exist build mkdir build
cd build

cmake .. -DKON_BUILD_TOOLS=ON -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="%QT_DIR%"
cmake --build . --config Release --target KonAnimator --target anim_compiler

echo.
echo Done:
echo   build\tools\KonAnimator\Release\KonAnimator.exe
echo   build\Release\anim_compiler.exe
pause
