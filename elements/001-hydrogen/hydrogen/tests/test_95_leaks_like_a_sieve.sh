#!/bin/bash
#
# Memory Leak Detection Test
# Tests for memory leaks using ASAN in debug build
# - Requires debug build with ASAN
# - Triggers clean shutdown with SIGINT
# - Analyzes and summarizes memory leaks
# - Fails if any leaks are found
#

# Get the directory where this script is located
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
HYDROGEN_DIR="$( cd "$SCRIPT_DIR/.." && pwd )"
source "$SCRIPT_DIR/support_utils.sh"

# Configuration
CONFIG_FILE=$(get_config_path "hydrogen_test_min.json")
if [ ! -f "$CONFIG_FILE" ]; then
    print_result 1 "Configuration file not found: $CONFIG_FILE"
    exit 1
fi

# Function to check for direct memory leaks
check_direct_leaks() {
    local log_file="$1"
    local summary_file="$2"
    local direct_count=0
    local direct_smallest=0
    local direct_largest=0
    local direct_total=0
    
    # Process direct leaks
    while IFS= read -r line; do
        if [[ $line =~ Direct\ leak\ of\ ([0-9,]+)\ byte\(s\) ]]; then
            size="${BASH_REMATCH[1]//,/}"
            ((direct_count++))
            ((direct_total+=size))
            
            # Update smallest/largest
            if [ $direct_smallest -eq 0 ] || [ $size -lt $direct_smallest ]; then
                direct_smallest=$size
            fi
            if [ $size -gt $direct_largest ]; then
                direct_largest=$size
            fi
        fi
    done < "$log_file"
    
    # Write direct leak summary
    {
        echo "Direct Leaks"
        echo "- Found: $direct_count"
        if [ $direct_count -gt 0 ]; then
            echo "- Smallest: $direct_smallest byte(s)"
            echo "- Largest: $direct_largest byte(s)"
            echo "- Total: $direct_total byte(s)"
        fi
        echo
    } >> "$summary_file"
    
    # Return success only if no leaks found
    return $((direct_count > 0))
}

# Function to check for indirect memory leaks
check_indirect_leaks() {
    local log_file="$1"
    local summary_file="$2"
    local indirect_count=0
    local indirect_smallest=0
    local indirect_largest=0
    local indirect_total=0
    
    # Process indirect leaks
    while IFS= read -r line; do
        if [[ $line =~ Indirect\ leak\ of\ ([0-9,]+)\ byte\(s\) ]]; then
            size="${BASH_REMATCH[1]//,/}"
            ((indirect_count++))
            ((indirect_total+=size))
            
            # Update smallest/largest
            if [ $indirect_smallest -eq 0 ] || [ $size -lt $indirect_smallest ]; then
                indirect_smallest=$size
            fi
            if [ $size -gt $indirect_largest ]; then
                indirect_largest=$size
            fi
        fi
    done < "$log_file"
    
    # Write indirect leak summary
    {
        echo "Indirect Leaks"
        echo "- Found: $indirect_count"
        if [ $indirect_count -gt 0 ]; then
            echo "- Smallest: $indirect_smallest byte(s)"
            echo "- Largest: $indirect_largest byte(s)"
            echo "- Total: $indirect_total byte(s)"
        fi
    } >> "$summary_file"
    
    # Return success only if no leaks found
    return $((indirect_count > 0))
}

