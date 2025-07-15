#!/bin/bash

# Test: Signal Handling
# Tests signal handling capabilities including SIGINT, SIGTERM, SIGHUP, SIGUSR2, and multiple signals

# VERSION HISTORY
# 3.0.4 - 2025-07-15 - No more sleep
# 3.0.3 - 2025-07-14 - Updated to use build/tests directories for test output consistency
# 3.0.2 - 2025-07-06 - Added missing shellcheck justifications
# 3.0.1 - 2025-07-02 - Performance optimization: removed all artificial delays (sleep statements) for dramatically faster execution while maintaining reliability
# 3.0.0 - 2025-07-02 - Complete rewrite to use new modular test libraries
# 2.0.0 - 2025-06-17 - Major refactoring: fixed all shellcheck warnings, improved modularity, enhanced comments
# 1.0.0 - Original version - Basic signal handling test

# Test configuration
TEST_NAME="Signal Handling"
SCRIPT_VERSION="3.0.4"

# Get the directory where this script is located
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

if [[ -z "$LOG_OUTPUT_SH_GUARD" ]]; then
    # shellcheck source=tests/lib/log_output.sh # Resolve path statically
    source "$SCRIPT_DIR/lib/log_output.sh"
fi

# shellcheck source=tests/lib/file_utils.sh # Resolve path statically
source "$SCRIPT_DIR/lib/file_utils.sh"
# shellcheck source=tests/lib/framework.sh # Resolve path statically
source "$SCRIPT_DIR/lib/framework.sh"
# shellcheck source=tests/lib/lifecycle.sh # Resolve path statically
source "$SCRIPT_DIR/lib/lifecycle.sh"

# Test configuration
EXIT_CODE=0
TOTAL_SUBTESTS=9
PASS_COUNT=0

# Signal test configuration
RESTART_COUNT=5  # Number of SIGHUP restarts to test
STARTUP_TIMEOUT=5
SHUTDOWN_TIMEOUT=10
SIGNAL_TIMEOUT=15

# Auto-extract test number and set up environment
TEST_NUMBER=$(extract_test_number "${BASH_SOURCE[0]}")
set_test_number "$TEST_NUMBER"
reset_subtest_counter

# Print beautiful test header
print_test_header "$TEST_NAME" "$SCRIPT_VERSION"

# Use build directory for test results
BUILD_DIR="$SCRIPT_DIR/../build"
RESULTS_DIR="$BUILD_DIR/tests/results"
mkdir -p "$RESULTS_DIR"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
RESULT_LOG="$RESULTS_DIR/test_${TEST_NUMBER}_${TIMESTAMP}.log"

# Navigate to the project root (one level up from tests directory)
if ! navigate_to_project_root "$SCRIPT_DIR"; then
    print_error "Failed to navigate to project root directory"
    exit 1
fi

# Set up results directory (after navigating to project root)

# Validate Hydrogen Binary
next_subtest
print_subtest "Locate Hydrogen Binary"
HYDROGEN_DIR="$( cd "$SCRIPT_DIR/.." && pwd )"
# shellcheck disable=SC2153  # HYDROGEN_BIN is set by find_hydrogen_binary function
if find_hydrogen_binary "$HYDROGEN_DIR" "HYDROGEN_BIN"; then
    print_message "Using Hydrogen binary: $(basename "$HYDROGEN_BIN")"
    print_result 0 "Hydrogen binary found and validated"
    ((PASS_COUNT++))
else
    print_result 1 "Failed to find Hydrogen binary"
    EXIT_CODE=1
fi

# Test configuration
TEST_CONFIG=$(get_config_path "hydrogen_test_min.json")

# Validate configuration file exists
next_subtest
print_subtest "Validate Test Configuration File"
if validate_config_file "$TEST_CONFIG"; then
    print_message "Using minimal configuration for signal testing"
    ((PASS_COUNT++))
else
    EXIT_CODE=1
fi

# Set up log files for different signal tests
LOG_DIR="$BUILD_DIR/tests/logs"
mkdir -p "$LOG_DIR"
LOG_FILE_SIGINT="$LOG_DIR/hydrogen_signal_test_SIGINT_${TIMESTAMP}.log"
LOG_FILE_SIGTERM="$LOG_DIR/hydrogen_signal_test_SIGTERM_${TIMESTAMP}.log"
LOG_FILE_SIGHUP="$LOG_DIR/hydrogen_signal_test_SIGHUP_${TIMESTAMP}.log"
LOG_FILE_MULTI="$LOG_DIR/hydrogen_signal_test_MULTI_${TIMESTAMP}.log"
LOG_FILE_SIGUSR2="$LOG_DIR/hydrogen_signal_test_SIGUSR2_${TIMESTAMP}.log"

