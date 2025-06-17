#!/bin/bash
#
# About this Script
#
# Hydrogen API Prefix Test
# Tests the API endpoints with different API prefix configurations:
# - Default "/api" prefix using standard hydrogen_test_api.json
# - Custom "/myapi" prefix using hydrogen_test_api_prefix.json
#
NAME_SCRIPT="Hydrogen API Prefix Test"
VERS_SCRIPT="2.0.0"

# VERSION HISTORY
# 2.0.0 - 2025-06-17 - Major refactoring: fixed all shellcheck warnings, improved modularity, enhanced comments

# Display script name and version
echo "=== $NAME_SCRIPT v$VERS_SCRIPT ==="

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
    
    # Log the full command to test details
    echo "[$(date '+%Y-%m-%d %H:%M:%S')] Testing $request_name" >> "$RESULTS_DIR/test_details.log"
    echo "Command: $curl_command" >> "$RESULTS_DIR/test_details.log"
    
    # Execute the command
    eval "$curl_command > $response_file"
    local curl_status=$?
    
    # Check if the request was successful
    if [ $curl_status -eq 0 ]; then
        # Log full response to test details
        {
            echo "Response received:"
            cat "$response_file"
            echo ""
        } >> "$RESULTS_DIR/test_details.log"
        
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
    local max_attempts=10  # 5 seconds total (0.5s * 10)
    local attempt=1
    
    print_info "Waiting for server to be ready at $base_url..."
    
    while [ $attempt -le $max_attempts ]; do
        if curl -s --max-time 1 "${base_url}" > /dev/null 2>&1; then
            print_info "Server is ready after $(( attempt * 5 / 10 )) seconds"
            return 0
        fi
        sleep 0.5
        ((attempt++))
    done
    
    print_error "Server failed to respond within 5 seconds"
    return 1
}

# Function to check if a port is in use
check_port_in_use() {
    local port="$1"
    if command -v lsof >/dev/null 2>&1; then
        lsof -i :"$port" >/dev/null 2>&1
        return $?
    elif command -v netstat >/dev/null 2>&1; then
        netstat -tuln | grep -q ":$port "
        return $?
    else
        # Fallback method - try to connect to the port
        (echo > "/dev/tcp/127.0.0.1/$port") >/dev/null 2>&1
        return $?
    fi
}

# Function to display process details
show_process_details() {
    local pid="$1"
    local process_name="$2"
    
    if [ -z "$pid" ]; then
        print_warning "No PID found for $process_name"
        return 1
    fi
    
    # Log process details to test_details.log
    {
        echo "[$(date '+%Y-%m-%d %H:%M:%S')] Process Details for $process_name (PID: $pid):"
        
        # Command line
        if [ -e "/proc/$pid/cmdline" ]; then
            echo "  Command line: $(tr '\0' ' ' < "/proc/$pid/cmdline")"
        else
            echo "  Command line: $(ps -p "$pid" -o args= 2>/dev/null || echo "Unknown")"
        fi
        
        # Resource usage
        if command -v ps >/dev/null 2>&1; then
            echo "  Resources: $(ps -p "$pid" -o %cpu,%mem,vsz,rss 2>/dev/null || echo "Unknown")"
        fi
        
        # Status
        if [ -e "/proc/$pid/status" ]; then
            echo "  Status:"
            grep -E '^State:|^Threads:|^VmRSS:|^VmSize:' "/proc/$pid/status" | sed 's/^/    /'
        fi
        echo ""
    } >> "$RESULTS_DIR/test_details.log"
    
    # Only show minimal info in console
    print_info "Process $process_name (PID: $pid) details logged"
    
    return 0
}

