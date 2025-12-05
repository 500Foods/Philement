#!/usr/bin/env bash

# Coverage Common Library
# Utilities and variables for coverage calculations

# LIBRARY FUNCTIONS
# analyze_combined_gcov_coverage()
# analyze_all_gcov_coverage_batch()
# load_ignore_patterns()
# should_ignore_file()
# load_source_files()
# identify_uncovered_files()
# analyze_gcov_file()
# collect_gcov_files()

# CHANGELOG
# 3.0.0 - 2025-12-05 - Added HYDROGEN_ROOT and HELIUM_ROOT environment variable checks
# 2.1.0 - 2025-08-10 - Added caching to analyze_all_gcov_coverage_batch()
# 2.0.0 - 2025-08-04 - GCOV Optimization Adventure
# 1.1.0 - 2025-07-30 - Removed cleanup_coverage_data - not needed
# 1.0.0 - 2025-07-21 - Initial version with common coverage functions

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

set -euo pipefail

# Guard clause to prevent multiple sourcing
[[ -n "${COVERAGE_COMMON_GUARD:-}" ]] && return 0
export COVERAGE_COMMON_GUARD="true"

# Library metadata
COVERAGE_COMMON_NAME="Coverage Common Library"
COVERAGE_COMMON_VERSION="3.0.0"
# shellcheck disable=SC2154 # TEST_NUMBER and TEST_COUNTER defined by caller
print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${COVERAGE_COMMON_NAME} ${COVERAGE_COMMON_VERSION}" "info" 2> /dev/null || true

