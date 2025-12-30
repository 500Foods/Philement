#!/usr/bin/env bash

# Test: HTML Linting
# Performs HTML validation using htmlhint

# CHANGELOG
# 3.1.0 - 2025-08-13 - Reviewed - removed mktemp call, not much else
# 3.0.1 - 2025-08-03 - Removed extraneous command -v calls
# 3.0.0 - 2025-07-30 - Overhaul #1
# 2.0.1 - 2025-07-18 - Fixed subshell issue in htmlhint output that prevented detailed error messages from being displayed in test output
# 2.0.0 - 2025-07-14 - Upgraded to use new modular test framework
# 1.0.0 - Initial version for HTML linting

set -euo pipefail

# Test configuration
TEST_NAME="HTML Lint"
TEST_ABBR="HTM"
TEST_NUMBER="96"
TEST_COUNTER=0
TEST_VERSION="3.1.0"

# shellcheck source=tests/lib/framework.sh # Reference framework directly
[[ -n "${FRAMEWORK_GUARD:-}" ]] || source "$(dirname "${BASH_SOURCE[0]}")/lib/framework.sh"
setup_test_environment

# Test setup
LINT_OUTPUT_LIMIT=10

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "HTML Linting (htmlhint)"

HTML_FILES=()
while read -r file; do
    rel_file="${file#./}"
    # shellcheck disable=SC2310 # We want to continue even if the test fails
    if ! should_exclude_file "${rel_file}"; then
        HTML_FILES+=("${rel_file}")
    fi
done < <("${FIND}" . -type f -name "*.html" || true)

HTML_COUNT=${#HTML_FILES[@]}

if [[ "${HTML_COUNT}" -gt 0 ]]; then
    TEMP_LOG="${LOG_PREFIX}${TIMESTAMP}_temp.log"
    
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Running htmlhint on ${HTML_COUNT} HTML files..."
    TEST_NAME="${TEST_NAME}  {BLUE}htmlhint: ${HTML_COUNT files){RESET}"

    # List files being checked for debugging
    for file in "${HTML_FILES[@]}"; do
        print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "Checking: ${file}"
    done
    
    # Use basic htmlhint rules if no config exists
    if [[ -f ".htmlhintrc" ]]; then
        htmlhint "${HTML_FILES[@]}" > "${TEMP_LOG}" 2>&1 || true
    else
        # Use recommended rules if no config
        htmlhint --rules "doctype-first,title-require,tag-pair,attr-lowercase,attr-value-double-quotes,id-unique,src-not-empty,attr-no-duplication" "${HTML_FILES[@]}" > "${TEMP_LOG}" 2>&1 || true
    fi
    
    # Filter out success messages and count actual issues
    # htmlhint success messages include "Scanned X files, no errors found"
    "${GREP}" -Ev "✓|^$|Scanned.*files.*no errors found|files.*no errors found" "${TEMP_LOG}" > "${TEMP_LOG}.filtered" || true
    ISSUE_COUNT=$(wc -l < "${TEMP_LOG}.filtered")
    
    if [[ "${ISSUE_COUNT}" -gt 0 ]]; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "htmlhint found ${ISSUE_COUNT} issues:"
        # Use process substitution to avoid subshell issue with OUTPUT_COLLECTION
        while IFS= read -r line; do
            print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "${line}"
        done < <(head -n "${LINT_OUTPUT_LIMIT}" "${TEMP_LOG}.filtered" || true)
        if [[ "${ISSUE_COUNT}" -gt "${LINT_OUTPUT_LIMIT}" ]]; then
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Output truncated - Showing ${LINT_OUTPUT_LIMIT} of ${ISSUE_COUNT} lines"
        fi
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Found ${ISSUE_COUNT} issues in ${HTML_COUNT} HTML files"
        EXIT_CODE=1
    else
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "No issues in ${HTML_COUNT} HTML files"
    fi

else
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "No HTML files to check"
    TEST_NAME="${TEST_NAME}  {BLUE}htmlhint: 0 files{RESET}"
fi

# Print test completion summary
print_test_completion "${TEST_NAME}" "${TEST_ABBR}" "${TEST_NUMBER}" "${TEST_VERSION}"

# Return status code if sourced, exit if run standalone
${ORCHESTRATION:-false} && return "${EXIT_CODE}" || exit "${EXIT_CODE}"
