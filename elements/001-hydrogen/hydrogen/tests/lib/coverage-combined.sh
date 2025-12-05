#!/usr/bin/env bash

# Coverage Combined Library
# Functions for calculating combined coverage analysis, and identifying uncovered files.

# LIBRARY FUNCTIONS
# calculate_combined_coverage()
# calculate_coverage_overlap()
# identify_uncovered_files()

# CHANGELOG
# 2.0.0 - 2025-12-05 - Added HYDROGEN_ROOT and HELIUM_ROOT environment variable checks
# 1.1.0 - 2025-08-10 - Added caching to calculate_combined_coverage()
# 1.0.0 - 2025-07-21 - Initial version with combined coverage functions

# Check for required HYDROGEN_ROOT environment variable
if [[ -z "${HYDROGEN_ROOT:-}" ]]; then
    echo "❌ Error: HYDROGEN_ROOT environment variable is not set"
    echo "Please set HYDROGEN_ROOT to the Hydrogen project's root directory"
    exit 1
fi                         

# Check for required HELIUM_ROOT environment variable
if [[ -z "${HELIUM_ROOT:-}" ]]; then
    echo "❌ Error: HELIUM_ROOT environment variable is not set"
    echo "Please set HELIUM_ROOT to the Helium project's root directory"
    exit 1
fi

# Guard clause to prevent multiple sourcing
[[ -n "${COVERAGE_COMBINED_GUARD:-}" ]] && return 0
export COVERAGE_COMBINED_GUARD="true"

# Library metadata
COVERAGE_COMBINED_NAME="Coverage Combined Library"
COVERAGE_COMBINED_VERSION="2.0.0"
# shellcheck disable=SC2154 # TEST_NUMBER and TEST_COUNTER defined by caller
print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${COVERAGE_COMBINED_NAME} ${COVERAGE_COMBINED_VERSION}" "info" 2> /dev/null || true

# Sort out directories
PROJECT_DIR="${HYDROGEN_ROOT}"
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
# Function to calculate combined coverage
calculate_combined_coverage() {
    local project_root
    project_root="$(cd "${SCRIPT_DIR}/../.." && pwd)"
    local unity_build_dir="${project_root}/build/unity"
    local blackbox_build_dir="${project_root}/build/coverage"
    local timestamp
    # shellcheck disable=SC2154 # DATE defined externally in framework.sh    
    timestamp=$("${DATE}" +"%Y-%m-%d %H:%M:%S")
    local marker_file="${BUILD_DIR}/combined_coverage_marker"
    local coverage_file="${COMBINED_COVERAGE_FILE}"

    # Check if cache marker and coverage file exist
    if [[ -f "${marker_file}" && -f "${coverage_file}" ]]; then
        cat "${coverage_file}" 2>/dev/null || echo "0.000"
        return 0
    fi

    # Use the efficient wrapper functions from coverage.sh
    local unity_coverage
    local blackbox_coverage
    unity_coverage=$(calculate_unity_coverage "${unity_build_dir}" "${timestamp}")
    blackbox_coverage=$(calculate_blackbox_coverage "${blackbox_build_dir}" "${timestamp}")
    
    # Use the efficient batch analysis function for true combined coverage
    analyze_all_gcov_coverage_batch "${unity_build_dir}" "${blackbox_build_dir}"
    
    # Calculate combined coverage from the batch analysis results
    local total_combined_instrumented=0
    local total_combined_covered=0
    
    # Process the combined coverage data from global arrays set by analyze_all_gcov_coverage_batch
    for file_path in "${!combined_instrumented_lines[@]}"; do
        local file_instrumented="${combined_instrumented_lines[${file_path}]:-0}"
        local file_covered="${combined_covered_lines[${file_path}]:-0}"
        
        total_combined_instrumented=$((total_combined_instrumented + file_instrumented))
        total_combined_covered=$((total_combined_covered + file_covered))
    done
    
    # Calculate combined coverage percentage
    local combined_coverage="0.000"
    if [[ ${total_combined_instrumented} -gt 0 ]]; then
        combined_coverage=$("${AWK}" "BEGIN {printf \"%.3f\", (${total_combined_covered} / ${total_combined_instrumented}) * 100}")
    fi
    
    # Store combined coverage result and create marker
    echo "${combined_coverage}" > "${coverage_file}"
    touch "${marker_file}"
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
    if [[ $("${AWK}" "BEGIN {print (${unity_coverage} < ${blackbox_coverage})}" || true) -eq 1 ]]; then
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
    local timestamp
    timestamp=$("${DATE}" +"%Y-%m-%d %H:%M:%S")
    
    # Use efficient blackbox coverage calculation to ensure gcov files are up to date
    local blackbox_build_dir="${project_root}/build/coverage"
    calculate_blackbox_coverage "${blackbox_build_dir}" "${timestamp}" >/dev/null
    
    # Load source files and ignore patterns using efficient functions
    load_source_files "${project_root}"
    
    # Process all source files using the cached data
    for source_file in "${SOURCE_FILES_CACHE[@]}"; do
        total_count=$((total_count + 1))
        
        # Check if this source file has corresponding gcov coverage
        local source_basename
        source_basename=$(basename "${source_file}")
        local found_coverage=false
        
        # Find gcov file using the same approach as calculate_coverage_generic
        local gcov_file
        gcov_file=$("${FIND}" "${blackbox_build_dir}" -name "${source_basename}.gcov" -type f | "${GREP}" -v '_test' 2>/dev/null | head -1 || true)
        
        if [[ -n "${gcov_file}" && -f "${gcov_file}" ]]; then
            # Check if the file has any coverage using the same pattern as calculate_coverage_generic
            if "${GREP}" -q "^[ \t]*[1-9][0-9]*.*:" "${gcov_file}" 2>/dev/null; then
                found_coverage=true
            fi
        fi
        
        if [[ "${found_coverage}" == true ]]; then
            covered_count=$((covered_count + 1))
        else
            uncovered_count=$((uncovered_count + 1))
            uncovered_files+=("${source_file}")
        fi
    done
    
    # Sort uncovered files for consistent output
    if [[ ${#uncovered_files[@]} -gt 0 ]]; then
        mapfile -t uncovered_files < <("${PRINTF}" '%s\n' "${uncovered_files[@]}" | sort || true)
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
