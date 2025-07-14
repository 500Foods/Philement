#!/bin/bash

# Test: Unity
# Runs unit tests using the Unity framework, treating each test file as a subtest

# CHANGELOG
# 2.0.3 - 2025-07-14 - Enhanced individual test reporting with INFO lines showing test descriptions and purposes
# 2.0.2 - 2025-07-06 - Added missing shellcheck justifications
# 2.0.1 - 2025-07-06 - Removed hardcoded absolute path; now handled by log_output.sh
# 2.0.0 - 2025-07-02 - Complete rewrite to use new modular test libraries
# 1.0.0 - 2025-06-25 - Initial version for running Unity tests

# Test configuration
TEST_NAME="Unity Unit Tests"
SCRIPT_VERSION="2.0.3"

# Get the directory where this script is located
TEST_SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

if [[ -z "$LOG_OUTPUT_SH_GUARD" ]]; then
    # shellcheck source=tests/lib/log_output.sh  # Resolve path statically
    source "$TEST_SCRIPT_DIR/lib/log_output.sh"
fi

# shellcheck source=tests/lib/file_utils.sh # Resolve path statically
source "$TEST_SCRIPT_DIR/lib/file_utils.sh"
# shellcheck source=tests/lib/framework.sh # Resolve path statically
source "$TEST_SCRIPT_DIR/lib/framework.sh"
# shellcheck source=tests/lib/lifecycle.sh # Resolve path statically
source "$TEST_SCRIPT_DIR/lib/lifecycle.sh"
# shellcheck source=tests/lib/coverage.sh # Resolve path statically
source "$TEST_SCRIPT_DIR/lib/coverage.sh"

# Restore SCRIPT_DIR after sourcing libraries (they may override it)
SCRIPT_DIR="$TEST_SCRIPT_DIR"

# Test configuration
EXIT_CODE=0
TOTAL_SUBTESTS=0
PASS_COUNT=0

# Auto-extract test number and set up environment
TEST_NUMBER=$(extract_test_number "${BASH_SOURCE[0]}")
set_test_number "$TEST_NUMBER"
reset_subtest_counter

# Print beautiful test header
print_test_header "$TEST_NAME" "$SCRIPT_VERSION"

# Set up results directory - use tmpfs build directory if available
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

# Configuration
HYDROGEN_DIR="$( cd "$SCRIPT_DIR/.." && pwd )"
UNITY_DIR="$SCRIPT_DIR/unity"

# Use tmpfs build directory if available for ultra-fast I/O
BUILD_DIR="$SCRIPT_DIR/../build"
if mountpoint -q "$BUILD_DIR" 2>/dev/null; then
    # tmpfs is mounted, use build/tests/ for ultra-fast I/O
    DIAG_DIR="$BUILD_DIR/tests/diagnostics"
    LOG_FILE="$BUILD_DIR/tests/logs/unity_tests.log"
else
    # Fallback to regular filesystem
    DIAG_DIR="$SCRIPT_DIR/diagnostics"
    LOG_FILE="$SCRIPT_DIR/logs/unity_tests.log"
fi
DIAG_TEST_DIR="$DIAG_DIR/unity_tests_${TIMESTAMP}"


# Create output directories
next_subtest
print_subtest "Create Output Directories"
if setup_output_directories "$RESULTS_DIR" "$DIAG_DIR" "$LOG_FILE" "$DIAG_TEST_DIR"; then
    ((PASS_COUNT++))
else
    EXIT_CODE=1
fi

# Function to download Unity framework if missing
download_unity_framework() {
    local framework_dir="$UNITY_DIR/framework"
    local unity_dir="$framework_dir/Unity"
    if [ ! -d "$unity_dir" ]; then
        print_message "Unity framework not found in $unity_dir. Downloading now..."
        mkdir -p "$framework_dir"
        if command -v curl >/dev/null 2>&1; then
            if curl -L https://github.com/ThrowTheSwitch/Unity/archive/refs/heads/master.zip -o "$framework_dir/unity.zip"; then
                unzip "$framework_dir/unity.zip" -d "$framework_dir/"
                mv "$framework_dir/Unity-master" "$unity_dir"
                rm "$framework_dir/unity.zip"
                print_result 0 "Unity framework downloaded and extracted successfully to $unity_dir."
                return 0
            else
                print_result 1 "Failed to download Unity framework with curl."
                return 1
            fi
        elif command -v git >/dev/null 2>&1; then
            if git clone https://github.com/ThrowTheSwitch/Unity.git "$unity_dir"; then
                print_result 0 "Unity framework cloned successfully to $unity_dir."
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
        print_message "Unity framework already exists in $unity_dir."
        return 0
    fi
}

