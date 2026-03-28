# Cross-compilation toolchain: Linux → Windows (x86_64)
# Uses MXE (M cross environment) mingw-w64 toolchain
# MXE prefix is auto-detected but can be overridden:
#   cmake -DMXE_ROOT=/path/to/mxe ...

set(CMAKE_SYSTEM_NAME    Windows)
set(CMAKE_SYSTEM_VERSION 10)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# ---- Locate MXE ----
if(NOT DEFINED MXE_ROOT)
    foreach(_candidate
        /usr/lib/mxe
        /opt/mxe
        $ENV{HOME}/mxe
        $ENV{MXE_ROOT}
    )
        if(EXISTS "${_candidate}/usr/bin/x86_64-w64-mingw32.static-gcc")
            set(MXE_ROOT "${_candidate}")
            break()
        endif()
    endforeach()
endif()

if(NOT EXISTS "${MXE_ROOT}")
    message(FATAL_ERROR
        "MXE not found. Install it (see build-windows.sh) or set -DMXE_ROOT=/path/to/mxe")
endif()

set(MXE_TARGET  "x86_64-w64-mingw32.static")
set(MXE_USR     "${MXE_ROOT}/usr")
set(MXE_BIN     "${MXE_USR}/bin")
set(MXE_PREFIX  "${MXE_USR}/${MXE_TARGET}")

message(STATUS "MXE root   : ${MXE_ROOT}")
message(STATUS "MXE target : ${MXE_TARGET}")

# ---- Compilers ----
set(CMAKE_C_COMPILER   "${MXE_BIN}/${MXE_TARGET}-gcc")
set(CMAKE_CXX_COMPILER "${MXE_BIN}/${MXE_TARGET}-g++")
set(CMAKE_RC_COMPILER  "${MXE_BIN}/${MXE_TARGET}-windres")

# ---- Find mode ----
set(CMAKE_FIND_ROOT_PATH "${MXE_PREFIX}")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# ---- Qt5 via MXE ----
set(Qt5_DIR "${MXE_PREFIX}/qt5/lib/cmake/Qt5")
set(QT_HOST_PATH "${MXE_PREFIX}/qt5")
