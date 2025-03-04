#!/bin/bash
#
# Hydrogen Test Runner
# Executes all tests with standardized formatting and generates a summary report
#
# Usage: ./test_all.sh [test_name|min|max|all]

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

# Start time for test runtime calculation
START_TIME=$(date +%s)

# Print header
print_header "Hydrogen Test Runner" | tee "$SUMMARY_LOG"
echo "Started at: $(date)" | tee -a "$SUMMARY_LOG"
echo "" | tee -a "$SUMMARY_LOG"

# Clean up before starting tests
cleanup_old_tests
ensure_server_not_running

# Make all test scripts executable
make_scripts_executable

# Function to run a specific test script
run_test() {
    local test_script="$1"
    local test_name=$(basename "$test_script" .sh | sed 's/^test_//')
    
    print_header "Running Test: $test_name" | tee -a "$SUMMARY_LOG"
    
    # Execute the test script
    print_command "$test_script" | tee -a "$SUMMARY_LOG"
    $test_script
    local test_exit_code=$?
    
    # Record test results
    if [ $test_exit_code -eq 0 ]; then
        print_result 0 "Test $test_name completed successfully" | tee -a "$SUMMARY_LOG"
        ALL_TEST_NAMES+=("$test_name")
        ALL_TEST_RESULTS+=(0)
        ALL_TEST_DETAILS+=("Test completed without errors")
        ALL_TEST_SUBTESTS+=(1) # Default to 1 subtest if not specified
    else
        print_result 1 "Test $test_name failed with exit code $test_exit_code" | tee -a "$SUMMARY_LOG"
        ALL_TEST_NAMES+=("$test_name")
        ALL_TEST_RESULTS+=(1)
        ALL_TEST_DETAILS+=("Test failed with errors")
        
        # Look for the most recent log file for this test
        local log_pattern="*$(echo $test_name | tr '_' '*')*.log"
        local latest_log=$(find "$RESULTS_DIR" -type f -name "$log_pattern" | sort -r | head -1)
        if [ -n "$latest_log" ] && [ -f "$latest_log" ]; then
            echo "" | tee -a "$SUMMARY_LOG"
            echo "==== TEST EXECUTION LOG ====" | tee -a "$SUMMARY_LOG"
            cat "$latest_log" | tee -a "$SUMMARY_LOG"
            echo "==== END OF TEST EXECUTION LOG ====" | tee -a "$SUMMARY_LOG"
        fi
    fi
    
    echo "" | tee -a "$SUMMARY_LOG"
    ensure_server_not_running
    
    return $test_exit_code
}

# Function to run a specific test with configuration
run_test_with_config() {
    local test_script="$1"
    local config_file="$2"
    local test_name=$(basename "$test_script" .sh | sed 's/^test_//')
    local config_name=$(basename "$config_file" .json)
    local config_path=$(get_config_path "$config_file")
    
    print_header "Running Test: $test_name with config $config_name" | tee -a "$SUMMARY_LOG"
    
    # Execute the test script with config file
    print_command "$test_script $config_path" | tee -a "$SUMMARY_LOG"
    $test_script "$config_path"
    local test_exit_code=$?
    
    # Record test results
    if [ $test_exit_code -eq 0 ]; then
        print_result 0 "Test $test_name with $config_name completed successfully" | tee -a "$SUMMARY_LOG"
        ALL_TEST_NAMES+=("$test_name ($config_name)")
        ALL_TEST_RESULTS+=(0)
        ALL_TEST_DETAILS+=("Test completed without errors")
        ALL_TEST_SUBTESTS+=(1) # Default to 1 subtest if not specified
    else
        print_result 1 "Test $test_name with $config_name failed with exit code $test_exit_code" | tee -a "$SUMMARY_LOG"
        ALL_TEST_NAMES+=("$test_name ($config_name)")
        ALL_TEST_RESULTS+=(1)
        ALL_TEST_DETAILS+=("Test failed with errors")
        
        # Look for the most recent log file for this test
        local log_pattern="*$(echo $test_name | tr '_' '*')*.log"
        local latest_log=$(find "$RESULTS_DIR" -type f -name "$log_pattern" | sort -r | head -1)
        if [ -n "$latest_log" ] && [ -f "$latest_log" ]; then
            echo "" | tee -a "$SUMMARY_LOG"
            echo "==== TEST EXECUTION LOG ====" | tee -a "$SUMMARY_LOG"
            cat "$latest_log" | tee -a "$SUMMARY_LOG"
            echo "==== END OF TEST EXECUTION LOG ====" | tee -a "$SUMMARY_LOG"
        fi
    fi
    
    echo "" | tee -a "$SUMMARY_LOG"
    ensure_server_not_running
    
    return $test_exit_code
}

