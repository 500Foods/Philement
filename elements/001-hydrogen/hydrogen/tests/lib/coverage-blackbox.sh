#!/bin/bash

# Coverage Blackbox Library
# This script provides functions for calculating blackbox test coverage,

# LIBRARY FUNCTIONS
# collect_blackbox_coverage()
# collect_blackbox_coverage_from_dir()

# CHANGELOG
# 1.0.0 - 2025-07-21 - Initial version with combined coverage functions

# Guard clause to prevent multiple sourcing
[[ -n "${COVERAGE_BLACKBOX_GUARD}" ]] && return 0
export COVERAGE_BLACKBOX_GUARD="true"

# Library metadata
COVERAGE_BLACKBOX_NAME="Coverage Blackbox Library"
COVERAGE_BLACKBOX_VERSION="1.0.0"
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

# Function to collect blackbox test coverage
# Usage: collect_blackbox_coverage <hydrogen_coverage_binary> <timestamp>
# Returns: Coverage percentage written to blackbox_coverage.txt
collect_blackbox_coverage() {
    local hydrogen_coverage_bin="$1"
    local timestamp="$2"
    local coverage_percentage="0.000"
    
    if [ ! -f "${hydrogen_coverage_bin}" ]; then
        echo "0.000" > "${BLACKBOX_COVERAGE_FILE}"
        return 1
    fi
    
    # Note: hydrogen_coverage is a server binary, not a utility
    # For blackbox coverage, we need to convert .gcda files to .gcov files
    local original_dir="${PWD}"
    local project_root="${original_dir}"
    
    # Find the directory where hydrogen_coverage was built (contains .gcno and .gcda files)
    # This should be the main build directory, not build_unity
    local build_dir
    if [[ -d "${original_dir}/build" ]]; then
        build_dir="${original_dir}/build"
    else
        build_dir="$(dirname "${hydrogen_coverage_bin}")"
    fi
    
    # Generate fresh gcov files from accumulated .gcda files (blackbox test results)
    cd "${build_dir}" || return 1
    
    # Find .gcda files and ensure corresponding .gcno files exist
    local gcda_files=()
    mapfile -t gcda_files < <(find . -name "*.gcda" -type f)
    
    if [ ${#gcda_files[@]} -eq 0 ]; then
        echo "No .gcda files found in ${build_dir}" >&2
        cd "${original_dir}" || return 1
        echo "0.000" > "${BLACKBOX_COVERAGE_FILE}"
        return 1
    fi
    
    # Generate gcov files more efficiently using parallel processing
    # Detect number of CPU cores for parallel build
    local cpu_cores
    if command -v nproc >/dev/null 2>&1; then
        cpu_cores=$(nproc)
    elif [ -f /proc/cpuinfo ]; then
        cpu_cores=$(grep -c ^processor /proc/cpuinfo)
    else
        cpu_cores=4  # Fallback to 4 cores
    fi
    
    if command -v xargs >/dev/null 2>&1; then
        # Use xargs for parallel processing with all available cores
        find . -name "*.gcda" -print0 | xargs -0 -P"${cpu_cores}" -I{} sh -c "
            gcda_dir=\"\$(dirname '{}')\"
            cd \"\${gcda_dir}\" && gcov \"\$(basename '{}')\" >/dev/null 2>&1
        "
    else
        # Fallback to optimized sequential processing
        find . -name "*.gcda" -exec sh -c '
            gcda_dir="$(dirname "$1")"
            cd "${gcda_dir}" && gcov "$(basename "$1")" >/dev/null 2>&1
        ' _ {} \;
    fi
    
    # Return to original directory
    cd "${original_dir}" || return 1
    
    # Parse gcov output similar to Unity but for blackbox tests
    local total_lines=0
    local covered_lines=0
    local instrumented_files=0
    local covered_files=0
    
    # Build list of valid gcov files that correspond to project source files
    local valid_gcov_files=()
    declare -A blackbox_gcov_cache
    
    # First, cache all available gcov files
    while IFS= read -r gcov_file; do
        local gcov_basename
        gcov_basename=$(basename "${gcov_file}")
        blackbox_gcov_cache["${gcov_basename}"]="${gcov_file}"
    done < <(find "${build_dir}" -name "*.gcov" -type f 2>/dev/null)
    
    # Debug: Check if we found any gcov files
    if [[ ${#blackbox_gcov_cache[@]} -eq 0 ]]; then
        echo "No gcov files found in ${build_dir}" >&2
        cd "${original_dir}" || return 1
        echo "0.000" > "${BLACKBOX_COVERAGE_FILE}"
        return 1
    fi
    
    
    # Process project source files to find matching gcov files
    local total_source_files=0
    local ignored_files=0
    local matched_files=0
    local unmatched_files=0
    
    while IFS= read -r source_file; do
        total_source_files=$((total_source_files + 1))
        
        if should_ignore_file "${source_file}" "${project_root}"; then
            ignored_files=$((ignored_files + 1))
            continue
        fi
        
        local source_basename
        source_basename=$(basename "${source_file}")
        local gcov_filename="${source_basename}.gcov"
        local gcov_file="${blackbox_gcov_cache["${gcov_filename}"]}"
        
        if [[ -n "${gcov_file}" && -f "${gcov_file}" ]]; then
            valid_gcov_files+=("${gcov_file}")
            instrumented_files=$((instrumented_files + 1))
            matched_files=$((matched_files + 1))
        else
            unmatched_files=$((unmatched_files + 1))
        fi
    done < <(get_cached_source_files "${project_root}")
    
    # Process valid gcov files individually for accurate coverage calculation
    if [[ ${#valid_gcov_files[@]} -gt 0 ]]; then
        for gcov_file in "${valid_gcov_files[@]}"; do
            # Use same awk processing as Test 11 for line counting per file
            local file_data
            file_data=$(awk '
                /^[ \t]*[0-9]+\*?:[ \t]*[0-9]+:/ { covered++; total++ }
                /^[ \t]*#####:[ \t]*[0-9]+\*?:/ { total++ }
                END { print total "," covered }
            ' "${gcov_file}" 2>/dev/null)
            
            local file_total="${file_data%,*}"
            local file_covered="${file_data#*,}"
            
            # Default to 0 if parsing failed
            file_total=${file_total:-0}
            file_covered=${file_covered:-0}
            
            total_lines=$((total_lines + file_total))
            covered_lines=$((covered_lines + file_covered))
            
            # Count as covered if any lines were executed
            if [[ ${file_covered} -gt 0 ]]; then
                covered_files=$((covered_files + 1))
            fi
        done
    else
        echo "No valid gcov files found for project source files" >&2
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

# Function to collect blackbox test coverage from a specific directory
# Usage: collect_blackbox_coverage_from_dir <coverage_directory> <timestamp>
# Returns: Coverage percentage written to blackbox_coverage.txt
collect_blackbox_coverage_from_dir() {
    local coverage_dir="$1"
    local timestamp="$2"
    local coverage_percentage="0.000"
    
    if [ ! -d "${coverage_dir}" ]; then
        echo "0.000" > "${BLACKBOX_COVERAGE_FILE}"
        return 1
    fi
    
    # Generate .gcov files from .gcda files in coverage directory
    local original_dir="${PWD}"
    cd "${coverage_dir}" || return 1
    
    # Generate gcov files efficiently using parallel processing
    # Detect number of CPU cores for parallel build
    local cpu_cores
    if command -v nproc >/dev/null 2>&1; then
        cpu_cores=$(nproc)
    elif [ -f /proc/cpuinfo ]; then
        cpu_cores=$(grep -c ^processor /proc/cpuinfo)
    else
        cpu_cores=4  # Fallback to 4 cores
    fi
    
    if command -v xargs >/dev/null 2>&1; then
        find . -name "*.gcda" -print0 | xargs -0 -P"${cpu_cores}" -I{} sh -c "
            gcda_dir=\"\$(dirname '{}')\"
            cd \"\${gcda_dir}\" && gcov \"\$(basename '{}')\" >/dev/null 2>&1
        "
    else
        find . -name "*.gcda" -exec sh -c '
            gcda_dir="$(dirname "$1")"
            cd "${gcda_dir}" && gcov "$(basename "$1")" >/dev/null 2>&1
        ' _ {} \;
    fi
    
    # Return to original directory
    cd "${original_dir}" || return 1
    
    # Use same logic as Test 11 Unity coverage calculation  
    local project_root="${original_dir}"
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
    done < <(find "${coverage_dir}" -name "*.gcov" -type f 2>/dev/null)
    
    # Count total files exactly like Test 11 - by counting filtered .gcov files
    instrumented_files=${#gcov_files_to_process[@]}
    
    # Process all gcov files exactly like Test 11 does
    if [[ ${#gcov_files_to_process[@]} -gt 0 ]]; then
        # Create temporary combined file (same as Test 11)
        local combined_gcov="/tmp/combined_blackbox.gcov"
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
    echo "${coverage_percentage}" > "${BLACKBOX_COVERAGE_FILE}"
    echo "${timestamp},${coverage_percentage},${covered_lines},${total_lines},${instrumented_files},${covered_files}" > "${BLACKBOX_COVERAGE_FILE}.detailed"
    
    echo "${coverage_percentage}"
    return 0
}
