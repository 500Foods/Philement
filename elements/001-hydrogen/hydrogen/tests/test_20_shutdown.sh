#!/bin/bash
#
# test_20_shutdown.sh - Hydrogen Shutdown Test Suite
# Version: 1.1.0
# 
# Version History:
# 1.0.0 - Initial version with basic shutdown testing
# 1.1.0 - Enhanced with proper error handling, modular functions, and shellcheck compliance
#
# Description:
# Tests the shutdown functionality of the Hydrogen server, including graceful shutdown,
# signal handling, and cleanup verification. Validates that the server properly terminates
# all processes and releases resources.
#
# Usage: ./test_20_shutdown.sh
#
# Dependencies:
# - support_utils.sh for common test utilities
# - Hydrogen server binary
# - Process monitoring tools (ps, pgrep)
#

# Script identification
readonly SCRIPT_NAME="test_20_shutdown.sh"
readonly SCRIPT_VERSION="1.1.0"

# Display script information
echo "Starting ${SCRIPT_NAME} v${SCRIPT_VERSION}"
echo "Hydrogen Shutdown Test Suite"
echo "================================"

# Get the directory where this script is located
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
HYDROGEN_DIR="$( cd "$SCRIPT_DIR/.." && pwd )"

# Include the common test utilities
# shellcheck source=support_utils.sh
source "$SCRIPT_DIR/support_utils.sh"

# Configuration
if [ -f "$HYDROGEN_DIR/hydrogen_release" ]; then
    HYDROGEN_BIN="$HYDROGEN_DIR/hydrogen_release"
    print_info "Using release build for testing"
else
    HYDROGEN_BIN="$HYDROGEN_DIR/hydrogen"
    print_info "Using standard build"
fi

CONFIG_FILE="$SCRIPT_DIR/configs/hydrogen_test_min.json"  # Use minimal config by default
STARTUP_TIMEOUT=5     # Shorter timeout for startup
SHUTDOWN_TIMEOUT=10   # Shorter timeout for shutdown
LOG_FILE="$SCRIPT_DIR/logs/hydrogen_shutdown_test.log"
RESULTS_DIR="$SCRIPT_DIR/results"
DIAG_DIR="$SCRIPT_DIR/diagnostics"

# Create output directories
mkdir -p "$RESULTS_DIR" "$DIAG_DIR" "$(dirname "$LOG_FILE")"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
RESULT_LOG="$RESULTS_DIR/shutdown_test_${TIMESTAMP}.log"
DIAG_TEST_DIR="$DIAG_DIR/shutdown_test_${TIMESTAMP}"
mkdir -p "$DIAG_TEST_DIR"

# Initialize subtest tracking
TOTAL_SUBTESTS=5  # Startup, signal handling, shutdown message, completion, process cleanup
PASS_COUNT=0

# Declare cleanup function for trap usage
# shellcheck disable=SC2317  # Function called via trap
cleanup_test_environment() {
    if [[ -n "${HYDROGEN_PID:-}" ]] && kill -0 "${HYDROGEN_PID}" 2>/dev/null; then
        echo "Cleaning up test process ${HYDROGEN_PID}"
        kill -TERM "${HYDROGEN_PID}" 2>/dev/null || true
        sleep 2
        
        # Force kill if still running
        if kill -0 "${HYDROGEN_PID}" 2>/dev/null; then
            echo "Force killing test process ${HYDROGEN_PID}"
            kill -KILL "${HYDROGEN_PID}" 2>/dev/null || true
        fi
    fi
}

# Wait for server startup to complete
wait_for_startup_completion() {
    local startup_start
    startup_start=$(date +%s)
    
    while true; do
        local current_time elapsed
        current_time=$(date +%s)
        elapsed=$((current_time - startup_start))
        
        if [ $elapsed -ge $STARTUP_TIMEOUT ]; then
            print_result 1 "Startup timeout" | tee -a "$RESULT_LOG"
            return 1
        fi
        
        if grep -q "Application started" "$LOG_FILE"; then
            print_info "Startup complete" | tee -a "$RESULT_LOG"
            return 0
        fi
        
        sleep 0.2
    done
}

