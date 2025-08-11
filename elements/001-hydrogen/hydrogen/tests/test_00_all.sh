#!/bin/bash

# Test: Test Suite Orchestration
# Executes all tests in parallel batches or sequentially and generates a summary report

# FUNCTIONS
# show_help() 
# run_single_test() 
# run_single_test_parallel() 
# run_specific_test() 
# run_all_tests() 
# run_all_tests_parallel() 

# CHANGELOG
# 6.4.0 - 2025-08-10 - Cleaned out some mktemp calls
# 6.3.0 - 2025-08-09 - Minor log file adjustmeents
# 6.2.0 - 2025-08-07 - Support for commas in test names (ie, thousands separators)
# 6.1.1 - 2025-08-03 - Removed extraneous command -v calls
# 6.1.0 - 2025-07-31 - Added tmpfs code that was originally in Test 01
# 6.0.0 - 2025-07-22 - Upgraded for more stringent shellcheck compliance
# 5.0.0 - 2025-07-19 - Script Review: Folder variables used consistently
# 4.2.1 - 2025-07-18 - Added timestamp to Test Suite Results footer
# 4.2.0 - 2025-07-18 - Added SVG generation for coverage table and test results; integrated SVG references in README.md generation
# 4.1.0 - 2025-07-14 - Added --sequential-groups option to run specific groups sequentially while others run in parallel
# 4.0.2 - 2025-07-14 - Added 100ms delay between parallel test launches to reduce startup contention during parallel execution
# 4.0.1 - 2025-07-07 - Fixed how individual test elapsed times are stored and accessed
# 4.0.1 - 2025-07-07 - Added missing shellcheck justifications
# 4.0.0 - 2025-07-04 - Added dynamic parallel execution with group-based batching for significant performance improvement
# 3.0.1 - 2025-07-03 - Enhanced --help to list test names, suppressed non-table output, updated history
# 3.0.0 - 2025-07-02 - Complete rewrite to use lib/ scripts, simplified orchestration approach

# Test configuration
TEST_NAME="Test Suite Orchestration"
TEST_ABBR="ORC"
TEST_NUMBER="00"
TEST_VERSION="6.4.0"
export TEST_NAME TEST_ABBR TEST_NUMBER TEST_VERSION
 
# shellcheck disable=SC1091 # Resolve path statically
source "$(dirname "${BASH_SOURCE[0]}")/lib/framework.sh"
setup_orchestration_environment

print_subtest "Verifying command/version availability"

commands=(
    "grep" "sort" "bc" "jq" "awk" "sed" "xargs" "nproc" "timeout" "date" "lsof"
    "cmake" "gcc" "ninja" "curl" "websocat" "wscat"
    "git" "md5sum" "cloc"
    "cppcheck" "shellcheck" "markdownlint" "eslint" "stylelint" "htmlhint" "jsonlint" "swagger-cli"
    "Oh.sh" "tables" "coverage_table.sh"
)

# Array to store results
declare -a results

# Cache directory
CACHE_CMD_DIR="${HOME}/.cache/hydrogen/commands"
mkdir -p "${CACHE_CMD_DIR}"
export CACHE_CMD_DIR


# shellcheck disable=SC2154 # PROJECT_DIR defined externally in framework.sh
pushd "${PROJECT_DIR}" > /dev/null || return 1

# Change to parent directory and update PATH once
export PATH="${PATH}:tests/lib"

# Collect commands that need processing (cache misses)
to_process=()
cached=0
for cmd in "${commands[@]}"; do
    cache_file="${CACHE_CMD_DIR}/${cmd// /_}"  # Replace spaces if any, though none here
    if [[ -f "${cache_file}" ]]; then
        # Read cached: format "path|version"
        IFS='|' read -r cached_path cached_version < "${cache_file}"
        current_path=$(command -v "${cmd}" 2>/dev/null || echo "")
        if [[ "${current_path}" = "${cached_path}" ]]; then
            ((cached++))
            if [[ -n "${cached_version}" ]]; then
                results+=("0|${cmd} @ ${cached_path}|${cached_version}")
            else
                results+=("0|${cmd} @ ${cached_path}|no version found")
            fi
            continue
        fi
    fi
    to_process+=("${cmd}")
done

