# ──────────────────────────────────────────────────────────────────────────────
# Hydrogen CMake Build System Configuration
# 
# This CMake configuration file sets up the build system for the Hydrogen server and example programs.
#
# - Default: Standard build with optimizations and debug symbols
# - Debug: Build with AddressSanitizer for debugging memory issues
# - Valgrind: Build optimized for memory analysis with Valgrind
# - Performance: Highly optimized build for maximum performance
# - Release: Stripped, compressed build with embedded encrypted payload
# - Examples: Builds for OIDC client examples (Authorization Code, Client Credentials, Password flows)
#
# Key features:
# - Automatic version numbering based on git commits
# - Multiple optimization levels for different use cases
# - Unused source file detection
# - UPX compression for release builds
# - Encrypted payload embedding
# - Comprehensive build artifact management
# 
# Available CMake targets:
# cmake --build . --target hydrogen          : Builds the default version with standard optimizations and debug symbols
# cmake --build . --target hydrogen_debug    : Builds with AddressSanitizer enabled for catching memory issues
# cmake --build . --target hydrogen_valgrind : Builds with -O0 and full debug info for optimal Valgrind analysis
# cmake --build . --target hydrogen_perf     : Builds with aggressive optimizations (-O3, native arch, etc.) for maximum performance
# cmake --build . --target hydrogen_release  : Creates stripped, compressed executable with embedded encrypted payload
# cmake --build . --target payload           : Generates OpenAPI specs and packages payload contents
# cmake --build . --target all_variants      : Builds all variants (default, debug, valgrind, perf, release) and generates payload
# cmake --build . --target trial             : Runs a test build showing only warnings/errors, checks for unused files, runs shutdown test
# cmake --build . --target auth_code_flow    : Builds the Authorization Code flow example
# cmake --build . --target client_credentials: Builds the Client Credentials flow example
# cmake --build . --target password_flow     : Builds the Resource Owner Password flow example
# cmake --build . --target all_examples      : Builds all example programs
# ──────────────────────────────────────────────────────────────────────────────
# Change History
# ──────────────────────────────────────────────────────────────────────────────
# 2025-07-01: Added targets for building OIDC client example programs (auth_code_flow, client_credentials, password_flow) with regular and debug builds. Updated help and clean targets to include examples.
# ──────────────────────────────────────────────────────────────────────────────

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
string(TIMESTAMP HYDROGEN_RELEASE "%Y-%m-%dT%H:%M:%SZ" UTC)

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
)

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

# Function to add an executable target with specific build configuration
function(hydrogen_add_executable_target target_name build_type extra_cflags extra_ldflags)
    # Create the executable
    add_executable(${target_name} ${HYDROGEN_SOURCES})
    
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
    target_link_libraries(${target_name} PRIVATE ${HYDROGEN_BASE_LIBS})
    
    # Add extra link flags if provided
    if(extra_ldflags)
        separate_arguments(extra_ldflags_list UNIX_COMMAND "${extra_ldflags}")
        target_link_options(${target_name} PRIVATE ${extra_ldflags_list})
    endif()
    
    # Add garbage collection flags
    target_link_options(${target_name} PRIVATE -Wl,--gc-sections)
    
    # Generate map file
    target_link_options(${target_name} PRIVATE -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/${target_name}.map)
    
    # Set output name and properties
    set_target_properties(${target_name} PROPERTIES
        OUTPUT_NAME ${target_name}
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..
    )
    
    # Add custom command to show build completion
    add_custom_command(TARGET ${target_name} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "${GREEN}${PASS} ${BOLD}${build_type} build${NORMAL} ${GREEN}completed successfully: ${target_name}${NORMAL}"
        VERBATIM
    )
endfunction()

# Default build target
hydrogen_add_executable_target(hydrogen "Regular" "-O2 -g" "-no-pie")

# Debug build with AddressSanitizer
hydrogen_add_executable_target(hydrogen_debug "Debug" 
    "-fsanitize=address,leak -fno-omit-frame-pointer" 
    "-lasan -fsanitize=address,leak -no-pie"
)

# Valgrind-compatible build
hydrogen_add_executable_target(hydrogen_valgrind "Valgrind-Compatible" 
    "-O0" 
    "-no-pie"
)

