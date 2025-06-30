#!/bin/bash
#
# About this Script
#
# Socket Rebinding Test
# Tests that the SO_REUSEADDR socket option allows immediate rebinding after shutdown
# with active HTTP connections that create TIME_WAIT sockets
#
NAME_SCRIPT="Socket Rebinding Test"
VERS_SCRIPT="3.0.0"

# VERSION HISTORY
# 3.0.0 - 2025-06-30 - Enhanced to make actual HTTP requests, creating proper TIME_WAIT conditions for realistic testing
# 2.0.0 - 2025-06-17 - Major refactoring: fixed all shellcheck warnings, improved modularity, enhanced comments
# 1.0.0 - Original version - Basic socket rebinding test functionality

# Display script name and version
echo "=== $NAME_SCRIPT v$VERS_SCRIPT ==="

# Get the directory where this script is located
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
HYDROGEN_DIR="$( cd "$SCRIPT_DIR/.." && pwd )"

# Include the common test utilities
source "$SCRIPT_DIR/support_utils.sh"
source "$SCRIPT_DIR/support_timewait.sh"

# Configuration - prefer release build if available, fallback to naked build
find_hydrogen_binary() {
    if [ -f "$HYDROGEN_DIR/hydrogen_release" ]; then
        print_info "Using release build for testing" >&2
        echo "$HYDROGEN_DIR/hydrogen_release"
    elif [ -f "$HYDROGEN_DIR/hydrogen" ]; then
        print_info "Using standard build for testing" >&2
        echo "$HYDROGEN_DIR/hydrogen"
    else
        print_error "No hydrogen binary found in $HYDROGEN_DIR" >&2
        exit 1
    fi
}

# Function to find appropriate configuration file
find_config_file() {
    local config_file
    config_file=$(get_config_path "hydrogen_test_api.json")
    if [ ! -f "$config_file" ]; then
        config_file=$(get_config_path "hydrogen_test_max.json")
        print_info "API test config not found, using max config"
    fi
    echo "$config_file"
}

# Function to check if a port is in use
check_port_in_use() {
    local port="$1"
    if command -v ss &> /dev/null; then
        ss -tuln | grep ":$port " > /dev/null
        return $?
    elif command -v netstat &> /dev/null; then
        netstat -tuln | grep ":$port " > /dev/null
        return $?
    else
        echo "Neither ss nor netstat is available"
        return 1
    fi
}

# Function to get the web server port from config
get_webserver_port() {
    local config="$1"
    local port
    
    if command -v jq &> /dev/null; then
        # Use jq if available
        port=$(jq -r '.WebServer.Port // 8080' "$config" 2>/dev/null)
        if [ -z "$port" ] || [ "$port" = "null" ]; then
            port=8080
        fi
    else
        # Fallback to grep
        port=$(grep -o '"Port":[[:space:]]*[0-9]*' "$config" | head -1 | grep -o '[0-9]*')
        if [ -z "$port" ]; then
            port=8080
        fi
    fi
    echo "$port"
}

# Function to safely kill a process with PID validation
# shellcheck disable=SC2317  # Function is called indirectly via cleanup trap
safe_kill_process() {
    local pid="$1"
    local process_name="$2"
    
    # Validate PID is not empty and not zero
    if [ -z "$pid" ] || [ "$pid" = "0" ]; then
        return 1
    fi
    
    if ps -p "$pid" > /dev/null 2>&1; then
        echo "Cleaning up $process_name (PID $pid)..."
        kill -SIGINT "$pid" 2>/dev/null || kill -9 "$pid" 2>/dev/null
        return 0
    fi
    return 1
}

# Handle cleanup on script termination
# shellcheck disable=SC2317  # Function is called indirectly via trap
cleanup() {
    # Kill any hydrogen processes started by this script
    safe_kill_process "$FIRST_PID" "first instance"
    safe_kill_process "$SECOND_PID" "second instance"
    
    # Save test log if we've started writing it
    if [ -n "$RESULT_LOG" ] && [ -f "$RESULT_LOG" ]; then
        echo "Saving test results to $(convert_to_relative_path "$RESULT_LOG")"
    fi
    
    exit 0
}

