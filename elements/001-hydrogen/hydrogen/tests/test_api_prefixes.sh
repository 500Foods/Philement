#!/bin/bash
#
# API Prefix Test
# Tests the API endpoints with different API prefix configurations:
# - Default "/api" prefix using standard hydrogen_test_api.json
# - Custom "/myapi" prefix using hydrogen_test_api_prefix.json

# Get the directory where this script is located
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

# Include the common test utilities
source "$SCRIPT_DIR/support_utils.sh"

# Create results directory if it doesn't exist
RESULTS_DIR="$SCRIPT_DIR/results"
mkdir -p "$RESULTS_DIR"

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
        curl_command="${curl_command/curl/curl --max-time 5}"
    fi
    
    print_command "$curl_command"
    eval "$curl_command > $response_file"
    CURL_STATUS=$?
    
    # Check if the request was successful
    if [ $CURL_STATUS -eq 0 ]; then
        print_info "Successfully received response!"
        
        # Only show a brief excerpt of the response to avoid flooding the output
        print_info "Response excerpt (first 5 lines):"
        head -n 5 "$response_file"
        if [ $(wc -l < "$response_file") -gt 5 ]; then
            echo "..."
        fi
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

# Function to check if a port is in use
check_port_in_use() {
    local port=$1
    if command -v lsof >/dev/null 2>&1; then
        lsof -i :$port >/dev/null 2>&1
        return $?
    elif command -v netstat >/dev/null 2>&1; then
        netstat -tuln | grep -q ":$port "
        return $?
    else
        # Fallback method - try to connect to the port
        (echo > /dev/tcp/127.0.0.1/$port) >/dev/null 2>&1
        return $?
    fi
}

# Function to display process details
show_process_details() {
    local pid=$1
    local process_name=$2
    
    if [ -z "$pid" ]; then
        print_warning "No PID found for $process_name"
        return 1
    fi
    
    print_info "Process Details for $process_name (PID: $pid):"
    
    # Show command line
    if [ -e "/proc/$pid/cmdline" ]; then
        local cmdline=$(tr '\0' ' ' < /proc/$pid/cmdline)
        print_info "  Command line: $cmdline"
    else
        # Fallback to ps
        print_info "  Command line: $(ps -p $pid -o args= 2>/dev/null || echo "Unknown")"
    fi
    
    # Show resource usage if possible
    if command -v ps >/dev/null 2>&1; then
        print_info "  Resources: $(ps -p $pid -o %cpu,%mem,vsz,rss 2>/dev/null || echo "Unknown")"
    fi
    
    # Show status
    if [ -e "/proc/$pid/status" ]; then
        print_info "  Status:"
        grep -E '^State:|^Threads:|^VmRSS:|^VmSize:' /proc/$pid/status | sed 's/^/    /'
    fi
    
    return 0
}

