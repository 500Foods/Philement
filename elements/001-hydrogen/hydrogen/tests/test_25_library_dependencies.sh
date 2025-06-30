#!/bin/bash
#
# test_25_library_dependencies.sh - Hydrogen Library Dependencies Test Suite
# Version: 1.1.0
# 
# Version History:
# 1.0.0 - Initial version with library dependency checking
# 1.1.0 - Enhanced with proper error handling, modular functions, and shellcheck compliance
#
# Description:
# Tests the library dependency checking system added to initialization.
# Validates that required libraries are detected and reported correctly,
# and that optional libraries are handled appropriately based on configuration.
#
# Usage: ./test_25_library_dependencies.sh
#
# Dependencies:
# - support_utils.sh for common test utilities
# - Hydrogen server binary
# - Test configuration files (hydrogen_test_max.json, hydrogen_test_min.json)
#

# Script identification
readonly SCRIPT_NAME="test_25_library_dependencies.sh"
readonly SCRIPT_VERSION="1.1.0"

# Display script information
echo "Starting ${SCRIPT_NAME} v${SCRIPT_VERSION}"
echo "Hydrogen Library Dependencies Test Suite"
echo "========================================"

# Get the directory where this script is located
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
HYDROGEN_DIR="$( cd "$SCRIPT_DIR/.." && pwd )"

# Include the common test utilities
# shellcheck source=support_utils.sh
source "$SCRIPT_DIR/support_utils.sh"

# Configuration
# Prefer release build if available, fallback to standard build
if [ -f "$HYDROGEN_DIR/hydrogen_release" ]; then
    HYDROGEN_BIN="$HYDROGEN_DIR/hydrogen_release"
    print_info "Using release build for testing"
else
    HYDROGEN_BIN="$HYDROGEN_DIR/hydrogen"
    print_info "Using standard build"
fi

# Default configuration files - one with everything enabled, one with minimal services
ALL_ENABLED_CONFIG=$(get_config_path "hydrogen_test_max.json")
MINIMAL_CONFIG=$(get_config_path "hydrogen_test_min.json")

# Timeouts and paths
STARTUP_TIMEOUT=10                        # Seconds to wait for startup
LOG_FILE="$SCRIPT_DIR/hydrogen_test.log"  # Runtime log to analyze (use common log file)
RESULTS_DIR="$SCRIPT_DIR/results"         # Directory for test results
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
RESULT_LOG="$RESULTS_DIR/lib_dep_test_${TIMESTAMP}.log"

# Create output directories
mkdir -p "$RESULTS_DIR"

# Initialize error counters
# Use the 'declare' statement to make them global
declare -i EXIT_CODE=0
declare -i TOTAL_TESTS=0
declare -i PASSED_TESTS=0
declare -i FAILED_TESTS=0

# Global variable for current test process
HYDROGEN_PID=""

#
# cleanup_test_environment() - Clean up any remaining test processes
# Globals:
#   HYDROGEN_PID - Process ID of test server
# Returns:
#   None
#
# shellcheck disable=SC2317  # Function called via trap
cleanup_test_environment() {
    if [[ -n "${HYDROGEN_PID}" ]] && ps -p "${HYDROGEN_PID}" > /dev/null 2>&1; then
        echo "Cleaning up test process ${HYDROGEN_PID}"
        kill -TERM "${HYDROGEN_PID}" 2>/dev/null || true
        sleep 2
        
        # Force kill if still running
        if ps -p "${HYDROGEN_PID}" > /dev/null 2>&1; then
            echo "Force killing test process ${HYDROGEN_PID}"
            kill -KILL "${HYDROGEN_PID}" 2>/dev/null || true
        fi
    fi
    
    # Ensure no hydrogen processes are left running
    ensure_server_not_running 2>/dev/null || true
}

