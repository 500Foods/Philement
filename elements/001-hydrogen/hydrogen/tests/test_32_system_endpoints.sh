#!/bin/bash
#
# Test: System API Endpoints
# Tests all the system API endpoints:
# - /api/system/test: Tests request handling with various parameters
# - /api/system/health: Tests the health check endpoint
# - /api/system/info: Tests the system information endpoint
# - /api/system/config: Tests the configuration endpoint
# - /api/system/prometheus: Tests the Prometheus metrics endpoint
# - /api/system/recent: Tests the recent activity log endpoint
# - /api/system/appconfig: Tests the application configuration endpoint
#
NAME_SCRIPT="System API Endpoints"
VERS_SCRIPT="3.0.0"

# VERSION HISTORY
# 3.0.0 - 2025-07-02 - Migrated to use lib/ scripts, following established test patterns
# 2.0.0 - 2025-06-17 - Major refactoring: fixed all shellcheck warnings, improved modularity, enhanced comments

# Get the directory where this script is located
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
HYDROGEN_DIR="$( cd "$SCRIPT_DIR/.." && pwd )"

# Source the library scripts with guard clauses
if [[ -z "$TABLES_SH_GUARD" ]] || ! command -v tables_render_from_json >/dev/null 2>&1; then
    # shellcheck source=tests/lib/tables.sh
    source "$SCRIPT_DIR/lib/tables.sh"
    export TABLES_SOURCED=true
fi

if [[ -z "$LOG_OUTPUT_SH_GUARD" ]]; then
    # shellcheck source=tests/lib/log_output.sh
    source "$SCRIPT_DIR/lib/log_output.sh"
fi

# shellcheck source=tests/lib/framework.sh
source "$SCRIPT_DIR/lib/framework.sh"
# shellcheck source=tests/lib/file_utils.sh
source "$SCRIPT_DIR/lib/file_utils.sh"
# shellcheck source=tests/lib/lifecycle.sh
source "$SCRIPT_DIR/lib/lifecycle.sh"
# shellcheck source=tests/lib/network_utils.sh
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
TOTAL_SUBTESTS=18
PASS_COUNT=0
EXIT_CODE=0

# Configuration file for API testing
CONFIG_PATH="$(get_config_path "hydrogen_test_system_endpoints.json")"

# Function to make a request and validate response
validate_api_request() {
    local request_name="$1"
    local url="$2"
    local expected_field="$3"
    local response_file="$RESULTS_DIR/response_${request_name}_${TIMESTAMP}"
    
    # Add appropriate extension based on endpoint type
    if [[ "$request_name" == "prometheus" ]] || [[ "$request_name" == "recent" ]] || [[ "$request_name" == "appconfig" ]]; then
        response_file="${response_file}.txt"
    else
        response_file="${response_file}.json"
    fi
    
    print_command "curl -s --max-time 10 --compressed \"$url\""
    if curl -s --max-time 10 --compressed "$url" > "$response_file" 2>/dev/null; then
        print_message "Request successful, checking response content"
        
        # For recent and appconfig endpoints, just show line count
        if [[ "$request_name" == "recent" ]] || [[ "$request_name" == "appconfig" ]]; then
            local line_count
            line_count=$(wc -l < "$response_file")
            print_message "Response contains $line_count lines"
        else
            print_message "Response received, validating content"
        fi
        
        # Validate that the response contains expected fields
        if [[ "$request_name" == "recent" ]]; then
            # Use fixed string search for recent endpoint
            if grep -F -q "[" "$response_file"; then
                print_result 0 "Response contains expected field: log entry"
                return 0
            else
                print_result 1 "Response doesn't contain expected field: log entry"
                return 1
            fi
        else
            # Normal pattern search for other endpoints
            if grep -q "$expected_field" "$response_file"; then
                print_result 0 "Response contains expected field: $expected_field"
                return 0
            else
                print_result 1 "Response doesn't contain expected field: $expected_field"
                print_message "Response content:"
                print_output "$(cat "$response_file")"
                return 1
            fi
        fi
    else
        print_result 1 "Failed to connect to server"
        return 1
    fi
}

