#!/bin/bash
#
# Hydrogen Shutdown Test Script
# Focused test for verifying clean shutdown behavior

# Get the directory where this script is located
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
HYDROGEN_DIR="$( cd "$SCRIPT_DIR/.." && pwd )"

# Include the common test utilities
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

# Start the test
start_test "Hydrogen Shutdown Test" | tee -a "$RESULT_LOG"

# Clean log file
> "$LOG_FILE"

# Launch Hydrogen
print_info "Starting Hydrogen..." | tee -a "$RESULT_LOG"
$HYDROGEN_BIN "$CONFIG_FILE" > "$LOG_FILE" 2>&1 &
HYDROGEN_PID=$!

if ! ps -p $HYDROGEN_PID > /dev/null; then
    print_result 1 "Failed to start Hydrogen" | tee -a "$RESULT_LOG"
    exit 1
fi

# Wait for startup
print_info "Waiting for startup..." | tee -a "$RESULT_LOG"
STARTUP_START=$(date +%s)
STARTUP_COMPLETE=false

while true; do
    CURRENT_TIME=$(date +%s)
    ELAPSED=$((CURRENT_TIME - STARTUP_START))
    
    if [ $ELAPSED -ge $STARTUP_TIMEOUT ]; then
        print_result 1 "Startup timeout" | tee -a "$RESULT_LOG"
        kill -9 $HYDROGEN_PID
        exit 1
    fi
    
    if grep -q "Application started" "$LOG_FILE"; then
        STARTUP_COMPLETE=true
        print_info "Startup complete" | tee -a "$RESULT_LOG"
        break
    fi
    
    sleep 0.2
done

# Brief pause to ensure stable state
sleep 1

# Send shutdown signal
print_info "Initiating shutdown..." | tee -a "$RESULT_LOG"
SHUTDOWN_START=$(date +%s)
kill -SIGINT $HYDROGEN_PID

# Monitor shutdown
print_info "Monitoring shutdown..." | tee -a "$RESULT_LOG"
SHUTDOWN_COMPLETE=false

while true; do
    CURRENT_TIME=$(date +%s)
    ELAPSED=$((CURRENT_TIME - SHUTDOWN_START))
    
    # Check if process exited
    if ! ps -p $HYDROGEN_PID > /dev/null; then
        if grep -q "Shutdown complete" "$LOG_FILE"; then
            SHUTDOWN_COMPLETE=true
            break
        else
            print_result 1 "Process exited without shutdown message" | tee -a "$RESULT_LOG"
            cp "$LOG_FILE" "$DIAG_TEST_DIR/incomplete_shutdown.log"
            exit 1
        fi
    fi
    
    # Check for timeout
    if [ $ELAPSED -ge $SHUTDOWN_TIMEOUT ]; then
        print_result 1 "Shutdown timeout" | tee -a "$RESULT_LOG"
        kill -9 $HYDROGEN_PID
        cp "$LOG_FILE" "$DIAG_TEST_DIR/timeout_shutdown.log"
        exit 1
    fi
    
    sleep 0.2
done

# Analyze results
if [ "$SHUTDOWN_COMPLETE" = true ]; then
    SHUTDOWN_DURATION=$((CURRENT_TIME - SHUTDOWN_START))
    print_info "Clean shutdown completed in ${SHUTDOWN_DURATION}s" | tee -a "$RESULT_LOG"
    print_result 0 "PASSED" | tee -a "$RESULT_LOG"
    exit 0
else
    print_result 1 "FAILED" | tee -a "$RESULT_LOG"
    exit 1
fi