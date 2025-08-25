#!/usr/bin/env bash

# Test: API Prefix
# Tests the API endpoints with different API prefix configurations using immediate restart approach:
# - Default "/api" prefix using standard hydrogen_test_api.json
# - Custom "/myapi" prefix using hydrogen_test_api_prefix.json
# - Uses immediate restart without waiting for TIME_WAIT (SO_REUSEADDR enabled)

# FUNCTIONS
# validate_api_request()
# run_api_prefix_test_parallel()
# analyze_api_prefix_test_results()

# CHANGELOG
# 7.0.0 - 2025-08-08 - Major refactor: Implemented parallel execution of API prefix tests following Test 13/20 patterns. 
#                    - Extracted modular functions run_api_prefix_test_parallel() and analyze_api_prefix_test_results(). 
#                    - Now runs both /api and /myapi tests simultaneously instead of sequentially, significantly reducing execution time.
# 6.0.0 - 2025-07-30 - Overhaul #1
# 5.0.7 - 2025-07-14 - Enhanced validate_api_request with retry logic to handle API subsystem initialization delays during parallel execution
# 5.0.6 - 2025-07-15 - No more sleep
# 5.0.5 - 2025-07-14 - Fixed database and log file conflicts in parallel execution by creating unique config files
# 5.0.4 - 2025-07-14 - Fixed parallel execution file conflicts by enhancing unique ID generation
# 5.0.3 - 2025-07-14 - Updated to use build/tests directories for test output consistency
# 5.0.2 - 2025-07-14 - Improved error handling in validate_api_request function to better handle parallel test execution
# 5.0.1 - 2025-07-06 - Added missing shellcheck justifications
# 5.0.0 - 2025-07-02 - Migrated to use lib/ scripts, following established test patterns
# 4.0.0 - 2025-06-30 - Updated to use immediate restart approach without TIME_WAIT waiting
# 3.0.0 - 2025-06-30 - Refactored to use robust server management functions from test_55
# 2.0.0 - 2025-06-17 - Major refactoring: fixed all shellcheck warnings, improved modularity, enhanced comments

set -euo pipefail

# Test Configuration
TEST_NAME="API Prefix"
TEST_ABBR="PRE"
TEST_NUMBER="20"
TEST_COUNTER=0
TEST_VERSION="7.0.0"

# shellcheck source=tests/lib/framework.sh # Reference framework directly
[[ -n "${FRAMEWORK_GUARD:-}" ]] || source "$(dirname "${BASH_SOURCE[0]}")/lib/framework.sh"
setup_test_environment

# Parallel execution configuration
declare -a PARALLEL_PIDS
declare -A API_TEST_CONFIGS

# API test configuration - format: "config_file:log_suffix:api_prefix:description"
API_TEST_CONFIGS=(
    ["First"]="${SCRIPT_DIR}/configs/hydrogen_test_${TEST_NUMBER}_prefix_1.json:one:/api:First API Prefix"
    ["Second"]="${SCRIPT_DIR}/configs/hydrogen_test_${TEST_NUMBER}_prefix_2.json:two:/myapi:Second API Prefix"
)

# Test timeouts
STARTUP_TIMEOUT=15
SHUTDOWN_TIMEOUT=10

