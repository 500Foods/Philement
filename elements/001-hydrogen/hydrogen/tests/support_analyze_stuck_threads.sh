#!/bin/bash
#
# Hydrogen Thread Analyzer
# Focused script to analyze thread states and identify potential shutdown stalls
#
# Usage: ./support_analyze_stuck_threads.sh <PID>

if [ $# -lt 1 ]; then
    echo "Usage: $0 <hydrogen_pid>"
    echo "  Analyzes thread states to help diagnose shutdown stalls."
    exit 1
fi

PID=$1
OUTPUT_DIR="./diagnostics/thread_analysis_$(date +%Y%m%d_%H%M%S)"

# Check if process exists
if ! ps -p $PID > /dev/null; then
    echo "Error: Process with PID $PID not found"
    exit 1
fi

# Create output directory
mkdir -p "$OUTPUT_DIR"
echo "Analyzing thread states for PID: $PID"
echo "Saving results to: $OUTPUT_DIR"

# Basic process info
echo "=== Process Info ===" | tee "$OUTPUT_DIR/process_summary.txt"
ps -p $PID -o pid,ppid,user,stat,wchan:20,time,cmd | tee -a "$OUTPUT_DIR/process_summary.txt"

# Get list of all thread IDs
THREAD_IDS=$(ps -L -o lwp $PID | grep -v LWP)
THREAD_COUNT=$(echo "$THREAD_IDS" | wc -l)

echo "Found $THREAD_COUNT threads" | tee -a "$OUTPUT_DIR/process_summary.txt"

# Analyze each thread
for TID in $THREAD_IDS; do
    THREAD_FILE="$OUTPUT_DIR/thread_${TID}.txt"
    echo "===== Thread $TID =====" | tee "$THREAD_FILE"
    
    # Thread state
    THREAD_STATE=$(ps -o state= -p $TID 2>/dev/null || echo "?")
    echo "State: $THREAD_STATE" | tee -a "$THREAD_FILE"
    
    # Decode thread state
    case "$THREAD_STATE" in
        "R")
            echo "  Running or runnable (on run queue)" | tee -a "$THREAD_FILE"
            ;;
        "S")
            echo "  Interruptible sleep (waiting for an event to complete)" | tee -a "$THREAD_FILE"
            ;;
        "D")
            echo "  Uninterruptible sleep (usually IO) - POTENTIAL STALL CAUSE" | tee -a "$THREAD_FILE"
            ;;
        "Z")
            echo "  Zombie (terminated but not reaped by parent)" | tee -a "$THREAD_FILE"
            ;;
        "T")
            echo "  Stopped (either by a job control signal or because it is being traced)" | tee -a "$THREAD_FILE"
            ;;
        *)
            echo "  Unknown state" | tee -a "$THREAD_FILE"
            ;;
    esac
    
    # WCHAN - kernel function where thread is sleeping
    WCHAN=$(ps -o wchan= -p $TID 2>/dev/null || echo "?")
    echo "Wait channel: $WCHAN" | tee -a "$THREAD_FILE"
    
    # Thread name 
    THREAD_NAME=$(ps -o comm= -p $TID 2>/dev/null || echo "?")
    echo "Thread name: $THREAD_NAME" | tee -a "$THREAD_FILE"
    
    # If in syscall, show which one
    if [ -f /proc/$PID/task/$TID/syscall ]; then
        SYSCALL=$(cat /proc/$PID/task/$TID/syscall 2>/dev/null)
        if [ ! -z "$SYSCALL" ] && [ "$SYSCALL" != "0 0 0 0 0 0 0" ]; then
            echo "Syscall: $SYSCALL" | tee -a "$THREAD_FILE"
        fi
    fi
    
    # Thread CPU time and memory
    ps -L -o pid,tid,pcpu,min_flt,maj_flt -p $PID | grep $TID | tee -a "$THREAD_FILE"
    
    # Stack (if available)
    if [ -f /proc/$PID/task/$TID/stack ]; then
        echo "Kernel stack:" | tee -a "$THREAD_FILE"
        cat /proc/$PID/task/$TID/stack 2>/dev/null | tee -a "$THREAD_FILE"
    fi
    
    echo "" | tee -a "$THREAD_FILE"
done

# Generate summary of thread states
echo "=== Thread State Summary ===" | tee "$OUTPUT_DIR/thread_state_summary.txt"
echo "Running (R): $(ps -L -o stat= -p $PID | grep -c "R")" | tee -a "$OUTPUT_DIR/thread_state_summary.txt"
echo "Interruptible sleep (S): $(ps -L -o stat= -p $PID | grep -c "S")" | tee -a "$OUTPUT_DIR/thread_state_summary.txt"
echo "Uninterruptible sleep (D): $(ps -L -o stat= -p $PID | grep -c "D")" | tee -a "$OUTPUT_DIR/thread_state_summary.txt"
echo "Stopped (T): $(ps -L -o stat= -p $PID | grep -c "T")" | tee -a "$OUTPUT_DIR/thread_state_summary.txt"
echo "Zombie (Z): $(ps -L -o stat= -p $PID | grep -c "Z")" | tee -a "$OUTPUT_DIR/thread_state_summary.txt"

# Look for potential issues
D_THREADS=$(ps -L -o tid,stat,wchan -p $PID | grep "D")
if [ ! -z "$D_THREADS" ]; then
    echo "CRITICAL: Found threads in uninterruptible sleep (D state) - potential stall cause:" | tee "$OUTPUT_DIR/critical_issues.txt"
    echo "$D_THREADS" | tee -a "$OUTPUT_DIR/critical_issues.txt"
fi

echo "Thread analysis complete. Results saved to $OUTPUT_DIR"