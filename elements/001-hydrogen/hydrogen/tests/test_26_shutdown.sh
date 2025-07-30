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


# Sort out directories
PROJECT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && cd .. && pwd )"
SCRIPT_DIR="${PROJECT_DIR}/tests"
LIB_DIR="${SCRIPT_DIR}/lib"
BUILD_DIR="${PROJECT_DIR}/build"
TESTS_DIR="${BUILD_DIR}/tests"
RESULTS_DIR="${TESTS_DIR}/results"
DIAGS_DIR="${TESTS_DIR}/diagnostics"
LOGS_DIR="${TESTS_DIR}/logs"
# CONFIG_DIR="${SCRIPT_DIR}/configs"
mkdir -p "${BUILD_DIR}" "${TESTS_DIR}" "${RESULTS_DIR}" "${DIAGS_DIR}" "${LOGS_DIR}"

# shellcheck source=tests/lib/framework.sh # Resolve path statically
[[ -n "${FRAMEWORK_GUARD}" ]] || source "${LIB_DIR}/framework.sh"
# shellcheck source=tests/lib/log_output.sh # Resolve path statically
[[ -n "${LOG_OUTPUT_GUARD}" ]] || source "${LIB_DIR}/log_output.sh"

# Test configuration
EXIT_CODE=0
PASS_COUNT=0
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
RESULT_LOG="${RESULTS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}.log"
# ERROR_OUTPUT="${LOGS_DIR}/json_error_output_${TIMESTAMP}.log"

# Auto-extract test number and set up environment
TEST_NUMBER=$(extract_test_number "${BASH_SOURCE[0]}")
set_test_number "${TEST_NUMBER}"
reset_subtest_counter

# Print beautiful test header
print_test_header "${TEST_NAME}" "${SCRIPT_VERSION}"

# Print framework and log output versions as they are already sourced
[[ -n "${ORCHESTRATION}" ]] || print_message "${FRAMEWORK_NAME} ${FRAMEWORK_VERSION}" "info"
[[ -n "${ORCHESTRATION}" ]] || print_message "${LOG_OUTPUT_NAME} ${LOG_OUTPUT_VERSION}" "info"
# shellcheck source=tests/lib/lifecycle.sh # Resolve path statically
[[ -n "${LIFECYCLE_GUARD}" ]] || source "${LIB_DIR}/lifecycle.sh"
# shellcheck source=tests/lib/file_utils.sh # Resolve path statically
[[ -n "${FILE_UTILS_GUARD}" ]] || source "${LIB_DIR}/file_utils.sh"

# Navigate to the project root (one level up from tests directory)
if ! navigate_to_project_root "${SCRIPT_DIR}"; then
    print_error "Failed to navigate to project root directory"
    exit 1
fi

# Validate Hydrogen Binary
next_subtest
print_subtest "Locate Hydrogen Binary"
HYDROGEN_DIR="$( cd "${SCRIPT_DIR}/.." && pwd )"
# shellcheck disable=SC2153  # HYDROGEN_BIN is set by find_hydrogen_binary function
if find_hydrogen_binary "${HYDROGEN_DIR}" "HYDROGEN_BIN"; then
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
print_test_completion "${TEST_NAME}"

# Return status code if sourced, exit if run standalone
if [[ "${ORCHESTRATION}" == "true" ]]; then
    return "${EXIT_CODE}"
else
    exit "${EXIT_CODE}"
fi
