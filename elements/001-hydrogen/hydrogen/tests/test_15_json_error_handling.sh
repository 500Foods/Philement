#!/usr/bin/env bash

# Test: JSON Error Handling
# Tests if the application correctly handles JSON syntax errors in configuration files
# and provides meaningful error messages with position information

# CHANGLEOG
# 4.1.0 - 2025-08-08 - General logging cleanup and tweaks
# 4.0.0 - 2025-07-30 - Overhaul #1
# 3.0.2 - 2025-07-14 - Updated to use build/tests directories for test output consistency
# 3.0.1 - 2025-07-06 - Added missing shellcheck justifications
# 3.0.0 - 2025-07-02 - Complete rewrite to use new modular test libraries
# 2.0.0 - 2025-06-17 - Major refactoring: fixed all shellcheck warnings, improved modularity, enhanced comments
# 1.0.0 - Original version - Basic JSON error handling test

set -euo pipefail

# Test configuration
TEST_NAME="JSON Error Handling"
TEST_ABBR="JEH"
TEST_NUMBER="15"
TEST_COUNTER=0
TEST_VERSION="4.1.0"

# shellcheck source=tests/lib/framework.sh # Reference framework directly
[[ -n "${FRAMEWORK_GUARD:-}" ]] || source "$(dirname "${BASH_SOURCE[0]}")/lib/framework.sh"
setup_test_environment

# Test Configuration
ERROR_OUTPUT="${LOGS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}_json_error.log"
TEST_CONFIG=$(get_config_path "hydrogen_test_15_json_error.json")

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

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Validate Test Configuration File"

# shellcheck disable=SC2310 # We want to continue even if the test fails
if validate_config_file "${TEST_CONFIG}"; then
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Using test file with JSON syntax error (missing comma)"
else
    EXIT_CODE=1
fi

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Launch with Malformed JSON Configuration"
print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Testing configuration: $(basename "${TEST_CONFIG}" .json)"

print_command "${TEST_NUMBER}" "${TEST_COUNTER}" "$(basename "${HYDROGEN_BIN}") $(basename "${TEST_CONFIG}")"

# Capture both stdout and stderr
if "${HYDROGEN_BIN}" "${TEST_CONFIG}" &> "${ERROR_OUTPUT}"; then
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Application should have exited with an error but didn't"
    EXIT_CODE=1
else
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Application exited with error as expected"
fi

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Verify Error Message Contains Position Information"
print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Examining error output..."

if "${GREP}" -q "line" "${ERROR_OUTPUT}" && "${GREP}" -q "column" "${ERROR_OUTPUT}"; then
    while IFS= read -r line; do
        output_line=$([[ "${line}" == \[* ]] && echo "${line:39}" || echo "${line}")
        print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "${output_line}"
    done < <(tail -n 5 "${ERROR_OUTPUT}" || true)
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Error message contains line and column information"
else
    while IFS= read -r line; do
        output_line=$([[ "${line}" == \[* ]] && echo "${line:39}" || echo "${line}")
        print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "${output_line}"
    done < <(tail -n 5 "${ERROR_OUTPUT}" || true)
    EXIT_CODE=1
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Error message does not contain line and column information"
fi

# Save error output to results directory
print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Log: ..${ERROR_OUTPUT}"

# Print completion table
print_test_completion "${TEST_NAME}" "${TEST_ABBR}" "${TEST_NUMBER}" "${TEST_VERSION}"

# Return status code if sourced, exit if run standalone
${ORCHESTRATION:-false} && return "${EXIT_CODE}" || exit "${EXIT_CODE}"
