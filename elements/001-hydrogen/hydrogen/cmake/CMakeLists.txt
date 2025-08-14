# Hydrogen CMake Build System Configuration

# This CMake configuration file sets up the build system for the Hydrogen server and example programs.

# - Default: Standard build with optimizations and debug symbols
# - Debug: Build with AddressSanitizer for debugging memory issues
# - Valgrind: Build optimized for memory analysis with Valgrind
# - Performance: Highly optimized build for maximum performance
# - Coverage: Build with gcov coverage instrumentation for unit testing
# - Release: Stripped, compressed build with embedded encrypted payload
# - Examples: Builds for OIDC client examples (Authorization Code, Client Credentials, Password flows)

# Key features:
# - Automatic version numbering based on git commits
# - Multiple optimization levels for different use cases
# - Unused source file detection
# - UPX compression for release builds
# - Encrypted payload embedding
# - Comprehensive build artifact management
 
# Available CMake targets:
# cmake --build . --target hydrogen          : Builds the default version with standard optimizations and debug symbols
# cmake --build . --target hydrogen_debug    : Builds with AddressSanitizer enabled for catching memory issues
# cmake --build . --target hydrogen_valgrind : Builds with -O0 and full debug info for optimal Valgrind analysis
# cmake --build . --target hydrogen_perf     : Builds with aggressive optimizations (-O3, native arch, etc.) for maximum performance
# cmake --build . --target hydrogen_coverage : Builds with gcov coverage instrumentation for unit testing
# cmake --build . --target hydrogen_release  : Creates stripped, compressed executable with embedded encrypted payload
# cmake --build . --target payload           : Generates OpenAPI specs and packages payload contents
# cmake --build . --target all_variants      : Builds all variants (default, debug, valgrind, perf, release) and generates payload
# cmake --build . --target trial             : Runs a test build showing only warnings/errors, checks for unused files, runs shutdown test
# cmake --build . --target auth_code_flow    : Builds the Authorization Code flow example
# cmake --build . --target client_credentials: Builds the Client Credentials flow example
# cmake --build . --target password_flow     : Builds the Resource Owner Password flow example
# cmake --build . --target all_examples      : Builds all example programs

# CHANGELOG
# 2025-07-01: Added targets for building OIDC client example programs (auth_code_flow, client_credentials, password_flow) with regular and debug builds. Updated help and clean targets to include examples.

cmake_minimum_required(VERSION 3.16)

# Project definition
project(Hydrogen
    VERSION 1.0.0
    DESCRIPTION "Hydrogen Server"
    LANGUAGES C
)

# Set C standard
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

# Build Performance Optimizations

# Detect available CPU cores for parallel builds
include(ProcessorCount)
ProcessorCount(CPU_CORES)
if(CPU_CORES EQUAL 0)
    set(CPU_CORES 4)  # Fallback to 4 cores
endif()

# Set parallel job count (use 200% of available cores to avoid system overload)
math(EXPR PARALLEL_JOBS "${CPU_CORES} * 20 / 10")
if(PARALLEL_JOBS LESS 1)
    set(PARALLEL_JOBS 1)
endif()

message(STATUS "Detected ${CPU_CORES} CPU cores, using ${PARALLEL_JOBS} parallel jobs")

# Enable parallel builds by default
set(CMAKE_BUILD_PARALLEL_LEVEL ${PARALLEL_JOBS} CACHE STRING "Number of parallel build jobs")

# Use Ninja generator for faster builds if available
if(NOT CMAKE_GENERATOR STREQUAL "Ninja")
    find_program(NINJA_EXECUTABLE ninja)
    if(NINJA_EXECUTABLE)
        message(STATUS "Ninja found: ${NINJA_EXECUTABLE} - Consider using -G Ninja for faster builds")
    endif()
endif()

# Enable compiler caching if available
find_program(CCACHE_EXECUTABLE ccache)
if(CCACHE_EXECUTABLE)
    message(STATUS "ccache found: ${CCACHE_EXECUTABLE} - Enabling compiler caching")
    set(CMAKE_C_COMPILER_LAUNCHER ${CCACHE_EXECUTABLE})
endif()

# CPU-specific optimizations
if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64")
    # Detect CPU features for optimal performance
    execute_process(
        COMMAND grep -m1 "model name" /proc/cpuinfo
        OUTPUT_VARIABLE CPU_MODEL
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    
    # Set CPU-specific optimizations based on detected processor
    if(CPU_MODEL MATCHES "Intel.*i[357]-[0-9]+K")
        set(CPU_ARCH_FLAGS "-march=native;-mtune=native")
        message(STATUS "Intel K-series CPU detected, using native optimizations")
    elseif(CPU_MODEL MATCHES "Intel")
        set(CPU_ARCH_FLAGS "-march=x86-64-v2;-mtune=intel")
        message(STATUS "Intel CPU detected, using x86-64-v2 optimizations")
    elseif(CPU_MODEL MATCHES "AMD")
        set(CPU_ARCH_FLAGS "-march=x86-64-v2;-mtune=generic")
        message(STATUS "AMD CPU detected, using x86-64-v2 optimizations")
    else()
        set(CPU_ARCH_FLAGS "-march=x86-64")
        message(STATUS "Generic x86-64 optimizations")
    endif()
else()
    set(CPU_ARCH_FLAGS "")
endif()

# Set default build type if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING "Choose the type of build" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "RelWithDebInfo" "MinSizeRel")
endif()

# Terminal output formatting for better readability
# Note: CMake handles escape sequences differently, so we use string literals
string(ASCII 27 ESC)
set(GREEN "${ESC}[0;32m")
set(RED "${ESC}[0;31m")
set(YELLOW "${ESC}[0;33m")
set(BLUE "${ESC}[0;34m")
set(CYAN "${ESC}[0;36m")
set(MAGENTA "${ESC}[0;35m")
set(BOLD "${ESC}[1m")
set(NORMAL "${ESC}[0m")

# Unicode status symbols for visual feedback
set(PASS "✅")
set(FAIL "❌")
set(WARN "⚠️")
set(INFO "🛈")

# Version information management
# - MAJOR.MINOR.PATCH follows semantic versioning
# - BUILD number is derived from git commit count plus offset
# - RELEASE timestamp is in ISO 8601 format
set(HYDROGEN_VERSION_MAJOR 1)
set(HYDROGEN_VERSION_MINOR 0)
set(HYDROGEN_VERSION_PATCH 0)

# Get git commit count
find_package(Git QUIET)
if(GIT_FOUND)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} log --oneline .
        COMMAND wc -l
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..
        OUTPUT_VARIABLE GIT_COMMIT_COUNT
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )
    if(NOT GIT_COMMIT_COUNT)
        set(GIT_COMMIT_COUNT 0)
    endif()
