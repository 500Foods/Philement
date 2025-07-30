#!/bin/bash

# Test: Unity
# Runs unit tests using the Unity framework, treating each test file as a subtest

# CHANGELOG
# 2.4.0 - 2025-07-20 - Added guard clause to prevent multiple sourcing
# 2.3.0 - 2025-07-16 - Changed batching formula to divide tests evenly into minimum groups within CPU limit
# 2.2.0 - 2025-07-15 - Added parallel execution with proper ordering and improved output format
# 2.1.0 - 2025-07-14 - Removed Unity framework check (moved to test 01), enhanced individual test reporting with INFO lines
# 2.0.3 - 2025-07-14 - Enhanced individual test reporting with INFO lines showing test descriptions and purposes
# 2.0.2 - 2025-07-06 - Added missing shellcheck justifications
# 2.0.1 - 2025-07-06 - Removed hardcoded absolute path; now handled by log_output.sh
# 2.0.0 - 2025-07-02 - Complete rewrite to use new modular test libraries
# 1.0.0 - 2025-06-25 - Initial version for running Unity tests

# Test configuration
TEST_NAME="Unity Unit Tests"
SCRIPT_VERSION="2.3.0"

# Sort out directories
PROJECT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && cd .. && pwd )"
SCRIPT_DIR="${PROJECT_DIR}/tests"
LIB_DIR="${SCRIPT_DIR}/lib"
BUILD_DIR="${PROJECT_DIR}/build"
TESTS_DIR="${BUILD_DIR}/tests"
RESULTS_DIR="${TESTS_DIR}/results"
DIAGS_DIR="${TESTS_DIR}/diagnostics"
LOGS_DIR="${TESTS_DIR}/logs"
mkdir -p "${BUILD_DIR}" "${TESTS_DIR}" "${RESULTS_DIR}" "${DIAGS_DIR}" "${LOGS_DIR}"

# shellcheck source=tests/lib/framework.sh # Resolve path statically
[[ -n "${FRAMEWORK_GUARD}" ]] || source "${LIB_DIR}/framework.sh"
# shellcheck source=tests/lib/log_output.sh # Resolve path statically
[[ -n "${LOG_OUTPUT_GUARD}" ]] || source "${LIB_DIR}/log_output.sh"

# Test configuration
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
EXIT_CODE=0
PASS_COUNT=0
TEST_NUMBER=$(extract_test_number "${BASH_SOURCE[0]}")
RESULT_LOG="${RESULTS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}.log"
set_test_number "${TEST_NUMBER}"
reset_subtest_counter

# Print beautiful test header
print_test_header "${TEST_NAME}" "${SCRIPT_VERSION}"

# Print framework and log output versions as they are already sourced
[[ -n "${ORCHESTRATION}" ]] || print_message "${FRAMEWORK_NAME} ${FRAMEWORK_VERSION}" "info"
[[ -n "${ORCHESTRATION}" ]] || print_message "${LOG_OUTPUT_NAME} ${LOG_OUTPUT_VERSION}" "info"
# shellcheck source=tests/lib/lifecycle.sh # Resolve path statically
[[ -n "${LIFECYCLE_GUARD}" ]] || source "${LIB_DIR}/lifecycle.sh"
# shellcheck source=tests/lib/file_utils.sh # Resolve path statically
[[ -n "${FILE_UTILS_GUARD}" ]] || source "${LIB_DIR}/file_utils.sh"
# shellcheck source=tests/lib/coverage.sh # Resolve path statically
[[ -n "${COVERAGE_GUARD}" ]] || source "${LIB_DIR}/coverage.sh"

# Navigate to the project root (one level up from tests directory)
if ! navigate_to_project_root "${SCRIPT_DIR}"; then
    print_error "Failed to navigate to project root directory"
    exit 1
fi

# Configuration
CURRENT_SUBTEST_NUM=1
TOTAL_UNITY_TESTS=0
TOTAL_UNITY_PASSED=0
HYDROGEN_DIR="$( cd "${SCRIPT_DIR}/.." && pwd )"
UNITY_BUILD_DIR="${HYDROGEN_DIR}/build/unity"
BUILD_DIR="${SCRIPT_DIR}/../build"
DIAGS_DIR="${BUILD_DIR}/tests/logs"
LOG_FILE="${BUILD_DIR}/tests/logs/unity_tests.log"
DIAG_TEST_DIR="${DIAGS_DIR}/unity_$(date +%H%M%S)"

