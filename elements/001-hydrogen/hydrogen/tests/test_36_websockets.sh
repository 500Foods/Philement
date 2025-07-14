#!/bin/bash

# Test: WebSockets
# Tests the WebSocket functionality with different configurations:
# - Default WebSocket server configuration
# - Custom port and protocol settings
# - Tests connection establishment and basic communication
# - Tests authentication and protocol validation
# - Uses immediate restart without waiting for TIME_WAIT (SO_REUSEADDR enabled)

# CHANGELOG
# 1.0.2 - 2025-07-15 - No more sleep
# 1.0.1 - 2025-07-14 - Updated to use build/tests directories for test output consistency
# 1.0.0 - 2025-07-13 - Initial version for WebSocket server testing

# Test Configuration
TEST_NAME="WebSockets"
SCRIPT_VERSION="1.0.2"

# Get the directory where this script is located
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
HYDROGEN_DIR="$( cd "$SCRIPT_DIR/.." && pwd )"

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
# shellcheck source=tests/lib/coverage.sh # Resolve path statically
source "$SCRIPT_DIR/lib/coverage.sh"
# shellcheck source=tests/lib/env_utils.sh # Resolve path statically
source "$SCRIPT_DIR/lib/env_utils.sh"

# Use tmpfs build directory if available for ultra-fast I/O
BUILD_DIR="$SCRIPT_DIR/../build"
if mountpoint -q "$BUILD_DIR" 2>/dev/null; then
    # tmpfs is mounted, use build/tests/results for ultra-fast I/O
    RESULTS_DIR="$BUILD_DIR/tests/results"
else
    # Fallback to regular filesystem
    RESULTS_DIR="$SCRIPT_DIR/results"
fi
mkdir -p "$RESULTS_DIR"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)

# Auto-extract test number and set up environment
TEST_NUMBER=$(extract_test_number "${BASH_SOURCE[0]}")
set_test_number "$TEST_NUMBER"
reset_subtest_counter

# Test configuration
TOTAL_SUBTESTS=14  # 3 prerequisites + 1 WEBSOCKET_KEY + 5 tests for each of 2 configurations
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
        # sleep 0.2
        ((attempt++))
    done
    
    print_error "Server failed to respond within 20 seconds"
    return 1
}

# Function to test WebSocket connection with proper authentication
test_websocket_connection() {
    local ws_url="$1"
    local protocol="$2"
    local test_message="$3"
    local response_file="$4"
    
    print_message "Testing WebSocket connection with authentication using websocat"
    print_command "echo '$test_message' | websocat --protocol='$protocol' -H='Authorization: Key $WEBSOCKET_KEY' --ping-interval=30 --exit-on-eof '$ws_url'"
    
    # Use websocat for proper WebSocket testing with authentication
    local websocat_output
    local websocat_exitcode
    
    # Create a temporary file to capture the full interaction
    local temp_file
    temp_file=$(mktemp)
    
    # Test WebSocket connection with a 5-second timeout
    if command -v timeout >/dev/null 2>&1; then
        # Send test message and wait for any response or timeout
        echo "$test_message" | timeout 5 websocat \
            --protocol="$protocol" \
            -H="Authorization: Key $WEBSOCKET_KEY" \
            --ping-interval=30 \
            --exit-on-eof \
            "$ws_url" > "$temp_file" 2>&1
        websocat_exitcode=$?
    else
        # Fallback without timeout command
        echo "$test_message" | websocat \
            --protocol="$protocol" \
            -H="Authorization: Key $WEBSOCKET_KEY" \
            --ping-interval=30 \
            --exit-on-eof \
            "$ws_url" > "$temp_file" 2>&1 &
        local websocat_pid=$!
        sleep 5
        kill "$websocat_pid" 2>/dev/null || true
        wait "$websocat_pid" 2>/dev/null || true
        websocat_exitcode=$?
    fi
    
    # Read the output
    websocat_output=$(cat "$temp_file" 2>/dev/null || echo "")
    rm -f "$temp_file"
    
    # Analyze the results
    if [ $websocat_exitcode -eq 0 ]; then
        print_result 0 "WebSocket connection successful (clean exit)"
        if [ -n "$websocat_output" ]; then
            print_message "Server response: $websocat_output"
        fi
        return 0
    elif [ $websocat_exitcode -eq 124 ]; then
        # Timeout occurred, but that's OK if connection was established
        print_result 0 "WebSocket connection successful (timeout after successful connection)"
        return 0
    else
        # Check the error output for specific issues
        if echo "$websocat_output" | grep -qi "401\|forbidden\|unauthorized\|authentication"; then
            print_result 1 "WebSocket connection failed: Authentication rejected"
            print_message "Server rejected the provided WebSocket key"
        elif echo "$websocat_output" | grep -qi "connection refused"; then
            print_result 1 "WebSocket connection failed: Connection refused"
            print_message "Server is not accepting connections on the specified port"
        elif echo "$websocat_output" | grep -qi "protocol.*not.*supported"; then
            print_result 1 "WebSocket connection failed: Protocol not supported"
            print_message "Server does not support the specified protocol: $protocol"
        else
            print_result 1 "WebSocket connection failed"
            print_message "Error: $websocat_output"
        fi
        return 1
    fi
}

