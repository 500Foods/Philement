#!/bin/bash

# Test: Compilation
# Tests compilation of the project using CMake

# This test verifies that the project can be compiled successfully
# using the CMake build system with various configurations.

# CHANGELOG
# 2.2.0 - 2025-07-14 - Added Unity framework check (moved from test 11), fixed tmpfs mount failure handling
# 2.1.0 - 2025-07-09 - Added payload generation subtest
# 2.0.1 - 2025-07-06 - Added missing shellcheck justifications
# 2.0.0 - 2025-07-02 - Complete rewrite to use new modular test libraries
# 1.0.0 - Initial version

# Test configuration
TEST_NAME="Compilation"
SCRIPT_VERSION="2.2.0"

# Get the directory where this script is located
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

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
# shellcheck source=tests/lib/coverage.sh # Resolve path statically
source "$SCRIPT_DIR/lib/coverage.sh"

# Test configuration
EXIT_CODE=0
TOTAL_SUBTESTS=18
PASS_COUNT=0

# Auto-extract test number and set up environment
TEST_NUMBER=$(extract_test_number "${BASH_SOURCE[0]}")
set_test_number "$TEST_NUMBER"
reset_subtest_counter

# Print beautiful test header
print_test_header "$TEST_NAME" "$SCRIPT_VERSION"

# Set up results directory
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

# Use tmpfs build directory if available for ultra-fast I/O
BUILD_DIR="$SCRIPT_DIR/../build"
if mountpoint -q "$BUILD_DIR" 2>/dev/null; then
    # tmpfs is mounted, use build/tests/results for ultra-fast I/O
    RESULTS_DIR="$BUILD_DIR/tests/results"
else
    # Fallback to regular filesystem
    RESULTS_DIR="$SCRIPT_DIR/results"
fi
mkdir -p "$RESULTS_DIR"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
RESULT_LOG="$RESULTS_DIR/test_${TEST_NUMBER}_${TIMESTAMP}.log"

# Navigate to the project root (one level up from tests directory)
if ! navigate_to_project_root "$SCRIPT_DIR"; then
    print_error "Failed to navigate to project root directory"
    exit 1
fi

# Clean previous coverage data at the start of compilation
cleanup_coverage_data 2>/dev/null || true

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

# Subtest: Setup tmpfs build directory
next_subtest
print_subtest "Setup tmpfs Build Directory"