else()
    set(GIT_COMMIT_COUNT 0)
endif()

# Add a custom target to check if version needs updating
add_custom_target(check_version
    COMMAND ${CMAKE_COMMAND} -E echo "🛈 Checking if version number needs updating..."
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/check_version.sh "${CMAKE_CURRENT_SOURCE_DIR}/.." "${GIT_COMMIT_COUNT}"
    COMMAND ${CMAKE_COMMAND} -E echo "✅ Version check completed."
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Checking if version number needs updating based on git commits"
)

math(EXPR HYDROGEN_BUILD_NUMBER "${GIT_COMMIT_COUNT} + 1000")
set(HYDROGEN_VERSION "${HYDROGEN_VERSION_MAJOR}.${HYDROGEN_VERSION_MINOR}.${HYDROGEN_VERSION_PATCH}.${HYDROGEN_BUILD_NUMBER}")

# Release timestamp in ISO 8601 format
string(TIMESTAMP HYDROGEN_RELEASE "%Y-%m-%d" )

message(STATUS "Hydrogen Version: ${HYDROGEN_VERSION}")
message(STATUS "Release Timestamp: ${HYDROGEN_RELEASE}")

# Find required packages
find_package(PkgConfig REQUIRED)

# Find required libraries
pkg_check_modules(JANSSON REQUIRED jansson)
pkg_check_modules(MICROHTTPD REQUIRED libmicrohttpd)
pkg_check_modules(WEBSOCKETS REQUIRED libwebsockets)
pkg_check_modules(BROTLI REQUIRED libbrotlienc libbrotlidec)

find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)

# Find optional packages
find_program(UPX_EXECUTABLE upx)
find_program(VALGRIND_EXECUTABLE valgrind)

# Compiler configuration and base flags
set(HYDROGEN_BASE_CFLAGS
    -std=c17
    -Wall
    -Wextra
    -pedantic
    -Werror
    -D_GNU_SOURCE
    -D_POSIX_C_SOURCE=200809L
    -ffunction-sections
    -fdata-sections
    -pipe  # Use pipes instead of temporary files for faster compilation
)

# Add CPU-specific optimizations to base flags for better performance
if(CPU_ARCH_FLAGS)
    list(APPEND HYDROGEN_BASE_CFLAGS ${CPU_ARCH_FLAGS})
endif()

# Base libraries
set(HYDROGEN_BASE_LIBS
    ${JANSSON_LIBRARIES}
    Threads::Threads
    ${MICROHTTPD_LIBRARIES}
    m
    OpenSSL::SSL
    OpenSSL::Crypto
    ${WEBSOCKETS_LIBRARIES}
    ${BROTLI_LIBRARIES}
)

# Include directories
set(HYDROGEN_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
    ${JANSSON_INCLUDE_DIRS}
    ${MICROHTTPD_INCLUDE_DIRS}
    ${WEBSOCKETS_INCLUDE_DIRS}
    ${BROTLI_INCLUDE_DIRS}
)

# Source file discovery
file(GLOB_RECURSE HYDROGEN_SOURCES
    "../src/*.c"
)

# Remove files that should not be linked (from .trial-ignore)
list(REMOVE_ITEM HYDROGEN_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/../src/not_hydrogen.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../src/config/config_defaults.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../src/config/mdns/config_mdns.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../src/config/oidc/config_oidc.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../src/config/oidc/config_oidc_endpoints.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../src/config/oidc/config_oidc_keys.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../src/config/oidc/config_oidc_security.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../src/config/oidc/config_oidc_tokens.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../src/config/print/config_print_buffers.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../src/config/print/config_print_priorities.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../src/config/print/config_print_queue.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../src/config/print/config_print_timeouts.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../src/config/resources/config_resources.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../src/config/security/keys.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../src/config/types/config_string.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../src/config/webserver/config_webserver.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../src/config/websocket/config_websocket.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../src/print/print_queue_manager.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../src/websocket/websocket_dynamic.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../src/api/system/system_service.c"
)

