#!/bin/bash
#
# Hydrogen Test Runner
# Executes all tests with standardized formatting and generates a summary report
#
# Usage: ./run_tests.sh [min|max|all]

# Get the directory where this script is located
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

# Include the common test utilities
source "$SCRIPT_DIR/test_utils.sh"

# Clean up previous test results and diagnostics
cleanup_old_tests() {
    print_info "Cleaning up previous test results and diagnostics..."
    
    # Define directories to clean
    RESULTS_DIR="$SCRIPT_DIR/results"
    DIAGNOSTICS_DIR="$SCRIPT_DIR/diagnostics"
    
    # Remove all files in results directory
    if [ -d "$RESULTS_DIR" ]; then
        rm -rf "$RESULTS_DIR"/*
        print_info "Removed old test results from $RESULTS_DIR"
    fi
    
    # Remove all files in diagnostics directory
    if [ -d "$DIAGNOSTICS_DIR" ]; then
        rm -rf "$DIAGNOSTICS_DIR"/*
        print_info "Removed old test diagnostics from $DIAGNOSTICS_DIR"
    fi
    
    print_info "Cleanup complete"
}

# Function to ensure the Hydrogen server isn't running
ensure_server_not_running() {
    print_info "Checking for running Hydrogen instances..."
    
    # Check if any Hydrogen processes are running
    HYDROGEN_PIDS=$(pgrep -f "hydrogen" || echo "")
    
    if [ ! -z "$HYDROGEN_PIDS" ]; then
        print_warning "Found running Hydrogen processes with PIDs: $HYDROGEN_PIDS"
        
        # Try graceful shutdown first
        for PID in $HYDROGEN_PIDS; do
            print_info "Attempting graceful shutdown of PID $PID..."
            kill -SIGINT $PID 2>/dev/null || true
        done
        
        # Wait a bit to allow for graceful shutdown
        sleep 5
        
        # Check if processes are still running
        HYDROGEN_PIDS=$(pgrep -f "hydrogen" || echo "")
        if [ ! -z "$HYDROGEN_PIDS" ]; then
            print_warning "Forcing termination of remaining processes: $HYDROGEN_PIDS"
            for PID in $HYDROGEN_PIDS; do
                kill -9 $PID 2>/dev/null || true
            done
        fi
    else
        print_info "No running Hydrogen processes found"
    fi
    
    # Check if port 5000 is in use
    if command -v netstat &> /dev/null; then
        PORT_IN_USE=$(netstat -tuln | grep ":5000 " | wc -l)
        if [ $PORT_IN_USE -gt 0 ]; then
            print_warning "Port 5000 is still in use, waiting for release..."
            sleep 10
        fi
    fi
    
    # Final check to ensure resources are released
    sleep 2
    print_info "Resource cleanup complete"
}

# Run cleanup before starting tests
cleanup_old_tests
ensure_server_not_running

# Make all test scripts executable
chmod +x $SCRIPT_DIR/test_utils.sh
chmod +x $SCRIPT_DIR/test_compilation.sh
chmod +x $SCRIPT_DIR/test_startup_shutdown.sh
chmod +x $SCRIPT_DIR/test_system_endpoints.sh
chmod +x $SCRIPT_DIR/analyze_stuck_threads.sh
chmod +x $SCRIPT_DIR/monitor_resources.sh

# Default output directories
RESULTS_DIR="$SCRIPT_DIR/results"
mkdir -p "$RESULTS_DIR"

# Get timestamp for this test run
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
SUMMARY_LOG="$RESULTS_DIR/test_summary_${TIMESTAMP}.log"

# Create arrays to track all tests
declare -a ALL_TEST_NAMES
declare -a ALL_TEST_RESULTS
declare -a ALL_TEST_DETAILS

# Print header
print_header "Hydrogen Test Runner" | tee "$SUMMARY_LOG"
echo "Started at: $(date)" | tee -a "$SUMMARY_LOG"
echo "" | tee -a "$SUMMARY_LOG"

# Function to run the compilation test
run_compilation_test() {
    print_header "Compilation Test" | tee -a "$SUMMARY_LOG"
    
    # Start the test
    print_command "$SCRIPT_DIR/test_compilation.sh" | tee -a "$SUMMARY_LOG"
    $SCRIPT_DIR/test_compilation.sh
    TEST_EXIT_CODE=$?
    
    # Report result
    if [ $TEST_EXIT_CODE -eq 0 ]; then
        print_result 0 "Compilation test completed successfully" | tee -a "$SUMMARY_LOG"
        ALL_TEST_NAMES+=("Compilation Test")
        ALL_TEST_RESULTS+=(0)
        ALL_TEST_DETAILS+=("All components compiled without errors")
    else
        print_result 1 "Compilation test failed with exit code $TEST_EXIT_CODE" | tee -a "$SUMMARY_LOG"
        ALL_TEST_NAMES+=("Compilation Test")
        ALL_TEST_RESULTS+=(1)
        ALL_TEST_DETAILS+=("One or more components failed to compile")
        
        # Look for the most recent compilation test log
        LATEST_LOG=$(find "$RESULTS_DIR" -type f -name "*compilation.log" | sort -r | head -1)
        if [ -n "$LATEST_LOG" ] && [ -f "$LATEST_LOG" ]; then
            echo "" | tee -a "$SUMMARY_LOG"
            echo "==== COMPILATION TEST EXECUTION LOG ====" | tee -a "$SUMMARY_LOG"
            cat "$LATEST_LOG" | tee -a "$SUMMARY_LOG"
            echo "==== END OF COMPILATION TEST EXECUTION LOG ====" | tee -a "$SUMMARY_LOG"
        fi
    fi
    
    echo "   Test log: $LATEST_LOG" | tee -a "$SUMMARY_LOG"
    echo "" | tee -a "$SUMMARY_LOG"
    return $TEST_EXIT_CODE
}

# Function to run a single startup/shutdown test with a configuration
run_startup_shutdown_test() {
    local CONFIG_FILE=$1
    local CONFIG_NAME=$(basename $CONFIG_FILE .json)
    
    print_header "Startup/Shutdown Test ($CONFIG_NAME)" | tee -a "$SUMMARY_LOG"
    
    # Start the test
    print_command "$SCRIPT_DIR/test_startup_shutdown.sh $SCRIPT_DIR/$CONFIG_FILE" | tee -a "$SUMMARY_LOG"
    $SCRIPT_DIR/test_startup_shutdown.sh "$SCRIPT_DIR/$CONFIG_FILE"
    TEST_EXIT_CODE=$?
    
    # Report result
    if [ $TEST_EXIT_CODE -eq 0 ]; then
        print_result 0 "Test with $CONFIG_FILE completed successfully" | tee -a "$SUMMARY_LOG"
        ALL_TEST_NAMES+=("Startup/Shutdown ($CONFIG_NAME)")
        ALL_TEST_RESULTS+=(0)
        ALL_TEST_DETAILS+=("Clean startup and shutdown sequence")
    else
        print_result 1 "Test with $CONFIG_FILE failed with exit code $TEST_EXIT_CODE" | tee -a "$SUMMARY_LOG"
        ALL_TEST_NAMES+=("Startup/Shutdown ($CONFIG_NAME)")
        ALL_TEST_RESULTS+=(1)
        ALL_TEST_DETAILS+=("Startup/shutdown sequence failed")
        
        # Look for the log file before trying to display it
        LATEST_LOG=$(find "$RESULTS_DIR" -type f -name "*$(basename $CONFIG_FILE .json).log" | sort -r | head -1)
        if [ -n "$LATEST_LOG" ] && [ -f "$LATEST_LOG" ]; then
            echo "" | tee -a "$SUMMARY_LOG"
            echo "==== TEST EXECUTION LOG FOR FAILED TEST ====" | tee -a "$SUMMARY_LOG"
            cat "$LATEST_LOG" | tee -a "$SUMMARY_LOG"
            echo "==== END OF TEST EXECUTION LOG ====" | tee -a "$SUMMARY_LOG"
            
            # Also display the actual Hydrogen internal log file
            HYDROGEN_LOG="$SCRIPT_DIR/hydrogen_test.log"
            if [ -f "$HYDROGEN_LOG" ]; then
                echo "" | tee -a "$SUMMARY_LOG"
                echo "==== HYDROGEN INTERNAL LOG (LAST 100 LINES) ====" | tee -a "$SUMMARY_LOG"
                tail -100 "$HYDROGEN_LOG" | tee -a "$SUMMARY_LOG"
                echo "==== END OF HYDROGEN INTERNAL LOG ====" | tee -a "$SUMMARY_LOG"
                echo "" | tee -a "$SUMMARY_LOG"
                print_info "Analyze the log above for error patterns to identify needed code changes in Hydrogen" | tee -a "$SUMMARY_LOG"
            else
                print_warning "Hydrogen log file not found at $HYDROGEN_LOG" | tee -a "$SUMMARY_LOG"
            fi
            echo "" | tee -a "$SUMMARY_LOG"
        fi
    fi
    
    # Look for the most recent result log for this test (if not already found)
    if [ -z "$LATEST_LOG" ]; then
        LATEST_LOG=$(find "$RESULTS_DIR" -type f -name "*$(basename $CONFIG_FILE .json).log" | sort -r | head -1)
    fi
    if [ -n "$LATEST_LOG" ]; then
        echo "   Test log: $LATEST_LOG" | tee -a "$SUMMARY_LOG"
        
        # Extract shutdown duration if available (with millisecond precision)
        SHUTDOWN_DURATION=$(grep "shut down in" "$LATEST_LOG" | grep -o "[0-9]*\.[0-9]* seconds" | head -1)
        if [ -n "$SHUTDOWN_DURATION" ]; then
            echo "   Shutdown completed in $SHUTDOWN_DURATION" | tee -a "$SUMMARY_LOG"
        fi
        
        # Check for thread leaks or stalls
        if grep -q "thread leak" "$LATEST_LOG"; then
            print_warning "Thread leaks detected" | tee -a "$SUMMARY_LOG"
        fi
        
        if grep -q "Shutdown timeout" "$LATEST_LOG"; then
            print_warning "Shutdown timed out" | tee -a "$SUMMARY_LOG"
        fi
    fi
    
    echo "" | tee -a "$SUMMARY_LOG"
    return $TEST_EXIT_CODE
}

# Function to run system endpoint tests
run_system_endpoint_tests() {
    print_header "System API Endpoints Test" | tee -a "$SUMMARY_LOG"
    
    # Start the test
    print_command "$SCRIPT_DIR/test_system_endpoints.sh" | tee -a "$SUMMARY_LOG"
    $SCRIPT_DIR/test_system_endpoints.sh
    TEST_EXIT_CODE=$?
    
    # Report result
    if [ $TEST_EXIT_CODE -eq 0 ]; then
        print_result 0 "System endpoint tests completed successfully" | tee -a "$SUMMARY_LOG"
        ALL_TEST_NAMES+=("System API Endpoints")
        ALL_TEST_RESULTS+=(0)
        ALL_TEST_DETAILS+=("All API endpoints responded correctly")
    else
        print_result 1 "System endpoint tests failed with exit code $TEST_EXIT_CODE" | tee -a "$SUMMARY_LOG"
        ALL_TEST_NAMES+=("System API Endpoints")
        ALL_TEST_RESULTS+=(1)
        ALL_TEST_DETAILS+=("One or more API endpoints failed to respond correctly")
        
        # Look for the log file
        LATEST_LOG=$(find "$RESULTS_DIR" -type f -name "system_test_*.log" | sort -r | head -1)
        if [ -n "$LATEST_LOG" ] && [ -f "$LATEST_LOG" ]; then
            echo "" | tee -a "$SUMMARY_LOG"
            echo "==== SYSTEM ENDPOINT TEST EXECUTION LOG ====" | tee -a "$SUMMARY_LOG"
            cat "$LATEST_LOG" | tee -a "$SUMMARY_LOG"
            echo "==== END OF SYSTEM ENDPOINT TEST EXECUTION LOG ====" | tee -a "$SUMMARY_LOG"
        fi
    fi
    
    echo "   Test log: $LATEST_LOG" | tee -a "$SUMMARY_LOG"
    echo "" | tee -a "$SUMMARY_LOG"
    return $TEST_EXIT_CODE
}

# Parse command line argument
TEST_TYPE=${1:-"all"}  # Default to "all" if not specified

# Run compilation test first
run_compilation_test
COMPILATION_EXIT_CODE=$?

# Only proceed with other tests if compilation passes
if [ $COMPILATION_EXIT_CODE -ne 0 ]; then
    print_warning "Compilation failed - skipping startup/shutdown tests" | tee -a "$SUMMARY_LOG"
    EXIT_CODE=$COMPILATION_EXIT_CODE
else
    case "$TEST_TYPE" in
        "min")
            print_info "Running test with minimal configuration only" | tee -a "$SUMMARY_LOG"
            run_startup_shutdown_test "hydrogen_test_min.json"
            EXIT_CODE=$?
            ;;
        "max")
            print_info "Running test with maximal configuration only" | tee -a "$SUMMARY_LOG"
            run_startup_shutdown_test "hydrogen_test_max.json" 
            EXIT_CODE=$?
            ;;
        "all")
            print_info "Running tests with both configurations" | tee -a "$SUMMARY_LOG"
            echo "" | tee -a "$SUMMARY_LOG"
            
            # Run minimal configuration test
            print_header "MINIMAL CONFIGURATION TEST" | tee -a "$SUMMARY_LOG"
            run_startup_shutdown_test "hydrogen_test_min.json"
            MIN_EXIT_CODE=$?
            
            # Ensure all resources are cleaned up before next test
            echo "" | tee -a "$SUMMARY_LOG"
            print_info "Waiting for resources to be released before next test..." | tee -a "$SUMMARY_LOG"
            ensure_server_not_running
            
            # Run maximal configuration test
            print_header "MAXIMAL CONFIGURATION TEST" | tee -a "$SUMMARY_LOG"
            run_startup_shutdown_test "hydrogen_test_max.json"
            MAX_EXIT_CODE=$?
            
            # Ensure all resources are cleaned up before next test
            echo "" | tee -a "$SUMMARY_LOG"
            print_info "Waiting for resources to be released before next test..." | tee -a "$SUMMARY_LOG"
            ensure_server_not_running
            
            # Run system endpoint tests
            print_header "SYSTEM ENDPOINT TESTS" | tee -a "$SUMMARY_LOG"
            run_system_endpoint_tests
            API_EXIT_CODE=$?
            
            # Final cleanup
            ensure_server_not_running
            
            # Set overall exit code
            if [ $MIN_EXIT_CODE -eq 0 ] && [ $MAX_EXIT_CODE -eq 0 ] && [ $API_EXIT_CODE -eq 0 ]; then
                EXIT_CODE=0
            else
                EXIT_CODE=1
            fi
            ;;
        *)
            print_warning "Invalid test type: $TEST_TYPE" | tee -a "$SUMMARY_LOG"
            echo "Usage: $0 [min|max|all]" | tee -a "$SUMMARY_LOG"
            echo "  min: Run with minimal configuration only" | tee -a "$SUMMARY_LOG"
            echo "  max: Run with maximal configuration only" | tee -a "$SUMMARY_LOG"
            echo "  all: Run with both configurations (default)" | tee -a "$SUMMARY_LOG"
            EXIT_CODE=1
            ;;
    esac
fi

# Print overall summary
print_header "Test Summary" | tee -a "$SUMMARY_LOG"
echo "Completed at: $(date)" | tee -a "$SUMMARY_LOG"
echo "Summary log: $SUMMARY_LOG" | tee -a "$SUMMARY_LOG"
echo "" | tee -a "$SUMMARY_LOG"

# Calculate pass/fail counts
PASS_COUNT=0
FAIL_COUNT=0
TOTAL_COUNT=${#ALL_TEST_NAMES[@]}

for result in "${ALL_TEST_RESULTS[@]}"; do
    if [ $result -eq 0 ]; then
        ((PASS_COUNT++))
    else
        ((FAIL_COUNT++))
    fi
done

# Print individual test results
echo -e "${BLUE}${BOLD}Individual Test Results:${NC}" | tee -a "$SUMMARY_LOG"
for i in "${!ALL_TEST_NAMES[@]}"; do
    print_test_item ${ALL_TEST_RESULTS[$i]} "${ALL_TEST_NAMES[$i]}" "${ALL_TEST_DETAILS[$i]}" | tee -a "$SUMMARY_LOG"
done

# Print statistics
print_test_summary $TOTAL_COUNT $PASS_COUNT $FAIL_COUNT | tee -a "$SUMMARY_LOG"

if [ $EXIT_CODE -eq 0 ]; then
    echo -e "${GREEN}${BOLD}${PASS_ICON} OVERALL RESULT: ALL TESTS PASSED${NC}" | tee -a "$SUMMARY_LOG"
else
    echo -e "${RED}${BOLD}${FAIL_ICON} OVERALL RESULT: ONE OR MORE TESTS FAILED${NC}" | tee -a "$SUMMARY_LOG"
fi
echo -e "${BLUE}============================================${NC}" | tee -a "$SUMMARY_LOG"

# Tips for additional diagnostics
echo "" | tee -a "$SUMMARY_LOG"
echo "For more detailed analysis:" | tee -a "$SUMMARY_LOG"
echo "  • Thread analysis:     $SCRIPT_DIR/analyze_stuck_threads.sh <pid>" | tee -a "$SUMMARY_LOG"
echo "  • Resource monitoring: $SCRIPT_DIR/monitor_resources.sh <pid> [seconds]" | tee -a "$SUMMARY_LOG"
echo "" | tee -a "$SUMMARY_LOG"

exit $EXIT_CODE