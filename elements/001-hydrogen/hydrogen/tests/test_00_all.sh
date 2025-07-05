#!/bin/bash
#
# Test 00: Test Suite Runner
# Executes all tests in parallel batches or sequentially and generates a summary report
#
# Usage: ./test_00_all.sh [test_name1 test_name2 ...] [--skip-tests] [--sequential] [--help]
#
# VERSION HISTORY
# 4.0.0 - 2025-07-04 - Added dynamic parallel execution with group-based batching for significant performance improvement
# 3.0.1 - 2025-07-03 - Enhanced --help to list test names, suppressed non-table output, updated history
# 3.0.0 - 2025-07-02 - Complete rewrite to use lib/ scripts, simplified orchestration approach
#

# Get the directory where this script is located
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

# Source the table rendering libraries for the summary
# shellcheck source=tests/lib/tables.sh
source "$SCRIPT_DIR/lib/tables.sh"
# shellcheck source=tests/lib/log_output.sh
source "$SCRIPT_DIR/lib/log_output.sh"
# shellcheck source=tests/lib/framework.sh
source "$SCRIPT_DIR/lib/framework.sh"
# shellcheck source=tests/lib/cloc.sh
source "$SCRIPT_DIR/lib/cloc.sh"

# Set flag to indicate tables.sh is sourced for performance optimization
export TABLES_SOURCED=true

# Test configuration
TEST_NAME="Test Suite Runner"
SCRIPT_VERSION="4.0.0"

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

# Results directory
RESULTS_DIR="$SCRIPT_DIR/results"
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

