#!/bin/bash
#
# Hydrogen Startup/Shutdown Test Script
# Tests the startup and shutdown process with enhanced diagnostics for stall detection

# Get the directory where this script is located
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
HYDROGEN_DIR="$( cd "$SCRIPT_DIR/.." && pwd )"

# Configuration
HYDROGEN_BIN="$HYDROGEN_DIR/hydrogen"  # Path to hydrogen executable
CONFIG_FILE="$1"                       # Accept config file as parameter
STARTUP_TIMEOUT=10                     # Seconds to wait for startup
SHUTDOWN_TIMEOUT=10                    # Hard limit on shutdown time
SHUTDOWN_ACTIVITY_TIMEOUT=5            # Timeout if no new log activity
LOG_FILE="$SCRIPT_DIR/hydrogen_test.log"  # Runtime log to analyze
RESULTS_DIR="$SCRIPT_DIR/results"        # Directory for test results
DIAG_DIR="$SCRIPT_DIR/diagnostics"       # Directory for diagnostic data

# Check if config file is provided
if [ -z "$CONFIG_FILE" ]; then
    echo "Usage: $0 <config_file.json>"
    echo "Example: $0 hydrogen_test_min.json"
    exit 1
fi

# Create output directories
mkdir -p "$RESULTS_DIR" "$DIAG_DIR"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
RESULT_LOG="$RESULTS_DIR/test_${TIMESTAMP}_$(basename $CONFIG_FILE .json).log"
DIAG_TEST_DIR="$DIAG_DIR/test_${TIMESTAMP}_$(basename $CONFIG_FILE .json)"
mkdir -p "$DIAG_TEST_DIR"

echo "=== HYDROGEN SHUTDOWN TEST ===" | tee -a "$RESULT_LOG"
echo "Config: $CONFIG_FILE" | tee -a "$RESULT_LOG"
echo "Timestamp: $(date)" | tee -a "$RESULT_LOG"
echo "===============================" | tee -a "$RESULT_LOG"

# Start with clean log
> "$LOG_FILE"

# Launch Hydrogen in background
echo "Starting Hydrogen with config: $CONFIG_FILE" | tee -a "$RESULT_LOG"
$HYDROGEN_BIN "$CONFIG_FILE" > "$LOG_FILE" 2>&1 &
HYDROGEN_PID=$!
echo "Started Hydrogen with PID: $HYDROGEN_PID" | tee -a "$RESULT_LOG"

# Check if process is running
if ! ps -p $HYDROGEN_PID > /dev/null; then
    echo "ERROR: Hydrogen failed to start" | tee -a "$RESULT_LOG"
    exit 1
fi

# Wait for startup
echo "Waiting $STARTUP_TIMEOUT seconds for startup..." | tee -a "$RESULT_LOG"
sleep $STARTUP_TIMEOUT

# Check if process is still running after startup
if ! ps -p $HYDROGEN_PID > /dev/null; then
    echo "ERROR: Hydrogen crashed during startup" | tee -a "$RESULT_LOG"
    cp "$LOG_FILE" "$DIAG_TEST_DIR/startup_crash.log"
    exit 1
fi

# Capture pre-shutdown diagnostics
echo "Capturing pre-shutdown state..." | tee -a "$RESULT_LOG"
ps -eLf | grep $HYDROGEN_PID > "$DIAG_TEST_DIR/pre_shutdown_threads.txt"
cat /proc/$HYDROGEN_PID/status > "$DIAG_TEST_DIR/pre_shutdown_status.txt" 2>/dev/null
ls -l /proc/$HYDROGEN_PID/fd/ > "$DIAG_TEST_DIR/pre_shutdown_fds.txt" 2>/dev/null

# Send shutdown signal
echo "Initiating shutdown at $(date +%H:%M:%S.%N)" | tee -a "$RESULT_LOG"
SHUTDOWN_START=$(date +%s)
kill -SIGINT $HYDROGEN_PID

# Monitor for log activity with timeout
LOG_SIZE_BEFORE=$(stat -c %s "$LOG_FILE" 2>/dev/null || echo 0)
LAST_ACTIVITY=$(date +%s)

echo "Monitoring shutdown progress..." | tee -a "$RESULT_LOG"

