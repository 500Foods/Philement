#!/bin/bash

# Test: Test Suite Orchestration
# Executes all tests in parallel batches or sequentially and generates a summary report

# CHANGELOG
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
SCRIPT_VERSION="6.0.0"
ORCHESTRATION="true"
export ORCHESTRATION

# Sort out directories
PROJECT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && cd .. && pwd )"
SCRIPT_DIR="${PROJECT_DIR}/tests"
LIB_DIR="${SCRIPT_DIR}/lib"
BUILD_DIR="${PROJECT_DIR}/build"
TESTS_DIR="${BUILD_DIR}/tests"
RESULTS_DIR="${TESTS_DIR}/results"
DIAGS_DIR="${TESTS_DIR}/diagnostics"
LOGS_DIR="${TESTS_DIR}/logs"

# Get start time
START_TIME=$(date +%s.%N 2>/dev/null || date +%s)

# shellcheck source=tests/lib/framework.sh # Resolve path statically
source "${LIB_DIR}/framework.sh"
# shellcheck source=tests/lib/log_output.sh # Resolve path statically
source "${LIB_DIR}/log_output.sh"

# Auto-extract test number and set up environment
TEST_NUMBER=$(extract_test_number "${BASH_SOURCE[0]}")
set_test_number "${TEST_NUMBER}"
reset_subtest_counter

# Print beautiful test suite header in blue
print_test_suite_header "${TEST_NAME}" "${SCRIPT_VERSION}"

# Print framework and log output versions as they are already sourced
print_message "${FRAMEWORK_NAME} ${FRAMEWORK_VERSION}" "info"
print_message "${LOG_OUTPUT_NAME} ${LOG_OUTPUT_VERSION}" "info"

# shellcheck source=tests/lib/lifecycle.sh # Resolve path statically
source "${LIB_DIR}/lifecycle.sh"
# shellcheck source=tests/lib/network_utils.sh # Resolve path statically
source "${LIB_DIR}/network_utils.sh"
# shellcheck source=tests/lib/file_utils.sh # Resolve path statically
source "${LIB_DIR}/file_utils.sh"
# shellcheck source=tests/lib/env_utils.sh # Resolve path statically
source "${LIB_DIR}/env_utils.sh"
# shellcheck source=tests/lib/cloc.sh # Resolve path statically
source "${LIB_DIR}/cloc.sh"
# shellcheck source=tests/lib/coverage.sh # Resolve path statically
source "${LIB_DIR}/coverage.sh"
# shellcheck source=tests/lib/coverage-common.sh # Resolve path statically
source "${LIB_DIR}/coverage-common.sh"
# shellcheck source=tests/lib/coverage-unity.sh # Resolve path statically
source "${LIB_DIR}/coverage-unity.sh"
# shellcheck source=tests/lib/coverage-blackbox.sh # Resolve path statically
source "${LIB_DIR}/coverage-blackbox.sh"
# shellcheck source=tests/lib/coverage-combined.sh # Resolve path statically
source "${LIB_DIR}/coverage-combined.sh"

# List of commands to check - we're assuming grep and sort are available
commands=(
    "grep" "sort" "bc" "jq" "awk" "sed" "xargs" "nproc" "timeout"
    "cmake" "curl" "websocat" 
    "git" "md5sum" "cloc"
    "cppcheck" "shellcheck" "markdownlint" "eslint" "stylelint" "htmlhint" "jsonlint"
    "Oh.sh" "tables" "coverage_table.sh"
)

# Array to store results
declare -a results

# Cache directory
CACHE_CMD_DIR="${HOME}/.cache/.hydrogen"
mkdir -p "${CACHE_CMD_DIR}"
export CACHE_CMD_DIR

# Change to parent directory and update PATH once
cd ..
export PATH="${PATH}:tests/lib"

# Collect commands that need processing (cache misses)
to_process=()
cached=0

