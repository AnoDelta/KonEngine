# tools/KonEditor/CMakeLists.txt
cmake_minimum_required(VERSION 3.16)
project(KonEditor VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets OpenGL)

set(KONSCRIPTIDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../KonScriptIDE/src)
set(KONANIMATOR_DIR  ${CMAKE_CURRENT_SOURCE_DIR}/../KonAnimator/src)
set(STB_DIR          ${CMAKE_CURRENT_SOURCE_DIR}/../../src/stb)

message(STATUS "KonEditor: configured")
message(STATUS "KonScriptIDE dir: ${KONSCRIPTIDE_DIR}")
message(STATUS "KonAnimator dir:  ${KONANIMATOR_DIR}")

set(SOURCES
    src/main.cpp
    src/KonEditor.cpp
    src/ProjectManager.cpp
    src/SceneTree.cpp
    src/SceneFormat.cpp
    src/ScriptDiffDialog.cpp
    src/Inspector.cpp
    src/Viewport.cpp
    src/ScriptEditor.cpp
    src/AssetBrowser.cpp
    src/BuildPanel.cpp
    src/DebugConsole.cpp
    src/WelcomeScreen.cpp
    src/AnimatorPanel.cpp
    src/AssetPackPanel.cpp
    ${KONSCRIPTIDE_DIR}/KonScriptEditor.cpp
    ${KONSCRIPTIDE_DIR}/KonScriptHighlighter.hpp
    ${KONANIMATOR_DIR}/PreviewWidget.cpp
    ${KONANIMATOR_DIR}/SpritesheetView.cpp
    ${KONANIMATOR_DIR}/TimelineWidget.cpp
)

if(WIN32)
    add_executable(KonEditor WIN32 ${SOURCES})
else()
    add_executable(KonEditor ${SOURCES})
endif()

target_include_directories(KonEditor PRIVATE
    src
    ${KONSCRIPTIDE_DIR}
    ${KONANIMATOR_DIR}
    ${STB_DIR}
)

target_link_libraries(KonEditor PRIVATE
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
    Qt5::OpenGL
)

add_custom_command(TARGET KonEditor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "=== Done! ==="
    COMMAND ${CMAKE_COMMAND} -E echo "  ./build/KonEditor"
)
