#!/bin/bash

# Framework Library
# Provides test lifecycle management and result tracking functions

# LIBRARY FUNCTIONS
# format_time_duration()
# format_file_size()
# get_elapsed_time()
# get_elapsed_time_decimal()
# set_test_number()
# next_subtest()
# reset_subtest_counter()
# start_test_timer()
# get_test_prefix()
# record_test_result()
# setup_orchestration_environment()
# setup_test_environment()
# evaluate_test_result_silent()
# update_readme_with_results()

# CHANGELOG
# 2.8.0 - 2025-08-08 - Cleaned out some mktemp calls
# 2.7.0 - 2025-08-07 - Support for commas in test names (ie, thousands separators)
# 2.6.0 - 2025-08-06 - Improvements to logging file handling, common TAB file naming
# 2.5.1 - 2025-08-03 - Removed extraneous command -v calls
# 2.5.0 - 2025-08-02 - Removed old functions, added some from log_output
# 2.4.0 - 2025-08-02 - Optimizations for formatting functions
# 2,3,1 - 2025-07-31 - Added LINT_EXCLUDES to setup_test_environment
# 2.3.0 - 2025-07-31 - Added update_readme_with_results() from Test 00
# 2.2.0 - 2025-07-20 - Added guard
# 2.1.0 - 2025-07-07 - Restructured how test elapsed times are stored and accessed
# 2.0.0 - 2025-07-02 - Updated to integrate with numbered output system
# 1.0.0 - 2025-07-02 - Initial creation from support_utils.sh migration

# Guard clause to prevent multiple sourcing
[[ -n "${FRAMEWORK_GUARD}" ]] && return 0
export FRAMEWORK_GUARD="true"

# Library metadata
FRAMEWORK_NAME="Framework Library"
FRAMEWORK_VERSION="2.8.0"
export FRAMEWORK_NAME FRAMEWORK_VERSION

# Set the number of CPU cores for parallel processing - why not oversubscribe?
CORES_ACTUAL=$(nproc)
CORES=$((CORES_ACTUAL * 2))
export CORES

# Function to format seconds as HH:MM:SS.ZZZ
format_time_duration() {
    local seconds="$1"
    local hours minutes secs milliseconds
    
    # Handle seconds that start with a decimal point (e.g., ".492")
    if [[ "${seconds}" =~ ^\. ]]; then
        seconds="0${seconds}"
    fi
    
    # Handle decimal seconds using parameter expansion
    if [[ "${seconds}" == *.* ]]; then
        secs="${seconds%.*}"
        milliseconds="${seconds#*.}"
        # Pad or truncate milliseconds to 3 digits
        milliseconds="${milliseconds}000"
        milliseconds="${milliseconds:0:3}"
    else
        secs="${seconds}"
        milliseconds="000"
    fi
    
    hours=$((secs / 3600))
    minutes=$(((secs % 3600) / 60))
    secs=$((secs % 60))
    
    printf "%02d:%02d:%02d.%s" "${hours}" "${minutes}" "${secs}" "${milliseconds}"
}

# Function to format file size with thousands separators
format_file_size() {
    local file_size="$1"
    printf "%'d" "${file_size}" 2>/dev/null || echo "${file_size}"
}