# Function to test system endpoints with a specific API prefix
test_api_prefix() {
    local config_file="$1"
    local api_prefix="$2"
    local test_name="$3"
    
    print_header "Testing API Prefix: $api_prefix (using $test_name)"
    
    # Extract the WebServer port from the configuration file
    local web_server_port=$(extract_web_server_port "$config_file")
    print_info "Using WebServer port: $web_server_port from configuration"
    
    # System information
    print_info "System Information:"
    print_info "  Hostname: $(hostname)"
    print_info "  Kernel: $(uname -r)"
    print_info "  Architecture: $(uname -m)"
    print_info "  Current time: $(date)"
    
    # Check for port conflicts before starting
    print_info "Checking if ports are available..."
    if check_port_in_use $web_server_port; then
        print_warning "Port $web_server_port appears to be in use! This may cause test failures."
        print_info "Processes using port $web_server_port:"
        if command -v lsof >/dev/null 2>&1; then
            lsof -i :$web_server_port | sed 's/^/    /'
        elif command -v netstat >/dev/null 2>&1; then
            netstat -tulnp 2>/dev/null | grep ":$web_server_port " | sed 's/^/    /'
        fi
    else
        print_info "  Port $web_server_port is available"
    fi
    
    # Results dictionary to track test results
    local HEALTH_RESULT=1
    local INFO_RESULT=1
    local INFO_JSON_RESULT=1
    local TEST_BASIC_GET_RESULT=1
    
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
    print_info "Starting hydrogen server ($(basename "$HYDROGEN_BIN")) with configuration ($(convert_to_relative_path "$config_file"))..."
    print_info "Startup command: $HYDROGEN_BIN $config_file"
    
    # Record process start time
    START_TIME=$(date +%s)
    
    # Run from the current directory (tests/) to ensure all output stays in tests/ directory
    "$HYDROGEN_BIN" "$config_file" > "$RESULTS_DIR/hydrogen_test_${test_name}.log" 2>&1 &
    INIT_PID=$!
    
    print_info "Initial PID: $INIT_PID (from shell)"
    
    # Wait briefly to allow the process to start
    sleep 2
    
    # Find the actual PID using grep
    ACTUAL_PID=$(pgrep -f ".*hydrogen.*$(basename "$config_file")" || echo "")
    
    # If we have multiple PIDs, take the first one
    if [[ "$ACTUAL_PID" == *$'\n'* ]]; then
        print_warning "Multiple PIDs found that match our pattern:"
        echo "$ACTUAL_PID" | sed 's/^/    /'
        ACTUAL_PID=$(echo "$ACTUAL_PID" | head -n1)
        print_info "Using first PID: $ACTUAL_PID"
    fi
    
    if [ -n "$ACTUAL_PID" ]; then
        print_info "Found actual Hydrogen PID: $ACTUAL_PID"
        
        # Get detailed information about the process
        show_process_details "$ACTUAL_PID" "Hydrogen Server"
        
        # Wait for the server to initialize
        print_info "Waiting for server to initialize (8 more seconds)..."
        sleep 8
        
        # Verify the process is still running
        if kill -0 $ACTUAL_PID 2>/dev/null; then
            print_info "Server is running (PID: $ACTUAL_PID)"
            
            # Check if the server is accepting connections
            if check_port_in_use $web_server_port; then
                print_info "Server is listening on port $web_server_port"
            else
                print_warning "Server does not appear to be listening on port $web_server_port!"
                # Show open ports from the process
                if command -v lsof >/dev/null 2>&1; then
                    print_info "Open ports for PID $ACTUAL_PID:"
                    lsof -p $ACTUAL_PID -i -P | sed 's/^/    /' || echo "    None found"
                fi
            fi
        else
            print_warning "Server process appears to have died during initialization!"
            print_info "Last 20 lines of server log:"
            tail -n 20 "$RESULTS_DIR/hydrogen_test_${test_name}.log" | sed 's/^/    /'
        fi
    else
        print_warning "Could not find hydrogen server PID. Server may have failed to start."
        print_info "Process list (ps aux | grep hydrogen):"
        ps aux | grep -E "[h]ydrogen|[H]ydrogen" | sed 's/^/    /'
        print_info "Last 20 lines of server log:"
        tail -n 20 "$RESULTS_DIR/hydrogen_test_${test_name}.log" | sed 's/^/    /'
    fi
    
    # Test health endpoint
    print_header "Testing Health Endpoint with prefix '$api_prefix'"
    validate_request "${test_name}_health" "curl -s --max-time 5 http://localhost:${web_server_port}${api_prefix}/system/health" "Yes, I'm alive, thanks!"
    HEALTH_RESULT=$?
    
    # Test info endpoint
    print_header "Testing Info Endpoint with prefix '$api_prefix'"
    validate_request "${test_name}_info" "curl -s --max-time 5 http://localhost:${web_server_port}${api_prefix}/system/info" "system"
    INFO_RESULT=$?
    
    # Also validate that it's valid JSON
    if [ $INFO_RESULT -eq 0 ]; then
        validate_json "$RESULTS_DIR/response_${test_name}_info.json"
        INFO_JSON_RESULT=$?
    else
        INFO_JSON_RESULT=1
    fi
    
    # Test test endpoint (basic GET)
    print_header "Testing Test Endpoint (GET) with prefix '$api_prefix'"
    validate_request "${test_name}_test_get" "curl -s --max-time 5 http://localhost:${web_server_port}${api_prefix}/system/test" "client_ip"
    TEST_BASIC_GET_RESULT=$?
    
    # Verify the server is still running after all tests
    print_header "Checking server status after tests"
    ACTUAL_PID=$(pgrep -f ".*hydrogen.*$(basename "$config_file")" || echo "")
    
    # Check server stability
    if [ -n "$ACTUAL_PID" ]; then
        print_info "Server is still running after tests (PID: $ACTUAL_PID)"
        
        # Show process details before stopping
        show_process_details "$ACTUAL_PID" "Hydrogen Server (before shutdown)"
        
        # Check how long the server has been running
        if [ -e "/proc/$ACTUAL_PID/stat" ]; then
            PROC_STAT=$(cat /proc/$ACTUAL_PID/stat)
            START_TIME_TICKS=$(echo $PROC_STAT | awk '{print $22}')
            CLK_TCK=$(getconf CLK_TCK)
            UPTIME=$(cat /proc/uptime | awk '{print $1}')
            PROC_UPTIME=$(echo "scale=2; $UPTIME - ($START_TIME_TICKS / $CLK_TCK)" | bc 2>/dev/null || echo "Unknown")
            print_info "Server uptime: approximately $PROC_UPTIME seconds"
        else
            # Fallback: Calculate from our recorded start time
            CURRENT_TIME=$(date +%s)
            UPTIME=$((CURRENT_TIME - START_TIME))
            print_info "Server uptime: approximately $UPTIME seconds (from script timing)"
        fi
        
        STABILITY_RESULT=0
        
        # Gather network information before stopping
        print_info "Network connections before shutdown:"
        if command -v lsof >/dev/null 2>&1; then
            lsof -p $ACTUAL_PID -i -P 2>/dev/null | sed 's/^/    /' || echo "    None found"
        elif command -v netstat >/dev/null 2>&1; then
            netstat -tulnp 2>/dev/null | grep "$ACTUAL_PID" | sed 's/^/    /' || echo "    None found"
        fi
        
        # Stop the server
        print_info "Stopping server (PID: $ACTUAL_PID)..."
        kill $ACTUAL_PID
        
        # Monitor shutdown progress
        for i in {1..5}; do
            sleep 1
            if ! kill -0 $ACTUAL_PID 2>/dev/null; then
                print_info "Server stopped gracefully after $i seconds"
                break
            else
                print_info "Server still shutting down ($i seconds elapsed)..."
            fi
        done
        
        # Make sure it's really stopped
        if kill -0 $ACTUAL_PID 2>/dev/null; then
            print_warning "Server didn't stop gracefully after 5 seconds, sending SIGTERM again..."
            kill -15 $ACTUAL_PID
            sleep 2
            
            if kill -0 $ACTUAL_PID 2>/dev/null; then
                print_warning "Server still not responding to SIGTERM, forcing termination with SIGKILL..."
                kill -9 $ACTUAL_PID
                sleep 1
                
                if kill -0 $ACTUAL_PID 2>/dev/null; then
                    print_result 1 "ERROR: Failed to terminate server process even with SIGKILL!"
                    STABILITY_RESULT=1
                else
                    print_warning "Server terminated with SIGKILL"
                fi
            else
                print_info "Server stopped with second SIGTERM"
            fi
        fi
        
        # Check for zombie processes
        ZOMBIE_PROCESSES=$(ps -ef | grep -E "defunct.*[h]ydrogen" || echo "")
        if [ -n "$ZOMBIE_PROCESSES" ]; then
            print_warning "Zombie hydrogen processes detected:"
            echo "$ZOMBIE_PROCESSES" | sed 's/^/    /'
        fi
    else
        print_result 1 "ERROR: Server has crashed or exited prematurely"
        print_info "Last 30 lines of server log:"
        tail -n 30 "$RESULTS_DIR/hydrogen_test_${test_name}.log" | sed 's/^/    /'
        STABILITY_RESULT=1
    fi
    
    # Calculate overall result
    if [ $HEALTH_RESULT -eq 0 ] && [ $INFO_RESULT -eq 0 ] && [ $INFO_JSON_RESULT -eq 0 ] && 
       [ $TEST_BASIC_GET_RESULT -eq 0 ] && [ $STABILITY_RESULT -eq 0 ]; then
        print_result 0 "All tests for API prefix '$api_prefix' passed successfully"
        return 0
    else
        print_result 1 "Some tests for API prefix '$api_prefix' failed"
        return 1
    fi
}


