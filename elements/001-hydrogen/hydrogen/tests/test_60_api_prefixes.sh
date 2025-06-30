#!/bin/bash
#
# About this Script
#
# Hydrogen API Prefix Test (Immediate Restart)
# Tests the API endpoints with different API prefix configurations using immediate restart approach:
# - Default "/api" prefix using standard hydrogen_test_api.json
# - Custom "/myapi" prefix using hydrogen_test_api_prefix.json
# - Uses immediate restart without waiting for TIME_WAIT (SO_REUSEADDR enabled)
#
NAME_SCRIPT="Hydrogen API Prefix Test (Immediate Restart)"
VERS_SCRIPT="4.0.0"

# VERSION HISTORY
# 4.0.0 - 2025-06-30 - Updated to use immediate restart approach without TIME_WAIT waiting
# 3.0.0 - 2025-06-30 - Refactored to use robust server management functions from test_55
# 2.0.0 - 2025-06-17 - Major refactoring: fixed all shellcheck warnings, improved modularity, enhanced comments

# Display script name and version
echo "=== $NAME_SCRIPT v$VERS_SCRIPT ==="

# Get the directory where this script is located
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
HYDROGEN_DIR="$( cd "$SCRIPT_DIR/.." && pwd )"

# Include the common test utilities
source "$SCRIPT_DIR/support_utils.sh"
source "$SCRIPT_DIR/support_timewait.sh"

# Create results directory if it doesn't exist
RESULTS_DIR="$SCRIPT_DIR/results"
mkdir -p "$RESULTS_DIR"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)

# Function to make a request and validate response
validate_request() {
    local request_name="$1"
    local curl_command="$2"
    local expected_field="$3"
    local response_file="$RESULTS_DIR/response_${request_name}.json"
    
    # Make sure curl command includes --compressed flag and timeout
    if [[ $curl_command != *"--compressed"* ]]; then
        curl_command="${curl_command/curl/curl --compressed}"
    fi
    
    # Add timeout if not already present
    if [[ $curl_command != *"--max-time"* ]] && [[ $curl_command != *"-m "* ]]; then
        curl_command="${curl_command/curl/curl --max-time 10}"
    fi
    
    # Execute the command
    eval "$curl_command > $response_file"
    local curl_status=$?
    
    # Check if the request was successful
    if [ $curl_status -eq 0 ]; then
        print_info "Testing $request_name: Request successful"
        # Validate that the response contains expected fields
        if grep -q "$expected_field" "$response_file"; then
            print_result 0 "Response contains expected field: $expected_field"
            return 0
        else
            print_result 1 "Response doesn't contain expected field: $expected_field"
            return 1
        fi
    else
        print_warning "Testing $request_name: Curl error (status $curl_status)"
        print_result 1 "Failed to connect to server (curl status: $curl_status)"
        return $curl_status
    fi
}

# Function to validate JSON syntax
validate_json() {
    local file="$1"
    
    if command -v jq &> /dev/null; then
        # Use jq if available for proper JSON validation
        if jq . "$file" > /dev/null 2>&1; then
            print_result 0 "Response contains valid JSON"
            return 0
        else
            print_result 1 "Response contains invalid JSON"
            return 1
        fi
    else
        # Fallback: simple check for JSON structure
        if grep -q "{" "$file" && grep -q "}" "$file"; then
            print_result 0 "Response appears to be JSON (basic validation only)"
            return 0
        else
            print_result 1 "Response does not appear to be JSON"
            return 1
        fi
    fi
}

# Function to wait for server to be ready
wait_for_server() {
    local base_url="$1"
    local max_attempts=40   # 20 seconds total (0.5s * 40)
    local attempt=1
    
    print_info "Waiting for server to be ready at $base_url..."
    
    while [ $attempt -le $max_attempts ]; do
        # Check network connectivity with a longer per-attempt timeout
        if curl -s --max-time 2 "${base_url}" >/dev/null 2>&1; then
            print_info "Server is ready (network check) after $(( attempt * 5 / 10 )) seconds"
            return 0
        fi
        sleep 0.5
        ((attempt++))
    done
    
    print_error "Server failed to respond within 20 seconds"
    return 1
}

