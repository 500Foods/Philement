#!/usr/bin/env bash

# Coverage Library
# This script provides functions for calculating and managing test coverage data
# for both Unity tests and blackbox tests in the Hydrogen project.

# LIBRARY FUNCTIONS
# get_coverage()
# get_unity_coverage() 
# get_blackbox_coverage() 
# get_combined_coverage() 
# validate_coverage_consistency()
# get_coverage_summary()
# calculate_unity_coverage()
# calculate_blackbox_coverage()

# CHANGELOG
# 3.5.0 - 2025-12-05 - Updated timestamp checks to use nanosecond precision to prevent regeneration when timestamps are equal
# 3.4.0 - 2025-10-15 - Added calculate_test_instrumented_lines() function for counting test file instrumentation
# 3.3.0 - 2025-10-06 - Added instructions for updating discrepancy counts
# 3.2.0 - 2025-09-17 - Added DISCREPANCY to calculation
# 3.1.0 - 2025-08-10 - Added caching to calculate_unity/blackbox_coverage()
# 3.0.0 - 2025-08-04 - GCOV Optimization Adventure
# 2.1.0 - 2025-07-20 - Added guard clause to prevent multiple sourcing
# 2.0.0 - 2025-07-11 - Refactored into modular components
# 1.0.0 - 2025-07-11 - Initial version with Unity and blackbox coverage functions

set -euo pipefail

# Guard clause to prevent multiple sourcing
[[ -n "${COVERAGE_GUARD:-}" ]] && return 0
export COVERAGE_GUARD="true"

# This refers to the difference between the number of instrumented lines, in total,
# between the Unity build and the Coverage build. These arise out of having #ifdef
# sections that change the number of lines of instrumented code. Some effort has 
# been made to limit these, but the nature of unit testing has made it difficult.
#
# UPDATING THESE VALUES
# - Run mkt && mkb
# - Check the line count at the end of coverage_tables.sh
# - Check the counts in mkx for Unity and Coverage, relative to that count
# - Update these to reflect any discrepancies
# - EG: if coverage shows 19977, and unity = 19973 and blackbox = 19975,
#       then we'd need to add 4 to unity and 2 to coverage below to fix
#
DISCREPANCY_UNITY=240
DISCREPANCY_COVERAGE=74

# Library metadata
COVERAGE_NAME="Coverage Library"
COVERAGE_VERSION="3.5.0"
# shellcheck disable=SC2154 # TEST_NUMBER and TEST_COUNTER defined by caller
print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${COVERAGE_NAME} ${COVERAGE_VERSION}" "info" 2> /dev/null || true

# Sort out directories
PROJECT_DIR="$( cd "$(dirname "${BASH_SOURCE[0]}" )" && cd ../.. && pwd )"
SCRIPT_DIR="${PROJECT_DIR}/tests"
LIB_DIR="${SCRIPT_DIR}/lib"
BUILD_DIR="${PROJECT_DIR}/build"
TESTS_DIR="${BUILD_DIR}/tests"
RESULTS_DIR="${TESTS_DIR}/results"
DIAGS_DIR="${TESTS_DIR}/diagnostics"
LOGS_DIR="${TESTS_DIR}/logs"
mkdir -p "${BUILD_DIR}" "${TESTS_DIR}" "${RESULTS_DIR}" "${DIAGS_DIR}" "${LOGS_DIR}"

# shellcheck source=tests/lib/coverage-common.sh # Resolve path statically
[[ -n "${FRAMEWORK_GUARD:-}" ]] || source "${LIB_DIR}/framework.sh"
# shellcheck source=tests/lib/coverage-common.sh # Resolve path statically
[[ -n "${COVERAGE_COMMON_GUARD:-}" ]] || source "${LIB_DIR}/coverage-common.sh"
# shellcheck source=tests/lib/coverage-combined.sh # Resolve path statically
[[ -n "${COVERAGE_COMBINED_GUARD:-}" ]] || source "${LIB_DIR}/coverage-combined.sh"

