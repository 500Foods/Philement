#!/bin/bash
#
# Test Template for Hydrogen
#
# This template demonstrates how to create standardized tests for the Hydrogen server.
# It includes examples of common test patterns and best practices derived from existing tests.
#
# IMPORTANT: This template is designed to pass shellcheck without warnings.
# Before submitting new tests, run: shellcheck -x -f gcc your_test.sh
#
# Usage: ./test_template.sh [--config min|max] [--skip-cleanup]
#
# Options:
#   --config min|max    Use minimal or maximal configuration (default: min)
#   --skip-cleanup     Don't remove temporary files after test
#
# Common Test Patterns Demonstrated:
# 1. Basic startup/shutdown validation
# 2. API endpoint testing with subtests
# 3. Signal handling
# 4. Resource monitoringi
# 5. Configuration validation
# 6. Subtest result tracking and export
#

# Script metadata (required for modern test pattern)
NAME_SCRIPT="Template Test"
VERS_SCRIPT="3.0.0"

# VERSION HISTORY
# 3.0.0 - 2025-06-17 - Major refactoring: fixed all shellcheck warnings, added subtest support, improved modularity
# 2.0.0 - Previous version - Basic template functionality
# 1.0.0 - Original version - Initial template

# Display script name and version
echo "=== $NAME_SCRIPT v$VERS_SCRIPT ==="

# Get the directory where this script is located
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

# Include the common test utilities
source "$SCRIPT_DIR/support_utils.sh"

# ====================================================================
# STEP 1: Set up the test environment
# ====================================================================

# Set up test environment with descriptive name (creates log file and timestamp)
# Choose a name that clearly describes what aspect is being tested
TEST_NAME="Template Test"
RESULT_LOG=$(setup_test_environment "$TEST_NAME")

# ====================================================================
# STEP 2: Parse command line arguments and configure test parameters
# ====================================================================

# Example of handling command line arguments
CONFIG_TYPE="min"
SKIP_CLEANUP=0

while [[ $# -gt 0 ]]; do
    case $1 in
        --config)
            CONFIG_TYPE="$2"
            shift 2
            ;;
        --skip-cleanup)
            SKIP_CLEANUP=1
            shift
            ;;
        *)
            echo "Unknown option: $1"
            exit 1
            ;;
    esac
done

# Select configuration file based on test needs
# Common patterns:
# 1. hydrogen_test_min.json - Minimal configuration for basic tests
# 2. hydrogen_test_max.json - Full feature testing
# 3. Custom config for specific test requirements
case $CONFIG_TYPE in
    "min")
        CONFIG_FILE=$(get_config_path "hydrogen_test_min.json")
        ;;
    "max")
        CONFIG_FILE=$(get_config_path "hydrogen_test_max.json")
        ;;
    *)
        echo "Invalid configuration type: $CONFIG_TYPE"
        exit 1
        ;;
esac

# Extract configuration values (if needed for test)
WEB_SERVER_PORT=$(extract_web_server_port "$CONFIG_FILE")
# Note: WEBSOCKET_PORT removed to fix shellcheck SC2034 (unused variable)

# ====================================================================
# STEP 3: Find appropriate Hydrogen binary
# ====================================================================

# Automatically find the best available Hydrogen binary
# Priority: release > debug > standard > valgrind
HYDROGEN_BIN=$(find_hydrogen_binary)

if [ -z "$HYDROGEN_BIN" ]; then
    print_error "Could not find Hydrogen binary" | tee -a "$RESULT_LOG"
    exit 1
fi

# ====================================================================
# STEP 4: Initialize subtest tracking
# ====================================================================

# Define total number of subtests for this template
TOTAL_SUBTESTS=4  # Startup, API, Resource, Signal
PASS_COUNT=0

# ====================================================================
# STEP 5: Define Test Functions
# ====================================================================