# Function to make a request and validate response with retry logic for API readiness
validate_api_request() {
    local request_name="$1"
    local url="$2"
    local expected_field="$3"
    local response_file="${LOG_PREFIX}test_${TEST_NUMBER}_${TIMESTAMP}_${request_name}.json"
    local error_file="${LOG_PREFIX}test_${TEST_NUMBER}_${TIMESTAMP}_${request_name}.log"

    print_command "${TEST_NUMBER}" "${TEST_COUNTER}" "curl -s --max-time 10 --compressed \"${url}\""
    
    # Retry logic for API readiness (especially important in parallel execution)
    local max_attempts=25
    local attempt=1
    local curl_exit_code=0
        
    while [[ "${attempt}" -le "${max_attempts}" ]]; do
        if [[ "${attempt}" -gt 1 ]]; then
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "API request attempt ${attempt} of ${max_attempts} (waiting for API subsystem initialization)..."
            #sleep 0.05 
        fi
        
        # Run curl and capture both exit code and any error output
        curl -s --max-time 10 --compressed "${url}" > "${response_file}" 2>"${error_file}"
        curl_exit_code=$?
        
        if [[ "${curl_exit_code}" -eq 0 ]]; then
            # Verify the file actually exists and has content
            if [[ -f "${response_file}" ]] && [[ -s "${response_file}" ]]; then
                # Check if we got a 404 or other error response
                if "${GREP}" -q "404 Not Found" "${response_file}" || "${GREP}" -q "<html>" "${response_file}"; then
                    if [[ "${attempt}" -eq "${max_attempts}" ]]; then
                        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "API endpoint still not ready after ${max_attempts} attempts"
                        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "API endpoint returned 404 or HTML error page"
                        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Response content:"
                        print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "$(cat "${response_file}" || true)"
                        return 1
                    else
                        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "API endpoint not ready yet (got 404/HTML), retrying..."
                        ((attempt++))
                        continue
                    fi
                fi
                
                print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Request successful, checking response content"
                
                # Validate that the response contains expected fields
                if "${GREP}" -q "${expected_field}" "${response_file}"; then
                    if [[ "${attempt}" -gt 1 ]]; then
                        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Response contains expected field: ${expected_field} (succeeded on attempt ${attempt})"
                    else
                        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Response contains expected field: ${expected_field}"
                    fi
                    return 0
                else
                    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Response doesn't contain expected field: ${expected_field}"
                    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Response content:"
                    print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "$(cat "${response_file}" || true)"
                    return 1
                fi
            else
                print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Response file was not created or is empty: ${response_file}"
                return 1
            fi
        else
            if [[ "${attempt}" -eq "${max_attempts}" ]]; then
                print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Failed to connect to server (curl exit code: ${curl_exit_code})"
                # Show curl error if available
                if [[ -f "${error_file}" ]] && [[ -s "${error_file}" ]]; then
                    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Curl error output:"
                    print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "$(cat "${error_file}" || true)"
                fi
                return 1
            else
                print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Connection failed on attempt ${attempt}, retrying..."
                ((attempt++))
                continue
            fi
        fi
    done
    
    return 1
}

# Function to test API endpoints for a specific configuration in parallel
run_api_prefix_test_parallel() {
    local test_name="$1"
    local config_file="$2"
    local log_suffix="$3"
    local api_prefix="$4"
    local description="$5"
    
    local log_file="${LOGS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}_${log_suffix}.log"
    local result_file="${LOG_PREFIX}test_${TEST_NUMBER}_${TIMESTAMP}_${log_suffix}.result"

    # Clear result file
    echo "TEST_NAME=${TEST_NAME}" >> "${result_file}"

    local port
    port=$(get_webserver_port "${config_file}")
    
    # Start hydrogen server
    "${HYDROGEN_BIN}" "${config_file}" > "${log_file}" 2>&1 &
    local hydrogen_pid=$!
    
    # Store PID for later reference
    echo "PID=${hydrogen_pid}" >> "${result_file}"
    
    # Wait for startup
    local startup_success=false
    local start_time
    start_time=${SECONDS}
    
    while true; do
        if [[ $((SECONDS - start_time)) -ge "${STARTUP_TIMEOUT}" ]]; then
            break
        fi
        
        if "${GREP}" -q "Application started" "${log_file}" 2>/dev/null; then
            startup_success=true
            break
        fi
        sleep 0.05
    done
    
    if [[ "${startup_success}" = true ]]; then
        echo "STARTUP_SUCCESS" >> "${result_file}"
        
        # Wait for server to be ready
        # shellcheck disable=SC2310 # We want to continue even if the test fails
        if wait_for_server_ready "http://localhost:${port}"; then
            echo "SERVER_READY" >> "${result_file}"
            
            # Test all API endpoints
            local all_tests_passed=true

            # Health endpoint test
            # shellcheck disable=SC2310 # We want to continue even if the test fails
            if validate_api_request "${log_suffix}_health" "http://localhost:${port}${api_prefix}/system/health" "Yes, I'm alive, thanks!"; then
                echo "HEALTH_TEST_PASSED" >> "${result_file}"
            else
                echo "HEALTH_TEST_FAILED" >> "${result_file}"
                all_tests_passed=false
            fi
            
            # Info endpoint test
            # shellcheck disable=SC2310 # We want to continue even if the test fails
            if validate_api_request "${log_suffix}_info" "http://localhost:${port}${api_prefix}/system/info" "system"; then
                echo "INFO_TEST_PASSED" >> "${result_file}"
            else
                echo "INFO_TEST_FAILED" >> "${result_file}"
                all_tests_passed=false
            fi
            
            # Test endpoint test
            # shellcheck disable=SC2310 # We want to continue even if the test fails
            if validate_api_request "${log_suffix}_test" "http://localhost:${port}${api_prefix}/system/test" "client_ip"; then
                echo "TEST_TEST_PASSED" >> "${result_file}"
            else
                echo "TEST_TEST_FAILED" >> "${result_file}"
                all_tests_passed=false
            fi
            
            if [[ "${all_tests_passed}" = true ]]; then
                echo "ALL_API_TESTS_PASSED" >> "${result_file}"
            else
                echo "SOME_API_TESTS_FAILED" >> "${result_file}"
            fi
        else
            echo "SERVER_NOT_READY" >> "${result_file}"
        fi
        
        # Stop the server
        if ps -p "${hydrogen_pid}" > /dev/null 2>&1; then
            kill -SIGINT "${hydrogen_pid}" 2>/dev/null || true
            # Wait for graceful shutdown
            local shutdown_start
            shutdown_start=${SECONDS}
            while ps -p "${hydrogen_pid}" > /dev/null 2>&1; do
                if [[ $((SECONDS - shutdown_start)) -ge "${SHUTDOWN_TIMEOUT}" ]]; then
                    kill -9 "${hydrogen_pid}" 2>/dev/null || true
                    break
                fi
                sleep 0.05
            done
        fi
        
        echo "TEST_COMPLETE" >> "${result_file}"
    else
        echo "STARTUP_FAILED" >> "${result_file}"
        echo "TEST_FAILED" >> "${result_file}"
        kill -9 "${hydrogen_pid}" 2>/dev/null || true
    fi
}

