#!/usr/bin/env bash

# Test: Coverage Collection and Analysis
# Collects and analyzes coverage data from Unity and blackbox tests

# CHANGELOG
# 3.0.0 - 2025-07-30 - Overhaul #1
# 2.0.1 - 2025-07-14 - Updated to use build/tests directories for test output consistency
# 2.0.0 - Initial version with comprehensive coverage analysis

set -euo pipefail

# Test configuration
TEST_NAME="Test Suite Coverage {BLUE}(coverage_table){RESET}"
TEST_ABBR="COV"
TEST_NUMBER="99"
TEST_COUNTER=0
TEST_VERSION="3.0.0"

# shellcheck source=tests/lib/framework.sh # Reference framework directly
[[ -n "${FRAMEWORK_GUARD:-}" ]] || source "$(dirname "${BASH_SOURCE[0]}")/lib/framework.sh"
setup_test_environment

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
    
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Files instrumented: ${formatted_unity_covered_files} of ${formatted_unity_instrumented_files} source files have coverage"
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Lines instrumented: ${formatted_unity_covered_lines} of ${formatted_unity_total_lines} executable lines covered"
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Unity test coverage recalled: ${unity_coverage_percentage}% - ${formatted_unity_covered_files} of ${formatted_unity_instrumented_files} files covered"
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
        
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Files instrumented: ${formatted_covered_files} of ${formatted_instrumented_files} source files have coverage"
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Lines instrumented: ${formatted_covered_lines} of ${formatted_total_lines} executable lines covered"
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Blackbox test coverage collected: ${coverage_percentage}% - ${formatted_covered_files} of ${formatted_instrumented_files} files covered"
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
    
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Combined coverage calculated: ${combined_coverage}% - ${formatted_covered} of ${formatted_total} lines covered"
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

