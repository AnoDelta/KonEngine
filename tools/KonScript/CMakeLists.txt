cmake_minimum_required(VERSION 3.16)
project(KonScript VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(KonScript INTERFACE)
target_include_directories(KonScript INTERFACE include)

add_executable(konscript src/main.cpp)
target_link_libraries(konscript PRIVATE KonScript)

option(KONSCRIPT_BUILD_TESTS "Build KonScript tests" OFF)
if(KONSCRIPT_BUILD_TESTS)
    add_executable(test_lexer  tests/test_lexer.cpp)
    add_executable(test_parser tests/test_parser.cpp)
    target_link_libraries(test_lexer  PRIVATE KonScript)
    target_link_libraries(test_parser PRIVATE KonScript)
endif()