for cmd in "${commands[@]}"; do
    cache_file="${CACHE_CMD_DIR}/${cmd// /_}"  # Replace spaces if any, though none here
    if [ -f "${cache_file}" ]; then
        # Read cached: format "path|version"
        IFS='|' read -r cached_path cached_version < "${cache_file}"
        current_path=$(command -v "${cmd}" 2>/dev/null || echo "")
        if [ "${current_path}" = "${cached_path}" ]; then
            ((cached++))
            if [ -n "${cached_version}" ]; then
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
if [ ${#to_process[@]} -gt 0 ]; then
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
    ' )
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

# Perform cleanup before starting tests
print_message "Cleaning Build Environment" "info"
perform_cleanup

# Arrays to track test results
declare -a TEST_NUMBERS
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

# Function to update README.md with test results
update_readme_with_results() {
    local readme_file="${PROJECT_DIR}/README.md"
    
    if [[ ! -f "${readme_file}" ]]; then
        echo "Warning: README.md not found at ${readme_file}"
        return 1
    fi
    
    # Calculate summary statistics
    local total_tests=${#TEST_NUMBERS[@]}
    local total_passed=0
    local total_failed=0
    local total_subtests=0
    local total_subtests_passed=0
    local total_subtests_failed=0
    
    for i in "${!TEST_SUBTESTS[@]}"; do
        total_subtests=$((total_subtests + TEST_SUBTESTS[i]))
        total_subtests_passed=$((total_subtests_passed + TEST_PASSED[i]))
        total_failed=$((total_subtests_failed + TEST_FAILED[i]))
        if [[ "${TEST_FAILED[i]}" -gt 0 ]]; then
            total_failed=$((total_failed + 1))
        else
            total_passed=$((total_passed + 1))
        fi
    done
    
    # Generate Timestamp, Create temporary file for new README content
    local temp_readme
    temp_readme=$(mktemp) || { echo "Error: Failed to create temporary file" >&2; return 1; }
    local timestamp
    timestamp=$(date)
    
    # Process README.md line by line
    local in_test_results=false
    local in_individual_results=false
    local in_repo_info=false
    
    while IFS= read -r line; do
        if [[ "${line}" == "## Latest Test Results" ]]; then
            in_test_results=true
            {
                echo "${line}"
                echo ""
                echo "Generated on: ${timestamp}"
                echo ""
                echo "### Summary"
                echo ""
                echo "| Metric | Value |"
                echo "| ------ | ----- |"
                echo "| Total Tests | ${total_tests} |"
                echo "| Passed | ${total_passed} |"
                echo "| Failed | ${total_failed} |"
                echo "| Total Subtests | ${total_subtests} |"
                echo "| Passed Subtests | ${total_subtests_passed} |"
                echo "| Failed Subtests | ${total_subtests_failed} |"
                echo "| Elapsed Time | ${TOTAL_ELAPSED_FORMATTED} |"
                echo "| Cumulative Time | ${TOTAL_RUNNING_TIME_FORMATTED} |"
                echo ""
                echo "[Test Suite Results](COMPLETE.svg) | [Test Suite Coverage](COVERAGE.svg)"
                echo ""
                echo "### Test Coverage"
                echo ""
                
                # Get detailed coverage information with thousands separators
                local unity_coverage_detailed="${RESULTS_DIR}/unity_coverage.txt.detailed"
                local blackbox_coverage_detailed="${RESULTS_DIR}/blackbox_coverage.txt.detailed"
                local combined_coverage_detailed="${RESULTS_DIR}/combined_coverage.txt.detailed"
                
                if [[ -f "${unity_coverage_detailed}" ]] || [[ -f "${blackbox_coverage_detailed}" ]] || [[ -f "${combined_coverage_detailed}" ]]; then
                    echo "| Test Type | Files Cover | Files Instr | Lines Cover | Lines Instr | Coverage | Timestamp |"
                    echo "| --------- | ----------- | ----------- | ----------- | ----------- | -------- | --------- |"
                    
                    # Unity coverage
                    if [[ -f "${unity_coverage_detailed}" ]]; then
                        local unity_timestamp unity_coverage_pct unity_covered unity_total unity_instrumented unity_covered_files
                        IFS=',' read -r unity_timestamp unity_coverage_pct unity_covered unity_total unity_instrumented unity_covered_files < "${unity_coverage_detailed}" 2>/dev/null
                        if [[ -n "${unity_total}" ]] && [[ "${unity_total}" -gt 0 ]]; then
                            # Add thousands separators
                            unity_covered_formatted=$(printf "%'d" "${unity_covered}" 2>/dev/null || echo "${unity_covered}")
                            unity_total_formatted=$(printf "%'d" "${unity_total}" 2>/dev/null || echo "${unity_total}")
                            unity_covered_files=${unity_covered_files:-0}
                            unity_instrumented=${unity_instrumented:-0}
                            echo "| Unity Tests | ${unity_covered_files} | ${unity_instrumented} | ${unity_covered_formatted} | ${unity_total_formatted} | ${unity_coverage_pct}% | ${unity_timestamp} |"
                        else
                            echo "| Unity Tests | 0 | 0 | 0 | 0 | 0.000% |  0 |"
                        fi
                    else
                        echo "| Unity Tests | 0 | 0 | 0 | 0 | 0.000% | 0 |"
                    fi
                    
                    # Blackbox coverage
                    if [[ -f "${blackbox_coverage_detailed}" ]]; then
                        local blackbox_timestamp blackbox_coverage_pct blackbox_covered blackbox_total blackbox_instrumented blackbox_covered_files
                        IFS=',' read -r blackbox_timestamp blackbox_coverage_pct blackbox_covered blackbox_total blackbox_instrumented blackbox_covered_files < "${blackbox_coverage_detailed}" 2>/dev/null
                        if [[ -n "${blackbox_total}" ]] && [[ "${blackbox_total}" -gt 0 ]]; then
                            # Add thousands separators
                            blackbox_covered_formatted=$(printf "%'d" "${blackbox_covered}" 2>/dev/null || echo "${blackbox_covered}")
                            blackbox_total_formatted=$(printf "%'d" "${blackbox_total}" 2>/dev/null || echo "${blackbox_total}")
                            blackbox_covered_files=${blackbox_covered_files:-0}
                            blackbox_instrumented=${blackbox_instrumented:-0}
                            echo "| Blackbox Tests | ${blackbox_covered_files} | ${blackbox_instrumented} | ${blackbox_covered_formatted} | ${blackbox_total_formatted} | ${blackbox_coverage_pct}% | ${blackbox_timestamp} |"
                        else
                            echo "| Blackbox Tests | 0 | 0 | 0 | 0 | 0.000% | 0 |"
                        fi
                    else
                        echo "| Blackbox Tests | 0 | 0 | 0 | 0 | 0.000% | 0 |"
                    fi
                    
                    # Combined coverage
                    if [[ -f "${combined_coverage_detailed}" ]]; then
                        local combined_timestamp combined_coverage_pct combined_covered combined_total combined_instrumented combined_covered_files
                        IFS=',' read -r combined_timestamp combined_coverage_pct combined_covered combined_total combined_instrumented combined_covered_files < "${combined_coverage_detailed}" 2>/dev/null
                        if [[ -n "${combined_total}" ]] && [[ "${combined_total}" -gt 0 ]]; then
                            # Add thousands separators
                            combined_covered_formatted=$(printf "%'d" "${combined_covered}" 2>/dev/null || echo "${combined_covered}")
                            combined_total_formatted=$(printf "%'d" "${combined_total}" 2>/dev/null || echo "${combined_total}")
                            combined_covered_files=${combined_covered_files:-0}
                            combined_instrumented=${combined_instrumented:-0}
                            echo "| Combined Tests | ${combined_covered_files} | ${combined_instrumented} | ${combined_covered_formatted} | ${combined_total_formatted} | ${combined_coverage_pct}% | ${combined_timestamp} |"
                        else
                            echo "| Combined Tests | 0 | 0 | 0 | 0 | 0.000% | 0 |"
                        fi
                    else
                        echo "| Combined Tests | 0 | 0 | 0 | 0 | 0.000% | 0 |"
                    fi
                else
                    echo "| Test Type | Files Cover | Files Instr | Lines Cover | Lines Instr | Coverage | Timestamp |"
                    echo "| --------- | ----------- | ----------- | ----------- | ----------- | -------- | --------- |"
                    echo "| Unity Tests | 0 | 0 | 0 | 0 | 0.000% | 0 |"
                    echo "| Blackbox Tests | 0 | 0 | 0 | 0 | 0.000% | 0 |"
                fi
                echo ""
            } >> "${temp_readme}"
            continue
        elif [[ "${line}" == "### Individual Test Results" ]]; then
            in_individual_results=true
            {
                echo "${line}"
                echo ""
                echo "| Status | Time | Test | Tests | Pass | Fail | Summary |"
                echo "| ------ | ---- | ---- | ----- | ---- | ---- | ------- |"
                
                # Add individual test results
                for i in "${!TEST_NUMBERS[@]}"; do
                    local status="✅"
                    local summary="Test completed without errors"
                    if [[ "${TEST_FAILED[${i}]}" -gt 0 ]]; then
                        status="❌"
                        summary="Test failed with errors"
                    fi
                    local time_formatted
                    time_formatted=$(format_time_duration "${TEST_ELAPSED[${i}]}")
                    echo "| ${status} | ${time_formatted} | ${TEST_NUMBERS[${i}]}_$(echo "${TEST_NAMES[${i}]}" | tr ' ' '_' | tr '[:upper:]' '[:lower:]') | ${TEST_SUBTESTS[${i}]} | ${TEST_PASSED[${i}]} | ${TEST_FAILED[${i}]} | ${summary} |" || true
                done
                echo ""
            } >> "${temp_readme}"
            continue
        elif [[ "${line}" == "## Repository Information" ]]; then
            in_repo_info=true
            in_test_results=false
            in_individual_results=false
            {
                echo "${line}"
                echo ""
                echo "Generated via cloc: ${timestamp}"
                echo ""
                
                # Use shared cloc library function, ensuring we're in the project root directory
                pushd "${PROJECT_DIR}" > /dev/null || return 1
                generate_cloc_for_readme "." ".lintignore"
                popd > /dev/null || return 1
            } >> "${temp_readme}"
            continue
        elif [[ "${in_test_results}" == true || "${in_individual_results}" == true || "${in_repo_info}" == true ]]; then
            # Skip existing content in these sections
            if [[ "${line}" == "## "* ]]; then
                # New section started, stop skipping
                in_test_results=false
                in_individual_results=false
                in_repo_info=false
                echo "${line}" >> "${temp_readme}"
            fi
            continue
        else
            echo "${line}" >> "${temp_readme}"
        fi
    done < "${readme_file}"
    
    # Replace original README with updated version
    if mv "${temp_readme}" "${readme_file}"; then
        # echo "Updated README.md with test results"
        :
    else
        echo "Error: Failed to update README.md"
        rm -f "${temp_readme}"
        return 1
    fi
}

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
    local test_number
    local test_name
    
    # Extract test number from script filename
    
    test_number=$(basename "${test_script}" .sh | sed 's/test_//' | sed 's/_.*//' || true)
    test_name=""
    
    # Handle skip mode
    if [[ "${SKIP_TESTS}" = true ]]; then
        # Extract test name from script filename (same logic as normal mode)
        test_name=$(basename "${test_script}" .sh | sed 's/test_[0-9]*_//' | tr '_' ' ' | sed 's/\b\w/\U&/g' || true)
        # Store skipped test results (don't print "Would run" message)
        TEST_NUMBERS+=("${test_number}")
        TEST_NAMES+=("${test_name}")
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
    
    # Don't calculate elapsed time here - we'll get it from the subtest result file
    # This eliminates the timing discrepancy by using a single source of truth
    local elapsed_formatted="0.000"
    
    # Extract test results from subtest file or use defaults
    local total_subtests=1 passed_subtests=0 elapsed_formatted="0.000"
    local test_name_for_file
    test_name_for_file=$(basename "${test_script}" .sh | sed 's/test_[0-9]*_//' | tr '_' ' ' | sed 's/\b\w/\U&/g' || true)
    local latest_subtest_file
    latest_subtest_file=$(find "${RESULTS_DIR}" -name "subtest_${test_number}_*.txt" -type f 2>/dev/null | sort -r | head -1 || true)
    
    if [[ -n "${latest_subtest_file}" ]] && [[ -f "${latest_subtest_file}" ]]; then
        IFS=',' read -r total_subtests passed_subtests test_name file_elapsed_time < "${latest_subtest_file}" 2>/dev/null || {
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
    TEST_NAMES+=("${test_name}")
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
    latest_subtest_file=$(find "${RESULTS_DIR}" -name "subtest_${test_number}_*.txt" -type f 2>/dev/null | sort -r | head -1 || true)
    
    if [[ -n "${latest_subtest_file}" ]] && [[ -f "${latest_subtest_file}" ]]; then
        # Read subtest results, test name, and elapsed time from the file
        IFS=',' read -r total_subtests passed_subtests test_name file_elapsed_time < "${latest_subtest_file}" 2>/dev/null || {
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
    echo "${test_number}|${test_name}|${total_subtests}|${passed_subtests}|${failed_subtests}|${elapsed_formatted}|${exit_code}" > "${temp_result_file}"
    
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
            temp_result_file=$(mktemp)
            temp_output_file=$(mktemp)
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
            # Clean up output file
            rm -f "${temp_output_file}"
        done
        
        # Collect results from background tests
        for i in "${!remaining_tests[@]}"; do
            local temp_result_file="${temp_result_files[${i}]}"
            if [[ -f "${temp_result_file}" ]] && [[ -s "${temp_result_file}" ]]; then
                # Read results from temporary file
                IFS='|' read -r test_number test_name total_subtests passed_subtests failed_subtests elapsed_formatted exit_code < "${temp_result_file}"
                
                # Store results in global arrays
                TEST_NUMBERS+=("${test_number}")
                TEST_NAMES+=("${test_name}")
                TEST_SUBTESTS+=("${total_subtests}")
                TEST_PASSED+=("${passed_subtests}")
                TEST_FAILED+=("${failed_subtests}")
                TEST_ELAPSED+=("${elapsed_formatted}")
            fi
            
            # Clean up result file
            rm -f "${temp_result_file}"
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

# Get coverage percentages for display
UNITY_COVERAGE=$(get_unity_coverage)
BLACKBOX_COVERAGE=$(get_blackbox_coverage)
COMBINED_COVERAGE=$(get_combined_coverage)

# Run coverage table before displaying test results
coverage_table_script="${LIB_DIR}/coverage_table.sh"
if [[ -x "${coverage_table_script}" ]]; then
    # Save coverage table output to file and display to console using tee
    coverage_table_file="${RESULTS_DIR}/coverage_table.txt"
    env -i bash "${coverage_table_script}" 2>/dev/null | tee "${coverage_table_file}" || true
    
    # Launch background process to generate COVERAGE.svg from saved file
    oh_script="${LIB_DIR}/Oh.sh"
    coverage_svg_path="${PROJECT_DIR}/COVERAGE.svg"
    if [[ -x "${oh_script}" ]] && [[ -f "${coverage_table_file}" ]]; then
        # Delete existing file before generating new one
        rm -f "${coverage_svg_path}"
        # Generate SVG from saved coverage table file in background
        ("${oh_script}" -i "${coverage_table_file}" -o "${coverage_svg_path}" 2>/dev/null) &
    fi
fi

# Calculate total elapsed time
END_TIME=$(date +%s.%N 2>/dev/null || date +%s)
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
        if command -v bc >/dev/null 2>&1; then
            TOTAL_RUNNING_TIME=$(echo "${TOTAL_RUNNING_TIME} + ${elapsed_time}" | bc)
        else
            TOTAL_RUNNING_TIME=$(awk "BEGIN {print (${TOTAL_RUNNING_TIME} + ${elapsed_time}); exit}")
        fi
    fi
done

# Format both times as HH:MM:SS.ZZZ
TOTAL_ELAPSED_FORMATTED=$(format_time_duration "${TOTAL_ELAPSED}")
TOTAL_RUNNING_TIME_FORMATTED=$(format_time_duration "${TOTAL_RUNNING_TIME}")

# Generate display timestamp for footer
display_timestamp=$(date '+%Y-%m-%d %H:%M:%S %Z')


# Create layout JSON string
layout_json_content='{
    "title": "Test Suite Results {NC}{RED}———{RESET}{BOLD}{CYAN} Unity {WHITE}'"${UNITY_COVERAGE}"'% {RESET}{RED}———{RESET}{BOLD}{CYAN} Blackbox {WHITE}'"${BLACKBOX_COVERAGE}"'% {RESET}{RED}———{RESET}{BOLD}{CYAN} Combined {WHITE}'"${COMBINED_COVERAGE}"'%{RESET}",
    "footer": "{CYAN}Cumulative {WHITE}'"${TOTAL_RUNNING_TIME_FORMATTED}"'{RED} ——— {RESET}{CYAN}Elapsed {WHITE}'"${TOTAL_ELAPSED_FORMATTED}"'{RED} ——— {CYAN}Timestamp {WHITE}'"${display_timestamp}"'{RESET}",
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
            "header": "Test Name",
            "key": "test_name",
            "datatype": "text",
            "width": 50,
            "justification": "left"
        },
        {
            "header": "Tests",
            "key": "tests",
            "datatype": "int",
            "width": 8,
            "justification": "right",
            "summary": "sum"
        },
        {
            "header": "Pass",
            "key": "pass",
            "datatype": "int",
            "width": 8,
            "justification": "right",
            "summary": "sum"
        },
        {
            "header": "Fail",
            "key": "fail",
            "datatype": "int",
            "width": 8,
            "justification": "right",
            "summary": "sum"
        },
        {
            "header": "Duration",
            "key": "elapsed",
            "datatype": "float",
            "width": 11,
            "justification": "right",
            "summary": "sum"
        }
    ]
}'

# Create data JSON string
data_json_content="["
for i in "${!TEST_NUMBERS[@]}"; do
    test_id="${TEST_NUMBERS[${i}]}-000"
    # Calculate group (tens digit) from test number
    group=$((${TEST_NUMBERS[${i}]} / 10))
    
    if [[ "${i}" -gt 0 ]]; then
        data_json_content+=","
    fi
    data_json_content+='
    {
        "group": '"${group}"',
        "test_id": "'"${test_id}"'",
        "test_name": "'"${TEST_NAMES[${i}]}"'",
        "tests": '"${TEST_SUBTESTS[${i}]}"',
        "pass": '"${TEST_PASSED[${i}]}"',
        "fail": '"${TEST_FAILED[${i}]}"',
        "elapsed": '"${TEST_ELAPSED[${i}]}"'
    }'
done
data_json_content+="]"

# Use tables executable to render the summary table
temp_dir=$(mktemp -d 2>/dev/null) || { echo "Error: Failed to create temporary directory"; exit 1; }
layout_json="${temp_dir}/summary_layout.json"
data_json="${temp_dir}/summary_data.json"

echo "${layout_json_content}" > "${layout_json}"
echo "${data_json_content}" > "${data_json}"

tables_exe="${LIB_DIR}/tables"
if [[ -x "${tables_exe}" ]]; then
    # Save test results table output to file and display to console using tee
    results_table_file="${RESULTS_DIR}/results_table.txt"
    "${tables_exe}" "${layout_json}" "${data_json}" 2>/dev/null | tee "${results_table_file}" || true
    
    # Launch background process to generate COMPLETE.svg from saved file
    oh_script="${LIB_DIR}/Oh.sh"
    results_svg_path="${PROJECT_DIR}/COMPLETE.svg"
    if [[ -x "${oh_script}" ]] && [[ -f "${results_table_file}" ]]; then
        # Delete existing file before generating new one
        rm -f "${results_svg_path}"
        # Generate SVG from saved results table file in background
        ("${oh_script}" -i "${results_table_file}" -o "${results_svg_path}" 2>/dev/null) &
    fi
fi

rm -rf "${temp_dir}" 2>/dev/null

# Update README.md with test results (only if not in skip mode)
if [[ "${SKIP_TESTS}" = false ]]; then
    update_readme_with_results
fi

exit "${OVERALL_EXIT_CODE}"
