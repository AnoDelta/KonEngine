@echo off
:: Build all KonEngine tools: KonAnimator, anim_compiler, KonPaktor, konpak

:: ---- Engine tools ----
echo Building KonAnimator and anim_compiler...
if not exist build mkdir build
cmake -B build -DKON_BUILD_TOOLS=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build --target KonAnimator
cmake --build build --target anim_compiler
if errorlevel 1 goto fail

:: ---- KonPaktor ----
echo.
echo Building KonPaktor and konpak...
cd tools\KonPaktor
if not exist build mkdir build
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --target KonPaktor
cmake --build build --target konpak
if errorlevel 1 (cd ..\.. & goto fail)
cd ..\..

echo.
echo ===================================================
echo  Done!
echo    KonAnimator  : build\tools\KonAnimator\KonAnimator.exe
echo    anim_compiler: build\anim_compiler.exe
echo    KonPaktor    : tools\KonPaktor\build\KonPaktor.exe
echo    konpak       : tools\KonPaktor\build\konpak.exe
echo ===================================================
goto end

:fail
echo Build failed!
exit /b 1

:end
pause