# Run version checks in parallel for misses using xargs with inlined logic
if [[ ${#to_process[@]} -gt 0 ]]; then
    # shellcheck disable=SC2016 # Script within a script doesn't make shellcheck very happy
    while IFS= read -r line; do
        results+=("${line}")
    done < <(printf "%s\n" "${to_process[@]}" | xargs -P 0 -I {} bash -c '
        cmd="{}"
        if command -v "${cmd}" >/dev/null 2>&1; then
            cmd_path=$(command -v "${cmd}")
            version=$("${cmd_path}" --version 2>&1 | grep -oE "[0-9]+\.[0-9]+([.-][0-9a-zA-Z]+)*" | head -n 1)
            if [ -n "${version}" ]; then
                echo "0|${cmd} @ ${cmd_path}|${version}"
            else
                echo "0|${cmd} @ ${cmd_path}|no version found"
            fi
            # Cache it: path|version
            cache_file="${CACHE_CMD_DIR}/${cmd// /_}"
            if [ -n "${version}" ]; then
                echo "${cmd_path}|${version}" > "${cache_file}"
            else
                echo "${cmd_path}|" > "${cache_file}"
            fi
        else
            echo "1|${cmd}|Command not found"
        fi
    ' ) || true
fi

# Sort the results array based on the message field (field 2, after the first '|')
declare -a sorted_results
while IFS= read -r line; do
    sorted_results+=("${line}")
done < <(printf "%s\n" "${results[@]}" | sort -f -t'|' -k2 || true)

# Process sorted results array and call print_result
for result in "${sorted_results[@]}"; do
    # Split result into status, message, and version (format: "status|message|version")
    IFS='|' read -r status message version <<< "${result}"
    print_result "${status}" "${message}: ${version}"
done
popd > /dev/null || return 1

dump_collected_output
clear_collected_output

# Arrays to track test results
declare -a TEST_NUMBERS
declare -a TEST_ABBREVS
declare -a TEST_VERSIONS
declare -a TEST_NAMES
declare -a TEST_SUBTESTS
declare -a TEST_PASSED
declare -a TEST_FAILED
declare -a TEST_ELAPSED

# Command line argument parsing
SKIP_TESTS=false
SEQUENTIAL_MODE=false
SEQUENTIAL_GROUPS=()
TEST_ARGS=()

# Parse all arguments
for arg in "$@"; do
    case "${arg}" in
        --skip-tests)
            SKIP_TESTS=true
            ;;
        --sequential)
            SEQUENTIAL_MODE=true
            ;;
        --sequential-groups=*)
            # Parse comma-separated list of groups
            IFS=',' read -ra groups <<< "${arg#--sequential-groups=}"
            for group in "${groups[@]}"; do
                # Validate that group is a number
                if [[ "${group}" =~ ^[0-9]+$ ]]; then
                    SEQUENTIAL_GROUPS+=("${group}")
                else
                    echo "Error: Invalid group number '${group}'. Groups must be numeric."
                    exit 1
                fi
            done
            ;;
        --help|-h)
            # Help will be handled later
            ;;
        *)
            TEST_ARGS+=("${arg}")
            ;;
    esac
done

show_help() {
    echo "Usage: $0 [test_name1 test_name2 ...] [--skip-tests] [--sequential] [--sequential-groups=M,N] [--help]"
    echo ""
    echo "Arguments:"
    echo "  test_name    Run specific tests (e.g., 01_compilation, 98_check_links)"
    echo ""
    echo "Options:"
    echo "  --skip-tests             Skip actual test execution, just show what tests would run"
    echo "  --sequential             Run tests sequentially instead of in parallel batches (default: parallel)"
    echo "  --sequential-groups=M,N  Run specific groups sequentially while others run in parallel"
    echo "  --help                   Show this help message"
    echo ""
    echo "Available Tests:"
    for test_script in "${TEST_SCRIPTS[@]}"; do
        local test_name
        test_name=$(basename "${test_script}" .sh | sed 's/test_//')
        echo "  ${test_name}"
    done
    echo ""
    echo "Examples:"
    echo "  $0                             # Run all tests in parallel batches"
    echo "  $0 01_compilation              # Run specific test"
    echo "  $0 01_compilation 03_code_size # Run multiple tests"
    echo "  $0 --sequential                # Run all tests sequentially"
    echo "  $0 --sequential-groups=3       # Run group 3x (30-39) sequentially, others in parallel"
    echo "  $0 --sequential-groups=3,4     # Run groups 3x and 4x sequentially, others in parallel"
    echo ""
    exit 0
}

