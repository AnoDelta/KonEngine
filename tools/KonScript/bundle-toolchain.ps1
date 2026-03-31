# ---------------------------------------------------------------------------
# bundle-toolchain.ps1
#
# Sets up the self-contained KonScript toolchain on Windows.
# Downloads LLVM prebuilt binaries and MinGW-w64 CRT for static linking.
#
# Run from tools/KonScript/:
#   powershell -ExecutionPolicy Bypass -File bundle-toolchain.ps1
#
# What it does:
#   1. Downloads LLVM prebuilt for Windows (llc.exe, ld.lld.exe, lld-link.exe)
#   2. Downloads MinGW-w64 sysroot (CRT objects + import libs)
#   3. Writes toolchain\llvm\bin\ and toolchain\sysroot\windows64\lib\
#
# After this, konscript.exe is fully self-contained — no MSVC, no MinGW
# installation needed on end-user machines.
# ---------------------------------------------------------------------------

param(
    [string]$Prefix = "",
    [string]$LlvmVersion = "17.0.6"
)

$ErrorActionPreference = "Stop"

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
if (-not $Prefix) { $Prefix = Join-Path $ScriptDir "toolchain" }

$LlvmDir   = Join-Path $Prefix "llvm"
$SysrootDir = Join-Path $Prefix "sysroot\windows64\lib"
$BuildDir  = Join-Path $Prefix "_build"

function Write-OK   { param($msg) Write-Host "[ok]  $msg" -ForegroundColor Green }
function Write-Info { param($msg) Write-Host "[..]  $msg" -ForegroundColor Cyan }
function Write-Warn { param($msg) Write-Host "[!!]  $msg" -ForegroundColor Yellow }
function Write-Fail { param($msg) Write-Host "[!!]  $msg" -ForegroundColor Red; exit 1 }

Write-Host ""
Write-Host "======================================================" -ForegroundColor White
Write-Host "  KonEditor Toolchain Builder (Windows)"               -ForegroundColor White
Write-Host "======================================================" -ForegroundColor White
Write-Host "  Prefix  : $Prefix"
Write-Host "  LLVM    : $LlvmVersion"
Write-Host "======================================================"
Write-Host ""

# ── Create directories ────────────────────────────────────────────────────
New-Item -ItemType Directory -Force -Path "$LlvmDir\bin"  | Out-Null
New-Item -ItemType Directory -Force -Path $SysrootDir     | Out-Null
New-Item -ItemType Directory -Force -Path $BuildDir       | Out-Null

# ── Helper: download with progress ───────────────────────────────────────
function Download-File {
    param([string]$Url, [string]$Dest)
    if (Test-Path $Dest) {
        Write-OK "Already downloaded: $(Split-Path -Leaf $Dest)"
        return
    }
    Write-Info "Downloading $(Split-Path -Leaf $Dest)..."
    $ProgressPreference = 'SilentlyContinue'
    Invoke-WebRequest -Uri $Url -OutFile $Dest -UseBasicParsing
    Write-OK "Downloaded $(Split-Path -Leaf $Dest)"
}

# ── Step 1: LLVM prebuilt for Windows ────────────────────────────────────
# LLVM GitHub releases provide a Windows zip with all tools
$LlvmZip  = Join-Path $BuildDir "llvm-windows.zip"
$LlvmUrl  = "https://github.com/llvm/llvm-project/releases/download/llvmorg-$LlvmVersion/clang+llvm-$LlvmVersion-x86_64-pc-windows-msvc.tar.xz"

# Check if LLVM tools already exist
$LlcExe = Join-Path $LlvmDir "bin\llc.exe"
$LldExe = Join-Path $LlvmDir "bin\ld.lld.exe"

if ((Test-Path $LlcExe) -and (Test-Path $LldExe)) {
    Write-OK "LLVM tools already present, skipping download"
} else {
    Write-Info "Checking for system LLVM first..."

    # Prefer system LLVM if available (much faster than downloading)
    $SystemLlc   = Get-Command "llc"    -ErrorAction SilentlyContinue
    $SystemLld   = Get-Command "ld.lld" -ErrorAction SilentlyContinue
    $SystemLldLk = Get-Command "lld-link" -ErrorAction SilentlyContinue
    $SystemWasm  = Get-Command "wasm-ld" -ErrorAction SilentlyContinue

    if ($SystemLlc -and $SystemLld) {
        Write-OK "Found system LLVM — copying to toolchain"
        Copy-Item $SystemLlc.Source   "$LlvmDir\bin\llc.exe"    -Force
        Copy-Item $SystemLld.Source   "$LlvmDir\bin\ld.lld.exe" -Force
        if ($SystemLldLk) { Copy-Item $SystemLldLk.Source "$LlvmDir\bin\lld-link.exe" -Force }
        if ($SystemWasm)  { Copy-Item $SystemWasm.Source  "$LlvmDir\bin\wasm-ld.exe"  -Force }
    } else {
        Write-Info "System LLVM not found — downloading prebuilt..."
        Write-Info "This is a large download (~500MB). Please wait..."

        # Download 7-Zip standalone for extraction if needed
        $SevenZip = Get-Command "7z" -ErrorAction SilentlyContinue
        if (-not $SevenZip) {
            $7zUrl  = "https://www.7-zip.org/a/7zr.exe"
            $7zExe  = Join-Path $BuildDir "7zr.exe"
            Download-File $7zUrl $7zExe
            $SevenZip = $7zExe
        }

        $LlvmTar = Join-Path $BuildDir "llvm.tar.xz"
        Download-File $LlvmUrl $LlvmTar

        Write-Info "Extracting LLVM (this takes a moment)..."
        $LlvmExtract = Join-Path $BuildDir "llvm-extract"
        New-Item -ItemType Directory -Force -Path $LlvmExtract | Out-Null
        & $SevenZip x $LlvmTar -o"$BuildDir" -y | Out-Null
        # .tar.xz extracts to .tar first
        $TarFile = Get-ChildItem $BuildDir -Filter "*.tar" | Select-Object -First 1
        if ($TarFile) {
            & $SevenZip x $TarFile.FullName -o"$LlvmExtract" -y | Out-Null
        }

        # Find and copy the tools
        $LlvmBinSrc = Get-ChildItem $LlvmExtract -Recurse -Filter "llc.exe" |
                      Select-Object -First 1 | Split-Path -Parent
        if (-not $LlvmBinSrc) {
            Write-Fail "Could not find llc.exe in extracted LLVM"
        }

        $tools = @("llc.exe","ld.lld.exe","lld-link.exe","wasm-ld.exe","llvm-ar.exe")
        foreach ($t in $tools) {
            $src = Join-Path $LlvmBinSrc $t
            if (Test-Path $src) {
                Copy-Item $src "$LlvmDir\bin\$t" -Force
                Write-OK "Copied $t"
            }
        }
    }
}

