#!/bin/bash
#
# Hydrogen Test Runner
# Executes all tests with standardized formatting and generates a summary report
#
# Usage: ./test_all.sh [test_name|min|max|all] [--skip-tests]

# About this Script
NAME_TEST_00="Hydrogen Test Runner"
VERS_TEST_00="1.0.3"

# VERSION HISTORY
# 1.0.5 - 2025-06-20 - Adjusted timeouts to reduce test_00_all.sh overhead execution time, minor tweaks
# 1.0.4 - 2025-06-20 - Added test execution time, upgraded timing to millisecond precision
# 1.0.3 - 2025-06-17 - Major refactoring: eliminated code duplication, improved modularity, enhanced comments
# 1.0.2 - 2025-06-16 - Version history and title added
# 1.0.1 - 2025-06-16 - Changes to support shellcheck validation
# 1.0.0 - 2025-06-16 - Version history started

# Test result constants for clarity
readonly TEST_RESULT_PASS=0
readonly TEST_RESULT_FAIL=1
readonly TEST_RESULT_SKIP=2

# Get the directory where this script is located
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

# Include the common test utilities
source "$SCRIPT_DIR/support_utils.sh"
source "$SCRIPT_DIR/support_cleanup.sh"

# Create results directory if it doesn't exist
RESULTS_DIR="$SCRIPT_DIR/results"
mkdir -p "$RESULTS_DIR"

# Get timestamp for this test run
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
SUMMARY_LOG="$RESULTS_DIR/test_summary_${TIMESTAMP}.log"

# Arrays to track all tests
declare -a ALL_TEST_NAMES
declare -a ALL_TEST_RESULTS
declare -a ALL_TEST_DETAILS
declare -a ALL_TEST_SUBTESTS
declare -a ALL_TEST_PASSED_SUBTESTS
declare -a ALL_TEST_DURATIONS

# Start time for test runtime calculation with millisecond precision if possible
if date --version >/dev/null 2>&1; then
    START_TIME=$(date +%s.%N)
else
    START_TIME=$(date +%s)
fi

    # Check for --help option first, before any output or processing
    for arg in "$@"; do
        if [ "$arg" == "--help" ] || [ "$arg" == "-h" ]; then
        echo "Usage: $0 [test_name|min|max|all] [--skip-tests] [--help|-h]"
        echo "  test_name: Run a specific test (e.g., compilation, startup_shutdown)"
        echo "  min: Run with minimal configuration only"
        echo "  max: Run with maximal configuration only"
        echo "  all: Run all tests (default)"
        echo "  --skip-tests: Skip actual test execution, just show what tests would run"
        echo "  --help|-h: Display this help message"
        exit 0
    fi
done

# Print header with version
print_header "$NAME_TEST_00 v$VERS_TEST_00" | tee "$SUMMARY_LOG"
echo "Started at: $(date)" | tee -a "$SUMMARY_LOG"
echo "" | tee -a "$SUMMARY_LOG"

# Clean up before starting tests
cleanup_old_tests
ensure_server_not_running

# Make all test scripts executable
make_scripts_executable

