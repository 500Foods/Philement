#!/bin/bash

# Coverage Combined Library
# Functions for calculating combined coverage analysis, and identifying uncovered files.

# LIBRARY FUNCTIONS
# calculate_combined_coverage()
# calculate_coverage_overlap()
# identify_uncovered_files()

# CHANGELOG
# 1.0.0 - 2025-07-21 - Initial version with combined coverage functions

# Guard clause to prevent multiple sourcing
[[ -n "${COVERAGE_COMBINED_GUARD}" ]] && return 0
export COVERAGE_COMBINED_GUARD="true"

# Library metadata
COVERAGE_COMBINED_NAME="Coverage Combined Library"
COVERAGE_COMBINED_VERSION="1.0.0"
print_message "${COVERAGE_COMBINED_NAME} ${COVERAGE_COMBINED_VERSION}" "info" 2> /dev/null || true

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

# Function to calculate combined coverage from Unity and blackbox tests
# Usage: calculate_combined_coverage
# Returns: Combined coverage percentage
calculate_combined_coverage() {
    local project_root
    project_root="$(cd "${SCRIPT_DIR}/../.." && pwd)"
    local unity_build_dir="${project_root}/build/unity"
    local blackbox_build_dir="${project_root}/build/coverage"
    
    local total_combined_instrumented=0
    local total_combined_covered=0
    
    # Get all source files and process matching gcov pairs
    while IFS= read -r source_file; do
        if should_ignore_file "${source_file}" "${project_root}"; then
            continue
        fi
        
        local source_basename
        source_basename=$(basename "${source_file}")
        local unity_gcov="${unity_build_dir}/src/${source_basename}.gcov"
        local blackbox_gcov="${blackbox_build_dir}/src/${source_basename}.gcov"
        
        # Find actual gcov files (they might be in subdirectories)
        [[ ! -f "${unity_gcov}" ]] && unity_gcov=$(find "${unity_build_dir}" -name "${source_basename}.gcov" -type f 2>/dev/null | head -1 || true)
        [[ ! -f "${blackbox_gcov}" ]] && blackbox_gcov=$(find "${blackbox_build_dir}" -name "${source_basename}.gcov" -type f 2>/dev/null | head -1 || true)
        
        # Skip if neither gcov file exists
        if [[ ! -f "${unity_gcov}" && ! -f "${blackbox_gcov}" ]]; then
            continue
        fi
        
        # Use our new combined analysis function
        local result
        result=$(analyze_combined_gcov_coverage "${unity_gcov}" "${blackbox_gcov}")
        
        local file_instrumented="${result%,*}"
        local file_combined_covered="${result#*,}"
        
        # Default to 0 if parsing failed
        file_instrumented=${file_instrumented:-0}
        file_combined_covered=${file_combined_covered:-0}
        
        total_combined_instrumented=$((total_combined_instrumented + file_instrumented))
        total_combined_covered=$((total_combined_covered + file_combined_covered))
        
    done < <(get_cached_source_files "${project_root}" || true)
    
    # Calculate combined coverage percentage
    local combined_coverage="0.000"
    if [[ ${total_combined_instrumented} -gt 0 ]]; then
        combined_coverage=$(awk "BEGIN {printf \"%.3f\", (${total_combined_covered} / ${total_combined_instrumented}) * 100}")
    fi
    
    # Store combined coverage result
    echo "${combined_coverage}" > "${COMBINED_COVERAGE_FILE}"
    echo "${combined_coverage}"
    return 0
}

# Function to calculate coverage overlap between Unity and blackbox tests
# Usage: calculate_coverage_overlap
# Returns: Overlap percentage between the two test types
calculate_coverage_overlap() {
    local unity_coverage="0.000"
    local blackbox_coverage="0.000"
    local overlap_percentage="0.000"
    
    # Read coverage data from files
    if [[ -f "${UNITY_COVERAGE_FILE}" ]]; then
        unity_coverage=$(cat "${UNITY_COVERAGE_FILE}" 2>/dev/null || echo "0.000")
    fi
    
    if [[ -f "${BLACKBOX_COVERAGE_FILE}" ]]; then
        blackbox_coverage=$(cat "${BLACKBOX_COVERAGE_FILE}" 2>/dev/null || echo "0.000")
    fi
    
    # Calculate overlap as the minimum of the two coverage percentages
    # This represents the files/lines that are covered by both test types
    if [[ $(awk "BEGIN {print (${unity_coverage} < ${blackbox_coverage})}" || true) -eq 1 ]]; then
        overlap_percentage="${unity_coverage}"
    else
        overlap_percentage="${blackbox_coverage}"
    fi
    
    # Store overlap coverage result
    echo "${overlap_percentage}" > "${OVERLAP_COVERAGE_FILE}"
    
    echo "${overlap_percentage}"
    return 0
}

# Function to identify uncovered source files
# Usage: identify_uncovered_files <project_root>
# Returns: Analysis of covered and uncovered source files
identify_uncovered_files() {
    local project_root="$1"
    local covered_count=0
    local uncovered_count=0
    local total_count=0
    local uncovered_files=()
    
    # For Test 99, only check blackbox coverage directory (build/coverage)
    local build_dirs=()
    build_dirs+=("${project_root}/build/coverage")
    
    # Cache all gcov files first to avoid repeated filesystem traversals
    declare -A gcov_cache
    for build_dir in "${build_dirs[@]}"; do
        if [[ -d "${build_dir}" ]]; then
            while IFS= read -r gcov_file; do
                local gcov_basename
                gcov_basename=$(basename "${gcov_file}")
                # Store the first found gcov file for each basename
                if [[ -z "${gcov_cache["${gcov_basename}"]}" ]]; then
                    gcov_cache["${gcov_basename}"]="${gcov_file}"
                fi
            done < <(find "${build_dir}" -name "*.gcov" -type f 2>/dev/null || true)
        fi
    done
    
    # Process all source files using cached gcov data and cached source files
    while IFS= read -r source_file; do
        if should_ignore_file "${source_file}" "${project_root}"; then
            continue
        fi
        
        total_count=$((total_count + 1))
        
        # Check if this source file has corresponding gcov coverage using cache
        local source_basename
        source_basename=$(basename "${source_file}")
        local gcov_file="${gcov_cache["${source_basename}.gcov"]}"
        local found_coverage=false
        
        if [[ -n "${gcov_file}" && -f "${gcov_file}" ]]; then
            # Check if the file has any coverage using more efficient approach
            if grep -q "^[ \t]*[1-9][0-9]*.*:" "${gcov_file}" 2>/dev/null; then
                found_coverage=true
            fi
        fi
        
        if [[ "${found_coverage}" == true ]]; then
            covered_count=$((covered_count + 1))
        else
            uncovered_count=$((uncovered_count + 1))
            uncovered_files+=("${source_file}")
        fi
    done < <(get_cached_source_files "${project_root}" || true)
    
    # Sort uncovered files for consistent output
    if [[ ${#uncovered_files[@]} -gt 0 ]]; then
        mapfile -t uncovered_files < <(printf '%s\n' "${uncovered_files[@]}" | sort || true)
    fi
    
    # Output results in a structured format
    echo "COVERED_FILES_COUNT:${covered_count}"
    echo "UNCOVERED_FILES_COUNT:${uncovered_count}"
    echo "TOTAL_SOURCE_FILES:${total_count}"
    echo "UNCOVERED_FILES:"
    
    # List uncovered files (now sorted)
    for file in "${uncovered_files[@]}"; do
        echo "${file}"
    done
    
    return 0
}
