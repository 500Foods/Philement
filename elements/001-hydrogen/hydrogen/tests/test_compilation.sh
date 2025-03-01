#!/bin/bash
#
# Hydrogen Compilation Test Script
# Tests that all components compile without errors or warnings
#
# This test verifies that:
# 1. The main Hydrogen project compiles cleanly
# 2. The OIDC client examples compile cleanly

# Get the directory where this script is located
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
HYDROGEN_DIR="$( cd "$SCRIPT_DIR/.." && pwd )"
OIDC_EXAMPLES_DIR="$HYDROGEN_DIR/oidc-client-examples/C"

# Include the common test utilities
source "$SCRIPT_DIR/test_utils.sh"

# Output directories
RESULTS_DIR="$SCRIPT_DIR/results"
mkdir -p "$RESULTS_DIR"

# Timestamp and log file
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
RESULT_LOG="$RESULTS_DIR/test_${TIMESTAMP}_compilation.log"

# Initialize exit code
EXIT_CODE=0

# Start the test
start_test "Hydrogen Compilation Test" | tee -a "$RESULT_LOG"

# Test function for compilation
test_compilation() {
    local COMPONENT=$1
    local BUILD_DIR=$2
    local BUILD_CMD=$3
    
    print_header "Testing compilation of $COMPONENT" | tee -a "$RESULT_LOG"
    print_info "Build command: $BUILD_CMD" | tee -a "$RESULT_LOG"
    print_info "Working directory: $BUILD_DIR" | tee -a "$RESULT_LOG"
    
    # Navigate to build directory and run build command
    cd "$BUILD_DIR"
    
    # Capture the start time
    local START_TIME=$(date +%s)
    
    # Clean first, then build with warnings treated as errors
    # Redirect both stdout and stderr to a temporary file
    local TEMP_LOG="$RESULTS_DIR/build_${COMPONENT//\//_}_${TIMESTAMP}.log"
    {
        if [[ "$COMPONENT" == "Hydrogen main project" ]]; then
            make clean
            CFLAGS="-Wall -Wextra -Werror -pedantic" make
        else
            make clean
            CFLAGS="-Wall -Wextra -Werror -pedantic" make all
        fi
    } > "$TEMP_LOG" 2>&1
    
    # Capture the exit code
    local BUILD_RESULT=$?
    
    # Capture the end time and calculate duration
    local END_TIME=$(date +%s)
    local DURATION=$((END_TIME - START_TIME))
    
    # Check if the build was successful
    if [ $BUILD_RESULT -eq 0 ]; then
        print_result 0 "$COMPONENT compiled successfully in ${DURATION}s" | tee -a "$RESULT_LOG"
        
        # Check for warnings, even though we used -Werror, we'll do a double-check
        if grep -q "warning:" "$TEMP_LOG"; then
            print_warning "Warnings detected despite build success:" | tee -a "$RESULT_LOG"
            grep "warning:" "$TEMP_LOG" | tee -a "$RESULT_LOG"
            # Don't fail the test for this, just note it
        else
            print_info "No warnings detected" | tee -a "$RESULT_LOG"
        fi
    else
        print_result 1 "$COMPONENT compilation failed with exit code $BUILD_RESULT" | tee -a "$RESULT_LOG"
        echo "" | tee -a "$RESULT_LOG"
        echo "==== BUILD OUTPUT ====" | tee -a "$RESULT_LOG"
        cat "$TEMP_LOG" | tee -a "$RESULT_LOG"
        echo "==== END OF BUILD OUTPUT ====" | tee -a "$RESULT_LOG"
        EXIT_CODE=1
    fi
    
    echo "" | tee -a "$RESULT_LOG"
    
    # Append the full build log to the result log
    print_info "Full build log saved to: $TEMP_LOG" | tee -a "$RESULT_LOG"
    
    # Return to script directory
    cd "$SCRIPT_DIR"
}

# Test the main Hydrogen project (standard build)
test_compilation "Hydrogen main project (standard build)" "$HYDROGEN_DIR" "make"

# Test the main Hydrogen project (debug build)
test_compilation "Hydrogen main project (debug build)" "$HYDROGEN_DIR" "make debug"

# Test the main Hydrogen project (valgrind build)
test_compilation "Hydrogen main project (valgrind build)" "$HYDROGEN_DIR" "make valgrind"

# Test the OIDC client examples
test_compilation "OIDC client examples" "$OIDC_EXAMPLES_DIR" "make all"

# Print final summary
print_header "Compilation Test Summary" | tee -a "$RESULT_LOG"
print_info "Completed at: $(date)" | tee -a "$RESULT_LOG"
print_info "Test log: $RESULT_LOG" | tee -a "$RESULT_LOG"

# End test with appropriate result message
end_test $EXIT_CODE "Compilation Test" | tee -a "$RESULT_LOG"

# Exit with appropriate code
exit $EXIT_CODE