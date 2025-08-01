#!/bin/bash

# Framework Library
# Provides test lifecycle management and result tracking functions

# LIBRARY FUNCTIONS
# format_time_duration()
# start_test()
# start_subtest()
# setup_orchestration_environment()
# setup_test_environment()
# navigate_to_project_root()
# export_test_results()
# run_check()
# evaluate_test_result()
# evaluate_test_result_silent()
# generate_html_report()
# start_test_suite()
# end_test_suite()
# increment_passed_tests()
# increment_failed_tests()
# get_passed_tests()
# get_failed_tests()
# get_total_tests()
# get_exit_code()
# print_subtest_header()
# skip_remaining_subtests()
# update_readme_with_results()

# CHANGELOG
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
FRAMEWORK_VERSION="2.3.1"
export FRAMEWORK_NAME FRAMEWORK_VERSION

# Set the number of CPU cores for parallel processing
CORES=$(nproc)
CORESPLUS=$((CORES * 2))
export CORES CORESPLUS

# Function to format seconds as HH:MM:SS.ZZZ
format_time_duration() {
    local seconds="$1"
    local hours minutes secs milliseconds
    
    # Handle seconds that start with a decimal point (e.g., ".492")
    if [[ "${seconds}" =~ ^\. ]]; then
        seconds="0${seconds}"
    fi
    
    # Handle decimal seconds
    if [[ "${seconds}" =~ ^([0-9]+)\.([0-9]+)$ ]]; then
        secs="${BASH_REMATCH[1]}"
        milliseconds="${BASH_REMATCH[2]}"
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

# Function to start a subtest with proper header and numbering
start_subtest() {
    local subtest_name="$1"
    local subtest_number="$2"
    
    set_subtest_number "${subtest_number}"
    print_subtest "${subtest_name}"
}

setup_orchestration_environment() {

    # Get start time
    START_TIME=$(date +%s.%N 2>/dev/null)

    # Naturally we're orchestrating
    ORCHESTRATION=true

    # Starting point
    TIMESTAMP=$(date +%Y%m%d_%H%M%S)
    # TS_ORC_LOG=$(date '+%Y%m%d_%H%M%S' 2>/dev/null)             # 20250730_124718                 eg: log filenames
    # TS_ORC_TMR=$(date '+%s.%N' 2>/dev/null)                     # 1753904852.568389297            eg: timers, elapsed times
    # TS_ORC_ISO=$(date '+%Y-%m-%d %H:%M:%S %Z' 2>/dev/null)      # 2025-07-30 12:47:46 PDT         eg: short display times
    TS_ORC_DSP=$(date '+%Y-%b-%d (%a) %H:%M:%S %Z' 2>/dev/null) # 2025-Jul-30 (Wed) 12:49:03 PDT  eg: long display times

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

    # Common utilities
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
            print_message "Build directory already mounted as tmpfs, emptying contents..."
            if rm -rf build/* 2>/dev/null; then
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

    mkdir -p "${BUILD_DIR}" "${TESTS_DIR}" "${RESULTS_DIR}" "${LOGS_DIR}" "${DIAGS_DIR}"

    # Common files
    # shellcheck disable=SC2154,SC2153 # TEST_NUMBER defined externally in framework.sh
    RESULT_LOG="${RESULTS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}.log"
    LOG_FILE="${LOGS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}.log"
    DIAG_FILE="${DIAGS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}.log"

    # Default exclude patterns for linting (can be overridden by .lintignore)
    if [[ -z "${LINT_EXCLUDES:-}" ]]; then
        readonly LINT_EXCLUDES=(
            "build/*"
        )   
    fi

    dump_collected_output
    clear_collected_output
}

# Function to set up the standard test environment with numbering
setup_test_environment() {

    # Starting point
    TIMESTAMP=$(date +%Y%m%d_%H%M%S)

    if [[ -z "${ORCHESTRATION}" ]]; then
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

        # Setup build folders
        mkdir -p "${BUILD_DIR}" "${TESTS_DIR}" "${RESULTS_DIR}" "${DIAGS_DIR}" "${LOGS_DIR}" 

        # Common utilitiex
        TABLES_EXTERNAL="${LIB_DIR}/tables"
        OH_EXTERNAL="${LIB_DIR}/Oh.sh"
        COVERAGE_EXTERNAL="${LIB_DIR}/coverage_table.sh"
        SITEMAP_EXTERNAL="${LIB_DIR}/github-sitemap.sh"
    else
      pushd "${PROJECT_DIR}" >/dev/null 2>&1 || return
    fi

    # Common files
    # shellcheck disable=SC2154,SC2153 # TEST_NUMBER defined externally in framework.sh
    RESULT_LOG="${RESULTS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}.log"
    LOG_FILE="${LOGS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}.log"
    DIAG_FILE="${DIAGS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}.log"

    # Extra handling
    DIAG_TEST_DIR="${DIAGS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}"
    mkdir -p "${DIAG_TEST_DIR}"

    # Common test configuration
    TIMESTAMP=$(date +%Y%m%d_%H%M%S)
    EXIT_CODE=0
    PASS_COUNT=0

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

        # Default exclude patterns for linting (can be overridden by .lintignore)
        if [[ -z "${LINT_EXCLUDES:-}" ]]; then
            readonly LINT_EXCLUDES=(
                "build/*"
            )   
        fi
    fi

    dump_collected_output
    clear_collected_output
}

# Function to export the test result to a standardized JSON format
export_test_results() {
    local test_name=$1
    local result=$2
    local details=$3
    local output_file=$4
    
    # Create a simple JSON structure
    cat > "${output_file}" << EOF
{
    "test_name": "${test_name}",
    "status": ${result},
    "details": "${details}",
    "timestamp": "$(date +%Y-%m-%d\ %H:%M:%S || true)"
}
EOF
}

# Function to run a check and track pass/fail counts
run_check() {
    local check_name="$1"
    local check_command="$2"
    local passed_checks_var="$3"
    local failed_checks_var="$4"
    
    print_command "${check_command}"
    if eval "${check_command}" >/dev/null 2>&1; then
        if command -v print_message >/dev/null 2>&1; then
            print_message "✓ ${check_name} passed"
        else
            echo "INFO: ✓ ${check_name} passed"
        fi
        eval "${passed_checks_var}=\$((\${${passed_checks_var}} + 1))"
        return 0
    else
        if command -v print_warning >/dev/null 2>&1; then
            print_warning "✗ ${check_name} failed"
        else
            echo "WARNING: ✗ ${check_name} failed"
        fi
        eval "${failed_checks_var}=\$((\${${failed_checks_var}} + 1))"
        return 1
    fi
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

# Function to generate an HTML summary report for a test
generate_html_report() {
    local result_file=$1
    local html_file="${result_file%.log}.html"
    
    # Create HTML header
    cat > "${html_file}" << EOF
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Hydrogen Test Results</title>
    <style>
        body {
            font-family: 'Courier New', monospace;
            line-height: 1.4;
            color: #333;
            max-width: 1200px;
            margin: 0 auto;
            padding: 20px;
            background-color: #f8f9fa;
        }
        h1, h2, h3 {
            color: #0066cc;
        }
        .test-header {
            background-color: #0066cc;
            color: white;
            padding: 15px;
            border-radius: 5px;
            margin: 20px 0 10px 0;
            font-weight: bold;
        }
        .test-content {
            background-color: white;
            border: 1px solid #ddd;
            border-radius: 5px;
            padding: 15px;
            margin-bottom: 20px;
        }
        .pass { color: #28a745; font-weight: bold; }
        .fail { color: #dc3545; font-weight: bold; }
        .warn { color: #ffc107; font-weight: bold; }
        .info { color: #17a2b8; font-weight: bold; }
        .cmd { color: #6f42c1; font-weight: bold; }
        .out { color: #6c757d; }
        .timestamp {
            color: #666;
            font-size: 0.9em;
            margin-bottom: 20px;
        }
        pre {
            background-color: #f8f9fa;
            padding: 10px;
            border-radius: 3px;
            overflow-x: auto;
            border-left: 4px solid #0066cc;
        }
        .test-number {
            background-color: #28a745;
            color: white;
            padding: 2px 8px;
            border-radius: 3px;
            font-weight: bold;
            margin-right: 10px;
        }
    </style>
</head>
<body>
    <h1>🧪 Hydrogen Test Results</h1>
    <p class="timestamp">Generated on $(date || true)</p>
    
    <div class="test-content">
        <h2>Test Log Contents</h2>
        <pre>$(cat "${result_file}" 2>/dev/null || echo "Log file not found" || true)</pre>
    </div>
</body>
</html>
EOF
    
    if command -v print_message >/dev/null 2>&1; then
        print_message "HTML report generated: ${html_file}"
    else
        echo "INFO: HTML report generated: ${html_file}"
    fi
    return 0
}

# Test suite management functions
start_test_suite() {
    local test_name="$1"
    # Initialize test tracking variables
    PASSED_TESTS=0
    FAILED_TESTS=0
    CURRENT_SUBTEST=0
    
    # Print suite header if log_output.sh is available
    if command -v print_info >/dev/null 2>&1; then
        print_info "Starting test suite: ${test_name}"
    else
        echo "INFO: Starting test suite: ${test_name}"
    fi
}

end_test_suite() {
    local total_tests=$((PASSED_TESTS + FAILED_TESTS))
    
    if command -v print_info >/dev/null 2>&1; then
        print_info "Test suite completed"
        print_info "Total tests: ${total_tests}, Passed: ${PASSED_TESTS}, Failed: ${FAILED_TESTS}"
    else
        echo "INFO: Test suite completed"
        echo "INFO: Total tests: ${total_tests}, Passed: ${PASSED_TESTS}, Failed: ${FAILED_TESTS}"
    fi
}

# Test counting functions
increment_passed_tests() {
    PASSED_TESTS=$((PASSED_TESTS + 1))
}

increment_failed_tests() {
    FAILED_TESTS=$((FAILED_TESTS + 1))
}

get_passed_tests() {
    echo "${PASSED_TESTS}"
}

get_failed_tests() {
    echo "${FAILED_TESTS}"
}

get_total_tests() {
    echo $((PASSED_TESTS + FAILED_TESTS))
}

get_exit_code() {
    if [[ "${FAILED_TESTS}" -gt 0 ]]; then
        echo 1
    else
        echo 0
    fi
}

# Subtest management functions
print_subtest_header() {
    local subtest_name="$1"
    CURRENT_SUBTEST=$((CURRENT_SUBTEST + 1))
    
    if command -v print_subtest_header >/dev/null 2>&1; then
        print_subtest_header "${subtest_name}"
    else
        echo ""
        echo "--- Subtest ${CURRENT_SUBTEST}: ${subtest_name} ---"
    fi
}

skip_remaining_subtests() {
    local reason="$1"
    if command -v print_warning >/dev/null 2>&1; then
        print_warning "Skipping remaining subtests: ${reason}"
    else
        echo "WARNING: Skipping remaining subtests: ${reason}"
    fi
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
    temp_readme=$(mktemp) || { echo "Error: Failed to create temporary file" >&2; return 1; }
    
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
                echo "Generated on: ${TS_ORC_DSP}"
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
                echo "Generated via cloc: ${TS_ORC_DSP}"
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