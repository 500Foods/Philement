#!/bin/bash

# Test: Shell Script Analysis
# Performs shellcheck analysis and exception justification checking

# CHANGELOG
# 3.1.0 - 2025-07-28 - Optimized caching, removed md5sum/wc spawns, single shellcheck run
# 3.0.0 - 2025-07-18 - Added caching mechanism for shellcheck results
# 2.0.1 - 2025-07-18 - Fixed subshell issue in shellcheck output
# 2.0.0 - 2025-07-14 - Upgraded to modular test framework
# 1.0.0 - Initial version

# Test configuration
TEST_NAME="Shell Linting"
SCRIPT_VERSION="3.1.0"

# Sort out directories
PROJECT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && cd .. && pwd )"
# CMAKE_DIR="${PROJECT_DIR}/cmake"
SCRIPT_DIR="${PROJECT_DIR}/tests"
LIB_DIR="${SCRIPT_DIR}/lib"
BUILD_DIR="${PROJECT_DIR}/build"
TESTS_DIR="${BUILD_DIR}/tests"
RESULTS_DIR="${TESTS_DIR}/results"
DIAGS_DIR="${TESTS_DIR}/diagnostics"
LOGS_DIR="${TESTS_DIR}/logs"
mkdir -p "${BUILD_DIR}" "${TESTS_DIR}" "${RESULTS_DIR}" "${DIAGS_DIR}" "${LOGS_DIR}"

# shellcheck source=tests/lib/framework.sh # Resolve path statically
[[ -n "${FRAMEWORK_GUARD}" ]] || source "${LIB_DIR}/framework.sh"
# shellcheck source=tests/lib/log_output.sh # Resolve path statically
[[ -n "${LOG_OUTPUT_GUARD}" ]] || source "${LIB_DIR}/log_output.sh"

# Test configuration
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
EXIT_CODE=0
PASS_COUNT=0
TEST_NUMBER=$(extract_test_number "${BASH_SOURCE[0]}")
RESULT_LOG="${RESULTS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}.log"
set_test_number "${TEST_NUMBER}"
reset_subtest_counter

# Print beautiful test header
print_test_header "${TEST_NAME}" "${SCRIPT_VERSION}"

# Print framework and log output versions as they are already sourced
[[ -n "${ORCHESTRATION}" ]] || print_message "${FRAMEWORK_NAME} ${FRAMEWORK_VERSION}" "info"
[[ -n "${ORCHESTRATION}" ]] || print_message "${LOG_OUTPUT_NAME} ${LOG_OUTPUT_VERSION}" "info"
# shellcheck source=tests/lib/file_utils.sh # Resolve path statically
[[ -n "${FILE_UTILS_GUARD}" ]] || source "${LIB_DIR}/file_utils.sh"

# Navigate to project root
if ! navigate_to_project_root "${SCRIPT_DIR}"; then
    print_error "Failed to navigate to project root directory"
    exit 1
fi

# Test configuration constants
[[ -z "${LINT_OUTPUT_LIMIT:-}" ]] && readonly LINT_OUTPUT_LIMIT=10
[[ -z "${LINT_EXCLUDES:-}" ]] && readonly LINT_EXCLUDES=(
    "build/*"
)

