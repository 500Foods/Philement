#!/usr/bin/env bash

# Test: Markdown Linting
# Performs markdown linting validation

# FUNCTIONS
# get_file_hash()

# CHANGELOG
# 3.1.0 - 2025-08-13 - General review, cleanup log files
# 3.0.1 - 2025-08-03 - Removed extraneous command -v calls
# 3.0.0 - 2025-07-30 - Overhaul #1
# 2.0.1 - 2025-07-18 - Fixed subshell issue in markdownlint output that prevented detailed error messages from being displayed in test output
# 2.0.0 - 2025-07-14 - Upgraded to use new modular test framework
# 1.0.0 - Initial version for markdown linting

set -euo pipefail

# Test configuration
TEST_NAME="Markdown Lint"
TEST_ABBR="MKD"
TEST_NUMBER="90"
TEST_COUNTER=0
TEST_VERSION="3.1.0"

# shellcheck source=tests/lib/framework.sh # Reference framework directly
[[ -n "${FRAMEWORK_GUARD:-}" ]] || source "$(dirname "${BASH_SOURCE[0]}")/lib/framework.sh"
setup_test_environment

# Test setup
LINT_OUTPUT_LIMIT=10
CACHE_DIR="${HOME}/.cache/lithium/markdownlint"
mkdir -p "${CACHE_DIR}"

# Function to get file hash (using md5sum or equivalent)
get_file_hash() {
    # shellcheck disable=SC2016 # Using single quotes on purpose to avoid escaping issues
    "${MD5SUM}" "$1" | "${AWK}" '{print $1}' || true
}

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Markdown Linting"

# Set paths for webroot and include directories (similar to test_04)
# PHILEMENT_ROOT="$(realpath ../../../)"
# export PHILEMENT_ROOT
# LITHIUM_ROOT="$(pwd)"
# export LITHIUM_ROOT
# LITHIUM_DOCS_ROOT="$(realpath ../../../docs/H)"
# export LITHIUM_DOCS_ROOT

# Get .lintignore filtered list of markdown files to check from multiple directories
MD_FILES=()
cd "${PROJECT_DIR}" || return &>/dev/null

# Search in current directory, LITHIUM_ROOT, and LITHIUM_DOCS_ROOT
mapfile -t md_file_list < <("${FIND}" . -type f -name "*.md" || true)
mapfile -t lithium_root_files < <("${FIND}" "${LITHIUM_ROOT}" -type f -name "*.md" 2>/dev/null || true)
mapfile -t lithium_docs_files < <("${FIND}" "${LITHIUM_DOCS_ROOT}" -type f -name "*.md" 2>/dev/null || true)

