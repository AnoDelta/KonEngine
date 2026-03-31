@echo off
:: Build KonScriptIDE — standalone KonScript text editor (Windows)
:: Usage:
::   build.bat              -- release build
::   build.bat --debug      -- debug build
::   build.bat --clean      -- clean and rebuild
setlocal EnableDelayedExpansion

set SCRIPT_DIR=%~dp0
set SCRIPT_DIR=%SCRIPT_DIR:~0,-1%
cd "%SCRIPT_DIR%"

set BUILD_TYPE=Release
set CLEAN=0

for %%A in (%*) do (
    if "%%A"=="--debug" set BUILD_TYPE=Debug
    if "%%A"=="--clean" set CLEAN=1
)

if %CLEAN%==1 (
    if exist build (
        echo Cleaning...
        rmdir /s /q build
    )
)

if not exist build mkdir build
cd build

:: Detect generator — prefer Ninja, fall back to VS, then NMake
set CMAKE_GEN=
where ninja >nul 2>&1 && set CMAKE_GEN=-G "Ninja"

echo Configuring KonScriptIDE (%BUILD_TYPE%)...
cmake .. %CMAKE_GEN% -DCMAKE_BUILD_TYPE=%BUILD_TYPE%
if errorlevel 1 goto fail

echo Building...
cmake --build . --config %BUILD_TYPE% --parallel
if errorlevel 1 goto fail

echo.
echo ===================================================
echo  Done!
echo    KonScriptIDE.exe : %SCRIPT_DIR%\build\KonScriptIDE.exe
echo.
echo  Run:
echo    build\KonScriptIDE.exe
echo    build\KonScriptIDE.exe path\to\file.ks
echo ===================================================
goto end

:fail
echo.
echo BUILD FAILED
exit /b 1

:end