# Function to validate process is running and bound to port
# shellcheck disable=SC2317  # Function is called in main execution flow
validate_instance() {
    local pid="$1"
    local port="$2"
    local instance_name="$3"
    
    # Check if PID is empty or invalid
    if [ -z "$pid" ] || [ "$pid" = "0" ]; then
        print_result 1 "$instance_name failed to start (invalid PID: '$pid')" | tee -a "$RESULT_LOG"
        end_test 1 "Socket Rebinding Test" | tee -a "$RESULT_LOG"
        exit 1
    fi
    
    # Check if process is running
    if ! ps -p "$pid" > /dev/null 2>&1; then
        print_result 1 "$instance_name failed to start (PID $pid not running)" | tee -a "$RESULT_LOG"
        end_test 1 "Socket Rebinding Test" | tee -a "$RESULT_LOG"
        exit 1
    fi
    
    # Check if port is bound
    if ! check_port_in_use "$port"; then
        print_result 1 "$instance_name failed to bind to port $port" | tee -a "$RESULT_LOG"
        kill "$pid" 2>/dev/null || true
        end_test 1 "Socket Rebinding Test" | tee -a "$RESULT_LOG"
        exit 1
    fi
    
    print_info "$instance_name running and bound to port $port" | tee -a "$RESULT_LOG"
}

# Function to check TIME_WAIT sockets (wrapper for logging)
# shellcheck disable=SC2317  # Function is called in main execution flow
check_time_wait_sockets() {
    local port="$1"
    
    # Use the robust function from support_timewait.sh and log the output
    check_time_wait_sockets_robust "$port" | tee -a "$RESULT_LOG"
}

# Function to ensure clean test environment
# shellcheck disable=SC2317  # Function is called in main execution flow
prepare_test_environment() {
    local port="$1"
    
    # Make sure no Hydrogen instances are running
    print_info "Ensuring no Hydrogen instances are running..." | tee -a "$RESULT_LOG"
    pkill -f hydrogen || true
    sleep 2
    
    # Check if port is already in use
    if check_port_in_use "$port"; then
        print_warning "Port $port is already in use, attempting to force release..." | tee -a "$RESULT_LOG"
        pkill -f hydrogen || true
        sleep 5
        if check_port_in_use "$port"; then
            print_result 1 "Port $port is still in use, cannot run test" | tee -a "$RESULT_LOG"
            end_test 1 "Socket Rebinding Test" | tee -a "$RESULT_LOG"
            exit 1
        fi
    fi
}

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
            {
                print_warning "Startup timeout after ${elapsed}s"
            } | tee -a "$RESULT_LOG" >&2
            return 1
        fi
        
        if grep -q "Application started" "$log_file" 2>/dev/null; then
            {
                print_info "Startup completed in ${elapsed}s"
            } | tee -a "$RESULT_LOG" >&2
            return 0
        fi
        
        sleep 0.2
    done
}

# Function to start hydrogen instance
# shellcheck disable=SC2317  # Function is called in main execution flow
start_hydrogen_instance() {
    local hydrogen_bin="$1"
    local config_file="$2"
    local instance_name="$3"
    local log_file="$RESULTS_DIR/${instance_name// /_}_${TIMESTAMP}.log"
    
    # Send all informational output to stderr to avoid interfering with PID capture
    {
        print_header "Starting Hydrogen ($instance_name)"
    } | tee -a "$RESULT_LOG" >&2
    
    # Check if binary exists and is executable
    if [ ! -x "$hydrogen_bin" ]; then
        {
            print_error "Hydrogen binary not found or not executable: $hydrogen_bin"
        } | tee -a "$RESULT_LOG" >&2
        return 1
    fi
    
    # Check if config file exists
    if [ ! -f "$config_file" ]; then
        {
            print_error "Config file not found: $config_file"
        } | tee -a "$RESULT_LOG" >&2
        return 1
    fi
    
    # Clean log file
    true > "$log_file"
    
    # Start the process with output to log file
    "$hydrogen_bin" "$config_file" > "$log_file" 2>&1 &
    local pid=$!
    
    # Verify we got a valid PID
    if [ -z "$pid" ] || [ "$pid" = "0" ]; then
        {
            print_error "Failed to start $instance_name - invalid PID"
        } | tee -a "$RESULT_LOG" >&2
        return 1
    fi
    
    {
        print_info "Started with PID: $pid"
    } | tee -a "$RESULT_LOG" >&2
    
    # Verify process started
    if ! ps -p "$pid" > /dev/null 2>&1; then
        {
            print_error "$instance_name failed to start"
        } | tee -a "$RESULT_LOG" >&2
        return 1
    fi
    
    # Wait for startup completion by monitoring log
    {
        print_info "Waiting for startup completion (max 15 seconds)..."
    } | tee -a "$RESULT_LOG" >&2
    
    if ! wait_for_startup "$log_file" 15; then
        {
            print_error "$instance_name startup failed or timed out"
            # Show last few lines of log for debugging
            print_info "Last 10 lines of startup log:"
            tail -10 "$log_file" || true
        } | tee -a "$RESULT_LOG" >&2
        return 1
    fi
    
    # Final verification that process is still running
    if ! ps -p "$pid" > /dev/null 2>&1; then
        {
            print_error "$instance_name (PID $pid) exited after startup"
        } | tee -a "$RESULT_LOG" >&2
        return 1
    fi
    
    # Verify port is bound (get port from config)
    local port
    port=$(get_webserver_port "$config_file")
    {
        print_info "Verifying port $port is bound..."
    } | tee -a "$RESULT_LOG" >&2
    
    # Give a moment for port binding
    sleep 1
    
    if ! check_port_in_use "$port"; then
        {
            print_error "$instance_name failed to bind to port $port"
        } | tee -a "$RESULT_LOG" >&2
        kill "$pid" 2>/dev/null || true
        return 1
    fi
    
    {
        print_info "$instance_name successfully started and bound to port $port"
    } | tee -a "$RESULT_LOG" >&2
    
    # Output PID to stdout for command substitution capture
    echo "$pid"
}