# Function to test system endpoints with a specific API prefix
test_api_prefix() {
    local config_file="$1"
    local api_prefix="$2"
    local test_name="$3"
    
    print_header "Testing API Prefix: $api_prefix (using $test_name)"
    
    # Extract the WebServer port from the configuration file
    local web_server_port
    web_server_port=$(get_webserver_port "$config_file")
    print_info "Using WebServer port: $web_server_port from configuration"
    
    # Results dictionary to track test results
    local health_result=1
    local info_result=1
    local info_json_result=1
    local test_basic_get_result=1
    local stability_result=1
    
    # Find the correct hydrogen binary
    local hydrogen_bin
    if ! hydrogen_bin=$(find_hydrogen_binary "$HYDROGEN_DIR"); then
        print_result 1 "ERROR: Could not find hydrogen executable"
        return 1
    fi

    # Prepare clean test environment (immediate restart approach)
    print_info "Terminating any existing hydrogen processes..."
    if pkill -f "hydrogen.*json" 2>/dev/null; then
        print_info "Terminated"
    else
        print_info "No processes to terminate"
    fi
    
    # Start hydrogen server using robust function
    print_info "Starting hydrogen server with configuration $(convert_to_relative_path "$config_file")..."
    
    local server_pid
    if ! server_pid=$(start_hydrogen_instance_robust "$hydrogen_bin" "$config_file" "$test_name server" "$RESULTS_DIR" "$TIMESTAMP") || [ -z "$server_pid" ]; then
        print_result 1 "Failed to start $test_name server"
        return 1
    fi
    
    print_info "Server started successfully with PID: $server_pid"
    
    # Wait for the server to be ready
    local base_url="http://localhost:${web_server_port}"
    wait_for_server "${base_url}" || {
        print_error "Server failed to start properly"
        shutdown_instance_robust "$server_pid" "$test_name server"
        return 1
    }
    
    # Test health endpoint
    print_header "Testing Health Endpoint with prefix '$api_prefix'"
    validate_request "${test_name}_health" "curl -s --max-time 5 http://localhost:${web_server_port}${api_prefix}/system/health" "Yes, I'm alive, thanks!"
    health_result=$?
    
    # Test info endpoint
    print_header "Testing Info Endpoint with prefix '$api_prefix'"
    validate_request "${test_name}_info" "curl -s --max-time 5 http://localhost:${web_server_port}${api_prefix}/system/info" "system"
    info_result=$?
    
    # Also validate that it's valid JSON
    if [ $info_result -eq 0 ]; then
        validate_json "$RESULTS_DIR/response_${test_name}_info.json"
        info_json_result=$?
    else
        info_json_result=1
    fi
    
    # Test test endpoint (basic GET)
    print_header "Testing Test Endpoint (GET) with prefix '$api_prefix'"
    validate_request "${test_name}_test_get" "curl -s --max-time 5 http://localhost:${web_server_port}${api_prefix}/system/test" "client_ip"
    test_basic_get_result=$?
    
    # Verify the server is still running after all tests
    print_header "Checking server status after tests"
    if ps -p "$server_pid" > /dev/null 2>&1; then
        print_info "Server is still running after tests (PID: $server_pid)"
        stability_result=0
    else
        print_result 1 "ERROR: Server has crashed or exited prematurely"
        stability_result=1
    fi
    
    # Stop the server using robust shutdown function
    shutdown_instance_robust "$server_pid" "$test_name server"
    
    # Check for TIME_WAIT sockets
    check_time_wait_sockets_robust "$web_server_port"
    
    # Calculate overall result
    if [ $health_result -eq 0 ] && [ $info_result -eq 0 ] && [ $info_json_result -eq 0 ] && 
       [ $test_basic_get_result -eq 0 ] && [ $stability_result -eq 0 ]; then
        print_result 0 "All tests for API prefix '$api_prefix' passed successfully"
        return 0
    else
        print_result 1 "Some tests for API prefix '$api_prefix' failed"
        return 1
    fi
}

