#!/usr/bin/env bash
#
# Test: Shutdown
# Tests the shutdown functionality of the application with a minimal configuration
#
# CHANGELOG
# 4.0.0 - 2025-09-19 - Added server log output and elapsed time display in test name
# 3.1.0 - 2025-08-08 - Reviewed, updated logging conventions
# 3.0.0 - 2025-07-30 - Overhaul #1
# 2.0.1 - 2025-07-06 - Added missing shellcheck justifications
# 2.0.0 - 2025-07-02 - Complete rewrite to use new modular test libraries
# 1.1.0 - Enhanced with proper error handling, modular functions, and shellcheck compliance
# 1.0.0 - Initial version with basic shutdown testing

set -euo pipefail

# Test configuration
TEST_NAME="Shutdown"
TEST_ABBR="SHD"
TEST_NUMBER="16"
TEST_COUNTER=0
TEST_VERSION="4.0.0"

# shellcheck source=tests/lib/framework.sh # Reference framework directly
[[ -n "${FRAMEWORK_GUARD:-}" ]] || source "$(dirname "${BASH_SOURCE[0]}")/lib/framework.sh"
setup_test_environment

# Test configuration
MIN_CONFIG="${SCRIPT_DIR}/configs/hydrogen_test_${TEST_NUMBER}_shutdown.json"
config_name=$(basename "${MIN_CONFIG}" .json)
STARTUP_TIMEOUT=10    # Seconds to wait for startup
SHUTDOWN_TIMEOUT=90   # Hard limit on shutdown time
SHUTDOWN_ACTIVITY_TIMEOUT=5  # Timeout if no new log activity

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Locate Hydrogen Binary"

HYDROGEN_BIN=''
HYDROGEN_BIN_BASE=''
# shellcheck disable=SC2310 # We want to continue even if the test fails
if find_hydrogen_binary "${PROJECT_DIR}"; then
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Using Hydrogen binary: ${HYDROGEN_BIN_BASE}"
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Hydrogen binary found and validated"
else
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Failed to find Hydrogen binary"
    EXIT_CODE=1
fi

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Validate Configuration File"

# shellcheck disable=SC2310 # We want to continue even if the test fails
if validate_config_file "${MIN_CONFIG}"; then
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Validated configuration file"
else
    EXIT_CODE=1
fi

# Run cycle - that's all this test is for
run_lifecycle_test "${MIN_CONFIG}" "${config_name}" "${DIAG_TEST_DIR}" "${STARTUP_TIMEOUT}" "${SHUTDOWN_TIMEOUT}" "${SHUTDOWN_ACTIVITY_TIMEOUT}" "${HYDROGEN_BIN}" "${LOG_FILE}" "PASS_COUNT" "EXIT_CODE"

# Add server log to output
print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Server Log: ..${LOG_FILE}"

# Extract elapsed time from log and update test name
if [[ -f "${LOG_FILE}" ]]; then
    # Extract "Total elapsed time" from log file
    elapsed_time=$("${AWK}" "/Total elapsed time:/ {print \$NF}" "${LOG_FILE}" 2>/dev/null | tail -1 || true)
    if [[ -n "${elapsed_time}" ]]; then
        TEST_NAME="${TEST_NAME}  {BLUE}cycle: ${elapsed_time}{RESET}"
    fi
fi

# Print completion table
print_test_completion "${TEST_NAME}" "${TEST_ABBR}" "${TEST_NUMBER}" "${TEST_VERSION}"

# Return status code if sourced, exit if run standalone
${ORCHESTRATION:-false} && return "${EXIT_CODE}" || exit "${EXIT_CODE}"