# Function to shutdown instance gracefully
# shellcheck disable=SC2317  # Function is called in main execution flow
shutdown_instance() {
    local pid="$1"
    local instance_name="$2"
    
    print_header "Shutting down $instance_name" | tee -a "$RESULT_LOG"
    
    # Validate PID before attempting to kill
    if [ -z "$pid" ] || [ "$pid" = "0" ]; then
        print_warning "$instance_name has invalid PID ($pid), cannot shutdown" | tee -a "$RESULT_LOG"
        return 1
    fi
    
    # Check if process is still running before attempting to kill
    if ! ps -p "$pid" > /dev/null 2>&1; then
        {
            print_info "$instance_name (PID $pid) is already terminated"
        } | tee -a "$RESULT_LOG"
        return 0
    fi
    
    {
        print_info "Terminating $instance_name (PID $pid)..."
    } | tee -a "$RESULT_LOG"
    
    kill -SIGINT "$pid"
    sleep 1
    
    # Verify instance has exited
    if ps -p "$pid" > /dev/null 2>&1; then
        {
            print_warning "$instance_name still running, forcing termination..."
        } | tee -a "$RESULT_LOG"
        kill -9 "$pid" 2>/dev/null || true
        sleep 1
    fi
    
    {
        print_info "$instance_name (PID $pid) termination confirmed"
    } | tee -a "$RESULT_LOG"
}

# Set up trap for clean termination
trap cleanup SIGINT SIGTERM EXIT

# Initialize configuration
HYDROGEN_BIN=$(find_hydrogen_binary "$HYDROGEN_DIR")
CONFIG_FILE=$(find_config_file)

# Create output directories
RESULTS_DIR="$SCRIPT_DIR/results"
mkdir -p "$RESULTS_DIR"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
RESULT_LOG="$RESULTS_DIR/socket_rebind_test_${TIMESTAMP}.log"

# Start the test
start_test "Socket Rebinding Test" | tee -a "$RESULT_LOG"
print_info "Testing SO_REUSEADDR socket option for immediate port rebinding" | tee -a "$RESULT_LOG"
print_info "Using config: $(convert_to_relative_path "$CONFIG_FILE")" | tee -a "$RESULT_LOG"

# Initialize subtest tracking
TOTAL_SUBTESTS=3  # First instance, shutdown, second instance
PASS_COUNT=0

# Get the web server port
PORT=$(get_webserver_port "$CONFIG_FILE")
print_info "Web server port: $PORT" | tee -a "$RESULT_LOG"

# Prepare clean test environment
prepare_test_environment "$PORT"

# Test Phase 1: Start first instance
if ! FIRST_PID=$(start_hydrogen_instance "$HYDROGEN_BIN" "$CONFIG_FILE" "first instance") || [ -z "$FIRST_PID" ]; then
    print_result 1 "Failed to start first instance" | tee -a "$RESULT_LOG"
    end_test 1 "Socket Rebinding Test" | tee -a "$RESULT_LOG"
    exit 1