# Sort and display uncovered files
if [[ ${#uncovered_files[@]} -gt 0 ]]; then
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Uncovered source files:"
    # Sort the array in place to avoid subshell issues with pipe
    mapfile -t sorted_uncovered_files < <("${PRINTF}" '%s\n' "${uncovered_files[@]}" | sort || true)
    for file in "${sorted_uncovered_files[@]}"; do
        print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "  ${file}"
    done
fi

# Calculate uncovered count from our data
uncovered_count=$((blackbox_instrumented_files - blackbox_covered_files))

# Format numbers without commas to avoid JSON parsing issues
formatted_covered_files=${blackbox_covered_files}
formatted_instrumented_files=${blackbox_instrumented_files}
formatted_uncovered_count=${uncovered_count}

print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Coverage analysis: ${formatted_covered_files} of ${formatted_instrumented_files} source files covered, ${formatted_uncovered_count} uncovered"

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Validate Unity Test Counts"
print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Validating unity test counts from multiple sources..."

# Get Test 10's total executed test count from the separate unity tests file
test10_total_executed=0
if [[ -f "${RESULTS_DIR}/unity_test_count.txt" ]]; then
    test10_total_executed=$(cat "${RESULTS_DIR}/unity_test_count.txt" 2>/dev/null || echo "0")
fi

# Parse Test 10's individual test execution results
declare -A test10_executed_counts

# Look for Test 10's diagnostic files to parse individual test results
if [[ -d "${BUILD_DIR}/tests/diagnostics" ]]; then
    # Find the most recent Test 10 diagnostic directory
    latest_test10_dir=$("${FIND}" "${BUILD_DIR}/tests/diagnostics" -name "test_10_*" -type d 2>/dev/null | sort -r | head -1 || true)

    if [[ -n "${latest_test10_dir}" ]]; then
        # Parse individual test execution results from diagnostic files
        while IFS= read -r -d '' diag_file; do
            if [[ "${diag_file}" =~ \.txt$ ]]; then
                # Extract test name from file path
                test_name=$(basename "${diag_file}" .txt)

                # Parse the test output to get executed test count
                if [[ -f "${diag_file}" ]]; then
                    # Look for "X Tests Y Failures Z Ignored" pattern
                    test_line=$("${GREP}" -E "[0-9]+ Tests [0-9]+ Failures [0-9]+ Ignored" "${diag_file}" 2>/dev/null | head -1 || true)
                    if [[ -n "${test_line}" ]]; then
                        # shellcheck disable=SC2016 # This is an awk script, not bash
                        executed_count=$("${AWK}" '{print $1}' <<< "${test_line}" || echo "0")
                        test10_executed_counts["${test_name}"]=${executed_count}
                    fi
                fi
            fi
        done < <("${FIND}" "${latest_test10_dir}" -name "*.txt" -type f -print0 2>/dev/null || true)
    fi
fi

# Calculate coverage_table.sh style count (sum of RUN_TEST calls mapped to source files)
coverage_table_count=0
declare -A unity_test_counts
declare -A grep_test_counts

# Count Unity tests per test file using grep
if [[ -d "tests/unity/src" ]]; then
    while IFS= read -r -d '' test_file; do
        # Skip if not a .c file
        [[ "${test_file}" =~ \.c$ ]] || continue

        # Get relative path from tests/unity/src/
        rel_path="${test_file#tests/unity/src/}"

        # Extract test file name
        test_basename=$(basename "${rel_path}" .c)

        # Count RUN_TEST occurrences in the test file
        test_count=$("${GREP}" -c "RUN_TEST(" "${test_file}" 2>/dev/null || echo "0")
        ignore_count=$("${GREP}" -Ec "if \(0\) RUN_TEST\(" "${test_file}" 2>/dev/null || echo "0")

        # Ensure they are valid numbers
        if ! [[ "${test_count}" =~ ^[0-9]+$ ]]; then
            test_count=0
        fi
        if ! [[ "${ignore_count}" =~ ^[0-9]+$ ]]; then
            ignore_count=0
        fi

        # Store grep count for this test file
        grep_count=$((test_count - ignore_count))
        grep_test_counts["${test_basename}"]=${grep_count}

        # Map test file to source file based on naming convention
        source_file=""
        if [[ "${test_basename}" =~ _test ]]; then
            source_name="${test_basename%%_test*}"
            source_dir=$("${DIRNAME}" "${rel_path}")

            # Handle directory mapping
            if [[ "${source_dir}" == "." ]]; then
                # Test file is in root of unity/src, map to src/
                source_file="src/${source_name}.c"
            else
                # Test file is in subdirectory, map to corresponding src subdirectory
                source_file="src/${source_dir}/${source_name}.c"
            fi

            # Add to existing count for this source file
            current_count=$(( ${unity_test_counts[${source_file}]:-0} ))
            unity_test_counts["${source_file}"]=$((current_count + grep_count))
        fi
    done < <("${FIND}" "tests/unity/src" -name "*test*.c" -type f -print0 2>/dev/null || true)

    # Sum all the counts
    for count in "${unity_test_counts[@]}"; do
        coverage_table_count=$((coverage_table_count + count))
    done
fi

# Get raw RUN_TEST count using grep
raw_runtest_count=0
if [[ -d "tests/unity/src" ]]; then
    raw_runtest_count=$("${GREP}" -r "RUN_TEST(" "tests/unity/src" 2>/dev/null | "${GREP}" -v "if (0)" | wc -l || echo "0")
fi

# Format numbers without commas to avoid JSON parsing issues
formatted_test10=${test10_total_executed}
formatted_coverage_table=${coverage_table_count}
formatted_raw=${raw_runtest_count}

print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Test counts comparison:"
print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "  Test 10 (executed): ${formatted_test10} tests"
print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "  Coverage table (mapped): ${formatted_coverage_table} tests"
print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "  Raw RUN_TEST (all): ${formatted_raw} tests"

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
discrepancy_found=false
discrepancy_details=""

if [[ "${test10_total_executed}" -ne "${coverage_table_count}" ]]; then
    discrepancy_found=true
    test10_diff=$((test10_total_executed - coverage_table_count))
    if [[ ${test10_diff} -gt 0 ]]; then
        discrepancy_details+="Test 10 has ${test10_diff} more tests than coverage table mapping. "
    else
        discrepancy_details+="Coverage table has $((coverage_table_count - test10_total_executed)) more tests than Test 10 executed. "
    fi
fi

if [[ "${coverage_table_count}" -ne "${raw_runtest_count}" ]]; then
    discrepancy_found=true
    raw_diff=$((raw_runtest_count - coverage_table_count))
    if [[ ${raw_diff} -gt 0 ]]; then
        discrepancy_details+="Raw count has ${raw_diff} more RUN_TEST calls than mapped tests. "
    else
        discrepancy_details+="Mapped tests have $((coverage_table_count - raw_runtest_count)) more tests than raw RUN_TEST calls. "
    fi
fi

if [[ ${total_discrepancies} -gt 0 ]]; then
    discrepancy_found=true
    discrepancy_details+="Found ${total_discrepancies} per-file discrepancies. "
fi

if [[ "${discrepancy_found}" = true ]]; then
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Test count discrepancies found: ${discrepancy_details}"

    # Show per-file discrepancies
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

    # Try to identify problematic test files
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Investigating potential causes..."

    # Check for test files that don't follow naming convention
    if [[ -d "tests/unity/src" ]]; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Test files not following naming convention:"
        while IFS= read -r -d '' test_file; do
            if [[ "${test_file}" =~ \.c$ ]]; then
                basename_file=$(basename "${test_file}")
                if [[ ! "${basename_file}" =~ _test ]]; then
                    rel_path="${test_file#tests/unity/src/}"
                    print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "  ${rel_path} (missing '_test' in filename)"
                fi
            fi
        done < <("${FIND}" "tests/unity/src" -name "*.c" -type f -print0 2>/dev/null || true)

        # Check for RUN_TEST calls in non-test files
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "RUN_TEST calls in non-test files:"
        while IFS= read -r -d '' non_test_file; do
            if [[ "${non_test_file}" =~ \.c$ ]]; then
                basename_file=$(basename "${non_test_file}")
                if [[ ! "${basename_file}" =~ _test ]]; then
                    runtest_count=$("${GREP}" -c "RUN_TEST(" "${non_test_file}" 2>/dev/null || echo "0")
                    if [[ "${runtest_count}" -gt 0 ]]; then
                        rel_path="${non_test_file#tests/unity/src/}"
                        print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "  ${rel_path}: ${runtest_count} RUN_TEST calls"
                    fi
                fi
            fi
        done < <("${FIND}" "tests/unity/src" -name "*.c" -type f -print0 2>/dev/null || true)
    fi

    # Check for test files with no mapped source files
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Test files with potential mapping issues:"
    for test_file in "${!unity_test_counts[@]}"; do
        if [[ "${unity_test_counts[${test_file}]}" -eq 0 ]]; then
            print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "  ${test_file}: 0 tests mapped (check naming convention)"
        fi
    done

else
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "All test counts match: ${formatted_test10} tests consistently reported"
fi

# Print test completion summary
print_test_completion "${TEST_NAME}" "${TEST_ABBR}" "${TEST_NUMBER}" "${TEST_VERSION}"

# Return status code if sourced, exit if run standalone
${ORCHESTRATION:-false} && return "${EXIT_CODE}" || exit "${EXIT_CODE}"