# Monitor the shutdown process
monitor_shutdown_process() {
    local shutdown_start
    shutdown_start=$(date +%s)
    
    while true; do
        local current_time elapsed
        current_time=$(date +%s)
        elapsed=$((current_time - shutdown_start))
        
        # Check if process exited
        if ! ps -p "$HYDROGEN_PID" > /dev/null; then
            if grep -q "Shutdown complete" "$LOG_FILE"; then
                local shutdown_duration=$((current_time - shutdown_start))
                print_info "Clean shutdown completed in ${shutdown_duration}s" | tee -a "$RESULT_LOG"
                return 0
            else
                print_result 1 "Process exited without shutdown message" | tee -a "$RESULT_LOG"
                cp "$LOG_FILE" "$DIAG_TEST_DIR/incomplete_shutdown.log"
                return 1
            fi
        fi
        
        # Check for timeout
        if [ $elapsed -ge $SHUTDOWN_TIMEOUT ]; then
            print_result 1 "Shutdown timeout" | tee -a "$RESULT_LOG"
            kill -9 "$HYDROGEN_PID"
            cp "$LOG_FILE" "$DIAG_TEST_DIR/timeout_shutdown.log"
            return 1
        fi
        
        sleep 0.2
    done
}

# Execute the main shutdown test
run_shutdown_test() {
    # Set up error handling
    trap cleanup_test_environment EXIT ERR
    
    # Start the test
    start_test "Hydrogen Shutdown Test" | tee -a "$RESULT_LOG"
    
    # Clean log file - use 'true' as no-op to fix SC2188
    true > "$LOG_FILE"
    
    # Launch Hydrogen
    print_info "Starting Hydrogen..." | tee -a "$RESULT_LOG"
    $HYDROGEN_BIN "$CONFIG_FILE" > "$LOG_FILE" 2>&1 &
    HYDROGEN_PID=$!
    
    if ! ps -p $HYDROGEN_PID > /dev/null; then
        print_result 1 "Failed to start Hydrogen" | tee -a "$RESULT_LOG"
        return 1
    fi
    
    # Wait for startup
    print_info "Waiting for startup..." | tee -a "$RESULT_LOG"
    if ! wait_for_startup_completion; then
        kill -9 $HYDROGEN_PID
        return 1
    fi
    
    # Brief pause to ensure stable state
    sleep 1
    
    # First subtest passed - clean startup
    ((PASS_COUNT++))
    
    # Send shutdown signal
    print_info "Initiating shutdown..." | tee -a "$RESULT_LOG"
    kill -SIGINT $HYDROGEN_PID
    
    # Monitor shutdown
    print_info "Monitoring shutdown..." | tee -a "$RESULT_LOG"
    if ! monitor_shutdown_process; then
        return 1
    fi
    
    # Analyze results and count successful subtests
    # Signal handling worked
    ((PASS_COUNT++))
    
    # Shutdown message was logged
    if grep -q "Shutdown complete" "$LOG_FILE"; then
        ((PASS_COUNT++))
    fi
    
    # Completed within timeout (already verified in monitor_shutdown_process)
    ((PASS_COUNT++))
    
    # No process remaining
    if ! ps -p $HYDROGEN_PID > /dev/null 2>&1; then
        ((PASS_COUNT++))
    fi
    
    # Get test name from script name
    TEST_NAME=$(basename "$0" .sh | sed 's/^test_//')
    
    # Export subtest results for test_all.sh
    export_subtest_results "$TEST_NAME" $TOTAL_SUBTESTS $PASS_COUNT
    
    print_info "Shutdown Test: $PASS_COUNT of $TOTAL_SUBTESTS subtests passed" | tee -a "$RESULT_LOG"
    
    if [ $PASS_COUNT -eq $TOTAL_SUBTESTS ]; then
        print_result 0 "PASSED" | tee -a "$RESULT_LOG"
        return 0
    else
        print_result 1 "FAILED" | tee -a "$RESULT_LOG"
        return 1
    fi
}

# Main execution function
main() {
    # Validate prerequisites
    if [[ ! -f "$HYDROGEN_BIN" ]]; then
        echo "Error: Hydrogen binary not found at $HYDROGEN_BIN"
        exit 1
    fi
    
    if [[ ! -f "$CONFIG_FILE" ]]; then
        echo "Error: Test configuration not found: $CONFIG_FILE"
        exit 1
    fi
    
    # Run the test
    if run_shutdown_test; then
        echo ""
        echo "✓ ${SCRIPT_NAME} completed successfully"
        exit 0
    else
        echo ""
        echo "✗ ${SCRIPT_NAME} completed with failures"
        exit 1
    fi
}

# Execute main function
main "$@"