# Function to test system endpoints with a specific API prefix
test_api_prefix() {
    local config_file="$1"
    local api_prefix="$2"
    local test_name="$3"
    
    print_header "Testing API Prefix: $api_prefix (using $test_name)"
    
    # Extract the WebServer port from the configuration file
    local web_server_port
    web_server_port=$(extract_web_server_port "$config_file")
    print_info "Using WebServer port: $web_server_port from configuration"
    
    # System information
    print_info "System Information:"
    print_info "  Hostname: $(hostname)"
    print_info "  Kernel: $(uname -r)"
    print_info "  Architecture: $(uname -m)"
    print_info "  Current time: $(date)"
    
    # Check for port conflicts and log details
    print_info "Checking port availability..."
    {
        echo "[$(date '+%Y-%m-%d %H:%M:%S')] Port availability check for $web_server_port:"
        if check_port_in_use "$web_server_port"; then
            echo "WARNING: Port $web_server_port is in use"
            echo "Processes using port $web_server_port:"
            if command -v lsof >/dev/null 2>&1; then
                lsof -i :"$web_server_port" 2>/dev/null
            elif command -v netstat >/dev/null 2>&1; then
                netstat -tulnp 2>/dev/null | grep ":$web_server_port "
            fi
            print_warning "Port $web_server_port is in use (see logs for details)"
        else
            echo "Port $web_server_port is available"
            print_info "Port $web_server_port is available"
        fi
        echo ""
    } >> "$RESULTS_DIR/test_details.log"
    
    # Results dictionary to track test results
    local health_result=1
    local info_result=1
    local info_json_result=1
    local test_basic_get_result=1
    
    # Determine the project root directory without changing the current directory
    local project_root
    project_root="$(cd "$(dirname "$0")/.." && pwd)"
    local relative_root
    relative_root="$(convert_to_relative_path "$project_root")"
    
    # Find the correct hydrogen binary - prefer release build
    local hydrogen_bin=""
    
    # First check for binaries in the project root
    if [ -f "$project_root/hydrogen_release" ]; then
        hydrogen_bin="$project_root/hydrogen_release"
        print_info "Using release build for testing"
    elif [ -f "$project_root/hydrogen" ]; then
        hydrogen_bin="$project_root/hydrogen"
        print_info "Using standard build"
    elif [ -f "$project_root/hydrogen_debug" ]; then
        hydrogen_bin="$project_root/hydrogen_debug"
        print_info "Using debug build for testing"
    elif [ -f "$project_root/hydrogen_perf" ]; then
        hydrogen_bin="$project_root/hydrogen_perf"
        print_info "Using performance build for testing"
    elif [ -f "$project_root/hydrogen_valgrind" ]; then
        hydrogen_bin="$project_root/hydrogen_valgrind"
        print_info "Using valgrind build for testing"
    else
        print_result 1 "ERROR: Could not find any hydrogen executable variant"
        print_info "Working directory: $relative_root"
        print_info "Available files:"
        # Use find instead of ls | grep to avoid SC2010
        find "$project_root" -maxdepth 1 -name "*hydrogen*" -type f
        return 1
    fi

    # Start hydrogen server in background with appropriate configuration
    print_info "Starting hydrogen server ($(basename "$hydrogen_bin")) with configuration ($(convert_to_relative_path "$config_file"))..."
    print_info "Startup command: $(convert_to_relative_path "$hydrogen_bin") $(convert_to_relative_path "$config_file")"
    
    # Record process start time
    local start_time
    start_time=$(date +%s)
    
    # Run from the current directory (tests/) to ensure all output stays in tests/ directory
    "$hydrogen_bin" "$config_file" > "$RESULTS_DIR/hydrogen_test_${test_name}.log" 2>&1 &
    local init_pid=$!
    
    print_info "Initial PID: $init_pid (from shell)"
    
    # Find the actual PID using pgrep
    local actual_pid
    actual_pid=$(pgrep -f ".*hydrogen.*$(basename "$config_file")" || echo "")
    
    # If we have multiple PIDs, take the first one
    if [[ "$actual_pid" == *$'\n'* ]]; then
        print_warning "Multiple PIDs found that match our pattern:"
        printf '    %s\n' "$actual_pid"
        actual_pid=$(echo "$actual_pid" | head -n1)
        print_info "Using first PID: $actual_pid"
    fi
    
    if [ -n "$actual_pid" ]; then
        print_info "Found actual Hydrogen PID: $actual_pid"
        
        # Get detailed information about the process
        show_process_details "$actual_pid" "Hydrogen Server"
        
        # Wait for the server to be ready
        local base_url="http://localhost:${web_server_port}"
        wait_for_server "${base_url}" || {
            print_error "Server failed to start properly"
            return 1
        }
        
        # Verify the process is still running
        if kill -0 "$actual_pid" 2>/dev/null; then
            print_info "Server is running (PID: $actual_pid)"
            
            # Check if the server is accepting connections
            if check_port_in_use "$web_server_port"; then
                print_info "Server is listening on port $web_server_port"
            else
                print_warning "Server does not appear to be listening on port $web_server_port!"
                # Show open ports from the process
                if command -v lsof >/dev/null 2>&1; then
                    print_info "Open ports for PID $actual_pid:"
                    lsof -p "$actual_pid" -i -P | sed 's/^/    /' || echo "    None found"
                fi
            fi
        else
            print_warning "Server process appears to have died during initialization!"
            print_info "Last 20 lines of server log:"
            tail -n 20 "$RESULTS_DIR/hydrogen_test_${test_name}.log" | sed 's/^/    /'
        fi
    else
        print_warning "Could not find hydrogen server PID. Server may have failed to start."
        print_info "Process list (pgrep hydrogen):"
        pgrep -l hydrogen | sed 's/^/    /' || echo "    No hydrogen processes found"
        print_info "Last 20 lines of server log:"
        tail -n 20 "$RESULTS_DIR/hydrogen_test_${test_name}.log" | sed 's/^/    /'
    fi
    
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
    actual_pid=$(pgrep -f ".*hydrogen.*$(basename "$config_file")" || echo "")
    
    # Check server stability
    local stability_result
    if [ -n "$actual_pid" ]; then
        print_info "Server is still running after tests (PID: $actual_pid)"
        
        # Show process details before stopping
        show_process_details "$actual_pid" "Hydrogen Server (before shutdown)"
        
        # Check how long the server has been running
        if [ -e "/proc/$actual_pid/stat" ]; then
            local proc_stat start_time_ticks clk_tck uptime proc_uptime
            proc_stat=$(cat "/proc/$actual_pid/stat")
            start_time_ticks=$(echo "$proc_stat" | awk '{print $22}')
            clk_tck=$(getconf CLK_TCK)
            uptime=$(awk '{print $1}' /proc/uptime)
            proc_uptime=$(echo "scale=2; $uptime - ($start_time_ticks / $clk_tck)" | bc 2>/dev/null || echo "Unknown")
            print_info "Server uptime: approximately $proc_uptime seconds"
        else
            # Fallback: Calculate from our recorded start time
            local current_time uptime_calc
            current_time=$(date +%s)
            uptime_calc=$((current_time - start_time))
            print_info "Server uptime: approximately $uptime_calc seconds (from script timing)"
        fi
        
        stability_result=0
        
        # Log detailed network information and show summary
        {
            echo "[$(date '+%Y-%m-%d %H:%M:%S')] Network connections before shutdown:"
            if command -v lsof >/dev/null 2>&1; then
                local connections connection_count
                connections=$(lsof -p "$actual_pid" -i -P 2>/dev/null)
                if [ -n "$connections" ]; then
                    echo "$connections"
                    connection_count=$(echo "$connections" | wc -l)
                    print_info "Network connections before shutdown: $connection_count connections"
                else
                    echo "No network connections found"
                    print_info "Network connections before shutdown: 0 connections"
                fi
            elif command -v netstat >/dev/null 2>&1; then
                local connections connection_count
                connections=$(netstat -tulnp 2>/dev/null | grep "$actual_pid")
                if [ -n "$connections" ]; then
                    echo "$connections"
                    connection_count=$(echo "$connections" | wc -l)
                    print_info "Network connections before shutdown: $connection_count connections"
                else
                    echo "No network connections found"
                    print_info "Network connections before shutdown: 0 connections"
                fi
            fi
            echo ""
        } >> "$RESULTS_DIR/test_details.log"
        
        # Stop the server using utility function
        stop_hydrogen_server "$actual_pid" 1
        
        # Check for zombie processes
        local zombie_processes
            zombie_processes=$(pgrep -l -f "defunct.*[h]ydrogen" || echo "")
        if [ -n "$zombie_processes" ]; then
            print_warning "Zombie hydrogen processes detected:"
            printf '    %s\n' "$zombie_processes"
        fi
    else
        print_result 1 "ERROR: Server has crashed or exited prematurely"
        print_info "Last 30 lines of server log:"
        tail -n 30 "$RESULTS_DIR/hydrogen_test_${test_name}.log" | sed 's/^/    /'
        stability_result=1
    fi
    
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

# Function to check socket states and handle TIME_WAIT
check_socket_states() {
    local port="$1"
    local port_name="$2"
    
    print_info "Checking socket states for $port_name..."
    {
        echo "[$(date '+%Y-%m-%d %H:%M:%S')] Checking TIME_WAIT sockets for port $port"
        if command -v ss &> /dev/null; then
            local time_wait_count
            time_wait_count=$(ss -tan | grep ":$port" | grep -c "TIME-WAIT" 2>/dev/null || echo 0)
            time_wait_count=$(echo "$time_wait_count" | tr -d '[:space:]')
            [ -z "$time_wait_count" ] && time_wait_count=0
            
            echo "Socket state details (ss):"
            ss -tan | grep ":$port" || echo "No sockets found"
            
            if [ "$time_wait_count" -gt 0 ]; then
                echo "Found $time_wait_count TIME-WAIT socket(s)"
                if command -v sysctl &> /dev/null && [ -w /proc/sys/net/ipv4/tcp_tw_reuse ]; then
                    local old_tw_reuse
                    old_tw_reuse=$(cat /proc/sys/net/ipv4/tcp_tw_reuse)
                    sudo sysctl -w net.ipv4.tcp_tw_reuse=1 > /dev/null 2>&1
                    echo "tcp_tw_reuse changed from $old_tw_reuse to $(cat /proc/sys/net/ipv4/tcp_tw_reuse 2>/dev/null || echo 'unknown')"
                fi
                print_info "Found $time_wait_count TIME-WAIT socket(s) - see logs for details"
            else
                echo "No TIME-WAIT sockets found"
                print_info "No TIME-WAIT sockets found"
            fi
        elif command -v netstat &> /dev/null; then
            local time_wait_count
            time_wait_count=$(netstat -tan | grep ":$port" | grep -c "TIME_WAIT" 2>/dev/null || echo 0)
            time_wait_count=$(echo "$time_wait_count" | tr -d '[:space:]')
            [ -z "$time_wait_count" ] && time_wait_count=0
            
            echo "Socket state details (netstat):"
            netstat -tan | grep ":$port" || echo "No sockets found"
            
            if [ "$time_wait_count" -gt 0 ]; then
                echo "Found $time_wait_count TIME-WAIT socket(s)"
                print_info "Found $time_wait_count TIME-WAIT socket(s) - see logs for details"
            else
                echo "No TIME-WAIT sockets found"
                print_info "No TIME-WAIT sockets found"
            fi
        fi
        echo ""
    } >> "$RESULTS_DIR/test_details.log"
}

# Function to wait for ports to be available
wait_for_ports_available() {
    local default_port="$1"
    local custom_port="$2"
    local wait_timeout=5  # Reduced from 30 to 5 seconds since we're actively monitoring
    
    print_info "Waiting for TIME_WAIT sockets to clear on ports $default_port and $custom_port (if any)..."
    
    # Check ports in a loop with shorter interval
    for _ in $(seq 1 $wait_timeout); do
        if ! check_port_in_use "$default_port" && \
           ([ "$default_port" = "$custom_port" ] || ! check_port_in_use "$custom_port"); then
            print_info "All ports are now available"
            break
        fi
        sleep 1
    done
}

# Function to perform final port checks
perform_final_port_checks() {
    local default_port="$1"
    local custom_port="$2"
    local port_check_failed=false
    
    # Check default port
    if check_port_in_use "$default_port"; then
        print_warning "Port $default_port is still not available after maximum wait time!"
        print_info "Socket details for port $default_port:"
        if command -v ss >/dev/null 2>&1; then
            ss -tan | grep ":$default_port" | sed 's/^/    /'
        elif command -v netstat >/dev/null 2>&1; then
            netstat -tan | grep ":$default_port" | sed 's/^/    /'
        fi
        
        # Let's see if anything is still binding to it
        if command -v lsof >/dev/null 2>&1; then
            print_info "Processes using port $default_port:"
            lsof -i :"$default_port" 2>/dev/null | sed 's/^/    /' || echo "    No processes found using lsof"
        fi
        port_check_failed=true
    fi
    
    # Check custom port if different
    if [ "$default_port" != "$custom_port" ] && check_port_in_use "$custom_port"; then
        print_warning "Port $custom_port is still not available after maximum wait time!"
        print_info "Socket details for port $custom_port:"
        if command -v ss >/dev/null 2>&1; then
            ss -tan | grep ":$custom_port" | sed 's/^/    /'
        elif command -v netstat >/dev/null 2>&1; then
            netstat -tan | grep ":$custom_port" | sed 's/^/    /'
        fi
        
        # Let's see if anything is still binding to it
        if command -v lsof >/dev/null 2>&1; then
            print_info "Processes using port $custom_port:"
            lsof -i :"$custom_port" 2>/dev/null | sed 's/^/    /' || echo "    No processes found using lsof"
        fi
        port_check_failed=true
    fi
    
    if $port_check_failed; then
        print_info "Trying alternate port handling - will continue anyway but test may fail"
    else
        print_info "All required ports are confirmed available for tests"
    fi
}

# Function to cleanup remaining processes
cleanup_remaining_processes() {
    print_info "Cleaning up any remaining server processes..."
    local remaining_processes
    remaining_processes=$(pgrep -f "hydrogen.*json" || echo "")
    if [ -n "$remaining_processes" ]; then
        print_warning "Found lingering hydrogen processes after tests:"
        ps -f -p "$remaining_processes" | sed 's/^/    /'
        print_info "Terminating remaining processes..."
        pkill -f "hydrogen.*json" 2>/dev/null
        sleep 1
        
        # Check for stubborn processes and force kill if necessary
        remaining_processes=$(pgrep -f "hydrogen.*json" || echo "")
        if [ -n "$remaining_processes" ]; then
            print_warning "Some processes still remain, using SIGKILL..."
            ps -f -p "$remaining_processes" | sed 's/^/    /'
            pkill -9 -f "hydrogen.*json" 2>/dev/null
        fi
    else
        print_info "No lingering hydrogen processes found"
    fi
}

# Function to log final system state
log_final_system_state() {
    {
        echo "[$(date '+%Y-%m-%d %H:%M:%S')] Final System State:"
        echo "Time: $(date)"
        if command -v free >/dev/null 2>&1; then
            echo "Memory summary:"
            free -h
        fi
        if command -v uptime >/dev/null 2>&1; then
            echo "System load:"
            uptime
        fi
    } >> "$RESULTS_DIR/test_details.log"
    print_info "Test complete - detailed system state logged"
}

# Main execution starts here

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

# Check for TIME_WAIT sockets and log detailed socket state
check_socket_states "$DEFAULT_PORT" "default port"

# Also check the custom port if it's different from the default
if [ "$DEFAULT_PORT" != "$CUSTOM_PORT" ]; then
    check_socket_states "$CUSTOM_PORT" "custom port"
fi

# Wait for ports to be released
wait_for_ports_available "$DEFAULT_PORT" "$CUSTOM_PORT"

# Final check before proceeding
perform_final_port_checks "$DEFAULT_PORT" "$CUSTOM_PORT"

# Add a small delay to ensure system stability before starting the next test
sleep 3

# Test with custom API prefix configuration (/myapi prefix)
test_api_prefix "$CUSTOM_CONFIG_PATH" "/myapi" "custom"
CUSTOM_PREFIX_RESULT=$?

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

# Cleanup and finalization
cleanup_remaining_processes

# End the test with final result
end_test $TEST_RESULT "API Prefix Test"

# Clean up response files but preserve logs if test failed
rm -f "$RESULTS_DIR"/response_*.json

# Only remove logs if tests were successful
if [ $TEST_RESULT -eq 0 ]; then
    print_info "Tests passed, cleaning up log files..."
    rm -f "$RESULTS_DIR"/hydrogen_test_*.log
else
    print_warning "Tests failed, preserving log files for analysis in $RESULTS_DIR/"
    # Create a dated backup of log files
    LOG_BACKUP_DIR="$RESULTS_DIR/failed_$(date +%Y%m%d_%H%M%S)"
    mkdir -p "$LOG_BACKUP_DIR"
    cp "$RESULTS_DIR"/hydrogen_test_*.log "$LOG_BACKUP_DIR/" 2>/dev/null
    print_info "Log files backed up to $LOG_BACKUP_DIR/"
fi

# Log final system state
log_final_system_state

exit $TEST_RESULT
