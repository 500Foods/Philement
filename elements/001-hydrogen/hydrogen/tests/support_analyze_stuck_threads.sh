#!/bin/bash
#
# Hydrogen Thread Analyzer
# Focused script to analyze thread states and identify potential shutdown stalls
#
# Usage: ./support_analyze_stuck_threads.sh <PID>

# About this Script
NAME_SCRIPT="Hydrogen Thread Analyzer"
VERS_SCRIPT="2.0.0"

# VERSION HISTORY
# 2.0.0 - 2025-06-17 - Major refactoring: fixed all shellcheck warnings, improved modularity, enhanced comments
# 1.0.1 - 2025-06-16 - Added thread state analysis and kernel stack inspection
# 1.0.0 - 2025-06-15 - Initial version with basic thread analysis

# Script configuration
SCRIPT_NAME="$(basename "$0")"
readonly SCRIPT_NAME

# Validate command line arguments
validate_arguments() {
    if [ $# -lt 1 ]; then
        echo "Usage: $SCRIPT_NAME <hydrogen_pid>"
        echo "  Analyzes thread states to help diagnose shutdown stalls."
        echo ""
        echo "  $NAME_SCRIPT v$VERS_SCRIPT"
        exit 1
    fi
}

# Validate that the process exists
validate_process() {
    local pid="$1"
    
    if ! ps -p "$pid" > /dev/null 2>&1; then
        echo "Error: Process with PID $pid not found"
        exit 1
    fi
}

# Create output directory for analysis results
setup_output_directory() {
    local pid="$1"
    local timestamp
    timestamp=$(date +%Y%m%d_%H%M%S)
    
    OUTPUT_DIR="./diagnostics/thread_analysis_${timestamp}_pid${pid}"
    mkdir -p "$OUTPUT_DIR"
    
    echo "Analyzing thread states for PID: $pid"
    echo "Saving results to: $OUTPUT_DIR"
    echo ""
}

# Gather basic process information
analyze_process_info() {
    local pid="$1"
    local summary_file="$OUTPUT_DIR/process_summary.txt"
    
    echo "=== Process Info ===" | tee "$summary_file"
    ps -p "$pid" -o pid,ppid,user,stat,wchan:20,time,cmd | tee -a "$summary_file"
    echo "" | tee -a "$summary_file"
}

# Get list of thread IDs for the process
get_thread_list() {
    local pid="$1"
    
    # Use pgrep-like approach instead of grepping ps output (fixes SC2009)
    ps -L -o lwp= -p "$pid" 2>/dev/null | grep -v "^[[:space:]]*$" || {
        echo "Error: Unable to get thread list for PID $pid"
        exit 1
    }
}

# Decode thread state into human-readable description
decode_thread_state() {
    local state="$1"
    
    case "$state" in
        "R")
            echo "Running or runnable (on run queue)"
            ;;
        "S")
            echo "Interruptible sleep (waiting for an event to complete)"
            ;;
        "D")
            echo "Uninterruptible sleep (usually IO) - POTENTIAL STALL CAUSE"
            ;;
        "Z")
            echo "Zombie (terminated but not reaped by parent)"
            ;;
        "T")
            echo "Stopped (either by a job control signal or because it is being traced)"
            ;;
        *)
            echo "Unknown state ($state)"
            ;;
    esac
}

# Analyze syscall information for a thread
analyze_thread_syscall() {
    local pid="$1"
    local tid="$2"
    local thread_file="$3"
    
    if [ -f "/proc/$pid/task/$tid/syscall" ]; then
        local syscall
        syscall=$(cat "/proc/$pid/task/$tid/syscall" 2>/dev/null)
        if [ -n "$syscall" ] && [ "$syscall" != "0 0 0 0 0 0 0" ]; then
            echo "Syscall: $syscall" | tee -a "$thread_file"
        fi
    fi
}

# Analyze kernel stack for a thread
analyze_thread_stack() {
    local pid="$1"
    local tid="$2"
    local thread_file="$3"
    
    if [ -f "/proc/$pid/task/$tid/stack" ]; then
        echo "Kernel stack:" | tee -a "$thread_file"
        tee -a "$thread_file" < "/proc/$pid/task/$tid/stack" 2>/dev/null
    fi
}