# Function to discover and run all tests
run_all_tests() {
    # First run compilation test as it's a prerequisite for other tests
    print_header "Running Compilation Test" | tee -a "$SUMMARY_LOG"
    run_test "$SCRIPT_DIR/test_compilation.sh"
    local compilation_exit_code=$?
    
    # Only proceed with other tests if compilation passes
    if [ $compilation_exit_code -ne 0 ]; then
        print_warning "Compilation failed - skipping remaining tests" | tee -a "$SUMMARY_LOG"
        return $compilation_exit_code
    fi
    
    # Run startup/shutdown tests with min configuration
    print_header "Running Startup/Shutdown Test (Min Config)" | tee -a "$SUMMARY_LOG"
    run_test_with_config "$SCRIPT_DIR/test_startup_shutdown.sh" "hydrogen_test_min.json"
    local min_exit_code=$?
    
    ensure_server_not_running
    
    # Run startup/shutdown tests with max configuration
    print_header "Running Startup/Shutdown Test (Max Config)" | tee -a "$SUMMARY_LOG"
    run_test_with_config "$SCRIPT_DIR/test_startup_shutdown.sh" "hydrogen_test_max.json"
    local max_exit_code=$?
    
    ensure_server_not_running
    
    # Find and run all other test scripts
    local test_scripts=$(find "$SCRIPT_DIR" -type f -name "test_*.sh" | grep -v "test_all.sh" | grep -v "test_startup_shutdown.sh" | grep -v "test_compilation.sh" | sort)
    
    local all_exit_codes=0
    for test_script in $test_scripts; do
        
        run_test "$test_script"
        local test_exit_code=$?
        
        if [ $test_exit_code -ne 0 ]; then
            all_exit_codes=1
        fi
        
        ensure_server_not_running
        echo "" | tee -a "$SUMMARY_LOG"
    done
    
    # Calculate overall result
    if [ $compilation_exit_code -eq 0 ] && [ $min_exit_code -eq 0 ] && [ $max_exit_code -eq 0 ] && [ $all_exit_codes -eq 0 ]; then
        return 0
    else
        return 1
    fi
}

# Function to run minimal configuration test only
run_min_configuration_test() {
    # First run compilation test as it's a prerequisite
    print_header "Running Compilation Test" | tee -a "$SUMMARY_LOG"
    run_test "$SCRIPT_DIR/test_compilation.sh"
    local compilation_exit_code=$?
    
    # Only proceed if compilation passes
    if [ $compilation_exit_code -ne 0 ]; then
        print_warning "Compilation failed - skipping startup/shutdown test" | tee -a "$SUMMARY_LOG"
        return $compilation_exit_code
    fi
    
    # Run startup/shutdown test with minimal configuration
    print_header "Running Startup/Shutdown Test (Min Config)" | tee -a "$SUMMARY_LOG"
    run_test_with_config "$SCRIPT_DIR/test_startup_shutdown.sh" "hydrogen_test_min.json"
    local min_exit_code=$?
    
    return $min_exit_code
}

# Function to run maximal configuration test only
run_max_configuration_test() {
    # First run compilation test as it's a prerequisite
    print_header "Running Compilation Test" | tee -a "$SUMMARY_LOG"
    run_test "$SCRIPT_DIR/test_compilation.sh"
    local compilation_exit_code=$?
    
    # Only proceed if compilation passes
    if [ $compilation_exit_code -ne 0 ]; then
        print_warning "Compilation failed - skipping startup/shutdown test" | tee -a "$SUMMARY_LOG"
        return $compilation_exit_code
    fi
    
    # Run startup/shutdown test with maximal configuration
    print_header "Running Startup/Shutdown Test (Max Config)" | tee -a "$SUMMARY_LOG"
    run_test_with_config "$SCRIPT_DIR/test_startup_shutdown.sh" "hydrogen_test_max.json"
    local max_exit_code=$?
    
    return $max_exit_code
}

# Function to run a specific named test
run_specific_test() {
    local test_name="$1"
    
    # Check if the test script exists
    if [ -f "$SCRIPT_DIR/test_${test_name}.sh" ]; then
        # Run compilation test first as it's usually a prerequisite
        print_header "Running Compilation Test" | tee -a "$SUMMARY_LOG"
        run_test "$SCRIPT_DIR/test_compilation.sh"
        local compilation_exit_code=$?
        
        # Only proceed if compilation passes
        if [ $compilation_exit_code -ne 0 ]; then
            print_warning "Compilation failed - skipping $test_name test" | tee -a "$SUMMARY_LOG"
            return $compilation_exit_code
        fi
        
        # Run the requested test
        print_header "Running Requested Test: $test_name" | tee -a "$SUMMARY_LOG"
        run_test "$SCRIPT_DIR/test_${test_name}.sh"
        return $?
    else
        print_result 1 "Test script not found: test_${test_name}.sh" | tee -a "$SUMMARY_LOG"
        return 1
    fi
}

