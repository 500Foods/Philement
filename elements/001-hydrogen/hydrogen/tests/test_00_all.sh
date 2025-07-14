#!/bin/bash

# Test: Test Suite Runner
# Executes all tests in parallel batches or sequentially and generates a summary report

# Usage: ./test_00_all.sh [test_name1 test_name2 ...] [--skip-tests] [--sequential] [--help]

# CHANGELOG
# 4.0.1 - 2025-07-07 - Fixed how individual test elapsed times are stored and accessed
# 4.0.1 - 2025-07-07 - Added missing shellcheck justifications
# 4.0.0 - 2025-07-04 - Added dynamic parallel execution with group-based batching for significant performance improvement
# 3.0.1 - 2025-07-03 - Enhanced --help to list test names, suppressed non-table output, updated history
# 3.0.0 - 2025-07-02 - Complete rewrite to use lib/ scripts, simplified orchestration approach

# Test configuration
TEST_NAME="Test Suite Runner"
SCRIPT_VERSION="4.0.1"

# Get the directory where this script is located
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

# Source the table rendering libraries for the summary
# shellcheck source=tests/lib/log_output.sh # Resolve path statically
source "$SCRIPT_DIR/lib/log_output.sh"
# shellcheck source=tests/lib/framework.sh # Resolve path statically
source "$SCRIPT_DIR/lib/framework.sh"
# shellcheck source=tests/lib/cloc.sh # Resolve path statically
source "$SCRIPT_DIR/lib/cloc.sh"
# shellcheck source=tests/lib/coverage.sh # Resolve path statically
source "$SCRIPT_DIR/lib/coverage.sh"

# Auto-extract test number and set up environment
TEST_NUMBER=$(extract_test_number "${BASH_SOURCE[0]}")
set_test_number "$TEST_NUMBER"
reset_subtest_counter

# Arrays to track test results
declare -a TEST_NUMBERS
declare -a TEST_NAMES
declare -a TEST_SUBTESTS
declare -a TEST_PASSED
declare -a TEST_FAILED
declare -a TEST_ELAPSED

# Results directory - use tmpfs build directory if available
BUILD_DIR="$SCRIPT_DIR/../build"
if mountpoint -q "$BUILD_DIR" 2>/dev/null; then
    # tmpfs is mounted, use build/tests/results for ultra-fast I/O
    RESULTS_DIR="$BUILD_DIR/tests/results"
else
    # Fallback to regular filesystem
    RESULTS_DIR="$SCRIPT_DIR/results"
fi
mkdir -p "$RESULTS_DIR"

# Command line argument parsing
SKIP_TESTS=false
SEQUENTIAL_MODE=false
TEST_ARGS=()

# Parse all arguments
for arg in "$@"; do
    case "$arg" in
        --skip-tests)
            SKIP_TESTS=true
            ;;
        --sequential)
            SEQUENTIAL_MODE=true
            ;;
        --help|-h)
            # Help will be handled later
            ;;
        *)
            TEST_ARGS+=("$arg")
            ;;
    esac
done

# Function to get coverage data by type
get_coverage() {
    local coverage_type="$1"
    local coverage_file="$RESULTS_DIR/${coverage_type}_coverage.txt"
    if [ -f "$coverage_file" ]; then
        cat "$coverage_file" 2>/dev/null || echo "0.000"
    else
        echo "0.000"
    fi
}

# Convenience functions for coverage types
get_latest_coverage() {
    local latest_file
    latest_file=$(find "$RESULTS_DIR" -name "coverage_*.txt" -type f 2>/dev/null | sort -r | head -1)
    [ -n "$latest_file" ] && [ -f "$latest_file" ] && cat "$latest_file" 2>/dev/null || echo "0.000"
}
get_unity_coverage() { get_coverage "unity"; }
get_blackbox_coverage() { get_coverage "blackbox"; }
get_combined_coverage() { get_coverage "combined"; }

