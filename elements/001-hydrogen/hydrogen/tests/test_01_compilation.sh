#!/bin/bash

# Test: Compilation
# Tests compilation of the project using CMake

# This test verifies that the project can be compiled successfully
# using the CMake build system with various configurations.

# CHANGELOG
# 2.0.1 - 2025-07-06 - Added missing shellcheck justifications
# 2.0.0 - 2025-07-02 - Complete rewrite to use new modular test libraries
# 1.0.0 - Initial version

# Test configuration
TEST_NAME="Compilation"
SCRIPT_VERSION="2.0.1"

# Get the directory where this script is located
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

# Only source tables.sh if not already loaded (check guard variable)
if [[ -z "$TABLES_SH_GUARD" ]] || ! command -v tables_render_from_json >/dev/null 2>&1; then
    # shellcheck source=tests/lib/tables.sh # Resolve path statically
    source "$SCRIPT_DIR/lib/tables.sh"
    export TABLES_SOURCED=true
fi

# Only source log_output.sh if not already loaded (check guard variable)
if [[ -z "$LOG_OUTPUT_SH_GUARD" ]]; then
    # shellcheck source=tests/lib/log_output.sh # Resolve path statically
    source "$SCRIPT_DIR/lib/log_output.sh"
fi

# Source other modular test libraries (always needed, not provided by test suite)
# shellcheck source=tests/lib/file_utils.sh # Resolve path statically
source "$SCRIPT_DIR/lib/file_utils.sh"
# shellcheck source=tests/lib/framework.sh # Resolve path statically
source "$SCRIPT_DIR/lib/framework.sh"

# Test configuration
EXIT_CODE=0
TOTAL_SUBTESTS=13
PASS_COUNT=0

# Auto-extract test number and set up environment
TEST_NUMBER=$(extract_test_number "${BASH_SOURCE[0]}")
set_test_number "$TEST_NUMBER"
reset_subtest_counter

# Print beautiful test header
print_test_header "$TEST_NAME" "$SCRIPT_VERSION"

# Set up results directory
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
RESULTS_DIR="$SCRIPT_DIR/results"
mkdir -p "$RESULTS_DIR"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
RESULT_LOG="$RESULTS_DIR/test_${TEST_NUMBER}_${TIMESTAMP}.log"

# Navigate to the project root (one level up from tests directory)
if ! navigate_to_project_root "$SCRIPT_DIR"; then
    print_error "Failed to navigate to project root directory"
    exit 1
fi

# Subtest: Check CMake availability
next_subtest
print_subtest "Check CMake Availability"
print_command "command -v cmake"
if command -v cmake >/dev/null 2>&1; then
    CMAKE_VERSION=$(cmake --version | head -n1)
    print_result 0 "CMake is available: $CMAKE_VERSION"
else
    print_result 1 "CMake is not available - required for compilation"
    EXIT_CODE=1
fi
evaluate_test_result_silent "CMake availability" "$EXIT_CODE" "PASS_COUNT" "EXIT_CODE"

# Subtest: Check for CMakeLists.txt
next_subtest
print_subtest "Check CMakeLists.txt"
print_command "test -f cmake/CMakeLists.txt"
if [ -f "cmake/CMakeLists.txt" ]; then
    file_size=$(get_file_size "cmake/CMakeLists.txt")
    formatted_size=$(format_file_size "$file_size")
    print_result 0 "CMakeLists.txt found in cmake directory (${formatted_size} bytes)"
else
    print_result 1 "CMakeLists.txt not found in cmake directory"
    EXIT_CODE=1
fi
evaluate_test_result_silent "CMakeLists.txt check" "$EXIT_CODE" "PASS_COUNT" "EXIT_CODE"

# Subtest: Check source files
next_subtest
print_subtest "Check Source Files"
print_command "test -d src && test -f src/hydrogen.c"
if [ -d "src" ] && [ -f "src/hydrogen.c" ]; then
    src_count=$(find src -name "*.c" -o -name "*.h" | wc -l)
    print_result 0 "Source directory found with $src_count C/H files"
