#!/bin/bash
#
# Signal Handling Test
# Tests Hydrogen's signal handling capabilities:
# - SIGINT: Clean shutdown
# - SIGTERM: Clean shutdown
# - SIGHUP: Restart with config reload
# - Multiple signal handling
# - Clean shutdown verification

# Get the directory where this script is located
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
HYDROGEN_DIR="$( cd "$SCRIPT_DIR/.." && pwd )"

# Include the common test utilities
UTILS_FILE="$SCRIPT_DIR/support_utils.sh"
if [ ! -f "$UTILS_FILE" ]; then
    echo "Error: support_utils.sh not found at: $UTILS_FILE"
    exit 1
fi

# Source the utilities with absolute path
. "$UTILS_FILE"

# Verify support functions are available
for func in print_info print_error print_header print_result start_test end_test; do
    if ! command -v $func >/dev/null 2>&1; then
        echo "Error: Required function '$func' not loaded from $UTILS_FILE"
        exit 1
    fi
done

# Configuration and path setup
# Determine which hydrogen build to use (prefer release build if available)
cd $(dirname $0)/..
if [ -f "./hydrogen_release" ]; then
    HYDROGEN_BIN="./hydrogen_release"
    print_info "Using release build for testing"
else
    HYDROGEN_BIN="./hydrogen"
    print_info "Using standard build"
fi

# Verify executable exists
if [ ! -f "$HYDROGEN_BIN" ]; then
    print_error "Hydrogen executable not found at: $HYDROGEN_BIN"
    exit 1
fi

# Use absolute path for config file to ensure it works after restart
CONFIG_FILE="$(cd "$SCRIPT_DIR/configs" && pwd)/hydrogen_test_min.json"
STARTUP_TIMEOUT=5     # Shorter timeout for startup
SHUTDOWN_TIMEOUT=10   # Shorter timeout for shutdown
LOG_DIR="$SCRIPT_DIR/logs"
LOG_FILE_SIGINT="$LOG_DIR/hydrogen_signal_test_SIGINT.log"
LOG_FILE_SIGTERM="$LOG_DIR/hydrogen_signal_test_SIGTERM.log"
LOG_FILE_SIGHUP="$LOG_DIR/hydrogen_signal_test_SIGHUP.log"
LOG_FILE_MULTI="$LOG_DIR/hydrogen_signal_test_MULTI.log"
RESULTS_DIR="$SCRIPT_DIR/results"
DIAG_DIR="$SCRIPT_DIR/diagnostics"

# Create output directories
mkdir -p "$RESULTS_DIR" "$DIAG_DIR" "$(dirname "$LOG_FILE")"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
RESULT_LOG="$RESULTS_DIR/signal_test_${TIMESTAMP}.log"
DIAG_TEST_DIR="$DIAG_DIR/signal_test_${TIMESTAMP}"
mkdir -p "$DIAG_TEST_DIR"

# Function to wait for startup
wait_for_startup() {
    local timeout=$1
    local log_file=$2
    local start_time=$(date +%s)
    
    while true; do
        if [ $(($(date +%s) - start_time)) -ge $timeout ]; then
            return 1
        fi
        
        if grep -q "Application started" "$log_file"; then
            return 0
        fi
        
        sleep 0.2
    done
}

# Function to wait for shutdown
wait_for_shutdown() {
    local pid=$1
    local timeout=$2
    local log_file=$3
    local start_time=$(date +%s)
    
    while true; do
        if [ $(($(date +%s) - start_time)) -ge $timeout ]; then
            return 1
        fi
        
        if ! ps -p $pid > /dev/null; then
            if grep -q "Shutdown complete" "$log_file"; then
                return 0
            else
                return 2
            fi
        fi
        
        sleep 0.2
    done
}