# Function to update README.md with test results
update_readme_with_results() {
    local readme_file="$SCRIPT_DIR/../README.md"
    
    if [ ! -f "$readme_file" ]; then
        echo "Warning: README.md not found at $readme_file"
        return 1
    fi
    
    # Calculate summary statistics
    local total_tests=${#TEST_NUMBERS[@]}
    local total_subtests=0
    local total_passed=0
    local total_failed=0
    local coverage_percentage
    coverage_percentage=$(get_latest_coverage)
    
    for i in "${!TEST_SUBTESTS[@]}"; do
        total_subtests=$((total_subtests + TEST_SUBTESTS[i]))
        total_passed=$((total_passed + TEST_PASSED[i]))
        total_failed=$((total_failed + TEST_FAILED[i]))
    done
    
    # Create temporary file for new README content
    local temp_readme
    temp_readme=$(mktemp) || { echo "Error: Failed to create temporary file" >&2; return 1; }
    
    # Generate timestamp
    local timestamp
    timestamp=$(date)
    
    # Process README.md line by line
    local in_test_results=false
    local in_individual_results=false
    local in_repo_info=false
    
    while IFS= read -r line; do
        if [[ "$line" == "## Latest Test Results" ]]; then
            in_test_results=true
            {
                echo "$line"
                echo ""
                echo "Generated on: $timestamp"
                echo ""
                echo "### Summary"
                echo ""
                echo "| Metric | Value |"
                echo "| ------ | ----- |"
                echo "| Total Tests | $total_tests |"
                echo "| Passed | $total_tests |"
                echo "| Failed | 0 |"
                echo "| Skipped | 0 |"
                echo "| Total Subtests | $total_subtests |"
                echo "| Passed Subtests | $total_passed |"
                echo "| Failed Subtests | $total_failed |"
                echo "| Elapsed Time | $TOTAL_ELAPSED_FORMATTED |"
                echo "| Running Time | $TOTAL_RUNNING_TIME_FORMATTED |"
                echo ""
                echo "### Test Coverage"
                echo ""
                
                # Get detailed coverage information with thousands separators
                local unity_coverage_detailed="$RESULTS_DIR/unity_coverage.txt.detailed"
                local blackbox_coverage_detailed="$RESULTS_DIR/blackbox_coverage.txt.detailed"
                local combined_coverage_detailed="$RESULTS_DIR/combined_coverage.txt.detailed"
                
                if [ -f "$unity_coverage_detailed" ] || [ -f "$blackbox_coverage_detailed" ] || [ -f "$combined_coverage_detailed" ]; then
                    echo "| Test Type | Files Cover | Files Instr | Lines Cover | Lines Instr | Coverage |"
                    echo "| --------- | ----------- | ----------- | ----------- | ----------- | -------- |"
                    
                    # Unity coverage
                    if [ -f "$unity_coverage_detailed" ]; then
                        local unity_timestamp unity_coverage_pct unity_covered unity_total unity_instrumented unity_covered_files
                        IFS=',' read -r unity_timestamp unity_coverage_pct unity_covered unity_total unity_instrumented unity_covered_files < "$unity_coverage_detailed" 2>/dev/null
                        if [ -n "$unity_total" ] && [ "$unity_total" -gt 0 ]; then
                            # Add thousands separators
                            unity_covered_formatted=$(printf "%'d" "$unity_covered" 2>/dev/null || echo "$unity_covered")
                            unity_total_formatted=$(printf "%'d" "$unity_total" 2>/dev/null || echo "$unity_total")
                            unity_covered_files=${unity_covered_files:-0}
                            unity_instrumented=${unity_instrumented:-0}
                            echo "| Unity Tests | ${unity_covered_files} | ${unity_instrumented} | ${unity_covered_formatted} | ${unity_total_formatted} | ${unity_coverage_pct}% |"
                        else
                            echo "| Unity Tests | 0 | 0 | 0 | 0 | 0.000% |"
                        fi
                    else
                        echo "| Unity Tests | 0 | 0 | 0 | 0 | 0.000% |"
                    fi
                    
                    # Blackbox coverage
                    if [ -f "$blackbox_coverage_detailed" ]; then
                        local blackbox_timestamp blackbox_coverage_pct blackbox_covered blackbox_total blackbox_instrumented blackbox_covered_files
                        IFS=',' read -r blackbox_timestamp blackbox_coverage_pct blackbox_covered blackbox_total blackbox_instrumented blackbox_covered_files < "$blackbox_coverage_detailed" 2>/dev/null
                        if [ -n "$blackbox_total" ] && [ "$blackbox_total" -gt 0 ]; then
                            # Add thousands separators
                            blackbox_covered_formatted=$(printf "%'d" "$blackbox_covered" 2>/dev/null || echo "$blackbox_covered")
                            blackbox_total_formatted=$(printf "%'d" "$blackbox_total" 2>/dev/null || echo "$blackbox_total")
                            blackbox_covered_files=${blackbox_covered_files:-0}
                            blackbox_instrumented=${blackbox_instrumented:-0}
                            echo "| Blackbox Tests | ${blackbox_covered_files} | ${blackbox_instrumented} | ${blackbox_covered_formatted} | ${blackbox_total_formatted} | ${blackbox_coverage_pct}% |"
                        else
                            echo "| Blackbox Tests | 0 | 0 | 0 | 0 | 0.000% |"
                        fi
                    else
                        echo "| Blackbox Tests | 0 | 0 | 0 | 0 | 0.000% |"
                    fi
                    
                    # Combined coverage
                    if [ -f "$combined_coverage_detailed" ]; then
                        local combined_timestamp combined_coverage_pct combined_covered combined_total combined_instrumented combined_covered_files
                        IFS=',' read -r combined_timestamp combined_coverage_pct combined_covered combined_total combined_instrumented combined_covered_files < "$combined_coverage_detailed" 2>/dev/null
                        if [ -n "$combined_total" ] && [ "$combined_total" -gt 0 ]; then
                            # Add thousands separators
                            combined_covered_formatted=$(printf "%'d" "$combined_covered" 2>/dev/null || echo "$combined_covered")
                            combined_total_formatted=$(printf "%'d" "$combined_total" 2>/dev/null || echo "$combined_total")
                            combined_covered_files=${combined_covered_files:-0}
                            combined_instrumented=${combined_instrumented:-0}
                            echo "| Combined Tests | ${combined_covered_files} | ${combined_instrumented} | ${combined_covered_formatted} | ${combined_total_formatted} | ${combined_coverage_pct}% |"
                        else
                            echo "| Combined Tests | 0 | 0 | 0 | 0 | 0.000% |"
                        fi
                    else
                        echo "| Combined Tests | 0 | 0 | 0 | 0 | 0.000% |"
                    fi
                else
                    echo "| Test Type | Files Cover | Files Instr | Lines Cover | Lines Instr | Coverage |"
                    echo "| --------- | ----------- | ----------- | ----------- | ----------- | -------- |"
                    echo "| Unity Tests | 0 | 0 | 0 | 0 | 0.000% |"
                    echo "| Blackbox Tests | 0 | 0 | 0 | 0 | 0.000% |"
                fi
                echo ""
            } >> "$temp_readme"
            continue
        elif [[ "$line" == "### Individual Test Results" ]]; then
            in_individual_results=true
            {
                echo "$line"
                echo ""
                echo "| Status | Time | Test | Tests | Pass | Fail | Summary |"
                echo "| ------ | ---- | ---- | ----- | ---- | ---- | ------- |"
                
                # Add individual test results
                for i in "${!TEST_NUMBERS[@]}"; do
                    local status="✅"
                    local summary="Test completed without errors"
                    if [ "${TEST_FAILED[$i]}" -gt 0 ]; then
                        status="❌"
                        summary="Test failed with errors"
                    fi
                    local time_formatted
                    time_formatted=$(format_time_duration "${TEST_ELAPSED[$i]}")
                    echo "| $status | $time_formatted | ${TEST_NUMBERS[$i]}_$(echo "${TEST_NAMES[$i]}" | tr ' ' '_' | tr '[:upper:]' '[:lower:]') | ${TEST_SUBTESTS[$i]} | ${TEST_PASSED[$i]} | ${TEST_FAILED[$i]} | $summary |"
                done
                echo ""
            } >> "$temp_readme"
            continue
        elif [[ "$line" == "## Repository Information" ]]; then
            in_repo_info=true
            in_test_results=false
            in_individual_results=false
            {
                echo "$line"
                echo ""
                echo "Generated via cloc: $timestamp"
                echo ""
                
                # Use shared cloc library function, ensuring we're in the project root directory
                pushd "$SCRIPT_DIR/.." > /dev/null || return 1
                generate_cloc_for_readme "." ".lintignore"
                popd > /dev/null || return 1
            } >> "$temp_readme"
            continue
        elif [[ "$in_test_results" == true || "$in_individual_results" == true || "$in_repo_info" == true ]]; then
            # Skip existing content in these sections
            if [[ "$line" == "## "* ]]; then
                # New section started, stop skipping
                in_test_results=false
                in_individual_results=false
                in_repo_info=false
                echo "$line" >> "$temp_readme"
            fi
            continue
        else
            echo "$line" >> "$temp_readme"
        fi
    done < "$readme_file"
    
    # Replace original README with updated version
    if mv "$temp_readme" "$readme_file"; then
        # echo "Updated README.md with test results"
        :
    else
        echo "Error: Failed to update README.md"
        rm -f "$temp_readme"
        return 1
    fi
}

# Function to format seconds as HH:MM:SS.ZZZ
format_time_duration() {
    local seconds="$1"
    local hours minutes secs milliseconds
    
    # Handle seconds that start with a decimal point (e.g., ".492")
    if [[ "$seconds" =~ ^\. ]]; then
        seconds="0$seconds"
    fi
    
    # Handle decimal seconds
    if [[ "$seconds" =~ ^([0-9]+)\.([0-9]+)$ ]]; then
        secs="${BASH_REMATCH[1]}"
        milliseconds="${BASH_REMATCH[2]}"
        # Pad or truncate milliseconds to 3 digits
        milliseconds="${milliseconds}000"
        milliseconds="${milliseconds:0:3}"
    else
        secs="$seconds"
        milliseconds="000"
    fi
    
    hours=$((secs / 3600))
    minutes=$(((secs % 3600) / 60))
    secs=$((secs % 60))
    
    printf "%02d:%02d:%02d.%s" "$hours" "$minutes" "$secs" "$milliseconds"
}

# Function to show help
show_help() {
    echo "Usage: $0 [test_name1 test_name2 ...] [--skip-tests] [--sequential] [--help]"
    echo ""
    echo "Arguments:"
    echo "  test_name    Run specific tests (e.g., 01_compilation, 98_check_links)"
    echo ""
    echo "Options:"
    echo "  --skip-tests Skip actual test execution, just show what tests would run"
    echo "  --sequential Run tests sequentially instead of in parallel batches (default: parallel)"
    echo "  --help       Show this help message"
    echo ""
    echo "Execution Modes:"
    echo "  Default:     Tests run in parallel batches grouped by tens digit (0x, 1x, 2x, etc.)"
    echo "  Sequential:  Tests run one at a time in numerical order (original behavior)"
    echo ""
    echo "Available Tests:"
    for test_script in "${TEST_SCRIPTS[@]}"; do
        local test_name
        test_name=$(basename "$test_script" .sh | sed 's/test_//')
        echo "  $test_name"
    done
    echo ""
    echo "Examples:"
    echo "  $0                     # Run all tests in parallel batches"
    echo "  $0 --sequential        # Run all tests sequentially"
    echo "  $0 01_compilation      # Run specific test"
    echo "  $0 01_compilation 03_code_size  # Run multiple tests"
    echo ""
    exit 0
}

# Get list of all test scripts, excluding test_00_all.sh and test_template.sh
# Run compilation test first, then all others in order, ending with test_99_codebase.sh
TEST_SCRIPTS=()

# Add compilation test first if it exists (check both 01 and 10 for compatibility)
if [ -f "$SCRIPT_DIR/test_01_compilation.sh" ]; then
    TEST_SCRIPTS+=("$SCRIPT_DIR/test_01_compilation.sh")
elif [ -f "$SCRIPT_DIR/test_10_compilation.sh" ]; then
    TEST_SCRIPTS+=("$SCRIPT_DIR/test_10_compilation.sh")
fi

# Add all other tests except 00, 01, 10, and template
while IFS= read -r script; do
    if [[ "$script" != *"test_00_all.sh" ]] && [[ "$script" != *"test_01_compilation.sh" ]] && [[ "$script" != *"test_10_compilation.sh" ]] && [[ "$script" != *"test_template.sh" ]]; then
        TEST_SCRIPTS+=("$script")
    fi
done < <(find "$SCRIPT_DIR" -name "test_*.sh" -type f | sort)

# Ensure test_99_codebase.sh is last if it exists
if [ -f "$SCRIPT_DIR/test_99_codebase.sh" ]; then
    # Remove test_99_codebase.sh from the array if it's there
    for i in "${!TEST_SCRIPTS[@]}"; do
        if [[ "${TEST_SCRIPTS[$i]}" == *"test_99_codebase.sh" ]]; then
            unset 'TEST_SCRIPTS[i]'
        fi
    done
    # Re-index array and add test_99 at the end
    TEST_SCRIPTS=("${TEST_SCRIPTS[@]}")
    TEST_SCRIPTS+=("$SCRIPT_DIR/test_99_codebase.sh")
fi

# Check for help flag and skip-tests in single loop
for arg in "$@"; do
    case "$arg" in
        --help|-h) show_help ;;
        --skip-tests) SKIP_TESTS=true ;;
    esac
