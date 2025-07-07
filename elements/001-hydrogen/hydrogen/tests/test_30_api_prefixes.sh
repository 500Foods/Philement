#!/bin/bash

# Test: API Prefix
# Tests the API endpoints with different API prefix configurations using immediate restart approach:
# - Default "/api" prefix using standard hydrogen_test_api.json
# - Custom "/myapi" prefix using hydrogen_test_api_prefix.json
# - Uses immediate restart without waiting for TIME_WAIT (SO_REUSEADDR enabled)

# CHANGELOG
# 5.0.1 - 2025-07-06 - Added missing shellcheck justifications
# 5.0.0 - 2025-07-02 - Migrated to use lib/ scripts, following established test patterns
# 4.0.0 - 2025-06-30 - Updated to use immediate restart approach without TIME_WAIT waiting
# 3.0.0 - 2025-06-30 - Refactored to use robust server management functions from test_55
# 2.0.0 - 2025-06-17 - Major refactoring: fixed all shellcheck warnings, improved modularity, enhanced comments

# Test Configuration
TEST_NAME="API Prefix"
SCRIPT_VERSION="5.0.1"

# Get the directory where this script is located
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
HYDROGEN_DIR="$( cd "$SCRIPT_DIR/.." && pwd )"

# Source the library scripts with guard clauses
if [[ -z "$TABLES_SH_GUARD" ]] || ! command -v tables_render_from_json >/dev/null 2>&1; then
    # shellcheck source=tests/lib/tables.sh # Resolve path statically
    source "$SCRIPT_DIR/lib/tables.sh"
    export TABLES_SOURCED=true
fi

if [[ -z "$LOG_OUTPUT_SH_GUARD" ]]; then
    # shellcheck source=tests/lib/log_output.sh # Resolve path statically
    source "$SCRIPT_DIR/lib/log_output.sh"
fi

# shellcheck source=tests/lib/framework.sh # Resolve path statically
source "$SCRIPT_DIR/lib/framework.sh"
# shellcheck source=tests/lib/file_utils.sh # Resolve path statically
source "$SCRIPT_DIR/lib/file_utils.sh"
# shellcheck source=tests/lib/lifecycle.sh # Resolve path statically
source "$SCRIPT_DIR/lib/lifecycle.sh"
# shellcheck source=tests/lib/network_utils.sh # Resolve path statically
source "$SCRIPT_DIR/lib/network_utils.sh"

# Initialize test environment
RESULTS_DIR="$SCRIPT_DIR/results"
mkdir -p "$RESULTS_DIR"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)

# Auto-extract test number and set up environment
TEST_NUMBER=$(extract_test_number "${BASH_SOURCE[0]}")
set_test_number "$TEST_NUMBER"
reset_subtest_counter

# Test configuration
TOTAL_SUBTESTS=10
PASS_COUNT=0
EXIT_CODE=0

# Configuration files
DEFAULT_CONFIG_PATH="$(get_config_path "hydrogen_test_api_test_1.json")"
CUSTOM_CONFIG_PATH="$(get_config_path "hydrogen_test_api_test_2.json")"

# Function to make a request and validate response
validate_api_request() {
    local request_name="$1"
    local url="$2"
    local expected_field="$3"
    local response_file="$RESULTS_DIR/response_${request_name}_${TIMESTAMP}.json"
    
    print_command "curl -s --max-time 10 --compressed \"$url\""
    if curl -s --max-time 10 --compressed "$url" > "$response_file" 2>/dev/null; then
        print_message "Request successful, checking response content"
        
        # Validate that the response contains expected fields
        if grep -q "$expected_field" "$response_file"; then
            print_result 0 "Response contains expected field: $expected_field"
            return 0
        else
            print_result 1 "Response doesn't contain expected field: $expected_field"
            print_message "Response content:"
            print_output "$(cat "$response_file")"
            return 1
        fi
    else
        print_result 1 "Failed to connect to server"
        return 1
    fi
}

# Function to wait for server to be ready
wait_for_server_ready() {
    local base_url="$1"
    local max_attempts=40   # 20 seconds total (0.5s * 40)
    local attempt=1
    
    print_message "Waiting for server to be ready at $base_url..."
    
    while [ $attempt -le $max_attempts ]; do
        if curl -s --max-time 2 "${base_url}" >/dev/null 2>&1; then
            print_message "Server is ready after $(( attempt * 5 / 10 )) seconds"
            return 0
        fi
        sleep 0.5
        ((attempt++))
    done
    
    print_error "Server failed to respond within 20 seconds"
    return 1
}


# Handle cleanup on script interruption (not normal exit)
# shellcheck disable=SC2317  # Function is invoked indirectly via trap
cleanup() {
    print_message "Cleaning up any remaining Hydrogen processes..."
    pkill -f "hydrogen.*json" 2>/dev/null || true
    exit "$EXIT_CODE"
}

# Set up trap for interruption only (not normal exit)
trap cleanup SIGINT SIGTERM

# Main execution starts here
print_test_header "$NAME_SCRIPT" "$SCRIPT_VERSION"

