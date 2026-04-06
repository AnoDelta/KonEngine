set(CMAKE_SYSTEM_NAME Windows)

# Use MXE compilers when MXE_ROOT is provided, otherwise fall back to system MinGW
if(DEFINED MXE_ROOT AND EXISTS "${MXE_ROOT}/usr/bin/x86_64-w64-mingw32.static-gcc")
    set(CMAKE_C_COMPILER   "${MXE_ROOT}/usr/bin/x86_64-w64-mingw32.static-gcc")
    set(CMAKE_CXX_COMPILER "${MXE_ROOT}/usr/bin/x86_64-w64-mingw32.static-g++")
    set(CMAKE_RC_COMPILER  "${MXE_ROOT}/usr/bin/x86_64-w64-mingw32.static-windres")
    set(CMAKE_FIND_ROOT_PATH "${MXE_ROOT}/usr/x86_64-w64-mingw32.static")
else()
    set(CMAKE_C_COMPILER   x86_64-w64-mingw32-gcc)
    set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++)
    set(CMAKE_RC_COMPILER  x86_64-w64-mingw32-windres)
    set(CMAKE_FIND_ROOT_PATH /usr/x86_64-w64-mingw32)
endif()

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
