#!/usr/bin/env bash

# Test: C/C++ Code Analysis
# Performs cppcheck analysis on C/C++ source files

# FUNCTIONS
# run_cppcheck()

# CHANGELOG
# 3.1.0 - 2025-08-13 - Review, removed custom should_exclude code, minor tewaks
# 3.0.1 - 2025-08-03 - Removed extraneous command -v calls
# 3.0.0 - 2025-07-30 - Overhaul #1
# 2.0.1 - 2025-07-18 - Fixed subshell issue in cppcheck output that prevented detailed error messages from being displayed in test output
# 2.0.0 - 2025-07-14 - Upgraded to use new modular test framework
# 1.0.0 - Initial version for C/C++ code analysis

# Test configuration
TEST_NAME="C Lint"
TEST_ABBR="GCC"
TEST_NUMBER="91"
TEST_VERSION="3.1.0"

# shellcheck source=tests/lib/framework.sh # Reference framework directly
[[ -n "${FRAMEWORK_GUARD}" ]] || source "$(dirname "${BASH_SOURCE[0]}")/lib/framework.sh"
setup_test_environment

# Test Setup
CACHE_DIR="${HOME}/.cache/cppcheck"
mkdir -p "${CACHE_DIR}"

# Function to run cppcheck
run_cppcheck() {
    local target_dir="$1"
    
    # Check for required files
    if [[ ! -f ".lintignore" ]]; then
        print_error ".lintignore not found"
        return 1
    fi
    if [[ ! -f ".lintignore-c" ]]; then
        print_error ".lintignore-c not found"
        return 1
    fi
    
    # Parse .lintignore-c
    local cppcheck_args=()
    while IFS='=' read -r key value; do
        case "${key}" in
            "enable") cppcheck_args+=("--enable=${value}") ;;
            "include") cppcheck_args+=("--include=${value}") ;;
            "check-level") cppcheck_args+=("--check-level=${value}") ;;
            "template") cppcheck_args+=("--template=${value}") ;;
            "option") cppcheck_args+=("${value}") ;;
            "suppress") cppcheck_args+=("--suppress=${value}") ;;
            *) ;;
        esac
    done < <(grep -v '^#' ".lintignore-c" | grep '=' || true)
    
    # Collect files with inline filtering using centralized exclusion function
    local files=()
    while read -r file; do
        rel_file="${file#./}"
        if ! should_exclude_file "${rel_file}"; then
            files+=("${file}")
        fi
    done < <(find . -type f \( -name "*.c" -o -name "*.h" -o -name "*.inc" \) || true)
    
    if [[ ${#files[@]} -gt 0 ]]; then
        cppcheck -j"${CORES}" --quiet --cppcheck-build-dir="${CACHE_DIR}" "${cppcheck_args[@]}" "${files[@]}" 2>&1
    else
        echo ""
    fi
}

print_subtest "C/C++ Code Linting (cppcheck)"
print_message "Detected ${CORES} CPU cores for parallel processing"

# Count files that will be checked (excluding .lintignore patterns)
C_FILES_TO_CHECK=()
while read -r file; do
    rel_file="${file#./}"
    if ! should_exclude_file "${rel_file}"; then
        C_FILES_TO_CHECK+=("${rel_file}")
    fi
done < <(find . -type f \( -name "*.c" -o -name "*.h" -o -name "*.inc" \) || true)

C_COUNT=${#C_FILES_TO_CHECK[@]}
print_message "Running cppcheck on ${C_COUNT} files..."
TEST_NAME="${TEST_NAME} {BLUE}(cppcheck: ${C_COUNT} files){RESET}"    

CPPCHECK_OUTPUT=$(run_cppcheck ".")

# Check for expected vs unexpected issues
EXPECTED_WARNING="warning: Possible null pointer dereference: ptr"
HAS_EXPECTED=0
OTHER_ISSUES=0

while IFS= read -r line; do
    if [[ "${line}" =~ ^🛈 ]]; then
        continue
    elif [[ "${line}" == *"${EXPECTED_WARNING}"* ]]; then
        HAS_EXPECTED=1
    elif [[ "${line}" =~ /.*: ]]; then
        ((OTHER_ISSUES++))
    fi
done <<< "${CPPCHECK_OUTPUT}"

# Display output
if [[ -n "${CPPCHECK_OUTPUT}" ]]; then
    print_message "cppcheck output:"
    # Use process substitution to avoid subshell issue with OUTPUT_COLLECTION
    while IFS= read -r line; do
        print_output "${line}"
    done < <(echo "${CPPCHECK_OUTPUT}")
fi

if [[ ${OTHER_ISSUES} -gt 0 ]]; then
    print_result 1 "Found ${OTHER_ISSUES} unexpected issues in ${C_COUNT} files"
    EXIT_CODE=1
else
    if [[ ${HAS_EXPECTED} -eq 1 ]]; then
        print_result 0 "Found 1 expected warning in ${C_COUNT} files"
    else
        print_result 0 "No issues found in ${C_COUNT} files"
    fi
    ((PASS_COUNT++))
fi

# Print test completion summary
print_test_completion "${TEST_NAME}" "${TEST_ABBR}" "${TEST_NUMBER}" "${TEST_VERSION}"

# Return status code if sourced, exit if run standalone
${ORCHESTRATION:-false} && return "${EXIT_CODE}" || exit "${EXIT_CODE}"
