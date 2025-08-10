#!/bin/bash

# Test: Startup/Shutdown
# Tests startup and shutdown of the application with minimal and maximal configurations

# CHANGELOG
# 4.1.0 - 2025-08-08 - Reviewed. Changed JSON filenames. 
# 4.0.0 - 2025-07-30 - Overhaul #1
# 3.0.1 - 2025-07-06 - Added missing shellcheck justifications
# 3.0.0 - 2025-07-02 - Complete rewrite to use new modular test libraries
# 2.0.0 - 2025-06-17 - Major refactoring: fixed all shellcheck warnings, improved modularity, enhanced comments
# 1.0.0 - Initial version

# Test configuration
TEST_NAME="Startup/Shutdown"
TEST_ABBR="UPD"
TEST_NUMBER="21"
TEST_VERSION="4.1.0"

# shellcheck source=tests/lib/framework.sh # Reference framework directly
[[ -n "${FRAMEWORK_GUARD}" ]] || source "$(dirname "${BASH_SOURCE[0]}")/lib/framework.sh"
setup_test_environment

# Test configuration
MIN_CONFIG="${SCRIPT_DIR}/configs/hydrogen_test_21_min.json"
MAX_CONFIG="${SCRIPT_DIR}/configs/hydrogen_test_21_max.json"
STARTUP_TIMEOUT=10    # Seconds to wait for startup
SHUTDOWN_TIMEOUT=90   # Hard limit on shutdown time
SHUTDOWN_ACTIVITY_TIMEOUT=5  # Timeout if no new log activity
LOG_MIN="${LOGS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}_min.log"
LOG_MAX="${LOGS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}_max.log"

print_subtest "Locate Hydrogen Binary"

HYDROGEN_BIN=''
HYDROGEN_BIN_BASE=''
if find_hydrogen_binary "${PROJECT_DIR}"; then
    print_message "Using Hydrogen binary: ${HYDROGEN_BIN_BASE}"
    print_result 0 "Hydrogen binary found and validated"
    ((PASS_COUNT++))
else
    print_result 1 "Failed to find Hydrogen binary"
    EXIT_CODE=1
fi

print_subtest "Validate Minimum Configuration File"

if validate_config_file "${MIN_CONFIG}"; then
    ((PASS_COUNT++))
else
    EXIT_CODE=1
fi

# Test minimal configuration
config_name=$(basename "${MIN_CONFIG}" .json)
run_lifecycle_test "${MIN_CONFIG}" "${config_name}" "${DIAG_TEST_DIR}" "${STARTUP_TIMEOUT}" "${SHUTDOWN_TIMEOUT}" "${SHUTDOWN_ACTIVITY_TIMEOUT}" "${HYDROGEN_BIN}" "${LOG_MIN}" "PASS_COUNT" "EXIT_CODE"

print_subtest "Validate Maximum Configuration File"

if validate_config_file "${MAX_CONFIG}"; then
    ((PASS_COUNT++))
else
    EXIT_CODE=1
fi

# Test maximal configuration
config_name=$(basename "${MAX_CONFIG}" .json)
run_lifecycle_test "${MAX_CONFIG}" "${config_name}" "${DIAG_TEST_DIR}" "${STARTUP_TIMEOUT}" "${SHUTDOWN_TIMEOUT}" "${SHUTDOWN_ACTIVITY_TIMEOUT}" "${HYDROGEN_BIN}" "${LOG_MAX}" "PASS_COUNT" "EXIT_CODE"

# Print completion table
print_test_completion "${TEST_NAME}" "${TEST_ABBR}" "${TEST_NUMBER}" "${TEST_VERSION}"

# Return status code if sourced, exit if run standalone
${ORCHESTRATION:-false} && return "${EXIT_CODE}" || exit "${EXIT_CODE}"
