#!/usr/bin/env bash

# Test: Shell Script Analysis
# Performs shellcheck analysis and exception justification checking

# FUNCTIONS
# process_file()

# CHANGELOG
# 4.1.1 - 2025-09-16 - Fixed stale justification error messages persisting from previous runs
# 4.1.0 - 2025-08-13 - General review, tweaks to log files
# 4.0.1 - 2025-08-03 - Removed extraneous command -v calls
# 4.0.0 - 2025-07-30 - Overhaul #1
# 3.1.0 - 2025-07-28 - Optimized caching, removed md5sum/wc spawns, single shellcheck run
# 3.0.0 - 2025-07-18 - Added caching mechanism for shellcheck results
# 2.0.1 - 2025-07-18 - Fixed subshell issue in shellcheck output
# 2.0.0 - 2025-07-14 - Upgraded to modular test framework
# 1.0.0 - Initial version

set -euo pipefail

# Test configuration
TEST_NAME="Bash Lint"
TEST_ABBR="BSH"
TEST_NUMBER="92"
TEST_COUNTER=0
TEST_VERSION="4.1.1"

# shellcheck source=tests/lib/framework.sh # Reference framework directly
[[ -n "${FRAMEWORK_GUARD:-}" ]] || source "$(dirname "${BASH_SOURCE[0]}")/lib/framework.sh"
setup_test_environment

# Test settings
LINT_OUTPUT_LIMIT=25
SHELLCHECK_CACHE_DIR="${HOME}/.cache/hydrogen/shellcheck"
mkdir -p "${SHELLCHECK_CACHE_DIR}"

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Shell Script Linting (shellcheck)"
print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Detected ${CORES} CPU cores for parallel processing"

