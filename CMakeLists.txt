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