# Extract ports from both configurations at the beginning
DEFAULT_CONFIG_PATH="$(get_config_path "hydrogen_test_api_test_1.json")"
CUSTOM_CONFIG_PATH="$(get_config_path "hydrogen_test_api_test_2.json")"
DEFAULT_PORT=$(extract_web_server_port "$DEFAULT_CONFIG_PATH")
CUSTOM_PORT=$(extract_web_server_port "$CUSTOM_CONFIG_PATH")

# Start the test
start_test "Hydrogen API Prefix Test"
print_info "Default configuration will use port: $DEFAULT_PORT"
print_info "Custom configuration will use port: $CUSTOM_PORT"

# Ensure clean state
print_info "Ensuring no existing hydrogen processes are running..."
pkill -f "hydrogen.*json" 2>/dev/null
sleep 2

# Test with default configuration (/api prefix)
test_api_prefix "$DEFAULT_CONFIG_PATH" "/api" "default"
DEFAULT_PREFIX_RESULT=$?

# Ensure server is stopped before starting next test
print_info "Making sure all previous servers are stopped..."
pkill -f "hydrogen.*json" 2>/dev/null
sleep 2

# Check for TIME_WAIT sockets and show detailed socket state
print_info "Checking for sockets in TIME_WAIT state on port $DEFAULT_PORT..."
if command -v ss &> /dev/null; then
    # Ensure we get a clean integer count without any extra characters
    TIME_WAIT_COUNT=$(ss -tan | grep ":$DEFAULT_PORT" | grep -c "TIME-WAIT" 2>/dev/null || echo 0)
    TIME_WAIT_COUNT=$(echo "$TIME_WAIT_COUNT" | tr -d '[:space:]')
    if [ -z "$TIME_WAIT_COUNT" ]; then
        TIME_WAIT_COUNT=0
    fi
    
    if [ "$TIME_WAIT_COUNT" -gt 0 ]; then
        print_info "Found $TIME_WAIT_COUNT socket(s) in TIME-WAIT state on port $DEFAULT_PORT"
        ss -tan | grep ":$DEFAULT_PORT" | grep "TIME-WAIT" | sed 's/^/    /'
        
        # Set the SO_REUSEADDR option on the next socket by modifying the net.ipv4.tcp_tw_reuse sysctl parameter
        # This is a temporary change that will help ensure the port can be rebound immediately
        if command -v sysctl &> /dev/null && [ -w /proc/sys/net/ipv4/tcp_tw_reuse ]; then
            print_info "Trying to enable tcp_tw_reuse to help with socket rebinding..."
            OLD_TW_REUSE=$(cat /proc/sys/net/ipv4/tcp_tw_reuse)
            sudo sysctl -w net.ipv4.tcp_tw_reuse=1 > /dev/null 2>&1 || true
            print_info "tcp_tw_reuse changed from $OLD_TW_REUSE to $(cat /proc/sys/net/ipv4/tcp_tw_reuse 2>/dev/null || echo 'unknown')"
        fi
    else
        print_info "No TIME-WAIT sockets found on port $DEFAULT_PORT"
    fi
