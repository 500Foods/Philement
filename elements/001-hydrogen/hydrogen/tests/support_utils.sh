#!/bin/bash
#
# Hydrogen Test Utilities
# Provides common functions for test scripts to ensure consistent formatting and reporting
#
NAME_SCRIPT="Hydrogen Test Utilities"
VERS_SCRIPT="2.0.0"

# VERSION HISTORY
# 2.0.0 - 2025-06-17 - Major refactoring: fixed all shellcheck warnings, improved modularity, enhanced comments

# Function to display script version information
print_utils_version() {
    echo "=== $NAME_SCRIPT v$VERS_SCRIPT ==="
}

# Set up color codes for better readability
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
# MAGENTA='\033[0;35m'  # Currently unused, commented out to avoid SC2034
BOLD='\033[1m'
NC='\033[0m' # No Color

# Icons/symbols for test results
PASS_ICON="✅"
FAIL_ICON="❌"
WARN_ICON="⚠️ "
INFO_ICON="🛈 "

# Function to print section headers
print_header() {
    echo -e "\n${BLUE}───────────────────────────────────────────────────────────────────────────────${NC}"
    echo -e "${BLUE}${BOLD}$1${NC}"
    echo -e "${BLUE}───────────────────────────────────────────────────────────────────────────────${NC}"
}

# Function to print test results
print_result() {
    local status=$1
    local message=$2
    
    if [ "$status" -eq 0 ]; then
        echo -e "${GREEN}${PASS_ICON} $message${NC}"
    else
        echo -e "${RED}${FAIL_ICON} $message${NC}"
    fi
}

# Function to print a warning
print_warning() {
    echo -e "${YELLOW}${WARN_ICON} $1${NC}"
}

# Function to print an error message
print_error() {
    echo -e "${RED}${FAIL_ICON} $1${NC}"
}

# Function to print an informational message
print_info() {
    echo -e "${CYAN}${INFO_ICON} $1${NC}"
}

# Function to print a command that will be executed
print_command() {
    # Convert any absolute paths in the command to relative paths
    # Look for paths containing /elements/001-hydrogen/hydrogen/ and convert them
    local cmd
    cmd=$(echo "$1" | sed -E "s|/[[:alnum:]/_-]+/elements/001-hydrogen/hydrogen/|hydrogen/|g")
    echo -e "${YELLOW}Executing: $cmd${NC}"
}

# Function to start a test run with proper header
start_test() {
    local test_name=$1
    print_header "$test_name"
    echo "Started at: $(date)"
    echo ""
}

# Function to end a test run with proper footer
end_test() {
    local test_result=$1
    local test_name=$2
    
    print_header "Test Summary"
    echo "Completed at: $(date)"
    
    if [ "$test_result" -eq 0 ]; then
        echo -e "${GREEN}${BOLD}${PASS_ICON} OVERALL RESULT: ALL TESTS PASSED${NC}"
    else
        echo -e "${RED}${BOLD}${FAIL_ICON} OVERALL RESULT: ONE OR MORE TESTS FAILED${NC}"
    fi
    echo -e "${BLUE}───────────────────────────────────────────────────────────────────────────────${NC}"
    echo ""
    
    return "$test_result"
}

# Function to print a test summary with details
print_test_summary() {
    local total=$1
    local passed=$2
    local failed=$3
    
    echo -e "\n${BLUE}${BOLD}Summary Statistics:${NC}"
    echo -e "  Total Tests: $total"
    echo -e "  ${GREEN}Passed: $passed${NC}"
    if [ "$failed" -gt 0 ]; then
        echo -e "  ${RED}Failed: $failed${NC}"
    else
        echo -e "  Failed: $failed"
    fi
    echo ""
}

# Function to print individual test results for a summary
print_test_item() {
    local status=$1
    local name=$2
    local details=$3
    
    if [ "$status" -eq 0 ]; then
        echo -e "  ${GREEN}${PASS_ICON} PASS${NC}: ${BOLD}$name${NC} - $details"
    else
        echo -e "  ${RED}${FAIL_ICON} FAIL${NC}: ${BOLD}$name${NC} - $details"
    fi
}

