#!/usr/bin/env bash

# Test: Coverage Collection and Analysis
# Collects and analyzes coverage data from Unity and blackbox tests

# CHANGELOG
# 4.0.0 - 2025-00-14 - Overhaul #2 - all about the test count stuff at the end
# 3.0.0 - 2025-07-30 - Overhaul #1
# 2.0.1 - 2025-07-14 - Updated to use build/tests directories for test output consistency
# 2.0.0 - Initial version with comprehensive coverage analysis

set -euo pipefail

# Test configuration
TEST_NAME="Test Suite Coverage {BLUE}(coverage_table){RESET}"
TEST_ABBR="COV"
TEST_NUMBER="99"
TEST_COUNTER=0
TEST_VERSION="4.0.0"

# shellcheck source=tests/lib/framework.sh # Reference framework directly
[[ -n "${FRAMEWORK_GUARD:-}" ]] || source "$(dirname "${BASH_SOURCE[0]}")/lib/framework.sh"
setup_test_environment

# Function to format numbers with thousands separators
format_number() {
    local num="${1}"
    printf "%'d" "${num}" 2>/dev/null || echo "${num}"
}

# Function to format duration with leading zero if < 1s
format_duration() {
    local duration="${1}"
    local bc_result
    bc_result=$(echo "${duration} < 1" | bc -l 2>/dev/null) || bc_result="1"
    if (( bc_result )); then
        printf "%.3fs" "${duration}"
    else
        printf "%.3fs" "${duration}"
    fi
}

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Recall Unity Test Coverage"
print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Recalling coverage data from Unity tests (Test 11)..."

# Read Unity coverage data from Test 11's stored results instead of recalculating
if [[ -f "${UNITY_COVERAGE_FILE}" ]] && [[ -f "${UNITY_COVERAGE_FILE}.detailed" ]]; then
    unity_coverage_percentage=$(cat "${UNITY_COVERAGE_FILE}" 2>/dev/null || echo "0.000")
    
    # Parse detailed coverage data to get file counts
    unity_instrumented_files_count=0
    unity_covered_files_count=0
    unity_covered_lines_count=0
    unity_total_lines_count=0
    IFS=',' read -r _ _ unity_covered_lines_count unity_total_lines_count unity_instrumented_files_count unity_covered_files_count < "${UNITY_COVERAGE_FILE}.detailed"
    
    # Format numbers without commas to avoid JSON parsing issues
    formatted_unity_covered_files=${unity_covered_files_count}
    formatted_unity_instrumented_files=${unity_instrumented_files_count}
    formatted_unity_covered_lines=${unity_covered_lines_count}
    formatted_unity_total_lines=${unity_total_lines_count}
    
    formatted_unity_covered_files_display=$(format_number "${formatted_unity_covered_files}")
    formatted_unity_instrumented_files_display=$(format_number "${formatted_unity_instrumented_files}")
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Files instrumented: ${formatted_unity_covered_files_display} of ${formatted_unity_instrumented_files_display} source files have coverage"
    formatted_unity_covered_lines_display=$(format_number "${formatted_unity_covered_lines}")
    formatted_unity_total_lines_display=$(format_number "${formatted_unity_total_lines}")
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Lines instrumented: ${formatted_unity_covered_lines_display} of ${formatted_unity_total_lines_display} executable lines covered"
    unity_covered_final=$(format_number "${formatted_unity_covered_files}")
    unity_instrumented_final=$(format_number "${formatted_unity_instrumented_files}")
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Unity test coverage recalled: ${unity_coverage_percentage}% - ${unity_covered_final} of ${unity_instrumented_final} files covered"
else
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Unity coverage data not found - Run Test 10 first to generate Unity coverage data"
    EXIT_CODE=1
fi

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Collect Blackbox Test Coverage"
print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Collecting coverage data from blackbox tests..."

# Check for blackbox coverage data in build/coverage directory only
BLACKBOX_COVERAGE_DIR="${BUILD_DIR}/coverage"