Write-OK "llc.exe    -> $LlvmDir\bin\llc.exe"
Write-OK "ld.lld.exe -> $LlvmDir\bin\ld.lld.exe"

# ── Step 2: MinGW-w64 sysroot (CRT objects + import libs) ────────────────
# We use WinLibs MinGW-w64 — small, reliable, no installer needed
$CrtObj = Join-Path $SysrootDir "crt2.o"

if (Test-Path $CrtObj) {
    Write-OK "MinGW-w64 sysroot already present, skipping"
} else {
    Write-Info "Downloading MinGW-w64 sysroot..."

    # WinLibs provides a standalone MinGW-w64 zip
    $MingwZip = Join-Path $BuildDir "mingw64.zip"
    $MingwUrl = "https://github.com/brechtsanders/winlibs_mingw/releases/download/13.2.0posix-11.0.1-ucrt-r5/winlibs-x86_64-posix-seh-gcc-13.2.0-llvm-17.0.6-mingw-w64ucrt-11.0.1-r5.zip"

    Download-File $MingwUrl $MingwZip

    Write-Info "Extracting MinGW-w64..."
    $MingwExtract = Join-Path $BuildDir "mingw64-extract"
    New-Item -ItemType Directory -Force -Path $MingwExtract | Out-Null

    $SevenZip = Get-Command "7z" -ErrorAction SilentlyContinue
    if ($SevenZip) {
        & $SevenZip x $MingwZip -o"$MingwExtract" -y | Out-Null
    } else {
        Expand-Archive -Path $MingwZip -DestinationPath $MingwExtract -Force
    }

    # Find the MinGW lib directory
    $MingwLib = Get-ChildItem $MingwExtract -Recurse -Filter "crt2.o" |
                Select-Object -First 1 | Split-Path -Parent
    if (-not $MingwLib) {
        Write-Fail "Could not find crt2.o in extracted MinGW-w64"
    }

    Write-Info "Copying CRT objects and import libs..."

    # CRT startup objects
    $crtFiles = @("crt2.o","crtbegin.o","crtend.o","dllcrt2.o")
    foreach ($f in $crtFiles) {
        $src = Join-Path $MingwLib $f
        if (Test-Path $src) {
            Copy-Item $src $SysrootDir -Force
            Write-OK "Copied $f"
        }
    }

    # Essential import libs for linking
    $libFiles = @(
        "libmingwex.a",
        "libmsvcrt.a",
        "libkernel32.a",
        "libucrt.a",
        "libmingw32.a",
        "libgcc.a",
        "libgcc_eh.a"
    )
    foreach ($f in $libFiles) {
        $src = Join-Path $MingwLib $f
        if (Test-Path $src) {
            Copy-Item $src $SysrootDir -Force
            Write-OK "Copied $f"
        }
    }
}

# ── Write README ──────────────────────────────────────────────────────────
@"
# KonEditor Toolchain (Windows)

Built by bundle-toolchain.ps1. Contains everything needed to compile
KonScript games on Windows without any external toolchain.

## Layout
toolchain\
  llvm\bin\
    llc.exe        - LLVM compiler (.ll -> .o)
    ld.lld.exe     - ELF linker
    lld-link.exe   - COFF/PE linker (Windows native)
    wasm-ld.exe    - WASM linker
  sysroot\windows64\lib\
    crt2.o         - MinGW-w64 CRT startup
    libmingwex.a   - MinGW runtime
    libmsvcrt.a    - MSVC CRT import lib
    libkernel32.a  - Windows kernel import lib

## Rebuilding
  powershell -ExecutionPolicy Bypass -File bundle-toolchain.ps1
"@ | Out-File -FilePath (Join-Path $Prefix "README.md") -Encoding utf8

# ── Done ──────────────────────────────────────────────────────────────────
Write-Host ""
Write-Host "======================================================" -ForegroundColor Green
Write-Host "  Toolchain ready!" -ForegroundColor Green
Write-Host "======================================================"
Write-Host ""
Write-Host "  Now build konscript.exe:"
Write-Host "    .\build.bat"
Write-Host ""
Write-Host "  Test:"
Write-Host "    konscript hello.ks"
Write-Host "    .\hello.exe"
Write-Host ""