done

# Function to perform cleanup before test execution
perform_cleanup() {
    # Silently perform cleanup before test execution
    
    # Define directories to clean
    local dirs_to_clean=(
        "$SCRIPT_DIR/logs"
        "$SCRIPT_DIR/results"
        "$SCRIPT_DIR/diagnostics"
        "$SCRIPT_DIR/../build"
        "$SCRIPT_DIR/../build/unity"
    )
    
    # Also clean tmpfs test directories if build is mounted on tmpfs
    local build_dir="$SCRIPT_DIR/../build"
    if mountpoint -q "$build_dir" 2>/dev/null; then
        dirs_to_clean+=(
            "$build_dir/tests/logs"
            "$build_dir/tests/results"
            "$build_dir/tests/diagnostics"
        )
    fi
    
    # Remove directories and their contents silently
    for dir in "${dirs_to_clean[@]}"; do
        if [ -d "$dir" ]; then
            rm -rf "$dir" > /dev/null 2>&1
        fi
    done
    
    # Remove hydrogen executables silently
    local hydrogen_exe="$SCRIPT_DIR/../hydrogen"
    if [ -f "$hydrogen_exe" ]; then
        rm -f "$hydrogen_exe" > /dev/null 2>&1
    fi
    
    # Run CMake clean if CMakeLists.txt exists, silently
    local cmake_dir="$SCRIPT_DIR/../cmake"
    if [ -f "$cmake_dir/CMakeLists.txt" ]; then
        cd "$cmake_dir" > /dev/null 2>&1 || return 1
        cmake --build . --target clean > /dev/null 2>&1
        cd "$SCRIPT_DIR" > /dev/null 2>&1 || return 1
    fi
}

