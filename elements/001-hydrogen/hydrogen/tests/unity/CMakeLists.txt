# CMake configuration for Unity tests in the hydrogen project

cmake_minimum_required(VERSION 3.10)

# Project name for Unity tests
project(hydrogen_unity_tests C)

# Set C standard
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED True)

# Define version strings as CMake variables
set(HYDROGEN_VERSION "1_0_0_test")
set(HYDROGEN_RELEASE "test_build")
set(HYDROGEN_BUILD_TYPE "Coverage")

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

# Function to filter out non-include directories from pkg-config results
function(filter_include_dirs var_name input_dirs)
    set(filtered_dirs "")
    foreach(dir ${input_dirs})
        # Only add directories that exist and are actual directories
        if(EXISTS "${dir}" AND IS_DIRECTORY "${dir}")
            # Skip directories that are clearly for linking (contain .so files, .a files, etc.)
            file(GLOB linker_files "${dir}/*.so*" "${dir}/*.a" "${dir}/*.dylib")
            if(NOT linker_files)
                list(APPEND filtered_dirs "${dir}")
            endif()
        endif()
    endforeach()
    set(${var_name} "${filtered_dirs}" PARENT_SCOPE)
endfunction()

# Filter include directories for dependencies to avoid linker warnings
filter_include_dirs(JANSSON_FILTERED_INCLUDES "${JANSSON_INCLUDE_DIRS}")
filter_include_dirs(MICROHTTPD_FILTERED_INCLUDES "${MICROHTTPD_INCLUDE_DIRS}")
filter_include_dirs(WEBSOCKETS_FILTERED_INCLUDES "${WEBSOCKETS_INCLUDE_DIRS}")
filter_include_dirs(BROTLI_FILTERED_INCLUDES "${BROTLI_INCLUDE_DIRS}")

# Include directories for dependencies (filtered)
include_directories(${JANSSON_FILTERED_INCLUDES})
include_directories(${MICROHTTPD_FILTERED_INCLUDES})
include_directories(${WEBSOCKETS_FILTERED_INCLUDES})
include_directories(${BROTLI_FILTERED_INCLUDES})

# Source files for Unity framework
set(UNITY_SOURCES
    ${UNITY_DIR}/src/unity.c
)

# Load .trial-ignore patterns early for filtering
set(TRIAL_IGNORE_FILE "${CMAKE_CURRENT_SOURCE_DIR}/../../.trial-ignore")
set(IGNORED_PATTERNS "")
if(EXISTS "${TRIAL_IGNORE_FILE}")
    file(STRINGS "${TRIAL_IGNORE_FILE}" IGNORE_LINES)
    foreach(line ${IGNORE_LINES})
        # Skip comments and empty lines
        if(NOT line MATCHES "^[[:space:]]*#" AND NOT line STREQUAL "")
            string(REGEX REPLACE "^\\./?" "" pattern "${line}")
            if(NOT pattern STREQUAL "")
                list(APPEND IGNORED_PATTERNS "${pattern}")
            endif()
        endif()
    endforeach()
    list(LENGTH IGNORED_PATTERNS PATTERN_COUNT)
    message(STATUS "Unity tests: Loaded ${PATTERN_COUNT} ignore patterns from .trial-ignore")
endif()

# Include actual Hydrogen source files for comprehensive coverage analysis
# This allows us to measure coverage across the entire project, not just test stubs
file(GLOB_RECURSE HYDROGEN_SOURCES_RAW 
    "${CMAKE_CURRENT_SOURCE_DIR}/../../src/*.c"
)

# Remove the main hydrogen.c file as it contains main() which conflicts with Unity
list(FILTER HYDROGEN_SOURCES_RAW EXCLUDE REGEX ".*hydrogen\\.c$")

# Filter HYDROGEN_SOURCES using .trial-ignore patterns (early filtering)
set(HYDROGEN_SOURCES "")
foreach(source_file ${HYDROGEN_SOURCES_RAW})
    file(RELATIVE_PATH rel_path "${CMAKE_CURRENT_SOURCE_DIR}/../../src" ${source_file})
    set(should_ignore FALSE)
    foreach(pattern ${IGNORED_PATTERNS})
        if(rel_path MATCHES "${pattern}")
            set(should_ignore TRUE)
            message(STATUS "Unity tests: Ignoring ${rel_path} (matches pattern: ${pattern})")
            break()
        endif()
    endforeach()
    if(NOT should_ignore)
        list(APPEND HYDROGEN_SOURCES ${source_file})
    endif()
