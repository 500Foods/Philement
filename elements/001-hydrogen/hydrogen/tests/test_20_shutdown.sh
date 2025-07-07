#!/bin/bash
#
# Test: Shutdown
# Tests the shutdown functionality of the application with a minimal configuration
#
# CHANGELOG
# 2.0.1 - 2025-07-06 - Added missing shellcheck justifications
# 2.0.0 - 2025-07-02 - Complete rewrite to use new modular test libraries
# 1.1.0 - Enhanced with proper error handling, modular functions, and shellcheck compliance
# 1.0.0 - Initial version with basic shutdown testing

# Test configuration
TEST_NAME="Shutdown"
SCRIPT_VERSION="2.0.1"

# Get the directory where this script is located
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

# Source the new modular test libraries with guard clauses
if [[ -z "$TABLES_SH_GUARD" ]] || ! command -v tables_render_from_json >/dev/null 2>&1; then
    # shellcheck source=tests/lib/tables.sh # Resolve path statically
    source "$SCRIPT_DIR/lib/tables.sh"
    export TABLES_SOURCED=true
fi

if [[ -z "$LOG_OUTPUT_SH_GUARD" ]]; then
    # shellcheck source=tests/lib/log_output.sh # Resolve path statically
    source "$SCRIPT_DIR/lib/log_output.sh"
fi

# shellcheck source=tests/lib/file_utils.sh # Resolve path statically
source "$SCRIPT_DIR/lib/file_utils.sh"
# shellcheck source=tests/lib/framework.sh # Resolve path statically
source "$SCRIPT_DIR/lib/framework.sh"
# shellcheck source=tests/lib/lifecycle.sh # Resolve path statically
source "$SCRIPT_DIR/lib/lifecycle.sh"

# Test configuration
EXIT_CODE=0
TOTAL_SUBTESTS=6
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

# Navigate to the project root (one level up from tests directory)
if ! navigate_to_project_root "$SCRIPT_DIR"; then
    print_error "Failed to navigate to project root directory"
    exit 1
fi

# Validate Hydrogen Binary
next_subtest
print_subtest "Locate Hydrogen Binary"
HYDROGEN_DIR="$( cd "$SCRIPT_DIR/.." && pwd )"
# shellcheck disable=SC2153  # HYDROGEN_BIN is set by find_hydrogen_binary function
if find_hydrogen_binary "$HYDROGEN_DIR" "HYDROGEN_BIN"; then
    print_message "Using Hydrogen binary: $HYDROGEN_BIN"
    print_result 0 "Hydrogen binary found and validated"
    ((PASS_COUNT++))
else
    print_result 1 "Failed to find Hydrogen binary"
    EXIT_CODE=1
fi

# Test configuration
MIN_CONFIG="$SCRIPT_DIR/configs/hydrogen_test_min.json"

# Validate configuration file exists
next_subtest
print_subtest "Validate Configuration File"
if validate_config_file "$MIN_CONFIG"; then
    ((PASS_COUNT++))
else
    EXIT_CODE=1
fi

# Timeouts and limits
STARTUP_TIMEOUT=5     # Seconds to wait for startup
SHUTDOWN_TIMEOUT=10   # Hard limit on shutdown time
SHUTDOWN_ACTIVITY_TIMEOUT=3  # Timeout if no new log activity

# Output files and directories
LOG_FILE="$SCRIPT_DIR/logs/hydrogen_shutdown_test.log"
DIAG_DIR="$SCRIPT_DIR/diagnostics"
DIAG_TEST_DIR="$DIAG_DIR/shutdown_test_${TIMESTAMP}"

# Create output directories
next_subtest
print_subtest "Create Output Directories"
if setup_output_directories "$RESULTS_DIR" "$DIAG_DIR" "$LOG_FILE" "$DIAG_TEST_DIR"; then
    ((PASS_COUNT++))
else
    EXIT_CODE=1
fi

# Test shutdown with minimal configuration
config_name=$(basename "$MIN_CONFIG" .json)
run_lifecycle_test "$MIN_CONFIG" "$config_name" "$DIAG_TEST_DIR" "$STARTUP_TIMEOUT" "$SHUTDOWN_TIMEOUT" "$SHUTDOWN_ACTIVITY_TIMEOUT" "$HYDROGEN_BIN" "$LOG_FILE" "PASS_COUNT" "EXIT_CODE"

# Export results for test_all.sh integration
# Derive test name from script filename for consistency with test_00_all.sh
TEST_IDENTIFIER=$(basename "${BASH_SOURCE[0]}" .sh | sed 's/test_[0-9]*_//')
export_subtest_results "${TEST_NUMBER}_${TEST_IDENTIFIER}" "$TOTAL_SUBTESTS" "$PASS_COUNT" "$TEST_NAME" > /dev/null

# Print completion table
print_test_completion "$TEST_NAME"

end_test $EXIT_CODE $TOTAL_SUBTESTS $PASS_COUNT > /dev/null

# Return status code if sourced, exit if run standalone
if [[ "$RUNNING_IN_TEST_SUITE" == "true" ]]; then
    return $EXIT_CODE
else
    exit $EXIT_CODE
fi
