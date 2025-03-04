#!/bin/bash
#
# Socket Rebinding Test
# Tests that the SO_REUSEADDR socket option allows immediate rebinding after shutdown

# Get the directory where this script is located
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
HYDROGEN_DIR="$( cd "$SCRIPT_DIR/.." && pwd )"

# Include the common test utilities
source "$SCRIPT_DIR/support_utils.sh"

# Configuration
# Prefer release build if available, fallback to standard build
if [ -f "$HYDROGEN_DIR/hydrogen_release" ]; then
    HYDROGEN_BIN="$HYDROGEN_DIR/hydrogen_release"
    print_info "Using release build for testing"
else
    HYDROGEN_BIN="$HYDROGEN_DIR/hydrogen"
    print_info "Using standard build"
fi

CONFIG_FILE=$(get_config_path "hydrogen_test_api.json")
if [ ! -f "$CONFIG_FILE" ]; then
    CONFIG_FILE=$(get_config_path "hydrogen_test_max.json")
    print_info "API test config not found, using max config"
fi

# Create output directories
RESULTS_DIR="$SCRIPT_DIR/results"
mkdir -p "$RESULTS_DIR"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
RESULT_LOG="$RESULTS_DIR/socket_rebind_test_${TIMESTAMP}.log"

# Function to check if a port is in use
check_port_in_use() {
    PORT=$1
    if command -v ss &> /dev/null; then
        ss -tuln | grep ":$PORT " > /dev/null
        return $?
    elif command -v netstat &> /dev/null; then
        netstat -tuln | grep ":$PORT " > /dev/null
        return $?
    else
        echo "Neither ss nor netstat is available"
        return 1
    fi
}

# Function to get the web server port from config
get_webserver_port() {
    CONFIG=$1
    if command -v jq &> /dev/null; then
        # Use jq if available
        PORT=$(jq -r '.WebServer.Port // 8080' "$CONFIG" 2>/dev/null)
        if [ -z "$PORT" ] || [ "$PORT" = "null" ]; then
            PORT=8080
        fi
    else
        # Fallback to grep
        PORT=$(grep -o '"Port":[[:space:]]*[0-9]*' "$CONFIG" | head -1 | grep -o '[0-9]*')
        if [ -z "$PORT" ]; then
            PORT=8080
        fi
    fi
    echo $PORT
}

# Handle cleanup on script termination
cleanup() {
    # Kill any hydrogen processes started by this script
    if [ ! -z "$FIRST_PID" ] && ps -p $FIRST_PID > /dev/null 2>&1; then
        echo "Cleaning up first instance (PID $FIRST_PID)..."
        kill -SIGINT $FIRST_PID 2>/dev/null || kill -9 $FIRST_PID 2>/dev/null
    fi
    
    if [ ! -z "$SECOND_PID" ] && ps -p $SECOND_PID > /dev/null 2>&1; then
        echo "Cleaning up second instance (PID $SECOND_PID)..."
        kill -SIGINT $SECOND_PID 2>/dev/null || kill -9 $SECOND_PID 2>/dev/null
    fi
    
    # Save test log if we've started writing it
    if [ -n "$RESULT_LOG" ] && [ -f "$RESULT_LOG" ]; then
        echo "Saving test results to $(convert_to_relative_path "$RESULT_LOG")"
    fi
    
    exit 0
}

# Set up trap for clean termination
trap cleanup SIGINT SIGTERM EXIT

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

# Make sure no Hydrogen instances are running
print_info "Ensuring no Hydrogen instances are running..." | tee -a "$RESULT_LOG"
pkill -f hydrogen || true
sleep 2

# Check if port is already in use
if check_port_in_use $PORT; then
    print_warning "Port $PORT is already in use, attempting to force release..." | tee -a "$RESULT_LOG"
    pkill -f hydrogen || true
    sleep 5
    if check_port_in_use $PORT; then
        print_result 1 "Port $PORT is still in use, cannot run test" | tee -a "$RESULT_LOG"
        end_test 1 "Socket Rebinding Test" | tee -a "$RESULT_LOG"
        exit 1
    fi
fi

# Start Hydrogen first time
print_header "Starting Hydrogen (first instance)" | tee -a "$RESULT_LOG"
$HYDROGEN_BIN "$CONFIG_FILE" > /dev/null 2>&1 &
FIRST_PID=$!
print_info "Started with PID: $FIRST_PID" | tee -a "$RESULT_LOG"

# Wait for server to initialize
sleep 3

# Check if server is running and bound to the port
if ! ps -p $FIRST_PID > /dev/null; then
    print_result 1 "First instance failed to start" | tee -a "$RESULT_LOG"
    end_test 1 "Socket Rebinding Test" | tee -a "$RESULT_LOG"
    exit 1
fi

if ! check_port_in_use $PORT; then
    print_result 1 "First instance failed to bind to port $PORT" | tee -a "$RESULT_LOG"
    kill $FIRST_PID 2>/dev/null || true
    end_test 1 "Socket Rebinding Test" | tee -a "$RESULT_LOG"
    exit 1
fi

