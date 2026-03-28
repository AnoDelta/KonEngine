@echo off
:: Install KonEngine library and optionally tools
:: Usage:
::   install.bat           -- engine library only
::   install.bat --tools   -- engine + all tools

set INSTALL_TOOLS=OFF
set INSTALL_PREFIX=C:\KonEngine

for %%A in (%*) do (
    if "%%A"=="--tools" set INSTALL_TOOLS=ON
)

:: ---- Build + install engine ----
echo Building KonEngine...
if not exist build mkdir build
cmake -B build -DCMAKE_BUILD_TYPE=Release ^
    -DCMAKE_INSTALL_PREFIX="%INSTALL_PREFIX%" ^
    -DKON_BUILD_TOOLS=%INSTALL_TOOLS%
cmake --build build --config Release
cmake --install build
if errorlevel 1 goto fail

echo.
echo KonEngine installed to %INSTALL_PREFIX%

:: ---- Install tools ----
if "%INSTALL_TOOLS%"=="ON" (
    echo.
    echo Installing tools...

    copy /Y build\tools\KonAnimator\Release\KonAnimator.exe "%INSTALL_PREFIX%\bin\"
    copy /Y build\Release\anim_compiler.exe "%INSTALL_PREFIX%\bin\"

    cd tools\KonPaktor
    if not exist build mkdir build
    cmake -B build -DCMAKE_BUILD_TYPE=Release ^
        -DCMAKE_INSTALL_PREFIX="%INSTALL_PREFIX%"
    cmake --build build --config Release --target KonPaktor
    cmake --build build --config Release --target konpak
    cmake --install build
    if errorlevel 1 (cd ..\.. & goto fail)
    cd ..\..

    echo.
    echo Tools installed to %INSTALL_PREFIX%\bin\
)

echo.
echo Done!
goto end

:fail
echo Installation failed!
exit /b 1

:end
pause