# Function to calculate elapsed time in SSS.ZZZ format for console output
get_elapsed_time() {
    if [[ -n "${TEST_START_TIME}" ]]; then
        local end_time
        end_time=$(/usr/bin/date +%s.%3N) 
        local end_secs=${end_time%.*}
        local end_ms=${end_time#*.}
        local start_secs=${TEST_START_TIME%.*}
        local start_ms=${TEST_START_TIME#*.}
        end_ms=$((10#${end_ms}))
        start_ms=$((10#${start_ms}))
        local end_total_ms=$((end_secs * 1000 + end_ms))
        local start_total_ms=$((start_secs * 1000 + start_ms))
        local elapsed_ms=$((end_total_ms - start_total_ms))
        local seconds=$((elapsed_ms / 1000))
        local milliseconds=$((elapsed_ms % 1000))
        printf "%03d.%03d" "${seconds}" "${milliseconds}"
    else
        echo "000.000"
    fi
}

# Function to calculate elapsed time in decimal format (X.XXX) for table output
get_elapsed_time_decimal() {
    if [[ -n "${TEST_START_TIME}" ]]; then
        local end_time
        end_time=$(/usr/bin/date +%s.%3N) 
        local end_secs=${end_time%.*}
        local end_ms=${end_time#*.}
        local start_secs=${TEST_START_TIME%.*}
        local start_ms=${TEST_START_TIME#*.}
        end_ms=$((10#${end_ms}))
        start_ms=$((10#${start_ms}))
        local end_total_ms=$((end_secs * 1000 + end_ms))
        local start_total_ms=$((start_secs * 1000 + start_ms))
        local elapsed_ms=$((end_total_ms - start_total_ms))
        local seconds=$((elapsed_ms / 1000))
        local milliseconds=$((elapsed_ms % 1000))
        printf "%d.%03d" "${seconds}" "${milliseconds}"
    else
        echo "0.000"
    fi
}

# Function to set the current test number (e.g., "10" for test_10_compilation.sh)
set_test_number() {
    CURRENT_TEST_NUMBER="$1"
    CURRENT_SUBTEST_NUMBER=""
}

# Function to increment and get next subtest number
next_subtest() {
    ((SUBTEST_COUNTER++))
    CURRENT_SUBTEST_NUMBER=$(printf "%03d" "${SUBTEST_COUNTER}")
}

# Function to reset subtest counter
reset_subtest_counter() {
    SUBTEST_COUNTER=0
    CURRENT_SUBTEST_NUMBER="000"
}

# Function to start test timing
start_test_timer() {
    TEST_START_TIME=$(/usr/bin/date +%s.%3N)
    TEST_PASSED_COUNT=0
    TEST_FAILED_COUNT=0
}

# Function to get the current test prefix for output
get_test_prefix() {
    if [[ -n "${CURRENT_SUBTEST_NUMBER}" ]]; then
        echo "${CURRENT_TEST_NUMBER}-${CURRENT_SUBTEST_NUMBER}"
    elif [[ -n "${CURRENT_TEST_NUMBER}" ]]; then
        echo "${CURRENT_TEST_NUMBER}"
    else
        echo "XX"
    fi
}

# Function to record test result for statistics
record_test_result() {
    local status=$1
    if [[ "${status}" -eq 0 ]]; then
        ((TEST_PASSED_COUNT++))
    else
        ((TEST_FAILED_COUNT++))
    fi
}

setup_orchestration_environment() {

    # Get start time
    START_TIME=$(/usr/bin/date +%s.%N 2>/dev/null)

    # Naturally we're orchestrating
    ORCHESTRATION=true

    # Starting point
    TIMESTAMP=$(/usr/bin/date +%Y%m%d_%H%M%S)
    TIMESTAMP_DISPLAY=$(/usr/bin/date '+%Y-%b-%d (%a) %H:%M:%S %Z' 2>/dev/null) # 2025-Jul-30 (Wed) 12:49:03 PDT  eg: long display times

    # All tests that run hydrogen run with a config that starts with hydrogen_test so we can
    # ensure nothing else is running by killing those processes at the start and at the end
    pkill -9 -f hydrogen_test_

    # Array for collecting output messages (for performance optimization and progressive feedback)
    # Output is cached and dumped each time a new TEST starts, providing progressive feedback
    declare -a OUTPUT_COLLECTION=()
    COLLECT_OUTPUT_MODE=true
    
    # Global folder variables
    PROJECT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && cd ../.. && pwd )"
    pushd "${PROJECT_DIR}" >/dev/null 2>&1 || return
    
    CMAKE_DIR="${PROJECT_DIR}/cmake"
    SCRIPT_DIR="${PROJECT_DIR}/tests"
    LIB_DIR="${SCRIPT_DIR}/lib"
    BUILD_DIR="${PROJECT_DIR}/build"
    TESTS_DIR="${BUILD_DIR}/tests"
    RESULTS_DIR="${TESTS_DIR}/results"
    DIAGS_DIR="${TESTS_DIR}/diagnostics"
    LOGS_DIR="${TESTS_DIR}/logs"
    CONFIG_DIR="${SCRIPT_DIR}/configs"

    # shellcheck disable=SC2153,SC2154 # TEST_NUMBER defined by caller
    DIAG_TEST_DIR="${DIAGS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}"

    mkdir -p "${BUILD_DIR}" "${TESTS_DIR}" "${RESULTS_DIR}" "${LOGS_DIR}" "${DIAGS_DIR}" "${DIAG_TEST_DIR}" 
    
    # Common utilities
    CLOC_EXTERNAL="/usr/bin/cloc"
    TABLES_EXTERNAL="${LIB_DIR}/tables"
    OH_EXTERNAL="${LIB_DIR}/Oh.sh"
    COVERAGE_EXTERNAL="${LIB_DIR}/coverage_table.sh"
    SITEMAP_EXTERNAL="${LIB_DIR}/github-sitemap.sh"

    # Need to load log_output a little earlier than the others
    # shellcheck source=tests/lib/log_output.sh # Resolve path statically
    [[ -n "${LOG_OUTPUT_GUARD}" ]] || source "${LIB_DIR}/log_output.sh"
    
    # Reset test counter to zero
    # shellcheck disable=SC2154,SC2153 # TEST_NUMBER defined in caller (Test 00)
    set_test_number "${TEST_NUMBER}"
    reset_subtest_counter

    # Print beautiful test header
    # shellcheck disable=SC2154,SC2153 # TEST_NAME, TEST_ABBR, TEST_NUMBER, TEST_VERSION defined externally in caller
    print_test_suite_header "${TEST_NAME}" "${TEST_ABBR}" "${TEST_NUMBER}" "${TEST_VERSION}"

    print_subtest "Loading Test Suite Libraries"
    # Print framework and log output versions as they are already sourced
    [[ -n "${ORCHESTRATION}" ]] || print_message "${FRAMEWORK_NAME} ${FRAMEWORK_VERSION}" "info"
    [[ -n "${ORCHESTRATION}" ]] || print_message "${LOG_OUTPUT_NAME} ${LOG_OUTPUT_VERSION}" "info"
    # shellcheck source=tests/lib/lifecycle.sh # Resolve path statically
    [[ -n "${LIFECYCLE_GUARD}" ]] || source "${LIB_DIR}/lifecycle.sh"
    # shellcheck source=tests/lib/file_utils.sh # Resolve path statically
    [[ -n "${FILE_UTILS_GUARD}" ]] || source "${LIB_DIR}/file_utils.sh"
    # shellcheck source=tests/lib/env_utils.sh # Resolve path statically
    [[ -n "${ENV_UTILS_GUARD}" ]] || source "${LIB_DIR}/env_utils.sh"
    # shellcheck source=tests/lib/network_utils.sh # Resolve path statically
    [[ -n "${NETWORK_UTILS_GUARD}" ]] || source "${LIB_DIR}/network_utils.sh"
    # shellcheck source=tests/lib/coverage.sh # Resolve path statically
    [[ -n "${COVERAGE_GUARD}" ]] || source "${LIB_DIR}/coverage.sh"
    # shellcheck source=tests/lib/cloc.sh # Resolve path statically
    [[ -n "${CLOC_GUARD}" ]] || source "${LIB_DIR}/cloc.sh"
    print_result 0 "Test Suite libraries initialized"

    next_subtest
    print_subtest "Checking for tmpfs Build setup"
    if [[ -d "build" ]]; then
        print_message "Build directory exists, checking mount status..."
    
        # Check if build is already a tmpfs mount
        if mountpoint -q build 2>/dev/null; then
            print_message "Build directory already mounted as tmpfs, emptying non-cmake-build contents..."
            if rm -rf build/coverage build/debug build/perf build/regular build/release build/tests build/unity build/valgrind build/*marker* build/gcov* 2>/dev/null; then
                print_result 0 "Build directory (tmpfs) emptied and ready for use"
            else
                print_result 1 "Failed to empty tmpfs build directory"
                EXIT_CODE=1
            fi
        else
            # Empty the regular directory and mount as tmpfs
            print_message "Emptying regular build directory..."
            print_command "rm -rf build/*"
            if rm -rf build/* 2>/dev/null; then
                print_message "Successfully emptied build directory"
                
                # Mount as tmpfs
                print_message "Mounting 'build' as tmpfs with 1GB size..."
                print_command "sudo mount -t tmpfs -o size=1G tmpfs build"
                if sudo mount -t tmpfs -o size=1G tmpfs build 2>/dev/null; then
                    print_result 0 "Build directory mounted as tmpfs (1GB) for faster I/O"
                    print_message "Warning: tmpfs is volatile; artifacts will be lost on unmount/reboot"
                else
                    print_result 0 "Build directory ready (tmpfs mount failed, using regular filesystem)"
                    print_message "Continuing with regular filesystem build directory"
                fi
            else
                print_result 1 "Failed to empty 'build' directory"
                EXIT_CODE=1
            fi
        fi
    else
        # Create the build directory and mount as tmpfs
        print_message "Creating 'build' directory..."
        print_command "mkdir build"
        if mkdir build 2>/dev/null; then
            print_message "Successfully created build directory"
            
            # Mount as tmpfs
            print_message "Mounting 'build' as tmpfs with 1GB size..."
            print_command "sudo mount -t tmpfs -o size=1G tmpfs build"
            if sudo mount -t tmpfs -o size=1G tmpfs build 2>/dev/null; then
                print_result 0 "Build directory created and mounted as tmpfs (1GB) for faster I/O"
                print_message "Warning: tmpfs is volatile; artifacts will be lost on unmount/reboot"
            else
                print_result 0 "Build directory created (tmpfs mount failed, using regular filesystem)"
                print_message "Continuing with regular filesystem build directory"
            fi
        else
            print_result 1 "Failed to create 'build' directory"
            EXIT_CODE=1
        fi
    fi

    # Common files
    # shellcheck disable=SC2154,SC2153 # TEST_NUMBER defined externally in framework.sh
    RESULT_LOG="${RESULTS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}.log"
    LOG_FILE="${LOGS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}.log"
    DIAG_FILE="${DIAGS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}.log"

    TAB_LAYOUT_HEADER="${DIAG_TEST_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}_table_layout_header.json"
    TAB_DATA_HEADER="${DIAG_TEST_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}_table_data_header.json"
    TAB_LAYOUT_FOOTER="${DIAG_TEST_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}_table_layout_footer.json"
    TAB_DATA_FOOTER="${DIAG_TEST_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}_table_data_footer.json"
    LOG_PREFIX="${DIAG_TEST_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}"

    dump_collected_output
    clear_collected_output
}

# Function to set up the standard test environment with numbering
setup_test_environment() {

    if [[ -z "${ORCHESTRATION}" ]]; then

        # All tests that run hydrogen run with a config that starts with hydrogen_test so we can
        # ensure nothing else is running by killing those processes at the start and at the end
        # We only do this for single tests if it isn't running under orchestration
        pkill -9 -f hydrogen_test_

        # Starting point
        TIMESTAMP=$(/usr/bin/date +%Y%m%d_%H%M%S)

        # Global folder variables
        PROJECT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && cd ../.. && pwd )"
        pushd "${PROJECT_DIR}" >/dev/null 2>&1 || return

        CMAKE_DIR="${PROJECT_DIR}/cmake"
        SCRIPT_DIR="${PROJECT_DIR}/tests"
        LIB_DIR="${SCRIPT_DIR}/lib"
        BUILD_DIR="${PROJECT_DIR}/build"
        TESTS_DIR="${BUILD_DIR}/tests"
        RESULTS_DIR="${TESTS_DIR}/results"
        DIAGS_DIR="${TESTS_DIR}/diagnostics"
        LOGS_DIR="${TESTS_DIR}/logs"
        CONFIG_DIR="${SCRIPT_DIR}/configs"

        # Array for collecting output messages (for performance optimization and progressive feedback)
        # Output is cached and dumped each time a new TEST starts, providing progressive feedback
        declare -a OUTPUT_COLLECTION=()
        COLLECT_OUTPUT_MODE=true

        # Common utilities
        CLOC_EXTERNAL="/usr/bin/cloc"
        TABLES_EXTERNAL="${LIB_DIR}/tables"
        OH_EXTERNAL="${LIB_DIR}/Oh.sh"
        COVERAGE_EXTERNAL="${LIB_DIR}/coverage_table.sh"
        SITEMAP_EXTERNAL="${LIB_DIR}/github-sitemap.sh"

    else
      pushd "${PROJECT_DIR}" >/dev/null 2>&1 || return
    fi

    # Common test configuration
    EXIT_CODE=0
    PASS_COUNT=0

    # Setup build folders
    # shellcheck disable=SC2153,SC2154 # TEST_NUMBER defined by caller
    DIAG_TEST_DIR="${DIAGS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}"
    mkdir -p "${BUILD_DIR}" "${TESTS_DIR}" "${RESULTS_DIR}" "${DIAGS_DIR}" "${LOGS_DIR}" "${DIAG_TEST_DIR}"

    # Common files
    # shellcheck disable=SC2154,SC2153 # TEST_NUMBER defined externally in framework.sh
    RESULT_LOG="${RESULTS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}.log"
    LOG_FILE="${LOGS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}.log"
    DIAG_FILE="${DIAGS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}.log"

    # Extra handling
    TAB_LAYOUT_HEADER="${DIAG_TEST_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}_table_layout_header.json"
    TAB_DATA_HEADER="${DIAG_TEST_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}_table_data_header.json"
    TAB_LAYOUT_FOOTER="${DIAG_TEST_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}_table_layout_footer.json"
    TAB_DATA_FOOTER="${DIAG_TEST_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}_table_data_footer.json"
    LOG_PREFIX="${DIAG_TEST_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}"

    # Need to load log_output a little earlier than the others
    # shellcheck source=tests/lib/log_output.sh # Resolve path statically
    [[ -n "${LOG_OUTPUT_GUARD}" ]] || source "${LIB_DIR}/log_output.sh"
    
    # Reset test counter to zero
    set_test_number "${TEST_NUMBER}"
    reset_subtest_counter

    # Print beautiful test header
    # shellcheck disable=SC2154,SC2153 # TEST_NAME, TEST_ABBR, TEST_NUMBER, TEST_VERSION defined externally in caller
    print_test_header "${TEST_NAME}" "${TEST_ABBR}" "${TEST_NUMBER}" "${TEST_VERSION}"
    
    if [[ -z "${ORCHESTRATION}" ]]; then
         # Print framework and log output versions as they are already sourced
        print_message "${FRAMEWORK_NAME} ${FRAMEWORK_VERSION}" "info"
        print_message "${LOG_OUTPUT_NAME} ${LOG_OUTPUT_VERSION}" "info"
        # shellcheck source=tests/lib/lifecycle.sh # Resolve path statically
        [[ -n "${LIFECYCLE_GUARD}" ]] || source "${LIB_DIR}/lifecycle.sh"
        # shellcheck source=tests/lib/file_utils.sh # Resolve path statically
        [[ -n "${FILE_UTILS_GUARD}" ]] || source "${LIB_DIR}/file_utils.sh"
        # shellcheck source=tests/lib/env_utils.sh # Resolve path statically
        [[ -n "${ENV_UTILS_GUARD}" ]] || source "${LIB_DIR}/env_utils.sh"
        # shellcheck source=tests/lib/network_utils.sh # Resolve path statically
        [[ -n "${NETWORK_UTILS_GUARD}" ]] || source "${LIB_DIR}/network_utils.sh"
        # shellcheck source=tests/lib/coverage.sh # Resolve path statically
        [[ -n "${COVERAGE_GUARD}" ]] || source "${LIB_DIR}/coverage.sh"
        # shellcheck source=tests/lib/cloc.sh # Resolve path statically
        [[ -n "${CLOC_GUARD}" ]] || source "${LIB_DIR}/cloc.sh"

    fi

    dump_collected_output
    clear_collected_output
}

# Function to evaluate test results silently (no output, just update counts)
evaluate_test_result_silent() {
    local test_name="$1"
    local failed_checks="$2"
    local pass_count_var="$3"
    local exit_code_var="$4"
    
    if [[ "${failed_checks}" -eq 0 ]]; then
        eval "${pass_count_var}=\$((\${${pass_count_var}} + 1))"
    else
        eval "${exit_code_var}=1"
    fi
    # Intentionally not calling print_result to avoid duplicate PASS messages
}

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
    
    # Create temporary file for new README content
    local temp_readme
    temp_readme="${RESULTS_DIR}/README_${TIMESTAMP}.md"
    
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
                echo "Generated on: ${TIMESTAMP_DISPLAY}"
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
                local unity_coverage_detailed="${RESULTS_DIR}/coverage_unity.txt.detailed"
                local blackbox_coverage_detailed="${RESULTS_DIR}/coverage_blackbox.txt.detailed"
                local combined_coverage_detailed="${RESULTS_DIR}/coverage_combined.txt.detailed"
                
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
                        fi
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
                        fi
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
                        fi
                    fi
                fi
                echo ""
            } >> "${temp_readme}"
            continue
        elif [[ "${line}" == "### Individual Test Results" ]]; then
            in_individual_results=true
            {
                echo "${line}"
                echo ""
                echo "| Status | Time | Test | Test Name | Tests | Pass | Fail |"
                echo "| ------ | ---- | -- | ---- | ----- | ---- | ---- |"
                
                # Add individual test results
                for i in "${!TEST_NUMBERS[@]}"; do
                    local test_status="✅"
                    if [[ "${TEST_FAILED[${i}]}" -gt 0 ]]; then
                        test_status="❌"
                    fi
                    local test_time_formatted
                    test_time_formatted=$(format_time_duration "${TEST_ELAPSED[${i}]}")
                    test_number_formatted=$(printf "%s-%s" "${TEST_NUMBERS[${i}]}" "${TEST_ABBREVS[${i}]}")
                    test_name_formatted="${TEST_NAMES[${i}]}"
                    test_name_formatted=${test_name_formatted//\{COMMA\}/,}
                    test_name_formatted=${test_name_formatted//\{BLUE\}/}
                    test_name_formatted=${test_name_formatted//\{RESET\}/}
                    echo "| ${test_status} | ${test_time_formatted} | ${test_number_formatted} | ${test_name_formatted} | ${TEST_SUBTESTS[${i}]} | ${TEST_PASSED[${i}]} | ${TEST_FAILED[${i}]} |" || true
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
                echo "Generated via cloc: ${TIMESTAMP_DISPLAY}"
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
    if ! mv "${temp_readme}" "${readme_file}"; then
        echo "Error: Failed to update README.md"
        return 1
    fi
}