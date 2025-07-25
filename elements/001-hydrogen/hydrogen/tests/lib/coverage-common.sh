#!/bin/bash

# Coverage Common Library
# Utilities and variables for coverage calculations

# LIBRARY FUNCTIONS
# analyze_combined_gcov_coverage()
# analyze_all_gcov_coverage_batch()
# load_ignore_patterns()
# should_ignore_file()
# load_source_files()
# identify_uncovered_files()
# cleanup_coverage_data()
# analyze_gcov_file()
# collect_gcov_files()

# CHANGELOG
# 1.0.0 - 2025-07-21 - Initial version with common coverage functions

# Guard clause to prevent multiple sourcing
[[ -n "${COVERAGE_COMMON_GUARD}" ]] && return 0
export COVERAGE_COMMON_GUARD="true"

# Library metadata
COVERAGE_COMMON_NAME="Coverage Common Library"
COVERAGE_COMMON_VERSION="2.1.0"
print_message "${COVERAGE_COMMON_NAME} ${COVERAGE_COMMON_VERSION}" "info" 2> /dev/null || true

# Sort out directories
PROJECT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && cd ../.. && pwd )"
# SCRIPT_DIR="${PROJECT_DIR}/tests"
# LIB_DIR="${SCRIPT_DIR}/lib"
BUILD_DIR="${PROJECT_DIR}/build"
TESTS_DIR="${BUILD_DIR}/tests"
RESULTS_DIR="${TESTS_DIR}/results"
DIAGS_DIR="${TESTS_DIR}/diagnostics"
LOGS_DIR="${TESTS_DIR}/logs"
mkdir -p "${BUILD_DIR}" "${TESTS_DIR}" "${RESULTS_DIR}" "${DIAGS_DIR}" "${LOGS_DIR}"

# Store resuls
UNITY_COVERAGE_FILE="${RESULTS_DIR}/coverage_unity.txt"
BLACKBOX_COVERAGE_FILE="${RESULTS_DIR}/coverage_blackbox.txt"
COMBINED_COVERAGE_FILE="${RESULTS_DIR}/coverage_combined.txt"
OVERLAP_COVERAGE_FILE="${RESULTS_DIR}/coverage_overlap.txt"