# Check if build directory exists
if [ -d "build" ]; then
    print_message "Build directory exists, checking mount status..."
    
    # Check if build is already a tmpfs mount
    if mountpoint -q build 2>/dev/null; then
        print_message "Build directory already mounted as tmpfs, emptying contents..."
        print_command "rm -rf build/*"
        if rm -rf build/* 2>/dev/null; then
            print_result 0 "Build directory (tmpfs) emptied and ready for use"
        else
            print_result 1 "Failed to empty tmpfs build directory"
            EXIT_CODE=1
        fi
    else
        # Empty the regular directory and mount as tmpfs
        print_message "Emptying regular build directory..."
        print_command "rm -rf build/*"
        if rm -rf build/* 2>/dev/null; then
            print_message "Successfully emptied build directory"
            
            # Mount as tmpfs
            print_message "Mounting 'build' as tmpfs with 1GB size..."
            print_command "sudo mount -t tmpfs -o size=1G tmpfs build"
            if sudo mount -t tmpfs -o size=1G tmpfs build 2>/dev/null; then
                print_result 0 "Build directory mounted as tmpfs (1GB) for faster I/O"
                print_message "Warning: tmpfs is volatile; artifacts will be lost on unmount/reboot"
            else
                print_result 0 "Build directory ready (tmpfs mount failed, using regular filesystem)"
                print_message "Continuing with regular filesystem build directory"
            fi
        else
            print_result 1 "Failed to empty 'build' directory"
            EXIT_CODE=1
        fi
    fi
else
    # Create the build directory and mount as tmpfs
    print_message "Creating 'build' directory..."
    print_command "mkdir build"
    if mkdir build 2>/dev/null; then
        print_message "Successfully created build directory"
        
        # Mount as tmpfs
        print_message "Mounting 'build' as tmpfs with 1GB size..."
        print_command "sudo mount -t tmpfs -o size=1G tmpfs build"
        if sudo mount -t tmpfs -o size=1G tmpfs build 2>/dev/null; then
            print_result 0 "Build directory created and mounted as tmpfs (1GB) for faster I/O"
            print_message "Warning: tmpfs is volatile; artifacts will be lost on unmount/reboot"
        else
            print_result 0 "Build directory created (tmpfs mount failed, using regular filesystem)"
            print_message "Continuing with regular filesystem build directory"
        fi
    else
        print_result 1 "Failed to create 'build' directory"
        EXIT_CODE=1
    fi
fi

evaluate_test_result_silent "Setup tmpfs build directory" "$EXIT_CODE" "PASS_COUNT" "EXIT_CODE"

# Function to download Unity framework if missing
download_unity_framework() {
    local unity_dir="$SCRIPT_DIR/unity"
    local framework_dir="$unity_dir/framework"
    local unity_framework_dir="$framework_dir/Unity"
    
    if [ ! -d "$unity_framework_dir" ]; then
        print_message "Unity framework not found in $unity_framework_dir. Downloading now..."
        mkdir -p "$framework_dir"
        if command -v curl >/dev/null 2>&1; then
            if curl -L https://github.com/ThrowTheSwitch/Unity/archive/refs/heads/master.zip -o "$framework_dir/unity.zip"; then
                unzip "$framework_dir/unity.zip" -d "$framework_dir/"
                mv "$framework_dir/Unity-master" "$unity_framework_dir"
                rm "$framework_dir/unity.zip"
                print_result 0 "Unity framework downloaded and extracted successfully to $unity_framework_dir."
                return 0
            else
                print_result 1 "Failed to download Unity framework with curl."
                return 1
            fi
        elif command -v git >/dev/null 2>&1; then
            if git clone https://github.com/ThrowTheSwitch/Unity.git "$unity_framework_dir"; then
                print_result 0 "Unity framework cloned successfully to $unity_framework_dir."
                return 0
            else
                print_result 1 "Failed to clone Unity framework with git."
                return 1
            fi
        else
            print_result 1 "Neither curl nor git is available to download the Unity framework."
            return 1
        fi
    else
        print_message "Unity framework already exists in $unity_framework_dir."
        return 0
    fi
}

# Subtest: Check Unity Framework
next_subtest
print_subtest "Check Unity Framework"
if download_unity_framework; then
    print_result 0 "Unity framework check passed."
else
    print_result 1 "Unity framework check failed."
    EXIT_CODE=1
fi
evaluate_test_result_silent "Unity framework check" "$EXIT_CODE" "PASS_COUNT" "EXIT_CODE"

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

# Subtest: Check and Generate Payload
next_subtest
print_subtest "Check and Generate Payload"
print_command "test -f payloads/payload.tar.br.enc"
if [ -f "payloads/payload.tar.br.enc" ]; then
    payload_size=$(get_file_size "payloads/payload.tar.br.enc")
    formatted_size=$(format_file_size "$payload_size")
    print_result 0 "Payload file exists: payloads/payload.tar.br.enc (${formatted_size} bytes)"
    ((PASS_COUNT++))
else
    print_message "Payload file not found, generating..."
    print_command "cd payloads"
    if safe_cd payloads; then
        print_command "./swagger-generate.sh"
        if ./swagger-generate.sh >/dev/null 2>&1; then
            print_message "Swagger generation completed"
            print_command "./payload-generate.sh"
            if ./payload-generate.sh >/dev/null 2>&1; then
                print_message "Payload generation completed"
                print_command "cd .."
                safe_cd ..
                if [ -f "payloads/payload.tar.br.enc" ]; then
                    payload_size=$(get_file_size "payloads/payload.tar.br.enc")
                    formatted_size=$(format_file_size "$payload_size")
                    print_result 0 "Payload file generated successfully: payloads/payload.tar.br.enc (${formatted_size} bytes)"
                    ((PASS_COUNT++))
                else
                    print_result 1 "Payload file not found after generation"
                    EXIT_CODE=1
                fi
            else
                print_result 1 "Payload generation failed"
                safe_cd ..
                EXIT_CODE=1
            fi
        else
            print_result 1 "Swagger generation failed"
            safe_cd ..
            EXIT_CODE=1
        fi
    else
        print_result 1 "Failed to enter payloads directory"
        EXIT_CODE=1
    fi
fi

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

# Subtest: Verify coverage executable
next_subtest
print_subtest "Verify Coverage Executable"
print_command "test -f hydrogen_coverage"
if [ -f "hydrogen_coverage" ]; then
    exe_size=$(get_file_size "hydrogen_coverage")
    formatted_size=$(format_file_size "$exe_size")
    print_result 0 "Executable created: hydrogen_coverage (${formatted_size} bytes)"
else
    print_result 1 "Coverage executable not found after build"
    EXIT_CODE=1
fi
evaluate_test_result_silent "Verify coverage executable" "$EXIT_CODE" "PASS_COUNT" "EXIT_CODE"

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

# Subtest: Verify coverage executable has payload
next_subtest
print_subtest "Verify Coverage Executable Payload"
print_command "test -f hydrogen_coverage && grep -q '<<< HERE BE ME TREASURE >>>' hydrogen_coverage"
if [ -f "hydrogen_coverage" ]; then
    # Check if the coverage binary has payload embedded using the correct marker
    if grep -q "<<< HERE BE ME TREASURE >>>" "hydrogen_coverage" 2>/dev/null; then
        coverage_size=$(get_file_size "hydrogen_coverage")
        formatted_size=$(format_file_size "$coverage_size")
        print_result 0 "Coverage executable has embedded payload (${formatted_size} bytes total)"
    else
        print_result 1 "Coverage executable appears to be missing embedded payload marker"
        EXIT_CODE=1
    fi
else
    print_result 1 "Coverage executable not found for payload verification"
    EXIT_CODE=1
fi
evaluate_test_result_silent "Verify coverage executable payload" "$EXIT_CODE" "PASS_COUNT" "EXIT_CODE"

# Subtest: Verify release executable has payload
next_subtest
print_subtest "Verify Release Executable Payload"
print_command "test -f hydrogen_release && grep -q '<<< HERE BE ME TREASURE >>>' hydrogen_release"
if [ -f "hydrogen_release" ]; then
    # Check if the release binary has payload embedded using the correct marker
    if grep -q "<<< HERE BE ME TREASURE >>>" "hydrogen_release" 2>/dev/null; then
        release_size=$(get_file_size "hydrogen_release")
        formatted_size=$(format_file_size "$release_size")
        print_result 0 "Release executable has embedded payload (${formatted_size} bytes total)"
    else
        print_result 1 "Release executable appears to be missing embedded payload marker"
        EXIT_CODE=1
    fi
else
    print_result 1 "Release executable not found for payload verification"
    EXIT_CODE=1
fi
evaluate_test_result_silent "Verify release executable payload" "$EXIT_CODE" "PASS_COUNT" "EXIT_CODE"

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
