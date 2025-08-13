#!/bin/bash

# Test: HTML Linting
# Performs HTML validation using htmlhint

# CHANGELOG
# 3.0.1 - 2025-08-03 - Removed extraneous command -v calls
# 3.0.0 - 2025-07-30 - Overhaul #1
# 2.0.1 - 2025-07-18 - Fixed subshell issue in htmlhint output that prevented detailed error messages from being displayed in test output
# 2.0.0 - 2025-07-14 - Upgraded to use new modular test framework
# 1.0.0 - Initial version for HTML linting

# Test configuration
TEST_NAME="HTML Lint"
TEST_ABBR="HTM"
TEST_NUMBER="96"
TEST_VERSION="3.0.1"

# shellcheck source=tests/lib/framework.sh # Reference framework directly
[[ -n "${FRAMEWORK_GUARD}" ]] || source "$(dirname "${BASH_SOURCE[0]}")/lib/framework.sh"
setup_test_environment

# Test configuration constants
[[ -z "${LINT_OUTPUT_LIMIT:-}" ]] && readonly LINT_OUTPUT_LIMIT=10

print_subtest "HTML Linting (htmlhint)"

HTML_FILES=()
while read -r file; do
    rel_file="${file#./}"
    if ! should_exclude_file "${rel_file}"; then
        HTML_FILES+=("${rel_file}")
    fi
done < <(find . -type f -name "*.html" || true)

HTML_COUNT=${#HTML_FILES[@]}

if [[ "${HTML_COUNT}" -gt 0 ]]; then
    TEMP_LOG=$(mktemp)
    
    print_message "Running htmlhint on ${HTML_COUNT} HTML files..."
    TEST_NAME="${TEST_NAME} {BLUE}(htmlhint: ${HTML_COUNT} files){RESET}"

    # List files being checked for debugging
    for file in "${HTML_FILES[@]}"; do
        print_output "Checking: ${file}"
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
    grep -v "✓" "${TEMP_LOG}" | \
    grep -v "^$" | \
    grep -v "Scanned.*files.*no errors found" | \
    grep -v "files.*no errors found" > "${TEMP_LOG}.filtered" || true
    ISSUE_COUNT=$(wc -l < "${TEMP_LOG}.filtered")
    
    if [[ "${ISSUE_COUNT}" -gt 0 ]]; then
        print_message "htmlhint found ${ISSUE_COUNT} issues:"
        # Use process substitution to avoid subshell issue with OUTPUT_COLLECTION
        while IFS= read -r line; do
            print_output "${line}"
        done < <(head -n "${LINT_OUTPUT_LIMIT}" "${TEMP_LOG}.filtered" || true)
        if [[ "${ISSUE_COUNT}" -gt "${LINT_OUTPUT_LIMIT}" ]]; then
            print_message "Output truncated. Showing ${LINT_OUTPUT_LIMIT} of ${ISSUE_COUNT} lines."
        fi
        print_result 1 "Found ${ISSUE_COUNT} issues in ${HTML_COUNT} HTML files"
        EXIT_CODE=1
    else
        print_result 0 "No issues in ${HTML_COUNT} HTML files"
        ((PASS_COUNT++))
    fi
    
    rm -f "${TEMP_LOG}" "${TEMP_LOG}.filtered"
else
    print_result 0 "No HTML files to check"
    TEST_NAME="${TEST_NAME} {BLUE}(htmlhint: 0 files){RESET}"

    ((PASS_COUNT++))
fi

# Print test completion summary
print_test_completion "${TEST_NAME}" "${TEST_ABBR}" "${TEST_NUMBER}" "${TEST_VERSION}"

# Return status code if sourced, exit if run standalone
${ORCHESTRATION:-false} && return "${EXIT_CODE}" || exit "${EXIT_CODE}"