# Function to wait for restart
wait_for_restart() {
    local pid=$1
    local timeout=$2
    local expected_count=${3:-1}  # Default to 1 if not specified
    local log_file=$4
    local start_time=$(date +%s)
    
    # For in-process restart we want to keep examining the log file
    print_info "Waiting for restart (expecting restart count: $expected_count)..."
    
    while true; do
        if [ $(($(date +%s) - start_time)) -ge $timeout ]; then
            return 1
        fi
        
        # Check for evidence of restart completion in the logs
        # Look for key markers indicating successful restart:
        # 1. "Restart completed successfully" message (most reliable)
        if grep -q "Restart completed successfully" "$log_file"; then
            # Check for expected restart count
            if grep -q "Restart count: $expected_count" "$log_file"; then
                print_info "Verified restart completed successfully with count $expected_count (PID: $pid)"
                return 0
            fi
        fi
        
        # 2. Check for restart sequence completion and startup with correct count
        if grep -q "Restart count: $expected_count" "$log_file" && \
           grep -q "Initiating graceful restart sequence" "$log_file" && \
           grep -q "Application started" "$log_file"; then
            print_info "Verified restart via restart sequence and startup messages with count $expected_count (PID: $pid)"
            return 0
        fi
        
        # 3. Check for in-process restart flow with correct count
        if grep -q "SIGHUP received, initiating restart" "$log_file" && \
           grep -q "Cleanup phase complete" "$log_file" && \
           grep -q "In-process restart successful" "$log_file" && \
           grep -q "Restart count: $expected_count" "$log_file"; then
            print_info "Verified restart via restart flow messages with count $expected_count (PID: $pid)"
            return 0
        fi
        
        # As a backup, still check for process-based restart (in case implementation changes)
        if ! ps -p $pid > /dev/null; then
            # Give time for new process to start
            sleep 2
            
            # Look for new hydrogen process
            for new_pid in $(pgrep -f "hydrogen.*$CONFIG_FILE"); do
                if [ "$new_pid" != "$pid" ] && ps -p $new_pid > /dev/null; then
                    # Wait for startup message in log
                    for i in {1..10}; do
                        if grep -q "Application started" "$log_file" && \
                           grep -q "Restart count: $expected_count" "$log_file"; then
                            # Update global PID for cleanup
                            HYDROGEN_PID=$new_pid
                            print_info "Found new process with PID: $new_pid and restart count $expected_count"
                            return 0
                        fi
                        sleep 0.5
                    done
                fi
            done
        fi
        
        sleep 0.2
    done
}

# Start the test
start_test "Hydrogen Signal Handling Test"

# Test Case 1: SIGINT handling
print_header "Test Case 1: SIGINT Signal Handling"
> "$LOG_FILE_SIGINT"  # Clear log file

print_info "Starting Hydrogen..." | tee -a "$RESULT_LOG"
$HYDROGEN_BIN "$CONFIG_FILE" > "$LOG_FILE_SIGINT" 2>&1 &
HYDROGEN_PID=$!

if ! wait_for_startup $STARTUP_TIMEOUT "$LOG_FILE_SIGINT"; then
    print_result 1 "Failed to start Hydrogen" | tee -a "$RESULT_LOG"
    kill -9 $HYDROGEN_PID 2>/dev/null
    exit 1
fi

print_info "Sending SIGINT..." | tee -a "$RESULT_LOG"
kill -SIGINT $HYDROGEN_PID

if wait_for_shutdown $HYDROGEN_PID $SHUTDOWN_TIMEOUT "$LOG_FILE_SIGINT"; then
    print_result 0 "SIGINT handled successfully" | tee -a "$RESULT_LOG"
    SIGINT_TEST=0
else
    print_result 1 "SIGINT handling failed" | tee -a "$RESULT_LOG"
    kill -9 $HYDROGEN_PID 2>/dev/null
    SIGINT_TEST=1
fi

sleep 1

# Test Case 2: SIGTERM handling
print_header "Test Case 2: SIGTERM Signal Handling"
> "$LOG_FILE_SIGTERM"  # Clear log file

print_info "Starting Hydrogen..." | tee -a "$RESULT_LOG"
$HYDROGEN_BIN "$CONFIG_FILE" > "$LOG_FILE_SIGTERM" 2>&1 &
HYDROGEN_PID=$!