# Function to run leak test with debug build
run_leak_test() {
    local binary="$1"
    local binary_name=$(basename "$binary")
    
    print_info "Starting leak test with $binary_name..."
    
    # Create log directory if it doesn't exist
    local log_dir="$SCRIPT_DIR/logs"
    mkdir -p "$log_dir"
    local log_file="$log_dir/hydrogen_leak_test.log"
    local summary_file="$log_dir/leak_summary.txt"
    
    # Start hydrogen server with ASAN options
    ASAN_OPTIONS="detect_leaks=1:log_path=$log_file" $binary "$CONFIG_FILE" > "$log_dir/server.log" 2>&1 &
    HYDROGEN_PID=$!
    
    # Wait for startup
    print_info "Waiting for startup (max 10s)..."
    STARTUP_START=$(date +%s)
    STARTUP_TIMEOUT=10
    STARTUP_COMPLETE=false
    
    while true; do
        CURRENT_TIME=$(date +%s)
        ELAPSED=$((CURRENT_TIME - STARTUP_START))
        
        if [ $ELAPSED -ge $STARTUP_TIMEOUT ]; then
            print_result 1 "Startup timeout after ${ELAPSED}s"
            kill -9 $HYDROGEN_PID 2>/dev/null
            return 1
        fi
        
        if ! kill -0 $HYDROGEN_PID 2>/dev/null; then
            print_result 1 "Server crashed during startup"
            return 1
        fi
        
        if grep -q "Application started" "$log_dir/server.log"; then
            STARTUP_COMPLETE=true
            print_info "Startup completed in ${ELAPSED}s"
            break
        fi
        
        sleep 0.2
    done
    
    # Send SIGINT to trigger clean shutdown
    print_info "Sending SIGINT to trigger clean shutdown..."
    kill -INT $HYDROGEN_PID
    
    # Wait for process to exit
    wait $HYDROGEN_PID
    EXIT_CODE=$?
    
    # Check if ASAN generated a leak report
    if [ ! -f "${log_file}.${HYDROGEN_PID}" ]; then
        print_result 1 "No ASAN leak report found"
        return 1
    fi
    
    # Check for direct leaks (subtest 1)
    print_header "Checking for direct memory leaks..."
    check_direct_leaks "${log_file}.${HYDROGEN_PID}" "$summary_file"
    DIRECT_RESULT=$?
    
    if [ $DIRECT_RESULT -eq 0 ]; then
        print_result 0 "No direct memory leaks found"
    else
        print_result 1 "Direct memory leaks detected"
    fi
    
    # Check for indirect leaks (subtest 2)
    print_header "Checking for indirect memory leaks..."
    check_indirect_leaks "${log_file}.${HYDROGEN_PID}" "$summary_file"
    INDIRECT_RESULT=$?
    
    if [ $INDIRECT_RESULT -eq 0 ]; then
        print_result 0 "No indirect memory leaks found"
    else
        print_result 1 "Indirect memory leaks detected"
    fi
    
    # Display full leak summary
    print_header "Memory Leak Summary"
    cat "$summary_file"
    
    # Copy logs to results directory
    mkdir -p "$SCRIPT_DIR/results"
    cp "${log_file}.${HYDROGEN_PID}" "$SCRIPT_DIR/results/leak_report_${TIMESTAMP}.txt"
    cp "$summary_file" "$SCRIPT_DIR/results/leak_summary_${TIMESTAMP}.txt"
    
    # Test passes only if no leaks found
    if [ $DIRECT_RESULT -eq 0 ] && [ $INDIRECT_RESULT -eq 0 ]; then
        return 0
    else
        return 1
    fi
}

# Start the test
start_test "Memory Leak Detection Test"

# Store timestamp for this test run
TIMESTAMP=$(date +%Y%m%d_%H%M%S)

# Look for debug build
DEBUG_BUILD="$HYDROGEN_DIR/hydrogen_debug"
if [ ! -f "$DEBUG_BUILD" ]; then
    print_result 1 "Debug build not found. This test requires the debug build with ASAN."
    end_test 1 "Memory Leak Test"
    exit 1
fi

# Verify ASAN is enabled in debug build
if ! readelf -s "$DEBUG_BUILD" | grep -q "__asan"; then
    print_result 1 "Debug build does not have ASAN enabled"
    end_test 1 "Memory Leak Test"
    exit 1
fi

# Run the leak test
run_leak_test "$DEBUG_BUILD"
TEST_RESULT=$?

# End the test
end_test $TEST_RESULT "Memory Leak Test"

exit $TEST_RESULT