# Get start time
START_TIME=$(date +%s.%N 2>/dev/null || date +%s)

# Print beautiful test suite header in blue
print_test_suite_header "$TEST_NAME" "$SCRIPT_VERSION"

# Perform cleanup before starting tests (unless skipping tests)
if [ "$SKIP_TESTS" = false ]; then
    perform_cleanup
fi

# Function to run a single test and capture results
run_single_test() {
    local test_script="$1"
    local test_number
    local test_name
    
    # Extract test number from script filename
    test_number=$(basename "$test_script" .sh | sed 's/test_//' | sed 's/_.*//')
    test_name=""
    
    # Handle skip mode
    if [ "$SKIP_TESTS" = true ]; then
        # Extract test name from script filename (same logic as normal mode)
        test_name=$(basename "$test_script" .sh | sed 's/test_[0-9]*_//' | tr '_' ' ' | sed 's/\b\w/\U&/g')
        # Store skipped test results (don't print "Would run" message)
        TEST_NUMBERS+=("$test_number")
        TEST_NAMES+=("$test_name")
        TEST_SUBTESTS+=(1)
        TEST_PASSED+=(0)
        TEST_FAILED+=(0)
        TEST_ELAPSED+=("0")
        return 0
    fi
    
    # Record start time for this test
    local test_start
    test_start=$(date +%s.%N 2>/dev/null || date +%s)
    
    # Set environment variable to indicate running in test suite context
    export RUNNING_IN_TEST_SUITE="true"
    
    # Source the test script instead of running it as a separate process
    # shellcheck disable=SC1090  # Can't follow non-constant source
    source "$test_script"
    local exit_code=$?
    
    # Don't calculate elapsed time here - we'll get it from the subtest result file
    # This eliminates the timing discrepancy by using a single source of truth
    local elapsed_formatted="0.000"
    
    # Extract test results from subtest file or use defaults
    local total_subtests=1 passed_subtests=0 elapsed_formatted="0.000"
    local test_name_for_file
    test_name_for_file=$(basename "$test_script" .sh | sed 's/test_[0-9]*_//' | tr '_' ' ' | sed 's/\b\w/\U&/g')
    local latest_subtest_file
    latest_subtest_file=$(find "$RESULTS_DIR" -name "subtest_${test_number}_*.txt" -type f 2>/dev/null | sort -r | head -1)
    
    if [ -n "$latest_subtest_file" ] && [ -f "$latest_subtest_file" ]; then
        IFS=',' read -r total_subtests passed_subtests test_name file_elapsed_time < "$latest_subtest_file" 2>/dev/null || {
            passed_subtests=$([ $exit_code -eq 0 ] && echo 1 || echo 0); test_name="$test_name_for_file"; file_elapsed_time="0.000"; }
        elapsed_formatted="$file_elapsed_time"
    else
        passed_subtests=$([ $exit_code -eq 0 ] && echo 1 || echo 0); test_name="$test_name_for_file"
        [ -t 1 ] && echo "Warning: No subtest result file found for test ${test_number}"
    fi
    
    # Calculate failed subtests
    local failed_subtests=$((total_subtests - passed_subtests))
    
    # Store results
    TEST_NUMBERS+=("$test_number")
    TEST_NAMES+=("$test_name")
    TEST_SUBTESTS+=("$total_subtests")
    TEST_PASSED+=("$passed_subtests")
    TEST_FAILED+=("$failed_subtests")
    TEST_ELAPSED+=("$elapsed_formatted")
    
    return $exit_code
}

