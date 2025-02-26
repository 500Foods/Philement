#!/bin/bash
#
# Hydrogen Test Runner
# Executes startup/shutdown tests with specified configurations
#
# Usage: ./run_tests.sh [min|max|all]

# Get the directory where this script is located
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

# Clean up previous test results and diagnostics
cleanup_old_tests() {
    echo "Cleaning up previous test results and diagnostics..."
    
    # Define directories to clean
    RESULTS_DIR="$SCRIPT_DIR/results"
    DIAGNOSTICS_DIR="$SCRIPT_DIR/diagnostics"
    
    # Remove all files in results directory
    if [ -d "$RESULTS_DIR" ]; then
        rm -rf "$RESULTS_DIR"/*
        echo "Removed old test results from $RESULTS_DIR"
    fi
    
    # Remove all files in diagnostics directory
    if [ -d "$DIAGNOSTICS_DIR" ]; then
        rm -rf "$DIAGNOSTICS_DIR"/*
        echo "Removed old test diagnostics from $DIAGNOSTICS_DIR"
    fi
    
    echo "Cleanup complete"
}

# Run cleanup before starting tests
cleanup_old_tests

# Make all test scripts executable
chmod +x $SCRIPT_DIR/test_startup_shutdown.sh
chmod +x $SCRIPT_DIR/analyze_stuck_threads.sh
chmod +x $SCRIPT_DIR/monitor_resources.sh

# Default output directories
RESULTS_DIR="$SCRIPT_DIR/results"
mkdir -p "$RESULTS_DIR"

# Get timestamp for this test run
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
SUMMARY_LOG="$RESULTS_DIR/test_summary_${TIMESTAMP}.log"

# Print header
echo "============================================" | tee "$SUMMARY_LOG"
echo "Hydrogen Startup/Shutdown Test Runner" | tee -a "$SUMMARY_LOG"
echo "Started at: $(date)" | tee -a "$SUMMARY_LOG"
echo "============================================" | tee -a "$SUMMARY_LOG"
echo "" | tee -a "$SUMMARY_LOG"

# Function to run a single test with a configuration
run_test() {
    local CONFIG_FILE=$1
    echo "Running test with configuration: $CONFIG_FILE" | tee -a "$SUMMARY_LOG"
    
    # Start the test
    $SCRIPT_DIR/test_startup_shutdown.sh "$SCRIPT_DIR/$CONFIG_FILE"
    TEST_EXIT_CODE=$?
    
    # Report result
    if [ $TEST_EXIT_CODE -eq 0 ]; then
        echo "✅ Test with $CONFIG_FILE completed successfully" | tee -a "$SUMMARY_LOG"
    else
        echo "❌ Test with $CONFIG_FILE failed with exit code $TEST_EXIT_CODE" | tee -a "$SUMMARY_LOG"
        
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
                echo "TIP: Analyze the log above for error patterns to identify needed code changes in Hydrogen" | tee -a "$SUMMARY_LOG"
            else
                echo "WARNING: Hydrogen log file not found at $HYDROGEN_LOG" | tee -a "$SUMMARY_LOG"
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
            echo "   ⚠️ Thread leaks detected" | tee -a "$SUMMARY_LOG"
        fi
        
        if grep -q "Shutdown timeout" "$LATEST_LOG"; then
            echo "   ⚠️ Shutdown timed out" | tee -a "$SUMMARY_LOG"
        fi
    fi
    
    echo "" | tee -a "$SUMMARY_LOG"
    return $TEST_EXIT_CODE
}

# Parse command line argument
TEST_TYPE=${1:-"all"}  # Default to "all" if not specified

case "$TEST_TYPE" in
    "min")
        echo "Running test with minimal configuration only" | tee -a "$SUMMARY_LOG"
        run_test "hydrogen_test_min.json"
        EXIT_CODE=$?
        ;;
    "max")
        echo "Running test with maximal configuration only" | tee -a "$SUMMARY_LOG"
        run_test "hydrogen_test_max.json" 
        EXIT_CODE=$?
        ;;
    "all")
        echo "Running tests with both configurations" | tee -a "$SUMMARY_LOG"
        echo "" | tee -a "$SUMMARY_LOG"
        
        # Run minimal configuration test
        echo "=== MINIMAL CONFIGURATION TEST ===" | tee -a "$SUMMARY_LOG"
        run_test "hydrogen_test_min.json"
        MIN_EXIT_CODE=$?
        
        echo "" | tee -a "$SUMMARY_LOG"
        
        # Run maximal configuration test
        echo "=== MAXIMAL CONFIGURATION TEST ===" | tee -a "$SUMMARY_LOG"
        run_test "hydrogen_test_max.json"
        MAX_EXIT_CODE=$?
        
        # Set overall exit code
        if [ $MIN_EXIT_CODE -eq 0 ] && [ $MAX_EXIT_CODE -eq 0 ]; then
            EXIT_CODE=0
        else
            EXIT_CODE=1
        fi
        ;;
    *)
        echo "Invalid test type: $TEST_TYPE" | tee -a "$SUMMARY_LOG"
        echo "Usage: $0 [min|max|all]" | tee -a "$SUMMARY_LOG"
        echo "  min: Run with minimal configuration only" | tee -a "$SUMMARY_LOG"
        echo "  max: Run with maximal configuration only" | tee -a "$SUMMARY_LOG"
        echo "  all: Run with both configurations (default)" | tee -a "$SUMMARY_LOG"
        exit 1
        ;;
esac

# Print overall summary
echo "============================================" | tee -a "$SUMMARY_LOG"
echo "Test Summary" | tee -a "$SUMMARY_LOG"
echo "Completed at: $(date)" | tee -a "$SUMMARY_LOG"
echo "Summary log: $SUMMARY_LOG" | tee -a "$SUMMARY_LOG"

if [ $EXIT_CODE -eq 0 ]; then
    echo "OVERALL RESULT: ✅ ALL TESTS PASSED" | tee -a "$SUMMARY_LOG"
else
    echo "OVERALL RESULT: ❌ ONE OR MORE TESTS FAILED" | tee -a "$SUMMARY_LOG"
fi
echo "============================================" | tee -a "$SUMMARY_LOG"

# Tips for additional diagnostics
echo "" | tee -a "$SUMMARY_LOG"
echo "For more detailed analysis:" | tee -a "$SUMMARY_LOG"
echo "  • Thread analysis:    $SCRIPT_DIR/analyze_stuck_threads.sh <pid>" | tee -a "$SUMMARY_LOG"
echo "  • Resource monitoring: $SCRIPT_DIR/monitor_resources.sh <pid> [seconds]" | tee -a "$SUMMARY_LOG"
echo "" | tee -a "$SUMMARY_LOG"

exit $EXIT_CODE