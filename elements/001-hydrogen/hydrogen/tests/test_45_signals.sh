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

# Test configuration
RESTART_COUNT=5  # Number of SIGHUP restarts to test

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

print_info "Configured to test $RESTART_COUNT restarts"

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

# Function to verify config dump
verify_config_dump() {
    local log_file=$1
    local timeout=$2
    local start_time=$(date +%s)
    
    # Wait for dump to start
    while true; do
        if [ $(($(date +%s) - start_time)) -ge $timeout ]; then
            return 1
        fi
        
        if grep -q "APPCONFIG Dump Started" "$log_file"; then
            break
        fi
        
        sleep 0.2
    done
    
    # Wait for dump to complete
    while true; do
        if [ $(($(date +%s) - start_time)) -ge $timeout ]; then
            return 1
        fi
        
        if grep -q "APPCONFIG Dump Complete" "$log_file"; then
            # Count lines between start and end
            local start_line=$(grep -n "APPCONFIG Dump Started" "$log_file" | tail -1 | cut -d: -f1)
            local end_line=$(grep -n "APPCONFIG Dump Complete" "$log_file" | tail -1 | cut -d: -f1)
            if [ -n "$start_line" ] && [ -n "$end_line" ]; then
                local dump_lines=$((end_line - start_line + 1))
                print_info "Config dump completed with $dump_lines lines"
                return 0
            fi
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
            # Check for restart count in either format
            if grep -q "Application restarts: $expected_count" "$log_file" || \
               grep -q "Restart count: $expected_count" "$log_file"; then
                print_info "Verified restart completed successfully with count $expected_count (PID: $pid)"
                return 0
            fi
        fi
        
        # 2. Check for restart sequence completion and startup with correct count
        if (grep -q "Application restarts: $expected_count" "$log_file" || \
            grep -q "Restart count: $expected_count" "$log_file") && \
           grep -q "Initiating graceful restart sequence" "$log_file" && \
           grep -q "Application started" "$log_file"; then
            print_info "Verified restart via restart sequence and startup messages with count $expected_count (PID: $pid)"
            return 0
        fi
        
        # 3. Check for in-process restart flow with correct count
        if grep -q "SIGHUP received, initiating restart" "$log_file" && \
           grep -q "Initiating in-process restart" "$log_file" && \
           (grep -q "Application restarts: $expected_count" "$log_file" || \
            grep -q "Restart count: $expected_count" "$log_file") && \
           grep -q "In-process restart successful" "$log_file" && \
           grep -q "Restart completed successfully" "$log_file"; then
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
                    # Wait for startup message in log with extended timeout
                    for i in {1..20}; do
                        if grep -q "Application started" "$log_file" && \
                           (grep -q "Application restarts: $expected_count" "$log_file" || \
                            grep -q "Restart count: $expected_count" "$log_file"); then
                            # Update global PID for cleanup
                            HYDROGEN_PID=$new_pid
                            print_info "Found new process with PID: $new_pid and restart count $expected_count"
                            # Give extra time for process to stabilize
                            sleep 2
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

# Store original PID and time for reference
ORIGINAL_PID=$HYDROGEN_PID
ORIGINAL_TIME=$(grep -o "System startup began: [0-9:T.-]*Z" "$LOG_FILE_SIGHUP" | cut -d' ' -f4)

if ! wait_for_startup $STARTUP_TIMEOUT "$LOG_FILE_SIGHUP"; then
    print_result 1 "Failed to start Hydrogen" | tee -a "$RESULT_LOG"
    kill -9 $HYDROGEN_PID 2>/dev/null
    exit 1
fi

# Test multiple restarts up to RESTART_COUNT
SIGHUP_TEST=1  # Default to failure
CURRENT_COUNT=1

while [ $CURRENT_COUNT -le $RESTART_COUNT ]; do
    print_info "Sending SIGHUP #$CURRENT_COUNT of $RESTART_COUNT..." | tee -a "$RESULT_LOG"
    kill -SIGHUP $HYDROGEN_PID || true
    
    # Wait for restart signal to be logged
    sleep 1
    if ! grep -q "SIGHUP received" "$LOG_FILE_SIGHUP"; then
        print_error "SIGHUP signal #$CURRENT_COUNT of $RESTART_COUNT not received" | tee -a "$RESULT_LOG"
        kill -9 $HYDROGEN_PID 2>/dev/null
        break
    else
        print_info "SIGHUP signal #$CURRENT_COUNT of $RESTART_COUNT received, waiting for restart..." | tee -a "$RESULT_LOG"
        if wait_for_restart $HYDROGEN_PID $((SHUTDOWN_TIMEOUT * 2)) $CURRENT_COUNT "$LOG_FILE_SIGHUP"; then
            print_info "Restart #$CURRENT_COUNT of $RESTART_COUNT verified with count $CURRENT_COUNT (PID: $HYDROGEN_PID)" | tee -a "$RESULT_LOG"
            
            # Check if this was the last restart
            if [ $CURRENT_COUNT -eq $RESTART_COUNT ]; then
                print_result 0 "Multiple SIGHUP restarts successful (verified $RESTART_COUNT restarts)" | tee -a "$RESULT_LOG"
                SIGHUP_TEST=0
                break
            fi
            
            # Increment counter and continue to next restart
            CURRENT_COUNT=$((CURRENT_COUNT + 1))
            sleep 2
        else
            print_result 1 "SIGHUP restart #$CURRENT_COUNT of $RESTART_COUNT failed" | tee -a "$RESULT_LOG"
            break
        fi
    fi
done

# Clean up the process
print_info "Cleaning up with SIGTERM to PID $HYDROGEN_PID" | tee -a "$RESULT_LOG"
kill -SIGTERM $HYDROGEN_PID 2>/dev/null || true
wait_for_shutdown $HYDROGEN_PID $SHUTDOWN_TIMEOUT "$LOG_FILE_SIGHUP" || true

# If cleanup failed, try to ensure process is killed
if ps -p $HYDROGEN_PID > /dev/null 2>&1; then
    print_info "Forcibly terminating process" | tee -a "$RESULT_LOG"
    kill -9 $HYDROGEN_PID 2>/dev/null || true
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

sleep 1

# Test Case 5: SIGUSR2 handling (config dump)
print_header "Test Case 5: SIGUSR2 Signal Handling (Config Dump)"
LOG_FILE_SIGUSR2="$LOG_DIR/hydrogen_signal_test_SIGUSR2.log"
> "$LOG_FILE_SIGUSR2"  # Clear log file

print_info "Starting Hydrogen..." | tee -a "$RESULT_LOG"
$HYDROGEN_BIN "$CONFIG_FILE" > "$LOG_FILE_SIGUSR2" 2>&1 &
HYDROGEN_PID=$!

if ! wait_for_startup $STARTUP_TIMEOUT "$LOG_FILE_SIGUSR2"; then
    print_result 1 "Failed to start Hydrogen" | tee -a "$RESULT_LOG"
    kill -9 $HYDROGEN_PID 2>/dev/null
    exit 1
fi

print_info "Sending SIGUSR2..." | tee -a "$RESULT_LOG"
kill -SIGUSR2 $HYDROGEN_PID

if verify_config_dump "$LOG_FILE_SIGUSR2" $SHUTDOWN_TIMEOUT; then
    print_result 0 "SIGUSR2 handled successfully (config dump verified)" | tee -a "$RESULT_LOG"
    SIGUSR2_TEST=0
else
    print_result 1 "SIGUSR2 handling failed" | tee -a "$RESULT_LOG"
    SIGUSR2_TEST=1
fi

# Clean up the process
print_info "Cleaning up with SIGTERM to PID $HYDROGEN_PID" | tee -a "$RESULT_LOG"
kill -SIGTERM $HYDROGEN_PID 2>/dev/null
wait_for_shutdown $HYDROGEN_PID $SHUTDOWN_TIMEOUT "$LOG_FILE_SIGUSR2" || true

sleep 1

# Calculate overall test result
TOTAL_TESTS=5
PASSED_TESTS=0
[ $SIGINT_TEST -eq 0 ] && ((PASSED_TESTS++))
[ $SIGTERM_TEST -eq 0 ] && ((PASSED_TESTS++))
[ $SIGHUP_TEST -eq 0 ] && ((PASSED_TESTS++))
[ $MULTI_SIGNAL_TEST -eq 0 ] && ((PASSED_TESTS++))
[ $SIGUSR2_TEST -eq 0 ] && ((PASSED_TESTS++))

# Print test summary
print_test_summary $TOTAL_TESTS $PASSED_TESTS $((TOTAL_TESTS - PASSED_TESTS))

# Get test name from script name
TEST_NAME=$(basename "$0" .sh | sed 's/^test_//')

# Export subtest results for test_all.sh
export_subtest_results "$TEST_NAME" $TOTAL_TESTS $PASSED_TESTS

# End the test with final result
if [ $PASSED_TESTS -eq $TOTAL_TESTS ]; then
    end_test 0 "Signal Handling Test"
    exit 0
else
    end_test 1 "Signal Handling Test"
    exit 1
fi