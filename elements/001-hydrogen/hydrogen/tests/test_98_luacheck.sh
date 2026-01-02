#!/usr/bin/env bash
 
# Test: Lua Code Analysis
# Performs luacheck analysis on Lua source files
 
# FUNCTIONS
# run_luacheck()
 
# CHANGELOG
# 1.1.0 - 2026-01-01 - Added HELIUM_ROOT Lua files to analysis
# 1.0.0 - 2025-10-15 - Initial version for Lua code analysis based on test_91_cppcheck.sh

set -euo pipefail

# Test configuration
TEST_NAME="Lua Lint"
TEST_ABBR="LUA"
TEST_NUMBER="98"
TEST_COUNTER=0
TEST_VERSION="1.0.0"

# shellcheck source=tests/lib/framework.sh # Reference framework directly
[[ -n "${FRAMEWORK_GUARD:-}" ]] || source "$(dirname "${BASH_SOURCE[0]}")/lib/framework.sh"
setup_test_environment

# Test Setup
CACHE_DIR="${HOME}/.cache/hydrogen/luacheck"
mkdir -p "${CACHE_DIR}"

# Function to run luacheck and get cache statistics
run_luacheck() {
    local target_dir="$1"

    # Check for required files
    if [[ ! -f ".lintignore" ]]; then
        print_error "${TEST_NUMBER}" "${TEST_COUNTER}" ".lintignore not found"
        return 1
    fi
    if [[ ! -f ".lintignore-lua" ]]; then
        print_error "${TEST_NUMBER}" "${TEST_COUNTER}" ".lintignore-lua not found"
        return 1
    fi

    # Parse .lintignore-lua
    local luacheck_args=()
    while IFS='=' read -r key value; do
        case "${key}" in
            "std") luacheck_args+=("--std" "${value}") ;;
            "max-line-length") luacheck_args+=("--max-line-length" "${value}") ;;
            "max-cyclomatic-complexity") luacheck_args+=("--max-cyclomatic-complexity" "${value}") ;;
            "global") luacheck_args+=("--globals" "${value}") ;;
            "ignore") luacheck_args+=("--ignore" "${value}") ;;
            "option") luacheck_args+=("${value}") ;;
            *) ;;
        esac
    done < <("${GREP}" -v '^#' ".lintignore-lua" | "${GREP}" '=' || true)

    # Collect files with inline filtering using centralized exclusion function
    local files=()
    
    # First, collect files from current directory (HYDROGEN_ROOT)
    while read -r file; do
        rel_file="${file#./}"
        # shellcheck disable=SC2310 # We want to continue even if the test fails
        if ! should_exclude_file "${rel_file}"; then
            files+=("${file}")
        fi
    done < <("${FIND}" . -type f -name "*.lua" || true)
    
    # Then, collect files from HELIUM_ROOT
    if [[ -d "${HELIUM_ROOT}" ]]; then
        while read -r file; do
            # Convert HELIUM_ROOT path to relative path for exclusion checking
            rel_file="${file#${HELIUM_ROOT}/}"
            rel_file="helium/${rel_file}"  # Add helium prefix to distinguish from hydrogen files
            # shellcheck disable=SC2310 # We want to continue even if the test fails
            if ! should_exclude_file "${rel_file}"; then
                files+=("${file}")
            fi
        done < <("${FIND}" "${HELIUM_ROOT}" -type f -name "*.lua" || true)
    fi

    if [[ ${#files[@]} -gt 0 ]]; then
        # Calculate modified files since last run
        local modified_count=0
        if [[ -f "${CACHE_DIR}/last_run" ]]; then
            for file in "${files[@]}"; do
                if [[ "${file}" -nt "${CACHE_DIR}/last_run" ]]; then
                    modified_count=$(( modified_count + 1 ))
                fi
            done
        else
            modified_count=${#files[@]}
        fi

        # Cache hits = total files - modified files
        local cache_hits=$(( ${#files[@]} - modified_count ))

        # Run luacheck to get issues
        local output
        output=$(luacheck "${luacheck_args[@]}" "${files[@]}" 2>&1 || true)

        # Update last run timestamp
        touch "${CACHE_DIR}/last_run"

        # Return cache_hits and output separated by a pipe delimiter
        echo "${cache_hits}|${output}"
    else
        echo "0|||"
    fi
}

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Lua Code Linting (luacheck)"
print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Detected ${CORES} CPU cores for parallel processing"

# Count files that will be checked (excluding .lintignore patterns)
LUA_FILES_TO_CHECK=()

# Count files from current directory (HYDROGEN_ROOT)
while read -r file; do
    rel_file="${file#./}"
    # shellcheck disable=SC2310 # We want to continue even if the test fails
    if ! should_exclude_file "${rel_file}"; then
        LUA_FILES_TO_CHECK+=("${rel_file}")
    fi
done < <("${FIND}" . -type f -name "*.lua" || true)

# Count files from HELIUM_ROOT
if [[ -d "${HELIUM_ROOT}" ]]; then
    while read -r file; do
        rel_file="${file#${HELIUM_ROOT}/}"
        rel_file="helium/${rel_file}"  # Add helium prefix to distinguish from hydrogen files
        # shellcheck disable=SC2310 # We want to continue even if the test fails
        if ! should_exclude_file "${rel_file}"; then
            LUA_FILES_TO_CHECK+=("${rel_file}")
        fi
    done < <("${FIND}" "${HELIUM_ROOT}" -type f -name "*.lua" || true)
fi

LUA_COUNT=${#LUA_FILES_TO_CHECK[@]}
print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Evaluating ${LUA_COUNT} files..."

TEST_NAME="${TEST_NAME}  {BLUE}luacheck: ${LUA_COUNT} files{RESET}"

# Run luacheck once to get both cache stats and issues
COMBINED_OUTPUT=$(run_luacheck ".")

# Parse the combined output
CACHE_HITS="${COMBINED_OUTPUT%%|*}"
LUACHECK_OUTPUT="${COMBINED_OUTPUT#*|}"

FILES_TO_RUN=$(( LUA_COUNT - CACHE_HITS ))

print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Found ${CACHE_HITS} files of ${LUA_COUNT} files in cache..."
print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Running luacheck on ${FILES_TO_RUN} / ${LUA_COUNT} files..."

# Check for issues
ISSUE_COUNT=0

# Count lines that contain actual luacheck issues (format: filename:line:column: message)
while IFS= read -r line; do
    if [[ "${line}" =~ ^[^:]+:[0-9]+:[0-9]+: ]]; then
        ISSUE_COUNT=$(( ISSUE_COUNT + 1 ))
    fi
done <<< "${LUACHECK_OUTPUT}"

# Display output
if [[ -n "${LUACHECK_OUTPUT}" ]]; then
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "luacheck output:"
    # Use process substitution to avoid subshell issue with OUTPUT_COLLECTION
    while IFS= read -r line; do
        print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "${line}"
    done < <(echo "${LUACHECK_OUTPUT}")
fi

if [[ ${ISSUE_COUNT} -gt 0 ]]; then
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Found ${ISSUE_COUNT} issues in ${LUA_COUNT} files"
    EXIT_CODE=1
else
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "No issues found in ${LUA_COUNT} files"
fi

# Print test completion summary
print_test_completion "${TEST_NAME}" "${TEST_ABBR}" "${TEST_NUMBER}" "${TEST_VERSION}"

# Return status code if sourced, exit if run standalone
${ORCHESTRATION:-false} && return "${EXIT_CODE}" || exit "${EXIT_CODE}"