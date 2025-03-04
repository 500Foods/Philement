#!/bin/bash
#
# Swagger UI Test
# Tests the Swagger UI functionality with different prefixes and trailing slashes:
# - Default "/swagger" prefix using hydrogen_test_max.json
# - Custom "/apidocs" prefix using hydrogen_test_swagger.json
# - Tests both with and without trailing slashes

# Get the directory where this script is located
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

# Include the common test utilities
source "$SCRIPT_DIR/support_utils.sh"

# Create results directory if it doesn't exist
RESULTS_DIR="$SCRIPT_DIR/results"
mkdir -p "$RESULTS_DIR"

# Function to check if an HTTP response contains expected content
check_response() {
    local url="$1"
    local expected_content="$2"
    local response_file="$3"
    local follow_redirects="$4"
    
    local curl_cmd="curl -s --max-time 5"
    if [ "$follow_redirects" = "true" ]; then
        curl_cmd="$curl_cmd -L"
    fi
    
    print_command "$curl_cmd $url"
    eval "$curl_cmd $url > $response_file"
    CURL_STATUS=$?
    
    if [ $CURL_STATUS -eq 0 ]; then
        print_info "Successfully received response from $url"
        
        # Check content (show limited output)
        print_info "Response excerpt (first 5 lines):"
        head -n 5 "$response_file"
        if [ $(wc -l < "$response_file") -gt 5 ]; then
            echo "..."
        fi
        
        # Check content
        if grep -q "$expected_content" "$response_file"; then
            print_result 0 "Response contains expected content: $expected_content"
            return 0
        else
            print_result 1 "Response doesn't contain expected content: $expected_content"
            return 1
        fi
    else
        print_result 1 "Failed to connect to server at $url (curl status: $CURL_STATUS)"
        return $CURL_STATUS
    fi
}

# Function to check if an HTTP redirect is returned
check_redirect() {
    local url="$1"
    local expected_location="$2"
    local redirect_file="$3"
    
    print_command "curl -v -s --max-time 5 -o /dev/null $url"
    curl -v -s --max-time 5 -o /dev/null "$url" 2> "$redirect_file"
    CURL_STATUS=$?
    
    if [ $CURL_STATUS -eq 0 ]; then
        print_info "Successfully received response from $url"
        
        # Check if it's a redirect (show limited output)
        print_info "Response header excerpt (relevant parts):"
        grep -E "< HTTP/|< Location:" "$redirect_file"
        
        # Check if it's a redirect
        if grep -q "< HTTP/1.1 301" "$redirect_file" && grep -q "< Location: $expected_location" "$redirect_file"; then
            print_result 0 "Response is a 301 redirect to $expected_location"
            return 0
        else
            print_result 1 "Response is not a redirect to $expected_location"
            return 1
        fi
    else
        print_result 1 "Failed to connect to server at $url (curl status: $CURL_STATUS)"
        return $CURL_STATUS
    fi
}