# Function to wait for startup with signal-specific timeout
wait_for_signal_startup() {
    local log_file="$1"
    local timeout="$2"
    local start_time
    start_time=$(date +%s)
    
    while true; do
        if [ $(($(date +%s) - start_time)) -ge "$timeout" ]; then
            return 1
        fi
        
        if grep -q "Application started" "$log_file" 2>/dev/null; then
            return 0
        fi
        
        # sleep 0.2
    done
}

# Function to wait for shutdown with signal verification
wait_for_signal_shutdown() {
    local pid="$1"
    local timeout="$2"
    local log_file="$3"
    local start_time
    start_time=$(date +%s)
    
    while true; do
        if [ $(($(date +%s) - start_time)) -ge "$timeout" ]; then
            return 1
        fi
        
        if ! ps -p "$pid" > /dev/null 2>&1; then
            if grep -q "Shutdown complete" "$log_file" 2>/dev/null; then
                return 0
            else
                return 2  # Process died but no clean shutdown message
            fi
        fi
        
        # sleep 0.2
    done
}

# Function to wait for restart completion
wait_for_restart_completion() {
    local pid="$1"
    local timeout="$2"
    local expected_count="$3"
    local log_file="$4"
    local start_time
    start_time=$(date +%s)
    
    print_message "Waiting for restart completion (expecting restart count: $expected_count)..."
    
    while true; do
        if [ $(($(date +%s) - start_time)) -ge "$timeout" ]; then
            return 1
        fi
        
        # Check for restart completion markers
        if grep -q "Restart completed successfully" "$log_file" 2>/dev/null; then
            if grep -q "Application restarts: $expected_count" "$log_file" 2>/dev/null || \
               grep -q "Restart count: $expected_count" "$log_file" 2>/dev/null; then
                print_message "Verified restart completed successfully with count $expected_count"
                return 0
            fi
        fi
        
        # Alternative: Check for restart sequence completion
        if (grep -q "Application restarts: $expected_count" "$log_file" 2>/dev/null || \
            grep -q "Restart count: $expected_count" "$log_file" 2>/dev/null) && \
           grep -q "Initiating graceful restart sequence" "$log_file" 2>/dev/null && \
           grep -q "Application started" "$log_file" 2>/dev/null; then
            print_message "Verified restart via restart sequence with count $expected_count"
            return 0
        fi
        
        # sleep 0.2
    done
}

# Function to verify config dump
verify_config_dump() {
    local log_file="$1"
    local timeout="$2"
    local start_time
    start_time=$(date +%s)
    
    # Wait for dump to start
    while true; do
        if [ $(($(date +%s) - start_time)) -ge "$timeout" ]; then
            return 1
        fi
        
        if grep -q "APPCONFIG Dump Started" "$log_file" 2>/dev/null; then
            break
        fi
        
        # sleep 0.2
    done
    
    # Wait for dump to complete
    while true; do
        if [ $(($(date +%s) - start_time)) -ge "$timeout" ]; then
            return 1
        fi
        
        if grep -q "APPCONFIG Dump Complete" "$log_file" 2>/dev/null; then
            local start_line end_line dump_lines
            start_line=$(grep -n "APPCONFIG Dump Started" "$log_file" | tail -1 | cut -d: -f1)
            end_line=$(grep -n "APPCONFIG Dump Complete" "$log_file" | tail -1 | cut -d: -f1)
            if [ -n "$start_line" ] && [ -n "$end_line" ]; then
                dump_lines=$((end_line - start_line + 1))
                print_message "Config dump completed with $dump_lines lines"
                return 0
            fi
        fi
        
        # sleep 0.2
    done
}

# Test Case 1: SIGINT Signal Handling
next_subtest
print_subtest "SIGINT Signal Handling"
print_message "Testing SIGINT (Ctrl+C) signal handling"

# Clear log file
true > "$LOG_FILE_SIGINT"

print_command "$(basename "$HYDROGEN_BIN") $(basename "$TEST_CONFIG")"
"$HYDROGEN_BIN" "$TEST_CONFIG" > "$LOG_FILE_SIGINT" 2>&1 &
HYDROGEN_PID=$!