# If no test arguments provided, run all tests by default
if [ ${#TEST_ARGS[@]} -eq 0 ]; then
    # No specific tests provided, so run all tests
    :
fi

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
    echo "  $0                                    # Run all tests in parallel batches"
    echo "  $0 --sequential                       # Run all tests sequentially"
    if [ ${#TEST_SCRIPTS[@]} -ge 1 ]; then
        local first_test
        first_test=$(basename "${TEST_SCRIPTS[0]}" .sh | sed 's/test_//')
        echo "  $0 $first_test                        # Run only the first test"
    else
        echo "  $0 <test_name>                        # Run a specific test"
    fi
    if [ ${#TEST_SCRIPTS[@]} -ge 2 ]; then
        local random_index1=$((RANDOM % ${#TEST_SCRIPTS[@]}))
        local random_index2=$((RANDOM % ${#TEST_SCRIPTS[@]}))
        while [ $random_index2 -eq $random_index1 ]; do
            random_index2=$((RANDOM % ${#TEST_SCRIPTS[@]}))
        done
        local random_test1
        random_test1=$(basename "${TEST_SCRIPTS[$random_index1]}" .sh | sed 's/test_//')
        local random_test2
        random_test2=$(basename "${TEST_SCRIPTS[$random_index2]}" .sh | sed 's/test_//')
        echo "  $0 $random_test1 $random_test2        # Run two specific tests"
    else
        echo "  $0 <test1> <test2>                    # Run two specific tests"
    fi
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

# Check for help flag
for arg in "$@"; do
    if [ "$arg" == "--help" ] || [ "$arg" == "-h" ]; then
        show_help
    fi
done

# Check if --skip-tests is provided as any argument
for arg in "$@"; do
    if [ "$arg" == "--skip-tests" ]; then
        SKIP_TESTS=true
    fi
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
        "$SCRIPT_DIR/../build_unity_tests"
    )
    
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
        echo "Would run: $test_script"
        # Store skipped test results
        TEST_NUMBERS+=("$test_number")
        TEST_NAMES+=("$test_name")
        TEST_SUBTESTS+=(1)
        TEST_PASSED+=(0)
        TEST_FAILED+=(0)
        TEST_ELAPSED+=("0.000")
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
    
    # Calculate elapsed time
    local test_end
    test_end=$(date +%s.%N 2>/dev/null || date +%s)
    local elapsed
    if [[ "$test_start" =~ \. && "$test_end" =~ \. ]]; then
        elapsed=$(echo "$test_end - $test_start" | bc 2>/dev/null || echo "$((test_end - test_start))")
    else
        elapsed=$((test_end - test_start))
    fi
    
    # Format elapsed time (without 's' suffix for table summation)
    local elapsed_formatted
    if [[ "$elapsed" =~ \. ]]; then
        elapsed_formatted=$(printf "%.3f" "$elapsed")
    else
        elapsed_formatted="$elapsed"
    fi
    
    # Look for subtest results file
    local total_subtests=1
    local passed_subtests=0
    
    # Find the most recent subtest file for this test
    local latest_subtest_file
    latest_subtest_file=$(find "$RESULTS_DIR" -name "subtest_${test_number}_*.txt" -type f 2>/dev/null | sort -r | head -1)
    
    if [ -n "$latest_subtest_file" ] && [ -f "$latest_subtest_file" ]; then
        # Read subtest results and test name
        IFS=',' read -r total_subtests passed_subtests test_name < "$latest_subtest_file" 2>/dev/null || {
            total_subtests=1
            passed_subtests=$([ $exit_code -eq 0 ] && echo 1 || echo 0)
            test_name=$(basename "$test_script" .sh | sed 's/test_[0-9]*_//' | tr '_' ' ' | sed 's/\b\w/\U&/g')
        }
    else
        # Default based on exit code if no file found
        total_subtests=1
        passed_subtests=$([ $exit_code -eq 0 ] && echo 1 || echo 0)
        test_name=$(basename "$test_script" .sh | sed 's/test_[0-9]*_//' | tr '_' ' ' | sed 's/\b\w/\U&/g')
        echo "Warning: No subtest result file found for test ${test_number}"
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
    
    # Calculate elapsed time
    local test_end
    test_end=$(date +%s.%N 2>/dev/null || date +%s)
    local elapsed
    if [[ "$test_start" =~ \. && "$test_end" =~ \. ]]; then
        elapsed=$(echo "$test_end - $test_start" | bc 2>/dev/null || echo "$((test_end - test_start))")
    else
        elapsed=$((test_end - test_start))
    fi
    
    # Format elapsed time (without 's' suffix for table summation)
    local elapsed_formatted
    if [[ "$elapsed" =~ \. ]]; then
        elapsed_formatted=$(printf "%.3f" "$elapsed")
    else
        elapsed_formatted="$elapsed"
    fi
    
    # Look for subtest results file
    local total_subtests=1
    local passed_subtests=0
    
    # Find the most recent subtest file for this test
    local latest_subtest_file
    latest_subtest_file=$(find "$RESULTS_DIR" -name "subtest_${test_number}_*.txt" -type f 2>/dev/null | sort -r | head -1)
    
    if [ -n "$latest_subtest_file" ] && [ -f "$latest_subtest_file" ]; then
        # Read subtest results and test name
        IFS=',' read -r total_subtests passed_subtests test_name < "$latest_subtest_file" 2>/dev/null || {
            total_subtests=1
            passed_subtests=$([ $exit_code -eq 0 ] && echo 1 || echo 0)
            test_name=$(basename "$test_script" .sh | sed 's/test_[0-9]*_//' | tr '_' ' ' | sed 's/\b\w/\U&/g')
        }
    else
        # Default based on exit code if no file found
        total_subtests=1
        passed_subtests=$([ $exit_code -eq 0 ] && echo 1 || echo 0)
        test_name=$(basename "$test_script" .sh | sed 's/test_[0-9]*_//' | tr '_' ' ' | sed 's/\b\w/\U&/g')
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
    TOTAL_ELAPSED=$(echo "$END_TIME - $START_TIME" | bc 2>/dev/null || echo "$((END_TIME - START_TIME))")
else
    TOTAL_ELAPSED=$((END_TIME - START_TIME))
fi

# Calculate total running time (sum of all test execution times)
TOTAL_RUNNING_TIME=0
for i in "${!TEST_ELAPSED[@]}"; do
    if [[ "${TEST_ELAPSED[$i]}" =~ ^[0-9]+(\.[0-9]+)?$ ]]; then
        if command -v bc >/dev/null 2>&1; then
            TOTAL_RUNNING_TIME=$(echo "$TOTAL_RUNNING_TIME + ${TEST_ELAPSED[$i]}" | bc)
        else
            TOTAL_RUNNING_TIME=$(awk "BEGIN {print ($TOTAL_RUNNING_TIME + ${TEST_ELAPSED[$i]}); exit}")
        fi
    fi
done

# Format both times as HH:MM:SS.ZZZ
TOTAL_ELAPSED_FORMATTED=$(format_time_duration "$TOTAL_ELAPSED")
TOTAL_RUNNING_TIME_FORMATTED=$(format_time_duration "$TOTAL_RUNNING_TIME")

# Generate summary table using the sourced table libraries
# Create layout JSON string
layout_json_content='{
    "title": "Test Suite Results",
    "footer": "Elapsed: '"$TOTAL_ELAPSED_FORMATTED"' ─── Running: '"$TOTAL_RUNNING_TIME_FORMATTED"'",
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
            "summary": "count"
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
            "header": "Elapsed",
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

# Use sourced tables.sh function to render the summary table directly
tables_render_from_json "$layout_json_content" "$data_json_content"

# Update README.md with test results (only if not in skip mode)
if [ "$SKIP_TESTS" = false ]; then
    update_readme_with_results
fi

exit $OVERALL_EXIT_CODE