elif command -v netstat &> /dev/null; then
    # Ensure we get a clean integer count without any extra characters
    TIME_WAIT_COUNT=$(netstat -tan | grep ":$DEFAULT_PORT" | grep -c "TIME_WAIT" 2>/dev/null || echo 0)
    TIME_WAIT_COUNT=$(echo "$TIME_WAIT_COUNT" | tr -d '[:space:]')
    if [ -z "$TIME_WAIT_COUNT" ]; then
        TIME_WAIT_COUNT=0
    fi
    
    if [ "$TIME_WAIT_COUNT" -gt 0 ]; then
        print_info "Found $TIME_WAIT_COUNT socket(s) in TIME_WAIT state on port $DEFAULT_PORT"
        netstat -tan | grep ":$DEFAULT_PORT" | grep "TIME_WAIT" | sed 's/^/    /'
    else
        print_info "No TIME_WAIT sockets found on port $DEFAULT_PORT"
    fi
fi

# Also check the custom port if it's different from the default
if [ "$DEFAULT_PORT" != "$CUSTOM_PORT" ]; then
    print_info "Also checking for sockets in TIME_WAIT state on port $CUSTOM_PORT..."
    if command -v ss &> /dev/null; then
        TIME_WAIT_COUNT=$(ss -tan | grep ":$CUSTOM_PORT" | grep -c "TIME-WAIT" 2>/dev/null || echo 0)
        TIME_WAIT_COUNT=$(echo "$TIME_WAIT_COUNT" | tr -d '[:space:]')
        if [ -z "$TIME_WAIT_COUNT" ]; then
            TIME_WAIT_COUNT=0
        fi
        
        if [ "$TIME_WAIT_COUNT" -gt 0 ]; then
            print_info "Found $TIME_WAIT_COUNT socket(s) in TIME-WAIT state on port $CUSTOM_PORT"
            ss -tan | grep ":$CUSTOM_PORT" | grep "TIME-WAIT" | sed 's/^/    /'
        else
            print_info "No TIME-WAIT sockets found on port $CUSTOM_PORT"
        fi
    elif command -v netstat &> /dev/null; then
        TIME_WAIT_COUNT=$(netstat -tan | grep ":$CUSTOM_PORT" | grep -c "TIME_WAIT" 2>/dev/null || echo 0)
        TIME_WAIT_COUNT=$(echo "$TIME_WAIT_COUNT" | tr -d '[:space:]')
        if [ -z "$TIME_WAIT_COUNT" ]; then
            TIME_WAIT_COUNT=0
        fi
        
        if [ "$TIME_WAIT_COUNT" -gt 0 ]; then
            print_info "Found $TIME_WAIT_COUNT socket(s) in TIME_WAIT state on port $CUSTOM_PORT"
            netstat -tan | grep ":$CUSTOM_PORT" | grep "TIME_WAIT" | sed 's/^/    /'
        else
            print_info "No TIME_WAIT sockets found on port $CUSTOM_PORT"
        fi
    fi