# Get list of all test scripts, excluding test_00_all.sh 
TEST_SCRIPTS=()

# Add all other tests except 00
# shellcheck disable=SC2154 # Defined externally in framework.sh
while IFS= read -r script; do
    if [[ "${script}" != *"test_00_all.sh" ]]; then
        TEST_SCRIPTS+=("${script}")
    fi
done < <(find "${SCRIPT_DIR}" -name "test_*.sh" -type f | sort || true)

# Check for help flag and skip-tests in single loop
for arg in "$@"; do
    case "${arg}" in
        --help|-h) show_help ;;
        --skip-tests) SKIP_TESTS=true ;;
        *) ;;
    esac
done

# Function to run a single test and capture results
run_single_test() {
    local test_script="$1"
    local test_name
    local test_abbrev
    local test_number
    local test_version
    
    # Extract test number from script filename
    {
        read -r test_name
        read -r test_abbrev
        read -r test_number
        read -r test_version
    } < <(awk -F= '
            /^TEST_NAME=/ {gsub(/"/, "", $2); print $2}
            /^TEST_ABBR=/ {gsub(/"/, "", $2); print $2}
            /^TEST_NUMBER=/ {gsub(/"/, "", $2); print $2}
            /^TEST_VERSION=/ {gsub(/"/, "", $2); print $2}
        ' "${test_script}" || true)

    # Handle skip mode
    if [[ "${SKIP_TESTS}" = true ]]; then
        # Store skipped test results (don't print "Would run" message)
        TEST_NUMBERS+=("${test_number}")
        TEST_ABBREVS+=("${test_abbrev}")
        TEST_NAMES+=("${test_name}")
        TEST_VERSIONS+=("${test_version}")
        TEST_SUBTESTS+=(1)
        TEST_PASSED+=(0)
        TEST_FAILED+=(0)
        TEST_ELAPSED+=("0")
        return 0
    fi
    
    # Source the test script instead of running it as a separate process
    # shellcheck disable=SC1090  # Can't follow non-constant source
    source "${test_script}"
    local exit_code=$?
    
   
    # Extract test results from subtest file or use defaults
    local total_subtests=1 
    local passed_subtests=0 
    local elapsed_formatted="0.000"
    local latest_subtest_file
    local test_name_for_file
    test_name_for_file="${test_abbrev}"
    # shellcheck disable=SC2154 # RESULTS_DIR defined externally in framework.sh
    latest_subtest_file=$(find "${RESULTS_DIR}" -name "test_${test_number}_*.txt" -type f 2>/dev/null | sort -r | head -1 || true)
    
    if [[ -n "${latest_subtest_file}" ]] && [[ -f "${latest_subtest_file}" ]]; then
        IFS=',' read -r total_subtests passed_subtests test_name file_elapsed_time test_abbrev test_version < "${latest_subtest_file}" 2>/dev/null || {
            passed_subtests=$([[ ${exit_code} -eq 0 ]] && echo 1 || echo 0); test_name="${test_name_for_file}"; file_elapsed_time="0.000"; 
        }
        elapsed_formatted="${file_elapsed_time}"
    else
        passed_subtests=$([[ ${exit_code} -eq 0 ]] && echo 1 || echo 0); test_name="${test_name_for_file}"
        [[ -t 1 ]] && echo "Warning: No subtest result file found for test ${test_number}"
    fi
    
    # Calculate failed subtests
    local failed_subtests=$((total_subtests - passed_subtests))
    
    # Store results
    TEST_NUMBERS+=("${test_number}")
    TEST_ABBREVS+=("${test_abbrev}")
    TEST_NAMES+=("${test_name}")
    TEST_VERSIONS+=("${test_version}")
    TEST_SUBTESTS+=("${total_subtests}")
    TEST_PASSED+=("${passed_subtests}")
    TEST_FAILED+=("${failed_subtests}")
    TEST_ELAPSED+=("${elapsed_formatted}")
    
    return "${exit_code}"
}

# Function to run a single test in parallel mode with output capture
run_single_test_parallel() {
    local test_script="$1"
    local temp_result_file="$2"
    local temp_output_file="$3"
    local test_number
    local test_name
    
    # Extract test number from script filename
    test_number=$(basename "${test_script}" .sh | sed 's/test_//' | sed 's/_.*//' || true)
    test_name=""
    
    # Capture all output from the test
    {
        # Source the test script instead of running it as a separate process
        # shellcheck disable=SC1090  # Can't follow non-constant source
        source "${test_script}"
    } > "${temp_output_file}" 2>&1
    local exit_code=$?
    
    # Don't calculate elapsed time here either - use file's elapsed time for consistency
    # This ensures parallel execution also uses single source of truth
    local elapsed_formatted="0.000"
    
    # Look for subtest results file written by the individual test
    local total_subtests=1
    local passed_subtests=0
    local test_name_for_file
    test_name_for_file=$(basename "${test_script}" .sh | sed 's/test_[0-9]*_//' | tr '_' ' ' | sed 's/\b\w/\U&/g' || true)
    
    # Find the most recent subtest file for this test
    local latest_subtest_file
    latest_subtest_file=$(find "${RESULTS_DIR}" -name "test_${test_number}_*.txt" -type f 2>/dev/null | sort -r | head -1 || true)
    
    if [[ -n "${latest_subtest_file}" ]] && [[ -f "${latest_subtest_file}" ]]; then
        # Read subtest results, test name, and elapsed time from the file
        IFS=',' read -r total_subtests passed_subtests test_name file_elapsed_time test_abbrev test_version < "${latest_subtest_file}" 2>/dev/null || {
            total_subtests=1
            passed_subtests=$([[ ${exit_code} -eq 0 ]] && echo 1 || echo 0)
            test_name="${test_name_for_file}"
            file_elapsed_time="0.000"
        }
        
        # Use file elapsed time - SINGLE SOURCE OF TRUTH from log_output.sh
        elapsed_formatted="${file_elapsed_time}"
    else
        # Default based on exit code if no file found
        total_subtests=1
        passed_subtests=$([[ ${exit_code} -eq 0 ]] && echo 1 || echo 0)
        test_name="${test_name_for_file}"
        elapsed_formatted="0.000"
    fi
    
    # Calculate failed subtests
    local failed_subtests=$((total_subtests - passed_subtests))
    
    # Write results to temporary file for collection by main process
    echo "${test_number}|${test_name}|${total_subtests}|${passed_subtests}|${failed_subtests}|${elapsed_formatted}|${test_abbrev}|${test_version}|${exit_code}" > "${temp_result_file}"
    
    return "${exit_code}"
}

# Function to run a specific test by name
run_specific_test() {
    local test_name="$1"
    local test_script="${SCRIPT_DIR}/test_${test_name}.sh"
    
    if [[ ! -f "${test_script}" ]]; then
        echo "Error: Test script not found: ${test_script}"
        echo "Available tests:"
        find "${SCRIPT_DIR}" -name "test_*.sh" -not -name "test_00_all.sh" | sort | while read -r script; do
            local name
            name=$(basename "${script}" .sh | sed 's/test_//')
            echo "  ${name}"
        done || true
        return 1
    fi
    
    if ! run_single_test "${test_script}"; then
        return 1
    fi
    return 0
}

# Function to run all tests in parallel batches
run_all_tests_parallel() {
    local overall_exit_code=0
    
    # Create associative array to group tests by tens digit
    declare -A test_groups
    
    # Group tests dynamically by tens digit
    for test_script in "${TEST_SCRIPTS[@]}"; do
        local test_number
        test_number=$(basename "${test_script}" .sh | sed 's/test_//' | sed 's/_.*//' || true)
        local group=$((test_number / 10))
        
        if [[ -z "${test_groups[${group}]}" ]]; then
            test_groups[${group}]="${test_script}"
        else
            test_groups[${group}]="${test_groups[${group}]} ${test_script}"
        fi
    done
    
    # Execute groups in numerical order
    for group in $(printf '%s\n' "${!test_groups[@]}" | sort -n); do
        # shellcheck disable=SC2206  # We want word splitting here for the array
        local group_tests=(${test_groups[${group}]})
        
        # Check if this group should run sequentially
        local run_sequential=false
        for sequential_group in "${SEQUENTIAL_GROUPS[@]}"; do
            if [[ "${group}" = "${sequential_group}" ]]; then
                run_sequential=true
                break
            fi
        done
        
        # Handle skip mode
        if [[ "${SKIP_TESTS}" = true ]]; then
            for test_script in "${group_tests[@]}"; do
                run_single_test "${test_script}"
            done
            continue
        fi
        
        # If group should run sequentially, run all tests one by one
        if [[ "${run_sequential}" = true ]]; then
            for test_script in "${group_tests[@]}"; do
                if ! run_single_test "${test_script}"; then
                    overall_exit_code=1
                fi
            done
            continue
        fi
            
        # If only one test in group, run it normally
        if [[ ${#group_tests[@]} -eq 1 ]]; then
            if ! run_single_test "${group_tests[0]}"; then
                overall_exit_code=1
            fi
            continue
        fi
        
        # Multiple tests in group: run first one live, others in background
        local first_test="${group_tests[0]}"
        local remaining_tests=("${group_tests[@]:1}")
        
        # Create temporary files for background tests
        local temp_result_files=()
        local temp_output_files=()
        local pids=()
        
        # Start background tests (all except first)
        for test_script in "${remaining_tests[@]}"; do
            local temp_result_file
            local temp_output_file
            local test_number
            test_number=$(basename "${test_script}" .sh | sed 's/test_//' | sed 's/_.*//' || true)

            # shellcheck disable=SC2154 # TIMESTAMP defined in framework.sh
            temp_result_file="${RESULTS_DIR}/test_${test_number}_${TIMESTAMP}_output.txt"
            temp_output_file="${RESULTS_DIR}/test_${test_number}_${TIMESTAMP}_output.log"
            mkdir -p "${RESULTS_DIR}"

            temp_result_files+=("${temp_result_file}")
            temp_output_files+=("${temp_output_file}")
            
            # Run test in background
            run_single_test_parallel "${test_script}" "${temp_result_file}" "${temp_output_file}" &
            pids+=($!)
        done
        
        # Run first test in foreground (shows output immediately)
        if ! run_single_test "${first_test}"; then
            overall_exit_code=1
        fi
        
        # Wait for all background tests to complete
        for i in "${!pids[@]}"; do
            if ! wait "${pids[${i}]}"; then
                overall_exit_code=1
            fi
        done
        
        # Display outputs from background tests in order
        for i in "${!remaining_tests[@]}"; do
            local temp_output_file="${temp_output_files[${i}]}"
            if [[ -f "${temp_output_file}" ]] && [[ -s "${temp_output_file}" ]]; then
                # Display the captured output
                cat "${temp_output_file}"
            fi
        done
        
        # Collect results from background tests
        for i in "${!remaining_tests[@]}"; do
            local temp_result_file="${temp_result_files[${i}]}"
            if [[ -f "${temp_result_file}" ]] && [[ -s "${temp_result_file}" ]]; then
                # Read results from temporary file
                IFS='|' read -r test_number test_name total_subtests passed_subtests failed_subtests elapsed_formatted test_abbr test_version exit_code< "${temp_result_file}"
                
                # Store results in global arrays
                TEST_NUMBERS+=("${test_number}")
                TEST_ABBREVS+=("${test_abbr}")
                TEST_VERSIONS+=("${test_version}")
                TEST_NAMES+=("${test_name}")
                TEST_SUBTESTS+=("${total_subtests}")
                TEST_PASSED+=("${passed_subtests}")
                TEST_FAILED+=("${failed_subtests}")
                TEST_ELAPSED+=("${elapsed_formatted}")
            fi
        done
        
        # Clear arrays for next group
        temp_result_files=()
        temp_output_files=()
        pids=()
    done
    
    return "${overall_exit_code}"
}

# Function to run all tests (sequential - original behavior)
run_all_tests() {
    local overall_exit_code=0
    
    for test_script in "${TEST_SCRIPTS[@]}"; do
        if ! run_single_test "${test_script}"; then
            overall_exit_code=1
        fi
    done
    
    return "${overall_exit_code}"
}

# Execute tests based on command line arguments
OVERALL_EXIT_CODE=0

# If no specific tests provided, run all tests
if [[ ${#TEST_ARGS[@]} -eq 0 ]]; then
    # Choose execution mode based on --sequential flag
    if [[ "${SEQUENTIAL_MODE}" = true ]]; then
        if ! run_all_tests; then
            OVERALL_EXIT_CODE=1
        fi
    else
        if ! run_all_tests_parallel; then
            OVERALL_EXIT_CODE=1
        fi
    fi
else
    # Process each test argument (always sequential for specific tests)
    for test_arg in "${TEST_ARGS[@]}"; do
        # Check if it's a specific test name
        if ! run_specific_test "${test_arg}"; then
            OVERALL_EXIT_CODE=1
        fi
    done
fi

# Let's kill any stragglers that didn't exit cleanly
pkill -f hydrogen_test_    

# Get coverage percentages for display
UNITY_COVERAGE=$(get_unity_coverage)
BLACKBOX_COVERAGE=$(get_blackbox_coverage)
COMBINED_COVERAGE=$(get_combined_coverage)

# Run coverage table before displaying test results

# Save coverage table output to file and display to console using tee
coverage_table_file="${RESULTS_DIR}/coverage_table.txt"
# shellcheck disable=SC2154 # COVERAGE_EXTERNAL defined externally in framework.sh
env -i bash "${COVERAGE_EXTERNAL}" 2>/dev/null | tee "${coverage_table_file}" || true

# Launch background process to generate COVERAGE.svg from saved file
coverage_svg_path="${PROJECT_DIR}/COVERAGE.svg"
# Delete existing file before generating new one
rm -f "${coverage_svg_path}"
# Generate SVG from saved coverage table file in background
# shellcheck disable=SC2154 # OH_EXTERNAL defined externally in framework.sh
("${OH_EXTERNAL}" -i "${coverage_table_file}" -o "${coverage_svg_path}" 2>/dev/null) &

# Calculate total elapsed time
END_TIME=$(date +%s.%N 2>/dev/null)
# shellcheck disable=SC2154 # START_TIME defined externally in framework.sh
if [[ "${START_TIME}" =~ \. ]] && [[ "${END_TIME}" =~ \. ]]; then
    TOTAL_ELAPSED=$(echo "${END_TIME} - ${START_TIME}" | bc 2>/dev/null || echo "0")
    # Ensure TOTAL_ELAPSED is not empty or starts with a dot
    if [[ -z "${TOTAL_ELAPSED}" || "${TOTAL_ELAPSED}" =~ ^\. ]]; then
        TOTAL_ELAPSED="0"
    fi
else
    TOTAL_ELAPSED=$((END_TIME - START_TIME))
fi

# Calculate total running time (sum of all test execution times)
TOTAL_RUNNING_TIME=0
for i in "${!TEST_ELAPSED[@]}"; do
    elapsed_time="${TEST_ELAPSED[${i}]}"
    # Handle elapsed times that might start with a dot (e.g., ".460")
    if [[ "${elapsed_time}" =~ ^\. ]]; then
        elapsed_time="0${elapsed_time}"
    fi
    if [[ "${elapsed_time}" =~ ^[0-9]+(\.[0-9]+)?$ ]]; then
        TOTAL_RUNNING_TIME=$(echo "${TOTAL_RUNNING_TIME} + ${elapsed_time}" | bc)
    fi
done

# Let's come up with a number that represents how much code is in our test suite
SCRIPT_SCALE=$(printf "%'d" "$(cd "${SCRIPT_DIR}" && find . -type f -name "*.sh" -exec grep -vE '^\s*(#|$)' {} + | wc -l)" || true)

# Format both times as HH:MM:SS.ZZZ
TOTAL_ELAPSED_FORMATTED=$(format_time_duration "${TOTAL_ELAPSED}")
TOTAL_RUNNING_TIME_FORMATTED=$(format_time_duration "${TOTAL_RUNNING_TIME}")

# Update README.md with test results (only if not in skip mode)
if [[ "${SKIP_TESTS}" = false ]]; then
    update_readme_with_results
fi

# Create layout JSON string
# shellcheck disable=SC2154 # TC_ORC_DSP defined externally in framework.sh
layout_json_content='{
    "title": "Test Suite Results {NC}{RED}———{RESET}{BOLD}{CYAN} Unity {WHITE}'"${UNITY_COVERAGE}"'% {RESET}{RED}———{RESET}{BOLD}{CYAN} Blackbox {WHITE}'"${BLACKBOX_COVERAGE}"'% {RESET}{RED}———{RESET}{BOLD}{CYAN} Combined {WHITE}'"${COMBINED_COVERAGE}"'%{RESET}",
    "footer": "{CYAN}Elapsed {WHITE}'"${TOTAL_ELAPSED_FORMATTED}"'{RED} ——— {CYAN}Cumulative {WHITE}'"${TOTAL_RUNNING_TIME_FORMATTED}"'{RED} ——— {RESET} {CYAN}Completed {WHITE}'"${TIMESTAMP_DISPLAY}"'{RESET}",
    "footer_position": "right",
    "columns": [
        {
            "header": "Group",
            "key": "group",
            "datatype": "int",
            "width": 5,
            "visible": false,
            "break": true
        },
        {
            "header": "Test #",
            "key": "test_id",
            "datatype": "text",
            "width": 8,
            "summary": "count",
	        "justification": "right"
        },
        {
            "header": "Test Name (Test Suite loc: '"${SCRIPT_SCALE})"'",
            "key": "test_name",
            "datatype": "text",
            "width": 44,
            "justification": "left"
        },
        {
            "header": "Version",
            "key": "test_version",
            "datatype": "text",
            "width": 9,
            "justification": "left"
        },
        {
            "header": "Tests",
            "key": "tests",
            "datatype": "int",
            "width": 7,
            "justification": "right",
            "summary": "sum"
        },
        {
            "header": "Pass",
            "key": "pass",
            "datatype": "int",
            "width": 7,
            "justification": "right",
            "summary": "sum"
        },
        {
            "header": "Fail",
            "key": "fail",
            "datatype": "int",
            "width": 7,
            "justification": "right",
            "summary": "sum"
        },
        {
            "header": "Duration",
            "key": "elapsed",
            "datatype": "float",
            "width": 10,
            "justification": "right",
            "summary": "sum"
        }
    ]
}'

# Create data JSON string
data_json_content="["
for i in "${!TEST_NUMBERS[@]}"; do
    test_id="${TEST_NUMBERS[${i}]}-${TEST_ABBREVS[${i}]}"
    test_name_adjusted="${TEST_NAMES[${i}]}"
    test_name_adjusted=${test_name_adjusted//\{COMMA\}/,}

    # Calculate group (tens digit) from test number
    group=$((${TEST_NUMBERS[${i}]} / 10))
    
    if [[ "${i}" -gt 0 ]]; then
        data_json_content+=","
    fi
    data_json_content+='
    {
        "group": '"${group}"',
        "test_id": "'"${test_id}"'",
        "test_name": "'${test_name_adjusted}'",
        "test_version": "'"${TEST_VERSIONS[${i}]}"'",
        "tests": '"${TEST_SUBTESTS[${i}]}"',
        "pass": '"${TEST_PASSED[${i}]}"',
        "fail": '"${TEST_FAILED[${i}]}"',
        "elapsed": '"${TEST_ELAPSED[${i}]}"'
    }'
done
data_json_content+="]"

# Use tables executable to render the summary table
layout_json="${RESULTS_DIR}/results_layout.json"
data_json="${RESULTS_DIR}/results_data.json"

echo "${layout_json_content}" > "${layout_json}"
echo "${data_json_content}" > "${data_json}"

# Save test results table output to file and display to console using tee
results_table_file="${RESULTS_DIR}/results_table.txt"
# shellcheck disable=SC2154 # TABLES_EXTERNAL defined externally in framework.sh
"${TABLES_EXTERNAL}" "${layout_json}" "${data_json}" 2>/dev/null | tee "${results_table_file}" || true

# Generate SVG from saved results table file in background
# shellcheck disable=SC2154 # OH_EXTERNAL defined externally in framework.sh
results_svg_path="${PROJECT_DIR}/COMPLETE.svg"
rm -f "${results_svg_path}"
("${OH_EXTERNAL}" -i "${results_table_file}" -o "${results_svg_path}" 2>/dev/null) &

exit "${OVERALL_EXIT_CODE}"
