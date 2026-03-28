cmake_minimum_required(VERSION 3.20)
project(KonAkiEngine VERSION 0.1.0)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Disable unnecessary GLFW targets
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

# Add the GLFW source tree
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/libs/glfw)

# Add GLM (header-only library)
add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/libs/glm)

# Collect engine sources
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
)

# anim_compiler.cpp is a standalone tool, not part of the engine library
list(FILTER LIB_SRCS EXCLUDE REGEX ".*anim_compiler\\.cpp$")

# Create the KonEngine static library
add_library(KonEngine STATIC ${LIB_SRCS})
target_include_directories(KonEngine PUBLIC src src/glad/include src/stb)

# Link KonEngine against GLFW and GLM
target_link_libraries(KonEngine PUBLIC glfw glm::glm)

# Link system libraries depending on platform
if(WIN32)
    target_link_libraries(KonEngine PRIVATE opengl32 gdi32 winmm)
else()
    target_link_libraries(KonEngine PRIVATE GL dl pthread X11 Xrandr Xi)
endif()

# Install targets and headers
install(TARGETS KonEngine ARCHIVE DESTINATION lib)
install(FILES src/KonEngine.hpp DESTINATION include)
install(DIRECTORY src/window src/renderer src/time src/camera src/collision
        DESTINATION include/KonEngine
        FILES_MATCHING PATTERN "*.hpp"
)

# Small test executable
add_executable(KonTest ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)
target_link_libraries(KonTest PRIVATE KonEngine)
set_target_properties(KonTest PROPERTIES EXCLUDE_FROM_ALL TRUE)

# -----------------------------------------------------------------------
# KonEngine test suite
# cmake -DKON_BUILD_TESTS=ON .. && cmake --build . --target KonEngineTest
# -----------------------------------------------------------------------
option(KON_BUILD_TESTS "Build the KonEngine test suite" OFF)
if(KON_BUILD_TESTS)
    add_subdirectory(tests)
endif()

# -----------------------------------------------------------------------
# Qt tools — anim_compiler and KonAnimator
# These are NOT built by default. Use the dedicated scripts:
#   Linux:   ./build-tools.sh
#   Windows: build-tools.bat
# Or build manually:
#   cmake --build build --target anim_compiler
#   cmake --build build --target KonAnimator
# -----------------------------------------------------------------------
option(KON_BUILD_TOOLS "Build KonAnimator and anim_compiler Qt tools" OFF)

if(KON_BUILD_TOOLS)
    find_package(Qt5 QUIET COMPONENTS Core Gui Widgets OpenGL)

    if(Qt5_FOUND)
        set(CMAKE_AUTOMOC ON)

        # anim_compiler
        if(WIN32)
            add_executable(anim_compiler WIN32
                ${CMAKE_CURRENT_SOURCE_DIR}/src/animation/anim_compiler.cpp)
        else()
            add_executable(anim_compiler
                ${CMAKE_CURRENT_SOURCE_DIR}/src/animation/anim_compiler.cpp)
        endif()
        target_include_directories(anim_compiler PRIVATE src)
        target_link_libraries(anim_compiler PRIVATE Qt5::Core Qt5::Gui Qt5::Widgets)

        # KonAnimator
        add_subdirectory(
            ${CMAKE_CURRENT_SOURCE_DIR}/tools/KonAnimator
            ${CMAKE_CURRENT_BINARY_DIR}/tools/KonAnimator)

        # Windows: copy Qt DLLs next to anim_compiler
        # (KonAnimator handles its own DLL copy in tools/KonAnimator/CMakeLists.txt)
        if(WIN32 AND NOT CMAKE_CROSSCOMPILING)
            add_custom_command(TARGET anim_compiler POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    $<TARGET_FILE:Qt5::Core>
                    $<TARGET_FILE:Qt5::Gui>
                    $<TARGET_FILE:Qt5::Widgets>
                    $<TARGET_FILE_DIR:anim_compiler>
                COMMENT "Copying Qt DLLs for anim_compiler...")
        endif()

        message(STATUS "KonAnimator and anim_compiler targets enabled")
    else()
        message(WARNING "KON_BUILD_TOOLS=ON but Qt5 not found — tools skipped")
    endif()
endif()