# Example: Basic API endpoint test function
test_api_endpoint() {
    local endpoint="$1"
    local expected="$2"
    local method="${3:-GET}"
    local data="${4:-}"
    
    print_header "Testing $method $endpoint" | tee -a "$RESULT_LOG"
    
    local curl_cmd="curl -s -X $method --max-time 5"
    if [ "$method" = "POST" ]; then
        curl_cmd="$curl_cmd -H 'Content-Type: application/json'"
        [ -n "$data" ] && curl_cmd="$curl_cmd -d '$data'"
    fi
    curl_cmd="$curl_cmd http://localhost:${WEB_SERVER_PORT}${endpoint}"
    
    run_curl_request "$endpoint" "$curl_cmd" "$expected" "$RESULT_LOG"
    return $?
}

# Example: Signal handling test function
test_signal_handling() {
    local signal="$1"
    local timeout="$2"
    
    print_header "Testing $signal Signal Handling" | tee -a "$RESULT_LOG"
    
    # Send signal to process
    kill -"$signal" "$SERVER_PID"
    
    # Wait for process to handle signal
    local end_time=$((SECONDS + timeout))
    while kill -0 "$SERVER_PID" 2>/dev/null && [ $SECONDS -lt "$end_time" ]; do
        sleep 1
    done
    
    # Check result
    if kill -0 "$SERVER_PID" 2>/dev/null; then
        print_result 1 "Server failed to handle $signal within ${timeout}s" | tee -a "$RESULT_LOG"
        return 1
    fi
    
    print_result 0 "Server handled $signal successfully" | tee -a "$RESULT_LOG"
    return 0
}

# Example: Resource monitoring test function
test_resource_usage() {
    local duration="$1"
    
    print_header "Monitoring Resource Usage" | tee -a "$RESULT_LOG"
    
    # Start resource monitoring in background
    "$SCRIPT_DIR/support_monitor_resources.sh" "$SERVER_PID" "$duration" &
    local monitor_pid=$!
    
    # Wait for monitoring to complete
    wait $monitor_pid
    
    # Analyze results (example thresholds)
    local mem_usage
    mem_usage=$(grep "Memory:" "diagnostics/resources_${SERVER_PID}.log" | tail -n1 | awk '{print $2}')
    if [ "$mem_usage" -gt 100000 ]; then
        print_result 1 "Memory usage too high: ${mem_usage}KB" | tee -a "$RESULT_LOG"
        return 1
    fi
    
    print_result 0 "Resource usage within limits" | tee -a "$RESULT_LOG"
    return 0
}

# Function to safely kill a process with PID validation
# shellcheck disable=SC2317  # Function is called indirectly via cleanup trap
safe_kill_process() {
    local pid="$1"
    local process_name="$2"
    
    # Validate PID is not empty and not zero
    if [ -z "$pid" ] || [ "$pid" = "0" ]; then
        return 1
    fi
    
    if ps -p "$pid" > /dev/null 2>&1; then
        echo "Cleaning up $process_name (PID $pid)..."
        kill -SIGINT "$pid" 2>/dev/null || kill -9 "$pid" 2>/dev/null
        return 0
    fi
    return 1
}

# Handle cleanup on script termination
# shellcheck disable=SC2317  # Function is called indirectly via trap
cleanup() {
    # Kill any hydrogen processes started by this script
    safe_kill_process "$SERVER_PID" "hydrogen server"
    
    # Save test log if we've started writing it
    if [ -n "$RESULT_LOG" ] && [ -f "$RESULT_LOG" ]; then
        echo "Saving test results to $(convert_to_relative_path "$RESULT_LOG")"
    fi
    
    exit 0
}

# Set up cleanup trap
trap cleanup EXIT INT TERM

# ====================================================================
# STEP 6: Implement Main Test Logic
# ====================================================================

