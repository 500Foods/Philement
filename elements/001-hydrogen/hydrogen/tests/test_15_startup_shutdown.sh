#!/bin/bash
#
# Hydrogen Startup/Shutdown Test Script
# Tests startup and shutdown with both minimal and maximal configurations

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

# Test configurations
MIN_CONFIG="$SCRIPT_DIR/configs/hydrogen_test_min.json"
MAX_CONFIG="$SCRIPT_DIR/configs/hydrogen_test_max.json"

# Timeouts
STARTUP_TIMEOUT=10    # Seconds to wait for startup
SHUTDOWN_TIMEOUT=90   # Hard limit on shutdown time
SHUTDOWN_ACTIVITY_TIMEOUT=5  # Timeout if no new log activity

# Output files and directories
LOG_FILE="$SCRIPT_DIR/logs/hydrogen_test.log"
RESULTS_DIR="$SCRIPT_DIR/results"
DIAG_DIR="$SCRIPT_DIR/diagnostics"

# Create output directories
mkdir -p "$RESULTS_DIR" "$DIAG_DIR" "$(dirname "$LOG_FILE")"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
RESULT_LOG="$RESULTS_DIR/startup_shutdown_test_${TIMESTAMP}.log"
DIAG_TEST_DIR="$DIAG_DIR/startup_shutdown_test_${TIMESTAMP}"
mkdir -p "$DIAG_TEST_DIR"

# Initialize test counters
TOTAL_SUBTESTS=6  # 3 subtests × 2 configs
PASSED_SUBTESTS=0

# Start the test
start_test "Hydrogen Startup/Shutdown Test" | tee -a "$RESULT_LOG"

