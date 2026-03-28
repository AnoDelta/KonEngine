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
# anim_compiler — standalone GUI tool
# -----------------------------------------------------------------------
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
target_link_libraries(anim_compiler PRIVATE glfw)
if(WIN32)
    target_link_libraries(anim_compiler PRIVATE opengl32)
else()
    target_link_libraries(anim_compiler PRIVATE GL)
endif()
set_target_properties(anim_compiler PROPERTIES EXCLUDE_FROM_ALL TRUE)
# -----------------------------------------------------------------------
# KonScript integration
# Provides konscript_sources() for game projects using KonEngine.
#
# Usage in your game's CMakeLists.txt:
#   konscript_sources(MyGame src/Player.ks src/Enemy.ks)
#
# This compiles each .ks file to .cpp at build time and adds it to
# the target's sources automatically.
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
    message(STATUS "  Install with: cd tools/KonScript && ./install.sh")
endif()

function(konscript_sources TARGET)
    if(NOT KONSCRIPT_COMPILER)
        message(WARNING "konscript_sources: konscript not found, skipping .ks files for ${TARGET}")
        return()
    endif()

    foreach(KS_FILE ${ARGN})
        # Get absolute path
        if(NOT IS_ABSOLUTE "${KS_FILE}")
            set(KS_FILE "${CMAKE_CURRENT_SOURCE_DIR}/${KS_FILE}")
        endif()

        # Output .cpp alongside the .ks file
        get_filename_component(KS_DIR  "${KS_FILE}" DIRECTORY)
        get_filename_component(KS_NAME "${KS_FILE}" NAME_WE)
        set(CPP_FILE "${KS_DIR}/${KS_NAME}.ks.cpp")

        # Custom command: run konscript to compile .ks -> .cpp
        add_custom_command(
            OUTPUT  "${CPP_FILE}"
            COMMAND "${KONSCRIPT_COMPILER}" "${KS_FILE}" -o "${CPP_FILE}"
            DEPENDS "${KS_FILE}" "${KONSCRIPT_COMPILER}"
            COMMENT "Compiling KonScript: ${KS_NAME}.ks"
            VERBATIM
        )

        # Add the generated .cpp to the target
        target_sources(${TARGET} PRIVATE "${CPP_FILE}")
    endforeach()
endfunction()