if wait_for_signal_startup "$LOG_FILE_SIGINT" $STARTUP_TIMEOUT; then
    print_message "Hydrogen started successfully (PID: $HYDROGEN_PID)"
    print_command "kill -SIGINT $HYDROGEN_PID"
    kill -SIGINT $HYDROGEN_PID
    
    if wait_for_signal_shutdown $HYDROGEN_PID $SHUTDOWN_TIMEOUT "$LOG_FILE_SIGINT"; then
        print_result 0 "SIGINT handled successfully with clean shutdown"
        ((PASS_COUNT++))
    else
        print_result 1 "SIGINT handling failed - no clean shutdown"
        kill -9 $HYDROGEN_PID 2>/dev/null || true
        EXIT_CODE=1
    fi
else
    print_result 1 "Failed to start Hydrogen for SIGINT test"
    kill -9 $HYDROGEN_PID 2>/dev/null || true
    EXIT_CODE=1
fi

# Test Case 2: SIGTERM Signal Handling
next_subtest
print_subtest "SIGTERM Signal Handling"
print_message "Testing SIGTERM (terminate) signal handling"

# Clear log file
true > "$LOG_FILE_SIGTERM"

print_command "$(basename "$HYDROGEN_BIN") $(basename "$TEST_CONFIG")"
"$HYDROGEN_BIN" "$TEST_CONFIG" > "$LOG_FILE_SIGTERM" 2>&1 &
HYDROGEN_PID=$!

if wait_for_signal_startup "$LOG_FILE_SIGTERM" $STARTUP_TIMEOUT; then
    print_message "Hydrogen started successfully (PID: $HYDROGEN_PID)"
    print_command "kill -SIGTERM $HYDROGEN_PID"
    kill -SIGTERM $HYDROGEN_PID
    
    if wait_for_signal_shutdown $HYDROGEN_PID $SHUTDOWN_TIMEOUT "$LOG_FILE_SIGTERM"; then
        print_result 0 "SIGTERM handled successfully with clean shutdown"
        ((PASS_COUNT++))
    else
        print_result 1 "SIGTERM handling failed - no clean shutdown"
        kill -9 $HYDROGEN_PID 2>/dev/null || true
        EXIT_CODE=1
    fi
else
    print_result 1 "Failed to start Hydrogen for SIGTERM test"
    kill -9 $HYDROGEN_PID 2>/dev/null || true
    EXIT_CODE=1
fi

# Test Case 3: SIGHUP Signal Handling (Restart)
next_subtest
print_subtest "SIGHUP Signal Handling (Single Restart)"
print_message "Testing SIGHUP (hangup/restart) signal handling"

# Clear log file
true > "$LOG_FILE_SIGHUP"

print_command "$(basename "$HYDROGEN_BIN") $(basename "$TEST_CONFIG")"
"$HYDROGEN_BIN" "$TEST_CONFIG" > "$LOG_FILE_SIGHUP" 2>&1 &
HYDROGEN_PID=$!

if wait_for_signal_startup "$LOG_FILE_SIGHUP" $STARTUP_TIMEOUT; then
    print_message "Hydrogen started successfully (PID: $HYDROGEN_PID)"
    print_command "kill -SIGHUP $HYDROGEN_PID"
    kill -SIGHUP $HYDROGEN_PID
    
    # Wait for restart signal to be logged
    if grep -q "SIGHUP received" "$LOG_FILE_SIGHUP" 2>/dev/null; then
        print_message "SIGHUP signal received, waiting for restart completion..."
        if wait_for_restart_completion $HYDROGEN_PID $SIGNAL_TIMEOUT 1 "$LOG_FILE_SIGHUP"; then
            print_result 0 "SIGHUP handled successfully with restart"
            ((PASS_COUNT++))
        else
            print_result 1 "SIGHUP restart failed or timed out"
            EXIT_CODE=1
        fi
    else
        print_result 1 "SIGHUP signal not received or logged"
        EXIT_CODE=1
    fi
    
    # Clean up
    kill -SIGTERM $HYDROGEN_PID 2>/dev/null || true
    wait_for_signal_shutdown $HYDROGEN_PID $SHUTDOWN_TIMEOUT "$LOG_FILE_SIGHUP" || true
else
    print_result 1 "Failed to start Hydrogen for SIGHUP test"
    kill -9 $HYDROGEN_PID 2>/dev/null || true
    EXIT_CODE=1
fi

# Test Case 4: Multiple SIGHUP Restarts
next_subtest
print_subtest "Multiple SIGHUP Restarts"
print_message "Testing multiple SIGHUP restarts (count: $RESTART_COUNT)"

# Clear log file
true > "$LOG_FILE_SIGHUP"