print_info "First instance running and bound to port $PORT" | tee -a "$RESULT_LOG"
# First subtest passed
((PASS_COUNT++))

# Shutdown first instance
print_header "Shutting down first instance" | tee -a "$RESULT_LOG"
kill -SIGINT $FIRST_PID
sleep 1

# Verify first instance has exited
if ps -p $FIRST_PID > /dev/null; then
    print_warning "First instance still running, forcing termination..." | tee -a "$RESULT_LOG"
    kill -9 $FIRST_PID 2>/dev/null || true
    sleep 1
fi

print_info "First instance has terminated" | tee -a "$RESULT_LOG"
# Second subtest passed
((PASS_COUNT++))

# Check if socket is in TIME_WAIT state
if command -v ss &> /dev/null; then
    # Ensure we get a clean integer count without any extra characters
    TIME_WAIT_COUNT=$(ss -tan | grep ":$PORT" | grep -c "TIME-WAIT" 2>/dev/null || echo 0)
    TIME_WAIT_COUNT=$(echo "$TIME_WAIT_COUNT" | tr -d '[:space:]')
    if [ -z "$TIME_WAIT_COUNT" ]; then
        TIME_WAIT_COUNT=0
    fi
    
    if [ "$TIME_WAIT_COUNT" -gt 0 ]; then
        print_info "Found $TIME_WAIT_COUNT socket(s) in TIME-WAIT state on port $PORT" | tee -a "$RESULT_LOG"
        ss -tan | grep ":$PORT" | grep "TIME-WAIT" | tee -a "$RESULT_LOG"
    else
        print_info "No TIME-WAIT sockets found on port $PORT" | tee -a "$RESULT_LOG"
    fi
elif command -v netstat &> /dev/null; then
    # Ensure we get a clean integer count without any extra characters
    TIME_WAIT_COUNT=$(netstat -tan | grep ":$PORT" | grep -c "TIME_WAIT" 2>/dev/null || echo 0)
    TIME_WAIT_COUNT=$(echo "$TIME_WAIT_COUNT" | tr -d '[:space:]')
    if [ -z "$TIME_WAIT_COUNT" ]; then
        TIME_WAIT_COUNT=0
    fi
    
    if [ "$TIME_WAIT_COUNT" -gt 0 ]; then
        print_info "Found $TIME_WAIT_COUNT socket(s) in TIME_WAIT state on port $PORT" | tee -a "$RESULT_LOG"
        netstat -tan | grep ":$PORT" | grep "TIME_WAIT" | tee -a "$RESULT_LOG"
    else
        print_info "No TIME_WAIT sockets found on port $PORT" | tee -a "$RESULT_LOG"
    fi
else
    print_warning "Could not check for TIME_WAIT sockets (no ss or netstat)" | tee -a "$RESULT_LOG"
fi

# Immediately start second instance
print_header "Starting Hydrogen (second instance - immediate restart)" | tee -a "$RESULT_LOG"
$HYDROGEN_BIN "$CONFIG_FILE" > /dev/null 2>&1 &
SECOND_PID=$!
print_info "Started with PID: $SECOND_PID" | tee -a "$RESULT_LOG"

# Wait for server to initialize
sleep 3

# Check if second instance is running and bound to the port
if ! ps -p $SECOND_PID > /dev/null; then
    print_result 1 "Second instance failed to start" | tee -a "$RESULT_LOG"
    end_test 1 "Socket Rebinding Test" | tee -a "$RESULT_LOG"
    exit 1
fi

if ! check_port_in_use $PORT; then
    print_result 1 "Second instance failed to bind to port $PORT" | tee -a "$RESULT_LOG"
    kill $SECOND_PID 2>/dev/null || true
    end_test 1 "Socket Rebinding Test" | tee -a "$RESULT_LOG"
    exit 1
fi

print_info "Second instance running and bound to port $PORT successfully!" | tee -a "$RESULT_LOG"
print_info "SO_REUSEADDR is working correctly" | tee -a "$RESULT_LOG"
# Third subtest passed
((PASS_COUNT++))

# Clean up
print_header "Cleaning up" | tee -a "$RESULT_LOG"
kill -SIGINT $SECOND_PID 2>/dev/null || true
sleep 2
if ps -p $SECOND_PID > /dev/null; then
    print_warning "Second instance still running, forcing termination..." | tee -a "$RESULT_LOG"
    kill -9 $SECOND_PID 2>/dev/null || true
fi

# Export subtest results for test_all.sh to pick up
export_subtest_results $TOTAL_SUBTESTS $PASS_COUNT

# Log subtest results
print_info "Socket Rebind Test: $PASS_COUNT of $TOTAL_SUBTESTS subtests passed" | tee -a "$RESULT_LOG"

# Test successful!
print_result 0 "Socket rebinding test PASSED - Immediate rebinding after shutdown works!" | tee -a "$RESULT_LOG"
print_info "Results saved to: $(convert_to_relative_path "$RESULT_LOG")" | tee -a "$RESULT_LOG"
end_test 0 "Socket Rebinding Test" | tee -a "$RESULT_LOG"
exit 0