# Function to test Swagger UI with a specific configuration
test_swagger_ui() {
    local config_file="$1"
    local swagger_prefix="$2"
    local test_name="$3"
    
    print_header "Testing Swagger UI: $swagger_prefix (using $test_name)"
    
    # Extract the WebServer port from the configuration file
    local web_server_port=$(extract_web_server_port "$config_file")
    print_info "Using WebServer port: $web_server_port from configuration"
    
    # Determine the project root directory without changing the current directory
    local PROJECT_ROOT="$(cd $(dirname $0)/.. && pwd)"
    local RELATIVE_ROOT="$(convert_to_relative_path "$PROJECT_ROOT")"
    
    # Find the correct hydrogen binary - prefer release build
    local HYDROGEN_BIN=""
    
    # First check for binaries in the project root
    if [ -f "$PROJECT_ROOT/hydrogen_release" ]; then
        HYDROGEN_BIN="$PROJECT_ROOT/hydrogen_release"
        print_info "Using release build for testing"
    elif [ -f "$PROJECT_ROOT/hydrogen" ]; then
        HYDROGEN_BIN="$PROJECT_ROOT/hydrogen"
        print_info "Using standard build"
    elif [ -f "$PROJECT_ROOT/hydrogen_debug" ]; then
        HYDROGEN_BIN="$PROJECT_ROOT/hydrogen_debug"
        print_info "Using debug build for testing"
    elif [ -f "$PROJECT_ROOT/hydrogen_perf" ]; then
        HYDROGEN_BIN="$PROJECT_ROOT/hydrogen_perf"
        print_info "Using performance build for testing"
    elif [ -f "$PROJECT_ROOT/hydrogen_valgrind" ]; then
        HYDROGEN_BIN="$PROJECT_ROOT/hydrogen_valgrind"
        print_info "Using valgrind build for testing"
    else
        print_result 1 "ERROR: Could not find any hydrogen executable variant"
        print_info "Working directory: $RELATIVE_ROOT"
        print_info "Available files:"
        ls -la "$PROJECT_ROOT" | grep hydrogen
        return 1
    fi

    # Start hydrogen server in background with appropriate configuration
    print_info "Starting hydrogen server with configuration ($(convert_to_relative_path "$config_file"))..."
    # Run from the current directory (tests/) to ensure all output stays in tests/ directory
    "$HYDROGEN_BIN" "$config_file" > "$RESULTS_DIR/hydrogen_test_${test_name}.log" 2>&1 &
    HYDROGEN_PID=$!

    # Wait for the server to start
    print_info "Waiting for server to initialize (10 seconds)..."
    sleep 10

    # Base URL for all requests
    local base_url="http://localhost:${web_server_port}"
    print_info "Using base URL: $base_url for tests"
    
    # Test 1: Path with trailing slash should serve index.html
    print_header "Test 1: Access Swagger UI with trailing slash"
    check_response "${base_url}${swagger_prefix}/" "swagger-ui" "$RESULTS_DIR/${test_name}_response_trailing_slash.html" "true"
    TRAILING_SLASH_RESULT=$?
    
    # Test 2: Path without trailing slash should redirect to trailing slash
    print_header "Test 2: Access Swagger UI without trailing slash (should redirect)"
    check_redirect "${base_url}${swagger_prefix}" "${swagger_prefix}/" "$RESULTS_DIR/${test_name}_redirect_headers.txt"
    NO_TRAILING_SLASH_RESULT=$?
    
    # Test 3: Check main page content
    print_header "Test 3: Verify Swagger UI content loads correctly"
    check_response "${base_url}${swagger_prefix}/" "swagger-ui" "$RESULTS_DIR/${test_name}_response_content.html" "true"
    CONTENT_CHECK_RESULT=$?
    
    # Test 4: Check for JavaScript files
    print_header "Test 4: Verify JavaScript files load correctly"
    check_response "${base_url}${swagger_prefix}/swagger-initializer.js" "window.ui = SwaggerUIBundle" "$RESULTS_DIR/${test_name}_initializer.js" "true"
    JS_CHECK_RESULT=$?
    
    # Check the actual PID from ps as $HYDROGEN_PID might not be reliable
    ACTUAL_PID=$(pgrep -f ".*hydrogen.*$(basename "$config_file")")
    
    # Check server stability
    if [ -n "$ACTUAL_PID" ]; then
        STABILITY_RESULT=0
        # Stop the server
        print_info "Stopping server (PID: $ACTUAL_PID)..."
        kill $ACTUAL_PID
        sleep 2
        
        # Make sure it's really stopped
        if kill -0 $ACTUAL_PID 2>/dev/null; then
            print_warning "Server didn't stop gracefully, forcing termination..."
            kill -9 $ACTUAL_PID
            sleep 1
        fi
    else
        print_result 1 "ERROR: Server has crashed (segmentation fault)"
        print_info "Server logs:"
        tail -n 30 "$RESULTS_DIR/hydrogen_test_${test_name}.log"
        STABILITY_RESULT=1
    fi
    
    # Calculate overall result
    if [ $TRAILING_SLASH_RESULT -eq 0 ] && [ $NO_TRAILING_SLASH_RESULT -eq 0 ] && 
       [ $CONTENT_CHECK_RESULT -eq 0 ] && [ $JS_CHECK_RESULT -eq 0 ] && 
       [ $STABILITY_RESULT -eq 0 ]; then
        print_result 0 "All tests for Swagger UI prefix '$swagger_prefix' passed successfully"
        return 0
    else
        print_result 1 "Some tests for Swagger UI prefix '$swagger_prefix' failed"
        return 1
    fi
}

# Start the test
start_test "Hydrogen Swagger UI Test"

# Ensure clean state
print_info "Ensuring no existing hydrogen processes are running..."
pkill -f "hydrogen.*json" 2>/dev/null
sleep 2

# Test with default Swagger prefix (/swagger) on a dedicated port
test_swagger_ui "$(get_config_path "hydrogen_test_swagger_test_1.json")" "/swagger" "swagger_default"
DEFAULT_PREFIX_RESULT=$?

# Ensure server is stopped before starting next test
print_info "Making sure all previous servers are stopped..."
pkill -f "hydrogen.*json" 2>/dev/null
sleep 2

# Test with custom Swagger prefix (/apidocs) on a different port
test_swagger_ui "$(get_config_path "hydrogen_test_swagger_test_2.json")" "/apidocs" "swagger_custom"
CUSTOM_PREFIX_RESULT=$?

# Calculate overall test result
if [ $DEFAULT_PREFIX_RESULT -eq 0 ] && [ $CUSTOM_PREFIX_RESULT -eq 0 ]; then
    TEST_RESULT=0
    print_result 0 "All Swagger UI tests passed successfully"
else
    TEST_RESULT=1
    print_result 1 "Some Swagger UI tests failed"
fi

# Ensure all servers are stopped at the end
print_info "Cleaning up any remaining server processes..."
pkill -f "hydrogen.*json" 2>/dev/null

# End the test with final result
end_test $TEST_RESULT "Swagger UI Test"

# Clean up response files
rm -f $RESULTS_DIR/*_response_*.html $RESULTS_DIR/*_redirect_*.txt $RESULTS_DIR/*_initializer.js $RESULTS_DIR/hydrogen_test_*.log

exit $TEST_RESULT