else
    print_result 1 "Source files not found - src/hydrogen.c missing"
    EXIT_CODE=1
fi
evaluate_test_result_silent "Source files check" "$EXIT_CODE" "PASS_COUNT" "EXIT_CODE"

# Subtest: Create build directory
next_subtest
print_subtest "Create Build Directory"
print_command "mkdir -p build"
if mkdir -p build 2>/dev/null; then
    print_result 0 "Build directory created/verified as sibling to cmake"
else
    print_result 1 "Failed to create build directory"
    EXIT_CODE=1
fi
evaluate_test_result_silent "Create build directory" "$EXIT_CODE" "PASS_COUNT" "EXIT_CODE"

# Subtest: Configure with CMake
next_subtest
print_subtest "CMake Configuration"
print_command "cd cmake"
if safe_cd cmake; then
    print_command "cmake -S . -B ../build --preset default"
    if cmake -S . -B ../build --preset default >/dev/null 2>&1; then
        print_result 0 "CMake configuration successful with preset default"
    else
        EXIT_CODE=1
        print_result 1 "CMake configuration failed"
    fi
    print_command "cd .."
    safe_cd ..
else
    print_result 1 "Failed to enter cmake directory"
    EXIT_CODE=1
fi
evaluate_test_result_silent "CMake configuration" "$EXIT_CODE" "PASS_COUNT" "EXIT_CODE"

# Subtest: Build all variants
next_subtest
print_subtest "Build All Variants"
print_command "cd cmake"
if safe_cd cmake; then
    print_command "cmake --build ../build --preset default --target all_variants"
    if cmake --build ../build --preset default --target all_variants >/dev/null 2>&1; then
        print_result 0 "All variants build successful with preset default"
    else
        EXIT_CODE=1
        print_result 1 "Build of all variants failed"
    fi
    print_command "cd .."
    safe_cd ..
else
    print_result 1 "Failed to enter cmake directory for building all variants"
    EXIT_CODE=1
fi
evaluate_test_result_silent "Build all variants" "$EXIT_CODE" "PASS_COUNT" "EXIT_CODE"

# Subtest: Verify default executable
next_subtest
print_subtest "Verify Default Executable"
print_command "test -f hydrogen"
if [ -f "hydrogen" ]; then
    exe_size=$(get_file_size "hydrogen")
    formatted_size=$(format_file_size "$exe_size")
    print_result 0 "Executable created: hydrogen (${formatted_size} bytes)"
else
    print_result 1 "Default executable not found after build"
    EXIT_CODE=1
fi
evaluate_test_result_silent "Verify default executable" "$EXIT_CODE" "PASS_COUNT" "EXIT_CODE"

# Subtest: Verify debug executable
next_subtest
print_subtest "Verify Debug Executable"
print_command "test -f hydrogen_debug"
if [ -f "hydrogen_debug" ]; then
    exe_size=$(get_file_size "hydrogen_debug")
    formatted_size=$(format_file_size "$exe_size")
    print_result 0 "Executable created: hydrogen_debug (${formatted_size} bytes)"
else
    print_result 1 "Debug executable not found after build"
    EXIT_CODE=1
fi
evaluate_test_result_silent "Verify debug executable" "$EXIT_CODE" "PASS_COUNT" "EXIT_CODE"

# Subtest: Verify performance executable
next_subtest
print_subtest "Verify Performance Executable"
print_command "test -f hydrogen_perf"
if [ -f "hydrogen_perf" ]; then
    exe_size=$(get_file_size "hydrogen_perf")
    formatted_size=$(format_file_size "$exe_size")
    print_result 0 "Executable created: hydrogen_perf (${formatted_size} bytes)"
else
    print_result 1 "Performance executable not found after build"
    EXIT_CODE=1
fi
evaluate_test_result_silent "Verify performance executable" "$EXIT_CODE" "PASS_COUNT" "EXIT_CODE"

