@echo off
:: ---------------------------------------------------------------------------
:: build.bat — Build KonScript on Windows
::
:: Supports:
::   - MSVC (cl.exe)  — if Visual Studio is installed
::   - MinGW-w64 (g++) — if MinGW is on PATH
::   - Clang (clang++) — if LLVM/Clang is installed
::
:: Bakes the toolchain path into the binary so konscript.exe works
:: from any directory without any environment variables.
::
:: Usage:
::   build.bat              -- auto-detect compiler
::   build.bat --msvc       -- force MSVC
::   build.bat --mingw      -- force MinGW g++
::   build.bat --clang      -- force clang++
:: ---------------------------------------------------------------------------
setlocal EnableDelayedExpansion

set SCRIPT_DIR=%~dp0
set SCRIPT_DIR=%SCRIPT_DIR:~0,-1%

:: ── Find toolchain ────────────────────────────────────────────────────────
set TOOLCHAIN_DIR=
if exist "%SCRIPT_DIR%\toolchain\llvm\bin\llc.exe" (
    set TOOLCHAIN_DIR=%SCRIPT_DIR%\toolchain
)
if not defined TOOLCHAIN_DIR (
    if defined KONSCRIPT_TOOLCHAIN (
        if exist "%KONSCRIPT_TOOLCHAIN%\llvm\bin\llc.exe" (
            set TOOLCHAIN_DIR=%KONSCRIPT_TOOLCHAIN%
        )
    )
)

echo.
echo Building konscript...
if defined TOOLCHAIN_DIR (
    echo   Toolchain: !TOOLCHAIN_DIR! ^(baked in^)
) else (
    echo   Toolchain: not found -- run bundle-toolchain.ps1 first
    echo   Binary will require KONSCRIPT_TOOLCHAIN env var
)

:: ── Detect or select compiler ─────────────────────────────────────────────
set COMPILER=
set COMPILER_NAME=

:: Check command line override
set FORCE_MSVC=0
set FORCE_MINGW=0
set FORCE_CLANG=0

for %%A in (%*) do (
    if "%%A"=="--msvc"  set FORCE_MSVC=1
    if "%%A"=="--mingw" set FORCE_MINGW=1
    if "%%A"=="--clang" set FORCE_CLANG=1
)

if %FORCE_MSVC%==1  goto try_msvc
if %FORCE_MINGW%==1 goto try_mingw
if %FORCE_CLANG%==1 goto try_clang

:: Auto-detect: prefer g++ (MinGW), then clang++, then cl
where g++     >nul 2>&1 && goto try_mingw
where clang++ >nul 2>&1 && goto try_clang
where cl      >nul 2>&1 && goto try_msvc

echo ERROR: No C++ compiler found.
echo.
echo Install one of:
echo   MinGW-w64: https://winlibs.com/
echo   LLVM/Clang: https://github.com/llvm/llvm-project/releases
echo   Visual Studio: https://visualstudio.microsoft.com/
goto :eof

:try_mingw
    where g++ >nul 2>&1
    if errorlevel 1 goto try_clang
    set COMPILER=g++
    set COMPILER_NAME=MinGW g++
    goto compile

:try_clang
    where clang++ >nul 2>&1
    if errorlevel 1 goto try_msvc
    set COMPILER=clang++
    set COMPILER_NAME=clang++
    goto compile

:try_msvc
    where cl >nul 2>&1
    if errorlevel 1 (
        echo ERROR: No compiler found.
        goto :eof
    )
    set COMPILER=cl
    set COMPILER_NAME=MSVC cl.exe
    goto compile_msvc

:: ── Compile with g++ or clang++ ──────────────────────────────────────────
:compile
echo   Compiler : %COMPILER_NAME%

set DEFINES=
if defined TOOLCHAIN_DIR (
    :: Escape backslashes for the -D flag
    set TC_ESCAPED=!TOOLCHAIN_DIR:\=\\!
    set DEFINES=-DKONSCRIPT_TOOLCHAIN_BUILTIN="!TC_ESCAPED!"
)

%COMPILER% -std=c++17 -O2 -I include %DEFINES% src\main.cpp -o konscript.exe
if errorlevel 1 (
    echo.
    echo BUILD FAILED
    goto :eof
)
goto done

:: ── Compile with MSVC ─────────────────────────────────────────────────────
:compile_msvc
echo   Compiler : %COMPILER_NAME%

set DEFINES=
if defined TOOLCHAIN_DIR (
    set TC_ESCAPED=!TOOLCHAIN_DIR:\=\\!
    set DEFINES=/DKONSCRIPT_TOOLCHAIN_BUILTIN="!TC_ESCAPED!"
)

:: MSVC flags: /std:c++17 /O2 /EHsc /Fe:konscript.exe
cl /std:c++17 /O2 /EHsc /I include %DEFINES% src\main.cpp /Fe:konscript.exe /link
if errorlevel 1 (
    echo.
    echo BUILD FAILED
    goto :eof
)
:: Clean up MSVC intermediate files
del /f /q konscript.obj 2>nul

:done
echo.
echo ===================================================
echo  Done!
echo    konscript.exe : %SCRIPT_DIR%\konscript.exe
if defined TOOLCHAIN_DIR (
echo.
echo    Toolchain baked in -- works from any directory
)
echo ===================================================
echo.
echo  Install with: install.bat
echo.