# Function to check Unity tests are available via CTest (assumes they're already built by main build system)
check_unity_tests_available() {
    next_subtest
    print_subtest "Check Unity Tests Available"
    print_message "Checking for Unity tests via CTest (should be built by main build system)..."
    
    # Check if CMake build directory exists and has CTest configuration
    local cmake_build_dir="$HYDROGEN_DIR/cmake"
    
    if [ ! -d "$cmake_build_dir" ]; then
        print_result 1 "CMake build directory not found: ${cmake_build_dir#"$SCRIPT_DIR"/..}"
        return 1
    fi
    
    # Change to CMake directory to check for Unity tests
    cd "$cmake_build_dir" || { print_result 1 "Failed to change to CMake build directory"; return 1; }
    
    # Check if Unity tests are registered with CTest
    if ctest -N | grep -q "test_hydrogen"; then
        # Also verify the executable exists in the correct location (build/unity/src/)
        local unity_test_exe="$HYDROGEN_DIR/build/unity/src/test_hydrogen"
        if [ -f "$unity_test_exe" ]; then
            exe_size=$(get_file_size "$unity_test_exe")
            formatted_size=$(format_file_size "$exe_size")
            print_result 0 "Unity tests available via CTest: ${unity_test_exe#"$SCRIPT_DIR"/..} (${formatted_size} bytes)"
            cd "$SCRIPT_DIR" || { print_result 1 "Failed to return to script directory"; return 1; }
            return 0
        else
            print_result 1 "Unity test registered with CTest but executable not found at ${unity_test_exe#"$SCRIPT_DIR"/..}"
            cd "$SCRIPT_DIR" || { print_result 1 "Failed to return to script directory"; return 1; }
            return 1
        fi
    else
        print_result 1 "Unity tests not found in CTest. Run 'cmake --build . --target unity_tests' from cmake directory first."
        cd "$SCRIPT_DIR" || { print_result 1 "Failed to return to script directory"; return 1; }
        return 1
    fi
}

# Function to run a single Unity test executable and report results
run_single_unity_test() {
    local test_name="$1"
    local test_exe="$HYDROGEN_DIR/build/unity/src/$test_name"
    
    next_subtest
    print_subtest "Run Unity Test: $test_name"
    
    if [ ! -f "$test_exe" ]; then
        print_result 1 "Unity test executable not found: ${test_exe#"$SCRIPT_DIR"/..}"
        return 1
    fi
    
    # Run the Unity test directly and capture output
    local temp_test_log="$DIAG_TEST_DIR/${test_name}_output.txt"
    mkdir -p "$(dirname "$temp_test_log")"
    true > "$temp_test_log"
    
    if "$test_exe" > "$temp_test_log" 2>&1; then
        # Parse the Unity test output to get test counts
        local test_count
        local failure_count
        local ignored_count
        test_count=$(grep -E "[0-9]+ Tests [0-9]+ Failures [0-9]+ Ignored" "$temp_test_log" | awk '{print $1}')
        failure_count=$(grep -E "[0-9]+ Tests [0-9]+ Failures [0-9]+ Ignored" "$temp_test_log" | awk '{print $3}')
        ignored_count=$(grep -E "[0-9]+ Tests [0-9]+ Failures [0-9]+ Ignored" "$temp_test_log" | awk '{print $5}')
        
        # Extract and display individual test names with INFO lines
        while IFS= read -r line; do
            # Look for lines that match Unity output format: filepath:line_number:test_name:result
            if [[ "$line" =~ ^.+:[0-9]+:([a-zA-Z_][a-zA-Z0-9_]*):([A-Z]+)$ ]]; then
                local individual_test_name="${BASH_REMATCH[1]}"
                local test_result="${BASH_REMATCH[2]}"
                
                print_message "$individual_test_name:$test_result"
            fi
        done < "$temp_test_log"
        
        if [ -n "$test_count" ] && [ -n "$failure_count" ] && [ -n "$ignored_count" ]; then
            local passed_count=$((test_count - failure_count - ignored_count))
            print_result 0 "Unity test $test_name passed: $test_count tests ($passed_count/$test_count passed)"
        else
            print_result 0 "Unity test $test_name passed"
        fi
        ((PASS_COUNT++))
        return 0
    else
        # Show failed test output
        while IFS= read -r line; do
            print_output "$line"
        done < "$temp_test_log"
        print_result 1 "Unity test $test_name failed"
        return 1
    fi
    
    cat "$temp_test_log" >> "$LOG_FILE"
}

