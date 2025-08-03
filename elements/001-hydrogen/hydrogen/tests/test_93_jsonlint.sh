#!/bin/bash

# Test: JSON Linting
# Performs JSON validation using jq

# CHANGELOG
# 3.0.0 - 2025-07-30 - Overhaul #1
# 2.0.0 - 2025-07-14 - Upgraded to use new modular test framework
# 1.0.0 - Initial version for JSON linting

# Test configuration
TEST_NAME="JSON Linting"
TEST_ABBR="JSN"
TEST_NUMBER="93"
TEST_VERSION="3.0.0"

# shellcheck source=tests/lib/framework.sh # Reference framework directly
[[ -n "${FRAMEWORK_GUARD}" ]] || source "$(dirname "${BASH_SOURCE[0]}")/lib/framework.sh"
setup_test_environment

# Default exclude patterns for linting (can be overridden by .lintignore)
# Only declare if not already defined (prevents readonly variable redeclaration when sourced)
if [[ -z "${LINT_EXCLUDES:-}" ]]; then
    readonly LINT_EXCLUDES=(
        "build/*"
    )
fi

print_subtest "JSON Linting"

if command -v jq >/dev/null 2>&1; then
    JSON_FILES=()
    while read -r file; do
        # Use custom exclusion for JSON files - exclude Unity framework but include test configs
        if [[ "${file}" != *"/tests/unity/framework/"* ]] && ! should_exclude_file "${file}"; then
            JSON_FILES+=("${file}")
        fi
    done < <(find . -type f -name "*.json" || true)
    
    # Also include the intentionally broken test JSON file
    INTENTIONAL_ERROR_FILE="./tests/configs/hydrogen_test_json.json"
    if [[ -f "${INTENTIONAL_ERROR_FILE}" ]]; then
        JSON_FILES+=("${INTENTIONAL_ERROR_FILE}")
    fi
    
    JSON_COUNT=${#JSON_FILES[@]}
    JSON_ISSUES=0
    EXPECTED_ERRORS=0
    
    if [[ "${JSON_COUNT}" -gt 0 ]]; then
        print_message "Checking ${JSON_COUNT} JSON files..."
        TEST_NAME="${TEST_NAME} {BLUE}(jsonlint: ${JSON_COUNT} files){RESET}"

        for file in "${JSON_FILES[@]}"; do
            if ! jq . "${file}" >/dev/null 2>&1; then
                if [[ "${file}" == *"hydrogen_test_json.json" ]]; then
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
else
    print_result 0 "jq not available (skipped)"
    ((PASS_COUNT++))
fi

# Print test completion summary
print_test_completion "${TEST_NAME}" "${TEST_ABBR}" "${TEST_NUMBER}" "${TEST_VERSION}"

# Return status code if sourced, exit if run standalone
${ORCHESTRATION:-false} && return "${EXIT_CODE}" || exit "${EXIT_CODE}"
