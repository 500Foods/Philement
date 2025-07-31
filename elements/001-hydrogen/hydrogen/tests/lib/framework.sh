#!/bin/bash

# Framework Library
# Provides test lifecycle management and result tracking functions

# LIBRARY FUNCTIONS
# format_time_duration()
# perform_cleanup()
# start_test()
# start_subtest()
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

# CHANGELOG
# 2.2.0 - 2025-07-20 - Added guard
# 2.1.0 - 2025-07-07 - Restructured how test elapsed times are stored and accessed
# 2.0.0 - 2025-07-02 - Updated to integrate with numbered output system
# 1.0.0 - 2025-07-02 - Initial creation from support_utils.sh migration

# Guard clause to prevent multiple sourcing
[[ -n "${FRAMEWORK_GUARD}" ]] && return 0
export FRAMEWORK_GUARD="true"

# Library metadata
FRAMEWORK_NAME="Framework Library"
FRAMEWORK_VERSION="2.2.0"
export FRAMEWORK_NAME FRAMEWORK_VERSION
# print_message "${FRAMEWORK_NAME} ${FRAMEWORK_VERSION}" "info"

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

# Perform cleanup before test execution
perform_cleanup() {
    # Clear out build directory
    rm -rf "${BUILD_DIR:?}" > /dev/null 2>&1

    # Remove hydrogen executables silently
    rm -f "${PROJECT_DIR}/hydrogen*" > /dev/null 2>&1

    # Build necessary folders
    mkdir -p "${BUILD_DIR}" "${TESTS_DIR}" "${RESULTS_DIR}" "${DIAGS_DIR}" "${LOGS_DIR}"
}

# Function to start a test run with proper header and numbering
start_test() {
    local test_name="$1"
    local script_version="$2"
    local script_path="$3"
    
    # Auto-extract test number from script filename if not provided
    local test_number
    if command -v extract_test_number >/dev/null 2>&1; then
        test_number=$(extract_test_number "${script_path}")
        set_test_number "${test_number}"
        reset_subtest_counter
        print_test_header "${test_name}" "${script_version}"
    else
        # Fallback if log_output.sh not available
        echo "==============================================================================="
        echo "TEST: ${test_name}"
        echo "==============================================================================="
        echo "Started at: $(date)" || true
        echo ""
    fi
}

# Function to start a subtest with proper header and numbering
start_subtest() {
    local subtest_name="$1"
    local subtest_number="$2"
    
    # Set the subtest number in log_output.sh
    if command -v set_subtest_number >/dev/null 2>&1; then
        set_subtest_number "${subtest_number}"
        print_subtest "${subtest_name}"
    else
        # Fallback if log_output.sh not available
        echo ""
        echo "-------------------------------------------------------------------------------"
        echo "SUBTEST ${subtest_number}: ${subtest_name}"
        echo "-------------------------------------------------------------------------------"
    fi
}

# Function to set up the standard test environment with numbering
setup_test_environment() {

    # Starting point
    TIMESTAMP=$(date +%Y%m%d_%H%M%S)

    # Global folder variables
    PROJECT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && cd ../.. && pwd )"
    cd "${PROJECT_DIR}" || exit

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

    # Common files
    # shellcheck disable=SC2154,SC2153 # TEST_NUMBER defined externally in framework.sh
    RESULT_LOG="${RESULTS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}.log"
    LOG_FILE="${LOGS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}.log"
    DIAG_FILE="${DIAGS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}.log"

    # Extra handling
    DIAG_TEST_DIR="${DIAGS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}"
    mkdir -p "${DIAG_TEST_DIR}"

    # Common utilitiex
    TABLES_EXTERNAL="${LIB_DIR}/tables"
    OH_EXTERNAL="${LIB_DIR}/Oh.sh"
    COVERAGE_EXTERNAL="${LIB_DIR}/coverage_table.sh"
    SITEMAP_EXTERNAL="${LIB_DIR}/github-sitemap.sh"
        
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

    dump_collected_output
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
