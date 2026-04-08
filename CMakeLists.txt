cmake_minimum_required(VERSION 3.20)

project(KonEngine VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Disable unnecessary GLFW targets
set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/libs/glfw)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/libs/glm)

# -----------------------------------------------------------------------
# Engine library sources
# -----------------------------------------------------------------------
file(GLOB_RECURSE LIB_SRCS
    src/window/*.cpp
    src/renderer/*.cpp
    src/renderer/opengl/*.cpp
    src/time/*.cpp
    src/glad/src/*.c
    src/input/*.cpp
    src/font/*.cpp
    src/audio/*.cpp
    src/collision/*.cpp
    src/animation/*.cpp
    src/node/*.cpp
    src/ui/*.cpp
)

# anim_compiler.cpp and main.cpp are standalone tools, not part of the library
list(FILTER LIB_SRCS EXCLUDE REGEX ".*anim_compiler\\.cpp$")
list(FILTER LIB_SRCS EXCLUDE REGEX ".*/src/main\\.cpp$")

# -----------------------------------------------------------------------
# Generate embedded logo header at build time
# -----------------------------------------------------------------------
set(KON_LOGO_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/src/window/kon_logo.h")
find_package(Python3 QUIET COMPONENTS Interpreter)
if(Python3_FOUND)
    add_custom_command(
        OUTPUT  "${KON_LOGO_HEADER}"
        COMMAND ${Python3_EXECUTABLE}
                "${CMAKE_CURRENT_SOURCE_DIR}/tools/gen_kon_logo.py"
                "${KON_LOGO_HEADER}"
                "${CMAKE_CURRENT_SOURCE_DIR}/logo.png"
        DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/tools/gen_kon_logo.py"
        COMMENT "Generating embedded KonEngine logo"
    )
else()
    # Fallback: generate a minimal stub if Python isn't available
    if(NOT EXISTS "${KON_LOGO_HEADER}")
        file(WRITE "${KON_LOGO_HEADER}"
            "#pragma once\n"
            "static const unsigned int KON_LOGO_WIDTH = 1;\n"
            "static const unsigned int KON_LOGO_HEIGHT = 1;\n"
            "static const unsigned char KON_LOGO_DATA[4] = {20,30,60,255};\n"
        )
    endif()
endif()

add_library(KonEngine STATIC
    ${LIB_SRCS}
    ${CMAKE_CURRENT_SOURCE_DIR}/src/asset_manager.cpp
    ${KON_LOGO_HEADER}
)

target_include_directories(KonEngine PUBLIC src src/glad/include src/stb)
target_link_libraries(KonEngine PUBLIC glfw glm::glm)

if(WIN32)
    target_link_libraries(KonEngine PRIVATE opengl32 gdi32 winmm)
else()
    target_link_libraries(KonEngine PRIVATE GL dl pthread X11 Xrandr Xi)
endif()

# -----------------------------------------------------------------------
# Optional .konpak support
# Bake a key into the binary so the engine can decrypt assets at runtime.
# End users cannot extract the pack — the key is only in the compiled exe.
#
# Enable in your game's CMakeLists.txt:
#   target_compile_definitions(MyGame PRIVATE KON_PACK_KEY="mygamekey")
#
# Or pass at cmake time to enable engine-wide:
#   cmake -B build -DKON_PACK_KEY="mygamekey"
#
# Then in main():
#   AssetManager::init("game.konpak"); // release
#   AssetManager::init("assets/");    // dev
# -----------------------------------------------------------------------
option(KON_PACK_SUPPORT "Build with .konpak asset encryption support" OFF)

if(KON_PACK_SUPPORT OR DEFINED KON_PACK_KEY)
    if(WIN32)
        if(CMAKE_CROSSCOMPILING)
            # Cross-compile (MXE): bcrypt is always available.
            # For zlib/compression: use bundled miniz if present, otherwise
            # build without compression (store-only mode).
            # Check for miniz in KonPaktor third_party or engine src
            set(MINIZ_DIR "${CMAKE_CURRENT_SOURCE_DIR}/tools/KonPaktor/third_party")
            if(NOT EXISTS "${MINIZ_DIR}/miniz.h")
                set(MINIZ_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
            endif()
            if(EXISTS "${MINIZ_DIR}/miniz.h")
                file(SIZE "${MINIZ_DIR}/miniz.h" MINIZ_SIZE)
                if(MINIZ_SIZE GREATER 10000)
                    target_include_directories(KonEngine PUBLIC "${MINIZ_DIR}")
                    target_compile_definitions(KonEngine PUBLIC KONPAK_USE_MINIZ)
                    message(STATUS "KonPak: using bundled miniz (cross-compile)")
                else()
                    message(STATUS "KonPak: miniz.h invalid (${MINIZ_SIZE} bytes) — store-only mode")
                endif()
            else()
                message(STATUS "KonPak: no miniz.h — store-only mode (compression disabled)")
            endif()

            find_path(BCRYPT_INCLUDE_DIR bcrypt.h
                HINTS ${CMAKE_FIND_ROOT_PATH}/include
                      /usr/lib/mingw64-toolchain/x86_64-w64-mingw32/include
                      ${MXE_ROOT}/usr/x86_64-w64-mingw32.static/include
                NO_DEFAULT_PATH)
            if(BCRYPT_INCLUDE_DIR)
                target_include_directories(KonEngine PRIVATE ${BCRYPT_INCLUDE_DIR})
            endif()
            target_link_libraries(KonEngine PUBLIC bcrypt)
        else()
            # Native Windows build (MSVC or MinGW with vcpkg)
            find_package(ZLIB REQUIRED)
            target_link_libraries(KonEngine PUBLIC bcrypt ZLIB::ZLIB)
        endif()
    else()
        find_package(OpenSSL REQUIRED)
        find_package(ZLIB REQUIRED)
        target_link_libraries(KonEngine PUBLIC OpenSSL::Crypto ZLIB::ZLIB)
    endif()

    target_compile_definitions(KonEngine PUBLIC KON_USE_PACK)
    if(DEFINED KON_PACK_KEY)
        target_compile_definitions(KonEngine PUBLIC KON_PACK_KEY="${KON_PACK_KEY}")
    endif()

    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/KonPaktor/src/konpak.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/konpak.hpp"
        COPYONLY
    )

    message(STATUS "KonPak support: ENABLED")
    if(DEFINED KON_PACK_KEY)
        message(STATUS "KonPak key: baked at compile time")
    endif()
else()
    message(STATUS "KonPak support: disabled (enable with -DKON_PACK_SUPPORT=ON)")
endif()

# Install
install(TARGETS KonEngine ARCHIVE DESTINATION lib)
install(FILES src/KonEngine.hpp src/asset_manager.hpp src/kon_script_api.hpp
    DESTINATION include
)
install(DIRECTORY src/color src/window src/renderer src/time src/camera
                  src/collision src/input src/font src/audio src/animation
                  src/node src/math src/tilemap src/ui
    DESTINATION include
    FILES_MATCHING PATTERN "*.hpp"
)
# glad and stb headers needed by renderer
install(DIRECTORY src/glad/include/glad src/glad/include/KHR
    DESTINATION include
)
install(FILES src/stb/stb_image.h src/stb/stb_truetype.h
    DESTINATION include/stb
)

# -----------------------------------------------------------------------
# KonTest -- small headless test binary (excluded from default build)
# -----------------------------------------------------------------------
add_executable(KonTest ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)
target_link_libraries(KonTest PRIVATE KonEngine)
set_target_properties(KonTest PROPERTIES EXCLUDE_FROM_ALL TRUE)

# -----------------------------------------------------------------------
# KonAnimator + anim_compiler -- Qt tools (optional, -DKON_BUILD_TOOLS=ON)
# -----------------------------------------------------------------------
option(KON_BUILD_TOOLS "Build KonAnimator and anim_compiler Qt tools" OFF)

if(KON_BUILD_TOOLS)
    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTORCC ON)
    find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets OpenGL)

    if(Qt5_FOUND)
        # anim_compiler
        set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libs/imgui)
        set(IMGUI_SRCS
            ${IMGUI_DIR}/imgui.cpp
            ${IMGUI_DIR}/imgui_draw.cpp
            ${IMGUI_DIR}/imgui_tables.cpp
            ${IMGUI_DIR}/imgui_widgets.cpp
            ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
            ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
        )

        add_executable(anim_compiler
            ${CMAKE_CURRENT_SOURCE_DIR}/src/animation/anim_compiler.cpp
            ${IMGUI_SRCS}
        )
        target_include_directories(anim_compiler PRIVATE
            ${IMGUI_DIR}
            ${IMGUI_DIR}/backends
            ${CMAKE_CURRENT_SOURCE_DIR}/libs/glfw/include
            ${CMAKE_CURRENT_SOURCE_DIR}/src/glad/include
        )
        target_link_libraries(anim_compiler PRIVATE glfw Qt5::Core Qt5::Gui Qt5::Widgets Qt5::OpenGL)
        if(WIN32)
            target_link_libraries(anim_compiler PRIVATE opengl32)
        else()
            target_link_libraries(anim_compiler PRIVATE GL)
        endif()

        # KonAnimator
        add_subdirectory(tools/KonAnimator)
        message(STATUS "KonAnimator + anim_compiler: enabled")
    else()
        message(WARNING "KON_BUILD_TOOLS=ON but Qt5 not found -- tools skipped")
    endif()