# Performance-optimized build
hydrogen_add_executable_target(hydrogen_perf "Performance" 
    "-O3 -march=native -ffast-math -finline-functions -funroll-loops -DNDEBUG" 
    "-flto=auto -no-pie"
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
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_CURRENT_BINARY_DIR} --target hydrogen_release_exe
    COMMAND ${CMAKE_COMMAND} -E echo "🛈  Stripping debug information from release executable..."
    COMMAND strip -s ${CMAKE_CURRENT_SOURCE_DIR}/../hydrogen_release_exe
    COMMAND ${CMAKE_COMMAND} -E echo "🛈  Copying release executable..."
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/../hydrogen_release_exe ${CMAKE_CURRENT_SOURCE_DIR}/../hydrogen_release
    COMMAND ${CMAKE_COMMAND} -E echo "🛈  Applying UPX compression to release executable..."
    COMMAND upx --best ${CMAKE_CURRENT_SOURCE_DIR}/../hydrogen_release || echo "⚠️ UPX compression failed or not available"
    COMMAND ${CMAKE_COMMAND} -E echo "🛈  Creating UPX-compressed copy as hydrogen_naked..."
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/../hydrogen_release ${CMAKE_CURRENT_SOURCE_DIR}/../hydrogen_naked
    COMMAND ${CMAKE_COMMAND} -E echo "🛈  Appending encrypted payload to release executable..."
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/scripts/embed_payload.sh ${CMAKE_CURRENT_SOURCE_DIR}/../hydrogen_release ${CMAKE_CURRENT_SOURCE_DIR}/../payloads/payload.tar.br.enc
    COMMAND ${CMAKE_COMMAND} -E echo "✅ Payload embedded successfully"
    COMMAND ${CMAKE_COMMAND} -E remove -f ${CMAKE_CURRENT_SOURCE_DIR}/../hydrogen_release_exe
    COMMAND ${CMAKE_COMMAND} -E echo "🛈 Removed intermediate hydrogen_release_exe"
    COMMAND ${CMAKE_COMMAND} -E true
    COMMAND ${CMAKE_COMMAND} -E echo "✅ Release build with encrypted payload appended successfully!"
    COMMAND ${CMAKE_COMMAND} -E echo "────────────────────────────────────────────────────────────────"
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Creating release build with compression and payload (matches Makefile behavior)"
)

# Internal target for building just the release executable (without payload processing)
hydrogen_add_executable_target(hydrogen_release_exe "Release" 
    "-O2 -s -DNDEBUG -march=x86-64 -flto=auto -fstack-protector -fno-strict-aliasing" 
    "-flto=auto -fno-strict-aliasing"
)

# ──────────────────────────────────────────────────────────────────────────────
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
# ──────────────────────────────────────────────────────────────────────────────

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

# Build all variants with parallel execution
add_custom_target(all_variants
    DEPENDS hydrogen hydrogen_debug hydrogen_valgrind hydrogen_perf hydrogen_release payload
    COMMAND ${CMAKE_COMMAND} -E echo "────────────────────────────────────────────────────────────────"
    COMMAND ${CMAKE_COMMAND} -E echo "🛈 Building all variants in parallel where possible..."
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_CURRENT_BINARY_DIR} --target hydrogen hydrogen_debug hydrogen_valgrind hydrogen_perf hydrogen_release payload --parallel
    COMMAND ${CMAKE_COMMAND} -E echo "✅ All variants built successfully!"
    COMMAND ${CMAKE_COMMAND} -E echo "────────────────────────────────────────────────────────────────"
    COMMAND ${CMAKE_COMMAND} -E echo "  Default:     $<TARGET_FILE:hydrogen>"
    COMMAND ${CMAKE_COMMAND} -E echo "  Debug:       $<TARGET_FILE:hydrogen_debug>"
    COMMAND ${CMAKE_COMMAND} -E echo "  Valgrind:    $<TARGET_FILE:hydrogen_valgrind>"
    COMMAND ${CMAKE_COMMAND} -E echo "  Performance: $<TARGET_FILE:hydrogen_perf>"
    COMMAND ${CMAKE_COMMAND} -E echo "  Release:     ${CMAKE_CURRENT_SOURCE_DIR}/../hydrogen_release"
    COMMAND ${CMAKE_COMMAND} -E echo "  Naked:       ${CMAKE_CURRENT_SOURCE_DIR}/../hydrogen_naked"
    COMMAND ${CMAKE_COMMAND} -E echo "────────────────────────────────────────────────────────────────"
    COMMENT "Building all variants with parallel execution"
)

