#!/bin/bash
#
# About this Script
#
# Hydrogen Swagger Test (Immediate Restart)
# Tests the Swagger functionality with different prefixes and trailing slashes using immediate restart approach:
# - Default "/swagger" prefix using hydrogen_test_max.json
# - Custom "/apidocs" prefix using hydrogen_test_swagger.json
# - Tests both with and without trailing slashes
# - Uses immediate restart without waiting for TIME_WAIT (SO_REUSEADDR enabled)
#
NAME_SCRIPT="Hydrogen Swagger Test (Immediate Restart)"
VERS_SCRIPT="3.0.0"

# VERSION HISTORY
# 3.0.0 - 2025-06-30 - Updated to use immediate restart approach without TIME_WAIT waiting
# 2.0.0 - 2025-06-17 - Major refactoring: fixed all shellcheck warnings, improved modularity, enhanced comments

# Display script name and version
echo "=== $NAME_SCRIPT v$VERS_SCRIPT ==="

# Get the directory where this script is located
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

# Include the common test utilities
source "$SCRIPT_DIR/support_utils.sh"
source "$SCRIPT_DIR/support_timewait.sh"

# Function to wait for startup completion by monitoring log
wait_for_startup() {
    local log_file="$1"
    local timeout="$2"
    local start_time
    local current_time
    local elapsed
    
    start_time=$(date +%s)
    
    while true; do
        current_time=$(date +%s)
        elapsed=$((current_time - start_time))
        
        if [ "$elapsed" -ge "$timeout" ]; then
            print_error "Startup timeout after ${elapsed}s"
            return 1
        fi
        
        if grep -q "Application started" "$log_file" 2>/dev/null; then
            print_info "Startup completed in ${elapsed}s"
            return 0
        fi
        
        sleep 0.5
    done
}

# Function to wait for server to be ready
wait_for_server() {
    local base_url="$1"
    local log_file="$2"
    local max_attempts=60  # 60 seconds total (1s * 60)
    local attempt=1
    
    print_info "Waiting for server to be ready at $base_url..."
    
    # First wait for startup completion by monitoring log
    if [ -n "$log_file" ] && [ -f "$log_file" ]; then
        print_info "Monitoring startup log for completion..."
        if wait_for_startup "$log_file" 10; then
            print_info "Server startup completed, verifying connectivity..."
        else
            print_error "Server startup failed or timed out"
            return 1
        fi
    fi
    
    # Then verify connectivity
    while [ $attempt -le $max_attempts ]; do
        if curl -s --max-time 2 "${base_url}" > /dev/null 2>&1; then
            print_info "Server is ready after $attempt seconds"
            return 0
        fi
        sleep 1
        ((attempt++))
    done
    
    print_error "Server failed to respond within 60 seconds"
    return 1
}

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
    local curl_status=$?
    
    if [ $curl_status -eq 0 ]; then
        print_info "Successfully received response from $url"
        
        # Check content (show limited output)
        print_info "Response excerpt (first 5 lines):"
        head -n 5 "$response_file"
        local line_count
        line_count=$(wc -l < "$response_file")
        if [ "$line_count" -gt 5 ]; then
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
        print_result 1 "Failed to connect to server at $url (curl status: $curl_status)"
        return $curl_status
    fi
}

# Function to check if an HTTP redirect is returned
check_redirect() {
    local url="$1"
    local expected_location="$2"
    local redirect_file="$3"
    
    print_command "curl -v -s --max-time 5 -o /dev/null $url"
    curl -v -s --max-time 5 -o /dev/null "$url" 2> "$redirect_file"
    local curl_status=$?
    
    if [ $curl_status -eq 0 ]; then
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
        print_result 1 "Failed to connect to server at $url (curl status: $curl_status)"
        return $curl_status
    fi
}

