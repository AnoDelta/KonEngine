cmake_minimum_required(VERSION 3.20)
project(KonAkiEngine)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

file(GLOB_RECURSE SRCS src/*.cpp src/*.c)

add_executable(KonEngine ${SRCS})

target_include_directories(KonEngine PRIVATE src src/glad/include)

if(WIN32)
    target_include_directories(KonEngine PRIVATE libs/windows/glfw/include src src/glad/include)
    target_link_directories(KonEngine PRIVATE libs/windows/glfw/lib-mingw-w64)
    target_link_libraries(KonEngine glfw3 opengl32 gdi32 winmm)
else()
    target_link_libraries(KonEngine glfw GL dl pthread X11 Xrandr Xi)
endif()
