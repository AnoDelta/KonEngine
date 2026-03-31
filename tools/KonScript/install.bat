@echo off
:: ---------------------------------------------------------------------------
:: install.bat — Install KonScript on Windows
::
:: Copies konscript.exe to a directory on PATH.
:: Default: %LOCALAPPDATA%\KonScript\bin (no admin needed)
:: Admin:   C:\Program Files\KonScript\bin
::
:: Usage:
::   install.bat                     -- install to LocalAppData (no admin)
::   install.bat --system            -- install to Program Files (admin)
::   install.bat --prefix=C:\mydir   -- install to custom directory
:: ---------------------------------------------------------------------------
setlocal EnableDelayedExpansion

set SCRIPT_DIR=%~dp0
set SCRIPT_DIR=%SCRIPT_DIR:~0,-1%

:: ── Parse args ────────────────────────────────────────────────────────────
set PREFIX=%LOCALAPPDATA%\KonScript
set SYSTEM_INSTALL=0

for %%A in (%*) do (
    if "%%A"=="--system" set SYSTEM_INSTALL=1
    if "%%A:~0,9%"=="--prefix=" (
        set PREFIX=%%A:~9%
    )
)

if %SYSTEM_INSTALL%==1 (
    set PREFIX=C:\Program Files\KonScript
)

set BIN_DIR=%PREFIX%\bin

:: ── Find binary to install ────────────────────────────────────────────────
set INSTALL_BIN=
if exist "%SCRIPT_DIR%\konscript.exe" (
    set INSTALL_BIN=%SCRIPT_DIR%\konscript.exe
) else if exist "%SCRIPT_DIR%\konscript-stage0.exe" (
    set INSTALL_BIN=%SCRIPT_DIR%\konscript-stage0.exe
) else (
    echo konscript.exe not found. Building first...
    call "%SCRIPT_DIR%\build.bat"
    if errorlevel 1 goto :eof
    set INSTALL_BIN=%SCRIPT_DIR%\konscript.exe
)

:: ── Create install directory ──────────────────────────────────────────────
if not exist "%BIN_DIR%" mkdir "%BIN_DIR%"

echo Installing to %BIN_DIR%...

copy /Y "%INSTALL_BIN%" "%BIN_DIR%\konscript.exe" >nul
if errorlevel 1 (
    echo ERROR: Could not copy to %BIN_DIR%
    if %SYSTEM_INSTALL%==0 (
        echo Try running as Administrator or use --system flag.
    )
    goto :eof
)

:: ── Add to PATH if not already there ─────────────────────────────────────
echo %PATH% | find /I "%BIN_DIR%" >nul 2>&1
if errorlevel 1 (
    echo Adding %BIN_DIR% to user PATH...
    setx PATH "%PATH%;%BIN_DIR%" >nul
    echo   Added. Restart your terminal for PATH to take effect.
) else (
    echo   Already on PATH.
)

echo.
echo ===================================================
echo  Installed!
echo    %BIN_DIR%\konscript.exe
echo.
echo  Usage:
echo    konscript hello.ks           -- build native binary
echo    konscript --cpp  hello.ks    -- transpile to C++
echo    konscript --llvm hello.ks    -- emit LLVM IR
echo    konscript --target linux64 hello.ks  -- cross-compile to Linux
echo ===================================================
echo.
echo  NOTE: Restart your terminal for PATH changes to take effect.
echo.