if [[ -d "${BLACKBOX_COVERAGE_DIR}" ]]; then
    # Collect blackbox coverage data strictly from build/coverage
    coverage_percentage=$(calculate_blackbox_coverage "${BLACKBOX_COVERAGE_DIR}" "${TIMESTAMP}")
    result=$?
    
    if [[ "${result}" -eq 0 ]]; then
        # Parse detailed coverage data to get file counts
        instrumented_files=0
        covered_files=0
        covered_lines=0
        total_lines=0
        if [[ -f "${BLACKBOX_COVERAGE_FILE}.detailed" ]]; then
            IFS=',' read -r _ _ covered_lines total_lines instrumented_files covered_files < "${BLACKBOX_COVERAGE_FILE}.detailed"
        fi
        
        # Format numbers without commas to avoid JSON parsing issues
        formatted_covered_files=${covered_files}
        formatted_instrumented_files=${instrumented_files}
        formatted_covered_lines=${covered_lines}
        formatted_total_lines=${total_lines}
        
        bb_covered_files_display=$(format_number "${formatted_covered_files}")
        bb_instrumented_files_display=$(format_number "${formatted_instrumented_files}")
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Files instrumented: ${bb_covered_files_display} of ${bb_instrumented_files_display} source files have coverage"
        bb_covered_lines_display=$(format_number "${formatted_covered_lines}")
        bb_total_lines_display=$(format_number "${formatted_total_lines}")
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Lines instrumented: ${bb_covered_lines_display} of ${bb_total_lines_display} executable lines covered"
        bb_covered_files_final=$(format_number "${formatted_covered_files}")
        bb_instrumented_files_final=$(format_number "${formatted_instrumented_files}")
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Blackbox test coverage collected: ${coverage_percentage}% - ${bb_covered_files_final} of ${bb_instrumented_files_final} files covered"
    else
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Failed to collect blackbox test coverage"
        EXIT_CODE=1
    fi
else
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Blackbox coverage directory not found at: ${BLACKBOX_COVERAGE_DIR}"
    EXIT_CODE=1
fi

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Calculate Combined Coverage"
print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Calculating combined coverage from Unity and blackbox tests..."

# Use the same logic as coverage_table.sh
if [[ -f "${UNITY_COVERAGE_FILE}.detailed" ]] && [[ -f "${BLACKBOX_COVERAGE_FILE}.detailed" ]]; then
    # Set up the same variables that coverage_table.sh uses
    UNITY_COVS="${BUILD_DIR}/unity/src"
    BLACKBOX_COVS="${BUILD_DIR}/coverage/src"
    
    # Clear our arrays and repopulate them using the working logic from coverage-common.sh
    unset unity_covered_lines unity_instrumented_lines coverage_covered_lines coverage_instrumented_lines all_files
    # shellcheck disable=SC2034 # These are likeely populated elsewhere
    declare -A unity_covered_lines
    # shellcheck disable=SC2034 # These are likeely populated elsewhere
    declare -A unity_instrumented_lines
    # shellcheck disable=SC2034 # These are likeely populated elsewhere
    declare -A coverage_covered_lines
    # shellcheck disable=SC2034 # These are likeely populated elsewhere
    declare -A coverage_instrumented_lines
    # shellcheck disable=SC2034 # These are likeely populated elsewhere
    declare -A all_files

    # Use optimized batch processing for all coverage types
    analyze_all_gcov_coverage_batch "${UNITY_COVS}" "${BLACKBOX_COVS}"
    
    # Populate all_files array from coverage arrays (already filtered at batch level)
    for file_path in "${!unity_covered_lines[@]}"; do
        all_files["${file_path}"]=1
    done
    for file_path in "${!coverage_covered_lines[@]}"; do
        all_files["${file_path}"]=1
    done
    
    # Calculate totals for summary using batch-processed arrays
    combined_total_covered=0
    combined_total_instrumented=0
    
    for file_path in "${!all_files[@]}"; do
        combined_covered=${combined_covered_lines["${file_path}"]:-0}
        combined_instrumented=${combined_instrumented_lines["${file_path}"]:-0}
        
        combined_total_covered=$((combined_total_covered + combined_covered))
        combined_total_instrumented=$((combined_total_instrumented + combined_instrumented))
    done
    
    # Calculate combined percentage
    combined_coverage="0.000"
    if [[ "${combined_total_instrumented}" -gt 0 ]]; then
        combined_coverage=$("${AWK}" "BEGIN {printf \"%.3f\", (${combined_total_covered} / ${combined_total_instrumented}) * 100}")
    fi
    
    # Format numbers without commas to avoid JSON parsing issues
    formatted_covered=${combined_total_covered}
    formatted_total=${combined_total_instrumented}
    
    # Store the combined coverage value for other scripts to use
    echo "${combined_coverage}" > "${COMBINED_COVERAGE_FILE}"
    
    # Calculate combined file counts for the detailed file
    combined_covered_files=0
    combined_instrumented_files=0
    for file_path in "${!all_files[@]}"; do
        combined_instrumented=${combined_instrumented_lines["${file_path}"]:-0}
        combined_covered=${combined_covered_lines["${file_path}"]:-0}
        
        combined_instrumented_files=$(( combined_instrumented_files + 1 ))
        if [[ "${combined_covered}" -gt 0 ]]; then
            combined_covered_files=$(( combined_covered_files + 1 ))
        fi
    done
    
    # Store detailed combined coverage data for Test 00 to use in README
    # Format: timestamp,percentage,covered_lines,total_lines,instrumented_files,covered_files
    echo "$("${DATE}" +%Y%m%d_%H%M%S),${combined_coverage},${combined_total_covered},${combined_total_instrumented},${combined_instrumented_files},${combined_covered_files}" > "${COMBINED_COVERAGE_FILE}.detailed" || true
    
    combined_covered_display=$(format_number "${formatted_covered}")
    combined_total_display=$(format_number "${formatted_total}")
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Combined coverage calculated: ${combined_coverage}% - ${combined_covered_display} of ${combined_total_display} lines covered"
else
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Failed to calculate combined coverage: detailed coverage data not available"
    EXIT_CODE=1
