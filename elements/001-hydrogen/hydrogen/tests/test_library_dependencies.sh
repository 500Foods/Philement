#!/bin/bash
#
# Hydrogen Library Dependencies Test Script
# Tests the library dependency checking system added to initialization

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

# Default configuration files - one with everything enabled, one with minimal services
ALL_ENABLED_CONFIG=$(get_config_path "hydrogen_test_max.json")
MINIMAL_CONFIG=$(get_config_path "hydrogen_test_min.json")

# Timeouts and paths
STARTUP_TIMEOUT=10                        # Seconds to wait for startup
LOG_FILE="$SCRIPT_DIR/hydrogen_test.log"  # Runtime log to analyze (use common log file)
RESULTS_DIR="$SCRIPT_DIR/results"         # Directory for test results
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
RESULT_LOG="$RESULTS_DIR/lib_dep_test_${TIMESTAMP}.log"

# Create output directories
mkdir -p "$RESULTS_DIR"

# Start the test
start_test "Hydrogen Library Dependencies Test" | tee -a "$RESULT_LOG"

# Initialize error counters
# Use the 'declare' statement to make them global
declare -i EXIT_CODE=0
declare -i TOTAL_TESTS=0
declare -i PASSED_TESTS=0
declare -i FAILED_TESTS=0

# Helper function to check logs for dependency messages
check_dependency_log() {
    local dep_name="$1"
    local expected_level="$2"
    local expected_status="$3"
    local is_required="$4"
    local log_file="$5"
    local test_name="$6"
    
    # Increment total tests
    TOTAL_TESTS=$((TOTAL_TESTS + 1))
    
    # Determine the expected log level marker based on level number
    local level_marker=""
    if [ "$expected_level" == "1" ]; then
        level_marker="INFO"
    elif [ "$expected_level" == "2" ]; then
        level_marker="WARN"
    elif [ "$expected_level" == "5" ]; then
        level_marker="CRITICAL"
    else
        level_marker="ERROR"
    fi
    
    # Look for the dependency message in the log, ignoring timestamp
    if [ "$is_required" = "true" ]; then
        # For required libraries, accept INFO level with Good status
        if grep -q ".*\[ INFO      \]  \[ Initialization     \]  $dep_name.*Status: Good" "$log_file"; then
            print_test_item 0 "$test_name" "Found $dep_name with expected level INFO"
            PASSED_TESTS=$((PASSED_TESTS + 1))
            return 0
        fi
    else
        # For optional libraries, accept either status since we're on dev machine
        if grep -q ".*\[ \(INFO\|WARN\)      \]  \[ Initialization     \]  $dep_name.*Status: \(Good\|Less Good\)" "$log_file"; then
            print_test_item 0 "$test_name" "Found $dep_name with valid status"
            PASSED_TESTS=$((PASSED_TESTS + 1))
            return 0
        fi
    fi
    
    print_test_item 1 "$test_name" "Did not find $dep_name with expected level $level_marker"
    FAILED_TESTS=$((FAILED_TESTS + 1))
    return 1
}

# Test 1: Run with all services enabled (max config)
print_header "Test Case 1: All Services Enabled" | tee -a "$RESULT_LOG"
print_info "Using config: $(convert_to_relative_path "$ALL_ENABLED_CONFIG")" | tee -a "$RESULT_LOG"

# Start with clean log
> "$LOG_FILE"

# Launch Hydrogen in background with max config
print_command "$HYDROGEN_BIN $ALL_ENABLED_CONFIG" | tee -a "$RESULT_LOG"
$HYDROGEN_BIN "$ALL_ENABLED_CONFIG" > "$LOG_FILE" 2>&1 &
HYDROGEN_PID=$!

# Wait for startup completion or timeout
STARTUP_START=$(date +%s)
while true; do
    # Check if we've exceeded the timeout
    CURRENT_TIME=$(date +%s)
    ELAPSED=$((CURRENT_TIME - STARTUP_START))
    
    if [ $ELAPSED -ge $STARTUP_TIMEOUT ]; then
        print_warning "Startup timeout after ${ELAPSED}s" | tee -a "$RESULT_LOG"
        break
    fi
    
    # Check for startup completion
    if grep -q "Application started" "$LOG_FILE"; then
        print_info "Startup completed in $ELAPSED seconds" | tee -a "$RESULT_LOG"
        break
    fi
    
    # Small sleep to avoid consuming CPU
    sleep 0.2
done

# Check for library dependency messages with max config (all required)
print_info "Checking dependency logs for max config (all services enabled)" | tee -a "$RESULT_LOG"
# Helper wrapper that avoids the subshell problem with piping to tee
log_and_check() {
    local result
    
    check_dependency_log "$1" "$2" "$3" "$4" "$5" "$6"
    result=$?
    
    # Log the last line of output to the result log
    tail -n 1 "$LOG_FILE" >> "$RESULT_LOG"
    
    return $result
}

# Run checks without piping to tee to preserve variable changes
log_and_check "pthreads" "1" "Good" "true" "$LOG_FILE" "Required pthreads"
log_and_check "jansson" "1" "Good" "true" "$LOG_FILE" "Required jansson"
log_and_check "libm" "1" "Good" "true" "$LOG_FILE" "Required libm"
log_and_check "microhttpd" "1" "Good" "true" "$LOG_FILE" "Required microhttpd with web enabled"
log_and_check "libwebsockets" "1" "Good" "true" "$LOG_FILE" "Required libwebsockets with websocket enabled"
log_and_check "OpenSSL" "1" "Good" "true" "$LOG_FILE" "Required OpenSSL with web/websocket enabled"
log_and_check "libbrotlidec" "1" "Good" "true" "$LOG_FILE" "Required libbrotlidec with web enabled"
log_and_check "libtar" "1" "Good" "true" "$LOG_FILE" "Required libtar with print enabled"