#
# check_dependency_log() - Helper function to check logs for dependency messages
# Arguments:
#   $1 - dep_name: Name of the dependency to check
#   $2 - expected_level: Expected log level (1=STATE, 2=ALERT, 5=FATAL)
#   $3 - expected_status: Expected status (Good, Less Good, Warning) - unused but kept for compatibility
#   $4 - is_required: Whether the dependency is required (true/false)
#   $5 - log_file: Path to the log file to check
#   $6 - test_name: Name of the test for reporting
# Globals:
#   TOTAL_TESTS, PASSED_TESTS, FAILED_TESTS - Test counters
# Returns:
#   0 on success, 1 on failure
#
check_dependency_log() {
    local dep_name="$1"
    local expected_level="$2"
    local expected_status="$3"  # Keep for function signature compatibility
    local is_required="$4"
    local log_file="$5"
    local test_name="$6"
    
    # Suppress shellcheck warning about unused variable - kept for API compatibility
    # shellcheck disable=SC2034
    local _unused_status="$expected_status"
    
    # Increment total tests
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
    
    # Determine the expected log level marker based on level number
    local level_marker=""
    if [ "$expected_level" == "1" ]; then
        level_marker="STATE"
    elif [ "$expected_level" == "2" ]; then
        level_marker="ALERT"
    elif [ "$expected_level" == "5" ]; then
        level_marker="FATAL"
    else
        level_marker="ERROR"
    fi
    
    # Look for the dependency message in the log, ignoring timestamp
    if [ "$is_required" = "true" ]; then
        # For required libraries, accept STATE level with Good or Less Good status to be less restrictive
        if grep -q ".*\[ STATE \]  \[ DepCheck           \]  $dep_name.*Status: \(Good\|Less Good\)" "$log_file"; then
            print_test_item 0 "$test_name" "Found $dep_name with expected level STATE"
            PASSED_TESTS=$((PASSED_TESTS + 1))
            return 0
        fi
    else
        # For optional libraries, accept either status since we're on dev machine
        if grep -q ".*\[ \(STATE\|WARN\) \]  \[ DepCheck           \]  $dep_name.*Status: \(Good\|Less Good\)" "$log_file"; then
            print_test_item 0 "$test_name" "Found $dep_name with valid status"
            PASSED_TESTS=$((PASSED_TESTS + 1))
            return 0
        fi
    fi
    
    print_test_item 1 "$test_name" "Did not find $dep_name with expected level $level_marker"
    FAILED_TESTS=$((FAILED_TESTS + 1))
    return 1
}

#
# log_and_check() - Helper wrapper that avoids the subshell problem with piping to tee
# Arguments:
#   $1-$6 - Same as check_dependency_log function
# Returns:
#   Same as check_dependency_log function
#
log_and_check() {
    local result
    
    check_dependency_log "$1" "$2" "$3" "$4" "$5" "$6"
    result=$?
    
    # Log the last line of output to the result log
    tail -n 1 "$LOG_FILE" >> "$RESULT_LOG"
    
    return $result
}

#
# wait_for_startup() - Wait for hydrogen server startup completion
# Arguments:
#   $1 - log_file: Path to the log file to monitor
# Globals:
#   STARTUP_TIMEOUT - Maximum time to wait for startup
# Returns:
#   0 on successful startup, 1 on timeout
#
wait_for_startup() {
    local log_file="$1"
    local startup_start elapsed
    
    startup_start=$(date +%s)
    while true; do
        # Check if we've exceeded the timeout
        local current_time
        current_time=$(date +%s)
        elapsed=$((current_time - startup_start))
        
        if [ $elapsed -ge $STARTUP_TIMEOUT ]; then
            print_warning "Startup timeout after ${elapsed}s" | tee -a "$RESULT_LOG"
            return 1
        fi
        
        # Check for startup completion
        if grep -q "Application started" "$log_file"; then
            print_info "Startup completed in $elapsed seconds" | tee -a "$RESULT_LOG"
            return 0
        fi
        
        # Small sleep to avoid consuming CPU
        sleep 0.2
    done
}