# While hydrogen is still running
while ps -p $HYDROGEN_PID >/dev/null; do
    # Check how long we've been trying to shutdown
    CURRENT_TIME=$(date +%s)
    ELAPSED=$((CURRENT_TIME - SHUTDOWN_START))
    
    # If shutdown is taking too long, collect diagnostics and force kill
    if [ $ELAPSED -ge $SHUTDOWN_TIMEOUT ]; then
        echo "CRITICAL: Shutdown timeout after ${ELAPSED}s - collecting diagnostics" | tee -a "$RESULT_LOG"
        
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
        echo "Forcing termination after ${ELAPSED}s" | tee -a "$RESULT_LOG"
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
            
            # Extract and show last line of log for monitoring
            LAST_LINE=$(tail -n 1 "$LOG_FILE")
            echo "Log activity at +${ELAPSED}s: $LAST_LINE" | tee -a "$RESULT_LOG"
        else
            # No new log activity, check how long it's been inactive
            INACTIVE_TIME=$((CURRENT_TIME - LAST_ACTIVITY))
            
            # If no activity for SHUTDOWN_ACTIVITY_TIMEOUT seconds, collect diagnostics
            if [ $INACTIVE_TIME -ge $SHUTDOWN_ACTIVITY_TIMEOUT ]; then
                echo "WARNING: No log activity for ${INACTIVE_TIME}s - collecting diagnostics" | tee -a "$RESULT_LOG"
                
                # Capture current thread state
                echo "Current threads at inactive +${INACTIVE_TIME}s:" | tee -a "$DIAG_TEST_DIR/inactive_threads.txt"
                ps -eLf | grep $HYDROGEN_PID >> "$DIAG_TEST_DIR/inactive_threads.txt" 2>/dev/null
                
                # Check if any threads are in uninterruptible sleep (D state)
                D_STATE=$(ps -eLo stat,tid | grep $HYDROGEN_PID | grep -c "D" 2>/dev/null || echo 0)
                if [ $D_STATE -gt 0 ]; then
                    echo "CRITICAL: Detected ${D_STATE} threads in uninterruptible state" | tee -a "$RESULT_LOG"
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
    echo "Hydrogen shut down in ${SHUTDOWN_DURATION} seconds" | tee -a "$RESULT_LOG"
    
    # Analyze shutdown log for issues
    if grep -q "Shutdown complete" "$LOG_FILE"; then
        echo "Clean shutdown detected" | tee -a "$RESULT_LOG"
    else
        echo "WARNING: No shutdown completion message found" | tee -a "$RESULT_LOG"
    fi
    
    # Check for known shutdown issues in logs
    if grep -q "threads still active" "$LOG_FILE"; then
        echo "ISSUE DETECTED: Thread leak reported by Hydrogen" | tee -a "$RESULT_LOG"
        grep -A 10 "threads still active" "$LOG_FILE" | tee -a "$DIAG_TEST_DIR/thread_leak.txt"
    fi
    
    # Copy full log file for reference
    cp "$LOG_FILE" "$DIAG_TEST_DIR/full_log.txt"
    
    # Analyze shutdown timing from logs if possible
    SHUTDOWN_START_LOG=$(grep -n "Cleaning up and shutting down" "$LOG_FILE" | head -1 | cut -d: -f1)
    SHUTDOWN_END_LOG=$(grep -n "Shutdown complete" "$LOG_FILE" | head -1 | cut -d: -f1)
    
    if [ ! -z "$SHUTDOWN_START_LOG" ] && [ ! -z "$SHUTDOWN_END_LOG" ]; then
        LOG_LINES=$((SHUTDOWN_END_LOG - SHUTDOWN_START_LOG))
        echo "Shutdown took ${LOG_LINES} log lines from signal to completion" | tee -a "$RESULT_LOG"
        
        # Extract shutdown sequence from log
        sed -n "${SHUTDOWN_START_LOG},${SHUTDOWN_END_LOG}p" "$LOG_FILE" > "$DIAG_TEST_DIR/shutdown_sequence.txt"
    fi
fi

# Generate summary report
echo "=== SHUTDOWN TEST SUMMARY ===" | tee -a "$RESULT_LOG"
echo "Config: $CONFIG_FILE" | tee -a "$RESULT_LOG"
echo "Start time: $(date -d @$SHUTDOWN_START)" | tee -a "$RESULT_LOG"
echo "End time: $(date -d @$SHUTDOWN_END)" | tee -a "$RESULT_LOG"
echo "Duration: ${SHUTDOWN_DURATION} seconds" | tee -a "$RESULT_LOG"

# Print final assessment
if [ $SHUTDOWN_DURATION -lt $SHUTDOWN_TIMEOUT ]; then
    if grep -q "threads still active" "$LOG_FILE" || ! grep -q "Shutdown complete" "$LOG_FILE"; then
        echo "RESULT: PASSED WITH WARNINGS (shutdown completed but with issues)" | tee -a "$RESULT_LOG"
    else
        echo "RESULT: PASSED (clean shutdown)" | tee -a "$RESULT_LOG"
    fi
else
    echo "RESULT: FAILED (shutdown timed out after ${SHUTDOWN_TIMEOUT}s)" | tee -a "$RESULT_LOG"
    echo "Diagnostics saved to: $DIAG_TEST_DIR/" | tee -a "$RESULT_LOG"
fi

echo "=== Test completed at $(date) ===" | tee -a "$RESULT_LOG"