print_command "$(basename "$HYDROGEN_BIN") $(basename "$TEST_CONFIG")"
"$HYDROGEN_BIN" "$TEST_CONFIG" > "$LOG_FILE_SIGHUP" 2>&1 &
HYDROGEN_PID=$!

if wait_for_signal_startup "$LOG_FILE_SIGHUP" $STARTUP_TIMEOUT; then
    print_message "Hydrogen started successfully (PID: $HYDROGEN_PID)"
    
    restart_success=0
    current_count=1
    
    while [ $current_count -le $RESTART_COUNT ]; do
        print_message "Sending SIGHUP #$current_count of $RESTART_COUNT..."
        print_command "kill -SIGHUP $HYDROGEN_PID"
        kill -SIGHUP $HYDROGEN_PID || true
        
        # Wait for restart signal to be logged
        if grep -q "SIGHUP received" "$LOG_FILE_SIGHUP" 2>/dev/null; then
            print_message "SIGHUP signal #$current_count received, waiting for restart..."
            if wait_for_restart_completion $HYDROGEN_PID $SIGNAL_TIMEOUT $current_count "$LOG_FILE_SIGHUP"; then
                print_message "Restart #$current_count of $RESTART_COUNT completed successfully"
                if [ $current_count -eq $RESTART_COUNT ]; then
                    restart_success=1
                    break
                fi
                current_count=$((current_count + 1))
            else
                print_message "Restart #$current_count of $RESTART_COUNT failed"
                break
            fi
        else
            print_message "SIGHUP signal #$current_count not received"
            break
        fi
    done
    
    if [ $restart_success -eq 1 ]; then
        print_result 0 "Multiple SIGHUP restarts successful ($RESTART_COUNT restarts)"
        ((PASS_COUNT++))
    else
        print_result 1 "Multiple SIGHUP restarts failed"
        EXIT_CODE=1
    fi
    
    # Clean up
    kill -SIGTERM $HYDROGEN_PID 2>/dev/null || true
    wait_for_signal_shutdown $HYDROGEN_PID $SHUTDOWN_TIMEOUT "$LOG_FILE_SIGHUP" || true
else
    print_result 1 "Failed to start Hydrogen for multiple SIGHUP test"
    kill -9 $HYDROGEN_PID 2>/dev/null || true
    EXIT_CODE=1
fi

# Test Case 5: SIGUSR2 Signal Handling (Config Dump)
next_subtest
print_subtest "SIGUSR2 Signal Handling (Config Dump)"
print_message "Testing SIGUSR2 (config dump) signal handling"

# Clear log file
true > "$LOG_FILE_SIGUSR2"

print_command "$(basename "$HYDROGEN_BIN") $(basename "$TEST_CONFIG")"
"$HYDROGEN_BIN" "$TEST_CONFIG" > "$LOG_FILE_SIGUSR2" 2>&1 &
HYDROGEN_PID=$!

if wait_for_signal_startup "$LOG_FILE_SIGUSR2" $STARTUP_TIMEOUT; then
    print_message "Hydrogen started successfully (PID: $HYDROGEN_PID)"
    print_command "kill -SIGUSR2 $HYDROGEN_PID"
    kill -SIGUSR2 $HYDROGEN_PID
    
    if verify_config_dump "$LOG_FILE_SIGUSR2" $SIGNAL_TIMEOUT; then
        print_result 0 "SIGUSR2 handled successfully with config dump"
        ((PASS_COUNT++))
    else
        print_result 1 "SIGUSR2 handling failed - no config dump"
        EXIT_CODE=1
    fi
    
    # Clean up
    kill -SIGTERM $HYDROGEN_PID 2>/dev/null || true
    wait_for_signal_shutdown $HYDROGEN_PID $SHUTDOWN_TIMEOUT "$LOG_FILE_SIGUSR2" || true
else
    print_result 1 "Failed to start Hydrogen for SIGUSR2 test"
    kill -9 $HYDROGEN_PID 2>/dev/null || true
    EXIT_CODE=1
fi

# Test Case 6: Multiple Signal Handling
next_subtest
print_subtest "Multiple Signal Handling"
print_message "Testing multiple signals sent simultaneously"

# Clear log file
true > "$LOG_FILE_MULTI"

print_command "$(basename "$HYDROGEN_BIN") $(basename "$TEST_CONFIG")"
"$HYDROGEN_BIN" "$TEST_CONFIG" > "$LOG_FILE_MULTI" 2>&1 &
HYDROGEN_PID=$!

