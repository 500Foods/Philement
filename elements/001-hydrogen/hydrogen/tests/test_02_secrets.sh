#!/usr/bin/env bash

# Test: Environment Variables
# Tests environment variable handling
# Validates that required environment variables for PAYLOAD_LOCK, PAYLOAD_KEY, WEBSOCKET_KEY, HYDROGEN_INSTALLER_LOCK and HYDROGEN_INSTALLER_KEY are set

# CHANGELOG
# 5.1.0 - 2025-09-06 - Added HYDROGEN_INSTALLER_LOCK, HYDROGEN_INSTALLER_KEY validation
# 5.0.0 - 2025-07-30 - Overhaul #1
# 4.0.0 - 2025-07-28 - Shellcheck fixs, added WEBSOCKET_KEY check, streamlined tests, renamed to secrets
# 3.0.2 - 2025-07-14 - Updated to use build/tests directories for test output consistency
# 3.0.1 - 2025-07-06 - Added missing shellcheck justifications
# 3.0.0 - 2025-07-02 - Complete rewrite to use new modular test libraries
# 2.0.0 - 2025-06-17 - Major refactoring: improved modularity, enhanced comments, added version tracking
# 1.0.0 - Initial version - Basic environment variable validation

set -euo pipefail

# Test configuration
TEST_NAME="Environment Variables"
TEST_ABBR="ENV"
TEST_NUMBER="02"
TEST_COUNTER=0
TEST_VERSION="5.1.0"

# shellcheck source=tests/lib/framework.sh # Reference framework directly
[[ -n "${FRAMEWORK_GUARD:-}" ]] || source "$(dirname "${BASH_SOURCE[0]}")/lib/framework.sh"
setup_test_environment

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Payload LOCK Environment Variable Present"

# shellcheck disable=SC2310 # We want to continue even if the test fails
if check_env_var "PAYLOAD_LOCK" "${PAYLOAD_LOCK}"; then
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "PAYLOAD_LOCK Found"
else
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "PAYLOAD_LOCK Missing or Invalid"
    EXIT_CODE=1
fi

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "RSA Key Format Validation For Payload Lock"

if [[ -n "${PAYLOAD_LOCK}" ]]; then
    print_command "${TEST_NUMBER}" "${TEST_COUNTER}" "echo \"\${PAYLOAD_LOCK}\" | base64 -d | openssl pkey -pubin -check -noout"
    # shellcheck disable=SC2310 # We want to continue even if the test fails
    if validate_rsa_key "PAYLOAD_LOCK" "${PAYLOAD_LOCK}" "public"; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "PAYLOAD_LOCK valid RSA Public Key Format"
    else
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "PAYLOAD_LOCK not valid RSA Public Key Format"
        EXIT_CODE=1
    fi
fi

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Payload KEY Environment Variable Present"

# shellcheck disable=SC2310 # We want to continue even if the test fails
if check_env_var "PAYLOAD_KEY" "${PAYLOAD_KEY}"; then
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "PAYLOAD_KEY Found"
else
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "PAYLOAD_KEY Missing or Invalid"
    EXIT_CODE=1
fi

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "RSA Key Format Validation For Payload Key"

if [[ -n "${PAYLOAD_KEY}" ]]; then
    print_command "${TEST_NUMBER}" "${TEST_COUNTER}" "echo \"\${PAYLOAD_KEY}\" | base64 -d | openssl rsa -check -noout"
    # shellcheck disable=SC2310 # We want to continue even if the test fails
    if validate_rsa_key "PAYLOAD_KEY" "${PAYLOAD_KEY}" "private"; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "PAYLOAD_KEY valid RSA Key Format"
    else
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "PAYLOAD_KEY not valid RSA Key Format"
        EXIT_CODE=1
    fi
fi

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Hydrogen Installer LOCK Environment Variable Present"

# shellcheck disable=SC2310 # We want to continue even if the test fails
if check_env_var "HYDROGEN_INSTALLER_LOCK" "${HYDROGEN_INSTALLER_LOCK}"; then
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "HYDROGEN_INSTALLER_LOCK Found"
else
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "HYDROGEN_INSTALLER_LOCK Missing or Invalid"
    EXIT_CODE=1
fi

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "RSA Key Format Validation For Hydrogen Installer Lock"

if [[ -n "${HYDROGEN_INSTALLER_LOCK}" ]]; then
    print_command "${TEST_NUMBER}" "${TEST_COUNTER}" "echo \"\${HYDROGEN_INSTALLER_LOCK}\" | base64 -d | openssl pkey -pubin -check -noout"
    # shellcheck disable=SC2310 # We want to continue even if the test fails
    if validate_rsa_key "HYDROGEN_INSTALLER_LOCK" "${HYDROGEN_INSTALLER_LOCK}" "public"; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "HYDROGEN_INSTALLER_LOCK valid RSA Public Key Format"
    else
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "HYDROGEN_INSTALLER_LOCK not valid RSA Public Key Format"
        EXIT_CODE=1
    fi
fi

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Hydrogen Installer KEY Environment Variable Present"

# shellcheck disable=SC2310 # We want to continue even if the test fails
if check_env_var "HYDROGEN_INSTALLER_KEY" "${HYDROGEN_INSTALLER_KEY}"; then
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "HYDROGEN_INSTALLER_KEY Found"
else
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "HYDROGEN_INSTALLER_KEY Missing or Invalid"
    EXIT_CODE=1
fi

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "RSA Key Format Validation For Hydrogen Installer Key"

if [[ -n "${HYDROGEN_INSTALLER_KEY}" ]]; then
    print_command "${TEST_NUMBER}" "${TEST_COUNTER}" "echo \"\${HYDROGEN_INSTALLER_KEY}\" | base64 -d | openssl rsa -check -noout"
    # shellcheck disable=SC2310 # We want to continue even if the test fails
    if validate_rsa_key "HYDROGEN_INSTALLER_KEY" "${HYDROGEN_INSTALLER_KEY}" "private"; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "HYDROGEN_INSTALLER_KEY valid RSA Key Format"
    else
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "HYDROGEN_INSTALLER_KEY not valid RSA Key Format"
        EXIT_CODE=1
    fi
fi

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "WebSocket KEY Environment Variable Present"

# shellcheck disable=SC2310 # We want to continue even if the test fails
if [[ -n "${WEBSOCKET_KEY}" ]];  then
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "WEBSOCKET_KEY Found"
else
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "WEBSOCKET_KEY Missing or Invalid"
    EXIT_CODE=1
fi

TEST_NAME="${TEST_NAME}  {BLUE}secrets: 5{RESET}"

# Print completion table
print_test_completion "${TEST_NAME}" "${TEST_ABBR}" "${TEST_NUMBER}" "${TEST_VERSION}" 

# Return status code if sourced, exit if run standalone
${ORCHESTRATION:-false} && return "${EXIT_CODE}" || exit "${EXIT_CODE}"