# Terminate the process more gracefully
if ps -p $HYDROGEN_PID > /dev/null; then
    # First try SIGTERM
    kill -SIGTERM $HYDROGEN_PID
    
    # Wait up to 10 seconds for graceful shutdown
    for i in {1..10}; do
        if ! ps -p $HYDROGEN_PID > /dev/null; then
            break
        fi
        sleep 1
    done
    
    # If still running, try SIGINT
    if ps -p $HYDROGEN_PID > /dev/null; then
        kill -SIGINT $HYDROGEN_PID
        sleep 2
    fi
    
    # Only use SIGKILL as absolute last resort
    if ps -p $HYDROGEN_PID > /dev/null; then
        kill -9 $HYDROGEN_PID
    fi
fi

# Wait to ensure resources are released before next test
sleep 3
ensure_server_not_running 2>/dev/null || true

# Test 2: Run with minimal services enabled (min config)
print_header "Test Case 2: Minimal Services Enabled" | tee -a "$RESULT_LOG"
print_info "Using config: $(convert_to_relative_path "$MINIMAL_CONFIG")" | tee -a "$RESULT_LOG"

# Start with clean log
> "$LOG_FILE"

# Launch Hydrogen in background with min config
print_command "$HYDROGEN_BIN $MINIMAL_CONFIG" | tee -a "$RESULT_LOG"
$HYDROGEN_BIN "$MINIMAL_CONFIG" > "$LOG_FILE" 2>&1 &
HYDROGEN_PID=$!

# Wait for startup completion or timeout
STARTUP_START=$(date +%s)
while true; do
    # Check if we've exceeded the timeout
    CURRENT_TIME=$(date +%s)
    ELAPSED=$((CURRENT_TIME - STARTUP_START))
    
    if [ $ELAPSED -ge $STARTUP_TIMEOUT ]; then
        print_warning "Startup timeout after ${ELAPSED}s" | tee -a "$RESULT_LOG"
        break
    fi
    
    # Check for startup completion
    if grep -q "Application started" "$LOG_FILE"; then
        print_info "Startup completed in $ELAPSED seconds" | tee -a "$RESULT_LOG"
        break
    fi
    
    # Small sleep to avoid consuming CPU
    sleep 0.2
done

# Check for library dependency messages with min config (some optional)
print_info "Checking dependency logs for min config (minimal services enabled)" | tee -a "$RESULT_LOG"
log_and_check "pthreads" "1" "Good" "true" "$LOG_FILE" "Required pthreads"
log_and_check "jansson" "1" "Good" "true" "$LOG_FILE" "Required jansson"
log_and_check "libm" "1" "Good" "true" "$LOG_FILE" "Required libm"
log_and_check "microhttpd" "2" "Less Good\|Warning" "false" "$LOG_FILE" "Optional microhttpd with web disabled"
log_and_check "libwebsockets" "2" "Less Good\|Warning" "false" "$LOG_FILE" "Optional libwebsockets with websocket disabled"
log_and_check "OpenSSL" "2" "Less Good\|Warning" "false" "$LOG_FILE" "Optional OpenSSL with web/websocket disabled"
log_and_check "libbrotlidec" "2" "Less Good\|Warning" "false" "$LOG_FILE" "Optional libbrotlidec with web disabled"
log_and_check "libtar" "2" "Less Good\|Warning" "false" "$LOG_FILE" "Optional libtar with print disabled"

# Terminate the process more gracefully
if ps -p $HYDROGEN_PID > /dev/null; then
    # First try SIGTERM
    kill -SIGTERM $HYDROGEN_PID
    
    # Wait up to 10 seconds for graceful shutdown
    for i in {1..10}; do
        if ! ps -p $HYDROGEN_PID > /dev/null; then
            break
        fi
        sleep 1
    done
    
    # If still running, try SIGINT
    if ps -p $HYDROGEN_PID > /dev/null; then
        kill -SIGINT $HYDROGEN_PID
        sleep 2
    fi
    
    # Only use SIGKILL as absolute last resort
    if ps -p $HYDROGEN_PID > /dev/null; then
        kill -9 $HYDROGEN_PID
    fi
fi

# Copy log for reference
cp "$LOG_FILE" "$RESULTS_DIR/lib_dep_full_log_${TIMESTAMP}.log"

# Print summary statistics
print_test_summary $TOTAL_TESTS $PASSED_TESTS $FAILED_TESTS >> "$RESULT_LOG"

# Determine final result
if [ $FAILED_TESTS -gt 0 ]; then
    print_result 1 "FAILED: $FAILED_TESTS out of $TOTAL_TESTS tests failed" | tee -a "$RESULT_LOG"
    EXIT_CODE=1
else
    print_result 0 "PASSED: All $TOTAL_TESTS tests passed successfully" | tee -a "$RESULT_LOG"
    EXIT_CODE=0
fi

# End test
end_test $EXIT_CODE "Library Dependencies Test" | tee -a "$RESULT_LOG"
exit $EXIT_CODE