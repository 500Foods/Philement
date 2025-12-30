#!/usr/bin/env bash

# Test: Startup/Shutdown
# Tests startup and shutdown of the application with minimal and maximal configurations

# CHANGELOG
# 5.0.0 - 2025-09-19 - Added server log output and elapsed time display in test name for both min/max configs
# 4.1.0 - 2025-08-08 - Reviewed. Changed JSON filenames.
# 4.0.0 - 2025-07-30 - Overhaul #1
# 3.0.1 - 2025-07-06 - Added missing shellcheck justifications
# 3.0.0 - 2025-07-02 - Complete rewrite to use new modular test libraries
# 2.0.0 - 2025-06-17 - Major refactoring: fixed all shellcheck warnings, improved modularity, enhanced comments
# 1.0.0 - Initial version

set -euo pipefail

# Test configuration
TEST_NAME="Startup/Shutdown"
TEST_ABBR="UPD"
TEST_NUMBER="17"
TEST_COUNTER=0
TEST_VERSION="5.0.0"

# shellcheck source=tests/lib/framework.sh # Reference framework directly
[[ -n "${FRAMEWORK_GUAR:-}" ]] || source "$(dirname "${BASH_SOURCE[0]}")/lib/framework.sh"
setup_test_environment

# Test configuration
MIN_CONFIG="${SCRIPT_DIR}/configs/hydrogen_test_${TEST_NUMBER}_startup_min.json"
MAX_CONFIG="${SCRIPT_DIR}/configs/hydrogen_test_${TEST_NUMBER}_startup_max.json"
STARTUP_TIMEOUT=10    # Seconds to wait for startup
SHUTDOWN_TIMEOUT=90   # Hard limit on shutdown time
SHUTDOWN_ACTIVITY_TIMEOUT=5  # Timeout if no new log activity
LOG_MIN="${LOGS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}_min.log"
LOG_MAX="${LOGS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}_max.log"

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

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Validate Minimum Configuration File"

# shellcheck disable=SC2310 # We want to continue even if the test fails
if validate_config_file "${MIN_CONFIG}"; then
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Validated Minimum Configuration File"
else
    EXIT_CODE=1
fi

# Test minimal configuration
config_name=$(basename "${MIN_CONFIG}" .json)
run_lifecycle_test "${MIN_CONFIG}" "${config_name}" "${DIAG_TEST_DIR}" "${STARTUP_TIMEOUT}" "${SHUTDOWN_TIMEOUT}" "${SHUTDOWN_ACTIVITY_TIMEOUT}" "${HYDROGEN_BIN}" "${LOG_MIN}" "PASS_COUNT" "EXIT_CODE"

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Validate Maximum Configuration File"

# shellcheck disable=SC2310 # We want to continue even if the test fails
if validate_config_file "${MAX_CONFIG}"; then
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Validated Maximum Configuration File"
else
    EXIT_CODE=1
fi

# Test maximal configuration
config_name=$(basename "${MAX_CONFIG}" .json)
run_lifecycle_test "${MAX_CONFIG}" "${config_name}" "${DIAG_TEST_DIR}" "${STARTUP_TIMEOUT}" "${SHUTDOWN_TIMEOUT}" "${SHUTDOWN_ACTIVITY_TIMEOUT}" "${HYDROGEN_BIN}" "${LOG_MAX}" "PASS_COUNT" "EXIT_CODE"

# Add server logs to output
print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Min Config Server Log: ..${LOG_MIN}"
print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Max Config Server Log: ..${LOG_MAX}"

# Extract elapsed times from logs and update test name
min_elapsed="" max_elapsed=""
if [[ -f "${LOG_MIN}" ]]; then
    min_elapsed=$("${AWK}" "/Total elapsed time:/ {print \$NF}" "${LOG_MIN}" 2>/dev/null | tail -1 || true)
fi
if [[ -f "${LOG_MAX}" ]]; then
    max_elapsed=$("${AWK}" "/Total elapsed time:/ {print \$NF}" "${LOG_MAX}" 2>/dev/null | tail -1 || true)
fi

# Update test name with timing info
if [[ -n "${min_elapsed}" || -n "${max_elapsed}" ]]; then
    timing_info=""
    if [[ -n "${min_elapsed}" ]]; then
        timing_info="${min_elapsed} / "
    fi
    if [[ -n "${max_elapsed}" ]]; then
        if [[ -n "${timing_info}" ]]; then
            timing_info="${timing_info} ${max_elapsed}"
        else
            timing_info="Max: ${max_elapsed}"
        fi
    fi
    TEST_NAME="${TEST_NAME}  {BLUE}cycle: ${timing_info}{RESET}"
fi

# Print completion table
print_test_completion "${TEST_NAME}" "${TEST_ABBR}" "${TEST_NUMBER}" "${TEST_VERSION}"

# Return status code if sourced, exit if run standalone
${ORCHESTRATION:-false} && return "${EXIT_CODE}" || exit "${EXIT_CODE}"
