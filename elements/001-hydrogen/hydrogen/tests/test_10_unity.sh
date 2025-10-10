#!/usr/bin/env bash

# Test: Unity
# Runs unit tests using the Unity framework, treating each test file as a subtest

# FUNCTIONS
# check_unity_tests_available()
# run_single_unity_test_parallel()
# run_unity_tests()

# CHANGELOG
# 3.4.0 - 2025-10-01 - Complete cache rewrite: consolidated cache file with in-memory lookups for instant processing
# 3.3.0 - 2025-10-01 - Optimized cached test processing using parallel xargs for instant results and fixed empty test array handling
# 3.2.0 - 2025-09-15 - Added test times to each test, a long-running warning, and a long-running count at the end
# 3.1.1 - 2025-08-23 - Added console dumps to give a more nuanced progress update  
# 3.1.0 - 2025-08-07 - Cleaned up use of tmp files and sorted out diagnostics files, removed extraneous function
# 3.0.0 - 2025-07-30 - Overhaul #1
# 2.4.0 - 2025-07-20 - Added guard clause to prevent multiple sourcing
# 2.3.0 - 2025-07-16 - Changed batching formula to divide tests evenly into minimum groups within CPU limit
# 2.2.0 - 2025-07-15 - Added parallel execution with proper ordering and improved output format
# 2.1.0 - 2025-07-14 - Removed Unity framework check (moved to test 01), enhanced individual test reporting with INFO lines
# 2.0.3 - 2025-07-14 - Enhanced individual test reporting with INFO lines showing test descriptions and purposes
# 2.0.2 - 2025-07-06 - Added missing shellcheck justifications
# 2.0.1 - 2025-07-06 - Removed hardcoded absolute path; now handled by log_output.sh
# 2.0.0 - 2025-07-02 - Complete rewrite to use new modular test libraries
# 1.0.0 - 2025-06-25 - Initial version for running Unity tests

set -euo pipefail

# Test configuration
TEST_NAME="Unity Unit Tests"
TEST_ABBR="UNT"
TEST_NUMBER="10"
TEST_COUNTER=0
TEST_VERSION="3.4.0"
TEST_TIMEOUT="30" # Seconds
LONG_RUNNING="0.250" # Seconds

# shellcheck source=tests/lib/framework.sh # Reference framework directly
[[ -n "${FRAMEWORK_GUARD:-}" ]] || source "$(dirname "${BASH_SOURCE[0]}")/lib/framework.sh"
setup_test_environment

# Configuration
UNITY_BUILD_DIR="${BUILD_DIR}/unity"
UNITY_CACHE_DIR="${BUILD_DIR}/cache"
UNITY_CACHE_FILE="${UNITY_CACHE_DIR}/unity_tests.cache"
TOTAL_UNITY_TESTS=0
TOTAL_UNITY_PASSED=0
TOTAL_LONG_RUNNING=0
CACHE_HITS=0
CACHE_MISSES=0

# Declare global associative array for cache data
declare -gA CACHE_DATA

