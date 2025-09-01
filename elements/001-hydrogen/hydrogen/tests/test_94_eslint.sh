#!/usr/bin/env bash

# Test: JavaScript Linting
# Performs JavaScript validation using eslint

# CHANGELOG
# 3.1.0 - 2025-08-13 - Reviewed - removed mktemp call, not much else
# 3.0.1 - 2025-08-03 - Removed extraneous command -v calls
# 3.0.0 - 2025-07-30 - Overhaul #1
# 2.0.1 - 2025-07-18 - Fixed subshell issue in eslint output that prevented detailed error messages from being displayed in test output
# 2.0.0 - 2025-07-14 - Upgraded to use new modular test framework
# 1.0.0 - Initial version for JavaScript linting

set -euo pipefail

# Test configuration
TEST_NAME="JavaScript Lint"
TEST_ABBR="JAV"
TEST_NUMBER="94"
TEST_COUNTER=0
TEST_VERSION="3.1.0"

# shellcheck source=tests/lib/framework.sh # Reference framework directly
[[ -n "${FRAMEWORK_GUARD:-}" ]] || source "$(dirname "${BASH_SOURCE[0]}")/lib/framework.sh"
setup_test_environment

# Test setup
# Only declare if not already defined (prevents readonly variable redeclaration when sourced)
LINT_OUTPUT_LIMIT=10

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "JavaScript Linting (eslint)"

JS_FILES=()
while read -r file; do
    rel_file="${file#./}"
    # shellcheck disable=SC2310 # We want to continue even if the test fails
    if ! should_exclude_file "${rel_file}"; then
        JS_FILES+=("${rel_file}")
    fi
done < <("${FIND}" . -type f -name "*.js" || true)

JS_COUNT=${#JS_FILES[@]}

if [[ "${JS_COUNT}" -gt 0 ]]; then
    TEMP_LOG="${LOG_PREFIX}${TIMESTAMP}_temp.log"

    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Running eslint on ${JS_COUNT} JavaScript files..."
    TEST_NAME="${TEST_NAME} {BLUE}(eslint: ${JS_COUNT} files){RESET}"

    # Use basic eslint rules if no config exists
    if [[ -f ".eslintrc.js" ]] || [[ -f ".eslintrc.json" ]] || [[ -f "eslint.config.js" ]]; then
        eslint "${JS_FILES[@]}" > "${TEMP_LOG}" 2>&1 || true
    else
        # Run ESLint with no config (defaults will apply)
        eslint --no-config-lookup "${JS_FILES[@]}" > "${TEMP_LOG}" 2>&1 || true
    fi
    
    ISSUE_COUNT=$(wc -l < "${TEMP_LOG}")
    if [[ "${ISSUE_COUNT}" -gt 0 ]]; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "eslint found ${ISSUE_COUNT} issues:"
        # Use process substitution to avoid subshell issue with OUTPUT_COLLECTION
        while IFS= read -r line; do
            print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "${line}"
        done < <(head -n "${LINT_OUTPUT_LIMIT}" "${TEMP_LOG}" || true)
        if [[ "${ISSUE_COUNT}" -gt "${LINT_OUTPUT_LIMIT}" ]]; then
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Output truncated - Showing ${LINT_OUTPUT_LIMIT} of ${ISSUE_COUNT} lines"
        fi
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Found ${ISSUE_COUNT} issues in ${JS_COUNT} JavaScript files"
        EXIT_CODE=1
    else
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "No issues in ${JS_COUNT} JavaScript files"
    fi

else
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "No JavaScript files to check"
    TEST_NAME="${TEST_NAME} {BLUE}(eslint: 0 files){RESET}"
fi

# Print test completion summary
print_test_completion "${TEST_NAME}" "${TEST_ABBR}" "${TEST_NUMBER}" "${TEST_VERSION}"

# Return status code if sourced, exit if run standalone
${ORCHESTRATION:-false} && return "${EXIT_CODE}" || exit "${EXIT_CODE}"
