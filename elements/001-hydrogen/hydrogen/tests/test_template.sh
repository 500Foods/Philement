#!/bin/bash
#
# Test Template for Hydrogen
# This template demonstrates how to create a new test using standardized utilities.
# Copy this file and replace the placeholders to create a new test.
#
# Usage: ./test_template.sh [optional_parameters]
#

# Get the directory where this script is located
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

# Include the common test utilities
source "$SCRIPT_DIR/support_utils.sh"

# ====================================================================
# STEP 1: Set up the test environment
# ====================================================================

# Set up test environment with descriptive name (creates log file and timestamp)
TEST_NAME="Template Test"
RESULT_LOG=$(setup_test_environment "$TEST_NAME")

# ====================================================================
# STEP 2: Configure test parameters
# ====================================================================

# Get configuration file path (use an existing config or create a new one)
CONFIG_FILE=$(get_config_path "hydrogen_test_min.json")

# Extract web server port from configuration (if needed for HTTP requests)
WEB_SERVER_PORT=$(extract_web_server_port "$CONFIG_FILE")

# ====================================================================
# STEP 3: Find appropriate Hydrogen binary
# ====================================================================

# Automatically find the best available Hydrogen binary (release preferred)
HYDROGEN_BIN=$(find_hydrogen_binary)

# ====================================================================
# STEP 4: Test Implementation
# ====================================================================

# Initialize test status (will be set based on test results)
TEST_RESULT=0

# Define your test case as a function for better organization
run_tests() {
    print_header "Running Test Cases" | tee -a "$RESULT_LOG"
    
    # === TEST CASE 1: Start the server ===
    print_header "Test Case 1: Server Startup" | tee -a "$RESULT_LOG"
    
    # Start the Hydrogen server
    SERVER_PID=$(start_hydrogen_server "$HYDROGEN_BIN" "$CONFIG_FILE" "$SCRIPT_DIR/hydrogen_test.log")
    
    # Check server started successfully
    if [ -z "$SERVER_PID" ]; then
        print_result 1 "Server failed to start" | tee -a "$RESULT_LOG"
        return 1
    fi
    
    print_result 0 "Server started successfully" | tee -a "$RESULT_LOG"
    
    # === TEST CASE 2: Make HTTP requests (if applicable) ===
    print_header "Test Case 2: API Requests" | tee -a "$RESULT_LOG"
    
    # Example: Test a health endpoint
    if [ -n "$WEB_SERVER_PORT" ]; then
        run_curl_request "health" "curl -s --max-time 5 http://localhost:${WEB_SERVER_PORT}/api/system/health" "alive" "$RESULT_LOG"
        local api_result=$?
        
        if [ $api_result -ne 0 ]; then
            print_result 1 "API test failed" | tee -a "$RESULT_LOG"
            TEST_RESULT=1
        else
            print_result 0 "API test succeeded" | tee -a "$RESULT_LOG"
        fi
    else
        print_warning "Skipping API tests (no web server port configured)" | tee -a "$RESULT_LOG"
    fi
    
    # === TEST CASE 3: Stop the server ===
    print_header "Test Case 3: Server Shutdown" | tee -a "$RESULT_LOG"
    
    # Stop the server with a 10-second timeout
    stop_hydrogen_server $SERVER_PID 10
    local stop_result=$?
    
    if [ $stop_result -ne 0 ]; then
        print_result 1 "Server failed to stop cleanly" | tee -a "$RESULT_LOG"
        TEST_RESULT=1
    else
        print_result 0 "Server stopped successfully" | tee -a "$RESULT_LOG"
    fi
    
    return $TEST_RESULT
}

# ====================================================================
# STEP 5: Execute Tests
# ====================================================================

# Run all the test cases
run_tests
TEST_RESULT=$?

# Clean up any temporary files
rm -f response_*.json

# ====================================================================
# STEP 6: Report Results
# ====================================================================

# Collect test results
declare -a TEST_NAMES
declare -a TEST_RESULTS
declare -a TEST_DETAILS

TEST_NAMES+=("Server Startup")
TEST_RESULTS+=($([ $TEST_RESULT -eq 0 ] && echo 0 || echo 1))
TEST_DETAILS+=("Server initialization test")

TEST_NAMES+=("API Endpoints")
TEST_RESULTS+=($([ $TEST_RESULT -eq 0 ] && echo 0 || echo 1))
TEST_DETAILS+=("API endpoint response test")

TEST_NAMES+=("Server Shutdown")
TEST_RESULTS+=($([ $TEST_RESULT -eq 0 ] && echo 0 || echo 1))
TEST_DETAILS+=("Server graceful shutdown test")

# Calculate success/failure statistics
PASS_COUNT=0
FAIL_COUNT=0
for result in "${TEST_RESULTS[@]}"; do
    if [ $result -eq 0 ]; then
        ((PASS_COUNT++))
    else
        ((FAIL_COUNT++))
    fi
done

# Print individual test results
echo -e "\n${BLUE}${BOLD}Individual Test Results:${NC}" | tee -a "$RESULT_LOG"
for i in "${!TEST_NAMES[@]}"; do
    print_test_item ${TEST_RESULTS[$i]} "${TEST_NAMES[$i]}" "${TEST_DETAILS[$i]}" | tee -a "$RESULT_LOG"
done

# Print summary statistics
print_test_summary $((PASS_COUNT + FAIL_COUNT)) $PASS_COUNT $FAIL_COUNT | tee -a "$RESULT_LOG"

# End the test with final result
end_test $TEST_RESULT "$TEST_NAME" | tee -a "$RESULT_LOG"

# Optionally generate HTML report
# generate_html_report "$RESULT_LOG"

exit $TEST_RESULT