# Function to validate JSON syntax
validate_json_response() {
    local file="$1"
    local endpoint_name="$2"
    
    if command -v jq &> /dev/null; then
        # Use jq if available for proper JSON validation
        if jq . "$file" > /dev/null 2>&1; then
            print_result 0 "$endpoint_name endpoint returns valid JSON"
            return 0
        else
            print_result 1 "$endpoint_name endpoint returns invalid JSON"
            return 1
        fi
    else
        # Fallback: simple check for JSON structure
        if grep -q "{" "$file" && grep -q "}" "$file"; then
            print_result 0 "$endpoint_name endpoint appears to be JSON (basic validation only)"
            return 0
        else
            print_result 1 "$endpoint_name endpoint does not appear to be JSON"
            return 1
        fi
    fi
}

# Function to validate line count in text response
validate_line_count() {
    local file="$1"
    local min_lines="$2"
    local endpoint_name="$3"
    local actual_lines
    actual_lines=$(wc -l < "$file")
    
    if [ "$actual_lines" -gt "$min_lines" ]; then
        print_result 0 "$endpoint_name endpoint contains $actual_lines lines (more than $min_lines required)"
        return 0
    else
        print_result 1 "$endpoint_name endpoint contains only $actual_lines lines ($min_lines required)"
        return 1
    fi
}

# Function to validate Prometheus format
validate_prometheus_format() {
    local file="$1"
    
    # Check content type
    if ! grep -q "Content-Type: text/plain" "$file"; then
        print_result 1 "Prometheus endpoint has incorrect content type"
        return 1
    fi
    
    # Check for required Prometheus format elements
    local prometheus_format_ok=1
    
    # Check for TYPE definitions
    if ! grep -q "^# TYPE" "$file"; then
        print_result 1 "Prometheus endpoint missing TYPE definitions"
        prometheus_format_ok=0
    fi
    
    # Check for specific metrics
    local required_metrics=(
        "system_info"
        "memory_total_bytes"
        "cpu_usage_total"
        "service_threads"
    )
    
    local metric
    for metric in "${required_metrics[@]}"; do
        if ! grep -q "^$metric" "$file"; then
            print_result 1 "Prometheus endpoint missing required metric: $metric"
            prometheus_format_ok=0
        fi
    done
    
    if [ $prometheus_format_ok -eq 1 ]; then
        print_result 0 "Prometheus endpoint contains valid format and metrics"
        return 0
    else
        return 1
    fi
}

# Function to wait for server to be ready
wait_for_server_ready() {
    local base_url="$1"
    local max_attempts=100   # 20 seconds total (0.2s * 100)
    local attempt=1
    
    print_message "Waiting for server to be ready at $base_url..."
    
    while [ $attempt -le $max_attempts ]; do
        if curl -s --max-time 2 "${base_url}" >/dev/null 2>&1; then
            print_message "Server is ready after $(( attempt * 2 / 10 )) seconds"
            return 0
        fi
        sleep 0.2
        ((attempt++))
    done
    
    print_error "Server failed to respond within 20 seconds"
    return 1
}