fi

# Add a longer wait period for socket release
print_info "Waiting for TIME_WAIT sockets to clear on ports $DEFAULT_PORT and $CUSTOM_PORT (if any)..."
WAIT_START=$(date +%s)
WAIT_TIMEOUT=30  # Maximum seconds to wait

# Wait in a loop until ports are genuinely available or timeout occurs
for i in $(seq 1 $WAIT_TIMEOUT); do
    DEFAULT_PORT_FREE=true
    CUSTOM_PORT_FREE=true
    
    # Check default port
    if check_port_in_use $DEFAULT_PORT; then
        DEFAULT_PORT_FREE=false
    fi
    
    # Check custom port if different
    if [ "$DEFAULT_PORT" != "$CUSTOM_PORT" ] && check_port_in_use $CUSTOM_PORT; then
        CUSTOM_PORT_FREE=false
    fi
    
    # If both ports are free, we can continue
    if $DEFAULT_PORT_FREE && $CUSTOM_PORT_FREE; then
        WAIT_END=$(date +%s)
        WAIT_DURATION=$((WAIT_END - WAIT_START))
        print_info "All ports are now available after waiting ${WAIT_DURATION} seconds"
        
        # Additional verification for default port
        IS_DEFAULT_PORT_TRULY_FREE=true
        if command -v lsof >/dev/null 2>&1; then
            LSOF_CHECK=$(lsof -i :$DEFAULT_PORT 2>/dev/null)
            if [ -n "$LSOF_CHECK" ]; then
                print_warning "lsof still shows port $DEFAULT_PORT in use:"
                echo "$LSOF_CHECK" | sed 's/^/    /'
                IS_DEFAULT_PORT_TRULY_FREE=false
            fi
        fi
        
        # Additional verification for custom port if different
        IS_CUSTOM_PORT_TRULY_FREE=true
        if [ "$DEFAULT_PORT" != "$CUSTOM_PORT" ] && command -v lsof >/dev/null 2>&1; then
            LSOF_CHECK=$(lsof -i :$CUSTOM_PORT 2>/dev/null)
            if [ -n "$LSOF_CHECK" ]; then
                print_warning "lsof still shows port $CUSTOM_PORT in use:"
                echo "$LSOF_CHECK" | sed 's/^/    /'
                IS_CUSTOM_PORT_TRULY_FREE=false
            fi
        fi
        
        # If both ports are truly free, break out of the loop
        if $IS_DEFAULT_PORT_TRULY_FREE && $IS_CUSTOM_PORT_TRULY_FREE; then
            break
        fi
    fi
    
    # Report status
    if ! $DEFAULT_PORT_FREE; then
        print_info "Waiting for port $DEFAULT_PORT to be released (attempt $i/$WAIT_TIMEOUT)..."
    fi
    if [ "$DEFAULT_PORT" != "$CUSTOM_PORT" ] && ! $CUSTOM_PORT_FREE; then
        print_info "Waiting for port $CUSTOM_PORT to be released (attempt $i/$WAIT_TIMEOUT)..."
    fi
    
    sleep 1
    
    # Every 5 seconds, try more aggressive cleanup
    if [ $((i % 5)) -eq 0 ]; then
        print_info "Attempting more aggressive cleanup..."
        
        # Kill any processes that might be using the ports
        if command -v fuser >/dev/null 2>&1; then
            if ! $DEFAULT_PORT_FREE; then
                fuser -k $DEFAULT_PORT/tcp >/dev/null 2>&1 || true
            fi
            if [ "$DEFAULT_PORT" != "$CUSTOM_PORT" ] && ! $CUSTOM_PORT_FREE; then
                fuser -k $CUSTOM_PORT/tcp >/dev/null 2>&1 || true
            fi
        fi
        
        # Display current socket state
        if command -v ss >/dev/null 2>&1; then
            if ! $DEFAULT_PORT_FREE; then
                print_info "Current socket states for port $DEFAULT_PORT:"
                ss -tan | grep ":$DEFAULT_PORT" | sed 's/^/    /' || echo "    No sockets found"
            fi
            if [ "$DEFAULT_PORT" != "$CUSTOM_PORT" ] && ! $CUSTOM_PORT_FREE; then
                print_info "Current socket states for port $CUSTOM_PORT:"
                ss -tan | grep ":$CUSTOM_PORT" | sed 's/^/    /' || echo "    No sockets found"
            fi
        fi
    fi
