#!/bin/bash

# Test: Environment Variables
# Tests environment variable handling 
# Validates that required environment variables for PAYLOAD_LOCK, PAYLOAD_KY and WEBSOCKET_KEY are set

# CHANGELOG
# 5.0.0 - 2025-07-30 - Overhaul #1
# 4.0.0 - 2025-07-28 - Shellcheck fixs, added WEBSOCKET_KEY check, streamlined tests, renamed to secrets
# 3.0.2 - 2025-07-14 - Updated to use build/tests directories for test output consistency
# 3.0.1 - 2025-07-06 - Added missing shellcheck justifications
# 3.0.0 - 2025-07-02 - Complete rewrite to use new modular test libraries
# 2.0.0 - 2025-06-17 - Major refactoring: improved modularity, enhanced comments, added version tracking
# 1.0.0 - Initial version - Basic environment variable validation

# Test configuration
TEST_NAME="Env Vars and Secrets"
TEST_ABBR="ENV"
TEST_NUMBER="14"
TEST_VERSION="5.0.0"

# shellcheck source=tests/lib/framework.sh # Reference framework directly
[[ -n "${FRAMEWORK_GUARD}" ]] || source "$(dirname "${BASH_SOURCE[0]}")/lib/framework.sh"
setup_test_environment

print_subtest "Payload LOCK Ennvironment Variable Present"

if check_env_var "PAYLOAD_LOCK" "${PAYLOAD_LOCK}"; then
    print_result 0 "PAYLOAD_LOCK Found"
    ((PASS_COUNT++))
else
    print_result 1 "PAYLOAD_LOCK Missing or Invalid"
    EXIT_CODE=1
fi

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

print_subtest "Payload KEY Ennvironment Variable Present"

if check_env_var "PAYLOAD_KEY" "${PAYLOAD_KEY}"; then
    print_result 0 "PAYLOAD_KEY Found"
    ((PASS_COUNT++))
else
    print_result 1 "PAYLOAD_KEY Missing or Invalid"
    EXIT_CODE=1
fi

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

print_subtest "WebSocket KEY Ennvironment Variable Present"

if check_env_var "WEBSOCKET_KEY" "${PAYLOAD_KEY}"; then
    print_result 0 "WEBSOCKET_KEY Found"
    ((PASS_COUNT++))
else
    print_result 1 "WEBSOCKET_KEY Missing or Invalid"
    EXIT_CODE=1
fi

# Print completion table
print_test_completion "${TEST_NAME}" "${TEST_ABBR}" "${TEST_NUMBER}" "${TEST_VERSION}" 

# Return status code if sourced, exit if run standalone
${ORCHESTRATION:-false} && return "${EXIT_CODE}" || exit "${EXIT_CODE}"
