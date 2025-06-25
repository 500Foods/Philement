#!/bin/bash
#
# Hydrogen Unity Tests Script
# Runs unit tests using the Unity framework, treating each test file as a subtest
#
NAME_SCRIPT="Hydrogen Unity Tests"
VERS_SCRIPT="1.0.0"

# VERSION HISTORY
# 1.0.0 - 2025-06-25 - Initial version for running Unity tests

# Display script name and version
echo "=== $NAME_SCRIPT v$VERS_SCRIPT ==="

# Get the directory where this script is located
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
HYDROGEN_DIR="$( cd "$SCRIPT_DIR/.." && pwd )"  # Used for potential future path references

# Include the common test utilities
source "$SCRIPT_DIR/support_utils.sh"

# Configuration
UNITY_DIR="$SCRIPT_DIR/unity"
RESULTS_DIR="$SCRIPT_DIR/results"
DIAG_DIR="$SCRIPT_DIR/diagnostics"
LOG_FILE="$SCRIPT_DIR/logs/unity_tests.log"

# Create output directories
create_output_directories() {
    local dirs=("$RESULTS_DIR" "$DIAG_DIR" "$(dirname "$LOG_FILE")")
    local dir
    
    for dir in "${dirs[@]}"; do
        if ! mkdir -p "$dir"; then
            print_error "Failed to create directory: $dir"
            return 1
        fi
    done
    return 0
}

if ! create_output_directories; then
    exit 1
fi

# Generate timestamp and result paths
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
RESULT_LOG="${RESULTS_DIR}/unity_tests_${TIMESTAMP}.log"
DIAG_TEST_DIR="${DIAG_DIR}/unity_tests_${TIMESTAMP}"

if ! mkdir -p "$DIAG_TEST_DIR"; then
    print_error "Failed to create diagnostics directory: $DIAG_TEST_DIR"
    exit 1
fi

# Initialize test counters
TOTAL_SUBTESTS=0
PASSED_SUBTESTS=0

# Start the test
start_test "Hydrogen Unity Tests" | tee -a "$RESULT_LOG"

# Function to compile Unity tests
compile_unity_tests() {
    print_info "Compiling Unity tests..." | tee -a "$RESULT_LOG"
    local build_dir="$HYDROGEN_DIR/build_unity_tests"
    mkdir -p "$build_dir"
    cd "$build_dir" || { print_error "Failed to change to build directory: $build_dir" | tee -a "$RESULT_LOG"; return 1; }
    cmake "$UNITY_DIR" || { print_error "CMake configuration failed for Unity tests" | tee -a "$RESULT_LOG"; return 1; }
    cmake --build . || { print_error "Build failed for Unity tests" | tee -a "$RESULT_LOG"; return 1; }
    cd "$SCRIPT_DIR" || { print_error "Failed to return to script directory: $SCRIPT_DIR" | tee -a "$RESULT_LOG"; return 1; }
    print_info "Unity tests compiled successfully." | tee -a "$RESULT_LOG"
    return 0
}

# Function to run Unity tests
run_unity_tests() {
    print_header "Running Unity Tests" | tee -a "$RESULT_LOG"
    
    if [ ! -d "$UNITY_DIR" ]; then
        print_error "Unity test directory not found: $UNITY_DIR" | tee -a "$RESULT_LOG"
        return 1
    fi
    
    local build_dir="$HYDROGEN_DIR/build_unity_tests"
    if [ ! -d "$build_dir" ]; then
        print_error "Build directory not found: $build_dir" | tee -a "$RESULT_LOG"
        return 1
    fi
    
    # Initially set TOTAL_SUBTESTS based on test files for informational purposes, searching only in the top-level unity directory
    local test_files=()
    mapfile -t test_files < <(find "$UNITY_DIR" -maxdepth 1 -type f -name "test_*.c")
    print_info "Found ${#test_files[@]} Unity test files in top-level unity directory." | tee -a "$RESULT_LOG"
    
    # TOTAL_SUBTESTS will be updated based on actual tests run by ctest
    TOTAL_SUBTESTS=0
    print_info "Running Unity tests to determine total subtests..." | tee -a "$RESULT_LOG"
    
    cd "$build_dir" || { print_error "Failed to change to build directory: $build_dir" | tee -a "$RESULT_LOG"; return 1; }
    ctest --output-on-failure | tee "$LOG_FILE"
    local ctest_result=$?
    cd "$SCRIPT_DIR" || { print_error "Failed to return to script directory: $SCRIPT_DIR" | tee -a "$RESULT_LOG"; return 1; }
    
    # Parse test results from ctest output to count total and passed tests
    PASSED_SUBTESTS=0
    TOTAL_SUBTESTS=0
    while IFS= read -r line; do
        if [[ $line =~ "Passed" ]]; then
            ((PASSED_SUBTESTS++))
            ((TOTAL_SUBTESTS++))
        elif [[ $line =~ "Failed" ]]; then
            ((TOTAL_SUBTESTS++))
        elif [[ $line =~ "Test #" ]]; then
            # If the line contains "Test #", it indicates a test execution line
            # This is a fallback to ensure we count all tests if "Failed" isn't explicitly mentioned
            if [[ ! $line =~ "Passed" ]]; then
                ((TOTAL_SUBTESTS++))
            fi
        fi
    done < "$LOG_FILE"
    print_info "Found $TOTAL_SUBTESTS Unity tests run as subtests." | tee -a "$RESULT_LOG"
    
    if [ $ctest_result -eq 0 ]; then
        print_info "All Unity tests passed." | tee -a "$RESULT_LOG"
    else
        print_warning "Some Unity tests failed. Check $LOG_FILE for details." | tee -a "$RESULT_LOG"
    fi
    
    return $ctest_result
}

# Compile and run tests
COMPILE_RESULT=0
TEST_RESULT=0

print_header "Compiling Unity Tests" | tee -a "$RESULT_LOG"
compile_unity_tests
COMPILE_RESULT=$?

if [ $COMPILE_RESULT -eq 0 ]; then
    run_unity_tests
    TEST_RESULT=$?
else
    print_error "Compilation failed, skipping test execution" | tee -a "$RESULT_LOG"
    TEST_RESULT=1
fi

# Get test name from script name
TEST_NAME=$(basename "$0" .sh | sed 's/^test_//')

# Export subtest results for test_all.sh
export_subtest_results "$TEST_NAME" "$TOTAL_SUBTESTS" "$PASSED_SUBTESTS"

# Print final summary
print_header "Test Summary" | tee -a "$RESULT_LOG"
print_info "Total subtests: $TOTAL_SUBTESTS" | tee -a "$RESULT_LOG"
print_info "Passed subtests: $PASSED_SUBTESTS" | tee -a "$RESULT_LOG"

# Determine final exit status
if [ $COMPILE_RESULT -eq 0 ] && [ $TEST_RESULT -eq 0 ]; then
    print_result 0 "All Unity tests passed (placeholder)" | tee -a "$RESULT_LOG"
    end_test 0 "Unity Tests" | tee -a "$RESULT_LOG"
    exit 0
else
    print_result 1 "One or more Unity tests failed or compilation issues occurred" | tee -a "$RESULT_LOG"
    end_test 1 "Unity Tests" | tee -a "$RESULT_LOG"
    exit 1
fi
