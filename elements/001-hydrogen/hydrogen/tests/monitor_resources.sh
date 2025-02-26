#!/bin/bash
#
# Hydrogen Resource Monitor
# Tracks system resources for a running process to help diagnose resource issues
#
# Usage: ./monitor_resources.sh <PID> [duration_seconds]

if [ $# -lt 1 ]; then
    echo "Usage: $0 <hydrogen_pid> [duration_seconds]"
    echo "  Monitors resource usage of a process until it exits or duration expires."
    echo "  Default duration is 60 seconds if not specified."
    exit 1
fi

PID=$1
DURATION=${2:-60}  # Default 60 seconds if not specified
OUTPUT_DIR="./diagnostics/resources_$(date +%Y%m%d_%H%M%S)"

# Check if process exists
if ! ps -p $PID > /dev/null; then
    echo "Error: Process with PID $PID not found"
    exit 1
fi

# Create output directory
mkdir -p "$OUTPUT_DIR"
echo "Monitoring resources for PID: $PID for up to $DURATION seconds"
echo "Saving results to: $OUTPUT_DIR"

# Get process name for reference
PROCESS_NAME=$(ps -p $PID -o comm= 2>/dev/null || echo "unknown")
echo "Process name: $PROCESS_NAME" | tee "$OUTPUT_DIR/info.txt"

# Create CSV header
echo "Timestamp,Elapsed,MemKB,MemPct,CPU,ThreadCount,FDCount" > "$OUTPUT_DIR/resource_usage.csv"

# Monitor for specified duration or until process exits
START_TIME=$(date +%s)
END_TIME=$((START_TIME + DURATION))
CURRENT_TIME=$START_TIME

echo "Starting resource monitoring at $(date)"

while [ $CURRENT_TIME -lt $END_TIME ] && ps -p $PID >/dev/null; do
    TIMESTAMP=$(date +%Y-%m-%d_%H:%M:%S)
    ELAPSED=$((CURRENT_TIME - START_TIME))
    
    # Memory usage
    MEM_USAGE=$(ps -o rss= -p $PID 2>/dev/null || echo "0")
    
    # Memory percentage
    TOTAL_MEM=$(grep MemTotal /proc/meminfo | awk '{print $2}')
    if [ -n "$TOTAL_MEM" ] && [ "$TOTAL_MEM" -gt 0 ]; then
        MEM_PCT=$(echo "scale=2; $MEM_USAGE / $TOTAL_MEM * 100" | bc 2>/dev/null || echo "0")
    else
        MEM_PCT="0"
    fi
    
    # CPU usage
    CPU_USAGE=$(ps -p $PID -o %cpu= 2>/dev/null | tr -d ' ' || echo "0")
    
    # Thread count
    THREAD_COUNT=$(ps -L -p $PID 2>/dev/null | wc -l || echo "0")
    THREAD_COUNT=$((THREAD_COUNT - 1))  # Subtract header line
    
    # FD count
    FD_COUNT=$(ls -l /proc/$PID/fd/ 2>/dev/null | wc -l || echo "0")
    
    # Record data
    echo "$TIMESTAMP,$ELAPSED,$MEM_USAGE,$MEM_PCT,$CPU_USAGE,$THREAD_COUNT,$FD_COUNT" >> "$OUTPUT_DIR/resource_usage.csv"
    
    # Take periodic snapshots of detailed information
    if [ $((ELAPSED % 10)) -eq 0 ]; then
        SNAPSHOT_DIR="$OUTPUT_DIR/snapshot_${ELAPSED}s"
        mkdir -p "$SNAPSHOT_DIR"
        
        echo "Taking detailed snapshot at +${ELAPSED}s"
        
        # Thread information
        ps -L -o pid,tid,pcpu,state,nlwp,wchan:32 -p $PID > "$SNAPSHOT_DIR/threads.txt" 2>/dev/null
        
        # Memory maps
        cat /proc/$PID/maps > "$SNAPSHOT_DIR/memory_maps.txt" 2>/dev/null
        
        # File descriptors
        ls -l /proc/$PID/fd/ > "$SNAPSHOT_DIR/open_files.txt" 2>/dev/null
        
        # Status info
        cat /proc/$PID/status > "$SNAPSHOT_DIR/proc_status.txt" 2>/dev/null
        
        # IO stats
        cat /proc/$PID/io > "$SNAPSHOT_DIR/io_stats.txt" 2>/dev/null
    fi
    
    # Sleep for 1 second between measurements
    sleep 1
    CURRENT_TIME=$(date +%s)
done

# Record final state
if ps -p $PID >/dev/null; then
    echo "Monitoring completed (duration limit reached)" | tee -a "$OUTPUT_DIR/info.txt"
    echo "Process is still running" | tee -a "$OUTPUT_DIR/info.txt"
    
    # Take a final snapshot
    FINAL_DIR="$OUTPUT_DIR/snapshot_final"
    mkdir -p "$FINAL_DIR"
    ps -L -o pid,tid,pcpu,state,nlwp,wchan:32 -p $PID > "$FINAL_DIR/threads.txt" 2>/dev/null
    ls -l /proc/$PID/fd/ > "$FINAL_DIR/open_files.txt" 2>/dev/null
else
    FINAL_TIME=$(date +%s)
    TOTAL_RUNTIME=$((FINAL_TIME - START_TIME))
    echo "Process exited after $TOTAL_RUNTIME seconds" | tee -a "$OUTPUT_DIR/info.txt"
fi

echo "Resource monitoring completed at $(date)"
echo "Results saved to: $OUTPUT_DIR"

# Generate simple statistics from the data
if [ -f "$OUTPUT_DIR/resource_usage.csv" ]; then
    echo "Generating statistics..."
    
    # Skip header line for calculations
    LINES=$(wc -l < "$OUTPUT_DIR/resource_usage.csv")
    if [ $LINES -gt 1 ]; then
        echo "=== Resource Usage Statistics ===" > "$OUTPUT_DIR/statistics.txt"
        
        # Memory stats (using awk to get min/max/avg)
        echo "Memory Usage (KB):" >> "$OUTPUT_DIR/statistics.txt"
        tail -n +2 "$OUTPUT_DIR/resource_usage.csv" | awk -F, '{sum+=$3; if(NR==1){min=max=$3} else {if($3<min){min=$3}; if($3>max){max=$3}}} END {printf "  Min: %d, Max: %d, Avg: %.1f\n", min, max, sum/NR}' >> "$OUTPUT_DIR/statistics.txt"
        
        # CPU stats
        echo "CPU Usage (%):" >> "$OUTPUT_DIR/statistics.txt"
        tail -n +2 "$OUTPUT_DIR/resource_usage.csv" | awk -F, '{sum+=$5; if(NR==1){min=max=$5} else {if($5<min){min=$5}; if($5>max){max=$5}}} END {printf "  Min: %.1f, Max: %.1f, Avg: %.1f\n", min, max, sum/NR}' >> "$OUTPUT_DIR/statistics.txt"
        
        # Thread count stats
        echo "Thread Count:" >> "$OUTPUT_DIR/statistics.txt"
        tail -n +2 "$OUTPUT_DIR/resource_usage.csv" | awk -F, '{sum+=$6; if(NR==1){min=max=$6} else {if($6<min){min=$6}; if($6>max){max=$6}}} END {printf "  Min: %d, Max: %d, Avg: %.1f\n", min, max, sum/NR}' >> "$OUTPUT_DIR/statistics.txt"
        
        # FD count stats
        echo "File Descriptor Count:" >> "$OUTPUT_DIR/statistics.txt"
        tail -n +2 "$OUTPUT_DIR/resource_usage.csv" | awk -F, '{sum+=$7; if(NR==1){min=max=$7} else {if($7<min){min=$7}; if($7>max){max=$7}}} END {printf "  Min: %d, Max: %d, Avg: %.1f\n", min, max, sum/NR}' >> "$OUTPUT_DIR/statistics.txt"
        
        cat "$OUTPUT_DIR/statistics.txt"
    fi
fi

exit 0