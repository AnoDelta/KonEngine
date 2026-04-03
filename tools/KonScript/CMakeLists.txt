cmake_minimum_required(VERSION 3.16)
project(KonScript VERSION 0.1.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── Options ──────────────────────────────────────────────────────────────
option(KONSCRIPT_BUILD_TESTS    "Build KonScript unit tests"           OFF)
option(KONSCRIPT_BUILD_RUNTIME  "Compile the KonScript C runtime"      ON)
option(KONSCRIPT_BAKE_TOOLCHAIN "Bake toolchain path into the binary"  ON)

# ── Header-only library (parser, codegen, typechecker, irgen) ────────────
add_library(KonScript INTERFACE)
target_include_directories(KonScript INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# ── Toolchain discovery ──────────────────────────────────────────────────
# Check for bundled toolchain alongside the source tree
set(KONSCRIPT_TOOLCHAIN_DIR "" CACHE PATH "Path to KonScript toolchain (llvm/bin, sysroot/)")

if(KONSCRIPT_BAKE_TOOLCHAIN AND NOT KONSCRIPT_TOOLCHAIN_DIR)
    # Auto-detect: check if toolchain/ exists next to this CMakeLists.txt
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/toolchain/llvm/bin")
        set(KONSCRIPT_TOOLCHAIN_DIR "${CMAKE_CURRENT_SOURCE_DIR}/toolchain")
        message(STATUS "KonScript toolchain found: ${KONSCRIPT_TOOLCHAIN_DIR}")
    elseif(DEFINED ENV{KONSCRIPT_TOOLCHAIN} AND EXISTS "$ENV{KONSCRIPT_TOOLCHAIN}/llvm/bin")
        set(KONSCRIPT_TOOLCHAIN_DIR "$ENV{KONSCRIPT_TOOLCHAIN}")
        message(STATUS "KonScript toolchain from env: ${KONSCRIPT_TOOLCHAIN_DIR}")
    else()
        message(STATUS "KonScript toolchain not found — binary will need KONSCRIPT_TOOLCHAIN env var")
        message(STATUS "  Run ./bundle-toolchain.sh to create it, then reconfigure")
    endif()
endif()

# ── konscript compiler binary ────────────────────────────────────────────
add_executable(konscript src/main.cpp)
target_link_libraries(konscript PRIVATE KonScript)

# Bake toolchain path if available
if(KONSCRIPT_TOOLCHAIN_DIR)
    target_compile_definitions(konscript PRIVATE
        KONSCRIPT_TOOLCHAIN_BUILTIN="${KONSCRIPT_TOOLCHAIN_DIR}"
    )
    message(STATUS "Toolchain path baked into binary: ${KONSCRIPT_TOOLCHAIN_DIR}")
endif()

# ── KonScript C runtime (_ks_runtime.o) ──────────────────────────────────
if(KONSCRIPT_BUILD_RUNTIME)
    add_library(ks_runtime STATIC _ks_runtime.c)
    set_target_properties(ks_runtime PROPERTIES
        OUTPUT_NAME "_ks_runtime"
        POSITION_INDEPENDENT_CODE ON
    )
endif()

# ── Tests ────────────────────────────────────────────────────────────────
if(KONSCRIPT_BUILD_TESTS)
    add_executable(test_lexer  tests/test_lexer.cpp)
    add_executable(test_parser tests/test_parser.cpp)
    target_link_libraries(test_lexer  PRIVATE KonScript)
    target_link_libraries(test_parser PRIVATE KonScript)

    # C++ unit tests
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_typechecker.cpp")
        add_executable(test_typechecker tests/test_typechecker.cpp)
        target_link_libraries(test_typechecker PRIVATE KonScript)
    endif()
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_const_scope.cpp")
        add_executable(test_const_scope tests/test_const_scope.cpp)
        target_link_libraries(test_const_scope PRIVATE KonScript)
    endif()
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_enum_switch.cpp")
        add_executable(test_enum_switch tests/test_enum_switch.cpp)
        target_link_libraries(test_enum_switch PRIVATE KonScript)
    endif()
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_enum_valuetype.cpp")
        add_executable(test_enum_valuetype tests/test_enum_valuetype.cpp)
        target_link_libraries(test_enum_valuetype PRIVATE KonScript)
    endif()
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_for_range.cpp")
        add_executable(test_for_range tests/test_for_range.cpp)
        target_link_libraries(test_for_range PRIVATE KonScript)
    endif()
endif()

# ── Installation ─────────────────────────────────────────────────────────
include(GNUInstallDirs)

install(TARGETS konscript
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(KONSCRIPT_BUILD_RUNTIME)
    install(TARGETS ks_runtime
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )
endif()

# Install runtime C source (needed for native compilation pipeline)
install(FILES _ks_runtime.c
    DESTINATION ${CMAKE_INSTALL_DATADIR}/konscript
)

# Install headers (for embedding konscript in other projects)
install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/konscript
    FILES_MATCHING PATTERN "*.hpp"
)

# Install ksc frontend script (Linux/macOS)
if(UNIX)
    # Generate ksc wrapper script
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/ksc"
"#!/bin/bash
# ksc — KonScript frontend (compile + run)
SCRIPT_DIR=\"$(cd \"$(dirname \"$0\")\" && pwd)\"
KS_FILE=\"$1\"
shift
if [ -z \"$KS_FILE\" ]; then
    echo \"Usage: ksc <file.ks> [args...]\"
    exit 1
fi
BASENAME=\"$(basename \"$KS_FILE\" .ks)\"
OUTDIR=\"/tmp/ksc_build\"
mkdir -p \"$OUTDIR\"
\"$SCRIPT_DIR/konscript\" \"$KS_FILE\" -o \"$OUTDIR/$BASENAME\" && \"$OUTDIR/$BASENAME\" \"$@\"
")
    install(PROGRAMS "${CMAKE_CURRENT_BINARY_DIR}/ksc"
        DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
endif()

# ── Summary ──────────────────────────────────────────────────────────────
message(STATUS "")
message(STATUS "KonScript ${PROJECT_VERSION} configuration:")
message(STATUS "  Toolchain baked : ${KONSCRIPT_TOOLCHAIN_DIR}")
message(STATUS "  Build runtime   : ${KONSCRIPT_BUILD_RUNTIME}")
message(STATUS "  Build tests     : ${KONSCRIPT_BUILD_TESTS}")
message(STATUS "  Install prefix  : ${CMAKE_INSTALL_PREFIX}")
message(STATUS "")