run_tests() {
    local test_result=0
    
    # === TEST CASE 1: Server Startup ===
    print_header "Test Case 1: Server Startup" | tee -a "$RESULT_LOG"
    
    # Start Hydrogen server
    SERVER_PID=$(start_hydrogen_server "$HYDROGEN_BIN" "$CONFIG_FILE" "$SCRIPT_DIR/hydrogen_test.log")
    
    if [ -z "$SERVER_PID" ]; then
        print_result 1 "Server failed to start" | tee -a "$RESULT_LOG"
        return 1
    fi
    print_result 0 "Server started successfully (PID: $SERVER_PID)" | tee -a "$RESULT_LOG"
    ((PASS_COUNT++))
    
    # === TEST CASE 2: API Testing (if web server enabled) ===
    if [ -n "$WEB_SERVER_PORT" ]; then
        print_header "Test Case 2: API Testing" | tee -a "$RESULT_LOG"
        
        # Example: Test health endpoint
        if test_api_endpoint "/api/system/health" "alive"; then
            ((PASS_COUNT++))
        else
            test_result=1
        fi
        
        # Example: Test POST endpoint
        if test_api_endpoint "/api/system/test" "success" "POST" '{"test":"data"}'; then
            # This would be a separate subtest if implemented
            print_result 0 "POST endpoint test passed" | tee -a "$RESULT_LOG"
        else
            test_result=1
        fi
    else
        # If no web server, still count as passed subtest
        print_result 0 "Web server disabled, skipping API tests" | tee -a "$RESULT_LOG"
        ((PASS_COUNT++))
    fi
    
    # === TEST CASE 3: Resource Monitoring ===
    print_header "Test Case 3: Resource Monitoring" | tee -a "$RESULT_LOG"
    if test_resource_usage 10; then
        ((PASS_COUNT++))
    else
        test_result=1
    fi
    
    # === TEST CASE 4: Signal Handling ===
    print_header "Test Case 4: Signal Handling" | tee -a "$RESULT_LOG"
    if test_signal_handling SIGTERM 10; then
        ((PASS_COUNT++))
    else
        test_result=1
    fi
    
    return $test_result
}

# ====================================================================
# STEP 7: Execute Tests and Report Results
# ====================================================================

# Run all test cases
run_tests
TEST_RESULT=$?

# Clean up unless --skip-cleanup was specified
if [ $SKIP_CLEANUP -eq 0 ]; then
    rm -f response_*.json
    rm -f test_*.tmp
fi

# Collect test results for reporting
declare -a TEST_NAMES=("Server Startup" "API Testing" "Resource Usage" "Signal Handling")
declare -a TEST_RESULTS
declare -a TEST_DETAILS

# Populate results (0=pass, 1=fail)
for name in "${TEST_NAMES[@]}"; do
    if [ $TEST_RESULT -eq 0 ]; then
        TEST_RESULTS+=(0)
    else
        TEST_RESULTS+=(1)
    fi
    TEST_DETAILS+=("${name} validation")
done

# Calculate statistics
TOTAL_PASS=0
TOTAL_FAIL=0
for result in "${TEST_RESULTS[@]}"; do
    if [ "$result" -eq 0 ]; then
        ((TOTAL_PASS++))
    else
        ((TOTAL_FAIL++))
    fi
done

# Print individual test results
echo -e "
${BLUE}${BOLD}Individual Test Results:${NC}" | tee -a "$RESULT_LOG"
for i in "${!TEST_NAMES[@]}"; do
    print_test_item "${TEST_RESULTS[$i]}" "${TEST_NAMES[$i]}" "${TEST_DETAILS[$i]}" | tee -a "$RESULT_LOG"
done

# Print summary statistics
print_test_summary $((TOTAL_PASS + TOTAL_FAIL)) $TOTAL_PASS $TOTAL_FAIL | tee -a "$RESULT_LOG"

# Get test name from script name for subtest export
SCRIPT_TEST_NAME=$(basename "$0" .sh)
SCRIPT_TEST_NAME="${SCRIPT_TEST_NAME#test_}"

# Export subtest results for test_all.sh to pick up
export_subtest_results "$SCRIPT_TEST_NAME" $TOTAL_SUBTESTS $PASS_COUNT

# Log subtest results
print_info "Template Test: $PASS_COUNT of $TOTAL_SUBTESTS subtests passed" | tee -a "$RESULT_LOG"

# End test with final result
end_test $TEST_RESULT "$TEST_NAME" | tee -a "$RESULT_LOG"

exit $TEST_RESULT
