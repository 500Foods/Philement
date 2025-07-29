#!/bin/bash

# Coverage Unity Library
# This script provides functions for calculating Unity test coverage from gcov data.

# LIBRARY FUNCTIONS
# calculate_unity_coverage()

# CHANGELOG
# 1.0.0 - 2025-07-21 - Initial version with Unity coverage calculation functions

# Guard clause to prevent multiple sourcing
[[ -n "${COVERAGE_UNITY_GUARD}" ]] && return 0
export COVERAGE_UNITY_GUARD="true"

# Library metadata
COVERAGE_UNITY_NAME="Coverage Unity Library"
COVERAGE_UNITY_VERSION="1.0.0"
print_message "${COVERAGE_UNITY_NAME} ${COVERAGE_UNITY_VERSION}" "info" 2> /dev/null || true

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

# Function to calculate Unity test coverage from gcov data
# Usage: calculate_unity_coverage <build_dir> <timestamp>
# Returns: Coverage percentage written to unity_coverage.txt
calculate_unity_coverage() {
    local build_dir="$1"
    local timestamp="$2"
    local coverage_percentage="0.000"
    
    if [[ ! -d "${build_dir}" ]]; then
        echo "0.000" > "${UNITY_COVERAGE_FILE}"
        echo "${timestamp},0.000,0,0,0,0" > "${UNITY_COVERAGE_FILE}.detailed"
        echo "0.000"
        return 1
    fi
    
    # Change to build directory to process gcov files
    local original_dir="${PWD}"
    cd "${build_dir}" || return 1
    
    # Generate gcov files more efficiently using parallel processing
    if command -v xargs >/dev/null 2>&1; then
        # Use xargs for parallel processing with all available cores
        find . -name "*.gcno" -print0 | xargs -0 -P 0 -I{} sh -c "
            gcno_dir=\"\$(dirname '{}')\"
            cd \"\${gcno_dir}\" && gcov \"\$(basename '{}')\" >/dev/null 2>&1
        " || true
    else
        # Fallback to optimized sequential processing
        find . -name "*.gcno" -exec sh -c '
            gcno_dir="$(dirname "$1")"
            cd "${gcno_dir}" && gcov "$(basename "$1")" >/dev/null 2>&1
        ' _ {} \;
    fi
    
    # Return to original directory
    cd "${original_dir}" || return 1
    
    # Parse gcov output to calculate overall coverage percentage
    local project_root="${original_dir}/.."
    
    # Process gcov files to extract coverage data, excluding external libraries and ignored files
    # This gives us coverage on the actual project code, not external dependencies
    local total_lines=0
    local covered_lines=0
    local instrumented_files=0
    local covered_files=0
    
    # Use exact same filtering logic as Test 11's inline calculation
    local gcov_files_to_process=()
    while IFS= read -r gcov_file; do
        if [[ -f "${gcov_file}" ]]; then
            # Skip Unity framework files and system files (same as Test 11)
            basename_file=$(basename "${gcov_file}")
            if [[ "${basename_file}" == "unity.c.gcov" ]] || [[ "${gcov_file}" == *"/usr/"* ]]; then
                continue
            fi
            
            # Skip system include files that show up in Source: lines
            if grep -q "Source:/usr/include/" "${gcov_file}" 2>/dev/null; then
                continue
            fi
            
            # Skip test files (same as coverage_table.sh)
            if [[ "${basename_file}" == "test_"* ]]; then
                continue
            fi
            
            # Skip system libraries and external dependencies (same as Test 11)
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
            
            # Check if this file should be ignored based on .trial-ignore patterns (same as Test 11)
            source_file="${basename_file%.gcov}"
            should_ignore=false
            
            # Read .trial-ignore patterns directly (same logic as Test 11)
            if [[ -f "${project_root}/.trial-ignore" ]]; then
                while IFS= read -r line; do
                    # Skip comments and empty lines
                    if [[ "${line}" =~ ^[[:space:]]*# ]] || [[ -z "${line}" ]]; then
                        continue
                    fi
                    # Remove leading ./ if present and add pattern
                    pattern="${line#./}"
                    if [[ "${source_file}" == *"${pattern}"* ]]; then
                        should_ignore=true
                        break
                    fi
                done < "${project_root}/.trial-ignore"
            fi
            
            if [[ "${should_ignore}" == true ]]; then
                continue
            fi
            
            gcov_files_to_process+=("${gcov_file}")
        fi
    done < <(find "${build_dir}" -name "*.gcov" -type f 2>/dev/null || true)
    
    # Count total files exactly like Test 11
    instrumented_files=${#gcov_files_to_process[@]}
    
    # Process all gcov files exactly like Test 11 does
    if [[ ${#gcov_files_to_process[@]} -gt 0 ]]; then
        # Create temporary combined file (same as Test 11)
        local combined_gcov="/tmp/combined_unity.gcov"
        cat "${gcov_files_to_process[@]}" > "${combined_gcov}"
        
        # Count only instrumented lines using same method as Test 11
        local line_counts
        line_counts=$(awk '
            /^[ \t]*[0-9]+\*?:[ \t]*[0-9]+:/ { covered++; total++ }
            /^[ \t]*#####:[ \t]*[0-9]+\*?:/ { total++ }
            END { print total "," covered }
        ' "${combined_gcov}" 2>/dev/null)
        
        total_lines="${line_counts%,*}"
        covered_lines="${line_counts#*,}"
        
        # Default to 0 if parsing failed
        total_lines=${total_lines:-0}
        covered_lines=${covered_lines:-0}
        
        # Count files individually for file statistics (same as Test 11)
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
    echo "${coverage_percentage}" > "${UNITY_COVERAGE_FILE}"
    echo "${timestamp},${coverage_percentage},${covered_lines},${total_lines},${instrumented_files},${covered_files}" > "${UNITY_COVERAGE_FILE}.detailed"
    
    echo "${coverage_percentage}"
    return 0
}
