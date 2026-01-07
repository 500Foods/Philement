#!/usr/bin/env bash

# Test: JSON Linting
# Performs JSON validation using jq

# CHANGELOG
# 3.2.0 - 2026-01-06 - Added JSON Schema validation for Hydrogen config files
# 3.1.0 - 2025-08-13 - Reviewed - removed mktemp call, not much else
# 3.0.1 - 2025-08-03 - Removed extraneous command -v calls
# 3.0.0 - 2025-07-30 - Overhaul #1
# 2.0.0 - 2025-07-14 - Upgraded to use new modular test framework
# 1.0.0 - Initial version for JSON linting

set -euo pipefail

# Test configuration
TEST_NAME="JSON Lint"
TEST_ABBR="JSN"
TEST_NUMBER="93"
TEST_COUNTER=0
TEST_VERSION="3.2.0"

# shellcheck source=tests/lib/framework.sh # Reference framework directly
[[ -n "${FRAMEWORK_GUARD:-}" ]] || source "$(dirname "${BASH_SOURCE[0]}")/lib/framework.sh"
setup_test_environment

# Test configuration
INTENTIONAL_ERROR_FILE="hydrogen_test_15_json_error.json"
SCHEMA_FILE="${HYDROGEN_ROOT}/tests/artifacts/hydrogen_config_schema.json"
JSONSCHEMA_CLI=$(command -v jsonschema-cli 2>/dev/null || echo "")

# Subtest 1: JSON Linting
print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "JSON Linting"

JSON_FILES=()
while read -r file; do
    rel_file="${file#./}"
    # shellcheck disable=SC2310 # We want to continue even if the test fails
    if ! should_exclude_file "${rel_file}"; then
        JSON_FILES+=("${rel_file}")
    fi
done < <("${FIND}" . -type f -name "*.json" || true)

JSON_COUNT=${#JSON_FILES[@]}
JSON_ISSUES=0
EXPECTED_ERRORS=0

if [[ "${JSON_COUNT}" -gt 0 ]]; then
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Checking ${JSON_COUNT} JSON files..."
    TEST_NAME="${TEST_NAME}  {BLUE}jsonlint: ${JSON_COUNT} files{RESET}"

    for file in "${JSON_FILES[@]}"; do
        if ! jq . "${file}" >/dev/null 2>&1; then
            if [[ "${file}" == *"${INTENTIONAL_ERROR_FILE}" ]]; then
                print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "Expected invalid JSON (for testing): ${file}"
                EXPECTED_ERRORS=$(( EXPECTED_ERRORS + 1 ))
            else
                print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "Unexpected invalid JSON: ${file}"
                JSON_ISSUES=$(( JSON_ISSUES + 1 ))
            fi
        fi
    done
fi

if [[ "${JSON_ISSUES}" -eq 0 ]]; then
    if [[ "${EXPECTED_ERRORS}" -gt 0 ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Found ${EXPECTED_ERRORS} expected error(s) and no unexpected issues in ${JSON_COUNT} JSON files"
    else
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "No issues in ${JSON_COUNT} JSON files"
    fi
else
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Found ${JSON_ISSUES} unexpected invalid JSON files"
    EXIT_CODE=1
fi

# Subtest 2+: JSON Schema Validation for Hydrogen Config Files (one test per file)
if [[ -z "${JSONSCHEMA_CLI}" ]]; then
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "JSON Schema Validation"
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 2 "jsonschema-cli not found - skipping schema validation"
elif [[ ! -f "${SCHEMA_FILE}" ]]; then
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "JSON Schema Validation"
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 2 "Schema file not found: ${SCHEMA_FILE} - skipping schema validation"
else
    # Find all Hydrogen configuration files (in configs directory) and sort them
    CONFIG_FILES=()
    while read -r file; do
        rel_file="${file#./}"
        # Only validate files in tests/configs directory and not the intentional error file
        if [[ "${rel_file}" == *"tests/configs/"* ]] && [[ "${rel_file}" != *"${INTENTIONAL_ERROR_FILE}" ]]; then
            # shellcheck disable=SC2310 # We want to continue even if the test fails
            if ! should_exclude_file "${rel_file}"; then
                CONFIG_FILES+=("${rel_file}")
            fi
        fi
    done < <("${FIND}" . -type f -name "hydrogen_test_*.json" | sort || true)
    
    CONFIG_COUNT=${#CONFIG_FILES[@]}
    
    if [[ "${CONFIG_COUNT}" -gt 0 ]]; then
        # Update test name to include count
        TEST_NAME="${TEST_NAME}  {BLUE}schema: ${CONFIG_COUNT} files{RESET}"
        
        # Print informational message about starting schema validation
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Starting Hydrogen Config JSON Schema Validation (${CONFIG_COUNT} files)..."
        
        # Validate each config file as a separate test
        for file in "${CONFIG_FILES[@]}"; do
            # Extract just the filename for the test name
            file_basename=$(basename "${file}")
            print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Schema: ${file_basename}"
            
            # Run jsonschema-cli and capture the output using text format for concise errors
            validation_output=$("${JSONSCHEMA_CLI}" "${SCHEMA_FILE}" -i "${file}" --output text 2>&1 || true)
            
            # Check if validation failed (output contains "INVALID")
            if echo "${validation_output}" | grep -q "INVALID"; then
                # Parse the output line by line to print each error separately
                while IFS= read -r line; do
                    [[ -n "${line}" ]] && print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "${line}"
                done <<< "${validation_output}"
                print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Schema validation failed"
                EXIT_CODE=1
            else
                print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Schema validation passed"
            fi
        done
    else
        print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "JSON Schema Validation"
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 2 "No Hydrogen config files found for schema validation"
    fi
fi

# Print test completion summary
print_test_completion "${TEST_NAME}" "${TEST_ABBR}" "${TEST_NUMBER}" "${TEST_VERSION}"

# Return status code if sourced, exit if run standalone
${ORCHESTRATION:-false} && return "${EXIT_CODE}" || exit "${EXIT_CODE}"
