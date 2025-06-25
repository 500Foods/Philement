# CMake configuration for Unity tests in the hydrogen project

cmake_minimum_required(VERSION 3.10)

# Project name for Unity tests
project(hydrogen_unity_tests C)

# Set C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)

# Path to Unity framework
set(UNITY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/framework/Unity")

# Include Unity headers
include_directories(${UNITY_DIR}/src)

# Source files for Unity framework
set(UNITY_SOURCES
    ${UNITY_DIR}/src/unity.c
)

# Enable testing
enable_testing()

# Find all test source files
file(GLOB TEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/test_*.c")

# Create an executable for each test file
foreach(TEST_SOURCE ${TEST_SOURCES})
    get_filename_component(TEST_NAME ${TEST_SOURCE} NAME_WE)
    add_executable(${TEST_NAME}
        ${TEST_SOURCE}
        ${UNITY_SOURCES}
    )
    # Link to any necessary project libraries if needed
    # target_link_libraries(${TEST_NAME} hydrogen_lib)
    
    # Add test to CTest
    add_test(NAME ${TEST_NAME}
             COMMAND ${TEST_NAME})
endforeach()

# Custom target to build and run all Unity tests
add_custom_target(run_unity_tests
    COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure
    DEPENDS ${TEST_SOURCES}
    COMMENT "Running all Unity tests..."
)