# Combine all file lists
all_md_files=("${md_file_list[@]}" "${lithium_root_files[@]}" "${lithium_docs_files[@]}")
for file in "${all_md_files[@]}"; do
    # Skip empty entries
    [[ -z "${file}" ]] && continue

    # Convert to relative paths for consistency
    if [[ "${file}" == "${LITHIUM_ROOT}"* ]]; then
        rel_file="${file#"${LITHIUM_ROOT}"/}"
    elif [[ "${file}" == "${LITHIUM_DOCS_ROOT}"* ]]; then
        rel_file="${file#"${LITHIUM_DOCS_ROOT}"/}"
    elif [[ "${file}" == ./* ]]; then
        rel_file="${file#./}"
    else
        rel_file="${file}"
    fi

    # Skip if file is empty or doesn't exist
    [[ -z "${rel_file}" ]] && continue
    [[ ! -f "${file}" ]] && continue

    # shellcheck disable=SC2310 # We want to continue even if the test fails
    if ! should_exclude_file "${rel_file}"; then
        MD_FILES+=("${file}")
    fi
done

MD_COUNT=${#MD_FILES[@]}

if [[ "${MD_COUNT}" -gt 0 ]]; then
    TEMP_LOG="${LOG_PREFIX}${TIMESTAMP}_temp.log"
    FILTERED_LOG="${LOG_PREFIX}${TIMESTAMP}_filtered.log"
    TEMP_NEW_LOG="${LOG_PREFIX}${TIMESTAMP}_new.log"
    temp_hashes="${LOG_PREFIX}${TIMESTAMP}_hashes.log"

    # Categorize files for caching
    cached_files=0
    to_process_files=()
    processed_files=0

    # Compute MD5 hashes for all files in one go
    # Use a temporary file to store the hashes
    printf "%s\0" "${MD_FILES[@]}" | "${XARGS}" -0 "${MD5SUM}" > "${temp_hashes}" 2>/dev/null

    # Process the hashes and categorize files
    while IFS=' ' read -r file_hash file; do
        cache_file="${CACHE_DIR}/${file##*/}_${file_hash}"
        if [[ -f "${cache_file}" ]]; then
            cached_files=$(( cached_files + 1 ))
            cat "${cache_file}" >> "${TEMP_LOG}" 2>/dev/null || true
        else
            to_process_files+=("${file}")
            processed_files=$(( processed_files + 1 ))
        fi
    done < "${temp_hashes}"

    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Using cached results for ${cached_files} files, processing ${processed_files} files out of ${MD_COUNT}..."

    TEST_NAME="${TEST_NAME}  {BLUE}markdownlint: ${MD_COUNT} files{RESET}"

    # Run markdownlint on files that need processing
    if [[ "${processed_files}" -gt 0 ]]; then

        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Running markdownlint on ${processed_files} files..."
        markdownlint --config ".lintignore-markdown" "${to_process_files[@]}" 2> "${TEMP_NEW_LOG}" || true
       
        # Cache new results
        for file in "${to_process_files[@]}"; do
            file_hash=$(get_file_hash "${file}")
            cache_file="${CACHE_DIR}/$(basename "${file}")_${file_hash}"
            "${GREP}" "^${file}:" "${TEMP_NEW_LOG}" > "${cache_file}" || true
        done
        
        # Append new results to total output
        cat "${TEMP_NEW_LOG}" >> "${TEMP_LOG}" 2>&1 || true
        rm -f "${TEMP_NEW_LOG}"
    fi
    
    if [[ -s "${TEMP_LOG}" ]]; then

        # Filter out Node.js deprecation warnings and other noise
        "${GREP}" -Ev "DeprecationWarning|Use \`node --trace-deprecation|\(node:|^$" "${TEMP_LOG}" > "${FILTERED_LOG}" || true
        ISSUE_COUNT=$(wc -l < "${FILTERED_LOG}")

        if [[ "${ISSUE_COUNT}" -gt 0 ]]; then

            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "markdownlint found ${ISSUE_COUNT} actual issues:"
        
            # Use process substitution to avoid subshell issue with OUTPUT_COLLECTION
            while IFS= read -r line; do
                print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "${line}"
            done < <(head -n "${LINT_OUTPUT_LIMIT}" "${FILTERED_LOG}" || true)

            if [[ "${ISSUE_COUNT}" -gt "${LINT_OUTPUT_LIMIT}" ]]; then
                print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Output truncated - Showing ${LINT_OUTPUT_LIMIT} of ${ISSUE_COUNT} lines"
            fi

            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Found ${ISSUE_COUNT} issues in ${MD_COUNT} markdown files"
            EXIT_CODE=1
        else
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "markdownlint completed (Node.js deprecation warnings filtered out)"
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "No issues in ${MD_COUNT} markdown files"
        fi
    else
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "No issues in ${MD_COUNT} markdown files"
    fi
    
else
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "No markdown files to check"
fi

# Print test completion summary
print_test_completion "${TEST_NAME}" "${TEST_ABBR}" "${TEST_NUMBER}" "${TEST_VERSION}"

# Return status code if sourced, exit if run standalone
${ORCHESTRATION:-false} && return "${EXIT_CODE}" || exit "${EXIT_CODE}"
