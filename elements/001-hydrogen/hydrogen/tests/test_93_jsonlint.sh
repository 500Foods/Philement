#!/bin/bash

# Test: JSON Linting
# Performs JSON validation using jq

# CHANGELOG
# 2.0.0 - 2025-07-14 - Upgraded to use new modular test framework
# 1.0.0 - Initial version for JSON linting

# Test configuration
TEST_NAME="JSON Linting"
SCRIPT_VERSION="2.0.0"

# Get the directory where this script is located
TEST_SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

if [[ -z "${LOG_OUTPUT_SH_GUARD}" ]]; then
    # shellcheck source=tests/lib/log_output.sh # Resolve path statically
    source "${TEST_SCRIPT_DIR}/lib/log_output.sh"
fi

# shellcheck source=tests/lib/file_utils.sh # Resolve path statically
source "${TEST_SCRIPT_DIR}/lib/file_utils.sh"
# shellcheck source=tests/lib/framework.sh # Resolve path statically
source "${TEST_SCRIPT_DIR}/lib/framework.sh"

# Restore SCRIPT_DIR after sourcing libraries (they may override it)
SCRIPT_DIR="${TEST_SCRIPT_DIR}"

# Test configuration
EXIT_CODE=0
TOTAL_SUBTESTS=1
PASS_COUNT=0

# Auto-extract test number and set up environment
TEST_NUMBER=$(extract_test_number "${BASH_SOURCE[0]}")
set_test_number "${TEST_NUMBER}"
reset_subtest_counter

# Print beautiful test header
print_test_header "${TEST_NAME}" "${SCRIPT_VERSION}"

# Set up results directory
BUILD_DIR="${SCRIPT_DIR}/../build"
RESULTS_DIR="${BUILD_DIR}/tests/results"
mkdir -p "${RESULTS_DIR}"
TIMESTAMP=$(date +%Y%m%d_%H%M%S)

# Navigate to the project root (one level up from tests directory)
if ! navigate_to_project_root "${SCRIPT_DIR}"; then
    print_error "Failed to navigate to project root directory"
    exit 1
fi

# Set up results directory (after navigating to project root)

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

# Subtest 1: Lint JSON files
next_subtest
print_subtest "JSON Linting"

if command -v jq >/dev/null 2>&1; then
    JSON_FILES=()
    while read -r file; do
        # Use custom exclusion for JSON files - exclude Unity framework but include test configs
        if [[ "${file}" != *"/tests/unity/framework/"* ]] && ! should_exclude_file "${file}"; then
            JSON_FILES+=("${file}")
        fi
    done < <(find . -type f -name "*.json")
    
    # Also include the intentionally broken test JSON file
    INTENTIONAL_ERROR_FILE="./tests/configs/hydrogen_test_json.json"
    if [ -f "${INTENTIONAL_ERROR_FILE}" ]; then
        JSON_FILES+=("${INTENTIONAL_ERROR_FILE}")
    fi
    
    JSON_COUNT=${#JSON_FILES[@]}
    JSON_ISSUES=0
    EXPECTED_ERRORS=0
    
    if [ "${JSON_COUNT}" -gt 0 ]; then
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
    
    if [ "${JSON_ISSUES}" -eq 0 ]; then
        if [ "${EXPECTED_ERRORS}" -gt 0 ]; then
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

# Print completion table
print_test_completion "${TEST_NAME}"

# Return status code if sourced, exit if run standalone
if [[ "${ORCHESTRATION}" == "true" ]]; then
    return "${EXIT_CODE}"
else
    exit "${EXIT_CODE}"
fi
