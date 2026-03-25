cmake_minimum_required(VERSION 3.20)
project(KonAkiEngine)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Library sources
file(GLOB_RECURSE LIB_SRCS
    src/window/*.cpp
    src/renderer/*.cpp
    src/renderer/opengl/*.cpp
    src/time/*.cpp
    src/glad/src/*.c
)

# The actual library
add_library(KonEngine STATIC ${LIB_SRCS})

target_include_directories(KonEngine PUBLIC src src/glad/include)

if(WIN32)
    target_include_directories(KonEngine PUBLIC libs/windows/glfw/include)
    if(MSVC)
        target_link_directories(KonEngine PUBLIC libs/windows/glfw/lib-vc2022)
    else()
        target_link_directories(KonEngine PUBLIC libs/windows/glfw/lib-mingw-w64)
    endif()
    target_link_libraries(KonEngine glfw3 opengl32 gdi32 winmm)
else()
    target_link_libraries(KonEngine glfw GL dl pthread X11 Xrandr Xi)
endif()

install(TARGETS KonEngine
    ARCHIVE DESTINATION lib
)

install(FILES src/KonEngine.hpp DESTINATION include/KonEngine)
install(DIRECTORY src/window src/renderer src/time
    DESTINATION include/KonEngine
    FILES_MATCHING PATTERN "*.hpp"
)

# Test executable
add_executable(KonEngineTest ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)
target_link_libraries(KonEngineTest KonEngine)
set_target_properties(KonEngineTest PROPERTIES EXCLUDE_FROM_ALL TRUE)