if ! wait_for_startup $STARTUP_TIMEOUT "$LOG_FILE_SIGTERM"; then
    print_result 1 "Failed to start Hydrogen" | tee -a "$RESULT_LOG"
    kill -9 $HYDROGEN_PID 2>/dev/null
    exit 1
fi

print_info "Sending SIGTERM..." | tee -a "$RESULT_LOG"
kill -SIGTERM $HYDROGEN_PID

if wait_for_shutdown $HYDROGEN_PID $SHUTDOWN_TIMEOUT "$LOG_FILE_SIGTERM"; then
    print_result 0 "SIGTERM handled successfully" | tee -a "$RESULT_LOG"
    SIGTERM_TEST=0
else
    print_result 1 "SIGTERM handling failed" | tee -a "$RESULT_LOG"
    kill -9 $HYDROGEN_PID 2>/dev/null
    SIGTERM_TEST=1
fi

sleep 1

# Test Case 3: SIGHUP handling (multiple restarts)
print_header "Test Case 3: SIGHUP Signal Handling (Multiple Restarts)"
> "$LOG_FILE_SIGHUP"  # Clear log file

print_info "Starting Hydrogen..." | tee -a "$RESULT_LOG"
$HYDROGEN_BIN "$CONFIG_FILE" > "$LOG_FILE_SIGHUP" 2>&1 &
HYDROGEN_PID=$!

# Store original PID for reference
ORIGINAL_PID=$HYDROGEN_PID

if ! wait_for_startup $STARTUP_TIMEOUT "$LOG_FILE_SIGHUP"; then
    print_result 1 "Failed to start Hydrogen" | tee -a "$RESULT_LOG"
    kill -9 $HYDROGEN_PID 2>/dev/null
    exit 1
fi

# Test first restart
print_info "Sending first SIGHUP..." | tee -a "$RESULT_LOG"
kill -SIGHUP $HYDROGEN_PID || true

# Wait for first restart signal to be logged
sleep 1
if ! grep -q "SIGHUP received" "$LOG_FILE_SIGHUP"; then
    print_error "First SIGHUP signal not received" | tee -a "$RESULT_LOG"
    kill -9 $HYDROGEN_PID 2>/dev/null
    SIGHUP_TEST=1
else
    print_info "First SIGHUP signal received, waiting for restart..." | tee -a "$RESULT_LOG"
    if wait_for_restart $HYDROGEN_PID $((SHUTDOWN_TIMEOUT * 2)) 1 "$LOG_FILE_SIGHUP"; then
        print_info "First restart verified with count 1 (PID: $HYDROGEN_PID)" | tee -a "$RESULT_LOG"
        
        # Give system time to stabilize after restart
        sleep 2
        
        # Test second restart
        print_info "Sending second SIGHUP..." | tee -a "$RESULT_LOG"
        kill -SIGHUP $HYDROGEN_PID || true
        
        # Wait for second restart to complete
        if wait_for_restart $HYDROGEN_PID $((SHUTDOWN_TIMEOUT * 2)) 2 "$LOG_FILE_SIGHUP"; then
            print_info "Second restart verified with count 2 (PID: $HYDROGEN_PID)" | tee -a "$RESULT_LOG"
            
            # Give system time to stabilize after restart
            sleep 2
            
            # Test third restart
            print_info "Sending third SIGHUP..." | tee -a "$RESULT_LOG"
            kill -SIGHUP $HYDROGEN_PID || true
            
            # Wait for third restart to complete
            if wait_for_restart $HYDROGEN_PID $((SHUTDOWN_TIMEOUT * 2)) 3 "$LOG_FILE_SIGHUP"; then
                print_info "Third restart verified with count 3 (PID: $HYDROGEN_PID)" | tee -a "$RESULT_LOG"
                print_result 0 "Multiple SIGHUP restarts successful (count verified up to 3)" | tee -a "$RESULT_LOG"
                SIGHUP_TEST=0
            else
                print_result 1 "Third SIGHUP restart failed" | tee -a "$RESULT_LOG"
                SIGHUP_TEST=1
            fi
        else
            print_result 1 "Second SIGHUP restart failed" | tee -a "$RESULT_LOG"
            SIGHUP_TEST=1
        fi
    else
        print_result 1 "First SIGHUP restart failed" | tee -a "$RESULT_LOG"
        SIGHUP_TEST=1
    fi
    
    # Clean up the restarted process - use || true to ignore errors if process is gone
    print_info "Cleaning up with SIGTERM to PID $HYDROGEN_PID" | tee -a "$RESULT_LOG"
    kill -SIGTERM $HYDROGEN_PID 2>/dev/null || true
    wait_for_shutdown $HYDROGEN_PID $SHUTDOWN_TIMEOUT "$LOG_FILE_SIGHUP" || true
    
    # If cleanup failed, try to ensure process is killed
    if ps -p $HYDROGEN_PID > /dev/null 2>&1; then
        print_info "Forcibly terminating process" | tee -a "$RESULT_LOG"
        kill -9 $HYDROGEN_PID 2>/dev/null || true
    fi
