#!/usr/bin/env bash

# Test: XML/SVG Linting
# Performs XML/SVG validation using xmlstarlet

# CHANGELOG
# 1.0.0 - Initial version for XML/SVG linting
# 1.1.0 - Changed to use xmlstarlet instead of xmllint

set -euo pipefail

# Test configuration
TEST_NAME="XML/SVG Lint"
TEST_ABBR="XML"
TEST_NUMBER="97"
TEST_COUNTER=0
TEST_VERSION="1.1.0"

# shellcheck source=tests/lib/framework.sh # Reference framework directly
[[ -n "${FRAMEWORK_GUARD:-}" ]] || source "$(dirname "${BASH_SOURCE[0]}")/lib/framework.sh"
setup_test_environment

# Test setup
LINT_OUTPUT_LIMIT=10

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "XML/SVG Linting (xmlstarlet)"

XML_FILES=()
while read -r file; do
    rel_file="${file#./}"
    # shellcheck disable=SC2310 # We want to continue even if the test fails
    if ! should_exclude_file "${rel_file}"; then
        XML_FILES+=("${rel_file}")
    fi
done < <("${FIND}" . -type f \( -name "*.xml" -o -name "*.svg" \) || true)

XML_COUNT=${#XML_FILES[@]}

if [[ "${XML_COUNT}" -gt 0 ]]; then
    TEMP_LOG="${LOG_PREFIX}${TIMESTAMP}_temp.log"
    
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Running xmlstarlet on ${XML_COUNT} XML/SVG files..."
    TEST_NAME="${TEST_NAME}  {BLUE}xmlstarlet: ${XML_COUNT} files{RESET}"

    # List files being checked for debugging
    for file in "${XML_FILES[@]}"; do
        print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "Checking: ${file}"
    done
    
    # Run xmlstarlet val on each file
    ISSUE_COUNT=0
    for file in "${XML_FILES[@]}"; do
        if ! xmlstarlet val "${file}" > /dev/null 2>&1; then
            echo "${file}: validation failed" >> "${TEMP_LOG}"
            ISSUE_COUNT=$(( ISSUE_COUNT + 1 ))
        fi
    done
    
    if [[ "${ISSUE_COUNT}" -gt 0 ]]; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "xmlstarlet found ${ISSUE_COUNT} issues:"
        # Use process substitution to avoid subshell issue with OUTPUT_COLLECTION
        while IFS= read -r line; do
            print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "${line}"
        done < <(head -n "${LINT_OUTPUT_LIMIT}" "${TEMP_LOG}" || true)
        if [[ "${ISSUE_COUNT}" -gt "${LINT_OUTPUT_LIMIT}" ]]; then
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Output truncated - Showing ${LINT_OUTPUT_LIMIT} of ${ISSUE_COUNT} lines"
        fi
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Found ${ISSUE_COUNT} issues in ${XML_COUNT} XML/SVG files"
        EXIT_CODE=1
    else
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "No issues in ${XML_COUNT} XML/SVG files"
    fi

else
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "No XML/SVG files to check"
    TEST_NAME="${TEST_NAME}  {BLUE}xmlstarlet: 0 files{RESET}"
fi

# Print test completion summary
print_test_completion "${TEST_NAME}" "${TEST_ABBR}" "${TEST_NUMBER}" "${TEST_VERSION}"

# Return status code if sourced, exit if run standalone
${ORCHESTRATION:-false} && return "${EXIT_CODE}" || exit "${EXIT_CODE}"