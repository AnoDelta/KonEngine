@echo off
:: ---------------------------------------------------------------------------
:: bootstrap.bat — KonScript self-hosting bootstrap (Windows)
::
:: Stage 0: build konscript.exe from C++ source
:: Stage 1: rebuild from konscript.ks using stage-0 binary
::
:: Usage:
::   bootstrap.bat           -- stage 0 only
::   bootstrap.bat --stage1  -- stage 0 + stage 1
::   bootstrap.bat --verify  -- stage 0 + 1 + 2 (self-hosting check)
:: ---------------------------------------------------------------------------
setlocal EnableDelayedExpansion

set SCRIPT_DIR=%~dp0
set SCRIPT_DIR=%SCRIPT_DIR:~0,-1%

set DO_STAGE1=0
set DO_VERIFY=0

for %%A in (%*) do (
    if "%%A"=="--stage1" set DO_STAGE1=1
    if "%%A"=="--verify" set DO_STAGE1=1 & set DO_VERIFY=1
    if "%%A"=="--help"   goto print_help
)
goto run

:print_help
echo Usage: bootstrap.bat [--stage1] [--verify]
echo   (no flags)  Build konscript.exe from C++ source (stage 0)
echo   --stage1    Also rebuild from konscript.ks using stage-0 binary
echo   --verify    Full self-hosting check
goto :eof

:run
echo.
echo ======================================================
echo   KonScript Bootstrap (Windows)
echo ======================================================
echo.

:: ── Stage 0: C++ build ───────────────────────────────────────────────────
echo [..] Stage 0: building konscript.exe from C++ source...
call "%SCRIPT_DIR%\build.bat"
if errorlevel 1 (
    echo [!!] Stage 0 FAILED
    goto :eof
)

:: Copy to konscript-stage0.exe
copy /Y "%SCRIPT_DIR%\konscript.exe" "%SCRIPT_DIR%\konscript-stage0.exe" >nul
echo [ok] konscript-stage0.exe built

if %DO_STAGE1%==0 (
    echo [ok] konscript.exe is the stage-0 C++ binary
    echo      Run with --stage1 to build self-hosted version
    goto done
)

:: ── Stage 1: self-hosted ─────────────────────────────────────────────────
echo.
echo [..] Stage 1: rebuilding from konscript.ks using stage-0 binary...

if not exist "%SCRIPT_DIR%\src\konscript.ks" (
    echo [!!] src\konscript.ks not found
    goto :eof
)

:: Find toolchain
set LLC=
set LLD=
set SYSROOT=

if exist "%SCRIPT_DIR%\toolchain\llvm\bin\llc.exe" (
    set LLC=%SCRIPT_DIR%\toolchain\llvm\bin\llc.exe
    set LLD=%SCRIPT_DIR%\toolchain\llvm\bin\ld.lld.exe
    set SYSROOT=%SCRIPT_DIR%\toolchain\sysroot\windows64\lib
) else if defined KONSCRIPT_TOOLCHAIN (
    if exist "%KONSCRIPT_TOOLCHAIN%\llvm\bin\llc.exe" (
        set LLC=%KONSCRIPT_TOOLCHAIN%\llvm\bin\llc.exe
        set LLD=%KONSCRIPT_TOOLCHAIN%\llvm\bin\ld.lld.exe
        set SYSROOT=%KONSCRIPT_TOOLCHAIN%\sysroot\windows64\lib
    )
)

if not defined LLC (
    echo [!!] Toolchain not found. Run bundle-toolchain.ps1 first.
    goto :eof
)

:: IRGen
"%SCRIPT_DIR%\konscript-stage0.exe" --llvm src\konscript.ks -o "%TEMP%\konscript-stage1.ll"
if errorlevel 1 (
    echo [!!] IRGen failed
    goto :eof
)
echo [ok] IRGen complete

:: llc
"%LLC%" -filetype=obj "%TEMP%\konscript-stage1.ll" ^
    -o "%TEMP%\konscript-stage1.obj" ^
    --mtriple=x86_64-pc-windows-msvc
if errorlevel 1 (
    echo [!!] llc failed
    goto :eof
)
echo [ok] llc complete

:: lld-link
set LLDLINK=%SCRIPT_DIR%\toolchain\llvm\bin\lld-link.exe
"%LLDLINK%" /OUT:konscript-stage1.exe /SUBSYSTEM:CONSOLE ^
    "%TEMP%\konscript-stage1.obj" ^
    "%SYSROOT%\libmingwex.a" ^
    "%SYSROOT%\libmsvcrt.a" ^
    "%SYSROOT%\libkernel32.a"
if errorlevel 1 (
    echo [!!] Link failed
    goto :eof
)
echo [ok] konscript-stage1.exe built

if %DO_VERIFY%==0 (
    :: Keep stage-0 as the working binary — stage-1 is not verified yet
    echo [!!] Stage-1 built but not promoted -- use --verify to promote when ready
    echo [ok] konscript.exe is still the stage-0 C++ binary
    goto done
)

:: ── Stage 2: verify ──────────────────────────────────────────────────────
echo.
echo [..] Stage 2: verifying self-hosting...

"%SCRIPT_DIR%\konscript-stage1.exe" --llvm src\konscript.ks -o "%TEMP%\konscript-stage2.ll"
if errorlevel 1 (
    echo [!!] Stage-2 IRGen failed
    goto :eof
)

fc /b "%TEMP%\konscript-stage1.ll" "%TEMP%\konscript-stage2.ll" >nul 2>&1
if errorlevel 1 (
    echo [!!] IR differs between stage 1 and stage 2 -- not yet fully self-hosting
    copy /Y "%SCRIPT_DIR%\konscript-stage0.exe" "%SCRIPT_DIR%\konscript.exe" >nul
) else (
    echo [ok] Stage 1 and Stage 2 produce identical IR -- self-hosting verified!
    copy /Y "%SCRIPT_DIR%\konscript-stage1.exe" "%SCRIPT_DIR%\konscript.exe" >nul
    echo [ok] konscript.exe promoted to stage-1 self-hosted binary
)

:done
echo.
echo ======================================================
echo   Done!
echo ======================================================
echo.
echo   konscript.exe : %SCRIPT_DIR%\konscript.exe
echo.
echo   Install with: install.bat
echo.