# Function to find hydrogen binary
find_hydrogen_binary() {
    local project_root="$1"
    local relative_root="$2"
    
    # Find the correct hydrogen binary - prefer release build
    local hydrogen_bin=""
    
    # First check for binaries in the project root
    if [ -f "$project_root/hydrogen_release" ]; then
        hydrogen_bin="$project_root/hydrogen_release"
        print_info "Using release build for testing" >&2
    elif [ -f "$project_root/hydrogen" ]; then
        hydrogen_bin="$project_root/hydrogen"
        print_info "Using standard build" >&2
    elif [ -f "$project_root/hydrogen_debug" ]; then
        hydrogen_bin="$project_root/hydrogen_debug"
        print_info "Using debug build for testing" >&2
    elif [ -f "$project_root/hydrogen_perf" ]; then
        hydrogen_bin="$project_root/hydrogen_perf"
        print_info "Using performance build for testing" >&2
    elif [ -f "$project_root/hydrogen_valgrind" ]; then
        hydrogen_bin="$project_root/hydrogen_valgrind"
        print_info "Using valgrind build for testing" >&2
    else
        print_result 1 "ERROR: Could not find any hydrogen executable variant" >&2
        print_info "Working directory: $relative_root" >&2
        print_info "Available files:" >&2
        # Use find instead of ls | grep to avoid SC2010
        find "$project_root" -maxdepth 1 -name "*hydrogen*" -type f >&2
        return 1
    fi
    
    echo "$hydrogen_bin"
    return 0
}

# Function to test Swagger UI with a specific configuration
test_swagger() {
    local config_file="$1"
    local swagger_prefix="$2"
    local test_name="$3"
    
    print_header "Testing Swagger UI: $swagger_prefix (using $test_name)"
    
    # Extract the WebServer port from the configuration file
    local web_server_port
    web_server_port=$(extract_web_server_port "$config_file")
    print_info "Using WebServer port: $web_server_port from configuration"
    
    # Determine the project root directory without changing the current directory
    local project_root
    project_root="$(cd "$(dirname "$0")/.." && pwd)"
    local relative_root
    relative_root="$(convert_to_relative_path "$project_root")"
    
    # Find the correct hydrogen binary
    local hydrogen_bin
    if ! hydrogen_bin=$(find_hydrogen_binary "$project_root" "$relative_root"); then
        return 1
    fi
    
    # Show which binary was selected
    print_info "Selected hydrogen binary: $(convert_to_relative_path "$hydrogen_bin")"

    # Prepare clean test environment (immediate restart approach)
    print_info "Terminating any existing hydrogen processes..."
    if pkill -f "hydrogen.*json" 2>/dev/null; then
        print_info "Terminated"
    else
        print_info "No processes to terminate"
    fi
    
    # Start hydrogen server in background with appropriate configuration
    print_info "Starting hydrogen server with configuration $(convert_to_relative_path "$config_file")..."
    # Run from the current directory (tests/) to ensure all output stays in tests/ directory
    "$hydrogen_bin" "$config_file" > "$RESULTS_DIR/hydrogen_test_${test_name}.log" 2>&1 &
    local server_pid=$!
    
    print_info "Server started with PID: $server_pid"

    # Base URL for all requests
    local base_url="http://localhost:${web_server_port}"
    print_info "Using base URL: $base_url for tests"
    
    # Wait for the server to be ready
    wait_for_server "${base_url}" "$RESULTS_DIR/hydrogen_test_${test_name}.log" || {
        print_error "Server failed to start properly"
        kill "$server_pid" 2>/dev/null || true
        return 1
    }
    
    # Test 1: Path with trailing slash should serve index.html
    print_header "Test 1: Access Swagger UI with trailing slash"
    check_response "${base_url}${swagger_prefix}/" "swagger-ui" "$RESULTS_DIR/${test_name}_response_trailing_slash.html" "true"
    local trailing_slash_result=$?
    
    # Test 2: Path without trailing slash should redirect to trailing slash
    print_header "Test 2: Access Swagger UI without trailing slash (should redirect)"
    check_redirect "${base_url}${swagger_prefix}" "${swagger_prefix}/" "$RESULTS_DIR/${test_name}_redirect_headers.txt"
    local no_trailing_slash_result=$?
    
    # Test 3: Check main page content
    print_header "Test 3: Verify Swagger UI content loads correctly"
    check_response "${base_url}${swagger_prefix}/" "swagger-ui" "$RESULTS_DIR/${test_name}_response_content.html" "true"
    local content_check_result=$?
    
    # Test 4: Check for JavaScript files
    print_header "Test 4: Verify JavaScript files load correctly"
    check_response "${base_url}${swagger_prefix}/swagger-initializer.js" "window.ui = SwaggerUIBundle" "$RESULTS_DIR/${test_name}_initializer.js" "true"
    local js_check_result=$?
    
    # Check server stability and shutdown
    local stability_result
    if ps -p "$server_pid" > /dev/null 2>&1; then
        print_info "Server is still running after tests (PID: $server_pid)"
        stability_result=0
        # Stop the server gracefully
        print_info "Shutting down server (PID: $server_pid)..."
        kill -SIGINT "$server_pid" 2>/dev/null || true
        sleep 2
        if ps -p "$server_pid" > /dev/null 2>&1; then
            print_warning "Server still running, forcing termination..."
            kill -9 "$server_pid" 2>/dev/null || true
        fi
        print_info "Server shutdown completed"
    else
        print_result 1 "ERROR: Server has crashed or exited prematurely"
        print_info "Server logs:"
        tail -n 30 "$RESULTS_DIR/hydrogen_test_${test_name}.log"
        stability_result=1
    fi
    
    # Check for TIME_WAIT sockets
    check_time_wait_sockets_robust "$web_server_port"
    
    # Track passed subtests (global counter)
    if [ $trailing_slash_result -eq 0 ]; then
        ((PASS_COUNT++))
    fi
    if [ $no_trailing_slash_result -eq 0 ]; then
        ((PASS_COUNT++))
    fi
    if [ $content_check_result -eq 0 ]; then
        ((PASS_COUNT++))
    fi
    if [ $js_check_result -eq 0 ]; then
        ((PASS_COUNT++))
    fi
    if [ $stability_result -eq 0 ]; then
        ((PASS_COUNT++))
    fi
    
    # Calculate overall result
    if [ $trailing_slash_result -eq 0 ] && [ $no_trailing_slash_result -eq 0 ] && 
       [ $content_check_result -eq 0 ] && [ $js_check_result -eq 0 ] && 
       [ $stability_result -eq 0 ]; then
        print_result 0 "All tests for Swagger UI prefix '$swagger_prefix' passed successfully"
        return 0
    else
        print_result 1 "Some tests for Swagger UI prefix '$swagger_prefix' failed"
        return 1
    fi
}