# Sort out directories
PROJECT_DIR="${HYDROGEN_ROOT}"
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
export UNITY_COVERAGE_FILE BLACKBOX_COVERAGE_FILE COMBINED_COVERAGE_FILE OVERLAP_COVERAGE_FILE

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
        # shellcheck disable=SC2154 # AWK defined externally in framework.sh
        result=$("${AWK}" '
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
        result=$("${AWK}" '
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
    result=$("${AWK}" -v unity_file="${unity_gcov}" -v blackbox_file="${blackbox_gcov}" '
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
    local marker_file="${BUILD_DIR}/gcov_batch_marker"
    local cache_file="${BUILD_DIR}/gcov_batch_cache.txt"
    local pattern_checksum
    # shellcheck disable=SC2154 # PRINTF defined externally in framework.sh
    pattern_checksum=$("${PRINTF}" '%s\n' "${IGNORE_PATTERNS[@]}" | sort | sha256sum | cut -d' ' -f1 2>/dev/null || echo "no_checksum" || true)
    local pattern_marker="${BUILD_DIR}/gcov_${pattern_checksum}_marker"

    # Check if cache marker, pattern marker, and cache file exist
    if [[ -f "${marker_file}" && -f "${cache_file}" && -f "${pattern_marker}" ]]; then
        # Load cached results into global arrays
        while IFS=':' read -r coverage_type file_path results; do
            if [[ -n "${coverage_type}" && -n "${file_path}" && -n "${results}" && "${file_path}" != "" ]]; then
                local instrumented="${results%,*}"
                local covered="${results#*,}"
                if [[ -z "${instrumented}" || -z "${covered}" ]]; then
                    continue
                fi
                case "${coverage_type}" in
                    "UNITY")
                        unity_instrumented_lines["${file_path}"]=${instrumented}
                        unity_covered_lines["${file_path}"]=${covered}
                        ;;
                    "COVERAGE")
                        coverage_instrumented_lines["${file_path}"]=${instrumented}
                        coverage_covered_lines["${file_path}"]=${covered}
                        ;;
                    "COMBINED")
                        combined_instrumented_lines["${file_path}"]=${instrumented}
                        combined_covered_lines["${file_path}"]=${covered}
                        ;;
                    *)
                        ;;
                esac
            fi
        done < "${cache_file}"
        export combined_instrumented_lines combined_covered_lines
        return 0
    fi

    # Get all .gcov files from both directories
    local unity_files=()
    local blackbox_files=()
    if [[ -d "${unity_dir}" ]]; then
        # shellcheck disable=SC2154 # GREP defined externally in framework.sh
        mapfile -t unity_files < <("${FIND}" "${unity_dir}" -name "*.c.gcov" -type f | "${GREP}" -v '_test' 2>/dev/null || true)
    fi
    if [[ -d "${blackbox_dir}" ]]; then
        mapfile -t blackbox_files < <("${FIND}" "${blackbox_dir}" -name "*.c.gcov" -type f | "${GREP}" -v '_test' 2>/dev/null || true)
    fi

    # Create union of all files by relative path (with filtering)
    declare -A all_file_set
    for file in "${unity_files[@]}" "${blackbox_files[@]}"; do
        local base_dir rel_path source_path basename_file
        if [[ "${file}" == "${unity_dir}"/* ]]; then
            base_dir="${unity_dir}"
        else
            base_dir="${blackbox_dir}"
        fi
        rel_path="${file#"${base_dir}/"}"
        source_path="${rel_path%.gcov}"
        if [[ "${source_path}" != src/* ]]; then
            source_path="src/${source_path}"
        fi
        # shellcheck disable=SC2310 # We want to continue even if the test fails
        if ! should_ignore_file "${source_path}" "${PROJECT_ROOT:-${PWD}}"; then
            all_file_set["${rel_path}"]=1
        fi
    done

    # Create temporary files
    local temp_input temp_output
    temp_input=$(mktemp) || return 1
    temp_output=$(mktemp) || { rm -f "${temp_input}"; return 1; }

    # Build concatenated input in one write block
    {
        for file_relpath in "${!all_file_set[@]}"; do
            local unity_file="${unity_dir}/${file_relpath}"
            local blackbox_file="${blackbox_dir}/${file_relpath}"
            "${PRINTF}" '###FILE_START:%s###\n' "${file_relpath}"
            if [[ -f "${unity_file}" ]]; then
                "${PRINTF}" '###UNITY_START###\n'
                cat "${unity_file}"
                "${PRINTF}" '###UNITY_END###\n'
            fi
            if [[ -f "${blackbox_file}" ]]; then
                "${PRINTF}" '###BLACKBOX_START###\n'
                cat "${blackbox_file}"
                "${PRINTF}" '###BLACKBOX_END###\n'
            fi
            "${PRINTF}" '###FILE_END###\n'
        done
    } > "${temp_input}"

    # Process with AWK (incremental counting with union for instrumented)
    # shellcheck disable=SC2016 # Using single quotes to avoid variable expansion in awk
    "${AWK}" '
    BEGIN {
        current_file = ""
        current_type = ""
    }
    /^###FILE_START:/ {
        current_file = substr($0, 15)
        gsub(/###$/, "", current_file)
        unity_covered_count = 0
        blackbox_covered_count = 0
        delete combined_covered_lines  # Reset for union
        delete instrumented_lines      # Reset for instrumented union
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
        if ($0 == "###FILE_END###") {
            if (current_file != "") {
                instrumented_count = length(instrumented_lines)
                source_path = current_file
                gsub(/\.gcov$/, "", source_path)
                if (!match(source_path, /^src\//)) {
                    source_path = "src/" source_path
                }
                combined_covered_count = length(combined_covered_lines)
                print "UNITY:" source_path ":" instrumented_count "," unity_covered_count
                print "COVERAGE:" source_path ":" instrumented_count "," blackbox_covered_count
                print "COMBINED:" source_path ":" instrumented_count "," combined_covered_count
            }
        }
        current_type = ""
        next
    }
    /^[ \t]*[0-9]+\*?:[ \t]*[0-9]+:/ {
        if (current_file != "" && current_type != "") {
            split($0, parts, ":")
            gsub(/^[ \t]*|[ \t]*$/, "", parts[2])
            line_num = parts[2]
            instrumented_lines[line_num] = 1
            if (current_type == "unity") {
                unity_covered_count++
                combined_covered_lines[line_num] = 1
            } else if (current_type == "blackbox") {
                blackbox_covered_count++
                combined_covered_lines[line_num] = 1
            }
        }
        next
    }
    /^[ \t]*#####:[ \t]*[0-9]+:/ {
        if (current_file != "" && current_type != "") {
            split($0, parts, ":")
            gsub(/^[ \t]*|[ \t]*$/, "", parts[2])
            line_num = parts[2]
            instrumented_lines[line_num] = 1
        }
        next
    }
    ' "${temp_input}" > "${temp_output}"

    # Process output and save to cache
    {
        while IFS=':' read -r coverage_type file_path results; do
            if [[ -n "${coverage_type}" && -n "${file_path}" && -n "${results}" && "${file_path}" != "" ]]; then
                local instrumented="${results%,*}"
                local covered="${results#*,}"
                if [[ -z "${instrumented}" || -z "${covered}" ]]; then
                    continue
                fi
                case "${coverage_type}" in
                    "UNITY")
                        unity_instrumented_lines["${file_path}"]=${instrumented}
                        unity_covered_lines["${file_path}"]=${covered}
                        "${PRINTF}" "UNITY:%s:%s,%s\n" "${file_path}" "${instrumented}" "${covered}"
                        ;;
                    "COVERAGE")
                        coverage_instrumented_lines["${file_path}"]=${instrumented}
                        coverage_covered_lines["${file_path}"]=${covered}
                        "${PRINTF}" "COVERAGE:%s:%s,%s\n" "${file_path}" "${instrumented}" "${covered}"
                        ;;
                    "COMBINED")
                        combined_instrumented_lines["${file_path}"]=${instrumented}
                        combined_covered_lines["${file_path}"]=${covered}
                        "${PRINTF}" "COMBINED:%s:%s,%s\n" "${file_path}" "${instrumented}" "${covered}"
                        ;;
                    *)
                        ;;
                esac
            fi
        done < "${temp_output}"
    } > "${cache_file}"

    # Export combined arrays and create markers
    export combined_instrumented_lines combined_covered_lines
    touch "${marker_file}"
    touch "${pattern_marker}"
    
    # Clean up old pattern markers
    "${FIND}" "${BUILD_DIR}" -name "gcov_pattern_*" ! -name "gcov_pattern_${pattern_checksum}" -delete 2>/dev/null || true

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
    IGNORE_PATTERNS+=("mock_")
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
            # shellcheck disable=SC2310 # We want to continue even if the test fails
            if should_ignore_file "${rel_path}" "${project_root}"; then
                continue
            fi
            
            # Skip test files
            local basename_file
            basename_file=$(basename "${file}")
            if [[ "${basename_file}" == *"_test"* ]]; then
                continue
            fi
            
            SOURCE_FILES_CACHE+=("${rel_path}")
        done < <("${FIND}" "${src_dir}" -type f \( -name "*.c" \) -print0 | "${GREP}" -v '_test' 2>/dev/null || true)
    fi
    
    SOURCE_FILE_CACHE_LOADED="true"
    return 0
}

# Function to get cached source files (used by other functions)
# Usage: get_cached_source_files <project_root>
get_cached_source_files() {
    local project_root="$1"
    
    # Load source files if not already loaded
    load_source_files "${project_root}"
    
    # Output cached source files
    "${PRINTF}" '%s\n' "${SOURCE_FILES_CACHE[@]}"
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
    line_counts=$("${AWK}" '
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
    source_line=$("${GREP}" '^        -:    0:Source:' "${gcov_file}" | cut -d':' -f3- || true)
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
    local project_root="${PROJECT_DIR}"
    
    # Load ignore patterns once for efficiency
    load_ignore_patterns "${project_root}"
    
    if [[ -d "${build_dir}" ]]; then
        while IFS= read -r gcov_file; do
            if [[ -f "${gcov_file}" ]]; then
                local basename_file
                basename_file=$(basename "${gcov_file}")
                
                # Apply the same efficient filtering logic
                # Common skips
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
                
                # Skip system include files that show up in Source: lines
                if "${GREP}" -q "Source:/usr/include/" "${gcov_file}" 2>/dev/null; then
                    continue
                fi
                
                # Check if this file should be ignored using efficient function
                local source_file="${basename_file%.gcov}"
                # shellcheck disable=SC2310 # We want to continue even if the test fails
                if should_ignore_file "src/${source_file}" "${project_root}"; then
                    continue
                fi
                
                analyze_gcov_file "${gcov_file}" "${coverage_type}"
                ((files_found++))
            fi
        done < <("${FIND}" "${build_dir}" -name "*c..gcov" -type f | "${GREP}" -v '_test' 2>/dev/null || true)
    fi
    
    return "${files_found}"
}