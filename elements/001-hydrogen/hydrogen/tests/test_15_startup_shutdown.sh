#!/bin/bash
#
# Hydrogen Startup/Shutdown Test Script
# Tests the startup and shutdown process with enhanced diagnostics for stall detection

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
    print_info "Release build not found, using standard build"
fi
CONFIG_FILE="$1"                       # Accept config file as parameter
STARTUP_TIMEOUT=10                     # Seconds to wait for startup
SHUTDOWN_TIMEOUT=90                    # Hard limit on shutdown time (90s for thorough testing)
SHUTDOWN_ACTIVITY_TIMEOUT=5            # Timeout if no new log activity
LOG_FILE="$SCRIPT_DIR/hydrogen_test.log"  # Runtime log to analyze
RESULTS_DIR="$SCRIPT_DIR/results"        # Directory for test results
DIAG_DIR="$SCRIPT_DIR/diagnostics"       # Directory for diagnostic data

# Check if config file is provided
if [ -z "$CONFIG_FILE" ]; then
    print_warning "Usage: $0 <config_file.json>"
    print_info "Example: $0 hydrogen_test_min.json"
    exit 1
fi

# Create output directories
mkdir -p "$RESULTS_DIR" "$DIAG_DIR"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
RESULT_LOG="$RESULTS_DIR/test_${TIMESTAMP}_$(basename $CONFIG_FILE .json).log"
DIAG_TEST_DIR="$DIAG_DIR/test_${TIMESTAMP}_$(basename $CONFIG_FILE .json)"
mkdir -p "$DIAG_TEST_DIR"

# Start the test
start_test "Hydrogen Startup/Shutdown Test" | tee -a "$RESULT_LOG"
print_info "Config: $(convert_to_relative_path "$CONFIG_FILE")" | tee -a "$RESULT_LOG"

# Start with clean log
> "$LOG_FILE"

# Launch Hydrogen in background
print_info "Starting Hydrogen with config: $(convert_to_relative_path "$CONFIG_FILE")" | tee -a "$RESULT_LOG"
$HYDROGEN_BIN "$CONFIG_FILE" > "$LOG_FILE" 2>&1 &
HYDROGEN_PID=$!
print_info "Started Hydrogen with PID: $HYDROGEN_PID" | tee -a "$RESULT_LOG"

# Check if process is running
if ! ps -p $HYDROGEN_PID > /dev/null; then
    print_result 1 "Hydrogen failed to start" | tee -a "$RESULT_LOG"
    exit 1
fi

# Wait for startup with active log monitoring
print_info "Waiting for startup (max ${STARTUP_TIMEOUT}s)..." | tee -a "$RESULT_LOG"

# Initialize counters and state
STARTUP_START=$(date +%s)
LOG_LINE_COUNT=0
STARTUP_COMPLETE=false

# Monitor for startup completion or timeout
while true; do
    # Check if we've exceeded the timeout
    CURRENT_TIME=$(date +%s)
    ELAPSED=$((CURRENT_TIME - STARTUP_START))
    
    if [ $ELAPSED -ge $STARTUP_TIMEOUT ]; then
        printf "\n" | tee -a "$RESULT_LOG"
        print_warning "Startup timeout after ${ELAPSED}s" | tee -a "$RESULT_LOG"
        break
    fi
    
    # Check log file for new content
    if [ -f "$LOG_FILE" ]; then
        # Count current lines
        NEW_LOG_LINE_COUNT=$(wc -l < "$LOG_FILE")
        
        # Update count display (in-place)
        if [ $NEW_LOG_LINE_COUNT -ne $LOG_LINE_COUNT ]; then
            printf "\rReceived log lines: %d" $NEW_LOG_LINE_COUNT | tee -a "$RESULT_LOG"
            LOG_LINE_COUNT=$NEW_LOG_LINE_COUNT
            
            # Check for log message indicating startup is complete
            if grep -q "Application started" "$LOG_FILE"; then
                STARTUP_COMPLETE=true
                printf "\n" | tee -a "$RESULT_LOG"
                STARTUP_MS=$((ELAPSED * 1000 / 1000 % 1000))  # Calculate milliseconds
                print_info "Startup completed in $ELAPSED.$STARTUP_MS seconds" | tee -a "$RESULT_LOG"
                break
            fi
        fi
    fi
    
    # Small sleep to avoid consuming CPU
    sleep 0.2
done