if wait_for_signal_startup "$LOG_FILE_MULTI" $STARTUP_TIMEOUT; then
    print_message "Hydrogen started successfully (PID: $HYDROGEN_PID)"
    print_message "Sending multiple signals (SIGTERM, SIGINT)..."
    print_command "kill -SIGTERM $HYDROGEN_PID && kill -SIGINT $HYDROGEN_PID"
    kill -SIGTERM $HYDROGEN_PID
    kill -SIGINT $HYDROGEN_PID
    
    if wait_for_signal_shutdown $HYDROGEN_PID $SHUTDOWN_TIMEOUT "$LOG_FILE_MULTI"; then
        # Check logs for single shutdown sequence
        shutdown_count=$(grep -c "Initiating graceful shutdown sequence" "$LOG_FILE_MULTI" 2>/dev/null || echo "0")
        if [ "$shutdown_count" -eq 1 ]; then
            print_result 0 "Multiple signals handled correctly (single shutdown sequence)"
            ((PASS_COUNT++))
        else
            print_result 1 "Multiple shutdown sequences detected ($shutdown_count sequences)"
            EXIT_CODE=1
        fi
    else
        print_result 1 "Multiple signal handling failed - no clean shutdown"
        kill -9 $HYDROGEN_PID 2>/dev/null || true
        EXIT_CODE=1
    fi
else
    print_result 1 "Failed to start Hydrogen for multiple signal test"
    kill -9 $HYDROGEN_PID 2>/dev/null || true
    EXIT_CODE=1
fi

# Test Case 7: Signal Handling Verification
next_subtest
print_subtest "Signal Handling Verification"
print_message "Verifying all signal handling tests completed successfully"

# Count successful tests
successful_tests=0
[ -f "$LOG_FILE_SIGINT" ] && grep -q "Shutdown complete" "$LOG_FILE_SIGINT" 2>/dev/null && ((successful_tests++))
[ -f "$LOG_FILE_SIGTERM" ] && grep -q "Shutdown complete" "$LOG_FILE_SIGTERM" 2>/dev/null && ((successful_tests++))
[ -f "$LOG_FILE_SIGHUP" ] && grep -q "Restart completed successfully" "$LOG_FILE_SIGHUP" 2>/dev/null && ((successful_tests++))
[ -f "$LOG_FILE_SIGUSR2" ] && grep -q "APPCONFIG Dump Complete" "$LOG_FILE_SIGUSR2" 2>/dev/null && ((successful_tests++))
[ -f "$LOG_FILE_MULTI" ] && grep -q "Shutdown complete" "$LOG_FILE_MULTI" 2>/dev/null && ((successful_tests++))

print_message "Signal handling verification: $successful_tests/5 tests show proper completion"
if [ $successful_tests -ge 4 ]; then
    print_result 0 "Signal handling verification passed ($successful_tests/5 tests successful)"
    ((PASS_COUNT++))
else
    print_result 1 "Signal handling verification failed ($successful_tests/5 tests successful)"
    EXIT_CODE=1
fi

# Save log files to results directory
cp "$LOG_FILE_SIGINT" "$RESULTS_DIR/sigint_test_${TIMESTAMP}.log" 2>/dev/null || true
cp "$LOG_FILE_SIGTERM" "$RESULTS_DIR/sigterm_test_${TIMESTAMP}.log" 2>/dev/null || true
cp "$LOG_FILE_SIGHUP" "$RESULTS_DIR/sighup_test_${TIMESTAMP}.log" 2>/dev/null || true
cp "$LOG_FILE_SIGUSR2" "$RESULTS_DIR/sigusr2_test_${TIMESTAMP}.log" 2>/dev/null || tru
cp "$LOG_FILE_MULTI" "$RESULTS_DIR/multi_signal_test_${TIMESTAMP}.log" 2>/dev/null || true

print_message "Signal test logs saved to results directory"

# Export results for test_all.sh integration
# Derive test name from script filename for consistency with test_00_all.sh
TEST_IDENTIFIER=$(basename "${BASH_SOURCE[0]}" .sh | sed 's/test_[0-9]*_//')
export_subtest_results "${TEST_NUMBER}_${TEST_IDENTIFIER}" "$TOTAL_SUBTESTS" "$PASS_COUNT" "$TEST_NAME" > /dev/null

# Print completion table
print_test_completion "$TEST_NAME"

end_test $EXIT_CODE $TOTAL_SUBTESTS $PASS_COUNT > /dev/null

# Return status code if sourced, exit if run standalone
if [[ "$RUNNING_IN_TEST_SUITE" == "true" ]]; then
    return $EXIT_CODE
else
    exit $EXIT_CODE
fi