endforeach()

list(LENGTH HYDROGEN_SOURCES SOURCE_COUNT)
message(STATUS "Unity tests: Processing ${SOURCE_COUNT} source files after .trial-ignore filtering")

# For comprehensive coverage, we include all source files but may need to provide
# stub implementations for functions that require complex initialization

# Build include directory flags from filtered directories
set(FILTERED_INCLUDE_FLAGS "")
foreach(dir ${JANSSON_FILTERED_INCLUDES})
    list(APPEND FILTERED_INCLUDE_FLAGS "-I${dir}")
endforeach()
foreach(dir ${MICROHTTPD_FILTERED_INCLUDES})
    list(APPEND FILTERED_INCLUDE_FLAGS "-I${dir}")
endforeach()
foreach(dir ${WEBSOCKETS_FILTERED_INCLUDES})
    list(APPEND FILTERED_INCLUDE_FLAGS "-I${dir}")
endforeach()
foreach(dir ${BROTLI_FILTERED_INCLUDES})
    list(APPEND FILTERED_INCLUDE_FLAGS "-I${dir}")
endforeach()

# Create object files for each hydrogen source file to maintain directory structure
set(HYDROGEN_OBJECT_FILES "")
foreach(SOURCE_FILE ${HYDROGEN_SOURCES})
    # Get relative path from src directory
    file(RELATIVE_PATH REL_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../../src" ${SOURCE_FILE})
    # Remove .c extension and get directory structure
    string(REPLACE ".c" "" OBJ_NAME ${REL_PATH})
    get_filename_component(OBJ_DIR ${REL_PATH} DIRECTORY)
    get_filename_component(OBJ_BASENAME ${REL_PATH} NAME_WE)
    
    # Set up output directory structure to mirror source
    if(OBJ_DIR)
        set(OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/src/${OBJ_DIR}")
        set(OUTPUT_OBJ "${OUTPUT_DIR}/${OBJ_BASENAME}.o")
    else()
        set(OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/src")
        set(OUTPUT_OBJ "${OUTPUT_DIR}/${OBJ_BASENAME}.o")
    endif()
    
    # Create custom command to compile source file to object file
    add_custom_command(
        OUTPUT ${OUTPUT_OBJ}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${OUTPUT_DIR}
        COMMAND ${CMAKE_C_COMPILER} 
            -std=c17 -Wall -Wextra -pedantic -Werror
            -D_GNU_SOURCE
            -D_POSIX_C_SOURCE=200809L
            -DVERSION='"${HYDROGEN_VERSION}"'
            -DRELEASE='"${HYDROGEN_RELEASE}"'
            -DBUILD_TYPE='"${HYDROGEN_BUILD_TYPE}"'
            -O0 -g3 -ggdb3 --coverage -fprofile-arcs -ftest-coverage -fno-omit-frame-pointer
            -I${CMAKE_CURRENT_SOURCE_DIR}/../../src
            ${FILTERED_INCLUDE_FLAGS}
            -c ${SOURCE_FILE} -o ${OUTPUT_OBJ}
        DEPENDS ${SOURCE_FILE}
        COMMENT "Compiling ${REL_PATH} to object file"
    )
    
    # Add to list for linking with test executables
    list(APPEND HYDROGEN_OBJECT_FILES ${OUTPUT_OBJ})
endforeach()

# Create a target that depends on all object files
add_custom_target(hydrogen_objects ALL DEPENDS ${HYDROGEN_OBJECT_FILES})

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
        VERSION="${HYDROGEN_VERSION}"
        RELEASE="${HYDROGEN_RELEASE}"
        BUILD_TYPE="${HYDROGEN_BUILD_TYPE}"
    )
    
    # Make sure object files are built before linking test executable
    add_dependencies(${TEST_NAME} hydrogen_objects)
    
    # Link with required libraries and coverage flags
    target_link_libraries(${TEST_NAME} 
        ${HYDROGEN_OBJECT_FILES}
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
