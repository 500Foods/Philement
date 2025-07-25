#!/bin/bash

# Framework Library
# Provides test lifecycle management and result tracking functions

# LIBRARY FUNCTIONS
# format_time_duration()
# perform_cleanup()
# start_test()
# start_subtest()
# end_test()
# setup_test_environment()
# navigate_to_project_root()
# export_subtest_results()
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

# Sort out directories
PROJECT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && cd ../.. && pwd )"
# SCRIPT_DIR="${PROJECT_DIR}/tests"
# LIB_DIR="${SCRIPT_DIR}/lib"
BUILD_DIR="${PROJECT_DIR}/build"
TESTS_DIR="${BUILD_DIR}/tests"
RESULTS_DIR="${TESTS_DIR}/results"
DIAGS_DIR="${TESTS_DIR}/diagnostics"
LOGS_DIR="${TESTS_DIR}/logs"
mkdir -p "${BUILD_DIR}" "${TESTS_DIR}" "${RESULTS_DIR}" "${DIAGS_DIR}" "${LOGS_DIR}"

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

# Function to end a test run with proper summary
end_test() {
    local test_result=$1
    local total_subtests=$2
    local passed_subtests=$3
    
    # Calculate failed subtests
    local failed_subtests=$((total_subtests - passed_subtests))
    
    # Use the new print_test_summary function
    if command -v print_test_summary >/dev/null 2>&1; then
        print_test_summary "${total_subtests}" "${passed_subtests}" "${failed_subtests}"
    else
        # Fallback if log_output.sh not available
        echo ""
        echo "==============================================================================="
        echo "Test Summary"
        echo "==============================================================================="
        echo "Completed at: $(date)" || true
        if [[ "${test_result}" -eq 0 ]]; then
            echo "OVERALL RESULT: ALL TESTS PASSED"
        else
            echo "OVERALL RESULT: SOME TESTS FAILED"
        fi
        echo "==============================================================================="
        echo ""
    fi
    
    return "${test_result}"
}

# Function to set up the standard test environment with numbering
setup_test_environment() {
    local test_name="$1"
    local test_number="$2"
    local script_dir
    script_dir="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
    
    # Create results directory in build/tests/results
    local build_dir="${script_dir}/../build"
    RESULTS_DIR="${build_dir}/tests/results"
    mkdir -p "${RESULTS_DIR}"
    
    # Get timestamp for this test run
    TIMESTAMP=$(date +%Y%m%d_%H%M%S)
    
    # Create a test-specific log file with shorter name
    local test_id
    test_id="test_${test_number}"
    RESULT_LOG="${RESULTS_DIR}/${test_id}_${TIMESTAMP}.log"
    
    # Start the test with numbering
    start_test "${test_name}" "${test_number}" | tee -a "${RESULT_LOG}" || true
    
    # Return log file path
    echo "${RESULT_LOG}"
}

# Function to navigate to the project root directory
navigate_to_project_root() {
    local script_dir="$1"
    local project_root="${script_dir}/.."
    if ! safe_cd "${project_root}"; then
        if command -v print_error >/dev/null 2>&1; then
            print_error "Failed to navigate to project root directory"
        else
            echo "ERROR: Failed to navigate to project root directory"
        fi
        return 1
    fi
    return 0
}

# Function to save subtest statistics and test name for use by test_all.sh
# NOTE: This function is now a no-op since print_test_completion() handles file writing
# to ensure single source of truth for timing. Kept for backward compatibility.
export_subtest_results() {
    local test_id=$1
    local total_subtests=$2
    local passed_subtests=$3
    local test_name=$4
    
    # This function no longer writes files to avoid duplicate file creation
    # with different timing values. File writing is now handled exclusively
    # by print_test_completion() in log_output.sh for single source of truth.
    
    # Return silently - no need to announce this internal operation
    return 0
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

# Function to evaluate test results and update pass count with output
evaluate_test_result() {
    local test_name="$1"
    local failed_checks="$2"
    local pass_count_var="$3"
    local exit_code_var="$4"
    
    if [[ "${failed_checks}" -eq 0 ]]; then
        if command -v print_result >/dev/null 2>&1; then
            print_result 0 "${test_name} test PASSED"
        else
            echo "RESULT: ${test_name} test PASSED"
        fi
        eval "${pass_count_var}=\$((\${${pass_count_var}} + 1))"
    else
        if command -v print_result >/dev/null 2>&1; then
            print_result 1 "${test_name} test FAILED"
        else
            echo "RESULT: ${test_name} test FAILED"
        fi
        eval "${exit_code_var}=1"
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
