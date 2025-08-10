#!/bin/bash

# Test: JSON Linting
# Performs JSON validation using jq

# CHANGELOG
# 3.0.1 - 2025-08-03 - Removed extraneous command -v calls
# 3.0.0 - 2025-07-30 - Overhaul #1
# 2.0.0 - 2025-07-14 - Upgraded to use new modular test framework
# 1.0.0 - Initial version for JSON linting

# Test configuration
TEST_NAME="JSON Linting"
TEST_ABBR="JSN"
TEST_NUMBER="93"
TEST_VERSION="3.0.1"

# shellcheck source=tests/lib/framework.sh # Reference framework directly
[[ -n "${FRAMEWORK_GUARD}" ]] || source "$(dirname "${BASH_SOURCE[0]}")/lib/framework.sh"
setup_test_environment

# Test configuration
INTENTIONAL_ERROR_FILE="hydrogen_test_15_json_error.json"

print_subtest "JSON Linting"

JSON_FILES=()
while read -r file; do
    rel_file="${file#./}"
    if ! should_exclude_file "${rel_file}"; then
        JSON_FILES+=("${rel_file}")
    fi
done < <(find . -type f -name "*.json" || true)

JSON_COUNT=${#JSON_FILES[@]}
JSON_ISSUES=0
EXPECTED_ERRORS=0

if [[ "${JSON_COUNT}" -gt 0 ]]; then
    print_message "Checking ${JSON_COUNT} JSON files..."
    TEST_NAME="${TEST_NAME} {BLUE}(jsonlint: ${JSON_COUNT} files){RESET}"

    for file in "${JSON_FILES[@]}"; do
        if ! jq . "${file}" >/dev/null 2>&1; then
            if [[ "${file}" == *"${INTENTIONAL_ERROR_FILE}" ]]; then
                print_output "Expected invalid JSON (for testing): ${file}"
                ((EXPECTED_ERRORS++))
            else
                print_output "Unexpected invalid JSON: ${file}"
                ((JSON_ISSUES++))
            fi
        fi
    done
fi

if [[ "${JSON_ISSUES}" -eq 0 ]]; then
    if [[ "${EXPECTED_ERRORS}" -gt 0 ]]; then
        print_result 0 "Found ${EXPECTED_ERRORS} expected error(s) and no unexpected issues in ${JSON_COUNT} JSON files"
    else
        print_result 0 "No issues in ${JSON_COUNT} JSON files"
    fi
    ((PASS_COUNT++))
else
    print_result 1 "Found ${JSON_ISSUES} unexpected invalid JSON files"
    EXIT_CODE=1
fi

# Print test completion summary
print_test_completion "${TEST_NAME}" "${TEST_ABBR}" "${TEST_NUMBER}" "${TEST_VERSION}"

# Return status code if sourced, exit if run standalone
${ORCHESTRATION:-false} && return "${EXIT_CODE}" || exit "${EXIT_CODE}"