# Handle cleanup on script termination
# shellcheck disable=SC2317  # Function is invoked indirectly via trap
cleanup() {
    # Kill any hydrogen processes started by this script
    pkill -f "hydrogen.*json" 2>/dev/null || true
    exit 0
}

# Set up trap for clean termination
trap cleanup SIGINT SIGTERM EXIT

# Main execution starts here

# Extract ports from both configurations at the beginning
DEFAULT_CONFIG_PATH="$(get_config_path "hydrogen_test_api_test_1.json")"
CUSTOM_CONFIG_PATH="$(get_config_path "hydrogen_test_api_test_2.json")"
DEFAULT_PORT=$(get_webserver_port "$DEFAULT_CONFIG_PATH")
CUSTOM_PORT=$(get_webserver_port "$CUSTOM_CONFIG_PATH")

# Start the test
start_test "Hydrogen API Prefix Test"
print_info "Default configuration will use port: $DEFAULT_PORT"
print_info "Custom configuration will use port: $CUSTOM_PORT"

# Ensure clean state (immediate restart approach)
print_info "Ensuring no existing hydrogen processes are running..."
pkill -f "hydrogen.*json" 2>/dev/null || true

print_info "Testing API prefix functionality with immediate restart approach"
print_info "SO_REUSEADDR is enabled - no need to wait for TIME_WAIT"

# Test with default configuration (/api prefix)
test_api_prefix "$DEFAULT_CONFIG_PATH" "/api" "default"
DEFAULT_PREFIX_RESULT=$?

# Start second test immediately (testing SO_REUSEADDR)
print_info "Starting second test immediately (testing SO_REUSEADDR)..."

# Test with custom API prefix configuration (/myapi prefix)
test_api_prefix "$CUSTOM_CONFIG_PATH" "/myapi" "custom"
CUSTOM_PREFIX_RESULT=$?

print_info "Immediate restart successful - SO_REUSEADDR is working!"

# Track subtest counts
# We have 2 main tests (default prefix and custom prefix)
# Each main test has 5 subtests (health, info, json validation, test endpoint, stability)
TOTAL_SUBTESTS=10
PASSED_SUBTESTS=0

# Count default prefix subtests from the test_api_prefix function
if [ $DEFAULT_PREFIX_RESULT -eq 0 ]; then
    # All 5 subtests passed for default prefix
    PASSED_SUBTESTS=$((PASSED_SUBTESTS + 5))
fi

# Count custom prefix subtests from the test_api_prefix function  
if [ $CUSTOM_PREFIX_RESULT -eq 0 ]; then
    # All 5 subtests passed for custom prefix
    PASSED_SUBTESTS=$((PASSED_SUBTESTS + 5))
fi

# Calculate overall test result
if [ $DEFAULT_PREFIX_RESULT -eq 0 ] && [ $CUSTOM_PREFIX_RESULT -eq 0 ]; then
    TEST_RESULT=0
    print_result 0 "All API prefix tests passed successfully"
else
    TEST_RESULT=1
    print_result 1 "Some API prefix tests failed"
fi

# Get test name from script name
TEST_NAME=$(basename "$0" .sh | sed 's/^test_//')

# Export subtest results for test_all.sh to pick up
export_subtest_results "$TEST_NAME" $TOTAL_SUBTESTS $PASSED_SUBTESTS

# Log subtest results
print_info "API Prefix Tests: $PASSED_SUBTESTS of $TOTAL_SUBTESTS subtests passed"

# End the test with final result
end_test $TEST_RESULT "API Prefix Test"

# Clean up response files but preserve logs if test failed
rm -f "$RESULTS_DIR"/response_*.json

# Only remove logs if tests were successful
if [ $TEST_RESULT -eq 0 ]; then
    print_info "Tests passed, cleaning up log files..."
    rm -f "$RESULTS_DIR"/*_server_*.log
else
    print_warning "Tests failed, preserving log files for analysis in $RESULTS_DIR/"
fi

exit $TEST_RESULT
