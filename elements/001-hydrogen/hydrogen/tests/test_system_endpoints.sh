#!/bin/bash
#
# System API Endpoints Test
# Tests all the system API endpoints:
# - /api/system/test: Tests request handling with various parameters
# - /api/system/health: Tests the health check endpoint
# - /api/system/info: Tests the system information endpoint
#

# Get the directory where this script is located
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

# Include the common test utilities
source "$SCRIPT_DIR/test_utils.sh"

# Function to make a request and validate response
validate_request() {
    local request_name="$1"
    local curl_command="$2"
    local expected_field="$3"
    local response_file="response_${request_name}.json"
    
    print_command "$curl_command"
    eval "$curl_command > $response_file"
    CURL_STATUS=$?
    
    # Check if the request was successful
    if [ $CURL_STATUS -eq 0 ]; then
        print_info "Successfully received response!"
        print_info "Response content:"
        cat "$response_file"
        echo ""
        
        # Validate that the response contains expected fields
        if grep -q "$expected_field" "$response_file"; then
            print_result 0 "Response contains expected field: $expected_field"
            return 0
        else
            print_result 1 "Response doesn't contain expected field: $expected_field"
            return 1
        fi
    else
        print_result 1 "Failed to connect to server (curl status: $CURL_STATUS)"
        return $CURL_STATUS
    fi
}

