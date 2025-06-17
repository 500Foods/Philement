#!/bin/bash
#
# About this Script
#
# Hydrogen System API Endpoints Test
# Tests all the system API endpoints:
# - /api/system/test: Tests request handling with various parameters
# - /api/system/health: Tests the health check endpoint
# - /api/system/info: Tests the system information endpoint
# - /api/system/config: Tests the configuration endpoint
# - /api/system/prometheus: Tests the Prometheus metrics endpoint
# - /api/system/recent: Tests the recent activity log endpoint
# - /api/system/appconfig: Tests the application configuration endpoint
#
NAME_SCRIPT="Hydrogen System API Endpoints Test"
VERS_SCRIPT="2.0.0"

# VERSION HISTORY
# 2.0.0 - 2025-06-17 - Major refactoring: fixed all shellcheck warnings, improved modularity, enhanced comments

# Display script name and version
echo "=== $NAME_SCRIPT v$VERS_SCRIPT ==="

# Get the directory where this script is located
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

# Include the common test utilities
source "$SCRIPT_DIR/support_utils.sh"

# Function to make a request and validate response
validate_request() {
    local request_name="$1"
    local curl_command="$2"
    local expected_field="$3"
    local response_file="response_${request_name}"
    
    # Add appropriate extension based on endpoint type
    if [[ "$request_name" == "prometheus" ]] || [[ "$request_name" == "recent" ]] || [[ "$request_name" == "appconfig" ]]; then
        response_file="${response_file}.txt"
    else
        response_file="${response_file}.json"
    fi
    
    # Make sure curl command includes --compressed flag and timeout
    if [[ $curl_command != *"--compressed"* ]]; then
        curl_command="${curl_command/curl/curl --compressed}"
    fi
    
    # Add timeout if not already present
    if [[ $curl_command != *"--max-time"* ]] && [[ $curl_command != *"-m "* ]]; then
        curl_command="${curl_command/curl/curl --max-time 5}"
    fi
    
    print_command "$curl_command"
    eval "$curl_command > $response_file"
    local curl_status=$?
    
    # Check if the request was successful
    if [ $curl_status -eq 0 ]; then
        print_info "Successfully received response!"
        
        # For recent and appconfig endpoints, just show line count
        if [[ "$request_name" == "recent" ]] || [[ "$request_name" == "appconfig" ]]; then
            local line_count
            line_count=$(wc -l < "$response_file")
            print_info "Response contains $line_count lines"
        else
            print_info "Response content:"
            cat "$response_file"
        fi
        echo ""
        
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
                return 1
            fi
        fi
    else
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

# Function to validate line count in text response
validate_line_count() {
    local file="$1"
    local min_lines="$2"
    local actual_lines
    actual_lines=$(wc -l < "$file")
    
    if [ "$actual_lines" -gt "$min_lines" ]; then
        print_result 0 "Response contains $actual_lines lines (more than $min_lines required)"
        return 0
    else
        print_result 1 "Response contains only $actual_lines lines ($min_lines required)"
        return 1
    fi
}

# Function to add a test result to our arrays
add_test_result() {
    TEST_NAMES+=("$1")
    TEST_RESULTS+=("$2")
    TEST_DETAILS+=("$3")
}

# Function to test health endpoint
test_health_endpoint() {
    local base_url="$1"
    
    print_header "Testing /api/system/health Endpoint"
    print_header "Test Case: Health Check"
    
    validate_request "health" "curl -s --max-time 5 ${base_url}/api/system/health" "Yes, I'm alive, thanks!"
    TEST_HEALTH_RESULT=$?
    echo ""
}

# Function to test config endpoint
test_config_endpoint() {
    local base_url="$1"
    
    print_header "Testing /api/system/config Endpoint"
    print_header "Test Case: Configuration Retrieval"
    
    validate_request "config" "curl -s --max-time 5 -H 'Accept-Encoding: br' ${base_url}/api/system/config" "ServerName"
    TEST_CONFIG_RESULT=$?
    
    # Also validate that it's valid JSON and matches our test config
    if [ $TEST_CONFIG_RESULT -eq 0 ]; then
        validate_json "response_config.json"
        TEST_CONFIG_JSON_RESULT=$?
    else
        TEST_CONFIG_JSON_RESULT=1
    fi
    echo ""
}

# Function to test info endpoint
test_info_endpoint() {
    local base_url="$1"
    
    print_header "Testing /api/system/info Endpoint"
    print_header "Test Case: System Information"
    
    validate_request "info" "curl -s --max-time 5 ${base_url}/api/system/info" "system"
    TEST_INFO_RESULT=$?
    
    # Also validate that it's valid JSON
    if [ $TEST_INFO_RESULT -eq 0 ]; then
        validate_json "response_info.json"
        TEST_INFO_JSON_RESULT=$?
    else
        TEST_INFO_JSON_RESULT=1
    fi
    echo ""
}

# Function to test prometheus endpoint
test_prometheus_endpoint() {
    local base_url="$1"
    
    print_header "Testing /api/system/prometheus Endpoint"
    print_header "Test Case: Prometheus Metrics"
    
    # Test basic content presence first
    validate_request "prometheus" "curl -s -i --max-time 5 ${base_url}/api/system/prometheus" "# HELP"
    TEST_PROMETHEUS_RESULT=$?
    
    if [ $TEST_PROMETHEUS_RESULT -eq 0 ]; then
        print_info "Testing Prometheus format and metrics..."
        
        # Check content type
        if ! grep -q "Content-Type: text/plain" response_prometheus.txt; then
            print_result 1 "Response has incorrect content type"
            TEST_PROMETHEUS_FORMAT=1
        else
            # Check for required Prometheus format elements
            local prometheus_format_ok=1
            
            # Check for TYPE definitions
            if ! grep -q "^# TYPE" response_prometheus.txt; then
                print_result 1 "Response missing TYPE definitions"
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
                if ! grep -q "^$metric" response_prometheus.txt; then
                    print_result 1 "Response missing required metric: $metric"
                    prometheus_format_ok=0
                fi
            done
            
            if [ $prometheus_format_ok -eq 1 ]; then
                print_result 0 "Response contains valid Prometheus format"
                TEST_PROMETHEUS_FORMAT=0
            else
                TEST_PROMETHEUS_FORMAT=1
            fi
        fi
    else
        TEST_PROMETHEUS_FORMAT=1
    fi
    echo ""
}

# Function to test system test endpoint
test_system_test_endpoint() {
    local base_url="$1"
    
    print_header "Testing /api/system/test Endpoint"
    
    # Test Case 1: Basic GET request
    print_header "Test Case 1: Basic GET Request"
    validate_request "basic_get" "curl -s --max-time 5 ${base_url}/api/system/test" "client_ip"
    TEST_BASIC_GET_RESULT=$?
    echo ""
    
    # Test Case 2: GET request with query parameters
    print_header "Test Case 2: GET Request with Query Parameters"
    validate_request "get_with_params" "curl -s --max-time 5 '${base_url}/api/system/test?param1=value1&param2=value2'" "param1"
    TEST_GET_PARAMS_RESULT=$?
    echo ""
    
    # Test Case 3: POST request with form data
    print_header "Test Case 3: POST Request with Form Data"
    validate_request "post_form" "curl -s --max-time 5 -X POST -H 'Content-Type: application/x-www-form-urlencoded' -d 'field1=value1&field2=value2' ${base_url}/api/system/test" "post_data"
    TEST_POST_FORM_RESULT=$?
    echo ""
    
    # Test Case 4: POST request with both query parameters and form data
    print_header "Test Case 4: POST Request with Query Parameters and Form Data"
    validate_request "post_with_params" "curl -s --max-time 5 -X POST -H 'Content-Type: application/x-www-form-urlencoded' -d 'field1=value1&field2=value2' '${base_url}/api/system/test?param1=value1&param2=value2'" "param1"
    TEST_POST_PARAMS_RESULT=$?
    echo ""
}

# Function to test recent endpoint
test_recent_endpoint() {
    local base_url="$1"
    
    print_header "Testing /api/system/recent Endpoint"
    print_header "Test Case: Recent Activity Log"
    
    validate_request "recent" "curl -s --max-time 5 ${base_url}/api/system/recent" "["
    TEST_RECENT_RESULT=$?
    
    if [ $TEST_RECENT_RESULT -eq 0 ]; then
        # Validate line count
        validate_line_count "response_recent.txt" 100
        TEST_RECENT_LINES=$?
    else
        TEST_RECENT_LINES=1
    fi
    echo ""
}

# Function to test appconfig endpoint
test_appconfig_endpoint() {
    local base_url="$1"
    
    print_header "Testing /api/system/appconfig Endpoint"
    print_header "Test Case: Application Configuration"
    
    validate_request "appconfig" "curl -s --max-time 5 ${base_url}/api/system/appconfig" "APPCONFIG"
    TEST_APPCONFIG_RESULT=$?
    
    if [ $TEST_APPCONFIG_RESULT -eq 0 ]; then
        # Validate line count
        validate_line_count "response_appconfig.txt" 100
        TEST_APPCONFIG_LINES=$?
    else
        TEST_APPCONFIG_LINES=1
    fi
    echo ""
}

# Function to run all test cases
run_tests() {
    # Base URL with dynamic port
    local base_url="http://localhost:${WEB_SERVER_PORT}"
    print_info "Using base URL: $base_url for all tests"
    
    # Run all endpoint tests (pass/fail counting is handled in individual test functions)
    test_health_endpoint "$base_url"
    test_config_endpoint "$base_url"
    test_info_endpoint "$base_url"
    test_prometheus_endpoint "$base_url"
    test_system_test_endpoint "$base_url"
    test_recent_endpoint "$base_url"
    test_appconfig_endpoint "$base_url"
    
    # Check server stability
    if kill -0 "$HYDROGEN_PID" 2>/dev/null; then
        TEST_STABILITY_RESULT=0
    else
        TEST_STABILITY_RESULT=1
    fi
    
    # Return 0 if server is stable, 1 otherwise
    return $TEST_STABILITY_RESULT
}

# Function to collect test results from the run_tests function
collect_test_results() {
    # Health endpoint test
    add_test_result "Health Endpoint" "${TEST_HEALTH_RESULT}" "GET /api/system/health - Health check response"
    
    # Config endpoint tests
    add_test_result "Config Endpoint - Content" "${TEST_CONFIG_RESULT}" "GET /api/system/config - Configuration retrieval"
    add_test_result "Config Endpoint - JSON Format" "${TEST_CONFIG_JSON_RESULT}" "GET /api/system/config - Valid JSON format"
    
    # Info endpoint tests
    add_test_result "Info Endpoint - Content" "${TEST_INFO_RESULT}" "GET /api/system/info - System information presence"
    add_test_result "Info Endpoint - JSON Format" "${TEST_INFO_JSON_RESULT}" "GET /api/system/info - Valid JSON format"
    
    # Prometheus endpoint tests
    add_test_result "Prometheus Endpoint - Content" "${TEST_PROMETHEUS_RESULT}" "GET /api/system/prometheus - Metrics presence"
    add_test_result "Prometheus Endpoint - Format" "${TEST_PROMETHEUS_FORMAT}" "GET /api/system/prometheus - Valid Prometheus format"
    
    # Test endpoint tests
    add_test_result "Test Endpoint - Basic GET" "${TEST_BASIC_GET_RESULT}" "GET /api/system/test - Basic request"
    add_test_result "Test Endpoint - GET with Parameters" "${TEST_GET_PARAMS_RESULT}" "GET /api/system/test?param1=value1&param2=value2 - Query parameters"
    add_test_result "Test Endpoint - POST with Form Data" "${TEST_POST_FORM_RESULT}" "POST /api/system/test - Form data handling"
    add_test_result "Test Endpoint - POST with Parameters" "${TEST_POST_PARAMS_RESULT}" "POST /api/system/test?param1=value1&param2=value2 - Combined parameters"
    
    # Recent endpoint tests
    add_test_result "Recent Endpoint - Content" "${TEST_RECENT_RESULT}" "GET /api/system/recent - Activity log presence"
    add_test_result "Recent Endpoint - Line Count" "${TEST_RECENT_LINES}" "GET /api/system/recent - More than 100 lines"
    
    # Appconfig endpoint tests
    add_test_result "Appconfig Endpoint - Content" "${TEST_APPCONFIG_RESULT}" "GET /api/system/appconfig - Configuration presence"
    add_test_result "Appconfig Endpoint - Line Count" "${TEST_APPCONFIG_LINES}" "GET /api/system/appconfig - More than 100 lines"
    
    # System stability test
    add_test_result "System Stability" "${TEST_STABILITY_RESULT}" "No crashes detected after running all tests"
}

# Function to check server logs and add compression test results
check_server_logs() {
    local timestamp="$1"
    
    # Check server logs for API-related errors - filter irrelevant messages
    print_info "Checking server logs for API-related errors..."
    grep -i "error\|warn\|fatal\|segmentation" "$SCRIPT_DIR/hydrogen_test.log" | grep -i "API\|System\|SystemTest\|SystemService\|Endpoint\|api" > "$RESULTS_DIR/system_test_errors_${timestamp}.log"
    
    if [ -s "$RESULTS_DIR/system_test_errors_${timestamp}.log" ]; then
        print_warning "API-related warning/error messages found in logs ($(convert_to_relative_path "$RESULTS_DIR/system_test_errors_${timestamp}.log")):"
        cat "$RESULTS_DIR/system_test_errors_${timestamp}.log"
    else
        print_info "No API-related error messages found in logs"
    fi

    # Check for Brotli compression logs
    print_info "Checking for Brotli compression logs..."
    grep -i "Brotli" "$SCRIPT_DIR/hydrogen_test.log" > "$RESULTS_DIR/brotli_compression_${timestamp}.log"
    if [ -s "$RESULTS_DIR/brotli_compression_${timestamp}.log" ]; then
        print_info "Brotli compression logs found ($(convert_to_relative_path "$RESULTS_DIR/brotli_compression_${timestamp}.log")):"
        cat "$RESULTS_DIR/brotli_compression_${timestamp}.log"
        # Check for compression metrics with level information
        if grep -q "Brotli(level=[0-9]\+).*bytes.*ratio.*compression.*time:" "$RESULTS_DIR/brotli_compression_${timestamp}.log"; then
            print_result 0 "Compression logs contain detailed metrics with compression level"
            add_test_result "Brotli Compression" 0 "Compression logs found with performance metrics and compression level"
        else
            print_warning "Compression logs found but missing metrics details"
            add_test_result "Brotli Compression" 1 "Compression logs missing metrics"
        fi
    else
        print_warning "No Brotli compression logs found"
        add_test_result "Brotli Compression" 1 "No compression logs found"
    fi
}

# Function to stop the hydrogen server
stop_hydrogen_server() {
    local pid="$1"
    
    print_info "Stopping server..."
    kill "$pid"
    sleep 0.5
    
    # Make sure it's really stopped
    if kill -0 "$pid" 2>/dev/null; then
        print_warning "Server didn't stop gracefully, forcing termination..."
        kill -9 "$pid"
    fi
    wait "$pid" 2>/dev/null
}

# Main execution starts here

# Start the test
start_test "Hydrogen System API Endpoints Test"

# Configuration file for API testing
CONFIG_FILE=$(get_config_path "hydrogen_test_system_endpoints.json")

# Extract the WebServer port from the configuration file
WEB_SERVER_PORT=$(extract_web_server_port "$CONFIG_FILE")
print_info "Using WebServer port: $WEB_SERVER_PORT from configuration"

# Determine which hydrogen build to use (prefer release build if available)
PROJECT_ROOT="$(cd "$(dirname "$0")/.." && pwd)" || exit 1
if [ -f "$PROJECT_ROOT/hydrogen_release" ]; then
    HYDROGEN_BIN="$PROJECT_ROOT/hydrogen_release"
    print_info "Using release build for testing"
else
    HYDROGEN_BIN="$PROJECT_ROOT/hydrogen"
    print_info "Release build not found, using standard build"
fi

# Start hydrogen server in background with appropriate configuration
print_info "Starting hydrogen server with API test configuration ($(convert_to_relative_path "$CONFIG_FILE"))..."
"$HYDROGEN_BIN" "$CONFIG_FILE" > "$SCRIPT_DIR/hydrogen_test.log" 2>&1 &
HYDROGEN_PID=$!

# Wait for the server to start
print_info "Waiting for server to initialize (2 seconds)..."
sleep 2

# Create results directory if it doesn't exist
RESULTS_DIR="$SCRIPT_DIR/results"
mkdir -p "$RESULTS_DIR"

# Store the timestamp for this test run
TIMESTAMP=$(date +%Y%m%d_%H%M%S)

# Initialize test result variables to ensure they're always defined
TEST_HEALTH_RESULT=1
TEST_INFO_RESULT=1
TEST_INFO_JSON_RESULT=1
TEST_PROMETHEUS_RESULT=1
TEST_PROMETHEUS_FORMAT=1
TEST_BASIC_GET_RESULT=1
TEST_GET_PARAMS_RESULT=1
TEST_POST_FORM_RESULT=1
TEST_POST_PARAMS_RESULT=1
TEST_STABILITY_RESULT=1
TEST_RECENT_RESULT=1
TEST_RECENT_LINES=1
TEST_APPCONFIG_RESULT=1
TEST_APPCONFIG_LINES=1
TEST_CONFIG_RESULT=1
TEST_CONFIG_JSON_RESULT=1

# Collect all test results for summary
declare -a TEST_NAMES
declare -a TEST_RESULTS
declare -a TEST_DETAILS

# Run the tests
run_tests
TEST_RESULT=$?

# Wait to check for delayed crashes
print_info "Waiting 3 seconds to check for delayed crashes..."
sleep 3

# Check if the server is still running after waiting
if kill -0 "$HYDROGEN_PID" 2>/dev/null; then
    print_result 0 "Server is still running (no crash detected)"
    
    # Check server logs and add compression test results
    check_server_logs "$TIMESTAMP"
    
    # Stop the server
    stop_hydrogen_server "$HYDROGEN_PID"
else
    print_result 1 "ERROR: Server has crashed (segmentation fault)"
    print_info "Server logs:"
    tail -n 30 "$SCRIPT_DIR/hydrogen_test.log"
    TEST_RESULT=1
    TEST_STABILITY_RESULT=1
fi

# Collect test results 
collect_test_results

# Calculate pass/fail counts
PASS_COUNT=0
FAIL_COUNT=0
for result in "${TEST_RESULTS[@]}"; do
    if [ "$result" -eq 0 ]; then
        ((PASS_COUNT++))
    else
        ((FAIL_COUNT++))
    fi
done

# Print individual test results
echo -e "\n${BLUE}${BOLD}Individual Test Results:${NC}"
for i in "${!TEST_NAMES[@]}"; do
    print_test_item "${TEST_RESULTS[$i]}" "${TEST_NAMES[$i]}" "${TEST_DETAILS[$i]}"
done

# Print summary statistics
print_test_summary $((PASS_COUNT + FAIL_COUNT)) $PASS_COUNT $FAIL_COUNT

# Get test name from script name
TEST_NAME=$(basename "$0" .sh)
TEST_NAME="${TEST_NAME#test_}"

# Export subtest results for test_all.sh to pick up
TOTAL_SUBTESTS=$((PASS_COUNT + FAIL_COUNT))
export_subtest_results "$TEST_NAME" $TOTAL_SUBTESTS $PASS_COUNT

# Log subtest results
print_info "System API Endpoints Test: $PASS_COUNT of $TOTAL_SUBTESTS subtests passed"

# End the test with final result
end_test $TEST_RESULT "System API Endpoints Test"

# Clean up
rm -f response_*.json response_*.txt

exit $TEST_RESULT