# Function to generate an HTML summary report for a test
generate_html_report() {
    local result_file=$1
    local html_file="${result_file%.log}.html"
    
    # Create HTML header
    cat > "$html_file" << EOF
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Hydrogen Test Results</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            line-height: 1.6;
            color: #333;
            max-width: 1000px;
            margin: 0 auto;
            padding: 20px;
        }
        h1, h2, h3 {
            color: #0066cc;
        }
        .test-summary {
            margin: 20px 0;
            padding: 15px;
            border-radius: 5px;
            background-color: #f5f5f5;
        }
        .test-header {
            background-color: #0066cc;
            color: white;
            padding: 10px 15px;
            border-radius: 5px 5px 0 0;
            margin-bottom: 0;
        }
        .test-body {
            border: 1px solid #ddd;
            border-top: none;
            border-radius: 0 0 5px 5px;
            padding: 15px;
            background-color: white;
        }
        .pass {
            color: #00aa00;
            font-weight: bold;
        }
        .fail {
            color: #dd0000;
            font-weight: bold;
        }
        .warning {
            color: #ff9900;
            font-weight: bold;
        }
        .stats {
            display: flex;
            justify-content: space-between;
            max-width: 300px;
            margin: 10px 0;
        }
        .test-item {
            margin: 10px 0;
            padding: 10px;
            border-radius: 5px;
            background-color: #f9f9f9;
        }
        .pass-icon:before {
            content: "✅ ";
        }
        .fail-icon:before {
            content: "❌ ";
        }
        .warn-icon:before {
            content: "⚠️ ";
        }
        .timestamp {
            color: #666;
            font-size: 0.9em;
        }
        pre {
            background-color: #f0f0f0;
            padding: 10px;
            border-radius: 3px;
            overflow-x: auto;
        }
    </style>
</head>
<body>
    <h1>Hydrogen Test Results</h1>
    <p class="timestamp">Generated on $(date)</p>
EOF
    
    # Parse the log file and generate the report content
    # This is a placeholder - each test would need to call this with appropriate parsing
    
    # Close the HTML
    cat >> "$html_file" << EOF
</body>
</html>
EOF
    
    echo "HTML report generated: $html_file"
    return 0
}

# Function to convert absolute path to path relative to hydrogen project root
convert_to_relative_path() {
    local absolute_path="$1"
    
    # Extract the part starting from "hydrogen" and keep everything after
    local relative_path
    relative_path=$(echo "$absolute_path" | sed -n 's|.*/hydrogen/|hydrogen/|p')
    
    # If the path contains elements/001-hydrogen/hydrogen but not starting with hydrogen/
    if [ -z "$relative_path" ]; then
        relative_path=$(echo "$absolute_path" | sed -n 's|.*/elements/001-hydrogen/hydrogen|hydrogen|p')
    fi
    
    # If we still couldn't find a match, return the original
    if [ -z "$relative_path" ]; then
        echo "$absolute_path"
    else
        echo "$relative_path"
    fi
}

# Function to get the full path to a configuration file
# This centralizes config file access and handles the configs/ subdirectory
get_config_path() {
    local config_file="$1"
    local script_dir
    script_dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
    local config_path="$script_dir/configs/$config_file"
    
    echo "$config_path"
}

# Function to extract web server port from a JSON configuration file
extract_web_server_port() {
    local config_file="$1"
    local default_port=5000
    
    # Extract port using grep and sed (simple approach, could be improved with jq)
    if command -v jq &> /dev/null; then
        # Use jq if available for proper JSON parsing
        local port
        port=$(jq -r '.WebServer.Port // 5000' "$config_file" 2>/dev/null)
        if jq -r '.WebServer.Port // 5000' "$config_file" >/dev/null 2>&1 && [ -n "$port" ] && [ "$port" != "null" ]; then
            echo "$port"
            return 0
        fi
    fi
    
    # Fallback method using grep and sed
    local port
    port=$(grep -o '"Port":[^,}]*' "$config_file" | head -1 | sed 's/"Port":\s*\([0-9]*\)/\1/')
    if [ -n "$port" ]; then
        echo "$port"
        return 0
    fi
    
    # Return default port if extraction fails
    echo "$default_port"
    return 0
}