# Function to run Unity tests via direct execution
run_unity_tests() {
    local overall_result=0
    
    # Find all Unity test executables
    local unity_build_dir="$HYDROGEN_DIR/build/unity/src"
    local unity_tests=()
    
    if [ -d "$unity_build_dir" ]; then
        # Look for executable files that match test_* pattern (recursively in subdirectories)
        while IFS= read -r -d '' test_exe; do
            local test_name
            test_name=$(basename "$test_exe")
            if [ -x "$test_exe" ] && [[ "$test_name" =~ ^test_[a-zA-Z_]+$ ]]; then
                # Get relative path from unity_build_dir to show directory structure
                local relative_path
                relative_path=$(realpath --relative-to="$unity_build_dir" "$test_exe")
                unity_tests+=("$relative_path")
            fi
        done < <(find "$unity_build_dir" -name "test_*" -type f -print0)
    fi
    
    if [ ${#unity_tests[@]} -eq 0 ]; then
        print_error "No Unity test executables found in $unity_build_dir"
        return 1
    fi
    
    # Run each Unity test
    for test_path in "${unity_tests[@]}"; do
        if ! run_single_unity_test "$test_path"; then
            overall_result=1
        fi
    done
    
    return $overall_result
}

# Check and download Unity framework
next_subtest
print_subtest "Check Unity Framework"
if download_unity_framework; then
    print_result 0 "Unity framework check passed."
    ((PASS_COUNT++))
else
    print_result 1 "Unity framework check failed."
    EXIT_CODE=1
fi

# Check and run tests
if check_unity_tests_available; then
    ((PASS_COUNT++))
    if ! run_unity_tests; then
        EXIT_CODE=1
    fi
    
    # Calculate Unity test coverage using optimized library function
    next_subtest
    print_subtest "Calculate Unity Test Coverage"
    print_message "Calculating Unity test coverage..."
    
    # Use the main build directory structure for Unity coverage
    build_dir="$HYDROGEN_DIR/build/unity/src"
    unity_coverage=$(calculate_unity_coverage "$build_dir" "$TIMESTAMP")
    result=$?
    
    if [ $result -eq 0 ]; then
        # Read detailed coverage information if available
        detailed_file="$RESULTS_DIR/unity_coverage.txt.detailed"
        if [ -f "$detailed_file" ]; then
            # Parse detailed coverage: timestamp,coverage_percentage,covered_lines,total_lines,instrumented_files,covered_files
            IFS=',' read -r timestamp_detail coverage_detail covered_lines total_lines instrumented_files covered_files < "$detailed_file"
            print_result 0 "Unity test coverage calculated: $unity_coverage% ($covered_lines/$total_lines lines, $covered_files/$instrumented_files files)"
        else
            print_result 0 "Unity test coverage calculated: $unity_coverage%"
        fi
        ((PASS_COUNT++))
    else
        print_result 1 "Failed to calculate Unity test coverage"
        EXIT_CODE=1
    fi
    TOTAL_SUBTESTS=$((TOTAL_SUBTESTS + 1))
else
    print_error "Unity tests not available, skipping test execution"
    EXIT_CODE=1
fi

# Export results for test_all.sh integration
# Derive test name from script filename for consistency with test_00_all.sh
TEST_IDENTIFIER=$(basename "${BASH_SOURCE[0]}" .sh | sed 's/test_[0-9]*_//')
export_subtest_results "${TEST_NUMBER}_${TEST_IDENTIFIER}" "$TOTAL_SUBTESTS" "$PASS_COUNT" "$TEST_NAME" > /dev/null

# Print completion table
print_test_completion "$TEST_NAME"

end_test "$EXIT_CODE" "$TOTAL_SUBTESTS" "$PASS_COUNT" > /dev/null

# Return status code if sourced, exit if run standalone
if [[ "$RUNNING_IN_TEST_SUITE" == "true" ]]; then
    return $EXIT_CODE
else
    exit $EXIT_CODE
fi