# Subtest: Verify valgrind executable
next_subtest
print_subtest "Verify Valgrind Executable"
print_command "test -f hydrogen_valgrind"
if [ -f "hydrogen_valgrind" ]; then
    exe_size=$(get_file_size "hydrogen_valgrind")
    formatted_size=$(format_file_size "$exe_size")
    print_result 0 "Executable created: hydrogen_valgrind (${formatted_size} bytes)"
else
    print_result 1 "Valgrind executable not found after build"
    EXIT_CODE=1
fi
evaluate_test_result_silent "Verify valgrind executable" "$EXIT_CODE" "PASS_COUNT" "EXIT_CODE"

# Subtest: Verify release and naked executables
next_subtest
print_subtest "Verify Release and Naked Executables"
print_command "test -f hydrogen_release && test -f hydrogen_naked"
if [ -f "hydrogen_release" ] && [ -f "hydrogen_naked" ]; then
    release_size=$(get_file_size "hydrogen_release")
    naked_size=$(get_file_size "hydrogen_naked")
    formatted_release_size=$(format_file_size "$release_size")
    formatted_naked_size=$(format_file_size "$naked_size")
    print_result 0 "Executables created: hydrogen_release (${formatted_release_size} bytes), hydrogen_naked (${formatted_naked_size} bytes)"
else
    print_result 1 "Release or naked executable not found after build"
    EXIT_CODE=1
fi
evaluate_test_result_silent "Verify release and naked executables" "$EXIT_CODE" "PASS_COUNT" "EXIT_CODE"

# Subtest: Build examples
next_subtest
print_subtest "Build Examples"
print_command "cd cmake"
if safe_cd cmake; then
    print_command "cmake --build ../build --preset default --target all_examples"
    if cmake --build ../build --preset default --target all_examples >/dev/null 2>&1; then
        print_result 0 "Examples build successful with preset default"
    else
        EXIT_CODE=1
        print_result 1 "Examples build failed"
    fi
    print_command "cd .."
    safe_cd ..
else
    print_result 1 "Failed to enter cmake directory for examples build"
    EXIT_CODE=1
fi
evaluate_test_result_silent "Build examples" "$EXIT_CODE" "PASS_COUNT" "EXIT_CODE"

# Subtest: Verify examples executables
next_subtest
print_subtest "Verify Examples Executables"
print_command "test -f examples/C/auth_code_flow && test -f examples/C/client_credentials && test -f examples/C/password_flow"
if [ -f "examples/C/auth_code_flow" ] && [ -f "examples/C/client_credentials" ] && [ -f "examples/C/password_flow" ]; then
    auth_size=$(get_file_size "examples/C/auth_code_flow")
    client_size=$(get_file_size "examples/C/client_credentials")
    password_size=$(get_file_size "examples/C/password_flow")
    formatted_auth_size=$(format_file_size "$auth_size")
    formatted_client_size=$(format_file_size "$client_size")
    formatted_password_size=$(format_file_size "$password_size")
    print_result 0 "Executables created: auth_code_flow (${formatted_auth_size} bytes), client_credentials (${formatted_client_size} bytes), password_flow (${formatted_password_size} bytes)"
else
    print_result 1 "One or more examples executables not found after build"
    EXIT_CODE=1
fi
evaluate_test_result_silent "Verify examples executables" "$EXIT_CODE" "PASS_COUNT" "EXIT_CODE"

# Export results for test_all.sh integration
# Derive test name from script filename for consistency with test_00_all.sh
TEST_IDENTIFIER=$(basename "${BASH_SOURCE[0]}" .sh | sed 's/test_[0-9]*_//')
export_subtest_results "${TEST_NUMBER}_${TEST_IDENTIFIER}" "$TOTAL_SUBTESTS" "$PASS_COUNT" "$TEST_NAME" > /dev/null

# Print completion table
print_test_completion "$TEST_NAME"

end_test $EXIT_CODE $TOTAL_SUBTESTS $PASS_COUNT > /dev/null

# Return status code if sourced, exit if run standalone
if [[ "$RUNNING_IN_TEST_SUITE" == "true" ]]; then
    return $EXIT_CODE
else
    exit $EXIT_CODE
fi