# Function to analyze results from parallel API prefix test execution
analyze_api_prefix_test_results() {
    local test_name="$1"
    local log_suffix="$2"
    local api_prefix="$3"
    local description="$4"
    local result_file="${LOG_PREFIX}test_${TEST_NUMBER}_${TIMESTAMP}_${log_suffix}.result"
    local subtest_counter
    
    if [[ ! -f "${result_file}" ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "No result file found for ${test_name}"
        return 1
    fi
    
    # Check startup
    if ! "${GREP}" -q "STARTUP_SUCCESS" "${result_file}" 2>/dev/null; then
        print_result "${TEST_NUMBER}" "${subtest_counter}" 1 "Failed to start Hydrogen for ${description} test"
        return 1
    fi
    
    # Check server readiness
    if ! "${GREP}" -q "SERVER_READY" "${result_file}" 2>/dev/null; then
        print_result "${TEST_NUMBER}" "${subtest_counter}" 1 "Server not ready for ${description} test"
        return 1
    fi
    
    # Check individual API tests
    local health_passed=false
    local info_passed=false
    local test_passed=false
    
    if "${GREP}" -q "HEALTH_TEST_PASSED" "${result_file}" 2>/dev/null; then
        health_passed=true
    fi
    
    if "${GREP}" -q "INFO_TEST_PASSED" "${result_file}" 2>/dev/null; then
        info_passed=true
    fi
    
    if "${GREP}" -q "TEST_TEST_PASSED" "${result_file}" 2>/dev/null; then
        test_passed=true
    fi
    
    # Return results via global variables for detailed reporting
    HEALTH_TEST_RESULT=${health_passed}
    INFO_TEST_RESULT=${info_passed}
    TEST_TEST_RESULT=${test_passed}
    
    # Return success only if all tests passed
    if "${GREP}" -q "ALL_API_TESTS_PASSED" "${result_file}" 2>/dev/null; then
        return 0
    else
        return 1
    fi
}

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Locate Hydrogen Binary"

HYDROGEN_BIN=''
HYDROGEN_BIN_BASE=''
# shellcheck disable=SC2310 # We want to continue even if the test fails
if find_hydrogen_binary "${PROJECT_DIR}"; then
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Using Hydrogen binary: ${HYDROGEN_BIN_BASE}"
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Hydrogen binary found and validated"
else
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Failed to find Hydrogen binary"
    EXIT_CODE=1
fi


# Validate both configuration files
config_valid=true
for test_config in "${!API_TEST_CONFIGS[@]}"; do
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Validate Configuration File for ${test_config}"

    # Parse test configuration
    IFS=':' read -r config_file log_suffix api_prefix description <<< "${API_TEST_CONFIGS[${test_config}]}"
    
    # shellcheck disable=SC2310 # We want to continue even if the test fails
    if validate_config_file "${config_file}"; then
        port=$(get_webserver_port "${config_file}")
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${description} configuration will use port: ${port}"
    else
        config_valid=false
    fi
done

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Validate Configuration Files"
if [[ "${config_valid}" = true ]]; then
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "All configuration files validated successfully"
else
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Configuration file validation failed"
    EXIT_CODE=1
fi

# Only proceed with API tests if binary and configs are available
if [[ "${EXIT_CODE}" -eq 0 ]]; then
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Running API prefix tests in parallel"
    
    # Start all API prefix tests in parallel with job limiting
    for test_config in "${!API_TEST_CONFIGS[@]}"; do
        # shellcheck disable=SC2312 # Job control with wc -l is standard practice
        while (( $(jobs -r | wc -l) >= CORES )); do
            wait -n  # Wait for any job to finish
        done
        
        # Parse test configuration
        IFS=':' read -r config_file log_suffix api_prefix description <<< "${API_TEST_CONFIGS[${test_config}]}"
        
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Starting parallel test: ${test_config} (${description})"
        
        # Run parallel API prefix test in background
        run_api_prefix_test_parallel "${test_config}" "${config_file}" "${log_suffix}" "${api_prefix}" "${description}" &
        PARALLEL_PIDS+=($!)
    done
    
    # Wait for all parallel tests to complete
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Waiting for ${#API_TEST_CONFIGS[@]} parallel API prefix tests to complete"
    for pid in "${PARALLEL_PIDS[@]}"; do
        wait "${pid}"
    done
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "All parallel tests completed - Analyzing results"
    
    # Process results sequentially for clean output
    for test_config in "${!API_TEST_CONFIGS[@]}"; do
        # Parse test configuration
        IFS=':' read -r config_file log_suffix api_prefix description <<< "${API_TEST_CONFIGS[${test_config}]}"
        
        # print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "API Prefix Test: ${description} (${api_prefix})"
        server_log="${LOGS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}_${log_suffix}.log"
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${test_config} Server Log: ..${server_log}" 

        # shellcheck disable=SC2310 # We want to continue even if the test fails
        if analyze_api_prefix_test_results "${test_config}" "${log_suffix}" "${api_prefix}" "${description}"; then
            # Test individual endpoint results for detailed feedback
            print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Health Endpoint - ${description}"
            if [[ "${HEALTH_TEST_RESULT}" = true ]]; then
                print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Health endpoint test passed"
            else
                print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Health endpoint test failed"
                EXIT_CODE=1
            fi
            
            print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Info Endpoint - ${description}"
            if [[ "${INFO_TEST_RESULT}" = true ]]; then
                print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Info endpoint test passed"
            else
                print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Info endpoint test failed"
                EXIT_CODE=1
            fi
            
            print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Test Endpoint - ${description}"
            if [[ "${TEST_TEST_RESULT}" = true ]]; then
                print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Test endpoint test passed"
            else
                print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Test endpoint test failed"
                EXIT_CODE=1
            fi
            
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}: All API endpoint tests passed"
        else
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${description} test failed"
            EXIT_CODE=1
        fi
    done
    
    # Print summary
    successful_configs=0
    for test_config in "${!API_TEST_CONFIGS[@]}"; do
        IFS=':' read -r config_file log_suffix api_prefix description <<< "${API_TEST_CONFIGS[${test_config}]}"
        result_file="${LOG_PREFIX}test_${TEST_NUMBER}_${TIMESTAMP}_${log_suffix}.result"
        if [[ -f "${result_file}" ]] && "${GREP}" -q "ALL_API_TESTS_PASSED" "${result_file}" 2>/dev/null; then
            successful_configs=$(( successful_configs + 1 ))
        fi
    done
    
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Summary: ${successful_configs}/${#API_TEST_CONFIGS[@]} API prefix configurations passed all tests"
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Parallel execution completed - SO_REUSEADDR allows immediate port reuse"
else
    # Skip API tests if prerequisites failed
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Skipping API prefix tests due to prerequisite failures"
    EXIT_CODE=1
fi

# Print test completion summary
print_test_completion "${TEST_NAME}" "${TEST_ABBR}" "${TEST_NUMBER}" "${TEST_VERSION}"

# Return status code if sourced, exit if run standalone
${ORCHESTRATION:-false} && return "${EXIT_CODE}" || exit "${EXIT_CODE}"