# Function to set up the standard test environment
setup_test_environment() {
    local test_name=$1
    local script_dir
    script_dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
    
    # Create results directory if it doesn't exist
    RESULTS_DIR="$script_dir/results"
    mkdir -p "$RESULTS_DIR"
    
    # Get timestamp for this test run
    TIMESTAMP=$(date +%Y%m%d_%H%M%S)
    
    # Create a test-specific log file
    local test_id
    test_id=$(echo "$test_name" | tr ' ' '_' | tr '[:upper:]' '[:lower:]')
    RESULT_LOG="$RESULTS_DIR/${test_id}_${TIMESTAMP}.log"
    
    # Print test start information
    start_test "$test_name" | tee -a "$RESULT_LOG"
    
    # Return log file path
    echo "$RESULT_LOG"
}

# Function to find the appropriate hydrogen binary (preferring release build)
# Removed duplicate find_hydrogen_binary function - using the one at line 536

# Function to start the hydrogen server with a specific configuration
start_hydrogen_server() {
    local hydrogen_bin=$1
    local config_file=$2
    local log_file=$3
    
    # Start in background
    print_info "Starting hydrogen server with configuration $(convert_to_relative_path "$config_file")..."
    
    # Use provided log file or default to hydrogen_test.log
    if [ -z "$log_file" ]; then
        local script_dir
        script_dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
        log_file="$script_dir/hydrogen_test.log"
    fi
    
    # Start the server and capture PID
    $hydrogen_bin "$config_file" > "$log_file" 2>&1 &
    local server_pid=$!
    
    # Wait a bit for startup
    print_info "Waiting for server to initialize (5 seconds)..."
    sleep 5
    
    # Check if process is still running
    if kill -0 "$server_pid" 2>/dev/null; then
        print_result 0 "Server started successfully with PID $server_pid"
    else
        print_result 1 "Server failed to start"
        return 1
    fi
    
    # Return the PID
    echo $server_pid
}

# Function to stop the hydrogen server
stop_hydrogen_server() {
    local server_pid=$1
    local wait_time=${2:-10}  # Default 10 seconds wait time
    
    # Check if PID exists
    if [ -z "$server_pid" ] || ! kill -0 "$server_pid" 2>/dev/null; then
        print_warning "No running server with PID $server_pid"
        return 1
    fi
    
    # Try graceful shutdown
    print_info "Stopping hydrogen server (PID $server_pid)..."
    kill -SIGINT "$server_pid"
    
    # Wait for server to stop
    print_info "Waiting for server to shut down (${wait_time} seconds)..."
    local count=0
    while kill -0 "$server_pid" 2>/dev/null && [ "$count" -lt "$wait_time" ]; do
        sleep 1
        ((count++))
    done
    
    # Check if server is still running
    if kill -0 "$server_pid" 2>/dev/null; then
        print_warning "Server didn't stop gracefully, forcing termination..."
        kill -9 "$server_pid"
        sleep 2
        if kill -0 "$server_pid" 2>/dev/null; then
            print_result 1 "Failed to stop server"
            return 1
        fi
    fi
    
    print_result 0 "Server stopped successfully"
    return 0
}

