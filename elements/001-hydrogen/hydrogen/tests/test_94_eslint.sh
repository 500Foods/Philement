#!/bin/bash

# Test: JavaScript Linting
# Performs JavaScript validation using eslint

# CHANGELOG
# 3.0.0 - 2025-07-30 - Overhaul #1
# 2.0.1 - 2025-07-18 - Fixed subshell issue in eslint output that prevented detailed error messages from being displayed in test output
# 2.0.0 - 2025-07-14 - Upgraded to use new modular test framework
# 1.0.0 - Initial version for JavaScript linting

# Test configuration
TEST_NAME="JavaScript Linting"
TEST_ABBR="JAV"
TEST_NUMBER="94"
TEST_VERSION="3.0.0"

# shellcheck source=tests/lib/framework.sh # Reference framework directly
[[ -n "${FRAMEWORK_GUARD}" ]] || source "$(dirname "${BASH_SOURCE[0]}")/lib/framework.sh"
setup_test_environment

# Test variables
TEST_SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

# Test configuration constants
# Only declare if not already defined (prevents readonly variable redeclaration when sourced)
if [[ -z "${LINT_OUTPUT_LIMIT:-}" ]]; then
    readonly LINT_OUTPUT_LIMIT=100
fi

# Default exclude patterns for linting (can be overridden by .lintignore)
# Only declare if not already defined (prevents readonly variable redeclaration when sourced)
if [[ -z "${LINT_EXCLUDES:-}" ]]; then
    readonly LINT_EXCLUDES=(
        "build/*"
    )
fi

# Check if a file should be excluded from linting
should_exclude_file() {
    local file="$1"
    local lint_ignore=".lintignore"
    local rel_file="${file#./}"  # Remove leading ./
    
    # Check .lintignore file first if it exists
    if [ -f "${lint_ignore}" ]; then
        while IFS= read -r pattern; do
            [[ -z "${pattern}" || "${pattern}" == \#* ]] && continue
            # Remove trailing /* if present for directory matching
            local clean_pattern="${pattern%/\*}"
            
            # Check if file matches pattern exactly or is within a directory pattern
            if [[ "${rel_file}" == "${pattern}" ]] || [[ "${rel_file}" == "${clean_pattern}"/* ]]; then
                return 0 # Exclude
            fi
        done < "${lint_ignore}"
    fi
    
    # Check default excludes
    for pattern in "${LINT_EXCLUDES[@]}"; do
        local clean_pattern="${pattern%/\*}"
        if [[ "${rel_file}" == "${pattern}" ]] || [[ "${rel_file}" == "${clean_pattern}"/* ]]; then
            return 0 # Exclude
        fi
    done
    
    return 1 # Do not exclude
}

# Subtest 1: Lint JavaScript files
next_subtest
print_subtest "JavaScript Linting (eslint)"

if command -v eslint >/dev/null 2>&1; then
    JS_FILES=()
    while read -r file; do
        if ! should_exclude_file "${file}"; then
            JS_FILES+=("${file}")
        fi
    done < <(find . -type f -name "*.js")
    
    JS_COUNT=${#JS_FILES[@]}
    
    if [ "${JS_COUNT}" -gt 0 ]; then
        TEMP_LOG=$(mktemp)
        
        print_message "Running eslint on ${JS_COUNT} JavaScript files..."
        TEST_NAME="${TEST_NAME} {BLUE}(eslint: ${JS_COUNT} files){RESET}"

        # Use basic eslint rules if no config exists
        if [ -f ".eslintrc.js" ] || [ -f ".eslintrc.json" ] || [ -f "eslint.config.js" ]; then
            eslint "${JS_FILES[@]}" > "${TEMP_LOG}" 2>&1 || true
        else
            # Use recommended rules if no config
            eslint --no-eslintrc --config '{"extends": ["eslint:recommended"], "env": {"browser": true, "es6": true}, "parserOptions": {"ecmaVersion": 2020}}' "${JS_FILES[@]}" > "${TEMP_LOG}" 2>&1 || true
        fi
        
        ISSUE_COUNT=$(wc -l < "${TEMP_LOG}")
        if [ "${ISSUE_COUNT}" -gt 0 ]; then
            print_message "eslint found ${ISSUE_COUNT} issues:"
            # Use process substitution to avoid subshell issue with OUTPUT_COLLECTION
            while IFS= read -r line; do
                print_output "${line}"
            done < <(head -n "${LINT_OUTPUT_LIMIT}" "${TEMP_LOG}")
            if [ "${ISSUE_COUNT}" -gt "${LINT_OUTPUT_LIMIT}" ]; then
                print_message "Output truncated. Showing ${LINT_OUTPUT_LIMIT} of ${ISSUE_COUNT} lines."
            fi
            print_result 1 "Found ${ISSUE_COUNT} issues in ${JS_COUNT} JavaScript files"
            EXIT_CODE=1
        else
            print_result 0 "No issues in ${JS_COUNT} JavaScript files"
            ((PASS_COUNT++))
        fi
        
        rm -f "${TEMP_LOG}"
    else
        print_result 0 "No JavaScript files to check"
        TEST_NAME="${TEST_NAME} {BLUE}(eslint: 0 files){RESET}"

        ((PASS_COUNT++))
    fi
else
    print_result 0 "eslint not available (skipped)"
    ((PASS_COUNT++))
fi

# Calculate overall test result
[[ "${PASS_COUNT}" -eq "${TOTAL_SUBTESTS}" ]] && EXIT_CODE=0 || EXIT_CODE=1

# Print test completion summary
print_test_completion "${TEST_NAME}" "${TEST_ABBR}" "${TEST_NUMBER}" "${TEST_VERSION}"

# Return status code if sourced, exit if run standalone
${ORCHESTRATION:-false} && return "${EXIT_CODE}" || exit "${EXIT_CODE}"
