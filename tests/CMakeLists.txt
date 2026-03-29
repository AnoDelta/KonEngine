# KonEngine/tests/CMakeLists.txt

# -----------------------------------------------------------------------
# C++ headless + visual test suite
# -----------------------------------------------------------------------
add_executable(KonEngineTest
    ${CMAKE_CURRENT_SOURCE_DIR}/test_main.cpp
)

target_include_directories(KonEngineTest PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/window
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/animation
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/node
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/collision
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/input
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/math
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/glad/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../libs/glfw/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../libs/glm
)

target_link_libraries(KonEngineTest PRIVATE KonEngine)

set_target_properties(KonEngineTest PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
)

add_custom_target(run_tests
    COMMAND KonEngineTest
    DEPENDS KonEngineTest
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Running KonEngine C++ test suite..."
)

# -----------------------------------------------------------------------
# KonScript test game (tests codegen + engine integration)
# Requires konscript compiler to be installed.
# -----------------------------------------------------------------------
if(KONSCRIPT_COMPILER)
    set(KS_TEST_SRC  "${CMAKE_CURRENT_SOURCE_DIR}/test_game.ks")
    set(KS_TEST_CPP  "${CMAKE_CURRENT_SOURCE_DIR}/test_game.ks.cpp")

    add_custom_command(
        OUTPUT  "${KS_TEST_CPP}"
        COMMAND "${KONSCRIPT_COMPILER}" "${KS_TEST_SRC}" -o "${KS_TEST_CPP}"
        DEPENDS "${KS_TEST_SRC}" "${KONSCRIPT_COMPILER}"
        COMMENT "Compiling KonScript test: test_game.ks"
        VERBATIM
    )

    add_executable(KonScriptTest "${KS_TEST_CPP}")

    target_include_directories(KonScriptTest PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../src
        ${CMAKE_CURRENT_SOURCE_DIR}/../src/glad/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../libs/glfw/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../libs/glm
    )

    target_link_libraries(KonScriptTest PRIVATE KonEngine)

    set_target_properties(KonScriptTest PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests
    )

    add_custom_target(run_ks_test
        COMMAND KonScriptTest
        DEPENDS KonScriptTest
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Running KonScript test game..."
    )

    message(STATUS "KonScript test: enabled (test_game.ks)")
else()
    message(STATUS "KonScript test: skipped (konscript not found)")
endif()