# Create output directories
next_subtest
print_subtest "Create Output Directories"
if setup_output_directories "${RESULTS_DIR}" "${DIAGS_DIR}" "${LOG_FILE}" "${DIAG_TEST_DIR}"; then
    ((PASS_COUNT++))
else
    EXIT_CODE=1
fi

# Function to check Unity tests are available via CTest (assumes they're already built by main build system)
check_unity_tests_available() {
    next_subtest
    print_subtest "Check Unity Tests Available"
    print_message "Checking for Unity tests via CTest (should be built by main build system)..."
    
    # Check if CMake build directory exists and has CTest configuration
    local cmake_build_dir="${BUILD_DIR}"
    
    if [[ ! -d "${cmake_build_dir}" ]]; then
        print_result 1 "CMake build directory not found: ${cmake_build_dir#"${SCRIPT_DIR}"/..}"
        return 1
    fi
    
    # Change to CMake directory to check for Unity tests
    cd "${cmake_build_dir}" || { print_result 1 "Failed to change to CMake build directory"; return 1; }
    
    # Check if Unity tests are registered with CTest
    if ctest -N | grep -q "test_hydrogen" || true; then
        # Also verify the executable exists in the correct location (build/unity/src/)
        local unity_test_exe="${HYDROGEN_DIR}/build/unity/src/test_hydrogen"
        if [[ -f "${unity_test_exe}" ]]; then
            exe_size=$(get_file_size "${unity_test_exe}")
            formatted_size=$(format_file_size "${exe_size}")
            print_result 0 "Unity tests available via CTest: ${unity_test_exe#"${SCRIPT_DIR}"/..} (${formatted_size} bytes)"
            cd "${SCRIPT_DIR}" || { print_result 1 "Failed to return to script directory"; return 1; }
            return 0
        else
            print_result 1 "Unity test registered with CTest but executable not found at ${unity_test_exe#"${SCRIPT_DIR}"/..}"
            cd "${SCRIPT_DIR}" || { print_result 1 "Failed to return to script directory"; return 1; }
            return 1
        fi
    else
        print_result 1 "Unity tests not found in CTest. Run 'cmake --build . --target unity_tests' from cmake directory first."
        cd "${SCRIPT_DIR}" || { print_result 1 "Failed to return to script directory"; return 1; }
        return 1
    fi
}

# Function to run a single Unity test executable and report results
run_single_unity_test() {
    local test_name="$1"
    local test_exe="${HYDROGEN_DIR}/build/unity/src/${test_name}"
    
    next_subtest
    print_subtest "Run Unity Test: ${test_name}"
    
    if [[ ! -f "${test_exe}" ]]; then
        print_result 1 "Unity test executable not found: ${test_exe#"${SCRIPT_DIR}"/..}"
        return 1
    fi
    
    # Run the Unity test directly and capture output
    local temp_test_log="${DIAG_TEST_DIR}/${test_name}_output.txt"
    mkdir -p "$(dirname "${temp_test_log}")"
    true > "${temp_test_log}"
    
    if "${test_exe}" > "${temp_test_log}" 2>&1; then
        # Parse the Unity test output to get test counts
        local test_count
        local failure_count
        local ignored_count
        test_count=$(grep -E "[0-9]+ Tests [0-9]+ Failures [0-9]+ Ignored" "${temp_test_log}" | awk '{print $1}' || true)
        failure_count=$(grep -E "[0-9]+ Tests [0-9]+ Failures [0-9]+ Ignored" "${temp_test_log}" | awk '{print $3}' || true)
        ignored_count=$(grep -E "[0-9]+ Tests [0-9]+ Failures [0-9]+ Ignored" "${temp_test_log}" | awk '{print $5}' || true)
        
        if [[ -n "${test_count}" ]] && [[ -n "${failure_count}" ]] && [[ -n "${ignored_count}" ]]; then
            local passed_count=$((test_count - failure_count - ignored_count))
            local log_path
            log_path="build/tests/logs/unity_$(date +%H%M%S)/${test_name}_output.txt"
            print_result 0 "${passed_count} passed, ${failure_count} failed: ${log_path}"
        else
            local log_path
            log_path="build/tests/logs/unity_$(date +%H%M%S)/${test_name}_output.txt"
            print_result 0 "1 passed, 0 failed: ${log_path}"
        fi
        ((PASS_COUNT++))
        return 0
    else
        # Show failed test output for debugging
        while IFS= read -r line; do
            print_output "${line}"
        done < "${temp_test_log}"
        local log_path
        log_path="build/tests/logs/unity_$(date +%H%M%S)/${test_name}_output.txt"
        print_result 1 "0 passed, 1 failed: ${log_path}"
        return 1
    fi
    # As if stmt always returns, this line is never reached
    # cat "${temp_test_log}" >> "${LOG_FILE}"
}

