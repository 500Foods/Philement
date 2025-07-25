#!/bin/bash

# Test: JSON Error Handling
# Tests if the application correctly handles JSON syntax errors in configuration files
# and provides meaningful error messages with position information

# CHANGLEOG
# 3.0.2 - 2025-07-14 - Updated to use build/tests directories for test output consistency
# 3.0.1 - 2025-07-06 - Added missing shellcheck justifications
# 3.0.0 - 2025-07-02 - Complete rewrite to use new modular test libraries
# 2.0.0 - 2025-06-17 - Major refactoring: fixed all shellcheck warnings, improved modularity, enhanced comments
# 1.0.0 - Original version - Basic JSON error handling test

# Test configuration
TEST_NAME="JSON Error Handling"
SCRIPT_VERSION="3.0.2"

# Sort out directories
PROJECT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && cd .. && pwd )"
SCRIPT_DIR="${PROJECT_DIR}/tests"
LIB_DIR="${SCRIPT_DIR}/lib"
BUILD_DIR="${PROJECT_DIR}/build"
TESTS_DIR="${BUILD_DIR}/tests"
RESULTS_DIR="${TESTS_DIR}/results"
DIAGS_DIR="${TESTS_DIR}/diagnostics"
LOGS_DIR="${TESTS_DIR}/logs"
mkdir -p "${BUILD_DIR}" "${TESTS_DIR}" "${RESULTS_DIR}" "${DIAGS_DIR}" "${LOGS_DIR}"

# shellcheck source=tests/lib/framework.sh # Resolve path statically
[[ -n "${FRAMEWORK_GUARD}" ]] || source "${LIB_DIR}/framework.sh"
# shellcheck source=tests/lib/log_output.sh # Resolve path statically
[[ -n "${LOG_OUTPUT_GUARD}" ]] || source "${LIB_DIR}/log_output.sh"

# Test configuration
EXIT_CODE=0
TOTAL_SUBTESTS=4
PASS_COUNT=0
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
RESULT_LOG="${RESULTS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}.log"
ERROR_OUTPUT="${LOGS_DIR}/json_error_output_${TIMESTAMP}.log"

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
    print_message "Using Hydrogen binary: $(basename "${HYDROGEN_BIN}")"
    print_result 0 "Hydrogen binary found and validated"
    ((PASS_COUNT++))
else
    print_result 1 "Failed to find Hydrogen binary"
    EXIT_CODE=1
fi

# Test configuration with JSON syntax error (missing comma)
TEST_CONFIG=$(get_config_path "hydrogen_test_json.json")

# Validate configuration file exists
next_subtest
print_subtest "Validate Test Configuration File"
if validate_config_file "${TEST_CONFIG}"; then
    print_message "Using test file with JSON syntax error (missing comma)"
    ((PASS_COUNT++))
else
    EXIT_CODE=1
fi

# Test 1: Run hydrogen with malformed config - should fail
next_subtest
print_subtest "Launch with Malformed JSON Configuration"
print_message "Testing configuration: $(basename "${TEST_CONFIG}" .json)"
print_command "$(basename "${HYDROGEN_BIN}") $(basename "${TEST_CONFIG}")"

# Capture both stdout and stderr
if "${HYDROGEN_BIN}" "${TEST_CONFIG}" &> "${ERROR_OUTPUT}"; then
    print_result 1 "Application should have exited with an error but didn't"
    EXIT_CODE=1
else
    print_result 0 "Application exited with error as expected"
    ((PASS_COUNT++))
fi

# Test 2: Check error output for line and column information
next_subtest
print_subtest "Verify Error Message Contains Position Information"
print_message "Examining error output..."

if grep -q "line" "${ERROR_OUTPUT}" && grep -q "column" "${ERROR_OUTPUT}"; then
    print_result 0 "Error message contains line and column information"
    print_message "Error output:"
    # Display the error output content
    while IFS= read -r line; do
        print_output "${line}"
    done < <(tail -n 5 "${ERROR_OUTPUT}" || true)
    ((PASS_COUNT++))
else
    print_result 1 "Error message does not contain line and column information"
    print_message "Error output:"
    # Display the error output content
    while IFS= read -r line; do
        print_output "${line}"
    done < <(tail -n 5 "${ERROR_OUTPUT}" || true)
    EXIT_CODE=1
fi

# Save error output to results directory
cp "${ERROR_OUTPUT}" "${RESULTS_DIR}/json_error_full_${TIMESTAMP}.txt"
print_message "Full error output saved to: $(convert_to_relative_path "${RESULTS_DIR}/json_error_full_${TIMESTAMP}.txt" || true)"

# Clean up temporary error output file
rm -f "${ERROR_OUTPUT}"

# Export results for test_all.sh integration
# Derive test identifier from script filename for consistency with test_00_all.sh
TEST_IDENTIFIER=$(basename "${BASH_SOURCE[0]}" .sh | sed 's/test_[0-9]*_//')
export_subtest_results "${TEST_NUMBER}_${TEST_IDENTIFIER}" "${TOTAL_SUBTESTS}" "${PASS_COUNT}" "${TEST_NAME}" > /dev/null

# Print completion table
print_test_completion "${TEST_NAME}"

end_test "${EXIT_CODE}" "${TOTAL_SUBTESTS}" "${PASS_COUNT}" > /dev/null

# Return status code if sourced, exit if run standalone
if [[ "${ORCHESTRATION}" == "true" ]]; then
    return "${EXIT_CODE}"
else
    exit "${EXIT_CODE}"
fi
