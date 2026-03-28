# tools/KonAnimator/CMakeLists.txt
# Works two ways:
#   1. Standalone:   cd tools/KonAnimator && ./build.sh
#   2. Subdirectory: included by the root KonEngine CMakeLists via add_subdirectory

# Only set up project/Qt when built standalone (not pulled in by parent)
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    cmake_minimum_required(VERSION 3.16)
    project(KonAnimator VERSION 0.1.0 LANGUAGES CXX)
    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTORCC ON)
    find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets OpenGL)
    find_package(OpenGL REQUIRED)
endif()

# -----------------------------------------------------------------------
# stb_image — use engine copy if present, fall back to local third_party/
# -----------------------------------------------------------------------
set(ENGINE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../..")
set(STB_INCLUDE  "${ENGINE_ROOT}/src/stb")

if(NOT EXISTS "${STB_INCLUDE}/stb_image.h")
    set(STB_INCLUDE "${CMAKE_CURRENT_SOURCE_DIR}/third_party/stb")
    message(STATUS "KonAnimator: stb not found in engine tree, using ${STB_INCLUDE}")
else()
    message(STATUS "KonAnimator: using engine stb at ${STB_INCLUDE}")
endif()

# -----------------------------------------------------------------------
# Sources
# -----------------------------------------------------------------------
set(KONANIMATOR_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/MainWindow.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/SpritesheetView.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/TimelineWidget.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/PreviewWidget.cpp
)

if(WIN32)
    add_executable(KonAnimator WIN32 ${KONANIMATOR_SOURCES})
else()
    add_executable(KonAnimator ${KONANIMATOR_SOURCES})
endif()

target_include_directories(KonAnimator PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${STB_INCLUDE}
)

target_link_libraries(KonAnimator PRIVATE
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
    Qt5::OpenGL
)

if(WIN32)
    target_link_libraries(KonAnimator PRIVATE opengl32 gdi32 winmm)
else()
    find_package(OpenGL QUIET)
    target_link_libraries(KonAnimator PRIVATE OpenGL::GL)
endif()

# Windows: copy Qt DLLs next to KonAnimator (both standalone and subdirectory builds)
if(WIN32 AND NOT CMAKE_CROSSCOMPILING)
    add_custom_command(TARGET KonAnimator POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:Qt5::Core>
            $<TARGET_FILE:Qt5::Gui>
            $<TARGET_FILE:Qt5::Widgets>
            $<TARGET_FILE:Qt5::OpenGL>
            $<TARGET_FILE_DIR:KonAnimator>
        COMMENT "Copying Qt DLLs...")
endif()

install(TARGETS KonAnimator DESTINATION bin)