fi

sleep 1

# Test Case 4: Multiple signal handling
print_header "Test Case 4: Multiple Signal Handling"
> "$LOG_FILE_MULTI"  # Clear log file

print_info "Starting Hydrogen..." | tee -a "$RESULT_LOG"
$HYDROGEN_BIN "$CONFIG_FILE" > "$LOG_FILE_MULTI" 2>&1 &
HYDROGEN_PID=$!

if ! wait_for_startup $STARTUP_TIMEOUT "$LOG_FILE_MULTI"; then
    print_result 1 "Failed to start Hydrogen" | tee -a "$RESULT_LOG"
    kill -9 $HYDROGEN_PID 2>/dev/null
    exit 1
fi

print_info "Sending multiple signals (SIGTERM, SIGINT)..." | tee -a "$RESULT_LOG"
kill -SIGTERM $HYDROGEN_PID
sleep 0.1
kill -SIGINT $HYDROGEN_PID

if wait_for_shutdown $HYDROGEN_PID $SHUTDOWN_TIMEOUT "$LOG_FILE_MULTI"; then
    # Check logs for single shutdown sequence
    SHUTDOWN_COUNT=$(grep -c "Initiating graceful shutdown sequence" "$LOG_FILE_MULTI")
    if [ "$SHUTDOWN_COUNT" -eq 1 ]; then
        print_result 0 "Multiple signals handled correctly (single shutdown)" | tee -a "$RESULT_LOG"
        MULTI_SIGNAL_TEST=0
    else
        print_result 1 "Multiple shutdown sequences detected" | tee -a "$RESULT_LOG"
        MULTI_SIGNAL_TEST=1
    fi
else
    print_result 1 "Multiple signal handling failed" | tee -a "$RESULT_LOG"
    kill -9 $HYDROGEN_PID 2>/dev/null
    MULTI_SIGNAL_TEST=1
fi

# Calculate overall test result
TOTAL_TESTS=4
PASSED_TESTS=0
[ $SIGINT_TEST -eq 0 ] && ((PASSED_TESTS++))
[ $SIGTERM_TEST -eq 0 ] && ((PASSED_TESTS++))
[ $SIGHUP_TEST -eq 0 ] && ((PASSED_TESTS++))
[ $MULTI_SIGNAL_TEST -eq 0 ] && ((PASSED_TESTS++))

# Print test summary
print_test_summary $TOTAL_TESTS $PASSED_TESTS $((TOTAL_TESTS - PASSED_TESTS))

# Export subtest results for test_all.sh
export_subtest_results $TOTAL_TESTS $PASSED_TESTS

# End the test with final result
if [ $PASSED_TESTS -eq $TOTAL_TESTS ]; then
    end_test 0 "Signal Handling Test"
    exit 0
else
    end_test 1 "Signal Handling Test"
    exit 1
fi