# Subtest 1: Find Hydrogen binary
next_subtest
print_subtest "Locate Hydrogen Binary"
# shellcheck disable=SC2153  # HYDROGEN_BIN is set by find_hydrogen_binary function
if find_hydrogen_binary "$HYDROGEN_DIR" "HYDROGEN_BIN"; then
    print_result 0 "Hydrogen binary found: $(basename "$HYDROGEN_BIN")"
    ((PASS_COUNT++))
else
    print_result 1 "Hydrogen binary not found"
    EXIT_CODE=1
fi

# Subtest 2: Validate configuration files
next_subtest
print_subtest "Validate Configuration Files"
if [ -f "$DEFAULT_CONFIG_PATH" ] && [ -f "$CUSTOM_CONFIG_PATH" ]; then
    print_result 0 "Both configuration files found"
    ((PASS_COUNT++))
    
    # Extract and display ports
    DEFAULT_PORT=$(get_webserver_port "$DEFAULT_CONFIG_PATH")
    CUSTOM_PORT=$(get_webserver_port "$CUSTOM_CONFIG_PATH")
    print_message "Default configuration will use port: $DEFAULT_PORT"
    print_message "Custom configuration will use port: $CUSTOM_PORT"
else
    print_result 1 "One or more configuration files not found"
    EXIT_CODE=1
fi

# Only proceed with API tests if binary and configs are available
if [ $EXIT_CODE -eq 0 ]; then
    # Ensure clean state
    print_message "Ensuring no existing Hydrogen processes are running..."
    pkill -f "hydrogen.*json" 2>/dev/null || true
    
    print_message "Testing API prefix functionality with immediate restart approach"
    print_message "SO_REUSEADDR is enabled - no need to wait for TIME_WAIT"
    
    # Global variables for server management
    HYDROGEN_PID=""
    DEFAULT_PORT=$(get_webserver_port "$DEFAULT_CONFIG_PATH")
    CUSTOM_PORT=$(get_webserver_port "$CUSTOM_CONFIG_PATH")
    
    # Subtest 3: Start server with default API prefix
    next_subtest
    print_subtest "Start Server with Default API Prefix (/api)"
    DEFAULT_LOG="$RESULTS_DIR/default_server_${TIMESTAMP}.log"
    if start_hydrogen_with_pid "$DEFAULT_CONFIG_PATH" "$DEFAULT_LOG" 15 "$HYDROGEN_BIN" "HYDROGEN_PID" && [ -n "$HYDROGEN_PID" ]; then
        print_result 0 "Server started successfully with PID: $HYDROGEN_PID"
        ((PASS_COUNT++))
    else
        print_result 1 "Failed to start server with default configuration"
        EXIT_CODE=1
    fi
    
    # Subtest 4: Test default API health endpoint
    next_subtest
    print_subtest "Test Default API Health Endpoint"
    if [ -n "$HYDROGEN_PID" ] && ps -p "$HYDROGEN_PID" > /dev/null 2>&1; then
        if wait_for_server_ready "http://localhost:$DEFAULT_PORT"; then
            if validate_api_request "default_health" "http://localhost:$DEFAULT_PORT/api/system/health" "Yes, I'm alive, thanks!"; then
                ((PASS_COUNT++))
            else
                EXIT_CODE=1
            fi
        else
            print_result 1 "Server not ready for health endpoint test"
            EXIT_CODE=1
        fi
    else
        print_result 1 "Server not running for health endpoint test"
        EXIT_CODE=1
    fi
    
    # Subtest 5: Test default API info endpoint
    next_subtest
    print_subtest "Test Default API Info Endpoint"
    if [ -n "$HYDROGEN_PID" ] && ps -p "$HYDROGEN_PID" > /dev/null 2>&1; then
        if validate_api_request "default_info" "http://localhost:$DEFAULT_PORT/api/system/info" "system"; then
            ((PASS_COUNT++))
        else
            EXIT_CODE=1
        fi
    else
        print_result 1 "Server not running for info endpoint test"
        EXIT_CODE=1
    fi
    
    # Subtest 6: Test default API test endpoint
    next_subtest
    print_subtest "Test Default API Test Endpoint"
    if [ -n "$HYDROGEN_PID" ] && ps -p "$HYDROGEN_PID" > /dev/null 2>&1; then
        if validate_api_request "default_test" "http://localhost:$DEFAULT_PORT/api/system/test" "client_ip"; then
            ((PASS_COUNT++))
        else
            EXIT_CODE=1
        fi
    else
        print_result 1 "Server not running for test endpoint test"
        EXIT_CODE=1
    fi
    
    # Stop the default server
    if [ -n "$HYDROGEN_PID" ]; then
        print_message "Stopping default server..."
        if stop_hydrogen "$HYDROGEN_PID" "$DEFAULT_LOG" 10 5 "$RESULTS_DIR"; then
            print_message "Default server stopped successfully"
        else
            print_warning "Default server shutdown had issues"
        fi
        check_time_wait_sockets "$DEFAULT_PORT"
        HYDROGEN_PID=""
    fi
    
    # Start second test immediately
    print_message "Starting second test immediately (testing SO_REUSEADDR)..."
    
    # Subtest 7: Start server with custom API prefix
    next_subtest
    print_subtest "Start Server with Custom API Prefix (/myapi)"
    CUSTOM_LOG="$RESULTS_DIR/custom_server_${TIMESTAMP}.log"
    if start_hydrogen_with_pid "$CUSTOM_CONFIG_PATH" "$CUSTOM_LOG" 15 "$HYDROGEN_BIN" "HYDROGEN_PID" && [ -n "$HYDROGEN_PID" ]; then
        print_result 0 "Server started successfully with PID: $HYDROGEN_PID"
        ((PASS_COUNT++))
    else
        print_result 1 "Failed to start server with custom configuration"
        EXIT_CODE=1
    fi
    
    # Subtest 8: Test custom API health endpoint
    next_subtest
    print_subtest "Test Custom API Health Endpoint"
    if [ -n "$HYDROGEN_PID" ] && ps -p "$HYDROGEN_PID" > /dev/null 2>&1; then
        if wait_for_server_ready "http://localhost:$CUSTOM_PORT"; then
            if validate_api_request "custom_health" "http://localhost:$CUSTOM_PORT/myapi/system/health" "Yes, I'm alive, thanks!"; then
                ((PASS_COUNT++))
            else
                EXIT_CODE=1
            fi
        else
            print_result 1 "Server not ready for health endpoint test"
            EXIT_CODE=1
        fi
    else
        print_result 1 "Server not running for health endpoint test"
        EXIT_CODE=1
    fi
    
    # Subtest 9: Test custom API info endpoint
    next_subtest
    print_subtest "Test Custom API Info Endpoint"
    if [ -n "$HYDROGEN_PID" ] && ps -p "$HYDROGEN_PID" > /dev/null 2>&1; then
        if validate_api_request "custom_info" "http://localhost:$CUSTOM_PORT/myapi/system/info" "system"; then
            ((PASS_COUNT++))
        else
            EXIT_CODE=1
        fi
    else
        print_result 1 "Server not running for info endpoint test"
        EXIT_CODE=1
    fi
    
    # Subtest 10: Test custom API test endpoint
    next_subtest
    print_subtest "Test Custom API Test Endpoint"
    if [ -n "$HYDROGEN_PID" ] && ps -p "$HYDROGEN_PID" > /dev/null 2>&1; then
        if validate_api_request "custom_test" "http://localhost:$CUSTOM_PORT/myapi/system/test" "client_ip"; then
            ((PASS_COUNT++))
        else
            EXIT_CODE=1
        fi
    else
        print_result 1 "Server not running for test endpoint test"
        EXIT_CODE=1
    fi
    
    # Stop the custom server
    if [ -n "$HYDROGEN_PID" ]; then
        print_message "Stopping custom server..."
        if stop_hydrogen "$HYDROGEN_PID" "$CUSTOM_LOG" 10 5 "$RESULTS_DIR"; then
            print_message "Custom server stopped successfully"
        else
            print_warning "Custom server shutdown had issues"
        fi
        check_time_wait_sockets "$CUSTOM_PORT"
        HYDROGEN_PID=""
    fi
    
    print_message "Immediate restart successful - SO_REUSEADDR is working!"