# Check if process is still running after startup
if ! ps -p $HYDROGEN_PID > /dev/null; then
    print_result 1 "Hydrogen crashed during startup" | tee -a "$RESULT_LOG"
    cp "$LOG_FILE" "$DIAG_TEST_DIR/startup_crash.log"
    exit 1
fi

# Capture pre-shutdown diagnostics
print_info "Capturing pre-shutdown state..." | tee -a "$RESULT_LOG"
ps -eLf | grep $HYDROGEN_PID > "$DIAG_TEST_DIR/pre_shutdown_threads.txt"
cat /proc/$HYDROGEN_PID/status > "$DIAG_TEST_DIR/pre_shutdown_status.txt" 2>/dev/null
ls -l /proc/$HYDROGEN_PID/fd/ > "$DIAG_TEST_DIR/pre_shutdown_fds.txt" 2>/dev/null

# Send shutdown signal
print_header "Initiating shutdown at $(date +%H:%M:%S.%3N)" | tee -a "$RESULT_LOG"
SHUTDOWN_START=$(date +%s)
kill -SIGINT $HYDROGEN_PID

# Monitor for log activity with timeout
LOG_SIZE_BEFORE=$(stat -c %s "$LOG_FILE" 2>/dev/null || echo 0)
LAST_ACTIVITY=$(date +%s)
# LOG_LINE_COUNT is already initialized and tracked during startup

print_info "Monitoring shutdown progress..." | tee -a "$RESULT_LOG"
print_info "Current log line count: $LOG_LINE_COUNT lines" | tee -a "$RESULT_LOG"