# Analyze a single thread in detail
analyze_single_thread() {
    local pid="$1"
    local tid="$2"
    local thread_file="$OUTPUT_DIR/thread_${tid}.txt"
    
    echo "===== Thread $tid =====" | tee "$thread_file"
    
    # Thread state
    local thread_state
    thread_state=$(ps -o state= -p "$tid" 2>/dev/null || echo "?")
    echo "State: $thread_state" | tee -a "$thread_file"
    
    # Decode thread state
    local state_description
    state_description=$(decode_thread_state "$thread_state")
    echo "  $state_description" | tee -a "$thread_file"
    
    # WCHAN - kernel function where thread is sleeping
    local wchan
    wchan=$(ps -o wchan= -p "$tid" 2>/dev/null || echo "?")
    echo "Wait channel: $wchan" | tee -a "$thread_file"
    
    # Thread name 
    local thread_name
    thread_name=$(ps -o comm= -p "$tid" 2>/dev/null || echo "?")
    echo "Thread name: $thread_name" | tee -a "$thread_file"
    
    # Syscall analysis
    analyze_thread_syscall "$pid" "$tid" "$thread_file"
    
    # Thread CPU time and memory
    ps -L -o pid,tid,pcpu,min_flt,maj_flt -p "$pid" | grep "$tid" | tee -a "$thread_file"
    
    # Kernel stack analysis
    analyze_thread_stack "$pid" "$tid" "$thread_file"
    
    echo "" | tee -a "$thread_file"
}

# Analyze all threads for the process
analyze_all_threads() {
    local pid="$1"
    local thread_ids
    local thread_count
    
    thread_ids=$(get_thread_list "$pid")
    thread_count=$(echo "$thread_ids" | wc -l)
    
    echo "Found $thread_count threads" | tee -a "$OUTPUT_DIR/process_summary.txt"
    echo ""
    
    # Analyze each thread individually
    local tid
    while read -r tid; do
        [ -n "$tid" ] && analyze_single_thread "$pid" "$tid"
    done <<< "$thread_ids"
}

# Generate summary statistics of thread states
generate_thread_state_summary() {
    local pid="$1"
    local summary_file="$OUTPUT_DIR/thread_state_summary.txt"
    
    echo "=== Thread State Summary ===" | tee "$summary_file"
    echo "Running (R): $(ps -L -o stat= -p "$pid" | grep -c "R")" | tee -a "$summary_file"
    echo "Interruptible sleep (S): $(ps -L -o stat= -p "$pid" | grep -c "S")" | tee -a "$summary_file"
    echo "Uninterruptible sleep (D): $(ps -L -o stat= -p "$pid" | grep -c "D")" | tee -a "$summary_file"
    echo "Stopped (T): $(ps -L -o stat= -p "$pid" | grep -c "T")" | tee -a "$summary_file"
    echo "Zombie (Z): $(ps -L -o stat= -p "$pid" | grep -c "Z")" | tee -a "$summary_file"
    echo ""
}

# Identify critical issues that could cause stalls
identify_critical_issues() {
    local pid="$1"
    local critical_file="$OUTPUT_DIR/critical_issues.txt"
    local d_threads
    
    # Look for threads in uninterruptible sleep (D state)
    d_threads=$(ps -L -o tid,stat,wchan -p "$pid" | grep "D")
    if [ -n "$d_threads" ]; then
        echo "CRITICAL: Found threads in uninterruptible sleep (D state) - potential stall cause:" | tee "$critical_file"
        echo "$d_threads" | tee -a "$critical_file"
        echo ""
        return 1
    fi
    
    return 0
}

# Generate analysis report summary
generate_analysis_report() {
    local pid="$1"
    local report_file="$OUTPUT_DIR/analysis_report.txt"
    
    {
        echo "=== Thread Analysis Report ==="
        echo "Generated by: $NAME_SCRIPT v$VERS_SCRIPT"
        echo "Timestamp: $(date)"
        echo "Process PID: $pid"
        echo ""
        echo "Files generated:"
        echo "  - process_summary.txt: Basic process information"
        echo "  - thread_state_summary.txt: Thread state statistics"
        echo "  - thread_*.txt: Individual thread analysis"
        if [ -f "$OUTPUT_DIR/critical_issues.txt" ]; then
            echo "  - critical_issues.txt: Critical issues found"
        fi
        echo ""
        echo "Analysis complete."
    } | tee "$report_file"
}

# Main execution function
main() {
    local pid="$1"
    
    echo "$NAME_SCRIPT v$VERS_SCRIPT"
    echo "Analyzing process threads for potential stalls..."
    echo ""
    
    # Validate inputs
    validate_process "$pid"
    
    # Set up analysis environment
    setup_output_directory "$pid"
    
    # Perform analysis
    analyze_process_info "$pid"
    analyze_all_threads "$pid"
    generate_thread_state_summary "$pid"
    
    # Check for critical issues
    if identify_critical_issues "$pid"; then
        echo "No critical thread issues detected."
    else
        echo "WARNING: Critical thread issues detected! Check critical_issues.txt"
    fi
    
    # Generate final report
    generate_analysis_report "$pid"
    
    echo "Thread analysis complete. Results saved to $OUTPUT_DIR"
}

# Script entry point
validate_arguments "$@"
main "$1"
