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
# cmake --build . --target configure_ccache  : Configure ccache with optimal settings for coverage builds
# cmake --build . --target ccache_stats      : Show ccache statistics and performance metrics

# CHANGELOG
# 2025-12-05: Cleaned up the README.md file to better reflect the current state of the Hydrogen build system
# 2025-12-02: Fixed coverage build GDB debugging: removed -no-pie to enable PIE for proper core dump analysis
# 2025-11-05: Fixed coverage build GDB debugging: replaced -no-pie with -rdynamic to export dynamic symbols for proper crash dump analysis
# 2025-10-10: Updated target_include_directories so we can use <src/...> and <unity...> instead of things like "../../../../src/hydrogen.h"
# 2025-09-22: Major refactor: split into more than a dozen separate .cmake files
# 2025-09-21: Added mock libraries to unity build profile, including mock_system, mock_threads, and mock_network, joining mock_info, mock_launch, mock_landing, mock_libmicrohttpd, mock_libwebsockets and mock_status
# 2025-09-18: Added hydrogen_unity static library with precompiled headers for src/hydrogen.h to improve Unity test build performance and ccache effectiveness. Unity tests now link to static library instead of individual object files. Added ccache optimization targets (configure_ccache, ccache_stats) with optimal settings for coverage builds.
# 2025-08-17: Added additional compiler flags for better warnings and error checking when building with clang on macOS.
# 2025-07-01: Added targets for building OIDC client example programs (auth_code_flow, client_credentials, password_flow) with regular and debug builds. Updated help and clean targets to include examples.

# Minimum cmake version required
cmake_minimum_required(VERSION 3.16)

# Project definition
project(Hydrogen
    VERSION 1.0.0
    DESCRIPTION "Hydrogen Server"
    LANGUAGES C
)

# Include configuration modules
include("${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists-version.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists-init.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists-base.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists-cache.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists-ninja.cmake")


# Include build variant configurations
include("${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists-regular.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists-debug.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists-valgrind.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists-perf.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists-coverage.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists-release.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists-examples.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists-unity.cmake")

# Include targets and packaging
include("${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists-package.cmake")
include("${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists-targets.cmake")


include("${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists-output.cmake")
