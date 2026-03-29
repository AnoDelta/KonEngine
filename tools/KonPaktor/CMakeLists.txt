cmake_minimum_required(VERSION 3.16)
project(KonPaktor VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

# -----------------------------------------------------------------------
# Dependencies
# -----------------------------------------------------------------------
find_package(Qt5 REQUIRED COMPONENTS Core Gui Widgets Multimedia)

# miniz (single header, bundled)
set(MINIZ_INCLUDE "${CMAKE_CURRENT_SOURCE_DIR}/third_party")

# Zlib — prefer static on Windows so no zlib1.dll is needed at runtime
if(WIN32)
    set(ZLIB_USE_STATIC_LIBS ON)
endif()
find_package(ZLIB REQUIRED)

# Crypto
if(WIN32)
    set(CRYPTO_LIBS bcrypt)
else()
    find_package(OpenSSL REQUIRED)
    set(CRYPTO_LIBS OpenSSL::Crypto)
endif()

# -----------------------------------------------------------------------
# CLI tool
# -----------------------------------------------------------------------
add_executable(konpak src/konpak_cli.cpp)
target_include_directories(konpak PRIVATE src ${MINIZ_INCLUDE})
target_link_libraries(konpak PRIVATE ZLIB::ZLIB ${CRYPTO_LIBS})
if(NOT WIN32)
    target_link_libraries(konpak PRIVATE stdc++fs)
endif()

# -----------------------------------------------------------------------
# Qt GUI tool
# -----------------------------------------------------------------------
if(WIN32)
    add_executable(KonPaktor WIN32 src/main.cpp src/MainWindow.cpp)
else()
    add_executable(KonPaktor src/main.cpp src/MainWindow.cpp)
endif()

target_include_directories(KonPaktor PRIVATE src ${MINIZ_INCLUDE})
target_link_libraries(KonPaktor PRIVATE
    Qt5::Core Qt5::Gui Qt5::Widgets Qt5::Multimedia
    ZLIB::ZLIB ${CRYPTO_LIBS})
if(NOT WIN32)
    target_link_libraries(KonPaktor PRIVATE stdc++fs)
endif()

# -----------------------------------------------------------------------
# Windows: copy Qt DLLs next to binaries
# -----------------------------------------------------------------------
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