# Function to check server logs for compression and errors
check_server_logs() {
    local log_file="$1"
    local timestamp="$2"
    
    # Check server logs for API-related errors - filter irrelevant messages
    print_message "Checking server logs for API-related errors..."
    if grep -i "error\|warn\|fatal\|segmentation" "$log_file" | grep -i "API\|System\|SystemTest\|SystemService\|Endpoint\|api" > "$RESULTS_DIR/system_test_errors_${timestamp}.log"; then
        if [ -s "$RESULTS_DIR/system_test_errors_${timestamp}.log" ]; then
            print_warning "API-related warning/error messages found in logs:"
            # Process each line individually for clean formatting
            while IFS= read -r line; do
                print_output "$line"
            done < "$RESULTS_DIR/system_test_errors_${timestamp}.log"
        fi
    fi

    # Check for Brotli compression logs
    print_message "Checking for Brotli compression logs..."
    if grep -i "Brotli" "$log_file" > "$RESULTS_DIR/brotli_compression_${timestamp}.log" 2>/dev/null; then
        if [ -s "$RESULTS_DIR/brotli_compression_${timestamp}.log" ]; then
            print_message "Brotli compression logs found:"
            # Process each line individually for clean formatting
            while IFS= read -r line; do
                print_output "$line"
            done < "$RESULTS_DIR/brotli_compression_${timestamp}.log"
            # Check for compression metrics with level information
            if grep -q "Brotli(level=[0-9]\+).*bytes.*ratio.*compression.*time:" "$RESULTS_DIR/brotli_compression_${timestamp}.log"; then
                print_result 0 "Compression logs contain detailed metrics with compression level"
                return 0
            else
                print_warning "Compression logs found but missing metrics details"
                return 1
            fi
        else
            print_warning "No Brotli compression logs found"
            return 1
        fi
    else
        print_warning "No Brotli compression logs found"
        return 1
    fi
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
print_test_header "$NAME_SCRIPT" "$VERS_SCRIPT"

# Subtest 1: Find Hydrogen binary
next_subtest
print_subtest "Locate Hydrogen Binary"
if HYDROGEN_BIN=$(find_hydrogen_binary "$HYDROGEN_DIR"); then
    print_result 0 "Hydrogen binary found: $(basename "$HYDROGEN_BIN")"
    ((PASS_COUNT++))
else
    print_result 1 "Hydrogen binary not found"
    EXIT_CODE=1
fi

# Subtest 2: Validate configuration file
next_subtest
print_subtest "Validate Configuration File"
if validate_config_file "$CONFIG_PATH"; then
    ((PASS_COUNT++))
    
    # Extract and display port
    SERVER_PORT=$(get_webserver_port "$CONFIG_PATH")
    print_message "Configuration will use port: $SERVER_PORT"
else
    EXIT_CODE=1
fi

# Only proceed with API tests if binary and config are available
if [ $EXIT_CODE -eq 0 ]; then
    # Ensure clean state
    print_message "Ensuring no existing Hydrogen processes are running..."
    pkill -f "hydrogen.*json" 2>/dev/null || true
    
    # Global variables for server management
    HYDROGEN_PID=""
    SERVER_LOG="$RESULTS_DIR/system_endpoints_server_${TIMESTAMP}.log"
    BASE_URL="http://localhost:$SERVER_PORT"
    
    # Subtest 3: Start server
    next_subtest
    print_subtest "Start Hydrogen Server"
    if HYDROGEN_PID=$(start_hydrogen "$CONFIG_PATH" "$SERVER_LOG" 15 "$HYDROGEN_BIN") && [ -n "$HYDROGEN_PID" ]; then
        print_result 0 "Server started successfully with PID: $HYDROGEN_PID"
        ((PASS_COUNT++))
    else
        print_result 1 "Failed to start server"
        EXIT_CODE=1
    fi
    
    # Wait for server to be ready
    if [ -n "$HYDROGEN_PID" ] && ps -p "$HYDROGEN_PID" > /dev/null 2>&1; then
        if ! wait_for_server_ready "$BASE_URL"; then
            print_result 1 "Server failed to become ready"
            EXIT_CODE=1
        fi
    fi
    
    # Subtest 4: Test health endpoint
    next_subtest
    print_subtest "Test Health Endpoint"
    if [ -n "$HYDROGEN_PID" ] && ps -p "$HYDROGEN_PID" > /dev/null 2>&1; then
        if validate_api_request "health" "$BASE_URL/api/system/health" "Yes, I'm alive, thanks!"; then
            ((PASS_COUNT++))
        else
            EXIT_CODE=1
        fi
    else
        print_result 1 "Server not running for health endpoint test"
        EXIT_CODE=1
    fi
    
    # Subtest 5: Test config endpoint
    next_subtest
    print_subtest "Test Config Endpoint"
    if [ -n "$HYDROGEN_PID" ] && ps -p "$HYDROGEN_PID" > /dev/null 2>&1; then
        if validate_api_request "config" "$BASE_URL/api/system/config" "ServerName"; then
            ((PASS_COUNT++))
        else
            EXIT_CODE=1
        fi
    else
        print_result 1 "Server not running for config endpoint test"
        EXIT_CODE=1
    fi
    
    # Subtest 6: Validate config JSON format
    next_subtest
    print_subtest "Validate Config JSON Format"
    if [ -f "$RESULTS_DIR/response_config_${TIMESTAMP}.json" ]; then
        if validate_json_response "$RESULTS_DIR/response_config_${TIMESTAMP}.json" "Config"; then
            ((PASS_COUNT++))
        else
            EXIT_CODE=1
        fi
    else
        print_result 1 "Config response file not found"
        EXIT_CODE=1
    fi
    
    # Subtest 7: Test info endpoint
    next_subtest
    print_subtest "Test Info Endpoint"
    if [ -n "$HYDROGEN_PID" ] && ps -p "$HYDROGEN_PID" > /dev/null 2>&1; then
        if validate_api_request "info" "$BASE_URL/api/system/info" "system"; then
            ((PASS_COUNT++))
        else
            EXIT_CODE=1
        fi
    else
        print_result 1 "Server not running for info endpoint test"
        EXIT_CODE=1
    fi
    
    # Subtest 8: Validate info JSON format
    next_subtest
    print_subtest "Validate Info JSON Format"
    if [ -f "$RESULTS_DIR/response_info_${TIMESTAMP}.json" ]; then
        if validate_json_response "$RESULTS_DIR/response_info_${TIMESTAMP}.json" "Info"; then
            ((PASS_COUNT++))
        else
            EXIT_CODE=1
        fi
    else
        print_result 1 "Info response file not found"
        EXIT_CODE=1
    fi
    
    # Subtest 9: Test prometheus endpoint
    next_subtest
    print_subtest "Test Prometheus Endpoint"
    if [ -n "$HYDROGEN_PID" ] && ps -p "$HYDROGEN_PID" > /dev/null 2>&1; then
        # Special handling for Prometheus endpoint to include headers
        print_command "curl -s --max-time 10 -i \"$BASE_URL/api/system/prometheus\""
        if curl -s --max-time 10 -i "$BASE_URL/api/system/prometheus" > "$RESULTS_DIR/response_prometheus_${TIMESTAMP}.txt" 2>/dev/null; then
            print_message "Request successful, checking response content"
            if grep -q "# HELP" "$RESULTS_DIR/response_prometheus_${TIMESTAMP}.txt"; then
                print_result 0 "Response contains expected field: # HELP"
                ((PASS_COUNT++))
            else
                print_result 1 "Response doesn't contain expected field: # HELP"
                EXIT_CODE=1
            fi
        else
            print_result 1 "Failed to connect to server"
            EXIT_CODE=1
        fi
    else
        print_result 1 "Server not running for prometheus endpoint test"
        EXIT_CODE=1
    fi
    
    # Subtest 10: Validate prometheus format
    next_subtest
    print_subtest "Validate Prometheus Format"
    if [ -f "$RESULTS_DIR/response_prometheus_${TIMESTAMP}.txt" ]; then
        if validate_prometheus_format "$RESULTS_DIR/response_prometheus_${TIMESTAMP}.txt"; then
            ((PASS_COUNT++))
        else
            EXIT_CODE=1
        fi
    else
        print_result 1 "Prometheus response file not found"
        EXIT_CODE=1
    fi
    
    # Subtest 11: Test system test endpoint (basic GET)
    next_subtest
    print_subtest "Test System Test Endpoint (Basic GET)"
    if [ -n "$HYDROGEN_PID" ] && ps -p "$HYDROGEN_PID" > /dev/null 2>&1; then
        if validate_api_request "basic_get" "$BASE_URL/api/system/test" "client_ip"; then
            ((PASS_COUNT++))
        else
            EXIT_CODE=1
        fi
    else
        print_result 1 "Server not running for basic GET test"
        EXIT_CODE=1
    fi
    
    # Subtest 12: Test system test endpoint (GET with parameters)
    next_subtest
    print_subtest "Test System Test Endpoint (GET with Parameters)"
    if [ -n "$HYDROGEN_PID" ] && ps -p "$HYDROGEN_PID" > /dev/null 2>&1; then
        if validate_api_request "get_with_params" "$BASE_URL/api/system/test?param1=value1&param2=value2" "param1"; then
            ((PASS_COUNT++))
        else
            EXIT_CODE=1
        fi
    else
        print_result 1 "Server not running for GET with parameters test"
        EXIT_CODE=1
    fi
    
    # Subtest 13: Test system test endpoint (POST with form data)
    next_subtest
    print_subtest "Test System Test Endpoint (POST with Form Data)"
    if [ -n "$HYDROGEN_PID" ] && ps -p "$HYDROGEN_PID" > /dev/null 2>&1; then
        print_command "curl -s --max-time 10 -X POST -H 'Content-Type: application/x-www-form-urlencoded' -d 'field1=value1&field2=value2' \"$BASE_URL/api/system/test\""
        if curl -s --max-time 10 -X POST -H 'Content-Type: application/x-www-form-urlencoded' -d 'field1=value1&field2=value2' "$BASE_URL/api/system/test" > "$RESULTS_DIR/response_post_form_${TIMESTAMP}.json" 2>/dev/null; then
            if grep -q "post_data" "$RESULTS_DIR/response_post_form_${TIMESTAMP}.json"; then
                print_result 0 "POST with form data successful"
                ((PASS_COUNT++))
            else
                print_result 1 "POST response doesn't contain expected field: post_data"
                EXIT_CODE=1
            fi
        else
            print_result 1 "Failed to connect to server for POST test"
            EXIT_CODE=1
        fi
    else
        print_result 1 "Server not running for POST with form data test"
        EXIT_CODE=1
    fi
    
    # Subtest 14: Test system test endpoint (POST with parameters and form data)
    next_subtest
    print_subtest "Test System Test Endpoint (POST with Parameters and Form Data)"
    if [ -n "$HYDROGEN_PID" ] && ps -p "$HYDROGEN_PID" > /dev/null 2>&1; then
        print_command "curl -s --max-time 10 -X POST -H 'Content-Type: application/x-www-form-urlencoded' -d 'field1=value1&field2=value2' \"$BASE_URL/api/system/test?param1=value1&param2=value2\""
        if curl -s --max-time 10 -X POST -H 'Content-Type: application/x-www-form-urlencoded' -d 'field1=value1&field2=value2' "$BASE_URL/api/system/test?param1=value1&param2=value2" > "$RESULTS_DIR/response_post_with_params_${TIMESTAMP}.json" 2>/dev/null; then
            if grep -q "param1" "$RESULTS_DIR/response_post_with_params_${TIMESTAMP}.json"; then
                print_result 0 "POST with parameters and form data successful"
                ((PASS_COUNT++))
            else
                print_result 1 "POST response doesn't contain expected field: param1"
                EXIT_CODE=1
            fi
        else
            print_result 1 "Failed to connect to server for POST with parameters test"
            EXIT_CODE=1
        fi
    else
        print_result 1 "Server not running for POST with parameters test"
        EXIT_CODE=1
    fi
    
    # Subtest 15: Test recent endpoint
    next_subtest
    print_subtest "Test Recent Endpoint"
    if [ -n "$HYDROGEN_PID" ] && ps -p "$HYDROGEN_PID" > /dev/null 2>&1; then
        if validate_api_request "recent" "$BASE_URL/api/system/recent" "["; then
            ((PASS_COUNT++))
        else
            EXIT_CODE=1
        fi
    else
        print_result 1 "Server not running for recent endpoint test"
        EXIT_CODE=1
    fi
    
    # Subtest 16: Validate recent endpoint line count
    next_subtest
    print_subtest "Validate Recent Endpoint Line Count"
    if [ -f "$RESULTS_DIR/response_recent_${TIMESTAMP}.txt" ]; then
        if validate_line_count "$RESULTS_DIR/response_recent_${TIMESTAMP}.txt" 100 "Recent"; then
            ((PASS_COUNT++))
        else
            EXIT_CODE=1
        fi
    else
        print_result 1 "Recent response file not found"
        EXIT_CODE=1
    fi
    
    # Subtest 17: Test appconfig endpoint
    next_subtest
    print_subtest "Test Appconfig Endpoint"
    if [ -n "$HYDROGEN_PID" ] && ps -p "$HYDROGEN_PID" > /dev/null 2>&1; then
        if validate_api_request "appconfig" "$BASE_URL/api/system/appconfig" "APPCONFIG"; then
            ((PASS_COUNT++))
        else
            EXIT_CODE=1
        fi
    else
        print_result 1 "Server not running for appconfig endpoint test"
        EXIT_CODE=1
    fi
    
    # Subtest 18: Check Brotli compression logs
    next_subtest
    print_subtest "Check Brotli Compression Logs"
    if check_server_logs "$SERVER_LOG" "$TIMESTAMP"; then
        ((PASS_COUNT++))
    else
        EXIT_CODE=1
    fi
    
    # Stop the server
    if [ -n "$HYDROGEN_PID" ]; then
        print_message "Stopping server..."
        if stop_hydrogen "$HYDROGEN_PID" "$SERVER_LOG" 10 5 "$RESULTS_DIR"; then
            print_message "Server stopped successfully"
        else
            print_warning "Server shutdown had issues"
        fi
        HYDROGEN_PID=""
    fi
    
    # Check server stability - the stop_hydrogen function already verifies clean shutdown
    print_message "Server process properly terminated"
    
else
    # Skip API tests if prerequisites failed
    print_message "Skipping API endpoint tests due to prerequisite failures"
    # Account for skipped subtests
    for i in {3..18}; do
        next_subtest
        print_subtest "Subtest $i (Skipped)"
        print_result 1 "Skipped due to prerequisite failures"
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
rm -f "$RESULTS_DIR"/response_*.json "$RESULTS_DIR"/response_*.txt

# Only remove logs if tests were successful
if [ $FINAL_RESULT -eq 0 ]; then
    print_message "Tests passed, cleaning up log files..."
    rm -f "$RESULTS_DIR"/*_server_*.log "$RESULTS_DIR"/*_errors_*.log "$RESULTS_DIR"/*_compression_*.log
else
    print_message "Tests failed, preserving log files for analysis in $RESULTS_DIR/"
fi

# Export results for test_all.sh integration
# Derive test name from script filename for consistency with test_00_all.sh
TEST_IDENTIFIER=$(basename "${BASH_SOURCE[0]}" .sh | sed 's/test_[0-9]*_//')
export_subtest_results "${TEST_NUMBER}_${TEST_IDENTIFIER}" "$TOTAL_SUBTESTS" "$PASS_COUNT" > /dev/null

# Print test completion summary
print_test_completion "$NAME_SCRIPT"

# Return status code if sourced, exit if run standalone
if [[ "$RUNNING_IN_TEST_SUITE" == "true" ]]; then
    return $FINAL_RESULT
else
    exit $FINAL_RESULT
fi
