cmake_minimum_required(VERSION 3.16)
project(KonEngineExample LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)

# Option 1: Engine as subdirectory (if cloned)
# add_subdirectory(../../ KonEngine)
# target_link_libraries(game PRIVATE KonEngine)

# Option 2: Pre-built engine library
find_package(PkgConfig QUIET)

add_executable(game main.cpp)
target_include_directories(game PRIVATE /usr/local/include)
target_link_libraries(game PRIVATE
    /usr/local/lib/libKonEngine.a
    /usr/local/lib/libglfw3.a
    GL X11 Xrandr Xi dl pthread m
)
