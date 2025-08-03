#!/bin/bash

# Test: Startup/Shutdown
# Tests startup and shutdown of the application with minimal and maximal configurations

# CHANGELOG
# 4.0.0 - 2025-07-30 - Overhaul #1
# 3.0.1 - 2025-07-06 - Added missing shellcheck justifications
# 3.0.0 - 2025-07-02 - Complete rewrite to use new modular test libraries
# 2.0.0 - 2025-06-17 - Major refactoring: fixed all shellcheck warnings, improved modularity, enhanced comments
# 1.0.0 - Initial version

# Test configuration
TEST_NAME="Startup/Shutdown"
TEST_ABBR="UPD"
TEST_NUMBER="22"
TEST_VERSION="4.0.0"

# shellcheck source=tests/lib/framework.sh # Reference framework directly
[[ -n "${FRAMEWORK_GUARD}" ]] || source "$(dirname "${BASH_SOURCE[0]}")/lib/framework.sh"
setup_test_environment

print_subtest "Locate Hydrogen Binary"

# shellcheck disable=SC2153  # HYDROGEN_BIN is set by find_hydrogen_binary function
if find_hydrogen_binary "${PROJECT_DIR}" "HYDROGEN_BIN"; then
    print_message "Using Hydrogen binary: $(basename "${HYDROGEN_BIN}")"
    print_result 0 "Hydrogen binary found and validated"
    ((PASS_COUNT++))
else
    print_result 1 "Failed to find Hydrogen binary"
    EXIT_CODE=1
fi

# Test configurations
MIN_CONFIG="${SCRIPT_DIR}/configs/hydrogen_test_min.json"
MAX_CONFIG="${SCRIPT_DIR}/configs/hydrogen_test_max.json"

print_subtest "Validate Configuration Files"

if validate_config_files "${MIN_CONFIG}" "${MAX_CONFIG}"; then
    ((PASS_COUNT++))
else
    EXIT_CODE=1
fi

# Timeouts and limits
STARTUP_TIMEOUT=10    # Seconds to wait for startup
SHUTDOWN_TIMEOUT=90   # Hard limit on shutdown time
SHUTDOWN_ACTIVITY_TIMEOUT=5  # Timeout if no new log activity

# Output files and directories - always use build/tests/ for consistency
BUILD_DIR="${SCRIPT_DIR}/../build"
LOG_FILE="${BUILD_DIR}/tests/logs/hydrogen_test.log"
DIAGS_DIR="${BUILD_DIR}/tests/diagnostics"
DIAG_TEST_DIR="${DIAGS_DIR}/startup_shutdown_test_${TIMESTAMP}"

print_subtest "Create Output Directories"

if setup_output_directories "${RESULTS_DIR}" "${DIAGS_DIR}" "${LOG_FILE}" "${DIAG_TEST_DIR}"; then
    ((PASS_COUNT++))
else
    EXIT_CODE=1
fi

# Test minimal configuration
config_name=$(basename "${MIN_CONFIG}" .json)
run_lifecycle_test "${MIN_CONFIG}" "${config_name}" "${DIAG_TEST_DIR}" "${STARTUP_TIMEOUT}" "${SHUTDOWN_TIMEOUT}" "${SHUTDOWN_ACTIVITY_TIMEOUT}" "${HYDROGEN_BIN}" "${LOG_FILE}" "PASS_COUNT" "EXIT_CODE"

# Test maximal configuration
config_name=$(basename "${MAX_CONFIG}" .json)
run_lifecycle_test "${MAX_CONFIG}" "${config_name}" "${DIAG_TEST_DIR}" "${STARTUP_TIMEOUT}" "${SHUTDOWN_TIMEOUT}" "${SHUTDOWN_ACTIVITY_TIMEOUT}" "${HYDROGEN_BIN}" "${LOG_FILE}" "PASS_COUNT" "EXIT_CODE"

# Print completion table
print_test_completion "${TEST_NAME}" "${TEST_ABBR}" "${TEST_NUMBER}" "${TEST_VERSION}"

# Return status code if sourced, exit if run standalone
${ORCHESTRATION:-false} && return "${EXIT_CODE}" || exit "${EXIT_CODE}"
