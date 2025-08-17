#!/usr/bin/env bash

# Test: Coverage Collection and Analysis
# Collects and analyzes coverage data from Unity and blackbox tests

# CHANGELOG
# 3.0.0 - 2025-07-30 - Overhaul #1
# 2.0.1 - 2025-07-14 - Updated to use build/tests directories for test output consistency
# 2.0.0 - Initial version with comprehensive coverage analysis

# Test configuration
TEST_NAME="Test Suite Coverage {BLUE}(coverage_table){RESET}"
TEST_ABBR="COV"
TEST_NUMBER="99"
TEST_VERSION="3.0.0"

# shellcheck source=tests/lib/framework.sh # Reference framework directly
[[ -n "${FRAMEWORK_GUARD}" ]] || source "$(dirname "${BASH_SOURCE[0]}")/lib/framework.sh"
setup_test_environment

print_subtest "Recall Unity Test Coverage"
print_message "Recalling coverage data from Unity tests (Test 11)..."

# Read Unity coverage data from Test 11's stored results instead of recalculating
if [[ -f "${UNITY_COVERAGE_FILE}" ]] && [[ -f "${UNITY_COVERAGE_FILE}.detailed" ]]; then
    unity_coverage_percentage=$(cat "${UNITY_COVERAGE_FILE}" 2>/dev/null || echo "0.000")
    
    # Parse detailed coverage data to get file counts
    unity_instrumented_files_count=0
    unity_covered_files_count=0
    unity_covered_lines_count=0
    unity_total_lines_count=0
    IFS=',' read -r _ _ unity_covered_lines_count unity_total_lines_count unity_instrumented_files_count unity_covered_files_count < "${UNITY_COVERAGE_FILE}.detailed"
    
    # Format numbers with thousands separators
    formatted_unity_covered_files=$(printf "%'d" "${unity_covered_files_count}")
    formatted_unity_instrumented_files=$(printf "%'d" "${unity_instrumented_files_count}")
    formatted_unity_covered_lines=$(printf "%'d" "${unity_covered_lines_count}")
    formatted_unity_total_lines=$(printf "%'d" "${unity_total_lines_count}")
    
    print_message "Files instrumented: ${formatted_unity_covered_files} of ${formatted_unity_instrumented_files} source files have coverage"
    print_message "Lines instrumented: ${formatted_unity_covered_lines} of ${formatted_unity_total_lines} executable lines covered"
    print_result 0 "Unity test coverage recalled: ${unity_coverage_percentage}% - ${formatted_unity_covered_files} of ${formatted_unity_instrumented_files} files covered"
    ((PASS_COUNT++))
else
    print_result 1 "Unity coverage data not found - Run Test 10 first to generate Unity coverage data"
    EXIT_CODE=1
fi

print_subtest "Collect Blackbox Test Coverage"
print_message "Collecting coverage data from blackbox tests..."

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
        
        # Format numbers with thousands separators
        formatted_covered_files=$(printf "%'d" "${covered_files}")
        formatted_instrumented_files=$(printf "%'d" "${instrumented_files}")
        formatted_covered_lines=$(printf "%'d" "${covered_lines}")
        formatted_total_lines=$(printf "%'d" "${total_lines}")
        
        print_message "Files instrumented: ${formatted_covered_files} of ${formatted_instrumented_files} source files have coverage"
        print_message "Lines instrumented: ${formatted_covered_lines} of ${formatted_total_lines} executable lines covered"
        print_result 0 "Blackbox test coverage collected: ${coverage_percentage}% - ${formatted_covered_files} of ${formatted_instrumented_files} files covered"
        ((PASS_COUNT++))
    else
        print_result 1 "Failed to collect blackbox test coverage"
        EXIT_CODE=1
    fi
else
    print_result 1 "Blackbox coverage directory not found at: ${BLACKBOX_COVERAGE_DIR}"
    EXIT_CODE=1
fi

print_subtest "Calculate Combined Coverage"
print_message "Calculating combined coverage from Unity and blackbox tests..."

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
    
    # Format numbers with thousands separators
    formatted_covered=$(printf "%'d" "${combined_total_covered}")
    formatted_total=$(printf "%'d" "${combined_total_instrumented}")
    
    # Store the combined coverage value for other scripts to use
    echo "${combined_coverage}" > "${COMBINED_COVERAGE_FILE}"
    
    # Calculate combined file counts for the detailed file
    combined_covered_files=0
    combined_instrumented_files=0
    for file_path in "${!all_files[@]}"; do
        combined_instrumented=${combined_instrumented_lines["${file_path}"]:-0}
        combined_covered=${combined_covered_lines["${file_path}"]:-0}
        
        ((combined_instrumented_files++))
        if [[ "${combined_covered}" -gt 0 ]]; then
            ((combined_covered_files++))
        fi
    done
    
    # Store detailed combined coverage data for Test 00 to use in README
    # Format: timestamp,percentage,covered_lines,total_lines,instrumented_files,covered_files
    echo "$("${DATE}" +%Y%m%d_%H%M%S),${combined_coverage},${combined_total_covered},${combined_total_instrumented},${combined_instrumented_files},${combined_covered_files}" > "${COMBINED_COVERAGE_FILE}.detailed" || true
    
    print_result 0 "Combined coverage calculated: ${combined_coverage}% - ${formatted_covered} of ${formatted_total} lines covered"
    ((PASS_COUNT++))
else
    print_result 1 "Failed to calculate combined coverage: detailed coverage data not available"
    EXIT_CODE=1
fi

print_subtest "Identify Uncovered Source Files"
print_message "Identifying source files not covered by blackbox tests..."

# Use the batch-proces coverage data to be consistent with the earlier calculations
# This ensures the file counts match between sections
blackbox_covered_files=0
blackbox_instrumented_files=0
uncovered_files=()

# Use the data we already calculated from analyze_all_gcov_coverage_batch
for file_path in "${!all_files[@]}"; do
    # Check if this file has any blackbox coverage
    blackbox_covered=${coverage_covered_lines["${file_path}"]:-0}
    
    ((blackbox_instrumented_files++))
    if [[ "${blackbox_covered}" -gt 0 ]]; then
        ((blackbox_covered_files++))
    else
        uncovered_files+=("${file_path}")
    fi
done

# Sort and display uncovered files
if [[ ${#uncovered_files[@]} -gt 0 ]]; then
    print_message "Uncovered source files:"
    # Sort the array in place to avoid subshell issues with pipe
    mapfile -t sorted_uncovered_files < <(printf '%s\n' "${uncovered_files[@]}" | sort || true)
    for file in "${sorted_uncovered_files[@]}"; do
        print_output "  ${file}"
    done
fi

# Calculate uncovered count from our data
uncovered_count=$((blackbox_instrumented_files - blackbox_covered_files))

# Format numbers with thousands separators
formatted_covered_files=$(printf "%'d" "${blackbox_covered_files}")
formatted_instrumented_files=$(printf "%'d" "${blackbox_instrumented_files}")
formatted_uncovered_count=$(printf "%'d" "${uncovered_count}")

print_result 0 "Coverage analysis: ${formatted_covered_files} of ${formatted_instrumented_files} source files covered, ${formatted_uncovered_count} uncovered"
((PASS_COUNT++))

# Print test completion summary
print_test_completion "${TEST_NAME}" "${TEST_ABBR}" "${TEST_NUMBER}" "${TEST_VERSION}"

# Return status code if sourced, exit if run standalone
${ORCHESTRATION:-false} && return "${EXIT_CODE}" || exit "${EXIT_CODE}"