# Function to test a specific configuration
test_configuration() {
    local config_file="$1"
    local config_name=$(basename "$config_file" .json)
    local diag_config_dir="$DIAG_TEST_DIR/${config_name}"
    mkdir -p "$diag_config_dir"
    
    print_header "Testing configuration: $config_name" | tee -a "$RESULT_LOG"
    
    # Clean log file
    > "$LOG_FILE"
    
    # Launch Hydrogen
    print_info "Starting Hydrogen with $config_name..." | tee -a "$RESULT_LOG"
    $HYDROGEN_BIN "$config_file" > "$LOG_FILE" 2>&1 &
    HYDROGEN_PID=$!
    
    if ! ps -p $HYDROGEN_PID > /dev/null; then
        print_result 1 "Failed to start Hydrogen" | tee -a "$RESULT_LOG"
        return 1
    fi
    
    # Wait for startup
    print_info "Waiting for startup (max ${STARTUP_TIMEOUT}s)..." | tee -a "$RESULT_LOG"
    STARTUP_START=$(date +%s)
    STARTUP_COMPLETE=false
    
    while true; do
        CURRENT_TIME=$(date +%s)
        ELAPSED=$((CURRENT_TIME - STARTUP_START))
        
        if [ $ELAPSED -ge $STARTUP_TIMEOUT ]; then
            print_warning "Startup timeout after ${ELAPSED}s" | tee -a "$RESULT_LOG"
            break
        fi
        
        if grep -q "Application started" "$LOG_FILE"; then
            STARTUP_COMPLETE=true
            print_info "Startup completed in ${ELAPSED}s" | tee -a "$RESULT_LOG"
            ((PASSED_SUBTESTS++))
            break
        fi
        
        sleep 0.2
    done
    
    # Capture pre-shutdown diagnostics
    ps -eLf | grep $HYDROGEN_PID > "$diag_config_dir/pre_shutdown_threads.txt"
    cat /proc/$HYDROGEN_PID/status > "$diag_config_dir/pre_shutdown_status.txt" 2>/dev/null
    ls -l /proc/$HYDROGEN_PID/fd/ > "$diag_config_dir/pre_shutdown_fds.txt" 2>/dev/null
    
    # Send shutdown signal
    print_info "Initiating shutdown..." | tee -a "$RESULT_LOG"
    SHUTDOWN_START=$(date +%s)
    kill -SIGINT $HYDROGEN_PID
    
    # Monitor shutdown
    LOG_SIZE_BEFORE=$(stat -c %s "$LOG_FILE" 2>/dev/null || echo 0)
    LAST_ACTIVITY=$(date +%s)
    SHUTDOWN_COMPLETE=false
    
    while ps -p $HYDROGEN_PID >/dev/null; do
        CURRENT_TIME=$(date +%s)
        ELAPSED=$((CURRENT_TIME - SHUTDOWN_START))
        
        # Check for timeout
        if [ $ELAPSED -ge $SHUTDOWN_TIMEOUT ]; then
            print_result 1 "Shutdown timeout after ${ELAPSED}s" | tee -a "$RESULT_LOG"
            
            # Collect diagnostics
            ps -eLo pid,tid,class,rtprio,stat,wchan:32,comm | grep $HYDROGEN_PID >> "$diag_config_dir/stuck_threads.txt"
            lsof -p $HYDROGEN_PID >> "$diag_config_dir/stuck_open_files.txt" 2>/dev/null
            
            kill -9 $HYDROGEN_PID
            cp "$LOG_FILE" "$diag_config_dir/timeout_shutdown.log"
            return 1
        fi
        
        # Check for log activity
        LOG_SIZE_NOW=$(stat -c %s "$LOG_FILE" 2>/dev/null || echo 0)
        if [ "$LOG_SIZE_NOW" -gt "$LOG_SIZE_BEFORE" ]; then
            LOG_SIZE_BEFORE=$LOG_SIZE_NOW
            LAST_ACTIVITY=$(date +%s)
        else
            INACTIVE_TIME=$((CURRENT_TIME - LAST_ACTIVITY))
            if [ "$INACTIVE_TIME" -ge "$SHUTDOWN_ACTIVITY_TIMEOUT" ]; then
                # Collect diagnostics but continue waiting
                ps -eLf | grep "$HYDROGEN_PID" >> "$diag_config_dir/inactive_threads.txt" 2>/dev/null
            fi
        fi
        
        sleep 0.2
    done
    
    # Check shutdown success
    SHUTDOWN_END=$(date +%s)
    SHUTDOWN_DURATION=$((SHUTDOWN_END - SHUTDOWN_START))
    
    # Subtest 2: Shutdown timing
    if [ $SHUTDOWN_DURATION -lt $SHUTDOWN_TIMEOUT ]; then
        print_info "Shutdown completed in ${SHUTDOWN_DURATION}s" | tee -a "$RESULT_LOG"
        ((PASSED_SUBTESTS++))
    fi
    
    # Subtest 3: Clean shutdown verification
    if grep -q "Shutdown complete" "$LOG_FILE" && ! grep -q "threads still active" "$LOG_FILE"; then
        print_info "Clean shutdown verified" | tee -a "$RESULT_LOG"
        ((PASSED_SUBTESTS++))
        SHUTDOWN_COMPLETE=true
    else
        print_warning "Shutdown completed but with issues" | tee -a "$RESULT_LOG"
        cp "$LOG_FILE" "$diag_config_dir/shutdown_issues.log"
    fi
    
    # Copy full log for reference
    cp "$LOG_FILE" "$diag_config_dir/full_log.txt"
    
    if [ "$STARTUP_COMPLETE" = true ] && [ "$SHUTDOWN_COMPLETE" = true ]; then
        print_result 0 "Configuration $config_name test passed" | tee -a "$RESULT_LOG"
        return 0
    else
        print_result 1 "Configuration $config_name test failed" | tee -a "$RESULT_LOG"
        return 1
    fi
}

# Test minimal configuration
MIN_RESULT=0
print_header "Testing Minimal Configuration" | tee -a "$RESULT_LOG"
test_configuration "$MIN_CONFIG"
MIN_RESULT=$?

# Test maximal configuration
MAX_RESULT=0
print_header "Testing Maximal Configuration" | tee -a "$RESULT_LOG"
test_configuration "$MAX_CONFIG"
MAX_RESULT=$?

# Export subtest results for test_all.sh
export_subtest_results $TOTAL_SUBTESTS $PASSED_SUBTESTS

# Print final summary
print_header "Test Summary" | tee -a "$RESULT_LOG"
print_info "Total subtests: $TOTAL_SUBTESTS" | tee -a "$RESULT_LOG"
print_info "Passed subtests: $PASSED_SUBTESTS" | tee -a "$RESULT_LOG"

if [ $MIN_RESULT -eq 0 ] && [ $MAX_RESULT -eq 0 ]; then
    print_result 0 "All configuration tests passed" | tee -a "$RESULT_LOG"
    end_test 0 "Startup/Shutdown Test" | tee -a "$RESULT_LOG"
    exit 0
else
    print_result 1 "One or more configuration tests failed" | tee -a "$RESULT_LOG"
    end_test 1 "Startup/Shutdown Test" | tee -a "$RESULT_LOG"
    exit 1
fi