endif()

# -----------------------------------------------------------------------
# Test suite
# -----------------------------------------------------------------------
add_subdirectory(tests)

# -----------------------------------------------------------------------
# KonScript integration
# Provides konscript_sources() for game projects using KonEngine.
#
# TWO SEPARATE BUILD PATHS:
#
#   1. CMake / submodule path  (this function)
#      Uses --cpp: transpiles .ks → .cpp, then cmake/g++ compiles normally.
#      Works everywhere. No LLVM toolchain needed. Full cross-compile support
#      via cmake's own toolchain mechanism (-DCMAKE_TOOLCHAIN_FILE=...).
#
#   2. KonEditor / native path  (KonBuild.hpp)
#      Uses IRGen + bundled llc/lld/musl. Produces standalone native binaries.
#      Used by the editor's Build workspace. Not involved in CMake builds.
#
# Usage in your game's CMakeLists.txt:
#   add_subdirectory(KonEngine)
#   add_executable(MyGame)
#   target_link_libraries(MyGame PRIVATE KonEngine)
#   konscript_sources(MyGame
#     src/player.ks   # module — no main(), #included by entry
#     src/wall.ks     # module — no main(), #included by entry
#     src/main.ks     # entry  — has main(), linked into the binary
#   )
#
# Cross-compiling (unchanged from before):
#   cmake -B build-win -DCMAKE_TOOLCHAIN_FILE=windows-toolchain.cmake
#   cmake --build build-win
#   konscript_sources() still works — --cpp produces portable C++.
# -----------------------------------------------------------------------

