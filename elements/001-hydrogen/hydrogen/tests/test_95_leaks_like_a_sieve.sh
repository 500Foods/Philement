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

# Process leak report and extract grouped leaks
extract_leak_info() {
    # Arguments: leak_type, log_file, output_file
    local type="$1"
    local file="$2"
    local output="$3"
    
    # Start with section header
    # echo "${type} Leak Details:" > "$output"
    echo "" >> "$output"
    
    # Count total leaks of this type
    local leak_count=$(grep -c "${type} leak of" "$file")
    
    # If no leaks, return early
    if [ $leak_count -eq 0 ]; then
        echo "$leak_count:0:0:0"
        return
    fi
    
    # Create temporary files
    local tmpfile=$(mktemp)
    local leak_data=$(mktemp)
    
    # Create tracking data for grouping
    declare -A leak_groups
    declare -A leak_sizes
    
    # Extract all leak blocks
    grep -n "${type} leak of" "$file" | 
    while IFS=: read -r line_num rest; do
        # Extract the leak size
        if [[ $rest =~ ${type}\ leak\ of\ ([0-9,]+)\ byte ]]; then
            local leak_size=${BASH_REMATCH[1]//,/}
            
            # Find first meaningful stack frame
            found_component=false
            
            # Get the 10 lines following this leak header
            head -n $((line_num + 10)) "$file" | tail -n 10 > "$tmpfile"
            
            # Process stack trace lines
            while IFS= read -r stack_line; do
                # Skip stack frames that are memory allocation functions
                if [[ $stack_line =~ malloc ]] || [[ $stack_line =~ calloc ]] || [[ $stack_line =~ realloc ]] || [[ $stack_line =~ free ]]; then
                    continue
                fi
                
                # Try to extract component information
                # Pattern 1: Function and file path in one section
                if [[ $stack_line =~ \#[0-9]+\ 0x[0-9a-f]+\ in\ ([^ ]+)\ ([^:]+):([0-9]+) ]]; then
                    func="${BASH_REMATCH[1]}"
                    file_path="${BASH_REMATCH[2]}"
                    line="${BASH_REMATCH[3]}"
                    
                    # Save this component info
                    echo "$file_path:$line in $func|$leak_size" >> "$leak_data"
                    found_component=true
                    break
                # Pattern 2: Function with parentheses
                elif [[ $stack_line =~ \#[0-9]+\ 0x[0-9a-f]+\ in\ ([^ ]+)\ \(([^:]+):([0-9]+)\) ]]; then
                    func="${BASH_REMATCH[1]}"
                    file_path="${BASH_REMATCH[2]}"
                    line="${BASH_REMATCH[3]}"
                    
                    # Save this component info
                    echo "$file_path:$line in $func|$leak_size" >> "$leak_data"
                    found_component=true
                    break
                fi
            done < "$tmpfile"
            
            # If no component found, use a placeholder
            if ! $found_component; then
                echo "unknown location|$leak_size" >> "$leak_data"
            fi
        fi
    done
    
    # Group by component and size, then sort by size (largest first)
    if [ -s "$leak_data" ]; then
        sort "$leak_data" | uniq -c | sort -k3,3nr | 
        while read -r count line; do
            component=$(echo "$line" | cut -d'|' -f1)
            size=$(echo "$line" | cut -d'|' -f2)
            echo "  $count $component ${type,,} leak of $size bytes" >> "$output"
        done
    fi
    
    # Gather statistics for summary
    local sizes=$(grep "${type} leak of" "$file" | sed -E "s/.*leak of ([0-9,]+) byte.*/\1/g" | tr -d ',')
    local total=0
    local smallest=999999
    local largest=0
    
    for size in $sizes; do
        # Add to total
        total=$((total + size))
        
        # Update smallest/largest
        if [ $size -lt $smallest ]; then
            smallest=$size
        fi
        if [ $size -gt $largest ]; then
            largest=$size
        fi
    done
    
    # Clean up temp files
    rm -f "$tmpfile" "$leak_data"
    
    echo "" >> "$output"
    
    # Return summary info for use in summary section
    echo "$leak_count:$smallest:$largest:$total"
}

# Function to check for direct memory leaks
check_direct_leaks() {
    local log_file="$1"
    local summary_file="$2"
    local details_file="$3"
    
    # Process direct leaks
    echo "Direct:"
    > "$details_file"
    local direct_summary=$(extract_leak_info "Direct" "$log_file" "$details_file")
    
    # Read summary info
    direct_count=$(echo "$direct_summary" | cut -d: -f1)
    direct_smallest=$(echo "$direct_summary" | cut -d: -f2)
    direct_largest=$(echo "$direct_summary" | cut -d: -f3)
    direct_total=$(echo "$direct_summary" | cut -d: -f4)
    
    # Write summary section
    {
        echo "Direct Leaks"
        echo "- Found: $direct_count"
        if [ $direct_count -gt 0 ]; then
            echo "- Smallest: $direct_smallest byte(s)"
            echo "- Largest: $direct_largest byte(s)"
            echo "- Total: $direct_total byte(s)"
        fi
        echo
    } > "$summary_file"
    
    # Return success only if no leaks found
    return $((direct_count > 0))
}


# Function to check for indirect memory leaks
check_indirect_leaks() {
    local log_file="$1"
    local summary_file="$2"
    local details_file="$3"
    
    # Process indirect leaks
    echo "Indirect:"
    > "$details_file"
    local indirect_summary=$(extract_leak_info "Indirect" "$log_file" "$details_file")
    
    # Read summary info
    indirect_count=$(echo "$indirect_summary" | cut -d: -f1)
    indirect_smallest=$(echo "$indirect_summary" | cut -d: -f2)
    indirect_largest=$(echo "$indirect_summary" | cut -d: -f3)
    indirect_total=$(echo "$indirect_summary" | cut -d: -f4)
    
    # Write summary section
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
    local details_file="$log_dir/leak_details.txt"
    
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
    
    # Clear previous results
    > "$details_file"
    > "$summary_file"
    
    # Check for direct leaks (subtest 1)
    print_header "Checking for direct memory leaks..."
    check_direct_leaks "${log_file}.${HYDROGEN_PID}" "$summary_file" "$details_file"
    DIRECT_RESULT=$?
    
    # Display result for direct leaks
    if [ $DIRECT_RESULT -eq 0 ]; then
        print_result 0 "No direct memory leaks found"
    else
        # Show leak details before result
        if [ -f "$details_file" ]; then
            cat "$details_file"
        fi
        print_result 1 "Direct memory leaks detected"
    fi
    
    # Check for indirect leaks (subtest 2)
    print_header "Checking for indirect memory leaks..."
    check_indirect_leaks "${log_file}.${HYDROGEN_PID}" "$summary_file" "$details_file"
    INDIRECT_RESULT=$?
    
    # Display result for indirect leaks
    if [ $INDIRECT_RESULT -eq 0 ]; then
        print_result 0 "No indirect memory leaks found"
    else
        # Show leak details before result
        if [ -f "$details_file" ]; then
            cat "$details_file"
        fi
        print_result 1 "Indirect memory leaks detected"
    fi
    
    # Display final summary
    print_header "Memory Leak Summary"
    cat "$summary_file"
    
    # Copy logs to results directory
    mkdir -p "$SCRIPT_DIR/results"
    cp "${log_file}.${HYDROGEN_PID}" "$SCRIPT_DIR/results/leak_report_${TIMESTAMP}.txt"
    cp "$summary_file" "$SCRIPT_DIR/results/leak_summary_${TIMESTAMP}.txt"
    cp "$details_file" "$SCRIPT_DIR/results/leak_details_${TIMESTAMP}.txt"
    
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

# Calculate total passed subtests
TOTAL_SUBTESTS=2  # Direct and indirect leak checks
PASSED_SUBTESTS=0
[ $DIRECT_RESULT -eq 0 ] && ((PASSED_SUBTESTS++))
[ $INDIRECT_RESULT -eq 0 ] && ((PASSED_SUBTESTS++))

# Get test name from script name
TEST_NAME=$(basename "$0" .sh | sed 's/^test_//')

# Export subtest results for test_all.sh
export_subtest_results "$TEST_NAME" $TOTAL_SUBTESTS $PASSED_SUBTESTS

# End the test
end_test $TEST_RESULT "Memory Leak Test"

exit $TEST_RESULT