# Function to run a single test in parallel mode with output capture
run_single_test_parallel() {
    local test_script="$1"
    local temp_result_file="$2"
    local temp_output_file="$3"
    local test_number
    local test_name
    
    # Extract test number from script filename
    test_number=$(basename "$test_script" .sh | sed 's/test_//' | sed 's/_.*//')
    test_name=""
    
    # Record start time for this test
    local test_start
    test_start=$(date +%s.%N 2>/dev/null || date +%s)
    
    # Set environment variable to indicate running in test suite context
    export RUNNING_IN_TEST_SUITE="true"
    
    # Capture all output from the test
    {
        # Source the test script instead of running it as a separate process
        # shellcheck disable=SC1090  # Can't follow non-constant source
        source "$test_script"
    } > "$temp_output_file" 2>&1
    local exit_code=$?
    
    # Don't calculate elapsed time here either - use file's elapsed time for consistency
    # This ensures parallel execution also uses single source of truth
    local elapsed_formatted="0.000"
    
    # Look for subtest results file written by the individual test
    local total_subtests=1
    local passed_subtests=0
    local test_name_for_file
    test_name_for_file=$(basename "$test_script" .sh | sed 's/test_[0-9]*_//' | tr '_' ' ' | sed 's/\b\w/\U&/g')
    
    # Find the most recent subtest file for this test
    local latest_subtest_file
    latest_subtest_file=$(find "$RESULTS_DIR" -name "subtest_${test_number}_*.txt" -type f 2>/dev/null | sort -r | head -1)
    
    if [ -n "$latest_subtest_file" ] && [ -f "$latest_subtest_file" ]; then
        # Read subtest results, test name, and elapsed time from the file
        IFS=',' read -r total_subtests passed_subtests test_name file_elapsed_time < "$latest_subtest_file" 2>/dev/null || {
            total_subtests=1
            passed_subtests=$([ $exit_code -eq 0 ] && echo 1 || echo 0)
            test_name="$test_name_for_file"
            file_elapsed_time="0.000"
        }
        
        # Use file elapsed time - SINGLE SOURCE OF TRUTH from log_output.sh
        elapsed_formatted="$file_elapsed_time"
    else
        # Default based on exit code if no file found
        total_subtests=1
        passed_subtests=$([ $exit_code -eq 0 ] && echo 1 || echo 0)
        test_name="$test_name_for_file"
        elapsed_formatted="0.000"
    fi
    
    # Calculate failed subtests
    local failed_subtests=$((total_subtests - passed_subtests))
    
    # Write results to temporary file for collection by main process
    echo "$test_number|$test_name|$total_subtests|$passed_subtests|$failed_subtests|$elapsed_formatted|$exit_code" > "$temp_result_file"
    
    return $exit_code
}