# While hydrogen is still running
while ps -p $HYDROGEN_PID >/dev/null; do
    # Check how long we've been trying to shutdown
    CURRENT_TIME=$(date +%s)
    ELAPSED=$((CURRENT_TIME - SHUTDOWN_START))
    
    # If shutdown is taking too long, collect diagnostics and force kill
    if [ $ELAPSED -ge $SHUTDOWN_TIMEOUT ]; then
        print_result 1 "Shutdown timeout after ${ELAPSED}s - collecting diagnostics" | tee -a "$RESULT_LOG"
        
        # Comprehensive diagnostics collection when stuck
        echo "Thread states:" | tee -a "$DIAG_TEST_DIR/stuck_threads.txt"
        ps -eLo pid,tid,class,rtprio,stat,wchan:32,comm | grep $HYDROGEN_PID >> "$DIAG_TEST_DIR/stuck_threads.txt" 2>/dev/null
        
        echo "Open files:" | tee -a "$DIAG_TEST_DIR/stuck_open_files.txt"
        lsof -p $HYDROGEN_PID >> "$DIAG_TEST_DIR/stuck_open_files.txt" 2>/dev/null
        
        echo "Process tree:" | tee -a "$DIAG_TEST_DIR/stuck_process_tree.txt"
        pstree -pla $HYDROGEN_PID >> "$DIAG_TEST_DIR/stuck_process_tree.txt" 2>/dev/null
        
        echo "Memory maps:" | tee -a "$DIAG_TEST_DIR/stuck_memory_maps.txt"
        cat /proc/$HYDROGEN_PID/maps >> "$DIAG_TEST_DIR/stuck_memory_maps.txt" 2>/dev/null
        
        # Deep thread analysis for each thread
        mkdir -p "$DIAG_TEST_DIR/thread_stacks"
        for TID in $(ps -L -o lwp $HYDROGEN_PID | grep -v LWP); do
            # Thread status from proc filesystem
            cat /proc/$HYDROGEN_PID/task/$TID/status > "$DIAG_TEST_DIR/thread_stacks/thread_${TID}_status.txt" 2>/dev/null
            cat /proc/$HYDROGEN_PID/task/$TID/stack > "$DIAG_TEST_DIR/thread_stacks/thread_${TID}_kernel_stack.txt" 2>/dev/null
            
            # Syscall info (what is the thread waiting on?)
            cat /proc/$HYDROGEN_PID/task/$TID/syscall > "$DIAG_TEST_DIR/thread_stacks/thread_${TID}_syscall.txt" 2>/dev/null
            
            # Get WCHAN info (kernel function where thread is sleeping)
            ps -o wchan= -p $TID > "$DIAG_TEST_DIR/thread_stacks/thread_${TID}_wchan.txt" 2>/dev/null
        done
        
        # Force termination
        print_warning "Forcing termination after ${ELAPSED}s" | tee -a "$RESULT_LOG"
        kill -9 $HYDROGEN_PID
        break
    fi
    
    # Check for log file activity
    if [ -f "$LOG_FILE" ]; then
        LOG_SIZE_NOW=$(stat -c %s "$LOG_FILE" 2>/dev/null || echo 0)
        if [ "$LOG_SIZE_NOW" -gt "$LOG_SIZE_BEFORE" ]; then
            # Log has new content, reset activity timer
            LOG_SIZE_BEFORE=$LOG_SIZE_NOW
            LAST_ACTIVITY=$(date +%s)
            
            # Count the current number of lines in the log file
            NEW_LOG_LINE_COUNT=$(wc -l < "$LOG_FILE")
            LINES_ADDED=$((NEW_LOG_LINE_COUNT - LOG_LINE_COUNT))
            LOG_LINE_COUNT=$NEW_LOG_LINE_COUNT
            
            # Update count display in-place, showing line count only with consistent padding
            printf "\rShutdown progress: Lines: %-6d  No log activity: %-4s" $LOG_LINE_COUNT "0s" | tee -a "$RESULT_LOG"
        else
            # No new log activity, check how long it's been inactive
            INACTIVE_TIME=$((CURRENT_TIME - LAST_ACTIVITY))
            
            # Always display current line count with inactivity timer and consistent padding
            printf "\rShutdown progress: Lines: %-6d  No log activity: %-4s" $LOG_LINE_COUNT "${INACTIVE_TIME}s" | tee -a "$RESULT_LOG"
            
            # If no activity for SHUTDOWN_ACTIVITY_TIMEOUT seconds, collect diagnostics
            # Use a simpler approach with hardcoded interval to avoid bash arithmetic errors
            MOD_RESULT=$(expr $INACTIVE_TIME % 5 2>/dev/null || echo 1)
            if [ "$INACTIVE_TIME" -ge "$SHUTDOWN_ACTIVITY_TIMEOUT" ] && [ "$MOD_RESULT" = "0" ]; then
                # Collect diagnostics but don't output to console (only to log files)
                # Capture current thread state
                echo "Current threads at inactive +${INACTIVE_TIME}s:" >> "$DIAG_TEST_DIR/inactive_threads.txt"
                ps -eLf | grep "$HYDROGEN_PID" >> "$DIAG_TEST_DIR/inactive_threads.txt" 2>/dev/null
                
                # Check if any threads are in uninterruptible sleep (D state)
                D_STATE=$(ps -eLo stat,tid | grep -w "$HYDROGEN_PID" | grep -c "D" 2>/dev/null || echo 0)
                if [ "$D_STATE" -gt 0 ]; then
                    print_warning "Detected ${D_STATE} threads in uninterruptible state" | tee -a "$RESULT_LOG"
                    ps -eLo stat,tid,wchan,cmd | grep $HYDROGEN_PID | grep "D" >> "$DIAG_TEST_DIR/D_state_threads.txt" 2>/dev/null
                fi
                
                # Reset activity timer but only once
                LAST_ACTIVITY=$(date +%s)
            fi
        fi
    fi
    
    # Small sleep to avoid consuming too much CPU in the monitoring loop
    sleep 0.2
done

# Cleanup and results
if ! ps -p $HYDROGEN_PID >/dev/null; then
    SHUTDOWN_END=$(date +%s)
    SHUTDOWN_DURATION=$((SHUTDOWN_END - SHUTDOWN_START))
    SHUTDOWN_MS=$((SHUTDOWN_DURATION * 1000 / 1000 % 1000))  # Calculate milliseconds
    printf "\n" | tee -a "$RESULT_LOG"  # Ensure we end the in-place line
    print_info "Hydrogen shut down in $SHUTDOWN_DURATION.$SHUTDOWN_MS seconds" | tee -a "$RESULT_LOG"
    
    # Analyze shutdown log for issues
    if grep -q "Shutdown complete" "$LOG_FILE"; then
        print_info "Clean shutdown detected" | tee -a "$RESULT_LOG"
    else
        print_warning "No shutdown completion message found" | tee -a "$RESULT_LOG"
    fi
    
    # Check for known shutdown issues in logs
    if grep -q "threads still active" "$LOG_FILE"; then
        print_warning "ISSUE DETECTED: Thread leak reported by Hydrogen" | tee -a "$RESULT_LOG"
        grep -A 10 "threads still active" "$LOG_FILE" | tee -a "$DIAG_TEST_DIR/thread_leak.txt"
    fi
    
    # Copy full log file for reference
    cp "$LOG_FILE" "$DIAG_TEST_DIR/full_log.txt"
    
    # Analyze shutdown timing from logs if possible
    SHUTDOWN_START_LOG=$(grep -n "Cleaning up and shutting down" "$LOG_FILE" | head -1 | cut -d: -f1)
    SHUTDOWN_END_LOG=$(grep -n "Shutdown complete" "$LOG_FILE" | head -1 | cut -d: -f1)
    
    if [ ! -z "$SHUTDOWN_START_LOG" ] && [ ! -z "$SHUTDOWN_END_LOG" ]; then
        LOG_LINES=$((SHUTDOWN_END_LOG - SHUTDOWN_START_LOG))
        print_info "Shutdown took ${LOG_LINES} log lines from signal to completion" | tee -a "$RESULT_LOG"
        
        # Extract shutdown sequence from log
        sed -n "${SHUTDOWN_START_LOG},${SHUTDOWN_END_LOG}p" "$LOG_FILE" > "$DIAG_TEST_DIR/shutdown_sequence.txt"
    fi