# Function to add an executable target with specific build configuration using clean directory structure
function(hydrogen_add_executable_target target_name build_type extra_cflags extra_ldflags)
    # Create object files for each hydrogen source file to maintain clean directory structure
    set(TARGET_OBJECT_FILES "")
    foreach(SOURCE_FILE ${HYDROGEN_SOURCES})
        # Get relative path from src directory
        file(RELATIVE_PATH REL_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../src" ${SOURCE_FILE})
        get_filename_component(OBJ_DIR ${REL_PATH} DIRECTORY)
        get_filename_component(OBJ_BASENAME ${REL_PATH} NAME_WE)
        
        # Set up output directory structure to mirror source in build/[target]/src/
        if(OBJ_DIR)
            set(OUTPUT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../build/${target_name}/src/${OBJ_DIR}")
            set(OUTPUT_OBJ "${OUTPUT_DIR}/${OBJ_BASENAME}.o")
        else()
            set(OUTPUT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../build/${target_name}/src")
            set(OUTPUT_OBJ "${OUTPUT_DIR}/${OBJ_BASENAME}.o")
        endif()
        
        # Prepare extra compile flags as a list
        if(extra_cflags)
            separate_arguments(extra_cflags_list UNIX_COMMAND "${extra_cflags}")
        else()
            set(extra_cflags_list "")
        endif()
        
        # Create custom command to compile source file to object file
        add_custom_command(
            OUTPUT ${OUTPUT_OBJ}
            COMMAND ${CMAKE_COMMAND} -E make_directory ${OUTPUT_DIR}
            COMMAND ${CMAKE_C_COMPILER} 
                ${HYDROGEN_BASE_CFLAGS}
                ${extra_cflags_list}
                -DVERSION='"${HYDROGEN_VERSION}"'
                -DRELEASE='"${HYDROGEN_RELEASE}"'
                -DBUILD_TYPE='"${build_type}"'
                -I${CMAKE_CURRENT_SOURCE_DIR}/../src
                ${JANSSON_CFLAGS}
                ${MICROHTTPD_CFLAGS}
                ${WEBSOCKETS_CFLAGS}
                ${BROTLI_CFLAGS}
                -c ${SOURCE_FILE} -o ${OUTPUT_OBJ}
            DEPENDS ${SOURCE_FILE}
            COMMENT "Compiling ${REL_PATH} to ${target_name} object file"
        )
        
        list(APPEND TARGET_OBJECT_FILES ${OUTPUT_OBJ})
    endforeach()

    # Create target that depends on all object files
    add_custom_target(${target_name}_objects DEPENDS ${TARGET_OBJECT_FILES})

    # Create the executable using the individual object files
    add_executable(${target_name} ${TARGET_OBJECT_FILES})
    add_dependencies(${target_name} ${target_name}_objects)
    
    # Set include directories
    target_include_directories(${target_name} PRIVATE ${HYDROGEN_INCLUDE_DIRS})
    
    # Add extra compile flags if provided
    if(extra_cflags)
        separate_arguments(extra_cflags_list UNIX_COMMAND "${extra_cflags}")
        target_compile_options(${target_name} PRIVATE ${extra_cflags_list})
    endif()
    
    # Set compile definitions
    target_compile_definitions(${target_name} PRIVATE
        VERSION="${HYDROGEN_VERSION}"
        RELEASE="${HYDROGEN_RELEASE}"
        BUILD_TYPE="${build_type}"
    )
    
    # Link libraries
    target_link_libraries(${target_name} PRIVATE ${HYDROGEN_BASE_LIBS})
    
    # Add extra link flags if provided
    if(extra_ldflags)
        separate_arguments(extra_ldflags_list UNIX_COMMAND "${extra_ldflags}")
        target_link_options(${target_name} PRIVATE ${extra_ldflags_list})
    endif()
    
    # Add garbage collection flags
    target_link_options(${target_name} PRIVATE -Wl,--gc-sections)
    
    # Generate map file in target directory
    target_link_options(${target_name} PRIVATE -Wl,-Map=${CMAKE_CURRENT_SOURCE_DIR}/../build/${target_name}/${target_name}.map)
    
    # Set output name and properties
    # Binary names should have hydrogen_ prefix except for regular which is just "hydrogen"
    if(${target_name} STREQUAL "regular")
        set(BINARY_NAME "hydrogen")
    else()
        set(BINARY_NAME "hydrogen_${target_name}")
    endif()
    
    set_target_properties(${target_name} PROPERTIES
        OUTPUT_NAME ${BINARY_NAME}
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..
        LINKER_LANGUAGE C
    )
    
    # Add custom command to show build completion
    add_custom_command(TARGET ${target_name} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "${GREEN}${PASS} ${BOLD}${build_type} build${NORMAL} ${GREEN}completed successfully: ${target_name}${NORMAL}"
        COMMAND ${CMAKE_COMMAND} -E echo "${CYAN}${INFO} Object files located in: ${CMAKE_CURRENT_SOURCE_DIR}/../build/${target_name}/src/${NORMAL}"
        VERBATIM
    )
endfunction()

# Default build target
hydrogen_add_executable_target(regular "Regular" "-O2 -g" "-no-pie")

# Debug build with AddressSanitizer
hydrogen_add_executable_target(debug "Debug" 
    "-g -fsanitize=address,leak -fno-omit-frame-pointer" 
    "-lasan -fsanitize=address,leak -no-pie"
)

# Valgrind-compatible build
hydrogen_add_executable_target(valgrind "Valgrind-Compatible" 
    "-O0 -g" 
    "-no-pie"
)

# Performance-optimized build
hydrogen_add_executable_target(perf "Performance"
    "-O3 -g -ffast-math -finline-functions -funroll-loops -DNDEBUG"
    "-flto=auto -no-pie"
)

# Coverage build for unit testing with gcov - uses clean build/coverage/src directory structure like Unity tests
# Create object files for each hydrogen source file to maintain clean directory structure
set(HYDROGEN_COVERAGE_OBJECT_FILES "")
foreach(SOURCE_FILE ${HYDROGEN_SOURCES})
    # Get relative path from src directory
    file(RELATIVE_PATH REL_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../src" ${SOURCE_FILE})
    get_filename_component(OBJ_DIR ${REL_PATH} DIRECTORY)
    get_filename_component(OBJ_BASENAME ${REL_PATH} NAME_WE)
    
    # Set up output directory structure to mirror source in build/coverage/src/
    if(OBJ_DIR)
        set(OUTPUT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../build/coverage/src/${OBJ_DIR}")
        set(OUTPUT_OBJ "${OUTPUT_DIR}/${OBJ_BASENAME}.o")
    else()
        set(OUTPUT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../build/coverage/src")
        set(OUTPUT_OBJ "${OUTPUT_DIR}/${OBJ_BASENAME}.o")
    endif()
    
    # Create custom command to compile source file to object file
    add_custom_command(
        OUTPUT ${OUTPUT_OBJ}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${OUTPUT_DIR}
        COMMAND ${CMAKE_C_COMPILER} 
            ${HYDROGEN_BASE_CFLAGS}
            -O0 -g3 -ggdb3 --coverage -fprofile-arcs -ftest-coverage -fno-omit-frame-pointer
            -DHYDROGEN_COVERAGE_BUILD
            -DVERSION='"${HYDROGEN_VERSION}"'
            -DRELEASE='"${HYDROGEN_RELEASE}"'
            -DBUILD_TYPE='"Coverage"'
            -I${CMAKE_CURRENT_SOURCE_DIR}/../src
            ${JANSSON_CFLAGS}
            ${MICROHTTPD_CFLAGS}
            ${WEBSOCKETS_CFLAGS}
            ${BROTLI_CFLAGS}
            -c ${SOURCE_FILE} -o ${OUTPUT_OBJ}
        DEPENDS ${SOURCE_FILE}
        COMMENT "Compiling ${REL_PATH} to coverage object file"
    )
    
    list(APPEND HYDROGEN_COVERAGE_OBJECT_FILES ${OUTPUT_OBJ})
endforeach()

# Create target that depends on all coverage object files
add_custom_target(hydrogen_coverage_objects DEPENDS ${HYDROGEN_COVERAGE_OBJECT_FILES})

# Create the coverage executable using the individual object files
add_executable(hydrogen_coverage ${HYDROGEN_COVERAGE_OBJECT_FILES})
add_dependencies(hydrogen_coverage hydrogen_coverage_objects)
target_link_libraries(hydrogen_coverage PRIVATE ${HYDROGEN_BASE_LIBS})
target_link_options(hydrogen_coverage PRIVATE --coverage -lgcov -no-pie)
set_target_properties(hydrogen_coverage PROPERTIES
    OUTPUT_NAME hydrogen_coverage
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..
    LINKER_LANGUAGE C
)

# Add custom command to show build completion for coverage target
add_custom_command(TARGET hydrogen_coverage POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E echo "${GREEN}${PASS} ${BOLD}Coverage build${NORMAL} ${GREEN}completed successfully: hydrogen_coverage${NORMAL}"
    COMMAND ${CMAKE_COMMAND} -E echo "${CYAN}${INFO} Coverage files located in: ${CMAKE_CURRENT_SOURCE_DIR}/../build/coverage/src/${NORMAL}"
    VERBATIM
)

# Coverage build with payload embedding (similar to release but without UPX compression)
# This matches the coverage binary behavior for comprehensive testing
add_custom_target(coverage
    DEPENDS hydrogen_coverage payload
    COMMAND ${CMAKE_COMMAND} -E echo "🛈  Building coverage executable with payload..."
    # Build the coverage executable
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_CURRENT_BINARY_DIR} --target hydrogen_coverage
    COMMAND ${CMAKE_COMMAND} -E echo "🛈  Creating copy for payload embedding..."
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/../hydrogen_coverage ${CMAKE_CURRENT_SOURCE_DIR}/../hydrogen_coverage_temp
    COMMAND ${CMAKE_COMMAND} -E echo "🛈  Appending encrypted payload to coverage executable..."
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/embed_payload.sh ${CMAKE_CURRENT_SOURCE_DIR}/../hydrogen_coverage_temp ${CMAKE_CURRENT_SOURCE_DIR}/../payloads/payload.tar.br.enc
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/../hydrogen_coverage_temp ${CMAKE_CURRENT_SOURCE_DIR}/../hydrogen_coverage
    COMMAND ${CMAKE_COMMAND} -E remove -f ${CMAKE_CURRENT_SOURCE_DIR}/../hydrogen_coverage_temp
    COMMAND ${CMAKE_COMMAND} -E echo "✅ Payload embedded successfully"
    COMMAND ${CMAKE_COMMAND} -E true
    COMMAND ${CMAKE_COMMAND} -E echo "✅ Coverage build with encrypted payload appended successfully!"
    COMMAND ${CMAKE_COMMAND} -E echo "────────────────────────────────────────────────────────────────"
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Creating coverage build with payload (no compression)"
)

# Clean coverage executables target (used by coverage with payload)
add_custom_target(clean_coverage_executables
    COMMAND ${CMAKE_COMMAND} -E echo "🛈  Cleaning previous coverage executables..."
    COMMAND ${CMAKE_COMMAND} -E remove -f ${CMAKE_CURRENT_SOURCE_DIR}/../hydrogen_coverage
    COMMENT "Cleaning previous coverage executables"
)

# Payload generation target
add_custom_target(payload
    COMMAND ${CMAKE_COMMAND} -E echo "────────────────────────────────────────────────────────────────"
    COMMAND ${CMAKE_COMMAND} -E echo " Checking Payload Contents"
    COMMAND ${CMAKE_COMMAND} -E echo "────────────────────────────────────────────────────────────────"
    COMMAND ${CMAKE_COMMAND} -E echo "🛈  Using existing payload file"
    COMMAND ${CMAKE_COMMAND} -E echo "✅ Payload contents ready!"
    COMMAND ${CMAKE_COMMAND} -E echo "────────────────────────────────────────────────────────────────"
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Ensuring payload contents are available"
)

# Clean executables target (used by release)
add_custom_target(clean_executables
    COMMAND ${CMAKE_COMMAND} -E echo "🛈  Cleaning previous release executables..."
    COMMAND ${CMAKE_COMMAND} -E remove -f ${CMAKE_CURRENT_SOURCE_DIR}/../hydrogen_release
    COMMAND ${CMAKE_COMMAND} -E remove -f ${CMAKE_CURRENT_SOURCE_DIR}/../hydrogen_naked
    COMMENT "Cleaning previous release executables"
)

# Release build with special optimizations, UPX compression, and payload embedding
# This matches the original Makefile 'make release' behavior exactly
add_custom_target(hydrogen_release
    DEPENDS clean_executables payload
    COMMAND ${CMAKE_COMMAND} -E echo "🛈  Building release executable..."
    # Build the release executable
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_CURRENT_BINARY_DIR} --target release
    COMMAND ${CMAKE_COMMAND} -E echo "🛈  Stripping debug information from release executable..."
    COMMAND strip -s ${CMAKE_CURRENT_SOURCE_DIR}/../hydrogen_release
    COMMAND ${CMAKE_COMMAND} -E echo "STEP 1: Moving hydrogen_release to hydrogen_naked"
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/../hydrogen_release ${CMAKE_CURRENT_SOURCE_DIR}/../hydrogen_naked
    COMMAND ${CMAKE_COMMAND} -E echo "STEP 2: UPX compress hydrogen_naked"
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/apply_upx.sh ${CMAKE_CURRENT_SOURCE_DIR}/../hydrogen_naked
    COMMAND ${CMAKE_COMMAND} -E echo "STEP 3: Copy hydrogen_naked to hydrogen_release"
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/../hydrogen_naked ${CMAKE_CURRENT_SOURCE_DIR}/../hydrogen_release
    COMMAND ${CMAKE_COMMAND} -E echo "STEP 4: Embed payload in hydrogen_release - hydrogen_naked untouched"
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/embed_payload.sh ${CMAKE_CURRENT_SOURCE_DIR}/../hydrogen_release ${CMAKE_CURRENT_SOURCE_DIR}/../payloads/payload.tar.br.enc
    COMMAND ${CMAKE_COMMAND} -E echo "FINAL CHECK:"
    COMMAND ls -lh ${CMAKE_CURRENT_SOURCE_DIR}/../hydrogen_naked ${CMAKE_CURRENT_SOURCE_DIR}/../hydrogen_release
    COMMAND ${CMAKE_COMMAND} -E echo "✅ Payload embedded successfully"
    COMMAND ${CMAKE_COMMAND} -E true
    COMMAND ${CMAKE_COMMAND} -E echo "✅ Release build with encrypted payload appended successfully!"
    COMMAND ${CMAKE_COMMAND} -E echo "────────────────────────────────────────────────────────────────"
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Creating release build with compression and payload (matches Makefile behavior)"
)

# Internal target for building just the release executable (without payload processing)
hydrogen_add_executable_target(release "Release"
    "-Os -s -DNDEBUG -march=x86-64 -flto=auto -fno-stack-protector -fno-asynchronous-unwind-tables -ffunction-sections -fdata-sections"
    "-flto=auto -Wl,--gc-sections -Wl,--strip-all"
)


# Example Programs for Hydrogen OIDC Client
# 
# This section defines targets for building the example programs located in
# examples/C/. These mirror the functionality of the Makefile in that directory.
# - auth_code_flow: Authorization Code flow with PKCE
# - client_credentials: Client Credentials flow
# - password_flow: Resource Owner Password flow
# 
# Available CMake targets for examples:
# cmake --build . --target auth_code_flow          : Builds the Authorization Code flow example
# cmake --build . --target client_credentials      : Builds the Client Credentials flow example
# cmake --build . --target password_flow           : Builds the Resource Owner Password flow example
# cmake --build . --target auth_code_flow_debug    : Builds the Authorization Code flow example with debug symbols
# cmake --build . --target client_credentials_debug: Builds the Client Credentials flow example with debug symbols
# cmake --build . --target password_flow_debug     : Builds the Resource Owner Password flow example with debug symbols
# cmake --build . --target all_examples            : Builds all example programs

# Find required libraries for examples
find_package(CURL REQUIRED)

# Function to add an executable target for example programs
function(hydrogen_add_example_target target_name source_file build_type extra_cflags extra_ldflags libraries)
    # Create the executable
    add_executable(${target_name} "${CMAKE_CURRENT_SOURCE_DIR}/../examples/C/${source_file}")
    
    # Set include directories
    target_include_directories(${target_name} PRIVATE ${HYDROGEN_INCLUDE_DIRS})
    
    # Set base compile options
    target_compile_options(${target_name} PRIVATE ${HYDROGEN_BASE_CFLAGS})
    
    # Add extra compile flags if provided
    if(extra_cflags)
        separate_arguments(extra_cflags_list UNIX_COMMAND "${extra_cflags}")
        target_compile_options(${target_name} PRIVATE ${extra_cflags_list})
    endif()
    
    # Set compile definitions
    target_compile_definitions(${target_name} PRIVATE
        VERSION="${HYDROGEN_VERSION}"
        RELEASE="${HYDROGEN_RELEASE}"
        BUILD_TYPE="${build_type}"
    )
    
    # Link libraries
    target_link_libraries(${target_name} PRIVATE ${libraries})
    
    # Add extra link flags if provided
    if(extra_ldflags)
        separate_arguments(extra_ldflags_list UNIX_COMMAND "${extra_ldflags}")
        target_link_options(${target_name} PRIVATE ${extra_ldflags_list})
    endif()
    
    # Set output name and properties
    set_target_properties(${target_name} PROPERTIES
        OUTPUT_NAME ${target_name}
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/../examples/C
    )
    
    # Add custom command to show build completion
    add_custom_command(TARGET ${target_name} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "${GREEN}${PASS} ${BOLD}${build_type} example build${NORMAL} ${GREEN}completed successfully: ${target_name}${NORMAL}"
        VERBATIM
    )
endfunction()

# Example libraries (using jansson, curl, and OpenSSL which are already found)
set(EXAMPLE_LIBS
    ${CURL_LIBRARIES}
    ${JANSSON_LIBRARIES}
    OpenSSL::SSL
    OpenSSL::Crypto
    ${MICROHTTPD_LIBRARIES}
)

# Authorization Code Flow example
hydrogen_add_example_target(auth_code_flow "auth_code_flow.c" "Regular" "-O2 -g" "" "${EXAMPLE_LIBS}")

# Client Credentials Flow example
hydrogen_add_example_target(client_credentials "client_credentials.c" "Regular" "-O2 -g" "" "${EXAMPLE_LIBS}")

# Resource Owner Password Flow example
hydrogen_add_example_target(password_flow "password_flow.c" "Regular" "-O2 -g" "" "${EXAMPLE_LIBS}")

# Debug versions of examples
hydrogen_add_example_target(auth_code_flow_debug "auth_code_flow.c" "Debug" "-fsanitize=address,leak -fno-omit-frame-pointer" "-lasan -fsanitize=address,leak" "${EXAMPLE_LIBS}")

hydrogen_add_example_target(client_credentials_debug "client_credentials.c" "Debug" "-fsanitize=address,leak -fno-omit-frame-pointer" "-lasan -fsanitize=address,leak" "${EXAMPLE_LIBS}")

hydrogen_add_example_target(password_flow_debug "password_flow.c" "Debug" "-fsanitize=address,leak -fno-omit-frame-pointer" "-lasan -fsanitize=address,leak" "${EXAMPLE_LIBS}")

# Target to build all examples
add_custom_target(all_examples
    DEPENDS auth_code_flow client_credentials password_flow
    COMMAND ${CMAKE_COMMAND} -E echo "✅ All examples built successfully"
    COMMENT "Building all OIDC client examples"
)

# Optimized all_variants target with better parallelization
add_custom_target(all_variants
    COMMAND ${CMAKE_COMMAND} -E echo "────────────────────────────────────────────────────────────────"
    COMMAND ${CMAKE_COMMAND} -E echo "🛈 Building all variants with ${PARALLEL_JOBS} parallel jobs..."
    COMMAND ${CMAKE_COMMAND} -E echo "🛈 Phase 1: Building independent targets in parallel..."
    
    # Phase 1: Build independent targets that can run in parallel
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_CURRENT_BINARY_DIR} --target regular debug valgrind perf --parallel ${PARALLEL_JOBS}
    
    # Phase 2: Build coverage and unity tests (can run in parallel with each other)
    COMMAND ${CMAKE_COMMAND} -E echo "🛈 Phase 2: Building coverage and unity tests..."
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_CURRENT_BINARY_DIR} --target hydrogen_coverage unity_tests --parallel ${PARALLEL_JOBS}
    
    # Phase 3: Build payload (quick step)
    COMMAND ${CMAKE_COMMAND} -E echo "🛈 Phase 3: Preparing payload..."
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_CURRENT_BINARY_DIR} --target payload
    
    # Phase 4: Build targets that depend on payload
    COMMAND ${CMAKE_COMMAND} -E echo "🛈 Phase 4: Building release and coverage with payload..."
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_CURRENT_BINARY_DIR} --target coverage hydrogen_release --parallel ${PARALLEL_JOBS}
    
    COMMAND ${CMAKE_COMMAND} -E echo "✅ All variants built successfully!"
    COMMAND ${CMAKE_COMMAND} -E echo "────────────────────────────────────────────────────────────────"
    COMMAND ${CMAKE_COMMAND} -E echo "  Default:     $<TARGET_FILE:regular>"
    COMMAND ${CMAKE_COMMAND} -E echo "  Debug:       $<TARGET_FILE:debug>"
    COMMAND ${CMAKE_COMMAND} -E echo "  Valgrind:    $<TARGET_FILE:valgrind>"
    COMMAND ${CMAKE_COMMAND} -E echo "  Performance: $<TARGET_FILE:perf>"
    COMMAND ${CMAKE_COMMAND} -E echo "  Coverage:    ${CMAKE_CURRENT_SOURCE_DIR}/../hydrogen_coverage"
    COMMAND ${CMAKE_COMMAND} -E echo "  Release:     ${CMAKE_CURRENT_SOURCE_DIR}/../hydrogen_release"
    COMMAND ${CMAKE_COMMAND} -E echo "  Naked:       ${CMAKE_CURRENT_SOURCE_DIR}/../hydrogen_naked"
    COMMAND ${CMAKE_COMMAND} -E echo "  Unity Tests: ${CMAKE_CURRENT_SOURCE_DIR}/../build/unity/src/"
    COMMAND ${CMAKE_COMMAND} -E echo "────────────────────────────────────────────────────────────────"
    COMMENT "Building all variants with optimized parallel execution (${PARALLEL_JOBS} jobs)"
)

