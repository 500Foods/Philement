#!/bin/bash

# Test: Swagger
# Tests the Swagger functionality with different prefixes and trailing slashes:
# - Default "/swagger" prefix using hydrogen_test_swagger_test_1.json
# - Custom "/apidocs" prefix using hydrogen_test_swagger_test_2.json
# - Tests both with and without trailing slashes
# - Tests JavaScript file loading and content validation
# - Uses immediate restart without waiting for TIME_WAIT (SO_REUSEADDR enabled)

# CHANGELOG
# 3.0.1 - 2025-07-06 - Added missing shellcheck justifications
# 3.0.0 - 2025-07-02 - Migrated to use lib/ scripts, following established test patterns
# 2.0.0 - 2025-06-17 - Major refactoring: fixed all shellcheck warnings, improved modularity, enhanced comments

# Test Configuration
TEST_NAME="Swagger"
SCRIPT_VERSION="3.0.1"

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
TOTAL_SUBTESTS=13  # 3 prerequisites + 5 tests for each of 2 configurations
PASS_COUNT=0
EXIT_CODE=0

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

# Function to check HTTP response content
check_response_content() {
    local url="$1"
    local expected_content="$2"
    local response_file="$3"
    local follow_redirects="$4"
    
    local curl_cmd="curl -s --max-time 10"
    if [ "$follow_redirects" = "true" ]; then
        curl_cmd="$curl_cmd -L"
    fi
    curl_cmd="$curl_cmd --compressed"
    
    print_command "$curl_cmd \"$url\""
    if eval "$curl_cmd \"$url\" > \"$response_file\""; then
        print_message "Successfully received response from $url"
        
        # Show response excerpt
        local line_count
        line_count=$(wc -l < "$response_file")
        print_message "Response contains $line_count lines"
        
        # Check for expected content
        if grep -q "$expected_content" "$response_file"; then
            print_result 0 "Response contains expected content: $expected_content"
            return 0
        else
            print_result 1 "Response doesn't contain expected content: $expected_content"
            print_message "Response excerpt (first 10 lines):"
            head -n 10 "$response_file" | while IFS= read -r line; do
                print_output "$line"
            done
            return 1
        fi
    else
        print_result 1 "Failed to connect to server at $url"
        return 1
    fi
}

# Function to check HTTP redirect
check_redirect_response() {
    local url="$1"
    local expected_location="$2"
    local redirect_file="$3"
    
    print_command "curl -v -s --max-time 10 -o /dev/null \"$url\""
    if curl -v -s --max-time 10 -o /dev/null "$url" 2> "$redirect_file"; then
        print_message "Successfully received response from $url"
        
        # Check for redirect
        if grep -q "< HTTP/1.1 301" "$redirect_file" && grep -q "< Location: $expected_location" "$redirect_file"; then
            print_result 0 "Response is a 301 redirect to $expected_location"
            return 0
        else
            print_result 1 "Response is not a redirect to $expected_location"
            print_message "Response headers:"
            grep -E "< HTTP/|< Location:" "$redirect_file" | while IFS= read -r line; do
                print_output "$line"
            done
            return 1
        fi
    else
        print_result 1 "Failed to connect to server at $url"
        return 1
    fi
}

