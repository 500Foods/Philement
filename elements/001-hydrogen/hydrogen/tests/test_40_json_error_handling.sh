#!/bin/bash
#
# JSON Error Handling Test
# Tests if the Hydrogen application correctly handles JSON syntax errors in configuration

# Get the directory where this script is located
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
HYDROGEN_DIR="$( cd "$SCRIPT_DIR/.." && pwd )"

# Include the common test utilities
source "$SCRIPT_DIR/support_utils.sh"

# Create output directories
RESULTS_DIR="$SCRIPT_DIR/results"
mkdir -p "$RESULTS_DIR"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
RESULT_LOG="$RESULTS_DIR/json_error_test_${TIMESTAMP}.log"
ERROR_OUTPUT="$RESULTS_DIR/json_error_output_${TIMESTAMP}.txt"

# Start the test
start_test "JSON Error Handling Test" | tee -a "$RESULT_LOG"

# Use the permanent test file with a JSON syntax error (missing comma)
TEST_CONFIG=$(get_config_path "hydrogen_test_json.json")
print_info "Using test file with JSON syntax error (missing comma)" | tee -a "$RESULT_LOG"

# Determine which hydrogen build to use (prefer release build if available)
if [ -f "$HYDROGEN_DIR/hydrogen_release" ]; then
    HYDROGEN_BIN="$HYDROGEN_DIR/hydrogen_release"
    print_info "Using release build for testing" | tee -a "$RESULT_LOG"
else
    HYDROGEN_BIN="$HYDROGEN_DIR/hydrogen"
    print_info "Standard build will be used" | tee -a "$RESULT_LOG"
fi

# Test 1: Run hydrogen with malformed config - should fail
print_header "Test 1: Launch with malformed JSON configuration" | tee -a "$RESULT_LOG"
print_command "$HYDROGEN_BIN $TEST_CONFIG" | tee -a "$RESULT_LOG"

if $HYDROGEN_BIN "$TEST_CONFIG" 2> "$ERROR_OUTPUT"; then
    print_result 1 "Application should have exited with an error but didn't" | tee -a "$RESULT_LOG"
    rm -f "$ERROR_OUTPUT"
    end_test 1 "JSON Error Handling Test" | tee -a "$RESULT_LOG"
    exit 1
else
    print_result 0 "Application exited with error as expected" | tee -a "$RESULT_LOG"
fi

# Test 2: Check error output for line and column information
print_header "Test 2: Verify error message contains position information" | tee -a "$RESULT_LOG"
print_info "Examining error output..." | tee -a "$RESULT_LOG"

if grep -q "line" "$ERROR_OUTPUT" && grep -q "column" "$ERROR_OUTPUT"; then
    print_result 0 "Error message contains line and column information" | tee -a "$RESULT_LOG"
    print_info "Error output:" | tee -a "$RESULT_LOG"
    cat "$ERROR_OUTPUT" | tee -a "$RESULT_LOG"
    TEST_RESULT=0
else
    print_result 1 "Error message does not contain line and column information" | tee -a "$RESULT_LOG"
    print_info "Error output:" | tee -a "$RESULT_LOG"
    cat "$ERROR_OUTPUT" | tee -a "$RESULT_LOG"
    TEST_RESULT=1
fi

# Save error output to results directory
cp "$ERROR_OUTPUT" "$RESULTS_DIR/json_error_full_${TIMESTAMP}.txt"
print_info "Full error output saved to: $(convert_to_relative_path "$RESULTS_DIR/json_error_full_${TIMESTAMP}.txt")" | tee -a "$RESULT_LOG"

# Clean up
rm -f "$ERROR_OUTPUT"

# Track subtest results
TOTAL_SUBTESTS=2  # Malformed JSON test and Error Position test
PASS_COUNT=0

# First test - Application exits with error when given malformed JSON
if ! grep -q "Application should have exited with an error but didn't" "$RESULT_LOG"; then
    ((PASS_COUNT++))
fi

# Second test - Error message contains position information
if grep -q "Error message contains line and column information" "$RESULT_LOG"; then
    ((PASS_COUNT++))
fi

# Get test name from script name
TEST_NAME=$(basename "$0" .sh | sed 's/^test_//')

# Export subtest results for test_all.sh to pick up
export_subtest_results "$TEST_NAME" $TOTAL_SUBTESTS $PASS_COUNT

# Log subtest results
print_info "JSON Error Handling Test: $PASS_COUNT of $TOTAL_SUBTESTS subtests passed" | tee -a "$RESULT_LOG"

# End test with appropriate exit code
end_test $TEST_RESULT "JSON Error Handling Test" | tee -a "$RESULT_LOG"
exit $TEST_RESULT