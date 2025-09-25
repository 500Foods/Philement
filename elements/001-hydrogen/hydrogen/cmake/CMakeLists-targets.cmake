# Build Targets
#
# This file contains the various build targets for Hydrogen.

# Resolve paths for consistent display
set(HYDROGEN_DIR "${CMAKE_CURRENT_SOURCE_DIR}/..")
get_filename_component(HYDROGEN_DIR "${HYDROGEN_DIR}" ABSOLUTE)

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

# Optimized all_variants target with better parallelization
add_custom_target(all_variants
    COMMAND ${CMAKE_COMMAND} -E echo "────────────────────────────────────────────────────────────────"
    COMMAND ${CMAKE_COMMAND} -E echo "🛈 Building all variants with ${PARALLEL_JOBS} parallel jobs..."
    COMMAND ${CMAKE_COMMAND} -E echo "🛈 Phase 1: Building independent targets in parallel..."

    # Phase 1: Build independent targets that can run in parallel
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_CURRENT_BINARY_DIR} --target debug valgrind perf --parallel ${PARALLEL_JOBS}

    # Phase 2: Build coverage and unity tests (can run in parallel with each other)
    COMMAND ${CMAKE_COMMAND} -E echo "🛈 Phase 2: Building coverage and unity tests..."
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_CURRENT_BINARY_DIR} --target hydrogen_coverage unity_tests --parallel ${PARALLEL_JOBS}

    # Phase 3: Build payload (quick step)
    COMMAND ${CMAKE_COMMAND} -E echo "🛈 Phase 3: Preparing payload..."
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_CURRENT_BINARY_DIR} --target payload

    # Phase 4: Build targets that depend on payload
    COMMAND ${CMAKE_COMMAND} -E echo "🛈 Phase 4: Building regular, release and coverage with payload..."
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_CURRENT_BINARY_DIR} --target hydrogen coverage hydrogen_release --parallel ${PARALLEL_JOBS}

    COMMAND ${CMAKE_COMMAND} -E echo "✅ All variants built successfully!"
    COMMAND ${CMAKE_COMMAND} -E echo "────────────────────────────────────────────────────────────────"
    COMMAND ${CMAKE_COMMAND} -E echo "  Default:     ${HYDROGEN_DIR}/hydrogen"
    COMMAND ${CMAKE_COMMAND} -E echo "  Debug:       $<TARGET_FILE:debug>"
    COMMAND ${CMAKE_COMMAND} -E echo "  Valgrind:    $<TARGET_FILE:valgrind>"
    COMMAND ${CMAKE_COMMAND} -E echo "  Performance: $<TARGET_FILE:perf>"
    COMMAND ${CMAKE_COMMAND} -E echo "  Coverage:    $<TARGET_FILE:hydrogen_coverage>"
    COMMAND ${CMAKE_COMMAND} -E echo "  Release:     ${HYDROGEN_DIR}/hydrogen_release"
    COMMAND ${CMAKE_COMMAND} -E echo "  Naked:       ${HYDROGEN_DIR}/hydrogen_naked"
    COMMAND ${CMAKE_COMMAND} -E echo "  Unity Tests: ${HYDROGEN_DIR}/build/unity/src/"
    COMMAND ${CMAKE_COMMAND} -E echo "────────────────────────────────────────────────────────────────"
    COMMENT "Building all variants with optimized parallel execution (${PARALLEL_JOBS} jobs)"
)

# Trial build target
add_custom_target(trial
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
    COMMAND ${CMAKE_COMMAND} -E remove -f ${HYDROGEN_DIR}/hydrogen
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
    COMMAND ${CMAKE_COMMAND} -E echo "  hydrogen          - Build default version with standard optimizations and embedded payload"
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