# Check if a file should be excluded from linting
should_exclude_file() {
    local file="$1"
    local lint_ignore=".lintignore"
    local rel_file="${file#./}"

    # Check .lintignore
    if [[ -f "${lint_ignore}" ]]; then
        while IFS= read -r pattern; do
            [[ -z "${pattern}" || "${pattern}" == \#* ]] && continue
            clean_pattern="${pattern%/*}"
            if [[ "${rel_file}" == "${pattern}" || "${rel_file}" == "${clean_pattern}"/* ]]; then
                return 0
            fi
        done < "${lint_ignore}"
    fi

    # Check default excludes
    for pattern in "${LINT_EXCLUDES[@]}"; do
        clean_pattern="${pattern%/*}"
        if [[ "${rel_file}" == "${pattern}" || "${rel_file}" == "${clean_pattern}"/* ]]; then
            return 0
        fi
    done
    return 1
}

# Subtest 1: Lint Shell scripts
next_subtest
print_subtest "Shell Script Linting (shellcheck)"

print_message "Detected ${CORES} CPU cores for parallel processing"

if command -v shellcheck >/dev/null 2>&1; then
    SHELL_FILES=()
    TEST_SHELL_FILES=()
    OTHER_SHELL_FILES=()
    while read -r file; do
        if ! should_exclude_file "${file}"; then
            SHELL_FILES+=("${file}")
            if [[ "${file}" == ./tests/* ]]; then
                TEST_SHELL_FILES+=("${file}")
            else
                OTHER_SHELL_FILES+=("${file}")
            fi
        fi
    done < <(find . -type f -name "*.sh")

    SHELL_COUNT=${#SHELL_FILES[@]}
    SHELL_ISSUES=0
    TEMP_OUTPUT=$(mktemp)

    if [ "${SHELL_COUNT}" -gt 0 ]; then
        print_message "Running shellcheck on ${SHELL_COUNT} shell scripts with caching..."
        TEST_NAME="${TEST_NAME} {BLUE}(shellheck: ${SHELL_COUNT} files){RESET}"

        # Cache directory
        CACHE_DIR="${HOME}/.cache/.shellcheck"
        mkdir -p "${CACHE_DIR}"

        # Batch compute content hashes for all files
        declare -A file_hashes
        while read -r hash file; do
            file_hashes["${file}"]="${hash}"
        done < <(md5sum "${SHELL_FILES[@]}")

        cached_files=0
        processed_scripts=0
        to_process=()

        for file in "${SHELL_FILES[@]}"; do
            content_hash="${file_hashes[${file}]}"
            # Flatten path for uniqueness (replace / with _)
            flat_path=$(echo "${file}" | tr '/' '_')
            cache_file="${CACHE_DIR}/${flat_path}_${content_hash}"
            if [ -f "${cache_file}" ]; then
                ((cached_files++))
                cat "${cache_file}" >> "${TEMP_OUTPUT}" 2>&1
            else
                to_process+=("${file}")
                ((processed_scripts++))
            fi
        done

        print_message "Using cached results for ${cached_files} files, processing ${processed_scripts} files..."

        # Function to run shellcheck and cache for a single file
        process_script() {
            local file="$1"
            local content_hash="$2"
            local flat_path=$(echo "${file}" | tr '/' '_')
            local cache_file="${CACHE_DIR}/${flat_path}_${content_hash}"
            shellcheck "${file}" > "${cache_file}" 2>&1
            cat "${cache_file}"
        }

        export -f process_script  # Export for subshells

        # Process non-cached files in parallel, passing file and hash
        if [ ${processed_scripts} -gt 0 ]; then
            for file in "${to_process[@]}"; do
                printf '%s %s\n' "${file}" "${file_hashes[${file}]}"
            done | xargs -n 2 -P "${CORES}" bash -c 'process_scripts "$0" "$1"' >> "${TEMP_OUTPUT}" 2>&1
        fi

        # Filter output
        sed "s|$(pwd)/||g; s|tests/||g" "${TEMP_OUTPUT}" > "${TEMP_OUTPUT}.filtered"

        SHELL_ISSUES=$(wc -l < "${TEMP_OUTPUT}.filtered")
        if [ "${SHELL_ISSUES}" -gt 0 ]; then
            FILES_WITH_ISSUES=$(cut -d: -f1 "${TEMP_OUTPUT}.filtered" | sort -u | wc -l)
            print_message "shellcheck found ${SHELL_ISSUES} issues in ${FILES_WITH_ISSUES} files:"
            while IFS= read -r line; do
                print_output "${line}"
            done < <(head -n "${LINT_OUTPUT_LIMIT}" "${TEMP_OUTPUT}.filtered")
            if [ "${SHELL_ISSUES}" -gt "${LINT_OUTPUT_LIMIT}" ]; then
                print_message "Output truncated. Showing ${LINT_OUTPUT_LIMIT} of ${SHELL_ISSUES} lines."
            fi
        fi

        rm -f "${TEMP_OUTPUT}" "${TEMP_OUTPUT}.filtered"
    fi

    if [ "${SHELL_ISSUES}" -eq 0 ]; then
        print_result 0 "No issues in ${SHELL_COUNT} shell files"
        ((PASS_COUNT++))
    else
        print_result 1 "Found ${SHELL_ISSUES} issues in shell files"
        EXIT_CODE=1
    fi
else
    print_result 0 "shellcheck not available (skipped)"
    ((PASS_COUNT++))
fi

# Subtest 2: Shellcheck Exception Justification Check
next_subtest
print_subtest "Shellcheck Exception Justification Check"

print_message "Checking for shellcheck directives in shell scripts..."

SHELLCHECK_DIRECTIVE_TOTAL=0
SHELLCHECK_DIRECTIVE_WITH_JUSTIFICATION=0
SHELLCHECK_DIRECTIVE_WITHOUT_JUSTIFICATION=0

# Function to process a single file and output counters + messages
process_file() {
    local file="$1"
    local total=0 with_just=0 without_just=0
    while IFS= read -r line; do
        if [[ "${line}" =~ ^[[:space:]]*"# shellcheck" ]]; then
            ((total++))
            if [[ "${line}" =~ ^[[:space:]]*"# shellcheck"[[:space:]]+[^[:space:]]+[[:space:]]+"#".* ]]; then
                ((with_just++))
            else
                ((without_just++))
                # Output to a file-specific log to avoid interleaving
                echo "No justification found in ${file}: ${line}" >> "${file}.nojust.log"
            fi
        fi
    done < "${file}"
    # Output counters in a parseable format
    echo "${total} ${with_just} ${without_just} ${file}"
}

export -f process_file

if [ ${#SHELL_FILES[@]} -gt 0 ]; then
    print_message "Analyzing ${SHELL_COUNT} shell scripts for shellcheck directives..."

    # Create a temporary directory for per-file logs
    tmp_dir=$(mktemp -d)
    trap 'rm -rf "${tmp_dir}"' EXIT

    # Parallelize with xargs, using 12 processes
    printf '%s\n' "${SHELL_FILES[@]}" | xargs -P 0 -I {} bash -c 'process_file "{}"' > "${tmp_dir}/results"

    # Aggregate results
    while IFS= read -r line; do
        read -r total with_just without_just file <<< "${line}"
        ((SHELLCHECK_DIRECTIVE_TOTAL += total))
        ((SHELLCHECK_DIRECTIVE_WITH_JUSTIFICATION += with_just))
        ((SHELLCHECK_DIRECTIVE_WITHOUT_JUSTIFICATION += without_just))
        # Collect any no-justification messages
        if [ -f "${file}.nojust.log" ]; then
            cat "${file}.nojust.log" >> "${tmp_dir}/nojust.log"
            rm "${file}.nojust.log"
        fi
    done < "${tmp_dir}/results"

    # Print collected no-justification messages
    if [ -f "${tmp_dir}/nojust.log" ]; then
        while IFS= read -r line; do
            print_output "${line}"
        done < "${tmp_dir}/nojust.log"
    fi

    print_message "INFO: Total shellcheck directives: ${SHELLCHECK_DIRECTIVE_TOTAL}"
    print_message "INFO: Directives with justification: ${SHELLCHECK_DIRECTIVE_WITH_JUSTIFICATION}"
    print_message "INFO: Directives without justification: ${SHELLCHECK_DIRECTIVE_WITHOUT_JUSTIFICATION}"

    if [ "${SHELLCHECK_DIRECTIVE_WITHOUT_JUSTIFICATION}" -eq 0 ]; then
        print_result 0 "All ${SHELLCHECK_DIRECTIVE_TOTAL} shellcheck directives have justifications"
        ((PASS_COUNT++))
    else
        print_result 1 "Found ${SHELLCHECK_DIRECTIVE_WITHOUT_JUSTIFICATION} shellcheck directives without justifications"
        EXIT_CODE=1
    fi
else
    print_result 0 "No shell scripts found to check for shellcheck directives"
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