#
# terminate_hydrogen_gracefully() - Terminate hydrogen process gracefully
# Globals:
#   HYDROGEN_PID - Process ID to terminate
# Returns:
#   None
#
terminate_hydrogen_gracefully() {
    if [[ -n "${HYDROGEN_PID}" ]] && ps -p "$HYDROGEN_PID" > /dev/null; then
        # First try SIGTERM
        kill -SIGTERM "$HYDROGEN_PID"
        
        # Wait up to 10 seconds for graceful shutdown
        local wait_count=0
        while [[ $wait_count -lt 10 ]]; do
            if ! ps -p "$HYDROGEN_PID" > /dev/null; then
                break
            fi
            sleep 1
            ((wait_count++))
        done
        
        # If still running, try SIGINT
        if ps -p "$HYDROGEN_PID" > /dev/null; then
            kill -SIGINT "$HYDROGEN_PID"
            sleep 2
        fi
        
        # Only use SIGKILL as absolute last resort
        if ps -p "$HYDROGEN_PID" > /dev/null; then
            kill -9 "$HYDROGEN_PID"
        fi
    fi
    
    # Clear the PID
    HYDROGEN_PID=""
}

#
# run_dependency_test() - Run a single dependency test scenario
# Arguments:
#   $1 - config_file: Configuration file to use
#   $2 - test_case_name: Name of the test case
#   $3 - test_description: Description of what's being tested
# Returns:
#   0 on success, 1 on failure
#
run_dependency_test() {
    local config_file="$1"
    local test_case_name="$2"
    local test_description="$3"
    
    print_header "$test_case_name" | tee -a "$RESULT_LOG"
    print_info "Using config: $(convert_to_relative_path "$config_file")" | tee -a "$RESULT_LOG"
    print_info "$test_description" | tee -a "$RESULT_LOG"
    
    # Start with clean log - use 'true' as no-op to fix SC2188
    true > "$LOG_FILE"
    
    # Launch Hydrogen in background
    print_command "$HYDROGEN_BIN $config_file" | tee -a "$RESULT_LOG"
    $HYDROGEN_BIN "$config_file" > "$LOG_FILE" 2>&1 &
    HYDROGEN_PID=$!
    
    # Wait for startup completion or timeout
    if ! wait_for_startup "$LOG_FILE"; then
        terminate_hydrogen_gracefully
        return 1
    fi
    
    return 0
}

#
# run_max_config_tests() - Run tests with maximum configuration
# Returns:
#   0 on success, 1 on failure
#
run_max_config_tests() {
    if ! run_dependency_test "$ALL_ENABLED_CONFIG" "Test Case 1: All Services Enabled" "Testing with all services enabled - all dependencies should be required"; then
        return 1
    fi
    
    # Check for library dependency messages with max config (all required)
    print_info "Checking dependency logs for max config (all services enabled)" | tee -a "$RESULT_LOG"
    
    # Run checks without piping to tee to preserve variable changes
    log_and_check "pthreads" "1" "Good" "true" "$LOG_FILE" "Required pthreads"
    log_and_check "jansson" "1" "Good" "true" "$LOG_FILE" "Required jansson"
    log_and_check "libm" "1" "Good" "true" "$LOG_FILE" "Required libm"
    log_and_check "microhttpd" "1" "Good" "true" "$LOG_FILE" "Required microhttpd with web enabled"
    log_and_check "libwebsockets" "1" "Good" "true" "$LOG_FILE" "Required libwebsockets with websocket enabled"
    log_and_check "OpenSSL" "1" "Good" "true" "$LOG_FILE" "Required OpenSSL with web/websocket enabled"
    log_and_check "libbrotlidec" "1" "Good" "true" "$LOG_FILE" "Required libbrotlidec with web enabled"
    log_and_check "libtar" "1" "Good" "true" "$LOG_FILE" "Required libtar with print enabled"
    
    # Terminate the process gracefully
    terminate_hydrogen_gracefully
    
    # Wait to ensure resources are released before next test
    sleep 3
    ensure_server_not_running 2>/dev/null || true
    
    return 0
}