done

# Final check before proceeding
PORT_CHECK_FAILED=false

# Check default port
if check_port_in_use $DEFAULT_PORT; then
    print_warning "Port $DEFAULT_PORT is still not available after maximum wait time!"
    print_info "Socket details for port $DEFAULT_PORT:"
    if command -v ss >/dev/null 2>&1; then
        ss -tan | grep ":$DEFAULT_PORT" | sed 's/^/    /'
    elif command -v netstat >/dev/null 2>&1; then
        netstat -tan | grep ":$DEFAULT_PORT" | sed 's/^/    /'
    fi
    
    # Let's see if anything is still binding to it
    if command -v lsof >/dev/null 2>&1; then
        print_info "Processes using port $DEFAULT_PORT:"
        lsof -i :$DEFAULT_PORT 2>/dev/null | sed 's/^/    /' || echo "    No processes found using lsof"
    fi
    PORT_CHECK_FAILED=true
fi

# Check custom port if different
if [ "$DEFAULT_PORT" != "$CUSTOM_PORT" ] && check_port_in_use $CUSTOM_PORT; then
    print_warning "Port $CUSTOM_PORT is still not available after maximum wait time!"
    print_info "Socket details for port $CUSTOM_PORT:"
    if command -v ss >/dev/null 2>&1; then
        ss -tan | grep ":$CUSTOM_PORT" | sed 's/^/    /'
    elif command -v netstat >/dev/null 2>&1; then
        netstat -tan | grep ":$CUSTOM_PORT" | sed 's/^/    /'
    fi
    
    # Let's see if anything is still binding to it
    if command -v lsof >/dev/null 2>&1; then
        print_info "Processes using port $CUSTOM_PORT:"
        lsof -i :$CUSTOM_PORT 2>/dev/null | sed 's/^/    /' || echo "    No processes found using lsof"
    fi
    PORT_CHECK_FAILED=true
