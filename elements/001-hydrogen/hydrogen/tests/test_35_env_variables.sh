#!/bin/bash
#
# Test 35: Environment Variable Substitution Test
# Tests that configuration values can be provided via environment variables
#
# VERSION HISTORY
# 3.0.0 - 2025-07-02 - Complete rewrite to use new modular test libraries and match test 15 structure
# 2.1.0 - 2025-07-02 - Migrated to use modular lib/ scripts
# 2.0.0 - 2025-06-17 - Major refactoring: fixed all shellcheck warnings, improved modularity, enhanced comments
# 1.0.0 - Original version - Basic environment variable testing functionality
#

# Get the directory where this script is located
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

# Source the new modular test libraries
source "$SCRIPT_DIR/lib/log_output.sh"
source "$SCRIPT_DIR/lib/file_utils.sh"
source "$SCRIPT_DIR/lib/framework.sh"
source "$SCRIPT_DIR/lib/env_utils.sh"
source "$SCRIPT_DIR/lib/lifecycle.sh"

# Test configuration
TEST_NAME="Environment Variable Substitution Test"
SCRIPT_VERSION="3.0.0"
EXIT_CODE=0
TOTAL_SUBTESTS=16
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

# Configuration file paths
CONFIG_FILE="$SCRIPT_DIR/configs/hydrogen_test_min.json"

# Timeouts and limits
STARTUP_TIMEOUT=10    # Seconds to wait for startup
SHUTDOWN_TIMEOUT=90   # Hard limit on shutdown time
SHUTDOWN_ACTIVITY_TIMEOUT=5  # Timeout if no new log activity

# Output files and directories
LOG_FILE="$SCRIPT_DIR/logs/hydrogen_test.log"
DIAG_DIR="$SCRIPT_DIR/diagnostics"
DIAG_TEST_DIR="$DIAG_DIR/env_variables_test_${TIMESTAMP}"

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

# Validate configuration file exists
next_subtest
print_subtest "Validate Configuration File"
if validate_config_file "$CONFIG_FILE"; then
    print_message "Using config: $(convert_to_relative_path "$CONFIG_FILE")"
    ((PASS_COUNT++))
else
    EXIT_CODE=1
fi

# Create output directories
next_subtest
print_subtest "Create Output Directories"
if setup_output_directories "$RESULTS_DIR" "$DIAG_DIR" "$LOG_FILE" "$DIAG_TEST_DIR"; then
    ((PASS_COUNT++))
else
    EXIT_CODE=1
fi

# Test basic environment variables with lifecycle test
set_basic_test_environment
config_name="basic_env_vars"
run_lifecycle_test "$CONFIG_FILE" "$config_name" "$DIAG_TEST_DIR" "$STARTUP_TIMEOUT" "$SHUTDOWN_TIMEOUT" "$SHUTDOWN_ACTIVITY_TIMEOUT" "$HYDROGEN_BIN" "$LOG_FILE" "PASS_COUNT" "EXIT_CODE"

# Test missing environment variables fallback
reset_environment_variables
config_name="missing_env_vars"
run_lifecycle_test "$CONFIG_FILE" "$config_name" "$DIAG_TEST_DIR" "$STARTUP_TIMEOUT" "$SHUTDOWN_TIMEOUT" "$SHUTDOWN_ACTIVITY_TIMEOUT" "$HYDROGEN_BIN" "$LOG_FILE" "PASS_COUNT" "EXIT_CODE"

# Test environment variable type conversion
set_type_conversion_environment
config_name="type_conversion"
run_lifecycle_test "$CONFIG_FILE" "$config_name" "$DIAG_TEST_DIR" "$STARTUP_TIMEOUT" "$SHUTDOWN_TIMEOUT" "$SHUTDOWN_ACTIVITY_TIMEOUT" "$HYDROGEN_BIN" "$LOG_FILE" "PASS_COUNT" "EXIT_CODE"

# Test environment variable validation
set_validation_test_environment
config_name="env_validation"
run_lifecycle_test "$CONFIG_FILE" "$config_name" "$DIAG_TEST_DIR" "$STARTUP_TIMEOUT" "$SHUTDOWN_TIMEOUT" "$SHUTDOWN_ACTIVITY_TIMEOUT" "$HYDROGEN_BIN" "$LOG_FILE" "PASS_COUNT" "EXIT_CODE"

# Reset environment variables
next_subtest
print_subtest "Reset Environment Variables"
reset_environment_variables
print_result 0 "All environment variables reset"
((PASS_COUNT++))

# Export results for test_all.sh integration
export_subtest_results "35_env_variables" $TOTAL_SUBTESTS $PASS_COUNT > /dev/null

# Print completion table
print_test_completion "$TEST_NAME"

end_test $EXIT_CODE $TOTAL_SUBTESTS $PASS_COUNT > /dev/null

exit $EXIT_CODE