# Function to validate JSON syntax
validate_json() {
    local file="$1"
    
    if command -v jq &> /dev/null; then
        # Use jq if available for proper JSON validation
        jq . "$file" > /dev/null 2>&1
        if [ $? -eq 0 ]; then
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

# Start the test
start_test "Hydrogen System API Endpoints Test"

# Configuration file for API testing
CONFIG_FILE="$SCRIPT_DIR/hydrogen_test_api.json"

# Determine which hydrogen build to use (prefer release build if available)
cd $(dirname $0)/..
if [ -f "./hydrogen_release" ]; then
    HYDROGEN_BIN="./hydrogen_release"
    print_info "Using release build for testing"
else
    HYDROGEN_BIN="./hydrogen"
    print_info "Release build not found, using standard build"
fi

# Start hydrogen server in background with appropriate configuration
print_info "Starting hydrogen server with API test configuration..."
$HYDROGEN_BIN "$CONFIG_FILE" > "$SCRIPT_DIR/hydrogen_test.log" 2>&1 &
HYDROGEN_PID=$!

# Wait for the server to start
print_info "Waiting for server to initialize (5 seconds)..."
sleep 5

# Create results directory if it doesn't exist
RESULTS_DIR="results"
mkdir -p "$RESULTS_DIR"

# Store the timestamp for this test run
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
TEST_LOG="$RESULTS_DIR/system_test_${TIMESTAMP}.log"

# Initialize test result variables to ensure they're always defined
TEST_HEALTH_RESULT=1
TEST_INFO_RESULT=1
TEST_INFO_JSON_RESULT=1
TEST_BASIC_GET_RESULT=1
TEST_GET_PARAMS_RESULT=1
TEST_POST_FORM_RESULT=1
TEST_POST_PARAMS_RESULT=1
TEST_STABILITY_RESULT=1

# Function to run all test cases
run_tests() {
    local pass_count=0
    local fail_count=0
    
    # ====================================================================
    # PART 1: Test /api/system/health endpoint
    # ====================================================================
    print_header "Testing /api/system/health Endpoint"
    
    # Test health endpoint with GET request
    print_header "Test Case: Health Check"
    validate_request "health" "curl -s http://localhost:5000/api/system/health" "Yes, I'm alive, thanks!"
    TEST_HEALTH_RESULT=$?
    if [ $TEST_HEALTH_RESULT -eq 0 ]; then
        ((pass_count++))
    else
        ((fail_count++))
    fi
    echo ""
    
    # ====================================================================
    # PART 2: Test /api/system/info endpoint
    # ====================================================================
    print_header "Testing /api/system/info Endpoint"
    
    # Test info endpoint with GET request
    print_header "Test Case: System Information"
    validate_request "info" "curl -s http://localhost:5000/api/system/info" "system"
    TEST_INFO_RESULT=$?
    
    # Also validate that it's valid JSON
    if [ $TEST_INFO_RESULT -eq 0 ]; then
        validate_json "response_info.json"
        TEST_INFO_JSON_RESULT=$?
        if [ $TEST_INFO_JSON_RESULT -eq 0 ]; then
            ((pass_count++))
        else
            ((fail_count++))
        fi
    else
        ((fail_count++))
        TEST_INFO_JSON_RESULT=1
    fi
    echo ""
    
    # ====================================================================
    # PART 3: Test /api/system/test endpoint
    # ====================================================================
    print_header "Testing /api/system/test Endpoint"
    
    # Test Case 1: Basic GET request
    print_header "Test Case 1: Basic GET Request"
    validate_request "basic_get" "curl -s http://localhost:5000/api/system/test"  "client_ip"
    TEST_BASIC_GET_RESULT=$?
    if [ $TEST_BASIC_GET_RESULT -eq 0 ]; then
        ((pass_count++))
    else
        ((fail_count++))
    fi
    echo ""
    
    # Test Case 2: GET request with query parameters
    print_header "Test Case 2: GET Request with Query Parameters"
    validate_request "get_with_params" "curl -s 'http://localhost:5000/api/system/test?param1=value1&param2=value2'" "param1"
    TEST_GET_PARAMS_RESULT=$?
    if [ $TEST_GET_PARAMS_RESULT -eq 0 ]; then
        ((pass_count++))
    else
        ((fail_count++))
    fi
    echo ""
    
    # Test Case 3: POST request with form data
    print_header "Test Case 3: POST Request with Form Data"
    validate_request "post_form" "curl -s -X POST -H 'Content-Type: application/x-www-form-urlencoded' -d 'field1=value1&field2=value2' http://localhost:5000/api/system/test" "post_data"
    TEST_POST_FORM_RESULT=$?
    if [ $TEST_POST_FORM_RESULT -eq 0 ]; then
        ((pass_count++))
    else
        ((fail_count++))
    fi
    echo ""
    
    # Test Case 4: POST request with both query parameters and form data
    print_header "Test Case 4: POST Request with Query Parameters and Form Data"
    validate_request "post_with_params" "curl -s -X POST -H 'Content-Type: application/x-www-form-urlencoded' -d 'field1=value1&field2=value2' 'http://localhost:5000/api/system/test?param1=value1&param2=value2'" "param1"
    TEST_POST_PARAMS_RESULT=$?
    if [ $TEST_POST_PARAMS_RESULT -eq 0 ]; then
        ((pass_count++))
    else
        ((fail_count++))
    fi
    echo ""
    
    # Check server stability
    if kill -0 $HYDROGEN_PID 2>/dev/null; then
        TEST_STABILITY_RESULT=0
    else
        TEST_STABILITY_RESULT=1
        ((fail_count++))
    fi
    
    # Return 0 if all tests pass, 1 otherwise
    if [ $fail_count -eq 0 ]; then
        return 0
    else
        return 1
    fi
}

# Run the tests
run_tests
TEST_RESULT=$?

# Collect all test results for summary
declare -a TEST_NAMES
declare -a TEST_RESULTS
declare -a TEST_DETAILS

# Function to add a test result to our arrays
add_test_result() {
    TEST_NAMES+=("$1")
    TEST_RESULTS+=($2)
    TEST_DETAILS+=("$3")
}

# Collect test results from the run_tests function
collect_test_results() {
    # Health endpoint test
    add_test_result "Health Endpoint" ${TEST_HEALTH_RESULT} "GET /api/system/health - Health check response"
    
    # Info endpoint tests
    add_test_result "Info Endpoint - Content" ${TEST_INFO_RESULT} "GET /api/system/info - System information presence"
    add_test_result "Info Endpoint - JSON Format" ${TEST_INFO_JSON_RESULT} "GET /api/system/info - Valid JSON format"
    
    # Test endpoint tests
    add_test_result "Test Endpoint - Basic GET" ${TEST_BASIC_GET_RESULT} "GET /api/system/test - Basic request"
    add_test_result "Test Endpoint - GET with Parameters" ${TEST_GET_PARAMS_RESULT} "GET /api/system/test?param1=value1&param2=value2 - Query parameters"
    add_test_result "Test Endpoint - POST with Form Data" ${TEST_POST_FORM_RESULT} "POST /api/system/test - Form data handling"
    add_test_result "Test Endpoint - POST with Parameters" ${TEST_POST_PARAMS_RESULT} "POST /api/system/test?param1=value1&param2=value2 - Combined parameters"
    
    # System stability test
    add_test_result "System Stability" ${TEST_STABILITY_RESULT} "No crashes detected after running all tests"
}

# Wait to check for delayed crashes
print_info "Waiting 10 seconds to check for delayed crashes..."
sleep 10

# Check if the server is still running after waiting
if kill -0 $HYDROGEN_PID 2>/dev/null; then
    print_result 0 "Server is still running (no crash detected)"
    
    # Check server logs for API-related errors - filter irrelevant messages
    print_info "Checking server logs for API-related errors..."
    grep -i "error\|warn\|fatal\|segmentation" hydrogen_test.log | grep -i "API\|System\|SystemTest\|SystemService\|Endpoint\|api" > "$RESULTS_DIR/system_test_errors_${TIMESTAMP}.log"
    
    if [ -s "$RESULTS_DIR/system_test_errors_${TIMESTAMP}.log" ]; then
        print_warning "API-related warning/error messages found in logs:"
        cat "$RESULTS_DIR/system_test_errors_${TIMESTAMP}.log"
    else
        print_info "No API-related error messages found in logs"
    fi
    
    # Stop the server
    print_info "Stopping server..."
    kill $HYDROGEN_PID
    sleep 1
    
    # Make sure it's really stopped
    if kill -0 $HYDROGEN_PID 2>/dev/null; then
        print_warning "Server didn't stop gracefully, forcing termination..."
        kill -9 $HYDROGEN_PID
    fi
    wait $HYDROGEN_PID 2>/dev/null
else
    print_result 1 "ERROR: Server has crashed (segmentation fault)"
    print_info "Server logs:"
    tail -n 30 hydrogen_test.log
    TEST_RESULT=1
    TEST_STABILITY_RESULT=1
fi

# Collect test results 
collect_test_results

# Calculate pass/fail counts
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
echo -e "\n${BLUE}${BOLD}Individual Test Results:${NC}"
for i in "${!TEST_NAMES[@]}"; do
    print_test_item ${TEST_RESULTS[$i]} "${TEST_NAMES[$i]}" "${TEST_DETAILS[$i]}"
done

# Print summary statistics
print_test_summary $((PASS_COUNT + FAIL_COUNT)) $PASS_COUNT $FAIL_COUNT

# End the test with final result
end_test $TEST_RESULT "System API Endpoints Test"

# Clean up
rm -f response_*.json

exit $TEST_RESULT