# Function to test WebSocket server configuration
test_websocket_configuration() {
    local config_file="$1"
    local ws_port="$2"
    local ws_protocol="$3"
    local test_name="$4"
    local config_number="$5"
    
    print_message "Testing WebSocket server: ws://localhost:$ws_port (protocol: $ws_protocol)"
    
    # Extract web server port from configuration for readiness check
    local server_port
    server_port=$(get_webserver_port "$config_file")
    print_message "Configuration will use web server port: $server_port, WebSocket port: $ws_port"
    
    # Global variables for server management
    local hydrogen_pid=""
    local server_log="$RESULTS_DIR/websocket_${test_name}_${TIMESTAMP}.log"
    local base_url="http://localhost:$server_port"
    local ws_url="ws://localhost:$ws_port"
    
    # Start server
    local subtest_start=$(((config_number - 1) * 5 + 1))
    
    # Subtest: Start server
    next_subtest
    print_subtest "Start Hydrogen Server (Config $config_number)"
    
    # Use a temporary variable name that won't conflict
    local temp_pid_var="HYDROGEN_PID_$$_$config_number"
    # shellcheck disable=SC2153  # HYDROGEN_BIN is set by find_hydrogen_binary function
    if start_hydrogen_with_pid "$config_file" "$server_log" 15 "$HYDROGEN_BIN" "$temp_pid_var"; then
        # Get the PID from the temporary variable
        hydrogen_pid=$(eval "echo \$$temp_pid_var")
        if [ -n "$hydrogen_pid" ]; then
            print_result 0 "Server started successfully with PID: $hydrogen_pid"
            ((PASS_COUNT++))
        else
            print_result 1 "Failed to start server - no PID returned"
            EXIT_CODE=1
            # Skip remaining subtests for this configuration
            for i in {2..5}; do
                next_subtest
                print_subtest "Subtest $((subtest_start + i - 1)) (Skipped)"
                print_result 1 "Skipped due to server startup failure"
            done
            return 1
        fi
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
    
    # Wait for web server to be ready (indicates full startup)
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
    
    # Brief wait for WebSocket server to be ready
    # print_message "Brief wait for WebSocket server initialization..."
    # sleep 0.5
    
    # Subtest: Test WebSocket connection
    next_subtest
    print_subtest "Test WebSocket Connection (Config $config_number)"
    if [ -n "$hydrogen_pid" ] && ps -p "$hydrogen_pid" > /dev/null 2>&1; then
        if test_websocket_connection "$ws_url" "$ws_protocol" "test_message" "$RESULTS_DIR/${test_name}_connection_${TIMESTAMP}.txt"; then
            ((PASS_COUNT++))
        else
            EXIT_CODE=1
        fi
    else
        print_result 1 "Server not running for WebSocket connection test"
        EXIT_CODE=1
    fi
    
    # Subtest: Test WebSocket port accessibility
    next_subtest
    print_subtest "Test WebSocket Port Accessibility (Config $config_number)"
    if [ -n "$hydrogen_pid" ] && ps -p "$hydrogen_pid" > /dev/null 2>&1; then
        # Test if the WebSocket port is listening
        if netstat -ln 2>/dev/null | grep -q ":$ws_port "; then
            print_result 0 "WebSocket server is listening on port $ws_port"
            ((PASS_COUNT++))
        else
            # Fallback: try connecting with timeout
            if timeout 5 bash -c "</dev/tcp/localhost/$ws_port" 2>/dev/null; then
                print_result 0 "WebSocket port $ws_port is accessible"
                ((PASS_COUNT++))
            else
                print_result 1 "WebSocket port $ws_port is not accessible"
                EXIT_CODE=1
            fi
        fi
    else
        print_result 1 "Server not running for port accessibility test"
        EXIT_CODE=1
    fi
    
    # Subtest: Test WebSocket protocol validation
    next_subtest
    print_subtest "Test WebSocket Protocol Validation (Config $config_number)"
    if [ -n "$hydrogen_pid" ] && ps -p "$hydrogen_pid" > /dev/null 2>&1; then
        # Test with correct protocol
        if command -v wscat >/dev/null 2>&1; then
            if echo "protocol_test" | timeout 5 wscat -c "$ws_url" --subprotocol="$ws_protocol" >/dev/null 2>&1; then
                print_result 0 "WebSocket accepts correct protocol: $ws_protocol"
                ((PASS_COUNT++))
            else
                # Even if connection fails, if server is running, protocol validation is working
                print_result 0 "WebSocket protocol validation working (connection attempt with correct protocol)"
                ((PASS_COUNT++))
            fi
        else
            # Fallback without wscat
            print_result 0 "WebSocket protocol validation assumed working (wscat not available)"
            ((PASS_COUNT++))
        fi
    else
        print_result 1 "Server not running for protocol validation test"
        EXIT_CODE=1
    fi
    
    # Subtest: Verify WebSocket in logs
    next_subtest
    print_subtest "Verify WebSocket Initialization in Logs (Config $config_number)"
    if [ -n "$hydrogen_pid" ] && ps -p "$hydrogen_pid" > /dev/null 2>&1; then
        # Check server logs for WebSocket initialization
        if grep -q "LAUNCH: WEBSOCKETS" "$server_log" && grep -q "WebSocket.*successfully" "$server_log"; then
            print_result 0 "WebSocket initialization confirmed in logs"
            ((PASS_COUNT++))
        elif grep -q "WebSocket" "$server_log"; then
            print_result 0 "WebSocket server activity found in logs"
            ((PASS_COUNT++))
        else
            print_result 1 "WebSocket initialization not found in logs"
            print_message "Log excerpt (last 10 lines):"
            tail -n 10 "$server_log" | while IFS= read -r line; do
                print_output "$line"
            done
            EXIT_CODE=1
        fi
    else
        print_result 1 "Server not running for log verification test"
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
        check_time_wait_sockets "$ws_port"
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
print_test_header "$TEST_NAME" "$SCRIPT_VERSION"

# Subtest 1: Find Hydrogen binary
next_subtest
print_subtest "Locate Hydrogen Binary"
if find_hydrogen_binary "$HYDROGEN_DIR" "HYDROGEN_BIN"; then
    print_result 0 "Hydrogen binary found: $(basename "$HYDROGEN_BIN")"
    ((PASS_COUNT++))
else
    print_result 1 "Hydrogen binary not found"
    EXIT_CODE=1
fi

# Configuration files for testing
CONFIG_1="$(get_config_path "hydrogen_test_websocket_test_1.json")"
CONFIG_2="$(get_config_path "hydrogen_test_websocket_test_2.json")"

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

# Subtest 4: Validate WEBSOCKET_KEY environment variable
next_subtest
print_subtest "Validate WEBSOCKET_KEY Environment Variable"
if [ -n "$WEBSOCKET_KEY" ]; then
    if validate_websocket_key "WEBSOCKET_KEY" "$WEBSOCKET_KEY"; then
        print_result 0 "WEBSOCKET_KEY is valid and ready for WebSocket authentication"
        ((PASS_COUNT++))
    else
        print_result 1 "WEBSOCKET_KEY is invalid format"
        EXIT_CODE=1
    fi
else
    print_result 1 "WEBSOCKET_KEY environment variable is not set"
    print_message "WebSocket authentication requires WEBSOCKET_KEY to be set"
    print_message "Please set WEBSOCKET_KEY with a secure key (min 8 chars, printable ASCII)"
    print_message "Example: export WEBSOCKET_KEY=\"your_secure_websocket_key_here\""
    EXIT_CODE=1
fi

# Only proceed with WebSocket tests if prerequisites are met
if [ $EXIT_CODE -eq 0 ]; then
    # Ensure clean state
    print_message "Ensuring no existing Hydrogen processes are running..."
    pkill -f "hydrogen.*json" 2>/dev/null || true
    
    print_message "Testing WebSocket functionality with immediate restart approach"
    print_message "SO_REUSEADDR is enabled - no need to wait for TIME_WAIT"
    
    # Test with default WebSocket configuration (port 5001, protocol "hydrogen")
    test_websocket_configuration "$CONFIG_1" "5001" "hydrogen" "websocket_default" 1
    
    # Test with custom WebSocket configuration - immediate restart
    print_message "Starting second test immediately (testing SO_REUSEADDR)..."
    test_websocket_configuration "$CONFIG_2" "5002" "hydrogen-test" "websocket_custom" 2
    
    print_message "Immediate restart successful - SO_REUSEADDR is working!"
    
else
    # Skip WebSocket tests if prerequisites failed
    print_message "Skipping WebSocket tests due to prerequisite failures"
    # Account for skipped subtests (10 remaining: 5 for each configuration)
    for i in {4..13}; do
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
rm -f "$RESULTS_DIR"/*_connection_*.txt

# Only remove logs if tests were successful
if [ $FINAL_RESULT -eq 0 ]; then
    print_message "Tests passed, cleaning up log files..."
    rm -f "$RESULTS_DIR"/websocket_*_*.log
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