fi

# Generate summary report
print_header "Shutdown Test Summary" | tee -a "$RESULT_LOG"
print_info "Config: $(convert_to_relative_path "$CONFIG_FILE")" | tee -a "$RESULT_LOG"
print_info "Start time: $(date -d @$SHUTDOWN_START +"%H:%M:%S.%3N")" | tee -a "$RESULT_LOG"
print_info "End time: $(date -d @$SHUTDOWN_END +"%H:%M:%S.%3N")" | tee -a "$RESULT_LOG"
print_info "Duration: $SHUTDOWN_DURATION.$SHUTDOWN_MS seconds" | tee -a "$RESULT_LOG"
print_info "Total log lines: $LOG_LINE_COUNT" | tee -a "$RESULT_LOG"

# Print final assessment and set exit code appropriately
EXIT_CODE=0
if [ $SHUTDOWN_DURATION -lt $SHUTDOWN_TIMEOUT ]; then
    if grep -q "threads still active" "$LOG_FILE" || ! grep -q "Shutdown complete" "$LOG_FILE"; then
        print_result 0 "PASSED WITH WARNINGS (shutdown completed but with issues)" | tee -a "$RESULT_LOG"
        # Warnings don't cause a test failure
    else
        print_result 0 "PASSED (clean shutdown)" | tee -a "$RESULT_LOG"
    fi
else
    print_result 1 "FAILED (shutdown timed out after ${SHUTDOWN_TIMEOUT}s)" | tee -a "$RESULT_LOG"
    print_info "Diagnostics saved to: $(convert_to_relative_path "$DIAG_TEST_DIR/")" | tee -a "$RESULT_LOG"
    EXIT_CODE=1  # Set non-zero exit code for failure
fi

# Track subtest results
TOTAL_SUBTESTS=3
PASS_COUNT=0

# Startup success subtest
if [ "$STARTUP_COMPLETE" = "true" ]; then
    print_info "Subtest: Startup - PASSED" | tee -a "$RESULT_LOG"
    ((PASS_COUNT++))
else
    print_info "Subtest: Startup - FAILED" | tee -a "$RESULT_LOG"
fi

# Shutdown timing subtest 
if [ $SHUTDOWN_DURATION -lt $SHUTDOWN_TIMEOUT ]; then
    print_info "Subtest: Shutdown Timing - PASSED" | tee -a "$RESULT_LOG"
    ((PASS_COUNT++))
else
    print_info "Subtest: Shutdown Timing - FAILED" | tee -a "$RESULT_LOG"
fi

# Clean shutdown subtest (no thread leaks, proper completion message)
if ! grep -q "threads still active" "$LOG_FILE" && grep -q "Shutdown complete" "$LOG_FILE"; then
    print_info "Subtest: Clean Shutdown - PASSED" | tee -a "$RESULT_LOG"
    ((PASS_COUNT++))
else
    print_info "Subtest: Clean Shutdown - FAILED" | tee -a "$RESULT_LOG"
fi

# Export subtest results for test_all.sh to pick up
export_subtest_results $TOTAL_SUBTESTS $PASS_COUNT

# Log subtest results
print_info "Startup/Shutdown Test: $PASS_COUNT of $TOTAL_SUBTESTS subtests passed" | tee -a "$RESULT_LOG"

# End test
end_test $EXIT_CODE "Startup/Shutdown Test with $(convert_to_relative_path "$CONFIG_FILE")" | tee -a "$RESULT_LOG"
exit $EXIT_CODE  # Return appropriate exit code