SHELL_FILES=()
TEST_SHELL_FILES=()
OTHER_SHELL_FILES=()
while read -r file; do
    # shellcheck disable=SC2310 # We want to continue even if the test fails
    if ! should_exclude_file "${file}"; then
        SHELL_FILES+=("${file}")
        if [[ "${file}" == ./tests/* ]]; then
            TEST_SHELL_FILES+=("${file}")
        else
            OTHER_SHELL_FILES+=("${file}")
        fi
    fi
done < <("${FIND}" . -type f -name "*.sh" || true)

SHELL_COUNT=${#SHELL_FILES[@]}
SHELL_ISSUES=0
TEMP_OUTPUT="${LOG_PREFIX}${TIMESTAMP}_temp.log"

if [[ "${SHELL_COUNT}" -gt 0 ]]; then
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Running shellcheck on ${SHELL_COUNT} shell scripts with caching..."
    TEST_NAME="${TEST_NAME} {BLUE}(shellheck: ${SHELL_COUNT} files){RESET}"

    # Batch compute content hashes for all files
    declare -A file_hashes
    while read -r hash file; do
        file_hashes["${file}"]="${hash}"
    done < <("${MD5SUM}" "${SHELL_FILES[@]}" || true)

    cached_files=0
    processed_scripts=0
    to_process=()

    for file in "${SHELL_FILES[@]}"; do
        content_hash="${file_hashes[${file}]}"
        # Flatten path for uniqueness (replace / with _)
        flat_path=$(echo "cache${file}" | tr '/' '_')
        cache_file="${SHELLCHECK_CACHE_DIR}/${flat_path}_${content_hash}"
        if [[ -f "${cache_file}" ]]; then
            cached_files=$(( cached_files + 1 ))
            cat "${cache_file}" >> "${TEMP_OUTPUT}" 2>&1
        else
            to_process+=("${file}")
            processed_scripts=$(( processed_scripts + 1 ))
        fi
    done

    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Using cached results for ${cached_files} files, processing ${processed_scripts} files..."

    # Function to run shellcheck and cache for a single file
    process_script() {
        local file="$1"
        local content_hash="$2"
        local flat_path
        flat_path=$(echo "cache${file}" | tr '/' '_')
        local cache_file="${SHELLCHECK_CACHE_DIR}/${flat_path}_${content_hash}"
        shellcheck "${file}" > "${cache_file}" 2>&1
        cat "${cache_file}"
    }

    # These are needed to ensure subshells can see them
    export -f process_script  
    export SHELLCHECK_CACHE_DIR

    # Process non-cached files in parallel, passing file and hash
    # shellcheck disable=SC2016 # Script within a script is tripping up shellcheck
    if [[ "${processed_scripts}" -gt 0 ]]; then
        for file in "${to_process[@]}"; do
            "${PRINTF}" '%s %s\n' "${file}" "${file_hashes[${file}]}" || true
        done | "${XARGS}" -n 2 -P "${CORES}" bash -c 'process_script "$0" "$1"' >> "${TEMP_OUTPUT}" 2>&1
    fi

    # Filter output
    "${SED}" "s|$(pwd)/||g; s|tests/||g" "${TEMP_OUTPUT}" > "${TEMP_OUTPUT}.filtered" || true

    SHELL_ISSUES=$(wc -l < "${TEMP_OUTPUT}.filtered")
    if [[ "${SHELL_ISSUES}" -gt 0 ]]; then
        FILES_WITH_ISSUES=$(cut -d: -f1 "${TEMP_OUTPUT}.filtered" | sort -u | wc -l || true)
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "shellcheck found ${SHELL_ISSUES} issues in ${FILES_WITH_ISSUES} files:"
        while IFS= read -r line; do
            print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "${line}"
        done < <(head -n "${LINT_OUTPUT_LIMIT}" "${TEMP_OUTPUT}.filtered" || true)
        if [[ "${SHELL_ISSUES}" -gt "${LINT_OUTPUT_LIMIT}" ]]; then
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Output truncated - Showing ${LINT_OUTPUT_LIMIT} of ${SHELL_ISSUES} lines"
        fi
    fi
fi

if [[ "${SHELL_ISSUES}" -eq 0 ]]; then
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "No issues in ${SHELL_COUNT} shell files"
else
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Found ${SHELL_ISSUES} issues in shell files"
    EXIT_CODE=1
fi

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Shellcheck Exception Justification Check"
print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Checking for shellcheck directives in shell scripts..."

with_just=0
without_just=0
SHELLCHECK_DIRECTIVE_TOTAL=0
SHELLCHECK_DIRECTIVE_WITH_JUSTIFICATION=0
SHELLCHECK_DIRECTIVE_WITHOUT_JUSTIFICATION=0

# Function to process a single file and output counters + messages
process_file() {
    local file="$1"
    local total=0 with_just=0 without_just=0
    # Remove any old no-justification log to prevent stale messages
    rm -f "${file}.nojust.log"
    while IFS= read -r line; do
        if [[ "${line}" =~ ^[[:space:]]*"# shellcheck" ]]; then
            total=$(( total + 1 ))
            if [[ "${line}" =~ ^[[:space:]]*"# shellcheck"[[:space:]]+[^[:space:]]+[[:space:]]+"#".* ]]; then
                with_just=$(( with_just + 1 ))
            else
                without_just=$(( without_just + 1 ))
                # Output to a file-specific log to avoid interleaving
                echo "No justification found in ${file}: ${line}" >> "${file}.nojust.log"
            fi
        fi
    done < "${file}"
    # Output counters in a parseable format
    echo "${total} ${with_just} ${without_just} ${file}"
}

# This is needed so it can be seen from a subshell
export -f process_file

if [[ ${#SHELL_FILES[@]} -gt 0 ]]; then
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Analyzing ${SHELL_COUNT} shell scripts for shellcheck directives..."

    # Create a temporary directory for per-file logs
    tmp_dir="${LOG_PREFIX}${TIMESTAMP}_${RANDOM}"

    # Parallelize with xargs
    # shellcheck disable=SC2312 # Using xargs to parallelize file processing
    "${PRINTF}" '%s\n' "${SHELL_FILES[@]}" | "${XARGS}" -P 0 -I {} bash -c 'process_file "{}"' > "${tmp_dir}.results" 

    # Aggregate results
    while IFS= read -r line; do
        read -r total with_just without_just file <<< "${line}"
        SHELLCHECK_DIRECTIVE_TOTAL=$(( SHELLCHECK_DIRECTIVE_TOTAL + total ))
        SHELLCHECK_DIRECTIVE_WITH_JUSTIFICATION=$(( SHELLCHECK_DIRECTIVE_WITH_JUSTIFICATION + with_just ))
        SHELLCHECK_DIRECTIVE_WITHOUT_JUSTIFICATION=$(( SHELLCHECK_DIRECTIVE_WITHOUT_JUSTIFICATION + without_just ))
        # Collect any no-justification messages
        if [[ -f "${file}.nojust.log" ]]; then
            cat "${file}.nojust.log" >> "${tmp_dir}_nojust.log"
        fi
    done < "${tmp_dir}.results"

    # Print collected no-justification messages
    if [[ -f "${tmp_dir}_nojust.log" ]]; then
        while IFS= read -r line; do
            print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "${line}"
        done < "${tmp_dir}_nojust.log"
    fi

    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "INFO: Total shellcheck directives: ${SHELLCHECK_DIRECTIVE_TOTAL}"
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "INFO: Directives with justification: ${SHELLCHECK_DIRECTIVE_WITH_JUSTIFICATION}"
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "INFO: Directives without justification: ${SHELLCHECK_DIRECTIVE_WITHOUT_JUSTIFICATION}"

    if [[ "${SHELLCHECK_DIRECTIVE_WITHOUT_JUSTIFICATION}" -eq 0 ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "All ${SHELLCHECK_DIRECTIVE_TOTAL} shellcheck directives have justifications"
    else
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Found ${SHELLCHECK_DIRECTIVE_WITHOUT_JUSTIFICATION} shellcheck directives without justifications"
        EXIT_CODE=1
    fi
else
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "No shell scripts found to check for shellcheck directives"
fi

# Print test completion summary
print_test_completion "${TEST_NAME}" "${TEST_ABBR}" "${TEST_NUMBER}" "${TEST_VERSION}"

# Return status code if sourced, exit if run standalone
${ORCHESTRATION:-false} && return "${EXIT_CODE}" || exit "${EXIT_CODE}"
