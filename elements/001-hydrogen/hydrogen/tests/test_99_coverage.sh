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

# Function to format numbers with thousands separators
format_number() {
    local num=$1
    printf "%'d" "$num" 2>/dev/null || echo "$num"
}

# Function to format duration with leading zero if < 1s
format_duration() {
    local duration=$1
    if (( $(echo "$duration < 1" | bc -l 2>/dev/null || echo "1") )); then
        printf "%.3fs" "$duration"
    else
        printf "%.3fs" "$duration"
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
    
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Files instrumented: $(format_number "${formatted_unity_covered_files}") of $(format_number "${formatted_unity_instrumented_files}") source files have coverage"
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Lines instrumented: $(format_number "${formatted_unity_covered_lines}") of $(format_number "${formatted_unity_total_lines}") executable lines covered"
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Unity test coverage recalled: ${unity_coverage_percentage}% - $(format_number "${formatted_unity_covered_files}") of $(format_number "${formatted_unity_instrumented_files}") files covered"
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
        
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Files instrumented: $(format_number "${formatted_covered_files}") of $(format_number "${formatted_instrumented_files}") source files have coverage"
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Lines instrumented: $(format_number "${formatted_covered_lines}") of $(format_number "${formatted_total_lines}") executable lines covered"
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Blackbox test coverage collected: ${coverage_percentage}% - $(format_number "${formatted_covered_files}") of $(format_number "${formatted_instrumented_files}") files covered"
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
    
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Combined coverage calculated: ${combined_coverage}% - $(format_number "${formatted_covered}") of $(format_number "${formatted_total}") lines covered"
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

print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Coverage analysis: $(format_number "${formatted_covered_files}") of $(format_number "${formatted_instrumented_files}") source files covered, $(format_number "${formatted_uncovered_count}") uncovered"

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
test10_duration=$(echo "$test10_end - $test10_start" | bc 2>/dev/null || echo "0")
print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Reviewing Test 10 (executed) counts: $(format_duration "${test10_duration}")"

coverage_start=$(date +%s.%3N)
coverage_table_count=0
if [[ -d "tests/unity/src" ]]; then
    # Quick count of test files with RUN_TEST calls
    while IFS= read -r -d '' c_file; do
        [[ "${c_file}" =~ \.c$ ]] || continue
        basename_file=$(basename "${c_file}" .c)
        if [[ "${basename_file}" =~ _test ]]; then
            test_count=$("${GREP}" -c "RUN_TEST(" "$c_file" 2>/dev/null || echo "0")
            ignore_count=$("${GREP}" -c "if (0) RUN_TEST(" "$c_file" 2>/dev/null || echo "0")

            # Ensure numeric values
            test_count=${test_count:-0}
            ignore_count=${ignore_count:-0}
            if ! [[ "${test_count}" =~ ^[0-9]+$ ]]; then test_count=0; fi
            if ! [[ "${ignore_count}" =~ ^[0-9]+$ ]]; then ignore_count=0; fi

            coverage_table_count=$((coverage_table_count + test_count - ignore_count))
        fi
    done < <("${FIND}" "tests/unity/src" -name "*_test*.c" -type f -print0 2>/dev/null || true)
fi
coverage_end=$(date +%s.%3N)
coverage_duration=$(echo "$coverage_end - $coverage_start" | bc 2>/dev/null || echo "0")
print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Reviewing Coverage (mapped) counts: $(format_duration "${coverage_duration}")"

raw_start=$(date +%s.%3N)
raw_runtest_count=0
if [[ -d "tests/unity/src" ]]; then
    raw_runtest_count=$("${GREP}" -r "RUN_TEST(" "tests/unity/src" --include="*.c" 2>/dev/null | "${GREP}" -v "if (0)" | wc -l || echo "0")
fi
raw_end=$(date +%s.%3N)
raw_duration=$(echo "$raw_end - $raw_start" | bc 2>/dev/null || echo "0")
print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Reviewing Raw (RUN_TEST) counts: $(format_duration "${raw_duration}")"

# Format numbers for display
formatted_test10=${test10_total_executed}
formatted_coverage_table=${coverage_table_count}
formatted_raw=${raw_runtest_count}

print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Test counts comparison:"
print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "  - Test 10 (executed): $(format_number "${formatted_test10}") tests"
print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "  - Coverage (mapped): $(format_number "${formatted_coverage_table}") tests"
print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "  - Raw (RUN_TEST): $(format_number "${formatted_raw}") tests"

# Check if totals match - if so, we're done quickly
if [[ "${test10_total_executed}" -eq "${coverage_table_count}" ]] && [[ "${coverage_table_count}" -eq "${raw_runtest_count}" ]]; then
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Test counts match"
    VALIDATION_END_TIME=$(date +%s.%3N)
    VALIDATION_DURATION=$(echo "$VALIDATION_END_TIME - $VALIDATION_START_TIME" | bc 2>/dev/null || echo "0")
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Test count validation took: $(format_duration "${VALIDATION_DURATION}")"
else
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Test counts don't match"
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Reviewing individual test file results"

    # Count issues for summary
    naming_convention_issues=0
    non_test_runtest_issues=0
    zero_test_files=0
    discrepancy_files=0

    # Quick checks without full detailed processing
    if [[ -d "tests/unity/src" ]]; then
        while IFS= read -r -d '' c_file; do
            [[ "${c_file}" =~ \.c$ ]] || continue
            basename_file=$(basename "${c_file}" .c)
            if [[ ! "${basename_file}" =~ _test ]]; then
                runtest_count=$("${GREP}" -c "RUN_TEST(" "$c_file" 2>/dev/null || echo "0")
                if [[ "${runtest_count}" -gt 0 ]]; then
                    non_test_runtest_issues=$((non_test_runtest_issues + 1))
                fi
                naming_convention_issues=$((naming_convention_issues + 1))
            fi
        done < <("${FIND}" "tests/unity/src" -name "*.c" -type f -print0 2>/dev/null || true)
    fi

    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "A) Test files not following naming convention: $(format_number "${naming_convention_issues}") files"
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "B) Raw (RUN_TEST) calls in non-test files: $(format_number "${non_test_runtest_issues}") files"
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "C) Test files with 0 tests: $(format_number "${zero_test_files}") files"
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "D) Test files with discrepancies: $(format_number "${discrepancy_files}") files"

    VALIDATION_END_TIME=$(date +%s.%3N)
    VALIDATION_DURATION=$(echo "$VALIDATION_END_TIME - $VALIDATION_START_TIME" | bc 2>/dev/null || echo "0")
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Test count validation took: $(format_duration "${VALIDATION_DURATION}")"
fi

# If we get here, counts don't match, so do detailed analysis
if [[ "${test10_total_executed}" -ne "${coverage_table_count}" ]] || [[ "${coverage_table_count}" -ne "${raw_runtest_count}" ]]; then
    # Parse Test 10's individual test execution results
    declare -A test10_executed_counts

    # Cache filesystem results to avoid repeated expensive operations
    declare -A cached_test_files
    declare -A cached_runtest_counts
    declare -A cached_ignore_counts

    # Look for Test 10's diagnostic files to parse individual test results (parallel optimized)
    if [[ -d "${BUILD_DIR}/tests/diagnostics" ]]; then
        # Initialize empty array in case we don't find diagnostic files
        test10_executed_counts=()
        DIAG_START_TIME=$(date +%s.%3N)

        # Find the most recent Test 10 diagnostic directory
        latest_test10_dir=$("${FIND}" "${BUILD_DIR}/tests/diagnostics" -name "test_10_*" -type d 2>/dev/null | sort -r | head -1 || true)

        if [[ -n "${latest_test10_dir}" ]]; then
            # Debug: Check what files we're finding
            found_files=$("${FIND}" "${latest_test10_dir}" -name "*.txt" -type f 2>/dev/null | wc -l)
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Debug: Found ${found_files} .txt files in ${latest_test10_dir}"

            # Use Test 10 style parallel processing for better performance
            diag_files=()
            diag_output=""
            while IFS= read -r -d '' file; do
                diag_files+=("$file")
            done < <("${FIND}" "${latest_test10_dir}" -name "*.txt" -type f -print0 2>/dev/null | head -20)

            # Process diagnostic files in batches like Test 10
            diag_batch_size=5
            diag_total=${#diag_files[@]}
            diag_i=0

            while [[ "${diag_i}" -lt "${diag_total}" ]]; do
                diag_batch_end=$((diag_i + diag_batch_size))
                if [[ "${diag_batch_end}" -gt "${diag_total}" ]]; then
                    diag_batch_end=${diag_total}
                fi

                diag_pids=()
                diag_temp_files=()

                # Start parallel processing for this batch
                for ((j=diag_i; j<diag_batch_end; j++)); do
                    diag_file="${diag_files[${j}]}"
                    temp_result_file="${LOG_PREFIX}_${RANDOM}.diag"
                    diag_temp_files+=("${temp_result_file}")

                    # Process file in background
                    (
                        if [[ -f "$diag_file" ]]; then
                            test_name=$(basename "$diag_file" .txt)
                            test_line=$("${GREP}" -E "[0-9]+ Tests [0-9]+ Failures [0-9]+ Ignored" "$diag_file" 2>/dev/null | head -1 || true)
                            if [[ -n "$test_line" ]]; then
                                executed_count=$("${AWK}" '{print $1}' <<< "$test_line" || echo "0")
                                echo "${test_name}:${executed_count}"
                            fi
                        fi
                    ) > "${temp_result_file}" &
                    diag_pids+=($!)
                done

                # Wait for all in batch to complete
                for pid in "${diag_pids[@]}"; do
                    wait "${pid}" 2>/dev/null || true
                done

                # Collect results from temp files
                for temp_file in "${diag_temp_files[@]}"; do
                    if [[ -f "${temp_file}" ]]; then
                        while IFS= read -r line; do
                            if [[ -n "$line" ]]; then
                                diag_output="${diag_output}${line}"$'\n'
                            fi
                        done < "${temp_file}"
                        rm -f "${temp_file}"
                    fi
                done

                diag_i=${diag_batch_end}
            done

            # Process the captured output
            while IFS= read -r line; do
                if [[ -n "$line" ]]; then
                    IFS=: read -r test_name executed_count <<< "$line"
                    test10_executed_counts["${test_name}"]=${executed_count}
                fi
            done <<< "$diag_output"

            # Debug: Check how many entries we parsed
            parsed_count=${#test10_executed_counts[@]}
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Debug: Parsed ${parsed_count} diagnostic entries"

            DIAG_END_TIME=$(date +%s.%3N)
            DIAG_DURATION=$(echo "$DIAG_END_TIME - $DIAG_START_TIME" | bc 2>/dev/null || echo "0")
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Parallel diagnostic file parsing took: $(format_duration "${DIAG_DURATION}")"
        fi
    fi

    # Calculate coverage_table.sh style count (sum of RUN_TEST calls mapped to source files)
    coverage_table_count=0
    declare -A unity_test_counts
    declare -A grep_test_counts

    # Batch process all C files using optimized parallel grep operations
    if [[ -d "tests/unity/src" ]]; then
        # Initialize arrays
        unity_test_counts=()
        grep_test_counts=()
        BATCH_START_TIME=$(date +%s.%3N)

        # Get all C files in one operation
        while IFS= read -r -d '' c_file; do
            # Skip if not a .c file
            [[ "${c_file}" =~ \.c$ ]] || continue

            # Get relative path from tests/unity/src/
            rel_path="${c_file#tests/unity/src/}"

            # Extract file name
            file_basename=$(basename "${rel_path}" .c)

            # Cache the file path for later use
            cached_test_files["${file_basename}"]="${c_file}"
        done < <("${FIND}" "tests/unity/src" -name "*.c" -type f -print0 2>/dev/null || true)

        # Process files with optimized parallel xargs approach
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Using optimized parallel xargs approach..."

        # Create a temporary file to store results from parallel processing
        temp_results_file=$(mktemp)

        # Use parallel xargs to count RUN_TEST occurrences and write to temp file
        "${FIND}" "tests/unity/src" -name "*.c" -type f -print0 2>/dev/null | xargs -0 -n 8 -P 4 bash -c '
            for file in "$@"; do
                basename_file=$(basename "$file" .c)
                rel_path=${file#tests/unity/src/}
                test_count=$("'"${GREP}"'" -c "RUN_TEST(" "$file" 2>/dev/null || echo "0")
                ignore_count=$("'"${GREP}"'" -c "if (0) RUN_TEST(" "$file" 2>/dev/null || echo "0")
                echo "${basename_file}:${test_count}:${ignore_count}:${rel_path}"
            done
        ' _ > "${temp_results_file}" 2>/dev/null

        # Store fast method results for later comparison
        fast_total=0
        declare -A file_test_counts
        declare -A file_ignore_counts

        # Read results and populate arrays
        while IFS=: read -r basename_file test_count ignore_count rel_path; do
            # Ensure they are valid numbers
            if ! [[ "${test_count}" =~ ^[0-9]+$ ]]; then
                test_count=0
            fi
            if ! [[ "${ignore_count}" =~ ^[0-9]+$ ]]; then
                ignore_count=0
            fi

            file_test_counts["${basename_file}"]=${test_count}
            file_ignore_counts["${basename_file}"]=${ignore_count}
            fast_total=$((fast_total + test_count - ignore_count))
        done < "${temp_results_file}"

        # Read results from temp file and process
        while IFS=: read -r basename_file test_count ignore_count rel_path; do
            # Ensure they are valid numbers
            if ! [[ "${test_count}" =~ ^[0-9]+$ ]]; then
                test_count=0
            fi
            if ! [[ "${ignore_count}" =~ ^[0-9]+$ ]]; then
                ignore_count=0
            fi

            cached_runtest_counts["${basename_file}"]=${test_count}
            cached_ignore_counts["${basename_file}"]=${ignore_count}

            # Only process test files for grep_test_counts and unity_test_counts
            if [[ "${basename_file}" =~ _test ]]; then
                # Store grep count for this test file
                grep_count=$((test_count - ignore_count))
                grep_test_counts["${basename_file}"]=${grep_count}

                # Map test file to source file based on naming convention
                source_name="${basename_file%%_test*}"
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
        done < "${temp_results_file}"

        # Clean up temp file
        rm -f "${temp_results_file}"

        # Sum all the counts
        for count in "${unity_test_counts[@]}"; do
            coverage_table_count=$((coverage_table_count + count))
        done

        # Debug: Check what we found
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Debug: Found ${#unity_test_counts[@]} mapped source files"
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Debug: Total coverage_table_count: ${coverage_table_count}"

        BATCH_END_TIME=$(date +%s.%3N)
        BATCH_DURATION=$(echo "$BATCH_END_TIME - $BATCH_START_TIME" | bc 2>/dev/null || echo "0")
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Optimized batch file processing took: $(format_duration "${BATCH_DURATION}")"
    fi

    # Update raw count with the more accurate cached version
    raw_runtest_count=0
    for test_basename in "${!cached_runtest_counts[@]}"; do
        test_count=${cached_runtest_counts["${test_basename}"]}
        ignore_count=${cached_ignore_counts["${test_basename}"]:-0}

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

    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Detailed test counts comparison:"
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "  Test 10 (executed): $(format_number "${formatted_test10}") tests"
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "  Coverage table (mapped): $(format_number "${formatted_coverage_table}") tests"
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "  Raw RUN_TEST (all): $(format_number "${formatted_raw}") tests"

    # Check for per-file discrepancies with debug output
    per_file_discrepancies=()
    total_discrepancies=0
    debug_test_files=0
    debug_diag_files=0

    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Debug: Found $(format_number "${#grep_test_counts[@]}") test files with RUN_TEST calls"
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Debug: Found $(format_number "${#test10_executed_counts[@]}") diagnostic entries"

    for test_file in "${!grep_test_counts[@]}"; do
        debug_test_files=$((debug_test_files + 1))
        grep_count=${grep_test_counts[${test_file}]}
        executed_count=${test10_executed_counts[${test_file}]:-0}

        if [[ ${grep_count} -ne ${executed_count} ]]; then
            diff=$((executed_count - grep_count))
            per_file_discrepancies+=("${test_file}: executed ${executed_count}, grep ${grep_count} (diff: ${diff})")
            total_discrepancies=$((total_discrepancies + 1))
        fi
    done

    # Debug: Check for test files not in diagnostics
    missing_in_diag=0
    for test_file in "${!grep_test_counts[@]}"; do
        if [[ -z "${test10_executed_counts[${test_file}]:-}" ]]; then
            missing_in_diag=$((missing_in_diag + 1))
        fi
    done

    # Debug: Check for diagnostic entries not in test files
    extra_in_diag=0
    for diag_file in "${!test10_executed_counts[@]}"; do
        if [[ -z "${grep_test_counts[${diag_file}]:-}" ]]; then
            extra_in_diag=$((extra_in_diag + 1))
        fi
    done

    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Debug: ${missing_in_diag} test files missing from diagnostics, ${extra_in_diag} extra entries in diagnostics"

    # Check for discrepancies
    discrepancy_found=false
    discrepancy_details=""

    if [[ "${test10_total_executed}" -ne "${coverage_table_count}" ]]; then
        discrepancy_found=true
        test10_diff=$((test10_total_executed - coverage_table_count))
        if [[ ${test10_diff} -gt 0 ]]; then
            discrepancy_details+="Test 10 has $(format_number "${test10_diff}") more tests than coverage table mapping. "
        else
            discrepancy_details+="Coverage table has $(format_number "$((coverage_table_count - test10_total_executed))") more tests than Test 10 executed. "
        fi
    fi

    if [[ "${coverage_table_count}" -ne "${raw_runtest_count}" ]]; then
        discrepancy_found=true
        raw_diff=$((raw_runtest_count - coverage_table_count))
        if [[ ${raw_diff} -gt 0 ]]; then
            discrepancy_details+="Raw count has $(format_number "${raw_diff}") more RUN_TEST calls than mapped tests. "
        else
            discrepancy_details+="Mapped tests have $(format_number "$((coverage_table_count - raw_runtest_count))") more tests than raw RUN_TEST calls. "
        fi
    fi

    if [[ ${total_discrepancies} -gt 0 ]]; then
        discrepancy_found=true
        discrepancy_details+="Found $(format_number "${total_discrepancies}") per-file discrepancies. "
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
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "All test counts match: $(format_number "${formatted_test10}") tests consistently reported"

        # Optionally show per-file discrepancies as informational (not failing)
        if [[ ${total_discrepancies} -gt 0 ]]; then
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Note: Found $(format_number "${total_discrepancies}") per-file discrepancies (informational only since totals match)"
        fi
    fi

    VALIDATION_END_TIME=$(date +%s.%3N)
    VALIDATION_DURATION=$(echo "$VALIDATION_END_TIME - $VALIDATION_START_TIME" | bc 2>/dev/null || echo "0")
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Total validation section took: $(format_duration "${VALIDATION_DURATION}") (detailed path)"
fi


# Print test completion summary
print_test_completion "${TEST_NAME}" "${TEST_ABBR}" "${TEST_NUMBER}" "${TEST_VERSION}"

# Return status code if sourced, exit if run standalone
${ORCHESTRATION:-false} && return "${EXIT_CODE}" || exit "${EXIT_CODE}"
