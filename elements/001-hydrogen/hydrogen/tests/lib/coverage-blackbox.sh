#!/bin/bash

# Coverage Blackbox Library
# This script provides functions for calculating blackbox test coverage,

# LIBRARY FUNCTIONS
# calculate_blackbox_coverage()

# CHANGELOG
# 1.0.1 - 2025-08-03 - Removed extraneous command -v calls
# 1.0.0 - 2025-07-21 - Initial version with combined coverage functions

# Guard clause to prevent multiple sourcing
[[ -n "${COVERAGE_BLACKBOX_GUARD}" ]] && return 0
export COVERAGE_BLACKBOX_GUARD="true"

# Library metadata
COVERAGE_BLACKBOX_NAME="Coverage Blackbox Library"
COVERAGE_BLACKBOX_VERSION="1.0.1"
print_message "${COVERAGE_BLACKBOX_NAME} ${COVERAGE_BLACKBOX_VERSION}" "info" 2> /dev/null || true

# Sort out directories
PROJECT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && cd ../.. && pwd )"
SCRIPT_DIR="${PROJECT_DIR}/tests"
LIB_DIR="${SCRIPT_DIR}/lib"
BUILD_DIR="${PROJECT_DIR}/build"
TESTS_DIR="${BUILD_DIR}/tests"
RESULTS_DIR="${TESTS_DIR}/results"
DIAGS_DIR="${TESTS_DIR}/diagnostics"
LOGS_DIR="${TESTS_DIR}/logs"
mkdir -p "${BUILD_DIR}" "${TESTS_DIR}" "${RESULTS_DIR}" "${DIAGS_DIR}" "${LOGS_DIR}"

# shellcheck source=tests/lib/coverage-common.sh # Resolve path statically
[[ -n "${COVERAGE_COMMON_GUARD}" ]] || source "${LIB_DIR}/coverage-common.sh"