# Function to analyze combined coverage from two gcov files for the same source file
# Usage: analyze_combined_gcov_coverage <unity_gcov_file> <blackbox_gcov_file>
# Returns: "instrumented_lines,combined_covered_lines"
analyze_combined_gcov_coverage() {
    local unity_gcov="$1"
    local blackbox_gcov="$2"
    
    # Handle cases where only one file exists
    if [[ ! -f "${unity_gcov}" && ! -f "${blackbox_gcov}" ]]; then
        echo "0,0,0"
        return 1
    elif [[ ! -f "${unity_gcov}" ]]; then
        # Only blackbox coverage exists
        local result
        result=$(awk '
            /^[ \t]*[0-9]+\*?:[ \t]*[0-9]+:/ { covered++; total++ }
            /^[ \t]*#####:[ \t]*[0-9]+:/ { total++ }
            END {
                if (total == "") total = 0;
                if (covered == "") covered = 0;
                print total "," covered "," covered
            }' "${blackbox_gcov}")
        echo "${result}"
        return 0
    elif [[ ! -f "${blackbox_gcov}" ]]; then
        # Only unity coverage exists
        local result
        result=$(awk '
            /^[ \t]*[0-9]+\*?:[ \t]*[0-9]+:/ { covered++; total++ }
            /^[ \t]*#####:[ \t]*[0-9]+:/ { total++ }
            END {
                if (total == "") total = 0;
                if (covered == "") covered = 0;
                print total "," covered "," covered
            }' "${unity_gcov}")
        echo "${result}"
        return 0
    fi
    
    # Both files exist - calculate TRUE union using the same logic as manual test
    local temp_dir
    temp_dir=$(mktemp -d 2>/dev/null) || return 1
    local unity_covered="${temp_dir}/unity_covered"
    local blackbox_covered="${temp_dir}/blackbox_covered"
    local all_instrumented="${temp_dir}/all_instrumented"
    local union_covered="${temp_dir}/union_covered"
    
    export unity_covered
    export blackbox_covered
    export all_instrumented 
    export union_covered

    # Single AWK script to process both files and calculate union in memory
    local result
    result=$(awk -v unity_file="${unity_gcov}" -v blackbox_file="${blackbox_gcov}" '
    BEGIN {
        # Process Unity file
        if (unity_file != "") {
            while ((getline line < unity_file) > 0) {
                if (match(line, /^[ \t]*[0-9]+\*?:[ \t]*[0-9]+:/)) {
                    split(line, parts, ":")
                    gsub(/^[ \t]*|[ \t]*$/, "", parts[2])
                    unity_covered[parts[2]] = 1
                    all_instrumented[parts[2]] = 1
                }
                else if (match(line, /^[ \t]*#####:[ \t]*[0-9]+:/)) {
                    split(line, parts, ":")
                    gsub(/^[ \t]*|[ \t]*$/, "", parts[2])
                    all_instrumented[parts[2]] = 1
                }
            }
            close(unity_file)
        }
        
        # Process Blackbox file
        if (blackbox_file != "") {
            while ((getline line < blackbox_file) > 0) {
                if (match(line, /^[ \t]*[0-9]+\*?:[ \t]*[0-9]+:/)) {
                    split(line, parts, ":")
                    gsub(/^[ \t]*|[ \t]*$/, "", parts[2])
                    blackbox_covered[parts[2]] = 1
                    all_instrumented[parts[2]] = 1
                }
                else if (match(line, /^[ \t]*#####:[ \t]*[0-9]+:/)) {
                    split(line, parts, ":")
                    gsub(/^[ \t]*|[ \t]*$/, "", parts[2])
                    all_instrumented[parts[2]] = 1
                }
            }
            close(blackbox_file)
        }
        
        # Count totals
        total_instrumented = 0
        for (line in all_instrumented) {
            total_instrumented++
        }
        
        total_covered = 0
        for (line in unity_covered) {
            total_covered++
        }
        for (line in blackbox_covered) {
            if (!(line in unity_covered)) {
                total_covered++
            }
        }
        
        print total_instrumented "," total_covered
    }')
    
    local total_instrumented="${result%,*}"
    local total_covered="${result#*,}"
    
    # Clean up temp files
    rm -rf "${temp_dir}" 2>/dev/null
    
    echo "${total_instrumented},${total_covered},${total_covered}"
}

# True batch processing function for all coverage analysis
# Usage: analyze_all_gcov_coverage_batch <unity_covs_dir> <blackbox_covs_dir>
# Sets global arrays: unity_covered_lines, unity_instrumented_lines, coverage_covered_lines, coverage_instrumented_lines, combined_covered_lines, combined_instrumented_lines
analyze_all_gcov_coverage_batch() {
    # Ensure global arrays exist
    declare -gA unity_covered_lines 2>/dev/null || true
    declare -gA unity_instrumented_lines 2>/dev/null || true
    declare -gA coverage_covered_lines 2>/dev/null || true
    declare -gA coverage_instrumented_lines 2>/dev/null || true
    declare -gA combined_covered_lines 2>/dev/null || true
    declare -gA combined_instrumented_lines 2>/dev/null || true
    local unity_dir="$1"
    local blackbox_dir="$2"
    
    # Create temporary file for concatenated input
    local temp_input
    temp_input=$(mktemp) || return 1
    
    # Get all .gcov files from both directories
    local unity_files=()
    local blackbox_files=()
    
    if [[ -d "${unity_dir}" ]]; then
        mapfile -t unity_files < <(find "${unity_dir}" -name "*.gcov" -type f 2>/dev/null || true)
    fi
    
    if [[ -d "${blackbox_dir}" ]]; then
        mapfile -t blackbox_files < <(find "${blackbox_dir}" -name "*.gcov" -type f 2>/dev/null || true)
    fi
    
    # Create union of all files by relative path from coverage directory (with filtering)
    declare -A all_file_set
    for file in "${unity_files[@]}"; do
        local rel_path="${file#"${unity_dir}/"}"
        # Convert to source path format for filtering
        local source_path="${rel_path}"
        if [[ "${source_path}" == *.gcov ]]; then
            source_path="${source_path%.gcov}"
            if [[ "${source_path}" != src/* ]]; then
                source_path="src/${source_path}"
            fi
        fi
        # Apply filtering
        if ! should_ignore_file "${source_path}" "${PROJECT_ROOT:-${PWD}}"; then
            all_file_set["${rel_path}"]=1
        fi
    done
    for file in "${blackbox_files[@]}"; do
        local rel_path="${file#"${blackbox_dir}/"}"
        # Convert to source path format for filtering
        local source_path="${rel_path}"
        if [[ "${source_path}" == *.gcov ]]; then
            source_path="${source_path%.gcov}"
            if [[ "${source_path}" != src/* ]]; then
                source_path="src/${source_path}"
            fi
        fi
        # Apply filtering
        if ! should_ignore_file "${source_path}" "${PROJECT_ROOT:-${PWD}}"; then
            all_file_set["${rel_path}"]=1
        fi
    done
    
    # Build concatenated input with file markers
    for file_relpath in "${!all_file_set[@]}"; do
        local unity_file="${unity_dir}/${file_relpath}"
        local blackbox_file="${blackbox_dir}/${file_relpath}"
        
        echo "###FILE_START:${file_relpath}###" >> "${temp_input}"
        
        if [[ -f "${unity_file}" ]]; then
            {
                echo "###UNITY_START###"
                cat "${unity_file}"
                echo "###UNITY_END###"
            } >> "${temp_input}"
        fi
        
        if [[ -f "${blackbox_file}" ]]; then
            {
                echo "###BLACKBOX_START###"
                cat "${blackbox_file}"
                echo "###BLACKBOX_END###"
            } >> "${temp_input}"
        fi
        
        echo "###FILE_END###" >> "${temp_input}"
    done
    
    # Create temporary file for AWK output
    local temp_output
    temp_output=$(mktemp) || { rm -f "${temp_input}"; return 1; }
    
    # Process all files in one AWK operation
    awk '
    BEGIN {
        current_file = ""
        current_type = ""
    }
    
    /^###FILE_START:/ {
        current_file = substr($0, 15)
        gsub(/###$/, "", current_file)
        next
    }
    
    /^###UNITY_START###/ {
        current_type = "unity"
        next
    }
    
    /^###BLACKBOX_START###/ {
        current_type = "blackbox"
        next
    }
    
    /^###(UNITY_END|BLACKBOX_END|FILE_END)###/ {
        current_type = ""
        next
    }
    
    # Process coverage lines
    /^[ \t]*[0-9]+\*?:[ \t]*[0-9]+:/ {
        if (current_file != "" && current_type != "") {
            split($0, parts, ":")
            gsub(/^[ \t]*|[ \t]*$/, "", parts[2])
            line_num = parts[2]
            
            if (current_type == "unity") {
                unity_covered[current_file][line_num] = 1
            } else if (current_type == "blackbox") {
                blackbox_covered[current_file][line_num] = 1
            }
            all_instrumented[current_file][line_num] = 1
        }
        next
    }
    
    # Process uncovered lines
    /^[ \t]*#####:[ \t]*[0-9]+:/ {
        if (current_file != "" && current_type != "") {
            split($0, parts, ":")
            gsub(/^[ \t]*|[ \t]*$/, "", parts[2])
            line_num = parts[2]
            
            all_instrumented[current_file][line_num] = 1
        }
        next
    }
    
    END {
        # Process all files that have any coverage data
        for (file in all_instrumented) {
            instrumented_count = 0
            for (line in all_instrumented[file]) {
                instrumented_count++
            }
            
            # Convert filename back to path format
            source_path = file
            if (match(source_path, /\.gcov$/)) {
                gsub(/\.gcov$/, "", source_path)
                # Path already contains subdirectory structure
                if (!match(source_path, /^src\//)) {
                    source_path = "src/" source_path
                }
            }
            
            # Calculate unity coverage
            unity_covered_count = 0
            for (line in unity_covered[file]) {
                unity_covered_count++
            }
            
            # Calculate blackbox coverage
            blackbox_covered_count = 0
            for (line in blackbox_covered[file]) {
                blackbox_covered_count++
            }
            
            # Calculate combined coverage (union)
            combined_covered_count = unity_covered_count
            for (line in blackbox_covered[file]) {
                if (!(line in unity_covered[file])) {
                    combined_covered_count++
                }
            }
            
            # Output all three coverage types
            print "UNITY:" source_path ":" instrumented_count "," unity_covered_count
            print "COVERAGE:" source_path ":" instrumented_count "," blackbox_covered_count  
            print "COMBINED:" source_path ":" instrumented_count "," combined_covered_count
        }
    }
    ' "${temp_input}" > "${temp_output}"
    
    # Process output in main shell to populate all global arrays
    while IFS=':' read -r coverage_type file_path results; do
        if [[ -n "${coverage_type}" && -n "${file_path}" && -n "${results}" && "${file_path}" != "" ]]; then
            local instrumented="${results%,*}"
            local covered="${results#*,}"
            
            # Skip invalid entries
            if [[ -z "${instrumented}" || -z "${covered}" ]]; then
                continue
            fi
            
            case "${coverage_type}" in
                "UNITY")
                    # shellcheck disable=SC2034 # declared globally elsewhere
                    unity_instrumented_lines["${file_path}"]=${instrumented}
                    # shellcheck disable=SC2034 # declared globally elsewhere
                    unity_covered_lines["${file_path}"]=${covered}
                    ;;
                "COVERAGE")
                    # shellcheck disable=SC2034 # declared globally elsewhere
                    coverage_instrumented_lines["${file_path}"]=${instrumented}
                    # shellcheck disable=SC2034 # declared globally elsewhere
                    coverage_covered_lines["${file_path}"]=${covered}
                    ;;
                "COMBINED")
                    # shellcheck disable=SC2034 # declared globally elsewhere
                    combined_instrumented_lines["${file_path}"]=${instrumented}
                    # shellcheck disable=SC2034 # declared globally elsewhere
                    combined_covered_lines["${file_path}"]=${covered}
                    ;;
                *)
                    echo "Warning: Unknown coverage type: ${coverage_type}" >&2
                    ;;
            esac
        fi
    done < "${temp_output}"
    
    # Clean up
    rm -f "${temp_input}" "${temp_output}"
    
    return 0
}

# Global ignore patterns and source file cache loaded once
IGNORE_PATTERNS_LOADED=""
IGNORE_PATTERNS=()
SOURCE_FILE_CACHE_LOADED=""
SOURCE_FILES_CACHE=()

# Function to load ignore patterns from .trial-ignore file
# Usage: load_ignore_patterns <project_root>
load_ignore_patterns() {
    local project_root="$1"
    
    # Only load once
    if [[ "${IGNORE_PATTERNS_LOADED}" == "true" ]]; then
        return 0
    fi
    
    IGNORE_PATTERNS=()
    
    # Add hardcoded filters
    IGNORE_PATTERNS+=("test_")
    IGNORE_PATTERNS+=("jansson")
    IGNORE_PATTERNS+=("microhttpd")
    IGNORE_PATTERNS+=("src/unity.c")
    
    # Load patterns from .trial-ignore
    if [[ -f "${project_root}/.trial-ignore" ]]; then
        # Use more efficient reading with mapfile
        local lines=()
        mapfile -t lines < "${project_root}/.trial-ignore"
        
        for line in "${lines[@]}"; do
            # Skip comments and empty lines more efficiently
            [[ "${line}" =~ ^[[:space:]]*# ]] && continue
            [[ -z "${line// }" ]] && continue
            
            # Remove leading ./ and add to patterns
            local pattern="${line#./}"
            [[ -n "${pattern}" ]] && IGNORE_PATTERNS+=("${pattern}")
        done
    fi
    
    IGNORE_PATTERNS_LOADED="true"
    return 0
}

# Function to check if a file should be ignored
# Usage: should_ignore_file <file_path> <project_root>
should_ignore_file() {
    local file_path="$1"
    local project_root="$2"
    
    # Load patterns if not loaded
    load_ignore_patterns "${project_root}"
    
    # Check against patterns
    for pattern in "${IGNORE_PATTERNS[@]}"; do
        if [[ "${file_path}" == *"${pattern}"* ]]; then
            return 0  # Ignore
        fi
    done
    
    return 1  # Do not ignore
}

# Function to load all source files from project directory
# Usage: load_source_files <project_root>
load_source_files() {
    local project_root="$1"
    
    # Only load once
    if [[ "${SOURCE_FILE_CACHE_LOADED}" == "true" ]]; then
        return 0
    fi
    
    SOURCE_FILES_CACHE=()
    local src_dir="${project_root}/src"
    
    if [[ -d "${src_dir}" ]]; then
        while IFS= read -r -d '' file; do
            # Get relative path from project root
            local rel_path="${file#"${project_root}"/}"
            
            # Skip ignored files
            if should_ignore_file "${rel_path}" "${project_root}"; then
                continue
            fi
            
            # Skip test files
            local basename_file
            basename_file=$(basename "${file}")
            if [[ "${basename_file}" == "test_"* ]]; then
                continue
            fi
            
            SOURCE_FILES_CACHE+=("${rel_path}")
        done < <(find "${src_dir}" -type f \( -name "*.c" -o -name "*.h" \) -print0 2>/dev/null || true)
    fi
    
    SOURCE_FILE_CACHE_LOADED="true"
    return 0
}

# Function to identify uncovered source files
# Usage: identify_uncovered_files <project_root>
identify_uncovered_files() {
    local project_root="$1"
    local uncovered_files=()
    
    # Load source files
    load_source_files "${project_root}"
    
    # Create temporary file for uncovered files
    local temp_uncovered
    temp_uncovered=$(mktemp)
    
    # Process each source file
    for file in "${SOURCE_FILES_CACHE[@]}"; do
        # Check if file has a corresponding .gcov in blackbox coverage directory
        local basename_file
        basename_file=$(basename "${file}" .c)
        # shellcheck disable=SC2154 # BLACKBOX_COVS assigned globally elsewhere
        local gcov_file="${BLACKBOX_COVS}/${basename_file}.c.gcov"
        
        # If gcov doesn't exist or has zero coverage, consider it uncovered
        if [[ ! -f "${gcov_file}" ]] || [[ $(awk '/^[ \t]*[0-9]+\*?:[ \t]*[0-9]+:/ { covered++ } END { print (covered == 0 ? 0 : 1) }' "${gcov_file}" || true) -eq 0 ]]; then
            uncovered_files+=("${file}")
            echo "${file}" >> "${temp_uncovered}"
        fi
    done
    
    # Calculate uncovered count
    local uncovered_count=${#uncovered_files[@]}
    
    # Output results
    echo "UNCOVERED_FILES_COUNT: ${uncovered_count}"
    echo "UNCOVERED_FILES:"
    cat "${temp_uncovered}"
    
    rm -f "${temp_uncovered}"
}

# Function to clean up coverage data files
# Usage: cleanup_coverage_data
cleanup_coverage_data() {
    rm -f "${UNITY_COVERAGE_FILE}" "${BLACKBOX_COVERAGE_FILE}" "${COMBINED_COVERAGE_FILE}" "${OVERLAP_COVERAGE_FILE}"
    rm -f "${UNITY_COVERAGE_FILE}.detailed" "${BLACKBOX_COVERAGE_FILE}.detailed"
    # shellcheck disable=SC2154 # GCOV_PREFIX assigned globally elsewhere
    rm -rf "${GCOV_PREFIX}" 2>/dev/null || true
    # Note: We don't remove .gcov files since they stay in their respective build directories
    # Only clean up the centralized results
    return 0
}

# Function to analyze a single gcov file and store data
analyze_gcov_file() {
    local gcov_file="$1"
    local coverage_type="$2"  # "unity" or "coverage"
    
    # Input validation - prevent hanging on empty or nonexistent files
    if [[ -z "${gcov_file}" ]]; then
        return 1
    fi
    
    if [[ ! -f "${gcov_file}" ]]; then
        return 1
    fi
    
    # Use the correct awk parsing logic for gcov format
    local line_counts
    line_counts=$(awk '
        /^[ \t]*[0-9]+\*?:[ \t]*[0-9]+:/ { covered++; total++ }
        /^[ \t]*#####:[ \t]*[0-9]+:/ { total++ }
        END {
            if (total == "") total = 0
            if (covered == "") covered = 0
            print total "," covered
        }
    ' "${gcov_file}" 2>/dev/null)
    
    local instrumented_lines="${line_counts%,*}"
    local covered_lines="${line_counts#*,}"
    
    # Default to 0 if parsing failed
    instrumented_lines=${instrumented_lines:-0}
    covered_lines=${covered_lines:-0}
    
    # Include all files, even those with 0 instrumented lines, for complete coverage table
    # This ensures the table shows all files that have gcov data, matching test 99's behavior
    
    # Extract relative path from Source: line in gcov file
    local source_line
    source_line=$(grep '^        -:    0:Source:' "${gcov_file}" | cut -d':' -f3- || true)
    local display_path
    if [[ -n "${source_line}" ]]; then
        display_path="${source_line#*/hydrogen/}"
    else
        # Fallback to basename
        display_path=$(basename "${gcov_file}" .gcov)
        display_path="src/${display_path}"
    fi
    
    # Store data in appropriate arrays
    # shellcheck disable=SC2034 # declared globally elsewhere
    all_files["${display_path}"]=1
    if [[ "${coverage_type}" == "unity" ]]; then
        # shellcheck disable=SC2034 # declared globally elsewhere
        unity_covered_lines["${display_path}"]=${covered_lines}
        # shellcheck disable=SC2034 # declared globally elsewhere
        unity_instrumented_lines["${display_path}"]=${instrumented_lines}
    else
        # shellcheck disable=SC2034 # declared globally elsewhere
        coverage_covered_lines["${display_path}"]=${covered_lines}
        # shellcheck disable=SC2034 # declared globally elsewhere
        coverage_instrumented_lines["${display_path}"]=${instrumented_lines}
    fi
}

# Function to collect and process gcov files from a directory
collect_gcov_files() {
    local build_dir="$1"
    local coverage_type="$2"
    local files_found=0
    
    if [[ -d "${build_dir}" ]]; then
        while IFS= read -r gcov_file; do
            if [[ -f "${gcov_file}" ]]; then
                # Use the exact same filtering logic as the working sections
                basename_file=$(basename "${gcov_file}")
                if [[ "${basename_file}" == "unity.c.gcov" ]] || [[ "${gcov_file}" == *"/usr/"* ]]; then
                    continue
                fi
                
                # Skip system include files that show up in Source: lines
                if grep -q "Source:/usr/include/" "${gcov_file}" 2>/dev/null; then
                    continue
                fi
                
                if [[ "${basename_file}" == "test_"* ]]; then
                    continue
                fi
                
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
                
                source_file="${basename_file%.gcov}"
                should_ignore=false
                
                if [[ -f "${project_root}/.trial-ignore" ]]; then
                    while IFS= read -r line; do
                        if [[ "${line}" =~ ^[[:space:]]*# ]] || [[ -z "${line}" ]]; then
                            continue
                        fi
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
                
                analyze_gcov_file "${gcov_file}" "${coverage_type}"
                ((files_found++))
            fi
        done < <(find "${build_dir}" -name "*.gcov" -type f 2>/dev/null || true)
    fi
    
    return "${files_found}"
}