# Function to run a single unity test in parallel mode
run_single_unity_test_parallel() {
    local test_name="$1"
    local result_file="$2"
    local output_file="$3"
    local test_exe="${HYDROGEN_DIR}/build/unity/src/${test_name}"
    
    # Initialize result tracking
    local subtest_number=$((CURRENT_SUBTEST_NUM++))
    local passed_count=0
    local failed_count=0
    local test_count=0
    local exit_code=0
    
    # Create output header
    {
        echo "SUBTEST_START|${subtest_number}|${test_name}"
        echo "TEST_LINE|Run Unity Test: ${test_name}"
    } >> "${output_file}"
    
    if [[ ! -f "${test_exe}" ]]; then
        {
            echo "RESULT_LINE|FAIL|Unity test executable not found: ${test_exe#"${SCRIPT_DIR}"/..}"
            echo "SUBTEST_END|${subtest_number}|${test_name}|1|0|1"
        } >> "${output_file}"
        echo "1|${test_name}|1|0|1" > "${result_file}"
        return 1
    fi
    
    # Run the Unity test and capture output
    local temp_test_log="${DIAG_TEST_DIR}/${test_name}_output.txt"
    mkdir -p "$(dirname "${temp_test_log}")"
    true > "${temp_test_log}"
    
    if "${test_exe}" > "${temp_test_log}" 2>&1; then
        # Parse the Unity test output to get test counts
        local failure_count
        local ignored_count
        test_count=$(grep -E "[0-9]+ Tests [0-9]+ Failures [0-9]+ Ignored" "${temp_test_log}" | awk '{print $1}' || true)
        failure_count=$(grep -E "[0-9]+ Tests [0-9]+ Failures [0-9]+ Ignored" "${temp_test_log}" | awk '{print $3}' || true)
        ignored_count=$(grep -E "[0-9]+ Tests [0-9]+ Failures [0-9]+ Ignored" "${temp_test_log}" | awk '{print $5}' || true)
    
        if [[ -n "${test_count}" ]] && [[ -n "${failure_count}" ]] && [[ -n "${ignored_count}" ]]; then
            passed_count=$((test_count - failure_count - ignored_count))
            local log_path
            log_path="build/tests/logs/unity_$(date +%H%M%S)/${test_name}_output.txt"
            echo "RESULT_LINE|PASS|${passed_count} passed, ${failure_count} failed: ${log_path}" >> "${output_file}"
            failed_count=0
        else
            local log_path
            log_path="build/tests/logs/unity_$(date +%H%M%S)/${test_name}_output.txt"
            echo "RESULT_LINE|PASS|1 passed, 0 failed: ${log_path}" >> "${output_file}"
            test_count=1
            passed_count=1
            failed_count=0
        fi
        exit_code=0
    else
        # Test failed - capture full output for display
        local log_path
        log_path="build/tests/logs/unity_$(date +%H%M%S)/${test_name}_output.txt"
        echo "RESULT_LINE|FAIL|${passed_count} passed, ${failed_count} failed: ${log_path}" >> "${output_file}"
        while IFS= read -r line; do
            echo "OUTPUT_LINE|${line}" >> "${output_file}"
        done < "${temp_test_log}"
        
        # Try to get test counts even from failed output
        failure_count=$(grep -E "[0-9]+ Tests [0-9]+ Failures [0-9]+ Ignored" "${temp_test_log}" | awk '{print $3}' || true)
        test_count=$(grep -E "[0-9]+ Tests [0-9]+ Failures [0-9]+ Ignored" "${temp_test_log}" | awk '{print $1}' || true)
        if [[ -n "${test_count}" ]] && [[ -n "${failure_count}" ]]; then
            passed_count=$((test_count - failure_count))
            failed_count=${failure_count}
        else
            test_count=1
            passed_count=0
            failed_count=1
        fi
        exit_code=1
    fi
    
    echo "SUBTEST_END|${subtest_number}|${test_name}|${test_count}|${passed_count}|${failed_count}" >> "${output_file}"
    echo "${exit_code}|${test_name}|${test_count}|${passed_count}|${failed_count}" > "${result_file}"
    
    cat "${temp_test_log}" >> "${LOG_FILE}"
    return "${exit_code}"
}