find_program(KONSCRIPT_COMPILER konscript
    HINTS
        ${CMAKE_CURRENT_SOURCE_DIR}/tools/KonScript
        ${CMAKE_CURRENT_BINARY_DIR}/tools/KonScript
        /usr/local/bin
        /usr/bin
)

if(KONSCRIPT_COMPILER)
    message(STATUS "KonScript compiler: ${KONSCRIPT_COMPILER}")
else()
    message(STATUS "KonScript compiler not found -- .ks files will not be compiled")
    message(STATUS "  Install with: cd tools/KonScript && ./build.sh && ./install.sh")
endif()

function(konscript_sources TARGET)
    if(NOT KONSCRIPT_COMPILER)
        message(WARNING "konscript_sources: konscript not found, skipping .ks files for ${TARGET}")
        return()
    endif()

    set(ALL_CPP_FILES "")
    set(ENTRY_CPP_FILES "")

    # Pass 1: register custom commands for all files, categorise entry vs module
    foreach(KS_FILE ${ARGN})
        if(NOT IS_ABSOLUTE "${KS_FILE}")
            set(KS_FILE "${CMAKE_CURRENT_SOURCE_DIR}/${KS_FILE}")
        endif()

        get_filename_component(KS_DIR  "${KS_FILE}" DIRECTORY)
        get_filename_component(KS_NAME "${KS_FILE}" NAME_WE)
        set(CPP_FILE "${KS_DIR}/${KS_NAME}.ks.cpp")

        # IMPORTANT: always pass --cpp so konscript uses the C++ transpiler
        # backend regardless of what the default mode is. This preserves
        # backwards compatibility — submodule users get C++ output that their
        # existing C++ toolchain can compile, with no dependency on the
        # bundled LLVM toolchain.
        add_custom_command(
            OUTPUT  "${CPP_FILE}"
            COMMAND "${KONSCRIPT_COMPILER}" --cpp "${KS_FILE}" -o "${CPP_FILE}"
            DEPENDS "${KS_FILE}" "${KONSCRIPT_COMPILER}"
            COMMENT "Compiling KonScript: ${KS_NAME}.ks → ${KS_NAME}.ks.cpp"
            VERBATIM
        )

        list(APPEND ALL_CPP_FILES "${CPP_FILE}")

        # Detect entry file by checking for func main() in source
        file(READ "${KS_FILE}" _KS_CONTENT)
        string(FIND "${_KS_CONTENT}" "func main()" _HAS_MAIN)
        if(_HAS_MAIN GREATER_EQUAL 0)
            list(APPEND ENTRY_CPP_FILES "${CPP_FILE}")
        endif()
    endforeach()

    # Ensure ALL .ks.cpp files are generated before the target compiles.
    # Modules must exist on disk before the entry file can #include them.
    add_custom_target(${TARGET}_ks_modules DEPENDS ${ALL_CPP_FILES})
    add_dependencies(${TARGET} ${TARGET}_ks_modules)

    # Only add entry files as compiled sources — modules are #included inside
    # the entry file's generated .cpp, not compiled as separate translation units.
    foreach(CPP_FILE ${ENTRY_CPP_FILES})
        target_sources(${TARGET} PRIVATE "${CPP_FILE}")
    endforeach()
endfunction()
