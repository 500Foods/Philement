#!/bin/bash
#
# Test 40: JSON Error Handling
# Tests if the application correctly handles JSON syntax errors in configuration files
# and provides meaningful error messages with position information
#
# VERSION HISTORY
# 3.0.0 - 2025-07-02 - Complete rewrite to use new modular test libraries
# 2.0.0 - 2025-06-17 - Major refactoring: fixed all shellcheck warnings, improved modularity, enhanced comments
# 1.0.0 - Original version - Basic JSON error handling test
#

# Get the directory where this script is located
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

# Source the new modular test libraries
source "$SCRIPT_DIR/lib/log_output.sh"
source "$SCRIPT_DIR/lib/file_utils.sh"
source "$SCRIPT_DIR/lib/framework.sh"
source "$SCRIPT_DIR/lib/lifecycle.sh"

# Test configuration
TEST_NAME="JSON Error Handling"
SCRIPT_VERSION="3.0.0"
EXIT_CODE=0
TOTAL_SUBTESTS=4
PASS_COUNT=0

# Auto-extract test number and set up environment
TEST_NUMBER=$(extract_test_number "${BASH_SOURCE[0]}")
set_test_number "$TEST_NUMBER"
reset_subtest_counter

# Print beautiful test header
print_test_header "$TEST_NAME" "$SCRIPT_VERSION"

# Set up results directory
RESULTS_DIR="$SCRIPT_DIR/results"
mkdir -p "$RESULTS_DIR"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
RESULT_LOG="$RESULTS_DIR/test_${TEST_NUMBER}_${TIMESTAMP}.log"
ERROR_OUTPUT="$RESULTS_DIR/json_error_output_${TIMESTAMP}.txt"

# Navigate to the project root (one level up from tests directory)
if ! navigate_to_project_root "$SCRIPT_DIR"; then
    print_error "Failed to navigate to project root directory"
    exit 1
fi

# Validate Hydrogen Binary
next_subtest
print_subtest "Locate Hydrogen Binary"
HYDROGEN_DIR="$( cd "$SCRIPT_DIR/.." && pwd )"
if HYDROGEN_BIN=$(find_hydrogen_binary "$HYDROGEN_DIR"); then
    print_message "Using Hydrogen binary: $HYDROGEN_BIN"
    print_result 0 "Hydrogen binary found and validated"
    ((PASS_COUNT++))
else
    print_result 1 "Failed to find Hydrogen binary"
    EXIT_CODE=1
fi

# Test configuration with JSON syntax error (missing comma)
TEST_CONFIG=$(get_config_path "hydrogen_test_json.json")

# Validate configuration file exists
next_subtest
print_subtest "Validate Test Configuration File"
if validate_config_file "$TEST_CONFIG"; then
    print_message "Using test file with JSON syntax error (missing comma)"
    ((PASS_COUNT++))
else
    EXIT_CODE=1
fi

# Test 1: Run hydrogen with malformed config - should fail
next_subtest
print_subtest "Launch with Malformed JSON Configuration"
print_message "Testing configuration: $(basename "$TEST_CONFIG" .json)"
print_command "$(basename "$HYDROGEN_BIN") $(basename "$TEST_CONFIG")"

# Capture both stdout and stderr
if "$HYDROGEN_BIN" "$TEST_CONFIG" &> "$ERROR_OUTPUT"; then
    print_result 1 "Application should have exited with an error but didn't"
    EXIT_CODE=1
else
    print_result 0 "Application exited with error as expected"
    ((PASS_COUNT++))
fi

# Test 2: Check error output for line and column information
next_subtest
print_subtest "Verify Error Message Contains Position Information"
print_message "Examining error output..."

if grep -q "line" "$ERROR_OUTPUT" && grep -q "column" "$ERROR_OUTPUT"; then
    print_result 0 "Error message contains line and column information"
    print_message "Error output:"
    # Display the error output content
    while IFS= read -r line; do
        print_output "$line"
    done < "$ERROR_OUTPUT"
    ((PASS_COUNT++))
else
    print_result 1 "Error message does not contain line and column information"
    print_message "Error output:"
    # Display the error output content
    while IFS= read -r line; do
        print_output "$line"
    done < "$ERROR_OUTPUT"
    EXIT_CODE=1
fi

# Save error output to results directory
cp "$ERROR_OUTPUT" "$RESULTS_DIR/json_error_full_${TIMESTAMP}.txt"
print_message "Full error output saved to: $(convert_to_relative_path "$RESULTS_DIR/json_error_full_${TIMESTAMP}.txt")"

# Clean up temporary error output file
rm -f "$ERROR_OUTPUT"

# Export results for test_all.sh integration
# Derive test name from script filename for consistency with test_00_all.sh
TEST_IDENTIFIER=$(basename "${BASH_SOURCE[0]}" .sh | sed 's/test_[0-9]*_//')
export_subtest_results "${TEST_NUMBER}_${TEST_IDENTIFIER}" "$TOTAL_SUBTESTS" "$PASS_COUNT" > /dev/null

# Print completion table
print_test_completion "$TEST_NAME"

end_test $EXIT_CODE $TOTAL_SUBTESTS $PASS_COUNT > /dev/null

exit $EXIT_CODE