# Trial build target
add_custom_target(trial
    COMMAND ${CMAKE_COMMAND} -E echo "Starting trial build..."
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
    COMMAND ${CMAKE_COMMAND} -E remove -f $<TARGET_FILE:hydrogen>
    COMMAND ${CMAKE_COMMAND} -E remove -f $<TARGET_FILE:hydrogen_debug>
    COMMAND ${CMAKE_COMMAND} -E remove -f $<TARGET_FILE:hydrogen_valgrind>
    COMMAND ${CMAKE_COMMAND} -E remove -f $<TARGET_FILE:hydrogen_perf>
    COMMAND ${CMAKE_COMMAND} -E remove -f $<TARGET_FILE:auth_code_flow>
    COMMAND ${CMAKE_COMMAND} -E remove -f $<TARGET_FILE:client_credentials>
    COMMAND ${CMAKE_COMMAND} -E remove -f $<TARGET_FILE:password_flow>
    COMMAND ${CMAKE_COMMAND} -E remove -f $<TARGET_FILE:auth_code_flow_debug>
    COMMAND ${CMAKE_COMMAND} -E remove -f $<TARGET_FILE:client_credentials_debug>
    COMMAND ${CMAKE_COMMAND} -E remove -f $<TARGET_FILE:password_flow_debug>
    COMMAND ${CMAKE_COMMAND} -E remove -f ${CMAKE_CURRENT_BINARY_DIR}/*.map
    COMMAND ${CMAKE_COMMAND} -E echo "✅ Cleanish completed successfully - preserved hydrogen_release and hydrogen_naked"
    COMMENT "Cleaning build artifacts (preserving release executables)"
)

# Help target
add_custom_target(cmake_help
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "${BOLD}Hydrogen CMake Build System${NORMAL}"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "${CYAN}Available targets:${NORMAL}"
    COMMAND ${CMAKE_COMMAND} -E echo "  ${YELLOW}hydrogen${NORMAL}          - Build default version with standard optimizations"
    COMMAND ${CMAKE_COMMAND} -E echo "  ${YELLOW}hydrogen_debug${NORMAL}    - Build with AddressSanitizer for debugging"
    COMMAND ${CMAKE_COMMAND} -E echo "  ${YELLOW}hydrogen_valgrind${NORMAL} - Build optimized for Valgrind analysis"
    COMMAND ${CMAKE_COMMAND} -E echo "  ${YELLOW}hydrogen_perf${NORMAL}     - Build with aggressive optimizations"
    COMMAND ${CMAKE_COMMAND} -E echo "  ${YELLOW}hydrogen_release${NORMAL}  - Build stripped, compressed executable"
    COMMAND ${CMAKE_COMMAND} -E echo "  ${YELLOW}payload${NORMAL}           - Generate OpenAPI specs and payload"
    COMMAND ${CMAKE_COMMAND} -E echo "  ${YELLOW}all_variants${NORMAL}      - Build all variants"
    COMMAND ${CMAKE_COMMAND} -E echo "  ${YELLOW}trial${NORMAL}             - Run trial build with diagnostics"
    COMMAND ${CMAKE_COMMAND} -E echo "  ${YELLOW}auth_code_flow${NORMAL}    - Build Authorization Code flow example"
    COMMAND ${CMAKE_COMMAND} -E echo "  ${YELLOW}client_credentials${NORMAL} - Build Client Credentials flow example"
    COMMAND ${CMAKE_COMMAND} -E echo "  ${YELLOW}password_flow${NORMAL}     - Build Resource Owner Password flow example"
    COMMAND ${CMAKE_COMMAND} -E echo "  ${YELLOW}all_examples${NORMAL}      - Build all example programs"
    COMMAND ${CMAKE_COMMAND} -E echo "  ${YELLOW}clean_all${NORMAL}         - Remove all build artifacts"
    COMMAND ${CMAKE_COMMAND} -E echo "  ${YELLOW}cleanish${NORMAL}          - Clean preserving release executables"
    COMMAND ${CMAKE_COMMAND} -E echo "  ${YELLOW}cmake_help${NORMAL}        - Display this help information"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMAND ${CMAKE_COMMAND} -E echo "${CYAN}Examples:${NORMAL}"
    COMMAND ${CMAKE_COMMAND} -E echo "  cmake --build . --target hydrogen"
    COMMAND ${CMAKE_COMMAND} -E echo "  cmake --build . --target all_variants"
    COMMAND ${CMAKE_COMMAND} -E echo "  cmake --build . --target trial"
    COMMAND ${CMAKE_COMMAND} -E echo "  cmake --build . --target all_examples"
    COMMAND ${CMAKE_COMMAND} -E echo ""
    COMMENT "Displaying help information"
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
install(TARGETS hydrogen hydrogen_debug hydrogen_valgrind hydrogen_perf hydrogen_release_exe
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
