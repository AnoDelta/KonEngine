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
)

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