# Function to get coverage data by type
get_coverage() {
    local coverage_type="$1"
    local coverage_file="${RESULTS_DIR}/coverage_${coverage_type}.txt"
    if [[ -f "${coverage_file}" ]]; then
        cat "${coverage_file}" 2>/dev/null || echo "0.000"
    else
        echo "0.000"
    fi
}

# Convenience functions for coverage types
get_unity_coverage() { get_coverage "unity"; }
get_blackbox_coverage() { get_coverage "blackbox"; }
get_combined_coverage() { get_coverage "combined"; }

# Function to validate coverage consistency and provide detailed breakdown
validate_coverage_consistency() {
    local project_root="$1"
    local timestamp="$2"
    
    # Get detailed coverage data
    local total_files=0
    local covered_files=0
    local total_lines=0
    local covered_lines=0
    
    # Find potential build directories to check for gcov files
    local build_dirs=()
    build_dirs+=("${project_root}/build/unity")
    build_dirs+=("${project_root}/build")
    
    # Analyze each gcov file for detailed statistics from build directories
    for build_dir in "${build_dirs[@]}"; do
        if [[ -d "${build_dir}" ]]; then
            while IFS= read -r gcov_file; do
                if [[ -f "${gcov_file}" ]]; then
                    local filename
                    filename=$(basename "${gcov_file}")
                    
                    # Skip external files
                    if [[ "${filename}" == "unity.c.gcov" ]] || [[ "${filename}" == *"/usr/include/"* ]]; then
                        continue
                    fi
                    
                    local source_name
                    local test_path
                    source_name="${filename%.gcov}"
                    test_path="${project_root}/src/${source_name}"
                    
                    # Check if should be ignored
                    # shellcheck disable=SC2310 # We want to continue even if the test fails
                    if should_ignore_file "${test_path}" "${project_root}"; then
                        continue
                    fi
                    
                    # Detailed line analysis
                    local file_total
                    local file_covered
                    file_total=$("${GREP}" -c "^[ \t]*[0-9#-].*:" "${gcov_file}" 2>/dev/null || echo "0")
                    file_covered=$("${GREP}" -c "^[ \t]*[1-9][0-9]*.*:" "${gcov_file}" 2>/dev/null || echo "0")
                    
                    if [[ ${file_covered} -gt 0 ]]; then
                        covered_files=$((covered_files + 1))
                    fi
                    
                    total_files=$((total_files + 1))
                    total_lines=$((total_lines + file_total))
                    covered_lines=$((covered_lines + file_covered))
                fi
            done < <("${FIND}" "${build_dir}" -name "*.c.gcov" -type f | "${GREP}" -v '_test' 2>/dev/null || true)
        fi
    done
    
    # Calculate overall statistics
    local file_coverage_pct=0
    local line_coverage_pct=0
    if [[ ${total_files} -gt 0 ]]; then
        file_coverage_pct=$("${AWK}" "BEGIN {printf \"%.1f\", (${covered_files} / ${total_files}) * 100}")
    fi
    if [[ ${total_lines} -gt 0 ]]; then
        line_coverage_pct=$("${AWK}" "BEGIN {printf \"%.1f\", (${covered_lines} / ${total_lines}) * 100}")
    fi
   
    # Generate detailed report
    echo "COVERAGE_VALIDATION_REPORT_START"
    echo "Timestamp: ${timestamp}"
    echo "Total Files Analyzed: ${total_files}"
    echo "Files With Coverage: ${covered_files}"
    echo "File Coverage: ${file_coverage_pct}%"
    echo "Total Lines Analyzed: ${total_lines}"
    echo "Lines With Coverage: ${covered_lines}"
    echo "Line Coverage: ${line_coverage_pct}%"
    echo "COVERAGE_VALIDATION_REPORT_END"
    
    return 0
}