fi

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Identify Uncovered Source Files"
print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Identifying source files not covered by blackbox tests..."

# Use the batch-proces coverage data to be consistent with the earlier calculations
# This ensures the file counts match between sections
blackbox_covered_files=0
blackbox_instrumented_files=0
uncovered_files=()

# Use the data we already calculated from analyze_all_gcov_coverage_batch
for file_path in "${!all_files[@]}"; do
    # Check if this file has any blackbox coverage
    blackbox_covered=${coverage_covered_lines["${file_path}"]:-0}
    
    blackbox_instrumented_files=$(( blackbox_instrumented_files + 1 ))
    if [[ "${blackbox_covered}" -gt 0 ]]; then
        blackbox_covered_files=$(( blackbox_covered_files + 1 ))
    else
        uncovered_files+=("${file_path}")
    fi
done

# Display uncovered files (sorting removed for performance)
if [[ ${#uncovered_files[@]} -gt 0 ]]; then
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Uncovered source files:"
    for file in "${uncovered_files[@]}"; do
        print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "  ${file}"
    done
fi

# Calculate uncovered count from our data
uncovered_count=$((blackbox_instrumented_files - blackbox_covered_files))

# Format numbers without commas to avoid JSON parsing issues
formatted_covered_files=${blackbox_covered_files}
formatted_instrumented_files=${blackbox_instrumented_files}
formatted_uncovered_count=${uncovered_count}

coverage_files_display=$(format_number "${formatted_covered_files}")
instrumented_files_display=$(format_number "${formatted_instrumented_files}")
uncovered_display=$(format_number "${formatted_uncovered_count}")
print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Coverage analysis: ${coverage_files_display} of ${instrumented_files_display} source files covered, ${uncovered_display} uncovered"

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Validate Unity Test Counts"
print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Validating Unity Framework unit test counts from multiple sources"

# Add timing to identify performance bottlenecks
VALIDATION_START_TIME=$(date +%s.%3N)

# Phase 1: Individual count reviews with timing
test10_start=$(date +%s.%3N)
test10_total_executed=0
if [[ -f "${RESULTS_DIR}/unity_test_count.txt" ]]; then
    test10_total_executed=$(cat "${RESULTS_DIR}/unity_test_count.txt" 2>/dev/null || echo "0")
fi
test10_end=$(date +%s.%3N)
test10_duration=$(echo "${test10_end} - ${test10_start}" | bc 2>/dev/null || echo "0")
test10_duration_display=$(format_duration "${test10_duration}")
print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Reviewing Test 10 (executed) counts: ${test10_duration_display}"

coverage_start=$(date +%s.%3N)

# Coverage (mapped) should count from diagnostic files using user's awk approach
coverage_table_count=0
if [[ -d "${BUILD_DIR}/tests/diagnostics" ]]; then
    # Find the most recent Test 10 diagnostic directory
    latest_test10_dir=$("${FIND}" "${BUILD_DIR}/tests/diagnostics" -name "test_10_*" -type d 2>/dev/null | sort -r | head -1 || true)

    if [[ -n "${latest_test10_dir}" ]]; then
        # Use user's ultra-efficient awk command to count from diagnostic files
        coverage_table_count=$("${FIND}" "${latest_test10_dir}" -name "*.txt" -type f -exec "${AWK}" "
        /Tests/ && /Failures/ && /Ignored/ {sum += \$1}
        ENDFILE {if (!/Tests/ || !/Failures/ || !/Ignored/) sum += 0}
        END {print sum}
        " {} + 2>/dev/null || echo "0")
    fi
fi

coverage_end=$(date +%s.%3N)
coverage_duration=$(echo "${coverage_end} - ${coverage_start}" | bc 2>/dev/null || echo "0")
coverage_duration_display=$(format_duration "${coverage_duration}")
print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Reviewing Coverage (mapped) counts: ${coverage_duration_display}"

raw_start=$(date +%s.%3N)
raw_runtest_count=0
if [[ -d "tests/unity/src" ]]; then
    raw_runtest_count=$("${GREP}" -r "RUN_TEST(" "tests/unity/src" --include="*.c" 2>/dev/null | "${GREP}" -v "if (0)" | wc -l || echo "0")
fi
raw_end=$(date +%s.%3N)
raw_duration=$(echo "${raw_end} - ${raw_start}" | bc 2>/dev/null || echo "0")
raw_duration_display=$(format_duration "${raw_duration}")
print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Reviewing Raw (RUN_TEST) counts: ${raw_duration_display}"

# Format numbers for display
formatted_test10="${test10_total_executed}"
formatted_coverage_table=${coverage_table_count}
formatted_raw=${raw_runtest_count}

print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Test counts comparison:"
test10_display=$(format_number "${formatted_test10}")
coverage_table_display=$(format_number "${formatted_coverage_table}")
raw_display=$(format_number "${formatted_raw}")
print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "  - Test 10 (executed): ${test10_display} tests"
print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "  - Coverage (mapped): ${coverage_table_display} tests"
print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "  - Raw (RUN_TEST): ${raw_display} tests"

if [[ "${test10_total_executed}" -eq "${coverage_table_count}" ]] && [[ "${coverage_table_count}" -eq "${raw_runtest_count}" ]]; then
    test10_final_display=$(format_number "${formatted_test10}")
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "All test counts match: ${test10_final_display} tests consistently reported"
else
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Test count discrepancies found"
fi

print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Additional test analysis"

# Always show fast A, B, C, D sections since they're useful and fast
if [[ -d "tests/unity/src" ]] && [[ -d "${BUILD_DIR}/tests/diagnostics" ]]; then
    # Find the most recent Test 10 diagnostic directory
    latest_test10_dir=$("${FIND}" "${BUILD_DIR}/tests/diagnostics" -name "test_10_*" -type d 2>/dev/null | sort -r | head -1 || true)

    if [[ -n "${latest_test10_dir}" ]]; then
        # A) Test files in tests/unity/src/ that don't match existing files in src/
        a_start=$(date +%s.%3N)
        orphan_test_files=$( find tests/unity/src -type f -name "*.c" | while read -r f; do src_f="${f/tests\/unity\/src/src}"; src_f="${src_f%%_test*.c}.c"; [[ ! -f "${src_f}" ]] && echo "${f}"; done | sort || true)
        a_end=$(date +%s.%3N)
        a_duration=$(echo "${a_end} - ${a_start}" | bc 2>/dev/null || echo "0")

        if [[ -n "${orphan_test_files}" ]]; then
            # Count the matches
            match_count=$(echo "${orphan_test_files}" | wc -l)
            a_duration_display=$(format_duration "${a_duration}")
            match_count_display=$(format_number "${match_count}")
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "A) Test files in tests/unity/src/ that don't match src/ files (${a_duration_display}): ${match_count_display} matches"
            # Process each line from the orphan_test_files output
            while IFS= read -r file; do
                [[ -z "${file}" ]] && continue
                print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "  ${file#tests/unity/src/}"
            done <<< "${orphan_test_files}"
        else
            a_duration_display=$(format_duration "${a_duration}")
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "A) Test files in tests/unity/src/ that don't match src/ files (${a_duration_display}):"
            print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "  None found"
        fi

        # B) Search src/ for occurrences of RUN_TEST
        b_start=$(date +%s.%3N)
        src_runtest_files=$("${GREP}" -nr "RUN_TEST(" "src/" 2>/dev/null | "${GREP}" -v "if (0)" | sort || true)
        b_end=$(date +%s.%3N)
        b_duration=$(echo "${b_end} - ${b_start}" | bc 2>/dev/null || echo "0")

        if [[ -n "${src_runtest_files}" ]]; then
            # Count the matches
            match_count=$(echo "${src_runtest_files}" | wc -l)
            b_duration_display=$(format_duration "${b_duration}")
            b_match_count_display=$(format_number "${match_count}")
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "B) RUN_TEST calls found in src/ directory (${b_duration_display}): ${b_match_count_display} matches"
            while IFS= read -r file; do
                [[ -z "${file}" ]] && continue
                print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "  ${file#src/}"
            done <<< "${src_runtest_files}"
        else
            b_duration_display=$(format_duration "${b_duration}")
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "B) RUN_TEST calls found in src/ directory (${b_duration_display}):"
            print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "  None found"
        fi

        # C) Test files with 0 tests (using optimized find/awk)
        c_start=$(date +%s.%3N)
        zero_test_files=$("${FIND}" "${latest_test10_dir}" -name "*.txt" -type f -exec "${AWK}" "
        BEGIN {found=0}
        /[0-9]+ Tests [0-9]+ Failures [0-9]+ Ignored/ {
            if (\$1 == 0) {
                test_name = FILENAME
                sub(/^.*\//, \"\", test_name)
                sub(/\.txt\$/, \"\", test_name)
                print test_name
            }
            found=1
            nextfile
        }
        ENDFILE {
            if (!found) {
                test_name = FILENAME
                sub(/^.*\//, \"\", test_name)
                sub(/\.txt\$/, \"\", test_name)
                print test_name \" (no test line found)\"
            }
        }
        " {} + 2>/dev/null | sort || true)
        c_end=$(date +%s.%3N)
        c_duration=$(echo "${c_end} - ${c_start}" | bc 2>/dev/null || echo "0")

        if [[ -n "${zero_test_files}" ]]; then
            # Count the matches
            match_count=$(echo "${zero_test_files}" | wc -l)
            c_duration_display=$(format_duration "${c_duration}")
            c_match_count_display=$(format_number "${match_count}")
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "C) Test files with 0 tests (${c_duration_display}): ${c_match_count_display} matches"
            while IFS= read -r file; do
                [[ -z "${file}" ]] && continue
                print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "  ${file}"
            done <<< "${zero_test_files}"
        else
            c_duration_display=$(format_duration "${c_duration}")
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "C) Test files with 0 tests (${c_duration_display}):"
            print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "  None found"
        fi

        # D) Show basic diagnostic information (simplified when counts match)
        if [[ "${test10_total_executed}" -eq "${coverage_table_count}" ]] && [[ "${coverage_table_count}" -eq "${raw_runtest_count}" ]]; then
            # Fast path - just show that diagnostic files are consistent
            diag_file_count=$("${FIND}" "${latest_test10_dir}" -name "*.txt" -type f 2>/dev/null | wc -l || echo "0")
            d_fast_duration_display=$(format_duration "0.001")
            diag_file_count_display=$(format_number "${diag_file_count}")
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "D) Test diagnostic files (${d_fast_duration_display}): ${diag_file_count_display} diagnostic files found"
            print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "  All counts consistent - no detailed comparison needed"
        else
            # Detailed analysis needed - do the expensive comparison
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "D) Detailed count comparison needed - performing expensive analysis..."
            
            # Only now do the expensive batch processing for section D
            declare -A cached_runtest_counts
            declare -A cached_ignore_counts
            
            # Use parallel xargs to count RUN_TEST occurrences
            temp_results_file=$(mktemp)
            export GREP
            "${FIND}" "tests/unity/src" -name "*.c" -type f -print0 2>/dev/null | xargs -0 -n 8 -P 4 bash -c "
                for file in \"\$@\"; do
                    basename_file=\$(basename \"\$file\" .c)
                    test_count=\$(\"\$GREP\" -c \"RUN_TEST(\" \"\$file\" 2>/dev/null || echo \"0\")
                    ignore_count=\$(\"\$GREP\" -c \"if (0) RUN_TEST(\" \"\$file\" 2>/dev/null || echo \"0\")
                    echo \"\${basename_file}:\${test_count}:\${ignore_count}\"
                done
            " _ > "${temp_results_file}" 2>/dev/null
            
            # Populate cache arrays
            while IFS=: read -r basename_file test_count ignore_count; do
                if [[ "${test_count}" =~ ^[0-9]+$ ]] && [[ "${ignore_count}" =~ ^[0-9]+$ ]]; then
                    cached_runtest_counts["${basename_file}"]=${test_count}
                    cached_ignore_counts["${basename_file}"]=${ignore_count}
                fi
            done < "${temp_results_file}"
            rm -f "${temp_results_file}"
            
            # Now do the diagnostic comparison
            diag_counts_output=$("${FIND}" "${latest_test10_dir}" -name "*.txt" -type f -exec "${AWK}" "
            BEGIN {
                test_name = \"\"
                diag_count = 0
            }
            FNR == 1 {
                test_name = FILENAME
                sub(/^.*\//, \"\", test_name)
                sub(/\.txt\$/, \"\", test_name)
                diag_count = 0
            }
            /[0-9]+ Tests [0-9]+ Failures [0-9]+ Ignored/ {
                diag_count = \$1
                print test_name \":\" diag_count
                nextfile
            }
            ENDFILE {
                if (diag_count == 0) {
                    print test_name \":0\"
                }
            }
            " {} + 2>/dev/null || true)
            
            discrepancy_files=""
            while IFS= read -r line; do
                [[ -z "${line}" ]] && continue
                IFS=: read -r test_name diag_count <<< "${line}"
                
                if [[ -n "${cached_runtest_counts["${test_name}"]:-}" ]]; then
                    source_count=${cached_runtest_counts["${test_name}"]}
                    ignore_count=${cached_ignore_counts["${test_name}"]:-0}
                    source_count=$((source_count - ignore_count))

                    if [[ "${diag_count}" != "${source_count}" ]]; then
                        discrepancy_files="${discrepancy_files}${test_name}: diag=${diag_count}, source=${source_count}"$'\n'
                    fi
                fi
            done <<< "${diag_counts_output}"

            if [[ -n "${discrepancy_files}" ]]; then
                discrepancy_files=$(echo -n "${discrepancy_files}" | sort || true)
                match_count=$(echo "${discrepancy_files}" | wc -l)
                d_match_count_display=$(format_number "${match_count}")
                print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "D) Test files with count discrepancies (detailed): ${d_match_count_display} matches"
                while IFS= read -r discrepancy; do
                    [[ -z "${discrepancy}" ]] && continue
                    print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "  ${discrepancy}"
                done <<< "${discrepancy_files}"
            else
                print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "D) Test files with count discrepancies (detailed):"
                print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "  None found"
            fi
        fi
fi
fi

# Only do expensive detailed failure analysis if counts don't match
if [[ "${test10_total_executed}" -ne "${coverage_table_count}" ]] || [[ "${coverage_table_count}" -ne "${raw_runtest_count}" ]]; then
    # Parse Test 10's individual test execution results
    declare -A test10_executed_counts
    declare -A grep_test_counts
    declare -A unity_test_counts

    # Cache filesystem results to avoid repeated expensive operations
    declare -A cached_test_files
    declare -A cached_runtest_counts
    declare -A cached_ignore_counts

    # Look for Test 10's diagnostic files to parse individual test results (parallel optimized)
    if [[ -d "${BUILD_DIR}/tests/diagnostics" ]]; then
        # Initialize empty array in case we don't find diagnostic files
        declare -A test10_executed_counts

        # Find the most recent Test 10 diagnostic directory
        latest_test10_dir=$("${FIND}" "${BUILD_DIR}/tests/diagnostics" -name "test_10_*" -type d 2>/dev/null | sort -r | head -1 || true)

        if [[ -n "${latest_test10_dir}" ]]; then
            # Ultra-efficient: single awk pass across all diagnostic files
            test10_total_executed=$("${FIND}" "${latest_test10_dir}" -name "*.txt" -type f -exec "${AWK}" "
            /Tests/ && /Failures/ && /Ignored/ {sum += \$1}
            ENDFILE {if (!/Tests/ || !/Failures/ || !/Ignored/) sum += 0}
            END {print sum}
            " {} + 2>/dev/null || echo "0")
    
            # Ultra-efficient awk-based parsing of diagnostic files
            diag_output=$("${FIND}" "${latest_test10_dir}" -name "*.txt" -type f -exec "${AWK}" "
            BEGIN {
                test_name = \"\"
                executed_count = 0
            }
            FNR == 1 {
                # Extract test name from filename
                test_name = FILENAME
                sub(/^.*\//, \"\", test_name)
                sub(/\.txt\$/, \"\", test_name)
                executed_count = 0
            }
            /[0-9]+ Tests [0-9]+ Failures [0-9]+ Ignored/ {
                executed_count = \$1
                print test_name \":\" executed_count
                nextfile  # Stop processing this file
            }
            ENDFILE {
                if (executed_count == 0) {
                    print test_name \":0\"
                }
            }
            " {} + 2>/dev/null || true)
    
            # Process the captured output
            while IFS= read -r line; do
                if [[ -n "${line}" ]]; then
                    IFS=: read -r test_name executed_count <<< "${line}"
                    test10_executed_counts["${test_name}"]=${executed_count}
                fi
            done <<< "${diag_output}"
        fi
    fi


    # Update raw count with the more accurate cached version
    raw_runtest_count=0
    for test_basename in "${!cached_runtest_counts[@]}"; do
        test_count="${cached_runtest_counts["${test_basename}"]}"
        ignore_count="${cached_ignore_counts["${test_basename}"]:-0}"

        # Ensure numeric values to avoid syntax errors
        if ! [[ "${test_count}" =~ ^[0-9]+$ ]]; then
            test_count=0
        fi
        if ! [[ "${ignore_count}" =~ ^[0-9]+$ ]]; then
            ignore_count=0
        fi

        raw_runtest_count=$((raw_runtest_count + test_count - ignore_count))
    done

    # Phase 3: Detailed comparison and discrepancy analysis (only if needed)
    formatted_test10=${test10_total_executed}
    formatted_coverage_table=${coverage_table_count}
    formatted_raw=${raw_runtest_count}

    # Check for per-file discrepancies
    per_file_discrepancies=()
    total_discrepancies=0

    for test_file in "${!grep_test_counts[@]}"; do
        grep_count=${grep_test_counts[${test_file}]}
        executed_count=${test10_executed_counts[${test_file}]:-0}

        if [[ ${grep_count} -ne ${executed_count} ]]; then
            diff=$((executed_count - grep_count))
            per_file_discrepancies+=("${test_file}: executed ${executed_count}, grep ${grep_count} (diff: ${diff})")
            total_discrepancies=$((total_discrepancies + 1))
        fi
    done

    # Check for discrepancies
    discrepancy_details=""

    if [[ "${test10_total_executed}" -ne "${coverage_table_count}" ]]; then
        
        test10_diff=$((test10_total_executed - coverage_table_count))
        if [[ "${test10_diff}" -gt 0 ]]; then
            test10_diff_display=$(format_number "${test10_diff}")
            discrepancy_details+="Test 10 has ${test10_diff_display} more tests than coverage table mapping. "
        else
            coverage_diff_calc=$((coverage_table_count - test10_total_executed))
            coverage_diff_display=$(format_number "${coverage_diff_calc}")
            discrepancy_details+="Coverage table has ${coverage_diff_display} more tests than Test 10 executed. "
        fi
    fi

    if [[ "${coverage_table_count}" -ne "${raw_runtest_count}" ]]; then
        
        raw_diff=$((raw_runtest_count - coverage_table_count))
        if [[ "${raw_diff}" -gt 0 ]]; then
            raw_diff_display=$(format_number "${raw_diff}")
            discrepancy_details+="Raw count has ${raw_diff_display} more RUN_TEST calls than mapped tests. "
        else
            mapped_diff_calc=$((coverage_table_count - raw_runtest_count))
            mapped_diff_display=$(format_number "${mapped_diff_calc}")
            discrepancy_details+="Mapped tests have ${mapped_diff_display} more tests than raw RUN_TEST calls. "
        fi
    fi

    if [[ ${total_discrepancies} -gt 0 ]]; then
        total_discrepancies_display=$(format_number "${total_discrepancies}")
        discrepancy_details+="Found ${total_discrepancies_display} per-file discrepancies. "
    fi

    # Only fail if total counts don't match - per-file discrepancies are informational when totals match
    if [[ "${test10_total_executed}" -ne "${coverage_table_count}" ]] || [[ "${coverage_table_count}" -ne "${raw_runtest_count}" ]]; then
        # Total counts don't match - this is a real failure
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Test count discrepancies found: ${discrepancy_details}"

        # Show per-file discrepancies for debugging
        if [[ ${total_discrepancies} -gt 0 ]]; then
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Per-file discrepancies (showing first 10):"
            for i in "${!per_file_discrepancies[@]}"; do
                if [[ ${i} -lt 10 ]]; then
                    print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "  ${per_file_discrepancies[${i}]}"
                fi
            done
            if [[ ${total_discrepancies} -gt 10 ]]; then
                print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "  ... and $((total_discrepancies - 10)) more"
            fi
        fi

        # Perform investigation for real failures
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Investigating potential causes..."

        # Check for test files that don't follow naming convention using cached data
        if [[ -d "tests/unity/src" ]]; then
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Test files not following naming convention:"
            for test_basename in "${!cached_test_files[@]}"; do
                test_file=${cached_test_files["${test_basename}"]}
                if [[ ! "${test_basename}" =~ _test ]]; then
                    rel_path="${test_file#tests/unity/src/}"
                    print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "  ${rel_path} (missing '_test' in filename)"
                fi
            done

            # Check for RUN_TEST calls in non-test files using cached data
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "RUN_TEST calls in non-test files:"
            for test_basename in "${!cached_test_files[@]}"; do
                test_file=${cached_test_files["${test_basename}"]}
                if [[ ! "${test_basename}" =~ _test ]]; then
                    runtest_count=${cached_runtest_counts["${test_basename}"]:-0}
                    if [[ "${runtest_count}" -gt 0 ]]; then
                        rel_path="${test_file#tests/unity/src/}"
                        print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "  ${rel_path}: ${runtest_count} RUN_TEST calls"
                    fi
                fi
            done
        fi

        # Check for test files with no mapped source files
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Test files with potential mapping issues:"
        for test_file in "${!unity_test_counts[@]}"; do
            if [[ "${unity_test_counts[${test_file}]}" -eq 0 ]]; then
                print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "  ${test_file}: 0 tests mapped (check naming convention)"
            fi
        done

    else
        # Total counts match - this is a PASS, regardless of per-file discrepancies
        test10_final_display=$(format_number "${formatted_test10}")
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "All test counts match: ${test10_final_display} tests consistently reported"

        # Optionally show per-file discrepancies as informational (not failing)
        if [[ "${total_discrepancies}" -gt 0 ]]; then
            total_discrepancies_final_display=$(format_number "${total_discrepancies}")
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Note: Found ${total_discrepancies_final_display} per-file discrepancies (informational only since totals match)"
        fi
    fi

VALIDATION_END_TIME=$(date +%s.%3N)
VALIDATION_DURATION=$(echo "${VALIDATION_END_TIME} - ${VALIDATION_START_TIME}" | bc 2>/dev/null || echo "0")
detailed_duration_display=$(format_duration "${VALIDATION_DURATION}")
print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Total validation section took: ${detailed_duration_display} (detailed path)"
fi


# Print test completion summary
print_test_completion "${TEST_NAME}" "${TEST_ABBR}" "${TEST_NUMBER}" "${TEST_VERSION}"

# Return status code if sourced, exit if run standalone
${ORCHESTRATION:-false} && return "${EXIT_CODE}" || exit "${EXIT_CODE}"
