#!/bin/bash

# Test: Markdown Linting
# Performs markdown linting validation

# CHANGELOG
# 3.0.1 - 2025-08-03 - Removed extraneous command -v calls
# 3.0.0 - 2025-07-30 - Overhaul #1
# 2.0.1 - 2025-07-18 - Fixed subshell issue in markdownlint output that prevented detailed error messages from being displayed in test output
# 2.0.0 - 2025-07-14 - Upgraded to use new modular test framework
# 1.0.0 - Initial version for markdown linting

# Test configuration
TEST_NAME="Markdown Lint"
TEST_ABBR="MKD"
TEST_NUMBER="90"
TEST_VERSION="3.0.1"

# shellcheck source=tests/lib/framework.sh # Reference framework directly
[[ -n "${FRAMEWORK_GUARD}" ]] || source "$(dirname "${BASH_SOURCE[0]}")/lib/framework.sh"
setup_test_environment

# Test configuration constants
[[ -z "${LINT_OUTPUT_LIMIT:-}" ]] && readonly LINT_OUTPUT_LIMIT=10

print_subtest "Markdown Linting"

# Get .lintignore filtered list of markdown files to check
MD_FILES=()
cd "${PROJECT_DIR}" || return &>/dev/null
mapfile -t md_file_list < <(find . -type f -name "*.md" || true)
for file in "${md_file_list[@]}"; do
    rel_file="${file#./}"
    if ! should_exclude_file "${rel_file}"; then
        MD_FILES+=("${rel_file}")
    fi
done

MD_COUNT=${#MD_FILES[@]}

if [[ "${MD_COUNT}" -gt 0 ]]; then
    TEMP_LOG=$(mktemp)
    FILTERED_LOG=$(mktemp)
    TEMP_NEW_LOG=$(mktemp)
    
    # Cache directory for markdownlint results
    CACHE_DIR="${HOME}/.cache/markdownlint"
    mkdir -p "${CACHE_DIR}"
    
    # Function to get file hash (using md5sum or equivalent)
    get_file_hash() {
        md5sum "$1" | awk '{print $1}' || true
    }
    
    # Categorize files for caching
    cached_files=0
    to_process_files=()
    processed_files=0
    
    for file in "${MD_FILES[@]}"; do
        file_hash=$(get_file_hash "${file}")
        cache_file="${CACHE_DIR}/${file##*/}_${file_hash}"
        if [[ -f "${cache_file}" ]]; then
            ((cached_files++))
            cat "${cache_file}" >> "${TEMP_LOG}" 2>&1 || true
        else
            to_process_files+=("${file}")
            ((processed_files++))
        fi
    done
    
    print_message "Using cached results for ${cached_files} files, processing ${processed_files} files out of ${MD_COUNT}..."
    TEST_NAME="${TEST_NAME} {BLUE}(markdownlint: ${MD_COUNT} files){RESET}"

    # Run markdownlint on files that need processing
    if [[ "${processed_files}" -gt 0 ]]; then
        print_message "Running markdownlint on ${processed_files} files..."
        markdownlint --config ".lintignore-markdown" "${to_process_files[@]}" 2> "${TEMP_NEW_LOG}"
       
        # Cache new results
        for file in "${to_process_files[@]}"; do
            file_hash=$(get_file_hash "${file}")
            cache_file="${CACHE_DIR}/$(basename "${file}")_${file_hash}"
            grep "^${file}:" "${TEMP_NEW_LOG}" > "${cache_file}" || true
        done
        
        # Append new results to total output
        cat "${TEMP_NEW_LOG}" >> "${TEMP_LOG}" 2>&1 || true
        rm -f "${TEMP_NEW_LOG}"
    fi
    
    if [[ -s "${TEMP_LOG}" ]]; then
        # Filter out Node.js deprecation warnings and other noise
        grep -v "DeprecationWarning" "${TEMP_LOG}" | \
        grep -v "Use \`node --trace-deprecation" | \
        grep -v "^(node:" | \
        grep -v "^$" > "${FILTERED_LOG}" || true
        
        ISSUE_COUNT=$(wc -l < "${FILTERED_LOG}")
        if [[ "${ISSUE_COUNT}" -gt 0 ]]; then
            print_message "markdownlint found ${ISSUE_COUNT} actual issues:"
            # Use process substitution to avoid subshell issue with OUTPUT_COLLECTION
            while IFS= read -r line; do
                print_output "${line}"
            done < <(head -n "${LINT_OUTPUT_LIMIT}" "${FILTERED_LOG}" || true)
            if [[ "${ISSUE_COUNT}" -gt "${LINT_OUTPUT_LIMIT}" ]]; then
                print_message "Output truncated. Showing ${LINT_OUTPUT_LIMIT} of ${ISSUE_COUNT} lines."
            fi
            print_result 1 "Found ${ISSUE_COUNT} issues in ${MD_COUNT} markdown files"
            EXIT_CODE=1
        else
            print_message "markdownlint completed (Node.js deprecation warnings filtered out)"
            print_result 0 "No issues in ${MD_COUNT} markdown files"
            ((PASS_COUNT++))
        fi
    else
        print_result 0 "No issues in ${MD_COUNT} markdown files"
        ((PASS_COUNT++))
    fi
    
    rm -f "${TEMP_LOG}" "${FILTERED_LOG}"
else
    print_result 0 "No markdown files to check"
    ((PASS_COUNT++))
fi

# Print test completion summary
print_test_completion "${TEST_NAME}" "${TEST_ABBR}" "${TEST_NUMBER}" "${TEST_VERSION}"

# Return status code if sourced, exit if run standalone
${ORCHESTRATION:-false} && return "${EXIT_CODE}" || exit "${EXIT_CODE}"