else
    # Skip API tests if prerequisites failed
    print_result 1 "Skipping API prefix tests due to prerequisite failures"
    # Account for skipped subtests
    for i in {3..10}; do
        print_result 1 "Subtest $i skipped due to prerequisite failures"
    done
    EXIT_CODE=1
fi

# Calculate overall test result
if [ $PASS_COUNT -eq $TOTAL_SUBTESTS ]; then
    FINAL_RESULT=0
else
    FINAL_RESULT=1
fi

# Clean up response files but preserve logs if test failed
rm -f "$RESULTS_DIR"/response_*.json

# Only remove logs if tests were successful
if [ $FINAL_RESULT -eq 0 ]; then
    print_message "Tests passed, cleaning up log files..."
    rm -f "$RESULTS_DIR"/*_server_*.log
else
    print_message "Tests failed, preserving log files for analysis in $RESULTS_DIR/"
fi

# Export results for test_all.sh integration
# Derive test name from script filename for consistency with test_00_all.sh
TEST_IDENTIFIER=$(basename "${BASH_SOURCE[0]}" .sh | sed 's/test_[0-9]*_//')
export_subtest_results "${TEST_NUMBER}_${TEST_IDENTIFIER}" "$TOTAL_SUBTESTS" "$PASS_COUNT" "$TEST_NAME" > /dev/null

# Print test completion summary
print_test_completion "$TEST_NAME"

end_test "$FINAL_RESULT" "$TOTAL_SUBTESTS" "$PASS_COUNT" > /dev/null

# Return status code if sourced, exit if run standalone
if [[ "$RUNNING_IN_TEST_SUITE" == "true" ]]; then
    return $FINAL_RESULT
else
    exit $FINAL_RESULT
fi
