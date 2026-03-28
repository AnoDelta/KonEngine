cmake_minimum_required(VERSION 3.16)
project(KonPaktor VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets Multimedia)

# zlib -- system on Linux, bundled with Qt on Windows
if(WIN32)
    # Qt5 ships zlib -- find it in the Qt prefix
    find_path(ZLIB_INCLUDE_DIR zlib.h
        HINTS "${Qt5Core_DIR}/../../../include"
              "${Qt5Core_DIR}/../../../../include"
              "C:/Qt/5.15.2/msvc2019_64/include")
    find_library(ZLIB_LIBRARY
        NAMES zlib zlib1 zlibstatic
        HINTS "${Qt5Core_DIR}/../../../lib"
              "${Qt5Core_DIR}/../../../../lib"
              "C:/Qt/5.15.2/msvc2019_64/lib")
    if(ZLIB_INCLUDE_DIR AND ZLIB_LIBRARY)
        message(STATUS "KonPaktor: zlib found at ${ZLIB_LIBRARY}")
        set(ZLIB_LIBS ${ZLIB_LIBRARY})
        set(ZLIB_INCLUDES ${ZLIB_INCLUDE_DIR})
    else()
        message(STATUS "KonPaktor: zlib not found via Qt, trying find_package")
        find_package(ZLIB REQUIRED)
        set(ZLIB_LIBS ZLIB::ZLIB)
        set(ZLIB_INCLUDES "")
    endif()
else()
    find_package(ZLIB REQUIRED)
    set(ZLIB_LIBS ZLIB::ZLIB)
    set(ZLIB_INCLUDES "")
endif()

# Crypto
if(WIN32)
    set(CRYPTO_LIBS bcrypt)
else()
    find_package(OpenSSL REQUIRED)
    set(CRYPTO_LIBS OpenSSL::Crypto)
endif()

# CLI tool
add_executable(konpak src/konpak_cli.cpp)
target_include_directories(konpak PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src" ${ZLIB_INCLUDES})
target_link_libraries(konpak PRIVATE ${CRYPTO_LIBS} ${ZLIB_LIBS})
if(NOT WIN32)
    target_link_libraries(konpak PRIVATE stdc++fs)
endif()

# Qt GUI tool
if(WIN32)
    add_executable(KonPaktor WIN32 src/main.cpp src/MainWindow.cpp)
else()
    add_executable(KonPaktor src/main.cpp src/MainWindow.cpp)
endif()

target_include_directories(KonPaktor PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/src" ${ZLIB_INCLUDES})
target_link_libraries(KonPaktor PRIVATE
    Qt5::Core Qt5::Gui Qt5::Widgets Qt5::Multimedia
    ${CRYPTO_LIBS} ${ZLIB_LIBS})
if(NOT WIN32)
    target_link_libraries(KonPaktor PRIVATE stdc++fs)
endif()

# Windows: copy Qt DLLs
if(WIN32 AND NOT CMAKE_CROSSCOMPILING)
    foreach(_tool konpak KonPaktor)
        add_custom_command(TARGET ${_tool} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_FILE:Qt5::Core>
                $<TARGET_FILE:Qt5::Gui>
                $<TARGET_FILE:Qt5::Widgets>
                $<TARGET_FILE_DIR:${_tool}>)
    endforeach()
endif()

install(TARGETS konpak KonPaktor DESTINATION bin)