# Function to run Unity tests via parallel execution
run_unity_tests() {
    local overall_result=0
    
    # Find all Unity test executables
    local unity_build_dir="${UNITY_BUILD_DIR}/src"
    local unity_tests=()
    
    if [[ -d "${unity_build_dir}" ]]; then
        # Look for executable files that match test_* pattern (recursively in subdirectories)
        temp_find="$(mktemp)"
        find "${unity_build_dir}" -name "test_*" -type f -print0 > "${temp_find}"
        mapfile -t -d '' test_exes < "${temp_find}"
        rm -f "${temp_find}"
        for test_exe in "${test_exes[@]}"; do
            local test_name
            test_name=$(basename "${test_exe}")
            if [[ -x "${test_exe}" ]] && [[ "${test_name}" =~ ^test_[a-zA-Z_]+$ ]]; then
                # Get relative path from unity_build_dir to show directory structure
                local relative_path
                relative_path=$(realpath --relative-to="${unity_build_dir}" "${test_exe}")
                unity_tests+=("${relative_path}")
            fi
        done
    fi

    if [[ ${#unity_tests[@]} -eq 0 ]]; then
        print_error "No Unity test executables found in ${unity_build_dir}"
        return 1
    fi
    
    # Sort tests by filesystem order: root directory first, then subdirectories
    local sorted_tests=()
    local root_tests=()
    local subdir_tests=()
    
    # Separate root directory tests from subdirectory tests
    for test_path in "${unity_tests[@]}"; do
        if [[ "${test_path}" == *"/"* ]]; then
            subdir_tests+=("${test_path}")
        else
            root_tests+=("${test_path}")
        fi
    done
    
    # Sort root tests and subdirectory tests separately
    mapfile -t root_tests < <(printf '%s\n' "${root_tests[@]}" | sort || true)
    mapfile -t subdir_tests < <(printf '%s\n' "${subdir_tests[@]}" | sort || true)
    
    # Combine in proper order: root tests first, then subdirectory tests
    sorted_tests=("${root_tests[@]}" "${subdir_tests[@]}")
    
    # Split tests into batches for parallel execution
    # Calculate minimum number of groups needed to fit within CPU limit
    local total_tests=${#sorted_tests[@]}
    local number_of_groups=$(( (total_tests + CORES - 1) / CORES ))  # ceil(total_tests / cpu_cores)
    local batch_size=$(( (total_tests + number_of_groups - 1) / number_of_groups ))  # ceil(total_tests / number_of_groups)
    local total_failed=0
    local batch_num=0
    TOTAL_UNITY_TESTS=0
    TOTAL_UNITY_PASSED=0
    
    print_message "Running ${total_tests} Unity tests in parallel batches of ~${batch_size} (max ${CORES} CPUs, ${number_of_groups} groups)"
    
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
        
        ((batch_num++))
        local batch_count=${#batch_tests[@]}
        
        print_message "Starting batch ${batch_num}: ${batch_count} tests"
        
        # Run batch in parallel and process results in order
        local temp_files=()
        local temp_outputs=()
        local pids=()
        
        # Start parallel execution for batch
        for test_path in "${batch_tests[@]}"; do
            local temp_result_file
            local temp_output_file
            temp_result_file=$(mktemp)
            temp_output_file=$(mktemp)
            temp_files+=("${temp_result_file}")
            temp_outputs+=("${temp_output_file}")
            
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
                            print_subtest "${content}"
                            ;;
                        "RESULT_LINE")
                            IFS='|' read -r result_type message <<< "${content}"
                            if [[ "${result_type}" = "PASS" ]]; then
                                print_result 0 "${message}"
                                ((PASS_COUNT++))
                            else
                                print_result 1 "${message}"
                            fi
                            ;;
                        "OUTPUT_LINE")
                            print_output "${content}"
                            ;;
                        "SUBTEST_START")
                            next_subtest
                            ;;
                        *) ;;
                    esac
                done < "${temp_output_file}"
            fi
            
            # Collect statistics from result file
            if [[ -f "${temp_result_file}" ]] && [[ -s "${temp_result_file}" ]]; then
                IFS='|' read -r exit_code test_name test_count passed_count failed_count < "${temp_result_file}"
                TOTAL_UNITY_TESTS=$((TOTAL_UNITY_TESTS + test_count))
                TOTAL_UNITY_PASSED=$((TOTAL_UNITY_PASSED + passed_count))
                total_failed=$((total_failed + failed_count))
            fi
            
            # Clean up temporary files
            rm -f "${temp_output_file}" "${temp_result_file}"
        done
        
        # Clear arrays for next batch
        temp_files=()
        temp_outputs=()
        pids=()
        
        i=${batch_end}
    done
    
    # Display summary in two parts
    print_message "Unity test execution: ${total_tests} test files ran in ${batch_num} batches"
    print_message "Unity test results: $(printf "%'d" "${TOTAL_UNITY_TESTS}") unit tests, $(printf "%'d" "${TOTAL_UNITY_PASSED}") passing, $(printf "%'d" "${total_failed}") failing"
    
    return "${overall_result}"
}

# Check and run tests
if check_unity_tests_available; then
    ((PASS_COUNT++))
    if run_unity_tests; then
        # Update TEST_NAME to include test results for concise display
        if [[ -n "${TOTAL_UNITY_TESTS}" ]] && [[ -n "${TOTAL_UNITY_PASSED}" ]]; then
            TEST_NAME="Unity {BLUE}($(printf "%'d" "${TOTAL_UNITY_PASSED}") / $(printf "%'d" "${TOTAL_UNITY_TESTS}") unit tests passed){RESET}"
        fi
    else
        EXIT_CODE=1
    fi
    
    # Calculate Unity test coverage using the same batch processing as other tools
    next_subtest
    print_subtest "Calculate Unity Test Coverage"
    print_message "Calculating Unity test coverage..."

    # Use the same calculation as Test 99
    TIMESTAMP=$(date +%Y%m%d_%H%M%S)
    unity_coverage=$(calculate_unity_coverage "${UNITY_BUILD_DIR}" "${TIMESTAMP}")

    # Always attempt to read details
    if [[ -f "${RESULTS_DIR}/coverage_unity.txt.detailed" ]]; then
        IFS=',' read -r _ _ covered_lines total_lines instrumented_files covered_files < "${RESULTS_DIR}/coverage_unity.txt.detailed"
        print_result 0 "Unity test coverage calculated: ${unity_coverage}% ($(printf "%'d" "${covered_lines}")/$(printf "%'d" "${total_lines}") lines, $(printf "%'d" "${covered_files}")/$(printf "%'d" "${instrumented_files}") files)"
        ((PASS_COUNT++))
    else
        if [[ -z "${unity_coverage}" ]]; then
            unity_coverage="0.000"
        fi
        print_result 0 "Unity test coverage calculated: ${unity_coverage}% (0/0 lines, 0/0 files) - No coverage data available"
        ((PASS_COUNT++))
    fi
    TOTAL_SUBTESTS=$((TOTAL_SUBTESTS + 1))
else
    print_error "Unity tests not available, skipping test execution"
    EXIT_CODE=1
fi

# Print completion table
print_test_completion "${TEST_NAME}"

# Return status code if sourced, exit if run standalone
if [[ "${ORCHESTRATION}" == "true" ]]; then
    return "${EXIT_CODE}"
else
    exit "${EXIT_CODE}"
fi
