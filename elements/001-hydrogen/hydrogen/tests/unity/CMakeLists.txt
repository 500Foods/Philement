# CMake configuration for Unity tests in the hydrogen project

cmake_minimum_required(VERSION 3.10)

# Project name for Unity tests
project(hydrogen_unity_tests C)

# Set C standard
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED True)

# Path to Unity framework
set(UNITY_DIR "${CMAKE_CURRENT_SOURCE_DIR}/framework/Unity")

# Include Unity headers and hydrogen source directories
include_directories(${UNITY_DIR}/src)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../src)

# Find required packages (same as main project)
find_package(PkgConfig REQUIRED)
pkg_check_modules(JANSSON REQUIRED jansson)
pkg_check_modules(MICROHTTPD REQUIRED libmicrohttpd)
pkg_check_modules(WEBSOCKETS REQUIRED libwebsockets)
pkg_check_modules(BROTLI REQUIRED libbrotlienc libbrotlidec)
find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)

# Include directories for dependencies
include_directories(${JANSSON_INCLUDE_DIRS})
include_directories(${MICROHTTPD_INCLUDE_DIRS})
include_directories(${WEBSOCKETS_INCLUDE_DIRS})
include_directories(${BROTLI_INCLUDE_DIRS})

# Source files for Unity framework
set(UNITY_SOURCES
    ${UNITY_DIR}/src/unity.c
)

# Include actual Hydrogen source files for comprehensive coverage analysis
# This allows us to measure coverage across the entire project, not just test stubs
file(GLOB_RECURSE HYDROGEN_SOURCES 
    "${CMAKE_CURRENT_SOURCE_DIR}/../../src/*.c"
)

# Remove the main hydrogen.c file as it contains main() which conflicts with Unity
list(FILTER HYDROGEN_SOURCES EXCLUDE REGEX ".*hydrogen\\.c$")

# For comprehensive coverage, we include all source files but may need to provide
# stub implementations for functions that require complex initialization

# Create static library for each hydrogen source file to maintain directory structure
set(HYDROGEN_OBJECTS "")
foreach(SOURCE_FILE ${HYDROGEN_SOURCES})
    # Get relative path from src directory
    file(RELATIVE_PATH REL_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../src" ${SOURCE_FILE})
    # Remove .c extension and create target name
    string(REPLACE ".c" "" OBJ_NAME ${REL_PATH})
    string(REPLACE "/" "_" OBJ_TARGET ${OBJ_NAME})
    
    # Create static library for this source file to get proper directory structure
    add_library(${OBJ_TARGET}_lib STATIC ${SOURCE_FILE})
    
    # Set compile flags for coverage
    target_compile_options(${OBJ_TARGET}_lib PRIVATE 
        -std=c17 -Wall -Wextra -pedantic -Werror
        -O0 -g3 -ggdb3 --coverage -fprofile-arcs -ftest-coverage -fno-omit-frame-pointer
    )
    
    # Set compile definitions
    target_compile_definitions(${OBJ_TARGET}_lib PRIVATE
        _GNU_SOURCE
        _POSIX_C_SOURCE=200809L
        VERSION="1.0.0.test"
        RELEASE="test-build"
        BUILD_TYPE="Coverage"
    )
    
    # Set output directory to mirror source structure  
    get_filename_component(OBJ_DIR ${REL_PATH} DIRECTORY)
    if(OBJ_DIR)
        set_target_properties(${OBJ_TARGET}_lib PROPERTIES
            ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/src/${OBJ_DIR}"
        )
    else()
        set_target_properties(${OBJ_TARGET}_lib PROPERTIES
            ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/src"
        )
    endif()
    
    # Add to list for linking
    list(APPEND HYDROGEN_OBJECTS ${OBJ_TARGET}_lib)
endforeach()

# Libraries needed for hydrogen
set(HYDROGEN_LIBS
    ${JANSSON_LIBRARIES}
    Threads::Threads
    ${MICROHTTPD_LIBRARIES}
    m
    OpenSSL::SSL
    OpenSSL::Crypto
    ${WEBSOCKETS_LIBRARIES}
    ${BROTLI_LIBRARIES}
)

# Compiler flags for coverage
set(COVERAGE_FLAGS -O0 -g3 -ggdb3 --coverage -fprofile-arcs -ftest-coverage -fno-omit-frame-pointer)
set(COVERAGE_LINK_FLAGS "--coverage;-lgcov")

# Enable testing
enable_testing()

# Find all test source files in the src directory
file(GLOB TEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/test_*.c")

# Create an executable for each test file
foreach(TEST_SOURCE ${TEST_SOURCES})
    get_filename_component(TEST_NAME ${TEST_SOURCE} NAME_WE)
    add_executable(${TEST_NAME}
        ${TEST_SOURCE}
        ${UNITY_SOURCES}
    )
    
    # Set compile flags for coverage on test executable
    target_compile_options(${TEST_NAME} PRIVATE ${COVERAGE_FLAGS})
    target_compile_definitions(${TEST_NAME} PRIVATE
        _GNU_SOURCE
        _POSIX_C_SOURCE=200809L
        VERSION="1.0.0.test"
        RELEASE="test-build"
        BUILD_TYPE="Coverage"
    )
    
    # Link with required libraries and coverage flags
    target_link_libraries(${TEST_NAME} 
        ${HYDROGEN_OBJECTS}
        ${HYDROGEN_LIBS}
        ${COVERAGE_LINK_FLAGS}
    )
    
    # Set output directory for test executable
    set_target_properties(${TEST_NAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
    )
    
    # Add test to CTest
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
endforeach()

# Ensure coverage data files (.gcno, .gcda) are generated in the correct location
# This is handled by the object library output directory settings above