# Function to validate JSON syntax in a file
validate_json() {
    local file="$1"
    
    if command -v jq &> /dev/null; then
        # Use jq if available for proper JSON validation
        jq . "$file" > /dev/null 2>&1
        if jq . "$file" >/dev/null 2>&1; then
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

# Function to make a request and validate response
run_curl_request() {
    local request_name="$1"
    local curl_command="$2"
    local expected_field="$3"
    local response_file="response_${request_name}.json"
    local result_log="$4"
    
    # If no result log is provided, use stdout
    if [ -z "$result_log" ]; then
        result_log="/dev/stdout"
    fi
    
    # Make sure curl command includes --compressed flag and timeout
    if [[ $curl_command != *"--compressed"* ]]; then
        curl_command="${curl_command/curl/curl --compressed}"
    fi
    
    # Add timeout if not already present
    if [[ $curl_command != *"--max-time"* ]] && [[ $curl_command != *"-m "* ]]; then
        curl_command="${curl_command/curl/curl --max-time 5}"
    fi
    
    print_command "$curl_command" | tee -a "$result_log"
    eval "$curl_command > $response_file"
    CURL_STATUS=$?
    
    # Check if the request was successful
    if [ $CURL_STATUS -eq 0 ]; then
        print_info "Successfully received response:" | tee -a "$result_log"
        tee -a "$result_log" < "$response_file"
        echo "" | tee -a "$result_log"
        
        # Validate that the response contains expected fields
        if grep -q "$expected_field" "$response_file"; then
            print_result 0 "Response contains expected field: $expected_field" | tee -a "$result_log"
            return 0
        else
            print_result 1 "Response doesn't contain expected field: $expected_field" | tee -a "$result_log"
            return 1
        fi
    else
        print_result 1 "Failed to connect to server (curl status: $CURL_STATUS)" | tee -a "$result_log"
        return $CURL_STATUS
    fi
}

# Function to export the test result to a standardized JSON format for the main summary
export_test_results() {
    local test_name=$1
    local result=$2
    local details=$3
    local output_file=$4
    
    # Create a simple JSON structure
    cat > "$output_file" << EOF
{
    "test_name": "$test_name",
    "status": $result,
    "details": "$details",
    "timestamp": "$(date +%Y-%m-%d\ %H:%M:%S)"
}
EOF
}

# Function to save subtest statistics for use by test_all.sh
# This creates a file that test_all.sh can read to get the actual subtest counts
export_subtest_results() {
    local test_name=$1      # Use the passed test name
    local total_subtests=$2
    local passed_subtests=$3
    local script_dir
    script_dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
    local results_dir="$script_dir/results"
    
    # Create results directory if it doesn't exist
    mkdir -p "$results_dir"
    
    # Create the subtest results file
    local subtest_file="$results_dir/subtest_${test_name}.txt"
    echo "${total_subtests},${passed_subtests}" > "$subtest_file"
    
    print_info "Exported subtest results: ${passed_subtests} of ${total_subtests} subtests passed"
    return 0
}

# ============================================================================
# ROBUST SERVER MANAGEMENT FUNCTIONS (from test_55)
# These functions provide reliable server startup and shutdown with proper
# process validation, graceful shutdown, and adequate wait times
# ============================================================================

# Function to find appropriate hydrogen binary
find_hydrogen_binary() {
    local hydrogen_dir="$1"
    if [ -f "$hydrogen_dir/hydrogen_release" ]; then
        print_info "Using release build for testing" >&2
        echo "$hydrogen_dir/hydrogen_release"
    elif [ -f "$hydrogen_dir/hydrogen" ]; then
        print_info "Using standard build for testing" >&2
        echo "$hydrogen_dir/hydrogen"
    else
        print_error "No hydrogen binary found in $hydrogen_dir" >&2
        return 1
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
            print_warning "Startup timeout after ${elapsed}s" >&2
            return 1
        fi
        
        if grep -q "Application started" "$log_file" 2>/dev/null; then
            print_info "Startup completed in ${elapsed}s" >&2
            return 0
        fi
        
        sleep 0.2
    done
}

# Load TIME_WAIT socket management utilities
SCRIPT_DIR_UTILS="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
source "$SCRIPT_DIR_UTILS/support_timewait.sh"

# Function to start hydrogen instance (robust version from test_55)
start_hydrogen_instance_robust() {
    local hydrogen_bin="$1"
    local config_file="$2"
    local instance_name="$3"
    local results_dir="$4"
    local timestamp="$5"
    local log_file="$results_dir/${instance_name// /_}_${timestamp}.log"
    
    # Send all informational output to stderr to avoid interfering with PID capture
    print_header "Starting Hydrogen ($instance_name)" >&2
    
    # Check if binary exists and is executable
    if [ ! -x "$hydrogen_bin" ]; then
        print_error "Hydrogen binary not found or not executable: $hydrogen_bin" >&2
        return 1
    fi
    
    # Check if config file exists
    if [ ! -f "$config_file" ]; then
        print_error "Config file not found: $config_file" >&2
        return 1
    fi
    
    # Clean log file
    true > "$log_file"
    
    # Start the process with output to log file
    # shellcheck disable=SC2086  # Variables are quoted individually; shellcheck flags this due to redirect
    "$hydrogen_bin" "$config_file" > "$log_file" 2>&1 &
    local pid=$!
    
    # Verify we got a valid PID
    if [ -z "$pid" ] || [ "$pid" = "0" ]; then
        print_error "Failed to start $instance_name - invalid PID" >&2
        return 1
    fi
    
    print_info "Started with PID: $pid" >&2
    
    # Verify process started
    if ! ps -p "$pid" > /dev/null 2>&1; then
        print_error "$instance_name failed to start" >&2
        return 1
    fi
    
    # Wait for startup completion by monitoring log
    print_info "Waiting for startup completion (max 15 seconds)..." >&2
    
    if ! wait_for_startup "$log_file" 15; then
        print_error "$instance_name startup failed or timed out" >&2
        print_info "Last 10 lines of startup log:" >&2
        tail -10 "$log_file" || true >&2
        return 1
    fi
    
    # Final verification that process is still running
    if ! ps -p "$pid" > /dev/null 2>&1; then
        print_error "$instance_name (PID $pid) exited after startup" >&2
        return 1
    fi
    
    # Verify port is bound (get port from config)
    local port
    port=$(get_webserver_port "$config_file")
    print_info "Verifying port $port is bound..." >&2
    
    # Give a moment for port binding
    sleep 1
    
    if ! check_port_in_use_robust "$port"; then
        print_error "$instance_name failed to bind to port $port" >&2
        kill "$pid" 2>/dev/null || true
        return 1
    fi
    
    print_info "$instance_name successfully started and bound to port $port" >&2
    
    # Output PID to stdout for command substitution capture
    echo "$pid"
}

# Function to shutdown instance gracefully (robust version from test_55)
shutdown_instance_robust() {
    local pid="$1"
    local instance_name="$2"
    
    print_header "Shutting down $instance_name"
    
    # Validate PID before attempting to kill
    if [ -z "$pid" ] || [ "$pid" = "0" ]; then
        print_warning "$instance_name has invalid PID ($pid), cannot shutdown"
        return 1
    fi
    
    # Check if process is still running before attempting to kill
    if ! ps -p "$pid" > /dev/null 2>&1; then
        print_info "$instance_name (PID $pid) is already terminated"
        return 0
    fi
    
    print_info "Terminating $instance_name (PID $pid)..."
    
    kill -SIGINT "$pid"
    sleep 1
    
    # Verify instance has exited
    if ps -p "$pid" > /dev/null 2>&1; then
        print_warning "$instance_name still running, forcing termination..."
        kill -9 "$pid" 2>/dev/null || true
        sleep 1
    fi
    
    print_info "$instance_name (PID $pid) termination confirmed"
}

# Function to get the web server port from config (if not already defined)
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

# Function to ensure clean test environment (robust version)
prepare_test_environment_robust() {
    local port="$1"
    
    # Make sure no Hydrogen instances are running
    print_info "Ensuring no Hydrogen instances are running..."
    pkill -f hydrogen || true
    sleep 2
    
    # Check if port is already in use and wait for it to become available
    if check_port_in_use_robust "$port"; then
        print_warning "Port $port is already in use, waiting for it to become available..."
        if ! wait_for_port_available "$port" 30; then
            print_warning "Port $port is still in use, attempting to force release..."
            pkill -f hydrogen || true
            sleep 5
            if check_port_in_use_robust "$port"; then
                print_result 1 "Port $port is still in use, cannot run test"
                return 1
            fi
        fi
    fi
}
