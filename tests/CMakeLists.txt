# KonEngine/tests/CMakeLists.txt

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
    COMMENT "Running KonEngine test suite..."
)
