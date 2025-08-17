#!/usr/bin/env bash

# Test: CSS Linting
# Performs CSS validation using stylelint

# CHANGELOG
# 3.1.0 - 2025-08-13 - Reviewed - removed mktemp call, not much else
# 3.0.1 - 2025-08-03 - Removed extraneous command -v calls
# 3.0.0 - 2025-07-30 - Overhaul #1
# 2.0.1 - 2025-07-18 - Fixed subshell issue in stylelint output that prevented detailed error messages from being displayed in test output
# 2.0.0 - 2025-07-14 - Upgraded to use new modular test framework
# 1.0.0 - Initial version for CSS linting

# Test configuration
TEST_NAME="CSS Lint"
TEST_ABBR="CSS"
TEST_NUMBER="95"
TEST_VERSION="3.1.0"

# shellcheck source=tests/lib/framework.sh # Reference framework directly
[[ -n "${FRAMEWORK_GUARD}" ]] || source "$(dirname "${BASH_SOURCE[0]}")/lib/framework.sh"
setup_test_environment

# Test setup
LINT_OUTPUT_LIMIT=10

print_subtest "CSS Linting (stylelint)"

CSS_FILES=()
while read -r file; do
    rel_file="${file#./}"
    if ! should_exclude_file "${rel_file}"; then
        CSS_FILES+=("${rel_file}")
    fi
done < <(find . -type f -name "*.css" || true)

CSS_COUNT=${#CSS_FILES[@]}

if [[ "${CSS_COUNT}" -gt 0 ]]; then
    TEMP_LOG="${LOG_PREFIX}${TIMESTAMP}_temp.log"
    
    print_message "Running stylelint on ${CSS_COUNT} CSS files..."
    TEST_NAME="${TEST_NAME} {BLUE}(stylelint: ${CSS_COUNT} files){RESET}"

    # Use basic stylelint rules if no config exists
    if [[ -f ".stylelintrc.json" ]] || [[ -f ".stylelintrc.js" ]] || [[ -f "stylelint.config.js" ]]; then
        stylelint "${CSS_FILES[@]}" > "${TEMP_LOG}" 2>&1 || true
    else
        # Use recommended rules if no config
        stylelint --config '{"extends": ["stylelint-config-standard"]}' "${CSS_FILES[@]}" > "${TEMP_LOG}" 2>&1 || true
    fi
    
    # Count actual issues (stylelint usually reports errors/warnings)
    ISSUE_COUNT=$(grep -c "✖" "${TEMP_LOG}" 2>/dev/null || wc -l < "${TEMP_LOG}")
    
    if [[ "${ISSUE_COUNT}" -gt 0 ]]; then
        print_message "stylelint found ${ISSUE_COUNT} issues:"
        # Use process substitution to avoid subshell issue with OUTPUT_COLLECTION
        while IFS= read -r line; do
            print_output "${line}"
        done < <(head -n "${LINT_OUTPUT_LIMIT}" "${TEMP_LOG}" || true)
        if [[ "${ISSUE_COUNT}" -gt "${LINT_OUTPUT_LIMIT}" ]]; then
            print_message "Output truncated - Showing ${LINT_OUTPUT_LIMIT} of ${ISSUE_COUNT} lines"
        fi
        print_result 1 "Found ${ISSUE_COUNT} issues in ${CSS_COUNT} CSS files"
        EXIT_CODE=1
    else
        print_result 0 "No issues in ${CSS_COUNT} CSS files"
        ((PASS_COUNT++))
    fi
else
    print_result 0 "No CSS files to check"
    TEST_NAME="${TEST_NAME} {BLUE}(stylelint: 0 files){RESET}"

    ((PASS_COUNT++))
fi

# Print test completion summary
print_test_completion "${TEST_NAME}" "${TEST_ABBR}" "${TEST_NUMBER}" "${TEST_VERSION}"

# Return status code if sourced, exit if run standalone
${ORCHESTRATION:-false} && return "${EXIT_CODE}" || exit "${EXIT_CODE}"