# Delete this cache as one of the tests' coverages depends on it being empty
rm -rf ~/.cache/hydrogen/depdendency/*

# Function to load consolidated cache into associative array
load_consolidated_cache() {
    local cache_file="$1"
    
    # Clear any existing cache data
    CACHE_DATA=()
    
    if [[ ! -f "${cache_file}" ]]; then
        return 0
    fi
    
    # Read cache file into associative array: test_path => mtime|exit_code|test_name|test_count|passed|failed|ignored|long_running
    while IFS='|' read -r test_path mtime exit_code test_name test_count passed_count failed_count ignored_count is_long_running; do
        if [[ -n "${test_path}" ]]; then
            CACHE_DATA["${test_path}"]="${mtime}|${exit_code}|${test_name}|${test_count}|${passed_count}|${failed_count}|${ignored_count}|${is_long_running}"
        fi
    done < "${cache_file}"
}

# Function to save consolidated cache
save_consolidated_cache() {
    local cache_file="$1"
    local temp_cache="${cache_file}.tmp"
    
    mkdir -p "$(dirname "${temp_cache}")" && touch "${temp_cache}"
    
    # Write cache data to temp file
    for test_path in "${!CACHE_DATA[@]}"; do
        echo "${test_path}|${CACHE_DATA[${test_path}]}"
    done > "${temp_cache}"
    
    # Atomic move
    mv "${temp_cache}" "${cache_file}"
}

# Function to check if cached result is valid for a test
is_cache_valid_for_test() {
    local test_path="$1"
    local test_exe="$2"
    
    # Check if we have cache data for this test
    if [[ -z "${CACHE_DATA[${test_path}]:-}" ]]; then
        return 1
    fi
    
    # Check if executable exists
    if [[ ! -f "${test_exe}" ]]; then
        return 1
    fi
    
    # Use bash -nt (newer than) test with cache file
    # If executable is newer than cache file, cache is invalid
    local cache_ref_file="${UNITY_CACHE_FILE}"
    if [[ "${test_exe}" -nt "${cache_ref_file}" ]]; then
        return 1
    fi
    
    return 0
}

# Function to update cache entry for a test
update_cache_entry() {
    local test_path="$1"
    local test_exe="$2"
    local result_file="$3"
    
    if [[ ! -f "${test_exe}" ]] || [[ ! -f "${result_file}" ]]; then
        return 1
    fi
    
    # Use SECONDS as simple timestamp (seconds since script start is fine for cache validation)
    local current_time="${SECONDS}"
    
    # Read result data using bash builtin
    local result_data
    result_data=$(<"${result_file}")
    
    # Update cache data: test_path => timestamp|result_data
    CACHE_DATA["${test_path}"]="${current_time}|${result_data}"
}

# Function to check Unity tests are available via CTest (assumes they're already built by main build system)
check_unity_tests_available() {

    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Check Unity Tests Available"
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Checking for Unity tests via CTest (should be built by main build system)..."
    
    # Check if CMake build directory exists and has CTest configuration
    local cmake_build_dir="${BUILD_DIR}"
    
    if [[ ! -d "${cmake_build_dir}" ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "CMake build directory not found: ${cmake_build_dir#"${SCRIPT_DIR}"/..}"
        return 1
    fi
    
    # Change to CMake directory to check for Unity tests
    cd "${cmake_build_dir}" || { print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Failed to change to CMake build directory"; return 1; }
    
    # Check if any Unity tests are available by looking for test executables
    if [[ -d "${UNITY_BUILD_DIR}/src" ]]; then
        # Look for any executable files that match test pattern
        local test_count
        test_count=$("${FIND}" "${UNITY_BUILD_DIR}/src" -name "*_test*" -type f -executable | wc -l || true)
        if [[ "${test_count}" -gt 0 ]]; then
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Unity tests available: ${test_count} test executables found in ${UNITY_BUILD_DIR#"${SCRIPT_DIR}"/..}/src"
            cd "${SCRIPT_DIR}" || { print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Failed to return to script directory"; return 1; }
            return 0
        else
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "No Unity test executables found in ${UNITY_BUILD_DIR#"${SCRIPT_DIR}"/..}/src"
            cd "${SCRIPT_DIR}" || { print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Failed to return to script directory"; return 1; }
            return 1
        fi
    else
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Unity build directory not found: ${UNITY_BUILD_DIR#"${SCRIPT_DIR}"/..}/src - Run 'cmake --build . --target unity_tests' from cmake directory first"
        cd "${SCRIPT_DIR}" || { print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Failed to return to script directory"; return 1; }
        return 1
    fi
}

# Function to run a single unity test in parallel mode
run_single_unity_test_parallel() {
    local test_name="$1"
    local result_file="$2"
    local output_file="$3"
    local test_exe="${UNITY_BUILD_DIR}/src/${test_name}"

    # Initialize result tracking
    local subtest_number=${TEST_COUNTER}
    local passed_count=0
    local failed_count=0
    local test_count=0
    local exit_code=0
    local ignored_count=0
    local is_long_running=0


    # Create output header
    {
        echo "SUBTEST_START|${subtest_number}|${test_name}"
        echo "TEST_LINE|Run Unity Test: ${test_name}"
    } >> "${output_file}"

    # This function is only called for tests that need to run (not cached)
    CACHE_MISSES=$((CACHE_MISSES + 1))
    
    if [[ ! -f "${test_exe}" ]]; then
        # Set test start time for missing executable case
        local test_start_time="${EPOCHREALTIME}"

        # Calculate test execution time (even for missing executable)
        local test_end_time="${EPOCHREALTIME}"
        local test_elapsed_seconds
        test_elapsed_seconds=$(awk "BEGIN {print ${test_end_time} - ${test_start_time}}")

        # Format time as X.XXXs
        local formatted_time
        formatted_time=$(printf "%.3fs" "${test_elapsed_seconds}")

        {
            echo "RESULT_LINE|FAIL|Unity test executable not found in ${formatted_time}: ${test_exe#"${SCRIPT_DIR}"/..}"
            echo "SUBTEST_END|${subtest_number}|${test_name}|1|0|1|${is_long_running}"
        } >> "${output_file}"
        echo "1|${test_name}|1|0|1|${is_long_running}" > "${result_file}"
        return 1
    fi
    
    # Run the Unity test and capture output
    local temp_test_log="${DIAGS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}/${test_name}.txt"
    local log_path="${LOG_PREFIX}.log"
    mkdir -p "$("${DIRNAME}" "${temp_test_log}" || true)"
    true > "${temp_test_log}"

    # Record start time for test execution measurement
    local test_start_time="${EPOCHREALTIME}"

    # Limit runtime of individual tests
    if timeout "${TEST_TIMEOUT}"s "${test_exe}" > "${temp_test_log}" 2>&1; then
        # Calculate test execution time
        local test_end_time="${EPOCHREALTIME}"
        local test_elapsed_seconds
        test_elapsed_seconds=$(awk "BEGIN {print ${test_end_time} - ${test_start_time}}")

        # Format time as X.XXXs
        local formatted_time
        formatted_time=$(printf "%.3fs" "${test_elapsed_seconds}")

        # Parse the Unity test output to get test counts
        local failure_count
        # shellcheck disable=SC2016 # Using single quotes on purpose to avoid escaping issues
        test_count=$("${GREP}" -E "[0-9]+ Tests [0-9]+ Failures [0-9]+ Ignored" "${temp_test_log}" | "${AWK}" '{print $1}' || true)
        # shellcheck disable=SC2016 # Using single quotes on purpose to avoid escaping issues
        failure_count=$("${GREP}" -E "[0-9]+ Tests [0-9]+ Failures [0-9]+ Ignored" "${temp_test_log}" | "${AWK}" '{print $3}' || true)
        # shellcheck disable=SC2016 # Using single quotes on purpose to avoid escaping issues
        ignored_count=$("${GREP}" -E "[0-9]+ Tests [0-9]+ Failures [0-9]+ Ignored" "${temp_test_log}" | "${AWK}" '{print $5}' || true)

        if [[ -n "${test_count}" ]] && [[ -n "${failure_count}" ]] && [[ -n "${ignored_count}" ]]; then
            passed_count=$((test_count - failure_count - ignored_count))

            # Check if test took longer than LONG_RUNNING seconds and add warning
            local is_long_running=0
            if awk "BEGIN {exit !(${test_elapsed_seconds} > ${LONG_RUNNING})}"; then
                echo "RESULT_LINE|WARN|Long running test: ${formatted_time}" >> "${output_file}"
                is_long_running=1
            fi

            echo "RESULT_LINE|PASS|${passed_count} passed, ${failure_count} failed in ${formatted_time}: ..${temp_test_log}" >> "${output_file}"
            failed_count=0
        else
            echo "RESULT_LINE|PASS|1 passed, 0 failed in ${formatted_time}: ${log_path}" >> "${output_file}"
            test_count=1
            passed_count=1
            failed_count=0
        fi
        exit_code=0
    else
        # Calculate test execution time (even for failed tests)
        local test_end_time="${EPOCHREALTIME}"
        local test_elapsed_seconds
        test_elapsed_seconds=$(awk "BEGIN {print ${test_end_time} - ${test_start_time}}")

        # Format time as X.XXXs
        local formatted_time
        formatted_time=$(printf "%.3fs" "${test_elapsed_seconds}")

        # Test failed - capture full output for display
        echo "RESULT_LINE|FAIL|${passed_count} passed, ${failed_count} failed in ${formatted_time}: ..${temp_test_log}" >> "${output_file}"
        while IFS= read -r line; do
            echo "OUTPUT_LINE|${line}" >> "${output_file}"
        done < "${temp_test_log}"

        # Try to get test counts even from failed output
        # shellcheck disable=SC2016 # Using single quotes on purpose to avoid escaping issues
        failure_count=$("${GREP}" -E "[0-9]+ Tests [0-9]+ Failures [0-9]+ Ignored" "${temp_test_log}" | "${AWK}" '{print $3}' || true)
        # shellcheck disable=SC2016 # Using single quotes on purpose to avoid escaping issues
        test_count=$("${GREP}" -E "[0-9]+ Tests [0-9]+ Failures [0-9]+ Ignored" "${temp_test_log}" | "${AWK}" '{print $1}' || true)
        if [[ -n "${test_count}" ]] && [[ -n "${failure_count}" ]]; then
            passed_count=$((test_count - failure_count))
            failed_count=${failure_count}
        else
            test_count=1
            passed_count=0
            failed_count=1
        fi

        # Check if failed test was also long-running
        if awk "BEGIN {exit !(${test_elapsed_seconds} > ${LONG_RUNNING})}"; then
            is_long_running=1
        fi

        exit_code=1
    fi
    
    # Add SUBTEST_END with long-running flag
    echo "SUBTEST_END|${subtest_number}|${test_name}|${test_count}|${passed_count}|${failed_count}|${ignored_count}|${is_long_running}" >> "${output_file}"
    echo "${exit_code}|${test_name}|${test_count}|${passed_count}|${failed_count}|${ignored_count}|${is_long_running}" > "${result_file}"

    # Note: Cache update happens in main process after results are collected
    return "${exit_code}"
}

# Function to run Unity tests via parallel execution
run_unity_tests() {
    local overall_result=0

    # Load consolidated cache
    load_consolidated_cache "${UNITY_CACHE_FILE}"

    # Find all Unity test executables
    local unity_build_dir="${UNITY_BUILD_DIR}/src"
    local unity_tests=()
    local cached_tests=()
    local tests_to_run=()

    if [[ -d "${unity_build_dir}" ]]; then
        # Look for executable files that match test pattern (recursively in subdirectories)
        temp_find="${LOG_PREFIX}_search_tests.log"
        "${FIND}" "${unity_build_dir}" -name "*test*" -type f -executable -print > "${temp_find}"

        while IFS= read -r test_exe; do
            if [[ -n "${test_exe}" ]] && [[ -x "${test_exe}" ]]; then
                local test_name
                test_name=$(basename "${test_exe}")
                # Accept any executable file with 'test' in the name
                if [[ "${test_name}" == *_test* ]]; then
                    # Get relative path from unity_build_dir to show directory structure
                    local relative_path
                    relative_path=$("${REALPATH}" --relative-to="${unity_build_dir}" "${test_exe}")
                    unity_tests+=("${relative_path}")

                    # Check if this test can use cached results
                    # shellcheck disable=SC2310 # We want to continue even if the function fails
                    if is_cache_valid_for_test "${relative_path}" "${test_exe}"; then
                        cached_tests+=("${relative_path}")
                    else
                        tests_to_run+=("${relative_path}")
                    fi
                fi
            fi
        done < "${temp_find}"
    fi

    if [[ ${#unity_tests[@]} -eq 0 ]]; then
        print_error "${TEST_NUMBER}" "${TEST_COUNTER}" "No Unity test executables found in ${unity_build_dir}"
        return 1
    fi

    # Process cached tests efficiently - single cache file read, instant processing
    local cached_count=${#cached_tests[@]}
    local to_run_count=${#tests_to_run[@]}

    if [[ "${cached_count}" -gt 0 ]]; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Processing ${cached_count} cached test results..."

        # Process all cached results using pure bash - no external commands
        local cached_total=0 cached_passed=0 cached_pass_count=0 cached_fail_count=0
        local cached_long_running=0 cached_failures=0
        
        for test_path in "${cached_tests[@]}"; do
            # Extract data from cache using bash parameter expansion
            local cache_entry="${CACHE_DATA[${test_path}]}"
            
            # Parse fields: timestamp|exit_code|test_name|test_count|passed|failed|ignored|long_running
            IFS='|' read -r _ exit_code _ test_count passed_count failed_count ignored_count is_long_running <<< "${cache_entry}"
            
            # Accumulate using bash arithmetic
            cached_total=$((cached_total + test_count - ignored_count))
            cached_passed=$((cached_passed + passed_count))
            
            if [[ "${exit_code}" == "0" ]]; then
                cached_pass_count=$((cached_pass_count + 1))
            else
                cached_fail_count=$((cached_fail_count + 1))
            fi
            
            if [[ "${failed_count}" -gt "0" ]]; then
                cached_failures=$((cached_failures + 1))
            fi
            
            if [[ "${is_long_running}" == "1" ]]; then
                cached_long_running=$((cached_long_running + 1))
            fi
        done
        
        CACHE_HITS=${cached_count}
        TOTAL_UNITY_TESTS=$((TOTAL_UNITY_TESTS + cached_total))
        TOTAL_UNITY_PASSED=$((TOTAL_UNITY_PASSED + cached_passed))
        TOTAL_LONG_RUNNING=$((TOTAL_LONG_RUNNING + cached_long_running))
        
        # Update test framework counters
        TEST_PASSED_COUNT=$((TEST_PASSED_COUNT + cached_pass_count))
        TEST_FAILED_COUNT=$((TEST_FAILED_COUNT + cached_fail_count))
        
        # Note any failures
        if [[ "${cached_failures}" -gt 0 ]]; then
            overall_result=1
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${cached_failures} cached tests have failures"
        fi
    fi
    
    # Only process tests that need to be run (not cached)
    local sorted_tests=()
    
    # Only sort and prepare if there are tests to run
    if [[ "${to_run_count}" -gt 0 ]]; then
        local root_tests=()
        local subdir_tests=()

        # Separate root directory tests from subdirectory tests
        for test_path in "${tests_to_run[@]}"; do
            if [[ "${test_path}" == *"/"* ]]; then
                subdir_tests+=("${test_path}")
            else
                root_tests+=("${test_path}")
            fi
        done

        # Sort root tests and subdirectory tests separately (only if not empty)
        if [[ ${#root_tests[@]} -gt 0 ]]; then
            mapfile -t root_tests < <("${PRINTF}" '%s\n' "${root_tests[@]}" | sort || true)
        fi
        if [[ ${#subdir_tests[@]} -gt 0 ]]; then
            mapfile -t subdir_tests < <("${PRINTF}" '%s\n' "${subdir_tests[@]}" | sort || true)
        fi

        # Combine in proper order: root tests first, then subdirectory tests
        sorted_tests=("${root_tests[@]}" "${subdir_tests[@]}")
    fi

    # Split tests into batches for parallel execution (only non-cached tests)
    local total_tests=${#sorted_tests[@]}
    local total_all_tests=$((cached_count + to_run_count))
    local total_failed=0
    local batch_num=0
    local filecounter=0

    if [[ "${total_tests}" -gt 0 ]]; then
        # Only calculate batch sizes if there are tests to run
        local corefactor="${CORES}"
        local number_of_groups=$(( (total_tests + corefactor - 1) / corefactor ))  # ceil(total_tests / cpu_cores)
        local batch_size=$(( (total_tests + number_of_groups - 1) / number_of_groups ))  # ceil(total_tests / number_of_groups)

        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Running ${total_tests} Unity tests in parallel batches of ~${batch_size} (max ${CORES} CPUs, ${number_of_groups} groups)"
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Total: ${total_all_tests} tests (${cached_count} cached, ${to_run_count} to run)"

        # Process tests in batches
        local i=0
        while [[ "${i}" -lt "${total_tests}" ]]; do
            local batch_tests=()
            local batch_end=$((i + batch_size))
            if [[ "${batch_end}" -gt "${total_tests}" ]]; then
                batch_end=${total_tests}
            fi

            # Create batch of tests
            for ((j=i; j<batch_end; j++)); do
                batch_tests+=("${sorted_tests[${j}]}")
            done

            batch_num=$(( batch_num + 1 ))
            local batch_count=${#batch_tests[@]}

            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Starting batch ${batch_num}: ${batch_count} tests"

            # Run batch in parallel and process results in order
            local temp_files=()
            local temp_outputs=()
            local pids=()

            # Start parallel execution for batch
            for test_path in "${batch_tests[@]}"; do
                local temp_result_file
                local temp_output_file
                temp_result_file="${LOG_PREFIX}_${filecounter}.result"
                temp_output_file="${LOG_PREFIX}_${filecounter}.output"
                temp_files+=("${temp_result_file}")
                temp_outputs+=("${temp_output_file}")
                filecounter=$(( filecounter + 1 ))
                # Run test in background
                run_single_unity_test_parallel "${test_path}" "${temp_result_file}" "${temp_output_file}" &
                pids+=($!)
            done

            # Wait for all tests in batch to complete
            for pid in "${pids[@]}"; do
                if ! wait "${pid}"; then
                    overall_result=1
                fi
            done

            # Process outputs in order (same order as input)
            for k in "${!batch_tests[@]}"; do
                local test_path="${batch_tests[${k}]}"
                local temp_output_file="${temp_outputs[${k}]}"
                local temp_result_file="${temp_files[${k}]}"

                # Display output in order
                if [[ -f "${temp_output_file}" ]] && [[ -s "${temp_output_file}" ]]; then
                    while IFS='|' read -r line_type content; do
                        case "${line_type}" in
                            "TEST_LINE")
                                print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "${content}"
                                ;;
                            "RESULT_LINE")
                                IFS='|' read -r result_type message <<< "${content}"
                                if [[ "${result_type}" = "PASS" ]]; then
                                    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "${message}"
                                elif [[ "${result_type}" = "WARN" ]]; then
                                    print_warning "${TEST_NUMBER}" "${TEST_COUNTER}" "${message}"
                                else
                                    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${message}"
                                fi
                                ;;
                            *) ;;
                        esac
                    done < "${temp_output_file}"
                fi

                # Collect statistics from result file and update cache
                if [[ -f "${temp_result_file}" ]] && [[ -s "${temp_result_file}" ]]; then
                    IFS='|' read -r exit_code test_name test_count passed_count failed_count ignored_count is_long_running < "${temp_result_file}"
                    TOTAL_UNITY_TESTS=$((TOTAL_UNITY_TESTS + test_count - ignored_count))
                    TOTAL_UNITY_PASSED=$((TOTAL_UNITY_PASSED + passed_count))
                    total_failed=$((total_failed + failed_count))
                    if [[ "${is_long_running}" == "1" ]]; then
                        TOTAL_LONG_RUNNING=$((TOTAL_LONG_RUNNING + 1))
                    fi
                    
                    # Update cache entry in main process (not subprocess)
                    local test_exe_path="${unity_build_dir}/${test_path}"
                    update_cache_entry "${test_path}" "${test_exe_path}" "${temp_result_file}"
                fi

            done

            # Clear arrays for next batch
            temp_files=()
            temp_outputs=()
            pids=()

            i=${batch_end}
        done
    else
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "All ${total_all_tests} tests are cached - no execution needed"
    fi
    
    # Save consolidated cache after any test runs
    if [[ "${total_tests}" -gt 0 ]]; then
        save_consolidated_cache "${UNITY_CACHE_FILE}"
    fi
    
    # Display summary in two parts
    local long_running_summary=""
    if [[ "${TOTAL_LONG_RUNNING}" -gt 0 ]]; then
        long_running_summary=", $("${PRINTF}" "%'d" "${TOTAL_LONG_RUNNING}" || true) long-running tests"
    fi
    local cache_summary=""
    if [[ $((CACHE_HITS + CACHE_MISSES)) -gt 0 ]]; then
        cache_summary=", $("${PRINTF}" "%'d" "${CACHE_HITS}" || true) cached, $("${PRINTF}" "%'d" "${CACHE_MISSES}" || true) executed"
    fi
    local batch_summary=""
    if [[ "${total_tests}" -gt 0 ]]; then
        batch_summary=", ${total_tests} test files ran in ${batch_num} batches"
    fi
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Unity test execution: ${total_all_tests} total tests${batch_summary}${long_running_summary}${cache_summary}"
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Unity test results: $("${PRINTF}" "%'d" "${TOTAL_UNITY_TESTS}" || true) unit tests, $("${PRINTF}" "%'d" "${TOTAL_UNITY_PASSED}" || true) passing, $("${PRINTF}" "%'d" "${total_failed}" || true) failing"

    # Store the total unity test count for other tests to use
    echo "${TOTAL_UNITY_TESTS}" > "${RESULTS_DIR}/unity_test_count.txt"

    return "${overall_result}"
}

# Check and run tests
# shellcheck disable=SC2310 # We want to continue even if the test fails
if check_unity_tests_available; then
    # shellcheck disable=SC2310 # We want to continue even if the test fails
    if run_unity_tests; then
        # Update TEST_NAME to include test results for concise display
        if [[ -n "${TOTAL_UNITY_TESTS}" ]] && [[ -n "${TOTAL_UNITY_PASSED}" ]]; then
            TEST_NAME="Unity {BLUE}($("${PRINTF}" "%'d" "${TOTAL_UNITY_PASSED}" || true) / $("${PRINTF}" "%'d" "${TOTAL_UNITY_TESTS}" || true) unit tests passed){RESET}"
        fi
    else
        EXIT_CODE=1
    fi
    
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Calculate Unity Test Coverage"
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Calculating Unity test coverage..."

    # Use the same calculation as Test 99
    unity_coverage=$(calculate_unity_coverage "${UNITY_BUILD_DIR}" "${TIMESTAMP}")

    # Always attempt to read details
    if [[ -f "${RESULTS_DIR}/coverage_unity.txt.detailed" ]]; then
        IFS=',' read -r _ _ covered_lines total_lines instrumented_files covered_files < "${RESULTS_DIR}/coverage_unity.txt.detailed"
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Unity test coverage calculated: ${unity_coverage}% ($(printf "%'d" "${covered_lines}")/$(printf "%'d" "${total_lines}") lines, $(printf "%'d" "${covered_files}")/$(printf "%'d" "${instrumented_files}") files)"
    else
        if [[ -z "${unity_coverage}" ]]; then
            unity_coverage="0.000"
        fi
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Unity test coverage calculated: ${unity_coverage}% (0/0 lines, 0/0 files) - No coverage data available"
    fi
else
    print_error "${TEST_NUMBER}" "${TEST_COUNTER}" "Unity tests not available, skipping test execution"
    EXIT_CODE=1
fi

# Print completion table
print_test_completion "${TEST_NAME}" "${TEST_ABBR}" "${TEST_NUMBER}" "${TEST_VERSION}"

## Return status code if sourced, exit if run standalone
${ORCHESTRATION:-false} && return "${EXIT_CODE}" || exit "${EXIT_CODE}"

