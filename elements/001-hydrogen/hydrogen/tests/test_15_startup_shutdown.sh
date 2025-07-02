#!/bin/bash
#
# Test 15: Hydrogen Startup/Shutdown Test
# Tests startup and shutdown of the Hydrogen application with minimal and maximal configurations
#
# VERSION HISTORY
# 3.0.0 - 2025-07-02 - Complete rewrite to use new modular test libraries
# 2.0.0 - 2025-06-17 - Major refactoring: fixed all shellcheck warnings, improved modularity, enhanced comments
# 1.0.0 - Initial version
#

# Get the directory where this script is located
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

# Source the new modular test libraries
source "$SCRIPT_DIR/lib/log_output.sh"
source "$SCRIPT_DIR/lib/file_utils.sh"
source "$SCRIPT_DIR/lib/framework.sh"
source "$SCRIPT_DIR/lib/lifecycle.sh"

# Test configuration
TEST_NAME="Hydrogen Startup/Shutdown Test"
SCRIPT_VERSION="3.0.0"
EXIT_CODE=0
TOTAL_SUBTESTS=9
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
print_subtest "Validate Hydrogen Binary"
HYDROGEN_DIR="$( cd "$SCRIPT_DIR/.." && pwd )"
if HYDROGEN_BIN=$(find_hydrogen_binary "$HYDROGEN_DIR"); then
    print_message "Using Hydrogen binary: $HYDROGEN_BIN"
    print_result 0 "Hydrogen binary found and validated"
    ((PASS_COUNT++))
else
    print_result 1 "Failed to find Hydrogen binary"
    EXIT_CODE=1
fi

# Test configurations
MIN_CONFIG="$SCRIPT_DIR/configs/hydrogen_test_min.json"
MAX_CONFIG="$SCRIPT_DIR/configs/hydrogen_test_max.json"

# Validate configuration files exist
next_subtest
print_subtest "Validate Configuration Files"
if validate_config_files "$MIN_CONFIG" "$MAX_CONFIG"; then
    ((PASS_COUNT++))
else
    EXIT_CODE=1
fi

# Timeouts and limits
STARTUP_TIMEOUT=10    # Seconds to wait for startup
SHUTDOWN_TIMEOUT=90   # Hard limit on shutdown time
SHUTDOWN_ACTIVITY_TIMEOUT=5  # Timeout if no new log activity

# Output files and directories
LOG_FILE="$SCRIPT_DIR/logs/hydrogen_test.log"
DIAG_DIR="$SCRIPT_DIR/diagnostics"
DIAG_TEST_DIR="$DIAG_DIR/startup_shutdown_test_${TIMESTAMP}"

# Create output directories
next_subtest
print_subtest "Create Output Directories"
if setup_output_directories "$RESULTS_DIR" "$DIAG_DIR" "$LOG_FILE" "$DIAG_TEST_DIR"; then
    ((PASS_COUNT++))
else
    EXIT_CODE=1
fi

# Test minimal configuration
config_name=$(basename "$MIN_CONFIG" .json)
run_lifecycle_test "$MIN_CONFIG" "$config_name" "$DIAG_TEST_DIR" "$STARTUP_TIMEOUT" "$SHUTDOWN_TIMEOUT" "$SHUTDOWN_ACTIVITY_TIMEOUT" "$HYDROGEN_BIN" "$LOG_FILE" "PASS_COUNT" "EXIT_CODE"

# Test maximal configuration
config_name=$(basename "$MAX_CONFIG" .json)
run_lifecycle_test "$MAX_CONFIG" "$config_name" "$DIAG_TEST_DIR" "$STARTUP_TIMEOUT" "$SHUTDOWN_TIMEOUT" "$SHUTDOWN_ACTIVITY_TIMEOUT" "$HYDROGEN_BIN" "$LOG_FILE" "PASS_COUNT" "EXIT_CODE"

# Export results for test_all.sh integration
export_subtest_results "15_startup_shutdown" $TOTAL_SUBTESTS $PASS_COUNT > /dev/null

# Print completion table
print_test_completion "$TEST_NAME"

end_test $EXIT_CODE $TOTAL_SUBTESTS $PASS_COUNT > /dev/null

exit $EXIT_CODE