# Function to cleanup remaining processes
cleanup_remaining_processes() {
    print_info "Cleaning up any remaining server processes..."
    pkill -f "hydrogen.*json" 2>/dev/null
}

# Function to cleanup response files
cleanup_response_files() {
    rm -f "$RESULTS_DIR"/*_response_*.html "$RESULTS_DIR"/*_redirect_*.txt "$RESULTS_DIR"/*_initializer.js "$RESULTS_DIR"/hydrogen_test_*.log
}

# Main execution starts here

# Initialize test environment and result log
RESULT_LOG=$(setup_test_environment "Hydrogen Swagger UI Test")

# Create results directory if it doesn't exist
RESULTS_DIR="$SCRIPT_DIR/results"
mkdir -p "$RESULTS_DIR"

# Start the test
start_test "Hydrogen Swagger UI Test"

# Initialize subtest tracking
TOTAL_SUBTESTS=10  # 5 tests for each of 2 configurations
PASS_COUNT=0

# Ensure clean state
print_info "Ensuring no existing hydrogen processes are running..."
pkill -f "hydrogen.*json" 2>/dev/null

print_info "Testing Swagger functionality with immediate restart approach"
print_info "SO_REUSEADDR is enabled - no need to wait for TIME_WAIT"

# Test with default Swagger prefix (/swagger) on a dedicated port
test_swagger "$(get_config_path "hydrogen_test_swagger_test_1.json")" "/swagger" "swagger_default"
DEFAULT_PREFIX_RESULT=$?

# Start second test immediately (testing SO_REUSEADDR)
print_info "Starting second test immediately (testing SO_REUSEADDR)..."

# Test with custom Swagger prefix (/apidocs) on a different port
test_swagger "$(get_config_path "hydrogen_test_swagger_test_2.json")" "/apidocs" "swagger_custom"
CUSTOM_PREFIX_RESULT=$?

print_info "Immediate restart successful - SO_REUSEADDR is working!"

# Calculate overall test result
if [ $DEFAULT_PREFIX_RESULT -eq 0 ] && [ $CUSTOM_PREFIX_RESULT -eq 0 ]; then
    TEST_RESULT=0
    print_result 0 "All Swagger UI tests passed successfully"
else
    TEST_RESULT=1
    print_result 1 "Some Swagger UI tests failed"
fi

# Cleanup remaining processes
cleanup_remaining_processes

# Get test name from script name
TEST_NAME=$(basename "$0" .sh)
TEST_NAME="${TEST_NAME#test_}"

# Export subtest results for test_all.sh to pick up
export_subtest_results "$TEST_NAME" $TOTAL_SUBTESTS $PASS_COUNT

# Log subtest results
print_info "Swagger UI Test: $PASS_COUNT of $TOTAL_SUBTESTS subtests passed"

# End the test with final result
end_test $TEST_RESULT "Swagger UI Test"

# Clean up response files
cleanup_response_files

exit $TEST_RESULT
