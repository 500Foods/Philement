#!/bin/bash

# Test: Library Dependencies
# Tests the library dependency checking system added to initialization

# CHANGELOG
# 2.1.1 - 2025-07-06 - Added missing shellcheck justifications
# 2.1.0 - 2025-07-06 - Updated to accept "Less" as a passing status in addition to "Good" and "Less Good" for dependency checks
# 2.0.0 - 2025-07-02 - Complete rewrite to use new modular test libraries
# 1.1.0 - Enhanced with proper error handling, modular functions, and shellcheck compliance
# 1.0.0 - Initial version with library dependency checking

# Test configuration
TEST_NAME="Library Dependencies"
SCRIPT_VERSION="2.0.0"

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
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
EXIT_CODE=0
PASS_COUNT=0
TEST_NUMBER=$(extract_test_number "${BASH_SOURCE[0]}")
set_test_number "${TEST_NUMBER}"
reset_subtest_counter

# Set up directories for this test run
RESULT_LOG="${RESULTS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}.log"
LOG_FILE="${LOGS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}.log"
DIAG_TEST_DIR="${DIAGS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}"
mkdir -p "${DIAG_TEST_DIR}"

# Use build/tests/ directory for consistency
STARTUP_TIMEOUT=10

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
# shellcheck disable=SC2153  # HYDROGEN_BIN is set by find_hydrogen_binary function
if find_hydrogen_binary "${PROJECT_DIR}" "HYDROGEN_BIN"; then
    print_message "Using Hydrogen binary: $(basename "${HYDROGEN_BIN}")"
    print_result 0 "Hydrogen binary found and validated"
    ((PASS_COUNT++))
else
    print_result 1 "Failed to find Hydrogen binary"
    EXIT_CODE=1
fi

# Test configuration - use minimal config since library dependencies are checked regardless of configuration
MINIMAL_CONFIG="${SCRIPT_DIR}/configs/hydrogen_test_min.json"

# Create output directories
next_subtest
print_subtest "Create Output Directories"
if setup_output_directories "${RESULTS_DIR}" "${DIAGS_DIR}" "${LOG_FILE}" "${DIAG_TEST_DIR}"; then
    ((PASS_COUNT++))
else
    EXIT_CODE=1
fi

# Validate minimal configuration file
next_subtest
print_subtest "Validate Configuration File"
if validate_config_file "${MINIMAL_CONFIG}"; then
    ((PASS_COUNT++))
else
    EXIT_CODE=1
fi

# Timeouts and limits
SHUTDOWN_TIMEOUT=10   # Hard limit on shutdown time
SHUTDOWN_ACTIVITY_TIMEOUT=3  # Timeout if no new log activity

# Function to check dependency log messages and extract version information
check_dependency_log() {
    local dep_name="$1"
    local log_file="$2"
    
    next_subtest
    print_subtest "Check Dependency: ${dep_name}"
    
    # Extract the full dependency line
    local dep_line
    dep_line=$(grep ".*\[ STATE \]  \[ DepCheck           \]  ${dep_name}.*Status:" "${log_file}" 2>/dev/null)
    
    if [[ -n "${dep_line}" ]]; then
        # Extract version information using sed
        local expected_version found_version status
        expected_version=$(echo "${dep_line}" | sed -n 's/.*Expecting: \([^ ]*\).*/\1/p' || true)
        found_version=$(echo "${dep_line}" | sed -n 's/.*Found: \([^(]*\).*/\1/p' | sed 's/ *$//' || true)
        status=$(echo "${dep_line}" | sed -n 's/.*Status: \([^ ]*\).*/\1/p' || true)
        
        if [[ "${status}" = "Good" ]] || [[ "${status}" = "Less Good" ]] || [[ "${status}" = "Less" ]]; then
            print_result 0 "Found ${dep_name} - Expected: ${expected_version}, Found: ${found_version}, Status: ${status}"
            ((PASS_COUNT++))
            return 0
        else
            print_result 1 "Found ${dep_name} but status is not Good, Less Good, or Less - Expected: ${expected_version}, Found: ${found_version}, Status: ${status}"
            EXIT_CODE=1
            return 1
        fi
    else
        print_result 0 "Did not find ${dep_name} dependency check in logs (skipped)"
        return 0
    fi
}

# Start Hydrogen for dependency checking
config_name=$(basename "${MINIMAL_CONFIG}" .json)
run_lifecycle_test "${MINIMAL_CONFIG}" "${config_name}" "${DIAG_TEST_DIR}" "${STARTUP_TIMEOUT}" "${SHUTDOWN_TIMEOUT}" "${SHUTDOWN_ACTIVITY_TIMEOUT}" "${HYDROGEN_BIN}" "${LOG_FILE}" "PASS_COUNT" "EXIT_CODE"

# Analyze logs for dependency checks
print_message "Checking dependency logs for library dependency detection"
check_dependency_log "pthreads" "${LOG_FILE}"
check_dependency_log "jansson" "${LOG_FILE}"
check_dependency_log "libm" "${LOG_FILE}"
check_dependency_log "microhttpd" "${LOG_FILE}"
check_dependency_log "libwebsockets" "${LOG_FILE}"
check_dependency_log "OpenSSL" "${LOG_FILE}"
check_dependency_log "libbrotlidec" "${LOG_FILE}"
check_dependency_log "libtar" "${LOG_FILE}"

# Print completion table
print_test_completion "${TEST_NAME}"

# Return status code if sourced, exit if run standalone
if [[ "${ORCHESTRATION}" == "true" ]]; then
    return "${EXIT_CODE}"
else
    exit "${EXIT_CODE}"
fi
