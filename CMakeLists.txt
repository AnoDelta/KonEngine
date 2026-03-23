cmake_minimum_required(VERSION 3.20)
project(KonAkiEngine)

set(CMAKE_CXX_STANDARD 17)

file(GLOB_RECURSE SRCS src/*.cpp)

add_executable(KonEngine ${SRCS})

target_include_directories(KonEngine PRIVATE src)

target_link_libraries(KonEngine glfw GL dl pthread X11 Xrandr Xi)