# Function to collect blackbox test coverage from a specific directory
# Usage: collect_blackbox_coverage_from_dir <coverage_directory> <timestamp>
# Returns: Coverage percentage written to blackbox_coverage.txt
calculate_blackbox_coverage() {
    local coverage_dir="$1"
    local timestamp="$2"
    local coverage_percentage="0.000"
    
    if [[ ! -d "${coverage_dir}" ]]; then
        echo "0.000" > "${BLACKBOX_COVERAGE_FILE}"
        echo "${timestamp},0.000,0,0,0,0" > "${BLACKBOX_COVERAGE_FILE}.detailed"
        echo "0.000"
        return 1
    fi
    
    # Load ignore patterns once
    local project_root="${PWD}"
    declare -a BLACKBOX_IGNORE_PATTERNS=()
    if [[ -f "${project_root}/.trial-ignore" ]]; then
        while IFS= read -r line; do
            # Skip comments and empty lines
            if [[ "${line}" =~ ^[[:space:]]*# ]] || [[ -z "${line}" ]]; then
                continue
            fi
            # Remove leading ./ if present
            pattern="${line#./}"
            BLACKBOX_IGNORE_PATTERNS+=("${pattern}")
        done < "${project_root}/.trial-ignore"
    fi
    
    # Change to coverage directory to process gcov files
    local original_dir="${PWD}"
    cd "${coverage_dir}" || return 1
    
    # Generate gcov files only for those that need updating
    declare -a to_process_dirs=()
    declare -a to_process_bases=()
    while IFS= read -r gcda; do
        if [[ -z "${gcda}" ]]; then continue; fi
        local dir="${gcda%/*}"
        local base="${gcda##*/}"
        base="${base%.gcda}"
        local gcno="${dir}/${base}.gcno"
        local gcov_file="${dir}/${base}.gcov"
        
        # shellcheck disable=SC2235 # I like it like this, thanks
        if [[ -f "${gcov_file}" && "${gcov_file}" -nt "${gcda}" ]] && ([[ ! -f "${gcno}" ]] || [[ "${gcov_file}" -nt "${gcno}" ]]); then
            continue
        fi
        
        to_process_dirs+=("${dir}")
        to_process_bases+=("${base}")
    done < <(find . -type f -name "*.gcda" 2>/dev/null || true)
    
    if [[ ${#to_process_bases[@]} -gt 0 ]]; then
        for i in "${!to_process_bases[@]}"; do
            pushd "${to_process_dirs[i]}" >/dev/null || continue
            gcov -b -c "${to_process_bases[i]}" >/dev/null 2>&1 || true
            popd >/dev/null || continue
        done
    fi
    
    # Return to original directory
    cd "${original_dir}" || return 1
    
    # Parse gcov output to calculate overall coverage percentage
    local total_lines=0
    local covered_lines=0
    local instrumented_files=0
    local covered_files=0
    
    # Process gcov files to extract coverage data, excluding external libraries and ignored files
    local gcov_files_to_process=()
    while IFS= read -r gcov_file; do
        if [[ -f "${gcov_file}" ]]; then
            # Use parameter expansion instead of basename
            local basename_file="${gcov_file##*/}"
            
            # Skip Unity framework files and system files
            if [[ "${basename_file}" == "unity.c.gcov" ]] || [[ "${gcov_file}" == *"/usr/"* ]]; then
                continue
            fi
            
            # Skip system libraries and external dependencies
            if [[ "${basename_file}" == *"jansson"* ]] || \
               [[ "${basename_file}" == *"json"* ]] || \
               [[ "${basename_file}" == *"curl"* ]] || \
               [[ "${basename_file}" == *"ssl"* ]] || \
               [[ "${basename_file}" == *"crypto"* ]] || \
               [[ "${basename_file}" == *"pthread"* ]] || \
               [[ "${basename_file}" == *"uuid"* ]] || \
               [[ "${basename_file}" == *"zlib"* ]] || \
               [[ "${basename_file}" == *"pcre"* ]]; then
                continue
            fi
            
            # Check if this file should be ignored based on .trial-ignore patterns
            local source_file="${basename_file%.gcov}"
            local should_ignore=false
            for pattern in "${BLACKBOX_IGNORE_PATTERNS[@]}"; do
                if [[ "${source_file}" == *"${pattern}"* ]]; then
                    should_ignore=true
                    break
                fi
            done
            
            if [[ "${should_ignore}" == true ]]; then
                continue
            fi
            
            gcov_files_to_process+=("${gcov_file}")
        fi
    done < <(find "${coverage_dir}" -name "*.gcov" -type f 2>/dev/null || true)
    
    # Count total files
    instrumented_files=${#gcov_files_to_process[@]}
    
    # Process all gcov files
    if [[ ${#gcov_files_to_process[@]} -gt 0 ]]; then
        # Create temporary combined file
        local combined_gcov="/tmp/combined_blackbox.gcov"
        cat "${gcov_files_to_process[@]}" > "${combined_gcov}"
        
        # Count only instrumented lines
        local line_counts
        line_counts=$(awk '
            /^[ \t]*[0-9]+\*?:[ \t]*[0-9]+:/ { covered++; total++ }
            /^[ \t]*#####:[ \t]*[0-9]+\*?:/ { total++ }
            END { print total "," covered }
        ' "${combined_gcov}" 2>/dev/null)
        
        total_lines="${line_counts%,*}"
        covered_lines="${line_counts#*,}"
        
        total_lines=${total_lines:-0}
        covered_lines=${covered_lines:-0}
        
        # Count files individually for file statistics
        covered_files=0
        for gcov_file in "${gcov_files_to_process[@]}"; do
            file_covered_lines=$(grep -c "^[ \t]*[1-9][0-9]*:[ \t]*[0-9][0-9]*:" "${gcov_file}" 2>/dev/null)
            if [[ -z "${file_covered_lines}" ]] || [[ ! "${file_covered_lines}" =~ ^[0-9]+$ ]]; then
                file_covered_lines=0
            fi
            if [[ ${file_covered_lines} -gt 0 ]]; then
                covered_files=$((covered_files + 1))
            fi
        done
        
        # Clean up temporary file
        rm -f "${combined_gcov}"
    fi
    
    # Calculate coverage percentage with 3 decimal places
    if [[ ${total_lines} -gt 0 ]]; then
        coverage_percentage=$(awk "BEGIN {printf \"%.3f\", (${covered_lines} / ${total_lines}) * 100}")
    fi
    
    # Store coverage result with timestamp
    echo "${coverage_percentage}" > "${BLACKBOX_COVERAGE_FILE}"
    echo "${timestamp},${coverage_percentage},${covered_lines},${total_lines},${instrumented_files},${covered_files}" > "${BLACKBOX_COVERAGE_FILE}.detailed"
    
    echo "${coverage_percentage}"
    return 0
}