fi

if $PORT_CHECK_FAILED; then
    print_info "Trying alternate port handling - will continue anyway but test may fail"
else
    print_info "All required ports are confirmed available for tests"
fi

# Add a small delay to ensure system stability before starting the next test
sleep 3

# Test with custom API prefix configuration (/myapi prefix)
test_api_prefix "$CUSTOM_CONFIG_PATH" "/myapi" "custom"
CUSTOM_PREFIX_RESULT=$?

# Calculate overall test result
if [ $DEFAULT_PREFIX_RESULT -eq 0 ] && [ $CUSTOM_PREFIX_RESULT -eq 0 ]; then
    TEST_RESULT=0
    print_result 0 "All API prefix tests passed successfully"
else
    TEST_RESULT=1
    print_result 1 "Some API prefix tests failed"
fi

# Ensure all servers are stopped at the end
print_info "Cleaning up any remaining server processes..."
REMAINING_PROCESSES=$(pgrep -f "hydrogen.*json" || echo "")
if [ -n "$REMAINING_PROCESSES" ]; then
    print_warning "Found lingering hydrogen processes after tests:"
    ps -f -p $REMAINING_PROCESSES | sed 's/^/    /'
    print_info "Terminating remaining processes..."
    pkill -f "hydrogen.*json" 2>/dev/null
    sleep 1
    
    # Check for stubborn processes and force kill if necessary
    REMAINING_PROCESSES=$(pgrep -f "hydrogen.*json" || echo "")
    if [ -n "$REMAINING_PROCESSES" ]; then
        print_warning "Some processes still remain, using SIGKILL..."
        ps -f -p $REMAINING_PROCESSES | sed 's/^/    /'
        pkill -9 -f "hydrogen.*json" 2>/dev/null
    fi
else
    print_info "No lingering hydrogen processes found"
fi

# End the test with final result
end_test $TEST_RESULT "API Prefix Test"

# Clean up response files but preserve logs if test failed
rm -f $RESULTS_DIR/response_*.json

# Only remove logs if tests were successful
if [ $TEST_RESULT -eq 0 ]; then
    print_info "Tests passed, cleaning up log files..."
    rm -f $RESULTS_DIR/hydrogen_test_*.log
else
    print_warning "Tests failed, preserving log files for analysis in $RESULTS_DIR/"
    # Create a dated backup of log files
    LOG_BACKUP_DIR="$RESULTS_DIR/failed_$(date +%Y%m%d_%H%M%S)"
    mkdir -p "$LOG_BACKUP_DIR"
    cp $RESULTS_DIR/hydrogen_test_*.log "$LOG_BACKUP_DIR/" 2>/dev/null
    print_info "Log files backed up to $LOG_BACKUP_DIR/"
fi

# Final system state
print_info "Final system state:"
print_info "  Time: $(date)"
if command -v free >/dev/null 2>&1; then
    print_info "  Memory summary:"
    free -h | sed 's/^/    /'
fi

exit $TEST_RESULT