#!/bin/bash

# Test: Payload Env Vars
# Tests environment variable handling for the payload system
# Validates that required payload environment variables are present and valid

# CHANGELOG
# 3.0.2 - 2025-07-14 - Updated to use build/tests directories for test output consistency
# 3.0.1 - 2025-07-06 - Added missing shellcheck justifications
# 3.0.0 - 2025-07-02 - Complete rewrite to use new modular test libraries
# 2.0.0 - 2025-06-17 - Major refactoring: improved modularity, enhanced comments, added version tracking
# 1.0.0 - Initial version - Basic environment variable validation

# Test configuration
TEST_NAME="Payload Env Vars"
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
TOTAL_SUBTESTS=2
PASS_COUNT=0
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
RESULT_LOG="${RESULTS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}.log"

# Auto-extract test number and set up environment
TEST_NUMBER=$(extract_test_number "${BASH_SOURCE[0]}")
set_test_number "${TEST_NUMBER}"
reset_subtest_counter

# Print beautiful test header
print_test_header "${TEST_NAME}" "${SCRIPT_VERSION}"

# Print framework and log output versions as they are already sourced
[[ -n "${ORCHESTRATION}" ]] || print_message "${FRAMEWORK_NAME} ${FRAMEWORK_VERSION}" "info"
[[ -n "${ORCHESTRATION}" ]] || print_message "${LOG_OUTPUT_NAME} ${LOG_OUTPUT_VERSION}" "info"
# shellcheck source=tests/lib/env_utils.sh # Resolve path statically
[[ -n "${ENV_UTILS_GUARD}" ]] || source "${LIB_DIR}/env_utils.sh"

# Subtest: Environment Variables Present
next_subtest
print_subtest "Environment Variables Present"

test_env_vars_present() {
    local failed_checks=0
    local passed_checks=0

    # Check PAYLOAD_KEY
    if check_env_var "PAYLOAD_KEY" "${PAYLOAD_KEY}"; then
        ((passed_checks++))
    else
        ((failed_checks++))
    fi

    # Check PAYLOAD_LOCK
    if check_env_var "PAYLOAD_LOCK" "${PAYLOAD_LOCK}"; then
        ((passed_checks++))
    else
        ((failed_checks++))
    fi

    # Evaluate test result
    evaluate_test_result "Environment variables present" "${failed_checks}" "PASS_COUNT" "EXIT_CODE"
}

test_env_vars_present

# Subtest: RSA Key Format Validation
next_subtest
print_subtest "RSA Key Format Validation"

test_rsa_key_format() {
    local failed_checks=0
    local passed_checks=0

    # Validate PAYLOAD_KEY (private key)
    if [[ -n "${PAYLOAD_KEY}" ]]; then
        print_command "echo \"\${PAYLOAD_KEY}\" | base64 -d | openssl rsa -check -noout"
        if validate_rsa_key "PAYLOAD_KEY" "${PAYLOAD_KEY}" "private"; then
            ((passed_checks++))
        else
            ((failed_checks++))
        fi
    fi

    # Validate PAYLOAD_LOCK (public key)
    if [[ -n "${PAYLOAD_LOCK}" ]]; then
        print_command "echo \"\${PAYLOAD_LOCK}\" | base64 -d | openssl rsa -pubin -noout"
        if validate_rsa_key "PAYLOAD_LOCK" "${PAYLOAD_LOCK}" "public"; then
            ((passed_checks++))
        else
            ((failed_checks++))
        fi
    fi

    # Evaluate test result
    evaluate_test_result "RSA key format validation" "${failed_checks}" "PASS_COUNT" "EXIT_CODE"
}

test_rsa_key_format

# Export results for test_all.sh integration
# Derive test name from script filename for consistency with test_00_all.sh
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