# Trial build target
add_custom_target(trial
#    COMMAND ${CMAKE_COMMAND} -E echo "Starting trial build..."
    COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/scripts/trial_build.sh" "${CMAKE_CURRENT_BINARY_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}"
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Running trial build with diagnostics"
)

# Clean targets
add_custom_target(clean_all
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}
    COMMAND ${CMAKE_COMMAND} -E echo "${GREEN}${PASS} Clean completed successfully${NORMAL}"
    COMMENT "Cleaning all build artifacts"
)

add_custom_target(cleanish
    COMMAND ${CMAKE_COMMAND} -E remove -f $<TARGET_FILE:regular>
    COMMAND ${CMAKE_COMMAND} -E remove -f $<TARGET_FILE:debug>
    COMMAND ${CMAKE_COMMAND} -E remove -f $<TARGET_FILE:valgrind>
    COMMAND ${CMAKE_COMMAND} -E remove -f $<TARGET_FILE:perf>
    COMMAND ${CMAKE_COMMAND} -E remove -f $<TARGET_FILE:hydrogen_coverage>
    COMMAND ${CMAKE_COMMAND} -E remove -f $<TARGET_FILE:auth_code_flow>
    COMMAND ${CMAKE_COMMAND} -E remove -f $<TARGET_FILE:client_credentials>
    COMMAND ${CMAKE_COMMAND} -E remove -f $<TARGET_FILE:password_flow>
    COMMAND ${CMAKE_COMMAND} -E remove -f $<TARGET_FILE:auth_code_flow_debug>
    COMMAND ${CMAKE_COMMAND} -E remove -f $<TARGET_FILE:client_credentials_debug>
    COMMAND ${CMAKE_COMMAND} -E remove -f $<TARGET_FILE:password_flow_debug>
    COMMAND ${CMAKE_COMMAND} -E remove -f ${CMAKE_CURRENT_BINARY_DIR}/*.map
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_CURRENT_SOURCE_DIR}/../build
    COMMAND ${CMAKE_COMMAND} -E echo "✅ Cleanish completed successfully - preserved hydrogen_release, hydrogen_naked, and hydrogen_coverage, removed build directories"
    COMMENT "Cleaning build artifacts (preserving release executables and coverage)"
)

# Help target
add_custom_target(cmake_help
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "Hydrogen CMake Build System - Optimized"
    COMMAND ${CMAKE_COMMAND} -E echo "CPU Cores: ${CPU_CORES}, Parallel Jobs: ${PARALLEL_JOBS}"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "Available targets:"
    COMMAND ${CMAKE_COMMAND} -E echo "  hydrogen          - Build default version with standard optimizations"
    COMMAND ${CMAKE_COMMAND} -E echo "  hydrogen_debug    - Build with AddressSanitizer for debugging"
    COMMAND ${CMAKE_COMMAND} -E echo "  hydrogen_valgrind - Build optimized for Valgrind analysis"
    COMMAND ${CMAKE_COMMAND} -E echo "  hydrogen_perf     - Build with aggressive optimizations"
    COMMAND ${CMAKE_COMMAND} -E echo "  hydrogen_coverage - Build with gcov coverage instrumentation"
    COMMAND ${CMAKE_COMMAND} -E echo "  coverage          - Build coverage executable with embedded payload"
    COMMAND ${CMAKE_COMMAND} -E echo "  hydrogen_release  - Build stripped, compressed executable"
    COMMAND ${CMAKE_COMMAND} -E echo "  unity_tests       - Build Unity unit tests with coverage"
    COMMAND ${CMAKE_COMMAND} -E echo "  payload           - Generate OpenAPI specs and payload"
    COMMAND ${CMAKE_COMMAND} -E echo "  all_variants      - Build all variants with optimized parallelization"
    COMMAND ${CMAKE_COMMAND} -E echo "  trial             - Run trial build with diagnostics"
    COMMAND ${CMAKE_COMMAND} -E echo "  auth_code_flow    - Build Authorization Code flow example"
    COMMAND ${CMAKE_COMMAND} -E echo "  client_credentials - Build Client Credentials flow example"
    COMMAND ${CMAKE_COMMAND} -E echo "  password_flow     - Build Resource Owner Password flow example"
    COMMAND ${CMAKE_COMMAND} -E echo "  all_examples      - Build all example programs"
    COMMAND ${CMAKE_COMMAND} -E echo "  clean_all         - Remove all build artifacts"
    COMMAND ${CMAKE_COMMAND} -E echo "  cleanish          - Clean preserving release executables"
    COMMAND ${CMAKE_COMMAND} -E echo "  cmake_help        - Display this help information"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "Performance Tips:"
    COMMAND ${CMAKE_COMMAND} -E echo "  • Use -G Ninja for faster builds: cmake -G Ninja .."
    COMMAND ${CMAKE_COMMAND} -E echo "  • Parallel builds: cmake --build . --parallel ${PARALLEL_JOBS}"
    COMMAND ${CMAKE_COMMAND} -E echo "  • Install ccache for faster rebuilds"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "Examples:"
    COMMAND ${CMAKE_COMMAND} -E echo "  cmake --build . --target hydrogen --parallel ${PARALLEL_JOBS}"
    COMMAND ${CMAKE_COMMAND} -E echo "  cmake --build . --target all_variants"
    COMMAND ${CMAKE_COMMAND} -E echo "  cmake --build . --target trial"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMENT "Displaying optimized build system help information"
)

# Unity Tests Integration
#
# This section integrates Unity unit tests directly into the main build system
# instead of using a separate CMakeLists.txt file. This allows for parallel builds
# and better integration while maintaining the separate build/unity/src structure.

# Unity framework path
set(UNITY_FRAMEWORK_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../tests/unity/framework/Unity")

# Unity test source files - search recursively to support directory structure
file(GLOB_RECURSE UNITY_TEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../tests/unity/src/*_test*.c")

# Unity framework source
set(UNITY_FRAMEWORK_SOURCES
    ${UNITY_FRAMEWORK_DIR}/src/unity.c
)

# Create Unity test object files in build/unity/src directory structure (same as original)
# Include hydrogen.c for coverage instrumentation consistency, but don't link it into test executables
set(UNITY_HYDROGEN_SOURCES ${HYDROGEN_SOURCES})
set(UNITY_HYDROGEN_SOURCES_FOR_LINKING ${HYDROGEN_SOURCES})
list(REMOVE_ITEM UNITY_HYDROGEN_SOURCES_FOR_LINKING "${CMAKE_CURRENT_SOURCE_DIR}/../src/hydrogen.c")

set(UNITY_OBJECT_FILES "")
set(UNITY_OBJECT_FILES_FOR_LINKING "")
foreach(SOURCE_FILE ${UNITY_HYDROGEN_SOURCES})
    # Get relative path from src directory
    file(RELATIVE_PATH REL_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../src" ${SOURCE_FILE})
    get_filename_component(OBJ_DIR ${REL_PATH} DIRECTORY)
    get_filename_component(OBJ_BASENAME ${REL_PATH} NAME_WE)
    
    # Set up output directory structure to mirror source in build/unity/src/
    if(OBJ_DIR)
        set(OUTPUT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../build/unity/src/${OBJ_DIR}")
        set(OUTPUT_OBJ "${OUTPUT_DIR}/${OBJ_BASENAME}.o")
    else()
        set(OUTPUT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../build/unity/src")
        set(OUTPUT_OBJ "${OUTPUT_DIR}/${OBJ_BASENAME}.o")
    endif()
    
    # Create custom command to compile source file to object file with Unity-specific flags
    add_custom_command(
        OUTPUT ${OUTPUT_OBJ}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${OUTPUT_DIR}
        COMMAND ${CMAKE_C_COMPILER}
            ${HYDROGEN_BASE_CFLAGS}
            -O0 -g3 -ggdb3 --coverage -fprofile-arcs -ftest-coverage -fno-omit-frame-pointer
            -DVERSION='"${HYDROGEN_VERSION}"'
            -DRELEASE='"${HYDROGEN_RELEASE}"'
            -DBUILD_TYPE='"Coverage"'
            -I${CMAKE_CURRENT_SOURCE_DIR}/../src
            -I${UNITY_FRAMEWORK_DIR}/src
            ${JANSSON_CFLAGS}
            ${MICROHTTPD_CFLAGS}
            ${WEBSOCKETS_CFLAGS}
            ${BROTLI_CFLAGS}
            -c ${SOURCE_FILE} -o ${OUTPUT_OBJ}
        DEPENDS ${SOURCE_FILE}
        COMMENT "Compiling ${REL_PATH} to Unity object file"
    )
    
    list(APPEND UNITY_OBJECT_FILES ${OUTPUT_OBJ})
    
    # Only add to linking list if not hydrogen.c (has main() function)
    if(NOT SOURCE_FILE STREQUAL "${CMAKE_CURRENT_SOURCE_DIR}/../src/hydrogen.c")
        list(APPEND UNITY_OBJECT_FILES_FOR_LINKING ${OUTPUT_OBJ})
    endif()
endforeach()

# Create target that depends on all Unity object files
add_custom_target(unity_objects DEPENDS ${UNITY_OBJECT_FILES})

# Create Unity test executables for each test file using custom build structure
foreach(TEST_SOURCE ${UNITY_TEST_SOURCES})
    get_filename_component(TEST_NAME ${TEST_SOURCE} NAME_WE)
    
    # Get relative path from tests/unity/src to determine output directory structure
    file(RELATIVE_PATH TEST_REL_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../tests/unity/src" ${TEST_SOURCE})
    get_filename_component(TEST_DIR ${TEST_REL_PATH} DIRECTORY)
    
    # Determine output directory - mirror source structure in build/unity/src
    if(TEST_DIR AND NOT TEST_DIR STREQUAL ".")
        set(TEST_OUTPUT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../build/unity/src/${TEST_DIR}")
    else()
        set(TEST_OUTPUT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../build/unity/src")
    endif()
    
    # Create Unity framework object files in build/unity/src directory structure
    set(UNITY_FRAMEWORK_OBJECT_FILES "")
    foreach(UNITY_SRC ${UNITY_FRAMEWORK_SOURCES})
        get_filename_component(UNITY_BASENAME ${UNITY_SRC} NAME_WE)
        set(UNITY_OUTPUT_OBJ "${CMAKE_CURRENT_SOURCE_DIR}/../build/unity/src/${UNITY_BASENAME}.o")
        
        # Create custom command to compile Unity framework source file to object file
        add_custom_command(
            OUTPUT ${UNITY_OUTPUT_OBJ}
            COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_SOURCE_DIR}/../build/unity/src
            COMMAND ${CMAKE_C_COMPILER}
                ${HYDROGEN_BASE_CFLAGS}
                -O0 -g3 -ggdb3 --coverage -fprofile-arcs -ftest-coverage -fno-omit-frame-pointer
                -I${UNITY_FRAMEWORK_DIR}/src
                -c ${UNITY_SRC} -o ${UNITY_OUTPUT_OBJ}
            DEPENDS ${UNITY_SRC}
            COMMENT "Compiling Unity framework ${UNITY_BASENAME} to object file"
        )
        
        list(APPEND UNITY_FRAMEWORK_OBJECT_FILES ${UNITY_OUTPUT_OBJ})
    endforeach()
    
    # Create test source object file in the appropriate subdirectory
    get_filename_component(TEST_BASENAME ${TEST_SOURCE} NAME_WE)
    if(TEST_DIR AND NOT TEST_DIR STREQUAL ".")
        set(TEST_OUTPUT_OBJ "${CMAKE_CURRENT_SOURCE_DIR}/../build/unity/src/${TEST_DIR}/${TEST_BASENAME}.o")
    else()
        set(TEST_OUTPUT_OBJ "${CMAKE_CURRENT_SOURCE_DIR}/../build/unity/src/${TEST_BASENAME}.o")
    endif()
    
    # Create custom command to compile test source file to object file
    add_custom_command(
        OUTPUT ${TEST_OUTPUT_OBJ}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${TEST_OUTPUT_DIR}
        COMMAND ${CMAKE_C_COMPILER}
            ${HYDROGEN_BASE_CFLAGS}
            -O0 -g3 -ggdb3 --coverage -fprofile-arcs -ftest-coverage -fno-omit-frame-pointer
            -DVERSION='"${HYDROGEN_VERSION}"'
            -DRELEASE='"${HYDROGEN_RELEASE}"'
            -DBUILD_TYPE='"Coverage"'
            -I${CMAKE_CURRENT_SOURCE_DIR}/../src
            -I${UNITY_FRAMEWORK_DIR}/src
            ${JANSSON_CFLAGS}
            ${MICROHTTPD_CFLAGS}
            ${WEBSOCKETS_CFLAGS}
            ${BROTLI_CFLAGS}
            -c ${TEST_SOURCE} -o ${TEST_OUTPUT_OBJ}
        DEPENDS ${TEST_SOURCE}
        COMMENT "Compiling ${TEST_BASENAME} test to object file"
    )
    
    # Create target that depends on all object files for this test
    add_custom_target(${TEST_NAME}_objects
        DEPENDS ${TEST_OUTPUT_OBJ} ${UNITY_FRAMEWORK_OBJECT_FILES} ${UNITY_OBJECT_FILES}
    )
    
    # Create the Unity test executable using the individual object files
    add_executable(${TEST_NAME}
        ${TEST_OUTPUT_OBJ}
        ${UNITY_FRAMEWORK_OBJECT_FILES}
    )
    add_dependencies(${TEST_NAME} ${TEST_NAME}_objects unity_objects)
    
    # Link with Unity object files and required libraries (excluding hydrogen.c which has main())
    target_link_libraries(${TEST_NAME}
        ${UNITY_OBJECT_FILES_FOR_LINKING}
        ${HYDROGEN_BASE_LIBS}
        --coverage
        -lgcov
    )
    
    # Add extra link flags
    target_link_options(${TEST_NAME} PRIVATE -Wl,--gc-sections)
    
    # Generate map file in the appropriate subdirectory
    target_link_options(${TEST_NAME} PRIVATE -Wl,-Map=${TEST_OUTPUT_DIR}/${TEST_NAME}.map)
    
    # Set output directory for test executable to mirror source structure
    set_target_properties(${TEST_NAME} PROPERTIES
        OUTPUT_NAME ${TEST_NAME}
        RUNTIME_OUTPUT_DIRECTORY "${TEST_OUTPUT_DIR}"
        LINKER_LANGUAGE C
    )
    
    # Add custom command to show build completion
    add_custom_command(TARGET ${TEST_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "${GREEN}${PASS} ${BOLD}Unity test build${NORMAL} ${GREEN}completed successfully: ${TEST_NAME}${NORMAL}"
        COMMAND ${CMAKE_COMMAND} -E echo "${CYAN}${INFO} Unity test artifacts located in: ${TEST_OUTPUT_DIR}/${NORMAL}"
        VERBATIM
    )
    
    # Add test to CTest
    add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME}
        WORKING_DIRECTORY ${TEST_OUTPUT_DIR}
    )
endforeach()

# Create list of all Unity test targets
set(UNITY_TEST_TARGETS "")
foreach(TEST_SOURCE ${UNITY_TEST_SOURCES})
    get_filename_component(TEST_NAME ${TEST_SOURCE} NAME_WE)
    list(APPEND UNITY_TEST_TARGETS ${TEST_NAME})
endforeach()

# Unity tests build target
add_custom_target(unity_tests
    DEPENDS ${UNITY_TEST_TARGETS}
    COMMAND ${CMAKE_COMMAND} -E echo "🛈  Building Unity tests with coverage instrumentation..."
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_CURRENT_BINARY_DIR} --target unity_objects --parallel
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_CURRENT_BINARY_DIR} --target ${UNITY_TEST_TARGETS} --parallel
    COMMAND ${CMAKE_COMMAND} -E echo "✅ Unity tests built successfully"
    COMMAND ${CMAKE_COMMAND} -E echo "🛈 Unity coverage files located in: ${CMAKE_CURRENT_SOURCE_DIR}/../build/unity/src/"
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Building Unity tests with coverage instrumentation"
)

# Enable testing
enable_testing()

# Add tests
add_test(NAME hydrogen_basic_test
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/../tests/22_startup_shutdown.sh
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

add_test(NAME hydrogen_compilation_test
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/../tests/test_01_compilation.sh
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

add_test(NAME hydrogen_dependencies_test
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/../tests/test_16_library_dependencies.sh
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

add_test(NAME hydrogen_unity_tests
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/../tests/test_11_unity.sh
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

# Set test properties
set_tests_properties(
    hydrogen_basic_test
    hydrogen_compilation_test
    hydrogen_dependencies_test
    PROPERTIES
    TIMEOUT 300
    ENVIRONMENT "CMAKE_BUILD=1"
)

# Install targets
install(TARGETS regular debug valgrind perf release
    RUNTIME DESTINATION bin
    COMPONENT Runtime
)

# CPack configuration for packaging
set(CPACK_PACKAGE_NAME "Hydrogen")
set(CPACK_PACKAGE_VERSION "${HYDROGEN_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Hydrogen OIDC Server")
set(CPACK_PACKAGE_VENDOR "Philement")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/../../../../LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/../README.md")

include(CPack)

# Print configuration summary
message(STATUS "")
message(STATUS "Hydrogen Build Configuration Summary:")
message(STATUS "  Version: ${HYDROGEN_VERSION}")
message(STATUS "  Release: ${HYDROGEN_RELEASE}")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  C Compiler: ${CMAKE_C_COMPILER}")
message(STATUS "  Install Prefix: ${CMAKE_INSTALL_PREFIX}")
if(UPX_EXECUTABLE)
    message(STATUS "  UPX Available: ${UPX_EXECUTABLE}")
else()
    message(STATUS "  UPX Available: No")
endif()
if(VALGRIND_EXECUTABLE)
    message(STATUS "  Valgrind Available: ${VALGRIND_EXECUTABLE}")
else()
    message(STATUS "  Valgrind Available: No")
endif()
message(STATUS "")
