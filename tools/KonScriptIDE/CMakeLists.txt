# tools/KonScriptIDE/CMakeLists.txt
# Works two ways:
#   1. Standalone:   cd tools/KonScriptIDE && mkdir build && cmake .. && make
#   2. Subdirectory: add_subdirectory(tools/KonScriptIDE) from root

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    cmake_minimum_required(VERSION 3.16)
    project(KonScriptIDE VERSION 0.1.0 LANGUAGES CXX)
    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTORCC ON)
    find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets)
endif()

# -----------------------------------------------------------------------
# KonScriptIDE library target
# The editor widget is compiled as a library so KonEditor can embed it
# without also pulling in the standalone main().
# -----------------------------------------------------------------------
set(KONSCRIPTIDE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/KonScriptEditor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/KonScriptIDE.cpp
)

set(KONSCRIPTIDE_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/KonScriptHighlighter.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/KonScriptEditor.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/KonScriptIDE.hpp
)

add_library(KonScriptIDELib STATIC
    ${KONSCRIPTIDE_SOURCES}
    ${KONSCRIPTIDE_HEADERS}
)

target_include_directories(KonScriptIDELib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(KonScriptIDELib PUBLIC
    Qt5::Core
    Qt5::Gui
    Qt5::Widgets
)

# -----------------------------------------------------------------------
# Standalone KonScriptIDE executable
# -----------------------------------------------------------------------
if(WIN32)
    add_executable(KonScriptIDE WIN32
        ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
    )
else()
    add_executable(KonScriptIDE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
    )
endif()

target_link_libraries(KonScriptIDE PRIVATE KonScriptIDELib)

# Windows: copy Qt DLLs next to executable
if(WIN32 AND NOT CMAKE_CROSSCOMPILING)
    add_custom_command(TARGET KonScriptIDE POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:Qt5::Core>
            $<TARGET_FILE:Qt5::Gui>
            $<TARGET_FILE:Qt5::Widgets>
            $<TARGET_FILE_DIR:KonScriptIDE>
        COMMENT "Copying Qt DLLs...")
endif()

install(TARGETS KonScriptIDE DESTINATION bin)

message(STATUS "KonScriptIDE: configured")