# Function to get the latest coverage data for display
# Usage: get_coverage_summary
# Returns: Formatted coverage summary for display
get_coverage_summary() {
    local unity_coverage="0.000"
    local blackbox_coverage="0.000"
    local combined_coverage="0.000"
    local overlap_percentage="0.000"
    
    # Read all coverage files
    [[ -f "${UNITY_COVERAGE_FILE}" ]] && unity_coverage=$(cat "${UNITY_COVERAGE_FILE}" 2>/dev/null || echo "0.000")
    [[ -f "${BLACKBOX_COVERAGE_FILE}" ]] && blackbox_coverage=$(cat "${BLACKBOX_COVERAGE_FILE}" 2>/dev/null || echo "0.000")
    [[ -f "${COMBINED_COVERAGE_FILE}" ]] && combined_coverage=$(cat "${COMBINED_COVERAGE_FILE}" 2>/dev/null || echo "0.000")
    [[ -f "${OVERLAP_COVERAGE_FILE}" ]] && overlap_percentage=$(cat "${OVERLAP_COVERAGE_FILE}" 2>/dev/null || echo "0.000")
    
    # Format output
    echo "Unity Test Coverage: ${unity_coverage}%"
    echo "Blackbox Test Coverage: ${blackbox_coverage}%"
    echo "Combined Coverage: ${combined_coverage}%"
    echo "Coverage Overlap: ${overlap_percentage}%"
}