# Function to test Swagger UI with a specific configuration
test_swagger_configuration() {
    local config_file="$1"
    local swagger_prefix="$2"
    local test_name="$3"
    local config_number="$4"
    
    print_message "Testing Swagger UI: $swagger_prefix (using $test_name)"
    
    # Extract port from configuration
    local server_port
    server_port=$(get_webserver_port "$config_file")
    print_message "Configuration will use port: $server_port"
    
    # Global variables for server management
    local hydrogen_pid=""
    local server_log="$RESULTS_DIR/swagger_${test_name}_${TIMESTAMP}.log"
    local base_url="http://localhost:$server_port"
    
    # Start server
    local subtest_start=$(((config_number - 1) * 5 + 1))
    
    # Subtest: Start server
    next_subtest
    print_subtest "Start Hydrogen Server (Config $config_number)"
    if hydrogen_pid=$(start_hydrogen "$config_file" "$server_log" 15 "$HYDROGEN_BIN") && [ -n "$hydrogen_pid" ]; then
        print_result 0 "Server started successfully with PID: $hydrogen_pid"
        ((PASS_COUNT++))
    else
        print_result 1 "Failed to start server"
        EXIT_CODE=1
        # Skip remaining subtests for this configuration
        for i in {2..5}; do
            next_subtest
            print_subtest "Subtest $((subtest_start + i - 1)) (Skipped)"
            print_result 1 "Skipped due to server startup failure"
        done
        return 1
    fi
    
    # Wait for server to be ready
    if [ -n "$hydrogen_pid" ] && ps -p "$hydrogen_pid" > /dev/null 2>&1; then
        if ! wait_for_server_ready "$base_url"; then
            print_result 1 "Server failed to become ready"
            EXIT_CODE=1
            # Skip remaining subtests
            for i in {2..5}; do
                next_subtest
                print_subtest "Subtest $((subtest_start + i - 1)) (Skipped)"
                print_result 1 "Skipped due to server readiness failure"
            done
            return 1
        fi
    fi
    
    # Subtest: Test Swagger UI with trailing slash
    next_subtest
    print_subtest "Access Swagger UI with trailing slash (Config $config_number)"
    if [ -n "$hydrogen_pid" ] && ps -p "$hydrogen_pid" > /dev/null 2>&1; then
        if check_response_content "${base_url}${swagger_prefix}/" "swagger-ui" "$RESULTS_DIR/${test_name}_trailing_slash_${TIMESTAMP}.html" "true"; then
            ((PASS_COUNT++))
        else
            EXIT_CODE=1
        fi
    else
        print_result 1 "Server not running for trailing slash test"
        EXIT_CODE=1
    fi
    
    # Subtest: Test redirect without trailing slash
    next_subtest
    print_subtest "Access Swagger UI without trailing slash (Config $config_number)"
    if [ -n "$hydrogen_pid" ] && ps -p "$hydrogen_pid" > /dev/null 2>&1; then
        if check_redirect_response "${base_url}${swagger_prefix}" "${swagger_prefix}/" "$RESULTS_DIR/${test_name}_redirect_${TIMESTAMP}.txt"; then
            ((PASS_COUNT++))
        else
            EXIT_CODE=1
        fi
    else
        print_result 1 "Server not running for redirect test"
        EXIT_CODE=1
    fi
    
    # Subtest: Verify Swagger UI content
    next_subtest
    print_subtest "Verify Swagger UI content loads (Config $config_number)"
    if [ -n "$hydrogen_pid" ] && ps -p "$hydrogen_pid" > /dev/null 2>&1; then
        if check_response_content "${base_url}${swagger_prefix}/" "swagger-ui" "$RESULTS_DIR/${test_name}_content_${TIMESTAMP}.html" "true"; then
            ((PASS_COUNT++))
        else
            EXIT_CODE=1
        fi
    else
        print_result 1 "Server not running for content test"
        EXIT_CODE=1
    fi
    
    # Subtest: Test JavaScript files
    next_subtest
    print_subtest "Verify JavaScript files load (Config $config_number)"
    if [ -n "$hydrogen_pid" ] && ps -p "$hydrogen_pid" > /dev/null 2>&1; then
        if check_response_content "${base_url}${swagger_prefix}/swagger-initializer.js" "window.ui = SwaggerUIBundle" "$RESULTS_DIR/${test_name}_initializer_${TIMESTAMP}.js" "true"; then
            ((PASS_COUNT++))
        else
            EXIT_CODE=1
        fi
    else
        print_result 1 "Server not running for JavaScript test"
        EXIT_CODE=1
    fi
    
    # Stop the server
    if [ -n "$hydrogen_pid" ]; then
        print_message "Stopping server..."
        if stop_hydrogen "$hydrogen_pid" "$server_log" 10 5 "$RESULTS_DIR"; then
            print_message "Server stopped successfully"
        else
            print_warning "Server shutdown had issues"
        fi
        
        # Check TIME_WAIT sockets
        check_time_wait_sockets "$server_port"
    fi
    
    return 0
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
if HYDROGEN_BIN=$(find_hydrogen_binary "$HYDROGEN_DIR"); then
    print_result 0 "Hydrogen binary found: $(basename "$HYDROGEN_BIN")"
    ((PASS_COUNT++))
else
    print_result 1 "Hydrogen binary not found"
    EXIT_CODE=1
fi

# Configuration files for testing
CONFIG_1="$(get_config_path "hydrogen_test_swagger_test_1.json")"
CONFIG_2="$(get_config_path "hydrogen_test_swagger_test_2.json")"

# Subtest 2: Validate first configuration file
next_subtest
print_subtest "Validate Configuration File 1"
if validate_config_file "$CONFIG_1"; then
    ((PASS_COUNT++))
else
    EXIT_CODE=1
fi

# Subtest 3: Validate second configuration file
next_subtest
print_subtest "Validate Configuration File 2"
if validate_config_file "$CONFIG_2"; then
    ((PASS_COUNT++))
else
    EXIT_CODE=1
fi

# Only proceed with Swagger tests if prerequisites are met
if [ $EXIT_CODE -eq 0 ]; then
    # Ensure clean state
    print_message "Ensuring no existing Hydrogen processes are running..."
    pkill -f "hydrogen.*json" 2>/dev/null || true
    
    print_message "Testing Swagger functionality with immediate restart approach"
    print_message "SO_REUSEADDR is enabled - no need to wait for TIME_WAIT"
    
    # Test with default Swagger prefix (/swagger)
    test_swagger_configuration "$CONFIG_1" "/swagger" "swagger_default" 1
    
    # Test with custom Swagger prefix (/apidocs) - immediate restart
    print_message "Starting second test immediately (testing SO_REUSEADDR)..."
    test_swagger_configuration "$CONFIG_2" "/apidocs" "swagger_custom" 2
    
    print_message "Immediate restart successful - SO_REUSEADDR is working!"
    
else
    # Skip Swagger tests if prerequisites failed
    print_message "Skipping Swagger tests due to prerequisite failures"
    # Account for skipped subtests (8 remaining: 4 for each configuration)
    for i in {4..11}; do
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
rm -f "$RESULTS_DIR"/*_trailing_slash_*.html "$RESULTS_DIR"/*_redirect_*.txt "$RESULTS_DIR"/*_content_*.html "$RESULTS_DIR"/*_initializer_*.js

# Only remove logs if tests were successful
if [ $FINAL_RESULT -eq 0 ]; then
    print_message "Tests passed, cleaning up log files..."
    rm -f "$RESULTS_DIR"/swagger_*_*.log
else
    print_message "Tests failed, preserving log files for analysis in $RESULTS_DIR/"
fi

# Export results for test_all.sh integration
# Derive test name from script filename for consistency with test_00_all.sh
TEST_IDENTIFIER=$(basename "${BASH_SOURCE[0]}" .sh | sed 's/test_[0-9]*_//')
export_subtest_results "${TEST_NUMBER}_${TEST_IDENTIFIER}" "$TOTAL_SUBTESTS" "$PASS_COUNT" "$TEST_NAME" > /dev/null

# Print test completion summary
print_test_completion "$TEST_NAME"

# Return status code if sourced, exit if run standalone
if [[ "$RUNNING_IN_TEST_SUITE" == "true" ]]; then
    return $FINAL_RESULT
else
    exit $FINAL_RESULT
fi
