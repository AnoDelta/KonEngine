@echo off
:: build-windows.bat — build KonAnimator natively on Windows
:: Requires Qt5 installed and either MSVC (via VS) or MinGW-w64

setlocal enabledelayedexpansion

:: -----------------------------------------------------------------------
:: Qt5 path — edit this if Qt is installed elsewhere
:: Common locations:
::   C:\Qt\5.15.2\msvc2019_64
::   C:\Qt\5.15.2\mingw81_64
:: -----------------------------------------------------------------------
set QT_DIR=C:\Qt\5.15.2\msvc2019_64

if not exist "%QT_DIR%\lib\cmake\Qt5\Qt5Config.cmake" (
    echo.
    echo  ERROR: Qt5 not found at %QT_DIR%
    echo  Edit QT_DIR in this script to point at your Qt5 installation.
    echo  Download Qt5 from: https://www.qt.io/download-open-source
    echo.
    pause
    exit /b 1
)

if not exist build-windows mkdir build-windows
cd build-windows

cmake .. ^
    -DQt5_DIR="%QT_DIR%\lib\cmake\Qt5" ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DCMAKE_PREFIX_PATH="%QT_DIR%"

cmake --build . --config Release --parallel

echo.
echo ===================================================
echo  Done!  build-windows\Release\KonAnimator.exe
echo.
echo  Run windeployqt to bundle Qt DLLs for distribution:
echo    %QT_DIR%\bin\windeployqt.exe Release\KonAnimator.exe
echo ===================================================
pause