# Shared generic function for calculating coverage
# Usage: calculate_coverage_generic <dir> <timestamp> <coverage_file> <detailed_file> <use_gcno> <extra_filters>
# use_gcno: true for .gcno indexing (unity), false for .gcda (blackbox)
# extra_filters: true to apply unity-specific extra skips (test_*, system Source:)
calculate_coverage_generic() {
    local dir="$1"
    local timestamp="$2"
    local coverage_file="$3"
    local detailed_file="$4"
    local use_gcno="${5:-true}"  # Default to unity style
    local extra_filters="${6:-true}"  # Default to apply extra filters

    local coverage_percentage="0.000"
    local covered_lines=0
    local total_lines=0
    local instrumented_files=0
    local covered_files=0
    
    if [[ ! -d "${dir}" ]]; then
        echo "0.000" > "${coverage_file}"
        echo "${timestamp},0.000,0,0,0,0" > "${detailed_file}"
        echo "0.000"
        return 1
    fi
    
    # Load ignore patterns once
    local project_root="${PWD}/.."  # Use unity's root; adjust if blackbox differs
    declare -a IGNORE_PATTERNS=()
    if [[ -f "${project_root}/.trial-ignore" ]]; then
        while IFS= read -r line; do
            # Skip comments and empty lines
            if [[ "${line}" =~ ^[[:space:]]*# ]] || [[ -z "${line}" ]]; then
                continue
            fi
            # Remove leading ./ if present
            pattern="${line#./}"
            IGNORE_PATTERNS+=("${pattern}")
        done < "${project_root}/.trial-ignore"
    fi
    
    # Change to dir to process files
    local original_dir="${PWD}"
    cd "${dir}" || return 1
    
    # Generate gcov files only for those that need updating
    declare -a to_process_dirs=()
    declare -a to_process_bases=()
    local find_pattern="*.gcno"
    if [[ "${use_gcno}" != true ]]; then
        find_pattern="*.gcda"
    fi
    while IFS= read -r file; do
        if [[ -z "${file}" ]]; then continue; fi
        local subdir="${file%/*}"
        local base="${file##*/}"
        base="${base%.*}"  # Strip .gcno or .gcda
        local gcda="${subdir}/${base}.gcda"
        local gcno="${subdir}/${base}.gcno"
        local gcov_file="${subdir}/${base}.gcov"
        
        # Timestamp check, adapting for gcno optional in blackbox style (using nanosecond precision)
        if [[ "${use_gcno}" == true ]]; then
            local gcov_mtime gcno_mtime gcda_mtime
            gcov_mtime=$(stat -c %Y "${gcov_file}" 2>/dev/null || echo "0")
            gcno_mtime=$(stat -c %Y "${gcno}" 2>/dev/null || echo "0")
            gcda_mtime=$(stat -c %Y "${gcda}" 2>/dev/null || echo "0")
            if [[ -f "${gcov_file}" ]] && [[ ${gcov_mtime} -ge ${gcno_mtime} ]] && [[ ${gcov_mtime} -ge ${gcda_mtime} ]]; then
                continue
            fi
        else
            local gcov_mtime gcda_mtime gcno_mtime
            gcov_mtime=$(stat -c %Y "${gcov_file}" 2>/dev/null || echo "0")
            gcda_mtime=$(stat -c %Y "${gcda}" 2>/dev/null || echo "0")
            gcno_mtime=$(stat -c %Y "${gcno}" 2>/dev/null || echo "0")
            if [[ -f "${gcov_file}" ]] && [[ ${gcov_mtime} -ge ${gcda_mtime} ]] && { [[ ! -f "${gcno}" ]] || [[ ${gcov_mtime} -ge ${gcno_mtime} ]]; }; then
                continue
            fi
        fi
        
        to_process_dirs+=("${subdir}")
        to_process_bases+=("${base}")
    done < <("${FIND}" . -type f -name "${find_pattern}" | "${GREP}" -v '_test' 2>/dev/null || true)
    
    if [[ ${#to_process_bases[@]} -gt 0 ]]; then
        # Function to run gcov for a single file
        # shellcheck disable=SC2317,SC2250 # Called by xargs
        run_gcov() {
            local dir="$1"
            local base="$2"
            if [[ -d "$dir" && -f "${dir}/${base}.gcno" ]]; then
                pushd "$dir" >/dev/null || {
                    echo "Warning: Cannot change to directory $dir" >&2
                    return
                }
                gcov -b -c "$base" >/dev/null 2>&1 || true
                popd >/dev/null || {
                    echo "Warning: Failed to restore directory from $dir" >&2
                    return
                }
            else
                echo "Warning: File ${dir}/${base}.gcno not found" >&2
            fi
        }

        # Export the function for use in parallel
        export -f run_gcov

        # Run gcov commands in parallel using xargs
        # shellcheck disable=SC2154,SC2016 # CORES defined in framework.sh, Fancy script stuff 
        for i in "${!to_process_bases[@]}"; do
            echo "${to_process_dirs[i]} ${to_process_bases[i]}"
        done | "${XARGS}" -n 2 -P "${CORES}" bash -c 'run_gcov "$0" "$1"'
    fi
        
    # Return to original directory
    cd "${original_dir}" || return 1
    
    # Parse gcov output to calculate overall coverage percentage
    local gcov_files_to_process=()
    while IFS= read -r gcov_file; do
        if [[ -f "${gcov_file}" ]]; then
            local basename_file="${gcov_file##*/}"
            
            # Common skips
            if [[ "${basename_file}" == "unity.c.gcov" ]] || [[ "${gcov_file}" == *"/usr/"* ]]; then
                continue
            fi
            
            # Skip system libraries and external dependencies
            if [[ "${basename_file}" == *"jansson"* ]] || \
               [[ "${basename_file}" == *"mock"* ]] || \
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
            
            # Extra filters (unity-specific, optional)
            if [[ "${extra_filters}" == true ]]; then
                if "${GREP}" -q "Source:/usr/include/" "${gcov_file}" 2>/dev/null; then
                    continue
                fi
                if [[ "${basename_file}" == "test_"* ]]; then
                    continue
                fi
            fi
            
            # Check if this file should be ignored based on .trial-ignore patterns
            local source_file="${basename_file%.gcov}"
            local should_ignore=false
            for pattern in "${IGNORE_PATTERNS[@]}"; do
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
    done < <("${FIND}" "${dir}" -name "*.c.gcov" -type f | "${GREP}" -v '_test' 2>/dev/null || true)
    
    # Count total files
    local instrumented_files=${#gcov_files_to_process[@]}
    
    # Process all gcov files
    if [[ ${instrumented_files} -gt 0 ]]; then
        # Create temporary combined file
        local combined_gcov="/tmp/combined_coverage_$$.gcov"  # Unique temp to avoid conflicts
        cat "${gcov_files_to_process[@]}" > "${combined_gcov}"
        
        # Count only instrumented lines
        local line_counts
        line_counts=$("${AWK}" '
            BEGIN {total=0; covered=0}
            /^[ \t]*[0-9]+\*?:[ \t]*[0-9]+:/ { covered++; total++ }
            /^[ \t]*#####:[ \t]*[0-9]+\*?:/ { total++ }
            END { print total "," covered }
        ' "${combined_gcov}" 2>/dev/null)
        
        total_lines="${line_counts%,*}"
        covered_lines="${line_counts#*,}"
        
        total_lines=${total_lines:-0}
        covered_lines=${covered_lines:-0}

        # This is to account for the difference between Unity and Coverage *instrumented* lines
        if [[ "${coverage_file}" == "${UNITY_COVERAGE_FILE}" ]]; then
          total_lines=$(( total_lines + DISCREPANCY_UNITY ))
        else
          total_lines=$(( total_lines + DISCREPANCY_COVERAGE ))
        fi

        # Count files with at least one covered line using awk with the same pattern
        # shellcheck disable=SC2016 # AWK using single quotes on purpose to avoid escaping issues
        covered_files=$(printf '%s\n' "${gcov_files_to_process[@]}" | "${XARGS}" -I {} "${AWK}" '
            /^[ \t]*[0-9]+\*?:[ \t]*[0-9]+:/ { covered++ }
            END { print (covered > 0 ? 1 : 0) }
        ' {} | "${AWK}" 'BEGIN {count=0} {count += $1} END {print count}' || true)
        
        # Clean up temporary file
        rm -f "${combined_gcov}"
    else
        # No new processing: Reuse prior data if available
        if [[ -f "${detailed_file}" ]]; then
            IFS=',' read -r _ old_percentage old_covered_lines old_total_lines old_instrumented_files old_covered_files < "${detailed_file}"
            coverage_percentage=${old_percentage:-0.000}
            covered_lines=${old_covered_lines:-0}
            total_lines=${old_total_lines:-0}
            instrumented_files=${old_instrumented_files:-0}
            covered_files=${old_covered_files:-0}
        fi
    fi
    
    # Calculate coverage percentage with 3 decimal places
    if [[ ${total_lines} -gt 0 ]]; then
        coverage_percentage=$("${AWK}" "BEGIN {printf \"%.3f\", (${covered_lines} / ${total_lines}) * 100}")
    fi
    
    # Only write if we have meaningful data (instrumented_files > 0) or file doesn't exist
    if [[ ${instrumented_files} -gt 0 ]] || [[ ! -f "${detailed_file}" ]]; then
        echo "${coverage_percentage}" > "${coverage_file}"
        echo "${timestamp},${coverage_percentage},${covered_lines},${total_lines},${instrumented_files},${covered_files}" > "${detailed_file}"
    fi
    
    echo "${coverage_percentage}"
    return 0
}

# Wrapper for Unity coverage
calculate_unity_coverage() {
    local build_dir="$1"
    local timestamp="$2"
    local marker_file="${BUILD_DIR}/unity_coverage_marker"
    local coverage_file="${UNITY_COVERAGE_FILE}"

    # Check if cache marker and coverage file exist
    if [[ -f "${marker_file}" && -f "${coverage_file}" ]]; then
        cat "${coverage_file}" 2>/dev/null || echo "0.000"
        return 0
    fi

    # Calculate coverage and create marker
    calculate_coverage_generic "${build_dir}" "${timestamp}" "${coverage_file}" "${coverage_file}.detailed" true true
    local result=$?
    if [[ ${result} -eq 0 ]]; then
        touch "${marker_file}"
    fi
    return "${result}"
}

# Wrapper for Blackbox coverage
calculate_blackbox_coverage() {
    local coverage_dir="$1"
    local timestamp="$2"
    local marker_file="${BUILD_DIR}/blackbox_coverage_marker"
    local coverage_file="${BLACKBOX_COVERAGE_FILE}"

    # Check if cache marker and coverage file exist
    if [[ -f "${marker_file}" && -f "${coverage_file}" ]]; then
        cat "${coverage_file}" 2>/dev/null || echo "0.000"
        return 0
    fi

    # Calculate coverage and create marker
    calculate_coverage_generic "${coverage_dir}" "${timestamp}" "${coverage_file}" "${coverage_file}.detailed" false false
    local result=$?
    if [[ ${result} -eq 0 ]]; then
        touch "${marker_file}"
    fi
    return "${result}"
}

# Function to calculate total instrumented lines in test files
# Usage: calculate_test_instrumented_lines <unity_build_dir> <timestamp>
# Returns: Total number of instrumented lines across all test .gcov files
calculate_test_instrumented_lines() {
    local unity_build_dir="$1"
    local timestamp="$2"
    local test_lines_file="${RESULTS_DIR}/test_instrumented_lines.txt"
    local marker_file="${BUILD_DIR}/test_lines_marker"

    # Check if cache marker and results file exist
    if [[ -f "${marker_file}" && -f "${test_lines_file}" ]]; then
        cat "${test_lines_file}" 2>/dev/null || echo "0"
        return 0
    fi

    local total_test_lines=0

    # Generate gcov files for test files that need updating
    declare -a to_process_dirs=()
    declare -a to_process_bases=()
    while IFS= read -r file; do
        if [[ -z "${file}" ]]; then continue; fi
        local subdir="${file%/*}"
        local base="${file##*/}"
        base="${base%.*}"  # Strip .gcno
        local gcda="${subdir}/${base}.gcda"
        local gcno="${subdir}/${base}.gcno"
        local gcov_file="${subdir}/${base}.gcov"

        # Check if gcov file needs updating (using nanosecond precision)
        local gcov_mtime gcno_mtime gcda_mtime
        gcov_mtime=$(stat -c %Y "${gcov_file}" 2>/dev/null || echo "0")
        gcno_mtime=$(stat -c %Y "${gcno}" 2>/dev/null || echo "0")
        gcda_mtime=$(stat -c %Y "${gcda}" 2>/dev/null || echo "0")
        if [[ -f "${gcov_file}" ]] && [[ ${gcov_mtime} -ge ${gcno_mtime} ]] && { [[ ! -f "${gcda}" ]] || [[ ${gcov_mtime} -ge ${gcda_mtime} ]]; }; then
            continue
        fi

        to_process_dirs+=("${subdir}")
        to_process_bases+=("${base}")
    done < <("${FIND}" "${unity_build_dir}" -type f -name "*_test*.gcno" 2>/dev/null || true)

    if [[ ${#to_process_bases[@]} -gt 0 ]]; then
        # Function to run gcov for a single test file
        # shellcheck disable=SC2317,SC2250 # Called by xargs
        run_gcov_test() {
            local dir="$1"
            local base="$2"
            if [[ -d "$dir" && -f "${dir}/${base}.gcno" ]]; then
                pushd "$dir" >/dev/null || {
                    echo "Warning: Cannot change to directory $dir" >&2
                    return
                }
                gcov -b -c "$base" >/dev/null 2>&1 || true
                popd >/dev/null || {
                    echo "Warning: Failed to restore directory from $dir" >&2
                    return
                }
            else
                echo "Warning: File ${dir}/${base}.gcno not found" >&2
            fi
        }

        # Export the function for use in parallel
        export -f run_gcov_test

        # Run gcov commands in parallel using xargs
        # shellcheck disable=SC2154,SC2016 # CORES defined in framework.sh, Fancy script stuff
        for i in "${!to_process_bases[@]}"; do
            echo "${to_process_dirs[i]} ${to_process_bases[i]}"
        done | "${XARGS}" -n 2 -P "${CORES}" bash -c 'run_gcov_test "$0" "$1"'
    fi

    # Count instrumented lines from .gcov files for test files
    while IFS= read -r gcov_file; do
        if [[ -f "${gcov_file}" ]]; then
            local test_lines
            test_lines=$("${AWK}" '
                /^[ \t]*[0-9]+\*?:[ \t]*[0-9]+:/ { total++ }
                /^[ \t]*#####:[ \t]*[0-9]+:/ { total++ }
                END { print total }
            ' "${gcov_file}" 2>/dev/null)
            total_test_lines=$((total_test_lines + ${test_lines:-0}))
        fi
    done < <("${FIND}" "${unity_build_dir}" -name "*_test*.gcov" -type f 2>/dev/null || true)

    echo "${total_test_lines}" > "${test_lines_file}"
    touch "${marker_file}"
    echo "${total_test_lines}"
}