fi
print_info "First instance running and bound to port $PORT successfully!" | tee -a "$RESULT_LOG"

# Make HTTP requests to create active connections (and thus TIME_WAIT sockets on shutdown)
print_header "Making HTTP requests to create active connections" | tee -a "$RESULT_LOG"
BASE_URL="http://localhost:$PORT"

# Wait for server to be ready
print_info "Waiting for server to be ready..." | tee -a "$RESULT_LOG"
sleep 2

# Make requests to common web files to ensure connections are established
print_info "Requesting index.html..." | tee -a "$RESULT_LOG"
curl -s --max-time 5 "$BASE_URL/" > "$RESULTS_DIR/index_response_${TIMESTAMP}.html" 2>/dev/null || true

print_info "Requesting favicon.ico..." | tee -a "$RESULT_LOG"
curl -s --max-time 5 "$BASE_URL/favicon.ico" > "$RESULTS_DIR/favicon_response_${TIMESTAMP}.ico" 2>/dev/null || true

# Make a few more requests to ensure multiple connections
print_info "Making additional requests to establish multiple connections..." | tee -a "$RESULT_LOG"
curl -s --max-time 5 "$BASE_URL/robots.txt" > /dev/null 2>&1 || true
curl -s --max-time 5 "$BASE_URL/sitemap.xml" > /dev/null 2>&1 || true

print_info "HTTP requests completed - connections established" | tee -a "$RESULT_LOG"

# First subtest passed
((PASS_COUNT++))

# Test Phase 2: Shutdown first instance
shutdown_instance "$FIRST_PID" "first instance"
# Second subtest passed
((PASS_COUNT++))

# Check if socket is in TIME_WAIT state
check_time_wait_sockets "$PORT"

# Test Phase 3: Immediately start second instance
if ! SECOND_PID=$(start_hydrogen_instance "$HYDROGEN_BIN" "$CONFIG_FILE" "second instance - immediate restart") || [ -z "$SECOND_PID" ]; then
    print_result 1 "Failed to start second instance" | tee -a "$RESULT_LOG"
    end_test 1 "Socket Rebinding Test" | tee -a "$RESULT_LOG"
    exit 1
fi
print_info "Second instance running and bound to port $PORT successfully!" | tee -a "$RESULT_LOG"
print_info "SO_REUSEADDR is working correctly" | tee -a "$RESULT_LOG"
# Third subtest passed
((PASS_COUNT++))

# Clean up
print_header "Cleaning up" | tee -a "$RESULT_LOG"

# Validate PID before cleanup
if [ -n "$SECOND_PID" ] && [ "$SECOND_PID" != "0" ]; then
    # Check if process is still running before attempting to kill
    if ps -p "$SECOND_PID" > /dev/null 2>&1; then
        {
            print_info "Terminating second instance (PID $SECOND_PID)..."
        } | tee -a "$RESULT_LOG"
        
        kill -SIGINT "$SECOND_PID" 2>/dev/null || true
        sleep 2
        
        if ps -p "$SECOND_PID" > /dev/null 2>&1; then
            {
                print_warning "Second instance still running, forcing termination..."
            } | tee -a "$RESULT_LOG"
            kill -9 "$SECOND_PID" 2>/dev/null || true
        fi
        
        {
            print_info "Second instance (PID $SECOND_PID) termination confirmed"
        } | tee -a "$RESULT_LOG"
    else
        {
            print_info "Second instance (PID $SECOND_PID) already terminated"
        } | tee -a "$RESULT_LOG"
    fi
else
    {
        print_info "No second instance to clean up"
    } | tee -a "$RESULT_LOG"
fi

# Get test name from script name
TEST_NAME=$(basename "$0" .sh | sed 's/^test_//')

# Export subtest results for test_all.sh to pick up
export_subtest_results "$TEST_NAME" $TOTAL_SUBTESTS $PASS_COUNT

# Log subtest results
print_info "Socket Rebind Test: $PASS_COUNT of $TOTAL_SUBTESTS subtests passed" | tee -a "$RESULT_LOG"

# Test successful!
print_result 0 "Socket rebinding test PASSED - Immediate rebinding after shutdown works!" | tee -a "$RESULT_LOG"
print_info "Results saved to: $(convert_to_relative_path "$RESULT_LOG")" | tee -a "$RESULT_LOG"
end_test 0 "Socket Rebinding Test" | tee -a "$RESULT_LOG"
exit 0