# Function to print summary statistics
print_summary_statistics() {
    local exit_code=$1
    
    # Calculate pass/fail counts
    local pass_count=0
    local fail_count=0
    local total_count=${#ALL_TEST_NAMES[@]}
    
    for result in "${ALL_TEST_RESULTS[@]}"; do
        if [ $result -eq 0 ]; then
            ((pass_count++))
        else
            ((fail_count++))
        fi
    done
    
    # Calculate end time and runtime
    local end_time=$(date +%s)
    local total_runtime=$((end_time - START_TIME))
    local runtime_min=$((total_runtime / 60))
    local runtime_sec=$((total_runtime % 60))
    local runtime_formatted="${runtime_min}m ${runtime_sec}s"
    
    # Print individual test results
    echo -e "\n${BLUE}${BOLD}Individual Test Results:${NC}" | tee -a "$SUMMARY_LOG"
    for i in "${!ALL_TEST_NAMES[@]}"; do
        # Add subtest count to the details
        local subtest_info="(${ALL_TEST_SUBTESTS[$i]} of ${ALL_TEST_SUBTESTS[$i]} Passed)"
        local test_info="${ALL_TEST_NAMES[$i]} ${subtest_info}"
        print_test_item ${ALL_TEST_RESULTS[$i]} "${test_info}" "${ALL_TEST_DETAILS[$i]}" | tee -a "$SUMMARY_LOG"
    done
    
    # Calculate total subtests
    local total_subtests=0
    for count in "${ALL_TEST_SUBTESTS[@]}"; do
        total_subtests=$((total_subtests + count))
    done
    
    # Print comprehensive statistics 
    echo "" | tee -a "$SUMMARY_LOG"
    echo -e "${BLUE}${BOLD}Summary Statistics:${NC}" | tee -a "$SUMMARY_LOG"
    echo -e "Total tests: ${total_count}" | tee -a "$SUMMARY_LOG"
    echo -e "Passed: ${pass_count}" | tee -a "$SUMMARY_LOG"
    echo -e "Failed: ${fail_count}" | tee -a "$SUMMARY_LOG"
    echo -e "Test runtime: ${runtime_formatted}" | tee -a "$SUMMARY_LOG"
    echo "" | tee -a "$SUMMARY_LOG"
    
    if [ $exit_code -eq 0 ]; then
        echo -e "${GREEN}${BOLD}${PASS_ICON} OVERALL RESULT: ALL TESTS PASSED" | tee -a "$SUMMARY_LOG"
    else
        echo -e "${RED}${BOLD}${FAIL_ICON} OVERALL RESULT: ONE OR MORE TESTS FAILED${NC}" | tee -a "$SUMMARY_LOG"
    fi
    echo -e "${BLUE}───────────────────────────────────────────────────────────────────────────────${NC}" | tee -a "$SUMMARY_LOG"
    
    # Tips for additional diagnostics
    echo "" | tee -a "$SUMMARY_LOG"
    echo "For more detailed analysis:" | tee -a "$SUMMARY_LOG"
    echo "  • Thread analysis:     $(convert_to_relative_path "$SCRIPT_DIR/support_analyze_stuck_threads.sh") <pid>" | tee -a "$SUMMARY_LOG"
    echo "  • Resource monitoring: $(convert_to_relative_path "$SCRIPT_DIR/support_monitor_resources.sh") <pid> [seconds]" | tee -a "$SUMMARY_LOG"
    echo "" | tee -a "$SUMMARY_LOG"
}

# Parse command line argument
TEST_TYPE=${1:-"all"}  # Default to "all" if not specified

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
        run_all_tests
        EXIT_CODE=$?
        ;;
    *)
        # Check if it's a specific test name
        if [ -f "$SCRIPT_DIR/test_${TEST_TYPE}.sh" ]; then
            print_info "Running specific test: $TEST_TYPE" | tee -a "$SUMMARY_LOG"
            run_specific_test "$TEST_TYPE"
            EXIT_CODE=$?
        else
            print_warning "Invalid test type or test name: $TEST_TYPE" | tee -a "$SUMMARY_LOG"
            echo "Usage: $0 [test_name|min|max|all]" | tee -a "$SUMMARY_LOG"
            echo "  test_name: Run a specific test (e.g., compilation, startup_shutdown)" | tee -a "$SUMMARY_LOG"
            echo "  min: Run with minimal configuration only" | tee -a "$SUMMARY_LOG"
            echo "  max: Run with maximal configuration only" | tee -a "$SUMMARY_LOG"
            echo "  all: Run all tests (default)" | tee -a "$SUMMARY_LOG"
            EXIT_CODE=1
        fi
        ;;
esac

# Print overall summary
print_header "Test Summary" | tee -a "$SUMMARY_LOG"
echo "Completed at: $(date)" | tee -a "$SUMMARY_LOG"
echo "Summary log: $(convert_to_relative_path "$SUMMARY_LOG")" | tee -a "$SUMMARY_LOG"
echo "" | tee -a "$SUMMARY_LOG"

# Print summary statistics
print_summary_statistics $EXIT_CODE

# Clean up any leftover log files in the test directory (not in results)
rm -f "$SCRIPT_DIR"/*.log

exit $EXIT_CODE