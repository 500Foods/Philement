#!/bin/bash
#
# Test: Shutdown
# Tests the shutdown functionality of the application with a minimal configuration
#
# CHANGELOG
# 3.0.0 - 2025-07-30 - Overhaul #1
# 2.0.1 - 2025-07-06 - Added missing shellcheck justifications
# 2.0.0 - 2025-07-02 - Complete rewrite to use new modular test libraries
# 1.1.0 - Enhanced with proper error handling, modular functions, and shellcheck compliance
# 1.0.0 - Initial version with basic shutdown testing

# Test configuration
TEST_NAME="Shutdown"
TEST_ABBR="SHD"
TEST_NUMBER="26"
TEST_VERSION="3.0.0"

# shellcheck source=tests/lib/framework.sh # Reference framework directly
[[ -n "${FRAMEWORK_GUARD}" ]] || source "$(dirname "${BASH_SOURCE[0]}")/lib/framework.sh"
setup_test_environment

# Validate Hydrogen Binary
next_subtest
print_subtest "Locate Hydrogen Binary"

# shellcheck disable=SC2153  # HYDROGEN_BIN is set by find_hydrogen_binary function
if find_hydrogen_binary "${PROJECT_DIR}" "HYDROGEN_BIN"; then
    print_message "Using Hydrogen binary: ${HYDROGEN_BIN}"
    print_result 0 "Hydrogen binary found and validated"
    ((PASS_COUNT++))
else
    print_result 1 "Failed to find Hydrogen binary"
    EXIT_CODE=1
fi

# Test configuration
MIN_CONFIG="${SCRIPT_DIR}/configs/hydrogen_test_min.json"

# Validate configuration file exists
next_subtest
print_subtest "Validate Configuration File"
if validate_config_file "${MIN_CONFIG}"; then
    ((PASS_COUNT++))
else
    EXIT_CODE=1
fi

# Timeouts and limits
STARTUP_TIMEOUT=5     # Seconds to wait for startup
SHUTDOWN_TIMEOUT=10   # Hard limit on shutdown time
SHUTDOWN_ACTIVITY_TIMEOUT=3  # Timeout if no new log activity

# Output files and directories
BUILD_DIR="${SCRIPT_DIR}/../build"
LOG_FILE="${BUILD_DIR}/tests/logs/hydrogen_shutdown_test.log"
DIAGS_DIR="${BUILD_DIR}/tests/diagnostics"
DIAG_TEST_DIR="${DIAGS_DIR}/shutdown_test_${TIMESTAMP}"

# Create output directories
next_subtest
print_subtest "Create Output Directories"
if setup_output_directories "${RESULTS_DIR}" "${DIAGS_DIR}" "${LOG_FILE}" "${DIAG_TEST_DIR}"; then
    ((PASS_COUNT++))
else
    EXIT_CODE=1
fi

# Test shutdown with minimal configuration
config_name=$(basename "${MIN_CONFIG}" .json)
run_lifecycle_test "${MIN_CONFIG}" "${config_name}" "${DIAG_TEST_DIR}" "${STARTUP_TIMEOUT}" "${SHUTDOWN_TIMEOUT}" "${SHUTDOWN_ACTIVITY_TIMEOUT}" "${HYDROGEN_BIN}" "${LOG_FILE}" "PASS_COUNT" "EXIT_CODE"

# Print completion table
print_test_completion "${TEST_NAME}" "${TEST_ABBR}" "${TEST_NUMBER}" "${TEST_VERSION}"

# Return status code if sourced, exit if run standalone
${ORCHESTRATION:-false} && return "${EXIT_CODE}" || exit "${EXIT_CODE}"
