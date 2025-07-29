#!/bin/bash

# Test: Environment Variables
# Tests environment variable handling 
# Validates that required environment variables for PAYLOAD_LOCK, PAYLOAD_KY and WEBSOCKET_KEY are set

# CHANGELOG
# 4.0.0 - 2025-07-28 - Shellcheck fixs, added WEBSOCKET_KEY check, streamlined tests, renamed to secrets
# 3.0.2 - 2025-07-14 - Updated to use build/tests directories for test output consistency
# 3.0.1 - 2025-07-06 - Added missing shellcheck justifications
# 3.0.0 - 2025-07-02 - Complete rewrite to use new modular test libraries
# 2.0.0 - 2025-06-17 - Major refactoring: improved modularity, enhanced comments, added version tracking
# 1.0.0 - Initial version - Basic environment variable validation

# Test configuration
TEST_NAME="Environment Variables"
SCRIPT_VERSION="4.0.0"

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
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
EXIT_CODE=0
PASS_COUNT=0
TEST_NUMBER=$(extract_test_number "${BASH_SOURCE[0]}")
set_test_number "${TEST_NUMBER}"
reset_subtest_counter

# Set up directories for this test run
RESULT_LOG="${RESULTS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}.log"
# LOG_FILE="${LOGS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}.log"
# DIAG_TEST_DIR="${DIAGS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}"
# mkdir -p "${DIAG_TEST_DIR}"

# Print beautiful test header
print_test_header "${TEST_NAME}" "${SCRIPT_VERSION}"

# Print framework and log output versions as they are already sourced
[[ -n "${ORCHESTRATION}" ]] || print_message "${FRAMEWORK_NAME} ${FRAMEWORK_VERSION}" "info"
[[ -n "${ORCHESTRATION}" ]] || print_message "${LOG_OUTPUT_NAME} ${LOG_OUTPUT_VERSION}" "info"
# shellcheck source=tests/lib/env_utils.sh # Resolve path statically
[[ -n "${ENV_UTILS_GUARD}" ]] || source "${LIB_DIR}/env_utils.sh"

# Subtest: Check PAYLOAD_LOCK
next_subtest
print_subtest "Payload LOCK Ennvironment Variable Present"

if check_env_var "PAYLOAD_LOCK" "${PAYLOAD_LOCK}"; then
    print_result 0 "PAYLOAD_LOCK Found"
    ((PASS_COUNT++))
else
    print_result 1 "PAYLOAD_LOCK Missing or Invalid"
    EXIT_CODE=1
fi

# Subtest: RSA Key Format Validation
next_subtest
print_subtest "RSA Key Format Validation For Payload Lock"

if [[ -n "${PAYLOAD_LOCK}" ]]; then
    print_command "echo \"\${PAYLOAD_LOCK}\" | base64 -d | openssl pkey -pubin -check -noout"
    if validate_rsa_key "PAYLOAD_LOCK" "${PAYLOAD_LOCK}" "public"; then
        print_result 0 "PAYLOAD_LOCK valid RSA Pulic Key Format"
        ((PASS_COUNT++))
    else
        print_result 1 "PAYLOAD_LOCK not vlaid RSA Public Key Format"
        EXIT_CODE=1
    fi
fi

# Subtest: Check PAYLOAD_KEY
next_subtest
print_subtest "Payload KEY Ennvironment Variable Present"

if check_env_var "PAYLOAD_KEY" "${PAYLOAD_KEY}"; then
    print_result 0 "PAYLOAD_KEY Found"
    ((PASS_COUNT++))
else
    print_result 1 "PAYLOAD_KEY Missing or Invalid"
    EXIT_CODE=1
fi

# Subtest: RSA Key Format Validation
next_subtest
print_subtest "RSA Key Format Validation For Payload Lock"

if [[ -n "${PAYLOAD_KEY}" ]]; then
    print_command "echo \"\${PAYLOAD_KEY}\" | base64 -d | openssl rsa -check -noout"
    if validate_rsa_key "PAYLOAD_KEY" "${PAYLOAD_KEY}" "private"; then
        print_result 0 "PAYLOAD_KEY valid RSA Key Format"
        ((PASS_COUNT++))
    else
        print_result 1 "PAYLOAD_KEY not vlaid RSA Key Format"
        EXIT_CODE=1
    fi
fi

# Subtest: Check WEBSOCKET_KEY
next_subtest
print_subtest "WebSocket KEY Ennvironment Variable Present"

if check_env_var "WEBSOCKET_KEY" "${PAYLOAD_KEY}"; then
    print_result 0 "WEBSOCKET_KEY Found"
    ((PASS_COUNT++))
else
    print_result 1 "WEBSOCKET_KEY Missing or Invalid"
    EXIT_CODE=1
fi

# Print completion table
print_test_completion "${TEST_NAME}"

# Return status code if sourced, exit if run standalone
if [[ "${ORCHESTRATION}" == "true" ]]; then
    return "${EXIT_CODE}"
else
    exit "${EXIT_CODE}"
fi