# Function to run a specific test by name
run_specific_test() {
    local test_name="$1"
    local test_script="$SCRIPT_DIR/test_${test_name}.sh"
    
    if [ ! -f "$test_script" ]; then
        echo "Error: Test script not found: $test_script"
        echo "Available tests:"
        find "$SCRIPT_DIR" -name "test_*.sh" -not -name "test_00_all.sh" -not -name "test_template.sh" | sort | while read -r script; do
            local name
            name=$(basename "$script" .sh | sed 's/test_//')
            echo "  $name"
        done
        return 1
    fi
    
    if ! run_single_test "$test_script"; then
        return 1
    fi
    return 0
}

# Function to run all tests in parallel batches
run_all_tests_parallel() {
    local overall_exit_code=0
    
    # Create associative array to group tests by tens digit
    declare -A test_groups
    
    # Group tests dynamically by tens digit
    for test_script in "${TEST_SCRIPTS[@]}"; do
        local test_number
        test_number=$(basename "$test_script" .sh | sed 's/test_//' | sed 's/_.*//')
        local group=$((test_number / 10))
        
        if [ -z "${test_groups[$group]}" ]; then
            test_groups[$group]="$test_script"
        else
            test_groups[$group]="${test_groups[$group]} $test_script"
        fi
    done
    
    # Execute groups in numerical order
    for group in $(printf '%s\n' "${!test_groups[@]}" | sort -n); do
        # shellcheck disable=SC2206  # We want word splitting here for the array
        local group_tests=(${test_groups[$group]})
        
        # Reorder tests to run longest-running ones first in the foreground
        local reordered_tests=()
        # Prioritize tests known to be slow
        local tests_to_move_front=("test_20_shutdown.sh" "test_12_env_payload.sh")
        
        # Find and move prioritized tests to the front
        for test_to_move in "${tests_to_move_front[@]}"; do
            for i in "${!group_tests[@]}"; do
                if [[ "${group_tests[$i]}" == *"$test_to_move" ]]; then
                    reordered_tests+=("${group_tests[$i]}")
                    unset 'group_tests[i]'
                    break
                fi
            done
        done
        
        # Add remaining tests after the prioritized ones
        reordered_tests+=("${group_tests[@]}")
        
        # Use the reordered list for execution
        group_tests=("${reordered_tests[@]}")
        
        # Handle skip mode
        if [ "$SKIP_TESTS" = true ]; then
            for test_script in "${group_tests[@]}"; do
                run_single_test "$test_script"
            done
            continue
        fi
        
        # If only one test in group, run it normally
        if [ ${#group_tests[@]} -eq 1 ]; then
            if ! run_single_test "${group_tests[0]}"; then
                overall_exit_code=1
            fi
            continue
        fi
        
        # Multiple tests in group: run first one live, others in background
        local first_test="${group_tests[0]}"
        local remaining_tests=("${group_tests[@]:1}")
        
        # Create temporary files for background tests
        local temp_result_files=()
        local temp_output_files=()
        local pids=()
        
        # Start background tests (all except first)
        for test_script in "${remaining_tests[@]}"; do
            local temp_result_file
            local temp_output_file
            temp_result_file=$(mktemp)
            temp_output_file=$(mktemp)
            temp_result_files+=("$temp_result_file")
            temp_output_files+=("$temp_output_file")
            
            # Run test in background
            run_single_test_parallel "$test_script" "$temp_result_file" "$temp_output_file" &
            pids+=($!)
        done
        
        # Run first test in foreground (shows output immediately)
        if ! run_single_test "$first_test"; then
            overall_exit_code=1
        fi
        
        # Wait for all background tests to complete
        local group_exit_code=0
        for i in "${!pids[@]}"; do
            if ! wait "${pids[$i]}"; then
                group_exit_code=1
                overall_exit_code=1
            fi
        done
        
        # Display outputs from background tests in order
        for i in "${!remaining_tests[@]}"; do
            local temp_output_file="${temp_output_files[$i]}"
            if [ -f "$temp_output_file" ] && [ -s "$temp_output_file" ]; then
                # Display the captured output
                cat "$temp_output_file"
            fi
            # Clean up output file
            rm -f "$temp_output_file"
        done
        
        # Collect results from background tests
        for i in "${!remaining_tests[@]}"; do
            local temp_result_file="${temp_result_files[$i]}"
            if [ -f "$temp_result_file" ] && [ -s "$temp_result_file" ]; then
                # Read results from temporary file
                IFS='|' read -r test_number test_name total_subtests passed_subtests failed_subtests elapsed_formatted exit_code < "$temp_result_file"
                
                # Store results in global arrays
                TEST_NUMBERS+=("$test_number")
                TEST_NAMES+=("$test_name")
                TEST_SUBTESTS+=("$total_subtests")
                TEST_PASSED+=("$passed_subtests")
                TEST_FAILED+=("$failed_subtests")
                TEST_ELAPSED+=("$elapsed_formatted")
            fi
            
            # Clean up result file
            rm -f "$temp_result_file"
        done
        
        # Clear arrays for next group
        temp_result_files=()
        temp_output_files=()
        pids=()
    done
    
    return $overall_exit_code
}

# Function to run all tests (sequential - original behavior)
run_all_tests() {
    local overall_exit_code=0
    
    for test_script in "${TEST_SCRIPTS[@]}"; do
        if ! run_single_test "$test_script"; then
            overall_exit_code=1
        fi
    done
    
    return $overall_exit_code
}

# Execute tests based on command line arguments
OVERALL_EXIT_CODE=0

# If no specific tests provided, run all tests
if [ ${#TEST_ARGS[@]} -eq 0 ]; then
    # Choose execution mode based on --sequential flag
    if [ "$SEQUENTIAL_MODE" = true ]; then
        if ! run_all_tests; then
            OVERALL_EXIT_CODE=1
        fi
    else
        if ! run_all_tests_parallel; then
            OVERALL_EXIT_CODE=1
        fi
    fi
else
    # Process each test argument (always sequential for specific tests)
    for test_arg in "${TEST_ARGS[@]}"; do
        # Check if it's a specific test name
        if ! run_specific_test "$test_arg"; then
            OVERALL_EXIT_CODE=1
        fi
    done
fi

# Calculate total elapsed time
END_TIME=$(date +%s.%N 2>/dev/null || date +%s)
if [[ "$START_TIME" =~ \. ]] && [[ "$END_TIME" =~ \. ]]; then
    TOTAL_ELAPSED=$(echo "$END_TIME - $START_TIME" | bc 2>/dev/null || echo "0")
    # Ensure TOTAL_ELAPSED is not empty or starts with a dot
    if [[ -z "$TOTAL_ELAPSED" || "$TOTAL_ELAPSED" =~ ^\. ]]; then
        TOTAL_ELAPSED="0"
    fi
else
    TOTAL_ELAPSED=$((END_TIME - START_TIME))
fi

# Calculate total running time (sum of all test execution times)
TOTAL_RUNNING_TIME=0
for i in "${!TEST_ELAPSED[@]}"; do
    elapsed_time="${TEST_ELAPSED[$i]}"
    # Handle elapsed times that might start with a dot (e.g., ".460")
    if [[ "$elapsed_time" =~ ^\. ]]; then
        elapsed_time="0$elapsed_time"
    fi
    if [[ "$elapsed_time" =~ ^[0-9]+(\.[0-9]+)?$ ]]; then
        if command -v bc >/dev/null 2>&1; then
            TOTAL_RUNNING_TIME=$(echo "$TOTAL_RUNNING_TIME + $elapsed_time" | bc)
        else
            TOTAL_RUNNING_TIME=$(awk "BEGIN {print ($TOTAL_RUNNING_TIME + $elapsed_time); exit}")
        fi
    fi
done

# Format both times as HH:MM:SS.ZZZ
TOTAL_ELAPSED_FORMATTED=$(format_time_duration "$TOTAL_ELAPSED")
TOTAL_RUNNING_TIME_FORMATTED=$(format_time_duration "$TOTAL_RUNNING_TIME")

# Get coverage percentages for display
UNITY_COVERAGE=$(get_unity_coverage)
BLACKBOX_COVERAGE=$(get_blackbox_coverage)
COMBINED_COVERAGE=$(get_combined_coverage)

# Run coverage table before displaying test results
coverage_table_script="$SCRIPT_DIR/lib/coverage_table.sh"
if [[ -x "$coverage_table_script" ]] && [ "$SKIP_TESTS" = false ]; then
    "$coverage_table_script" 2>/dev/null || true
fi

# Generate summary table using the sourced table libraries
# Create layout JSON string

layout_json_content='{
    "title": "Test Suite Results {NC}{RED}———{RESET}{BOLD} Unity: '"$UNITY_COVERAGE"'% {RESET}{RED}———{RESET}{BOLD} Blackbox: '"$BLACKBOX_COVERAGE"'% {RESET}{RED}———{RESET}{BOLD} Combined: '"$COMBINED_COVERAGE"'%",
    "footer": "Cumulative: '"$TOTAL_RUNNING_TIME_FORMATTED"'{RED} ——— {RESET}{CYAN}Elapsed: '"$TOTAL_ELAPSED_FORMATTED"'",
    "footer_position": "right",
    "columns": [
        {
            "header": "Group",
            "key": "group",
            "datatype": "int",
            "width": 5,
            "visible": false,
            "break": true
        },
        {
            "header": "Test #",
            "key": "test_id",
            "datatype": "text",
            "width": 8,
            "summary": "count",
	    "justification": "right"
        },
        {
            "header": "Test Name",
            "key": "test_name",
            "datatype": "text",
            "width": 50,
            "justification": "left"
        },
        {
            "header": "Tests",
            "key": "tests",
            "datatype": "int",
            "width": 8,
            "justification": "right",
            "summary": "sum"
        },
        {
            "header": "Pass",
            "key": "pass",
            "datatype": "int",
            "width": 8,
            "justification": "right",
            "summary": "sum"
        },
        {
            "header": "Fail",
            "key": "fail",
            "datatype": "int",
            "width": 8,
            "justification": "right",
            "summary": "sum"
        },
        {
            "header": "Duration",
            "key": "elapsed",
            "datatype": "float",
            "width": 11,
            "justification": "right",
            "summary": "sum"
        }
    ]
}'

# Create data JSON string
data_json_content="["
for i in "${!TEST_NUMBERS[@]}"; do
    test_id="${TEST_NUMBERS[$i]}-000"
    # Calculate group (tens digit) from test number
    group=$((${TEST_NUMBERS[$i]} / 10))
    
    if [ "$i" -gt 0 ]; then
        data_json_content+=","
    fi
    data_json_content+='
    {
        "group": '"$group"',
        "test_id": "'"$test_id"'",
        "test_name": "'"${TEST_NAMES[$i]}"'",
        "tests": '"${TEST_SUBTESTS[$i]}"',
        "pass": '"${TEST_PASSED[$i]}"',
        "fail": '"${TEST_FAILED[$i]}"',
        "elapsed": '"${TEST_ELAPSED[$i]}"'
    }'
done
data_json_content+="]"

# Use tables executable to render the summary table
temp_dir=$(mktemp -d 2>/dev/null) || { echo "Error: Failed to create temporary directory"; exit 1; }
layout_json="$temp_dir/summary_layout.json"
data_json="$temp_dir/summary_data.json"

echo "$layout_json_content" > "$layout_json"
echo "$data_json_content" > "$data_json"

tables_exe="$SCRIPT_DIR/lib/tables"
if [[ -x "$tables_exe" ]]; then
    "$tables_exe" "$layout_json" "$data_json" 2>/dev/null
fi

rm -rf "$temp_dir" 2>/dev/null

# Update README.md with test results (only if not in skip mode)
if [ "$SKIP_TESTS" = false ]; then
    update_readme_with_results
fi

exit $OVERALL_EXIT_CODE
