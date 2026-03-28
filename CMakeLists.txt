cmake_minimum_required(VERSION 3.20)
project(KonEngine VERSION 0.1.0)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Disable unnecessary GLFW targets
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
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
)

# anim_compiler.cpp and main.cpp are standalone tools, not part of the library
list(FILTER LIB_SRCS EXCLUDE REGEX ".*anim_compiler\\.cpp$")
list(FILTER LIB_SRCS EXCLUDE REGEX ".*/src/main\\.cpp$")

add_library(KonEngine STATIC ${LIB_SRCS})
target_include_directories(KonEngine PUBLIC src src/glad/include src/stb)
target_link_libraries(KonEngine PUBLIC glfw glm::glm)

if(WIN32)
    target_link_libraries(KonEngine PRIVATE opengl32 gdi32 winmm)
else()
    target_link_libraries(KonEngine PRIVATE GL dl pthread X11 Xrandr Xi)
endif()

# Install
install(TARGETS KonEngine ARCHIVE DESTINATION lib)
install(FILES src/KonEngine.hpp DESTINATION include)
install(DIRECTORY src/window src/renderer src/time src/camera src/collision
                  src/input src/font src/audio src/animation src/node
        DESTINATION include/KonEngine
        FILES_MATCHING PATTERN "*.hpp"
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
    find_package(Qt5 QUIET COMPONENTS Core Gui Widgets OpenGL)
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
# Usage in your game's CMakeLists.txt:
#   add_subdirectory(KonEngine)
#   add_executable(MyGame)
#   target_link_libraries(MyGame PRIVATE KonEngine)
#   konscript_sources(MyGame src/main.ks)
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

    foreach(KS_FILE ${ARGN})
        if(NOT IS_ABSOLUTE "${KS_FILE}")
            set(KS_FILE "${CMAKE_CURRENT_SOURCE_DIR}/${KS_FILE}")
        endif()

        get_filename_component(KS_DIR  "${KS_FILE}" DIRECTORY)
        get_filename_component(KS_NAME "${KS_FILE}" NAME_WE)
        set(CPP_FILE "${KS_DIR}/${KS_NAME}.ks.cpp")

        add_custom_command(
            OUTPUT  "${CPP_FILE}"
            COMMAND "${KONSCRIPT_COMPILER}" "${KS_FILE}" -o "${CPP_FILE}"
            DEPENDS "${KS_FILE}" "${KONSCRIPT_COMPILER}"
            COMMENT "Compiling KonScript: ${KS_NAME}.ks"
            VERBATIM
        )

        target_sources(${TARGET} PRIVATE "${CPP_FILE}")
    endforeach()
endfunction()