# Helper function to format duration in HH:MM:SS or HH:MM:SS.FFF if milliseconds are available
format_duration() {
    local duration=$1
    if [[ "$duration" =~ \. ]]; then
        # Duration includes milliseconds
        IFS='.' read -r secs millis <<< "$duration"
        local hours=$((secs / 3600))
        local minutes=$(((secs % 3600) / 60))
        local seconds=$((secs % 60))
        # Extract first 3 digits of milliseconds if available, remove leading zeros
        local ms=${millis:0:3}
        ms=$((10#${ms:-0}))  # Force decimal interpretation to avoid octal error
        printf "%02d:%02d:%02d.%03d" $hours $minutes $seconds "$ms"
    else
        # Duration is in whole seconds
        local hours=$((duration / 3600))
        local minutes=$(((duration % 3600) / 60))
        local seconds=$((duration % 60))
        printf "%02d:%02d:%02d" $hours $minutes $seconds
    fi
}

# Helper function to handle subtest results file processing
# Reads subtest counts from file or defaults to single test based on exit code
process_subtest_results() {
    local test_name="$1"
    local test_exit_code="$2"
    local subtest_file="$RESULTS_DIR/subtest_${test_name}.txt"
    local total_subtests=1
    local passed_subtests=0
    
    if [ -f "$subtest_file" ]; then
        # Read subtest results from file
        IFS=',' read -r total_subtests passed_subtests < "$subtest_file"
        print_info "Found subtest results: $passed_subtests of $total_subtests subtests passed" | tee -a "$SUMMARY_LOG"
        # Archive the file for debugging with timestamp
        mv "$subtest_file" "${subtest_file}.${TIMESTAMP}"
    else
        # Default to single subtest based on exit code
        total_subtests=1
        if [ "$test_exit_code" -eq $TEST_RESULT_PASS ]; then
            passed_subtests=1
        else
            passed_subtests=0
        fi
        # Create archived subtest file for debugging consistency
        echo "${total_subtests},${passed_subtests}" > "${subtest_file}.${TIMESTAMP}"
    fi
    
    # Return values via global variables (bash limitation workaround)
    SUBTEST_TOTAL=$total_subtests
    SUBTEST_PASSED=$passed_subtests
}

# Helper function to include test execution log on failure
# Searches for and includes the most recent log file for failed tests
include_test_log_on_failure() {
    local test_name="$1"
    
    # Create pattern to find log files for this test (handles underscores in names)
    local log_pattern
    log_pattern="*$(echo "$test_name" | tr '_' '*')*.log"
    local latest_log
    latest_log=$(find "$RESULTS_DIR" -type f -name "$log_pattern" | sort -r | head -1)
    
    if [ -n "$latest_log" ] && [ -f "$latest_log" ]; then
        echo "" | tee -a "$SUMMARY_LOG"
        echo "==== TEST EXECUTION LOG ====" | tee -a "$SUMMARY_LOG"
        tee -a "$SUMMARY_LOG" < "$latest_log"
        echo "==== END OF TEST EXECUTION LOG ====" | tee -a "$SUMMARY_LOG"
    fi
}

# Helper function to record test results in global arrays
# Centralizes the logic for storing test outcome data
record_test_results() {
    local test_name="$1"
    local test_result="$2"
    local test_details="$3"
    local total_subtests="$4"
    local passed_subtests="$5"
    local test_duration="$6"
    
    ALL_TEST_NAMES+=("$test_name")
    ALL_TEST_RESULTS+=("$test_result")
    ALL_TEST_DETAILS+=("$test_details")
    ALL_TEST_SUBTESTS+=("$total_subtests")
    ALL_TEST_PASSED_SUBTESTS+=("$passed_subtests")
    
    # Only record duration for executed tests (not skipped)
    if [ "$test_result" -ne $TEST_RESULT_SKIP ]; then
        ALL_TEST_DURATIONS+=("$test_duration")
    fi
}

# Core function to register a test without executing it (for --skip-tests mode)
register_test_without_executing() {
    local test_script="$1"
    local config_file="$2"  # Optional parameter
    local test_name
    test_name=$(basename "$test_script" .sh | sed 's/^test_//')
    local relative_script
    relative_script=$(convert_to_relative_path "$test_script")
    
    # Build display name and command based on whether config is provided
    local display_name="$test_name"
    local command_display="$relative_script"
    
    if [ -n "$config_file" ]; then
        local config_name
        config_name=$(basename "$config_file" .json)
        local config_path
        config_path=$(get_config_path "$config_file")
        display_name="$test_name ($config_name)"
        command_display="$relative_script $config_path"
    fi
    
    print_header "Skipping Test: $display_name" | tee -a "$SUMMARY_LOG"
    print_info "Test would run command: $command_display" | tee -a "$SUMMARY_LOG"
    
    # Record skipped test with default subtest counts
    print_result $TEST_RESULT_SKIP "Test $display_name skipped" | tee -a "$SUMMARY_LOG"
    record_test_results "$display_name" $TEST_RESULT_SKIP "Test was skipped" 1 0 ""
    
    echo "" | tee -a "$SUMMARY_LOG"
    return 0
}

# Core function to run a test script with optional configuration
# This consolidates the previously duplicated run_test() and run_test_with_config() functions
run_test_internal() {
    local test_script="$1"
    local config_file="$2"  # Optional parameter - empty string if no config
    
    # Handle skip mode by delegating to registration function
    if [ "$SKIP_TESTS" = true ]; then
        register_test_without_executing "$test_script" "$config_file"
        return 0
    fi
    
    local test_start_time
    if date --version >/dev/null 2>&1; then
        test_start_time=$(date +%s.%N)
    else
        test_start_time=$(date +%s)
    fi
    
    # Extract test name from script filename
    local test_name
    test_name=$(basename "$test_script" .sh | sed 's/^test_//')
    
    # Build display name and execution command based on config presence
    local display_name="$test_name"
    local execution_command="$test_script"
    
    if [ -n "$config_file" ]; then
        local config_name
        config_name=$(basename "$config_file" .json)
        local config_path
        config_path=$(get_config_path "$config_file")
        display_name="$test_name ($config_name)"
        execution_command="$test_script $config_path"
    fi
    
    print_header "Running Test: $display_name" | tee -a "$SUMMARY_LOG"
    
    # Execute the test script with or without config parameter
    print_command "$execution_command" | tee -a "$SUMMARY_LOG"
    if [ -n "$config_file" ]; then
        local config_path
        config_path=$(get_config_path "$config_file")
        $test_script "$config_path"
    else
        $test_script
    fi
    local test_exit_code=$?
    
    # Process subtest results using helper function
    process_subtest_results "$test_name" "$test_exit_code"
    local total_subtests=$SUBTEST_TOTAL
    local passed_subtests=$SUBTEST_PASSED
    
    # Calculate test duration
    local test_end_time
    if date --version >/dev/null 2>&1; then
        test_end_time=$(date +%s.%N)
        # Calculate duration with milliseconds if possible
        if [[ "$test_start_time" =~ \. && "$test_end_time" =~ \. ]]; then
            test_duration=$(echo "$test_end_time - $test_start_time" | bc)
        else
            test_duration=$((test_end_time - test_start_time))
        fi
    else
        test_end_time=$(date +%s)
        test_duration=$((test_end_time - test_start_time))
    fi
    
    # Record results and handle success/failure reporting
    if [ $test_exit_code -eq $TEST_RESULT_PASS ]; then
        print_result $TEST_RESULT_PASS "Test $display_name completed successfully" | tee -a "$SUMMARY_LOG"
        record_test_results "$display_name" $TEST_RESULT_PASS "Test completed without errors" "$total_subtests" "$passed_subtests" "$test_duration"
    else
        print_result $TEST_RESULT_FAIL "Test $display_name failed with exit code $test_exit_code" | tee -a "$SUMMARY_LOG"
        record_test_results "$display_name" $TEST_RESULT_FAIL "Test failed with errors" "$total_subtests" "$passed_subtests" "$test_duration"
        
        # Include execution log for failed tests
        include_test_log_on_failure "$test_name"
    fi
    
    echo "" | tee -a "$SUMMARY_LOG"
    
    # Ensure server cleanup only when not in skip mode (quick mode for inter-test cleanup)
    if [ "$SKIP_TESTS" = false ]; then
        ensure_server_not_running true
    fi
    
    return $test_exit_code
}

# Public interface functions that delegate to the internal implementation
run_test() {
    run_test_internal "$1" ""
}

run_test_with_config() {
    run_test_internal "$1" "$2"
}

# Function to discover and run all tests
# Runs compilation test first, then all other tests in sorted order
run_all_tests() {
    # Run compilation test first - it's critical for other tests to succeed
    print_header "Running Compilation Test" | tee -a "$SUMMARY_LOG"
    run_test "$SCRIPT_DIR/test_10_compilation.sh"
    local compilation_exit_code=$?
    
    # Only proceed with other tests if compilation passes (unless in skip mode)
    if [ $compilation_exit_code -ne 0 ] && [ "$SKIP_TESTS" = false ]; then
        print_warning "Compilation failed - skipping remaining tests" | tee -a "$SUMMARY_LOG"
        return $compilation_exit_code
    fi
    
    # Find and run all test scripts in sorted order, excluding special cases
    local all_exit_codes=0
    local test_scripts_list
    test_scripts_list=$(find "$SCRIPT_DIR" -type f -name "test_*.sh" | grep -v "test_00_all.sh" | grep -v "test_10_compilation.sh" | grep -v "test_template.sh" | sort)
    # Removed unused variables test_scripts and all_exit_codes_inner
    while IFS= read -r test_script; do
        if [ -n "$test_script" ]; then
        run_test "$test_script"
        local test_exit_code=$?
        
        # Track any failures (unless in skip mode)
        if [ $test_exit_code -ne 0 ] && [ "$SKIP_TESTS" = false ]; then
            all_exit_codes=1
        fi
        
        # Ensure server cleanup between tests (unless in skip mode) - use quick mode
        if [ "$SKIP_TESTS" = false ]; then
            ensure_server_not_running true
        fi
            echo "" | tee -a "$SUMMARY_LOG"
        fi
    done <<< "$test_scripts_list"
    
    # Calculate overall result based on compilation and individual test results
    if [ "$SKIP_TESTS" = true ]; then
        return 0
    elif [ $compilation_exit_code -eq 0 ] && [ $all_exit_codes -eq 0 ]; then
        return 0
    else
        return 1
    fi
}

# Helper function to run prerequisite tests (environment and compilation)
# Centralizes the common pattern of running env and compilation tests before others
run_prerequisite_tests() {
    local test_name="$1"  # Name of the main test being run (for skip logic)
    
    # Run environment variable test first (unless it's the test being requested)
    if [ "$test_name" != "12_env_payload" ]; then
        print_header "Running Environment Variable Test" | tee -a "$SUMMARY_LOG"
        run_test "$SCRIPT_DIR/test_12_env_payload.sh"
        local env_exit_code=$?
        
        # Only proceed if environment variables are properly set
        if [ $env_exit_code -ne 0 ]; then
            print_warning "Environment variable test failed - skipping remaining tests" | tee -a "$SUMMARY_LOG"
            return $env_exit_code
        fi

        # Run compilation test next (unless it's the test being requested)
        if [ "$test_name" != "10_compilation" ]; then
            print_header "Running Compilation Test" | tee -a "$SUMMARY_LOG"
            run_test "$SCRIPT_DIR/test_10_compilation.sh"
            local compilation_exit_code=$?
            
            # Only proceed if compilation passes
            if [ $compilation_exit_code -ne 0 ]; then
                print_warning "Compilation failed - skipping $test_name test" | tee -a "$SUMMARY_LOG"
                return $compilation_exit_code
            fi
        fi
    fi
    
    return 0
}

# Function to run minimal configuration test only
run_min_configuration_test() {
    run_prerequisite_tests "min_config"
    local prereq_exit_code=$?
    
    if [ $prereq_exit_code -ne 0 ]; then
        return $prereq_exit_code
    fi
    
    # Run startup/shutdown test with minimal configuration
    print_header "Running Startup/Shutdown Test (Min Config)" | tee -a "$SUMMARY_LOG"
    run_test_with_config "$SCRIPT_DIR/test_15_startup_shutdown.sh" "hydrogen_test_min.json"
    return $?
}

# Function to run maximal configuration test only
run_max_configuration_test() {
    run_prerequisite_tests "max_config"
    local prereq_exit_code=$?
    
    if [ $prereq_exit_code -ne 0 ]; then
        return $prereq_exit_code
    fi
    
    # Run startup/shutdown test with maximal configuration
    print_header "Running Startup/Shutdown Test (Max Config)" | tee -a "$SUMMARY_LOG"
    run_test_with_config "$SCRIPT_DIR/test_15_startup_shutdown.sh" "hydrogen_test_max.json"
    return $?
}

# Function to run a specific named test
run_specific_test() {
    local test_name="$1"
    
    # Check if the test script exists
    if [ -f "$SCRIPT_DIR/test_${test_name}.sh" ]; then
        # Handle skip mode
        if [ "$SKIP_TESTS" = true ]; then
            print_header "Skipping Requested Test: $test_name" | tee -a "$SUMMARY_LOG"
            register_test_without_executing "$SCRIPT_DIR/test_${test_name}.sh"
            return 0
        fi
        
        # Run prerequisite tests if needed
        run_prerequisite_tests "$test_name"
        local prereq_exit_code=$?
        
        if [ $prereq_exit_code -ne 0 ]; then
            return $prereq_exit_code
        fi
        
        # Run the requested test
        print_header "Running Requested Test: $test_name" | tee -a "$SUMMARY_LOG"
        run_test "$SCRIPT_DIR/test_${test_name}.sh"
        return $?
    else
        print_result $TEST_RESULT_FAIL "Test script not found: test_${test_name}.sh" | tee -a "$SUMMARY_LOG"
        return 1
    fi
}

# Custom print_test_item function to handle test result display with duration
print_test_item() {
    local result=$1
    local test_name=$2
    local details=$3
    local duration=$4
    
    # Format the duration if provided, otherwise use empty string
    local duration_str=""
    if [ -n "$duration" ]; then
        duration_str="${NC}$(format_duration "$duration")${GREEN} "
    fi
    
    # Display with appropriate icon and color based on result
    if [ "$result" -eq $TEST_RESULT_PASS ]; then
        echo -e "${GREEN}${PASS_ICON} ${duration_str}${test_name}${NC} - ${details}"
    elif [ "$result" -eq $TEST_RESULT_SKIP ]; then
        echo -e "${YELLOW}⏭️  ${duration_str}${test_name}${NC} - ${details}"
    else
        echo -e "${RED}${FAIL_ICON} ${duration_str}${test_name}${NC} - ${details}"
    fi
}

# Helper function to calculate test statistics
# Separates the counting logic from the display logic
calculate_test_statistics() {
    local pass_count=0
    local fail_count=0
    local skip_count=0
    local total_count=${#ALL_TEST_NAMES[@]}
    
    # Count results by type
    for result in "${ALL_TEST_RESULTS[@]}"; do
        if [ "$result" -eq $TEST_RESULT_PASS ]; then
            ((pass_count++))
        elif [ "$result" -eq $TEST_RESULT_SKIP ]; then
            ((skip_count++))
        else
            ((fail_count++))
        fi
    done
    
    # Calculate subtest totals (excluding skipped tests)
    local total_subtests=0
    local total_passed_subtests=0
    for i in "${!ALL_TEST_SUBTESTS[@]}"; do
        if [ "${ALL_TEST_RESULTS[$i]}" -ne $TEST_RESULT_SKIP ]; then
            total_subtests=$((total_subtests + ${ALL_TEST_SUBTESTS[$i]}))
            total_passed_subtests=$((total_passed_subtests + ${ALL_TEST_PASSED_SUBTESTS[$i]}))
        fi
    done
    local total_failed_subtests=$((total_subtests - total_passed_subtests))
    
    # Calculate runtime
    local end_time
    if date --version >/dev/null 2>&1; then
        end_time=$(date +%s.%N)
        if [[ "$START_TIME" =~ \. && "$end_time" =~ \. ]]; then
            total_runtime=$(echo "$end_time - $START_TIME" | bc)
        else
            total_runtime=$((end_time - START_TIME))
        fi
    else
        end_time=$(date +%s)
        total_runtime=$((end_time - START_TIME))
    fi
    local runtime_formatted
    runtime_formatted=$(format_duration "$total_runtime")
    
    # Return values via global variables (bash limitation workaround)
    STATS_PASS_COUNT=$pass_count
    STATS_FAIL_COUNT=$fail_count
    STATS_SKIP_COUNT=$skip_count
    STATS_TOTAL_COUNT=$total_count
    STATS_TOTAL_SUBTESTS=$total_subtests
    STATS_PASSED_SUBTESTS=$total_passed_subtests
    STATS_FAILED_SUBTESTS=$total_failed_subtests
    STATS_RUNTIME=$runtime_formatted
}

# Function to print individual test results
print_individual_test_results() {
    echo -e "${BLUE}${BOLD}Individual Test Results:${NC}" | tee -a "$SUMMARY_LOG"
    for i in "${!ALL_TEST_NAMES[@]}"; do
        # Add subtest count to the details
        local total_subtests=${ALL_TEST_SUBTESTS[$i]}
        local passed_subtests=${ALL_TEST_PASSED_SUBTESTS[$i]}
        
        local subtest_info
        if [ "${ALL_TEST_RESULTS[$i]}" -eq $TEST_RESULT_SKIP ]; then
            subtest_info="(Skipped)"
        else
            subtest_info="(${passed_subtests} of ${total_subtests} Passed)"
        fi
        
        local test_info="${ALL_TEST_NAMES[$i]} ${subtest_info}"
        local duration_str=""
        if [ "${ALL_TEST_RESULTS[$i]}" -ne $TEST_RESULT_SKIP ]; then
            duration_str="${ALL_TEST_DURATIONS[$i]}"
        fi
        print_test_item "${ALL_TEST_RESULTS[$i]}" "${test_info}" "${ALL_TEST_DETAILS[$i]}" "$duration_str" | tee -a "$SUMMARY_LOG"
    done
}

# Function to print summary statistics
# Refactored to be more modular and easier to maintain
print_summary_statistics() {
    local exit_code=$1
    
    # Calculate all statistics using helper function
    calculate_test_statistics
    
    # Print individual test results
    print_individual_test_results
    
    # Print comprehensive statistics 
    echo "" | tee -a "$SUMMARY_LOG"
    echo -e "${BLUE}${BOLD}Summary Statistics:${NC}" | tee -a "$SUMMARY_LOG"
    echo -e "Total tests: ${STATS_TOTAL_COUNT}" | tee -a "$SUMMARY_LOG"
    echo -e "Passed: ${STATS_PASS_COUNT}" | tee -a "$SUMMARY_LOG"
    echo -e "Failed: ${STATS_FAIL_COUNT}" | tee -a "$SUMMARY_LOG"
    echo -e "Skipped: ${STATS_SKIP_COUNT}" | tee -a "$SUMMARY_LOG"
    echo -e "Total subtests: ${STATS_TOTAL_SUBTESTS}" | tee -a "$SUMMARY_LOG"
    echo -e "Passed subtests: ${STATS_PASSED_SUBTESTS}" | tee -a "$SUMMARY_LOG"
    echo -e "Failed subtests: ${STATS_FAILED_SUBTESTS}" | tee -a "$SUMMARY_LOG"
    echo -e "Test runtime: ${STATS_RUNTIME}" | tee -a "$SUMMARY_LOG"
    # Calculate total test execution time (sum of individual test durations)
    local total_execution=0.0
    for duration in "${ALL_TEST_DURATIONS[@]}"; do
        if [[ "$duration" =~ \. ]]; then
            total_execution=$(echo "$total_execution + $duration" | bc)
        else
            total_execution=$(echo "$total_execution + $duration" | bc)
        fi
    done
    local execution_formatted
    execution_formatted=$(format_duration "$total_execution")
    echo -e "Test execution: ${execution_formatted}" | tee -a "$SUMMARY_LOG"
    echo "" | tee -a "$SUMMARY_LOG"
    
    # Print overall result with appropriate formatting
    if [ "$SKIP_TESTS" = true ]; then
        echo -e "${YELLOW}${BOLD}OVERALL RESULT: ALL TESTS SKIPPED${NC}" | tee -a "$SUMMARY_LOG"
    elif [ "$exit_code" -eq 0 ]; then
        echo -e "${GREEN}${BOLD}${PASS_ICON} OVERALL RESULT: ALL TESTS PASSED${NC}" | tee -a "$SUMMARY_LOG"
    else
        echo -e "${RED}${BOLD}${FAIL_ICON} OVERALL RESULT: ONE OR MORE TESTS FAILED${NC}" | tee -a "$SUMMARY_LOG"
    fi
    echo -e "${BLUE}───────────────────────────────────────────────────────────────────────────────${NC}" | tee -a "$SUMMARY_LOG"
    
    # Always generate README section
    generate_readme_section
}

# Function to check if a file should be excluded (mirroring logic from test_99_codebase.sh)
should_exclude_file() {
    local file="$1"
    local lint_ignore="$SCRIPT_DIR/../.lintignore"
    local rel_file="${file#"$SCRIPT_DIR/.."/}"
    
    # Check .lintignore file first if it exists
    if [ -f "$lint_ignore" ]; then
        while IFS= read -r pattern; do
            [[ -z "$pattern" || "$pattern" == \#* ]] && continue
            shopt -s extglob
            if [[ "$rel_file" == @($pattern) ]]; then
                shopt -u extglob
                return 0 # Exclude
            fi
            shopt -u extglob
        done < "$lint_ignore"
    fi
    
    # Check default excludes
    local LINT_EXCLUDES=(
        "build/*"
        "build_debug/*"
        "build_perf/*"
        "build_release/*"
        "build_valgrind/*"
        "tests/logs/*"
        "tests/results/*"
        "tests/diagnostics/*"
    )
    for pattern in "${LINT_EXCLUDES[@]}"; do
        shopt -s extglob
        if [[ "$rel_file" == @($pattern) ]]; then
            shopt -u extglob
            return 0 # Exclude
        fi
        shopt -u extglob
    done
    
    return 1 # Do not exclude
}

# Function to run cloc and generate repository information
generate_repository_info() {
    local repo_info_file="$RESULTS_DIR/repository_info.md"
    
    {
        echo "## Repository Information"
        echo ""
        echo "Generated via cloc: $(date)"
        echo ""
        echo "\`\`\`cloc"
    } > "$repo_info_file"
    
    # Save current directory and change to project root
    local start_dir
    start_dir=$(pwd)
    
    cd "$SCRIPT_DIR/.." || {
        print_warning "Failed to change to project directory for repository analysis" | tee -a "$SUMMARY_LOG"
        return 1
    }
    
    # Create a temporary exclude list based on .lintignore and default excludes using the same logic as test_99_codebase.sh
    local exclude_list
    exclude_list=$(mktemp)
    : > "$exclude_list"
    local start_dir
    start_dir=$(pwd)
    cd "$SCRIPT_DIR/.." || {
        print_warning "Failed to change to project directory for exclude list generation" | tee -a "$SUMMARY_LOG"
        return 1
    }
    while read -r file; do
        if should_exclude_file "$file"; then
            echo "${file#"$SCRIPT_DIR/.."/}" >> "$exclude_list"
        fi
    done < <(find "$SCRIPT_DIR/.." -type f | sort)
    cd "$start_dir" || {
        print_warning "Failed to return to start directory after exclude list generation" | tee -a "$SUMMARY_LOG"
    }
    
    # Run cloc with specific locale settings and exclude list
    if env LC_ALL=en_US.UTF_8 LC_TIME= LC_ALL= LANG= LANGUAGE= cloc . --quiet --force-lang="C,inc" --exclude-list-file="$exclude_list" >> "$repo_info_file" 2>&1; then
        echo "\`\`\`" >> "$repo_info_file"
        print_info "Generated repository information with cloc analysis" | tee -a "$SUMMARY_LOG"
    else
        {
            echo "### Code Summary"
            echo ""
            echo "Unable to generate code statistics. Make sure 'cloc' is installed."
        } >> "$repo_info_file"
        print_warning "Failed to run cloc analysis for repository information" | tee -a "$SUMMARY_LOG"
    fi
    
    # Clean up temporary file
    rm -f "$exclude_list"
    
    # Return to start directory
    cd "$start_dir" || {
        print_warning "Failed to return to start directory after repository analysis" | tee -a "$SUMMARY_LOG"
    }
    
    return 0
}

# Function to generate README section with test results
generate_readme_section() {
    local readme_section_file="$RESULTS_DIR/latest_test_results.md"
    
    # Calculate statistics for README generation
    calculate_test_statistics
    
    {
        echo "## Latest Test Results"
        echo ""
        echo "Generated on: $(date)"
        echo ""
        echo "### Summary"
        echo ""
        echo "| Metric | Value |"
        echo "| ------ | ----- |"
        echo "| Total Tests | ${STATS_TOTAL_COUNT} |"
        echo "| Passed | ${STATS_PASS_COUNT} |"
        echo "| Failed | ${STATS_FAIL_COUNT} |"
        echo "| Skipped | ${STATS_SKIP_COUNT} |"
        echo "| Total Subtests | ${STATS_TOTAL_SUBTESTS} |"
        echo "| Passed Subtests | ${STATS_PASSED_SUBTESTS} |"
        echo "| Failed Subtests | ${STATS_FAILED_SUBTESTS} |"
        echo "| Runtime | ${STATS_RUNTIME} |"
        echo ""
        echo "### Individual Test Results"
        echo ""
        echo "| Status | Time | Test | Subs | Pass | Summary |"
        echo "| ------ | ---- | ---- | ---- | ---- | ------- |"
    } > "$readme_section_file"
    
    # Generate individual test result rows
    for i in "${!ALL_TEST_NAMES[@]}"; do
        local status_icon
        if [ "${ALL_TEST_RESULTS[$i]}" -eq $TEST_RESULT_PASS ]; then
            status_icon="✅"
        elif [ "${ALL_TEST_RESULTS[$i]}" -eq $TEST_RESULT_SKIP ]; then
            status_icon="⏭️"
        else
            status_icon="❌"
        fi
        
        local duration_str=""
        if [ "${ALL_TEST_RESULTS[$i]}" -ne $TEST_RESULT_SKIP ]; then
            duration_str="$(format_duration "${ALL_TEST_DURATIONS[$i]}")"
        fi
        
        local test_name="${ALL_TEST_NAMES[$i]// \(*\)/}"  # Remove parenthetical info
        local total_subtests=${ALL_TEST_SUBTESTS[$i]}
        local passed_subtests=${ALL_TEST_PASSED_SUBTESTS[$i]}
        
        if [ "${ALL_TEST_RESULTS[$i]}" -eq $TEST_RESULT_SKIP ]; then
            echo "| $status_icon | - | $test_name | - | - | ${ALL_TEST_DETAILS[$i]} |" >> "$readme_section_file"
        else
            echo "| $status_icon | $duration_str | $test_name | $total_subtests | $passed_subtests | ${ALL_TEST_DETAILS[$i]} |" >> "$readme_section_file"
        fi
    done
    
    local relative_path
    relative_path=$(convert_to_relative_path "$readme_section_file")
    print_info "Prepared test results section in $relative_path" | tee -a "$SUMMARY_LOG"
    
    # Generate repository information as well
    generate_repository_info
    local repo_info_file="$RESULTS_DIR/repository_info.md"
    local repo_info_path
    repo_info_path=$(convert_to_relative_path "$repo_info_file")
    print_info "Prepared repository statistics from cloc in $repo_info_path" | tee -a "$SUMMARY_LOG"
    
    # Add to README.md if it exists
    local readme_file="$SCRIPT_DIR/../README.md"
    local readme_path="hydrogen/README.md"
    
    if [ -f "$readme_file" ]; then
        # Remove existing sections if they exist
        if grep -q "^## Latest Test Results" "$readme_file"; then
    local temp_file
    temp_file=$(mktemp)
    sed '/^## Latest Test Results/,/^## /{ /^## Latest Test Results/d; /^## /!d; }' "$readme_file" > "$temp_file"
    sed -i '/^## Repository Information/,/^## /{ /^## Repository Information/d; /^## /!d; }' "$temp_file"
            mv "$temp_file" "$readme_file"
        fi
        
        # Append new sections
        {
            cat "$readme_section_file"
            echo ""
            cat "$repo_info_file"
        } >> "$readme_file"
        print_result $TEST_RESULT_PASS "Added test results and repository statistics to $readme_path" | tee -a "$SUMMARY_LOG"
    else
        print_warning "README.md not found at $readme_file" | tee -a "$SUMMARY_LOG"
    fi
}

# Removed duplicate --help check as it is now handled earlier in the script

# Parse command line arguments
TEST_TYPE=${1:-"all"}  # Default to "all" if not specified
SKIP_TESTS=false

# Check if --skip-tests is provided as any argument
for arg in "$@"; do
    if [ "$arg" == "--skip-tests" ]; then
        SKIP_TESTS=true
    fi
done

# If the first argument is --skip-tests, set TEST_TYPE to "all"
if [ "$TEST_TYPE" == "--skip-tests" ]; then
    TEST_TYPE="all"
fi

# Run tests based on command line argument
case "$TEST_TYPE" in
    "min")
        print_info "Running test with minimal configuration only" | tee -a "$SUMMARY_LOG"
        run_min_configuration_test
        EXIT_CODE=$?
        ;;
    "max")
        print_info "Running test with maximal configuration only" | tee -a "$SUMMARY_LOG"
        run_max_configuration_test
        EXIT_CODE=$?
        ;;
    "all")
        print_info "Running all tests" | tee -a "$SUMMARY_LOG"
        if [ "$SKIP_TESTS" = true ]; then
            print_info "Skipping actual test execution (--skip-tests enabled)" | tee -a "$SUMMARY_LOG"
        fi
        run_all_tests
        EXIT_CODE=$?
        ;;
    *)
        # Check if it's a specific test name
        if [ -f "$SCRIPT_DIR/test_${TEST_TYPE}.sh" ]; then
            print_info "Running specific test: $TEST_TYPE" | tee -a "$SUMMARY_LOG"
            if [ "$SKIP_TESTS" = true ]; then
                print_info "Skipping actual test execution (--skip-tests enabled)" | tee -a "$SUMMARY_LOG"
            fi
            run_specific_test "$TEST_TYPE"
            EXIT_CODE=$?
        else
            print_warning "Invalid test type or test name: $TEST_TYPE" | tee -a "$SUMMARY_LOG"
            echo "Usage: $0 [test_name|min|max|all] [--skip-tests]" | tee -a "$SUMMARY_LOG"
            echo "  test_name: Run a specific test (e.g., compilation, startup_shutdown)" | tee -a "$SUMMARY_LOG"
            echo "  min: Run with minimal configuration only" | tee -a "$SUMMARY_LOG"
            echo "  max: Run with maximal configuration only" | tee -a "$SUMMARY_LOG"
            echo "  all: Run all tests (default)" | tee -a "$SUMMARY_LOG"
            echo "  --skip-tests: Skip actual test execution, just show what tests would run" | tee -a "$SUMMARY_LOG"
            EXIT_CODE=1
        fi
        ;;
esac

# Print overall summary
print_header "Test Summary" | tee -a "$SUMMARY_LOG"
echo "Started at: $(date -d @"$START_TIME")" | tee -a "$SUMMARY_LOG"
echo "Completed at: $(date)" | tee -a "$SUMMARY_LOG"
echo "Summary log: $(convert_to_relative_path "$SUMMARY_LOG")" | tee -a "$SUMMARY_LOG"
echo "" | tee -a "$SUMMARY_LOG"

# Print summary statistics
print_summary_statistics $EXIT_CODE

# Clean up any leftover log files in the test directory (not in results)
rm -f "$SCRIPT_DIR"/*.log

exit $EXIT_CODE