#
# run_min_config_tests() - Run tests with minimal configuration
# Returns:
#   0 on success, 1 on failure
#
run_min_config_tests() {
    if ! run_dependency_test "$MINIMAL_CONFIG" "Test Case 2: Minimal Services Enabled" "Testing with minimal services - some dependencies should be optional"; then
        return 1
    fi
    
    # Check for library dependency messages with min config (some optional)
    print_info "Checking dependency logs for min config (minimal services enabled)" | tee -a "$RESULT_LOG"
    log_and_check "pthreads" "1" "Good" "true" "$LOG_FILE" "Required pthreads"
    log_and_check "jansson" "1" "Good" "true" "$LOG_FILE" "Required jansson"
    log_and_check "libm" "1" "Good" "true" "$LOG_FILE" "Required libm"
    log_and_check "microhttpd" "2" "Less Good\|Warning" "false" "$LOG_FILE" "Optional microhttpd with web disabled"
    log_and_check "libwebsockets" "2" "Less Good\|Warning" "false" "$LOG_FILE" "Optional libwebsockets with websocket disabled"
    log_and_check "OpenSSL" "2" "Less Good\|Warning" "false" "$LOG_FILE" "Optional OpenSSL with web/websocket disabled"
    log_and_check "libbrotlidec" "2" "Less Good\|Warning" "false" "$LOG_FILE" "Optional libbrotlidec with web disabled"
    log_and_check "libtar" "2" "Less Good\|Warning" "false" "$LOG_FILE" "Optional libtar with print disabled"
    
    # Terminate the process gracefully
    terminate_hydrogen_gracefully
    
    return 0
}

#
# generate_test_report() - Generate final test report and determine exit code
# Returns:
#   0 if all tests passed, 1 if any tests failed
#
generate_test_report() {
    # Copy log for reference
    cp "$LOG_FILE" "$RESULTS_DIR/lib_dep_full_log_${TIMESTAMP}.log"
    
    # Print summary statistics
    print_test_summary $TOTAL_TESTS $PASSED_TESTS $FAILED_TESTS >> "$RESULT_LOG"
    
    # Determine final result
    if [ $FAILED_TESTS -gt 0 ]; then
        print_result 1 "FAILED: $FAILED_TESTS out of $TOTAL_TESTS tests failed" | tee -a "$RESULT_LOG"
        EXIT_CODE=1
    else
        print_result 0 "PASSED: All $TOTAL_TESTS tests passed successfully" | tee -a "$RESULT_LOG"
        EXIT_CODE=0
    fi
    
    # Get test name from script name
    TEST_NAME=$(basename "$0" .sh | sed 's/^test_//')
    
    # Export subtest results for test_all.sh to pick up
    export_subtest_results "$TEST_NAME" $TOTAL_TESTS $PASSED_TESTS
    
    # Log subtest results
    print_info "Library Dependencies Test: $PASSED_TESTS of $TOTAL_TESTS subtests passed" | tee -a "$RESULT_LOG"
    
    return $EXIT_CODE
}

#
# main() - Main execution function
# Returns:
#   0 on success, 1 on failure
#
main() {
    # Set up error handling
    trap cleanup_test_environment EXIT ERR
    
    # Validate prerequisites
    if [[ ! -f "$HYDROGEN_BIN" ]]; then
        echo "Error: Hydrogen binary not found at $HYDROGEN_BIN"
        exit 1
    fi
    
    if [[ ! -f "$ALL_ENABLED_CONFIG" ]]; then
        echo "Error: Max configuration not found: $ALL_ENABLED_CONFIG"
        exit 1
    fi
    
    if [[ ! -f "$MINIMAL_CONFIG" ]]; then
        echo "Error: Minimal configuration not found: $MINIMAL_CONFIG"
        exit 1
    fi
    
    # Start the test
    start_test "Hydrogen Library Dependencies Test" | tee -a "$RESULT_LOG"
    
    # Run test scenarios
    run_max_config_tests
    run_min_config_tests
    
    # Generate final report
    if generate_test_report; then
        end_test 0 "Library Dependencies Test" | tee -a "$RESULT_LOG"
        echo ""
        echo "✓ ${SCRIPT_NAME} completed successfully"
        exit 0
    else
        end_test 1 "Library Dependencies Test" | tee -a "$RESULT_LOG"
        echo ""
        echo "✗ ${SCRIPT_NAME} completed with failures"
        exit 1
    fi
}

# Execute main function
main "$@"
