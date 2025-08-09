#!/bin/bash

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

# Test Configuration
TEST_NAME="API Prefix"
TEST_ABBR="PRE"
TEST_NUMBER="30"
TEST_VERSION="7.0.0"

# shellcheck source=tests/lib/framework.sh # Reference framework directly
[[ -n "${FRAMEWORK_GUARD}" ]] || source "$(dirname "${BASH_SOURCE[0]}")/lib/framework.sh"
setup_test_environment

# Parallel execution configuration
MAX_JOBS=$(nproc 2>/dev/null || echo 2)  # Use 2 for this test since we have exactly 2 configurations
declare -a PARALLEL_PIDS
declare -A API_TEST_CONFIGS

# API test configuration - format: "config_file:log_suffix:api_prefix:description"
API_TEST_CONFIGS=(
    ["DEFAULT"]="${SCRIPT_DIR}/configs/hydrogen_test_30_api_1.json:one:/api:First API Prefix"
    ["CUSTOM"]="${SCRIPT_DIR}/configs/hydrogen_test_30_api_2.json:two:/myapi:Second API Prefix"
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

    print_command "curl -s --max-time 10 --compressed \"${url}\""
    
    # Retry logic for API readiness (especially important in parallel execution)
    local max_attempts=25
    local attempt=1
    local curl_exit_code=0
        
    while [[ "${attempt}" -le "${max_attempts}" ]]; do
        if [[ "${attempt}" -gt 1 ]]; then
            print_message "API request attempt ${attempt} of ${max_attempts} (waiting for API subsystem initialization)..."
            #sleep 0.05 
        fi
        
        # Run curl and capture both exit code and any error output
        curl -s --max-time 10 --compressed "${url}" > "${response_file}" 2>"${error_file}"
        curl_exit_code=$?
        
        if [[ "${curl_exit_code}" -eq 0 ]]; then
            # Verify the file actually exists and has content
            if [[ -f "${response_file}" ]] && [[ -s "${response_file}" ]]; then
                # Check if we got a 404 or other error response
                if grep -q "404 Not Found" "${response_file}" || grep -q "<html>" "${response_file}"; then
                    if [[ "${attempt}" -eq "${max_attempts}" ]]; then
                        print_message "API endpoint still not ready after ${max_attempts} attempts"
                        print_result 1 "API endpoint returned 404 or HTML error page"
                        print_message "Response content:"
                        print_output "$(cat "${response_file}" || true)"
                        return 1
                    else
                        print_message "API endpoint not ready yet (got 404/HTML), retrying..."
                        ((attempt++))
                        continue
                    fi
                fi
                
                print_message "Request successful, checking response content"
                
                # Validate that the response contains expected fields
                if grep -q "${expected_field}" "${response_file}"; then
                    if [[ "${attempt}" -gt 1 ]]; then
                        print_result 0 "Response contains expected field: ${expected_field} (succeeded on attempt ${attempt})"
                    else
                        print_result 0 "Response contains expected field: ${expected_field}"
                    fi
                    return 0
                else
                    print_result 1 "Response doesn't contain expected field: ${expected_field}"
                    print_message "Response content:"
                    print_output "$(cat "${response_file}" || true)"
                    return 1
                fi
            else
                print_result 1 "Response file was not created or is empty: ${response_file}"
                return 1
            fi
        else
            if [[ "${attempt}" -eq "${max_attempts}" ]]; then
                print_result 1 "Failed to connect to server (curl exit code: ${curl_exit_code})"
                # Show curl error if available
                if [[ -f "${error_file}" ]] && [[ -s "${error_file}" ]]; then
                    print_message "Curl error output:"
                    print_output "$(cat "${error_file}" || true)"
                fi
                # Clean up error file
                rm -f "${error_file}" 2>/dev/null
                return 1
            else
                print_message "Connection failed on attempt ${attempt}, retrying..."
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
    local port
    port=$(get_webserver_port "${config_file}")
    
    # Clear result file
    true > "${result_file}"
    
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
        
        if grep -q "Application started" "${log_file}" 2>/dev/null; then
            startup_success=true
            break
        fi
        sleep 0.1
    done
    
    if [[ "${startup_success}" = true ]]; then
        echo "STARTUP_SUCCESS" >> "${result_file}"
        
        # Wait for server to be ready
        if wait_for_server_ready "http://localhost:${port}"; then
            echo "SERVER_READY" >> "${result_file}"
            
            # Test all API endpoints
            local all_tests_passed=true
            
            # Health endpoint test
            if validate_api_request "${log_suffix}_health" "http://localhost:${port}${api_prefix}/system/health" "Yes, I'm alive, thanks!"; then
                echo "HEALTH_TEST_PASSED" >> "${result_file}"
            else
                echo "HEALTH_TEST_FAILED" >> "${result_file}"
                all_tests_passed=false
            fi
            
            # Info endpoint test
            if validate_api_request "${log_suffix}_info" "http://localhost:${port}${api_prefix}/system/info" "system"; then
                echo "INFO_TEST_PASSED" >> "${result_file}"
            else
                echo "INFO_TEST_FAILED" >> "${result_file}"
                all_tests_passed=false
            fi
            
            # Test endpoint test
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
                sleep 0.1
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
    
    if [[ ! -f "${result_file}" ]]; then
        print_result 1 "No result file found for ${test_name}"
        return 1
    fi
    
    # Check startup
    if ! grep -q "STARTUP_SUCCESS" "${result_file}" 2>/dev/null; then
        print_result 1 "Failed to start Hydrogen for ${description} test"
        return 1
    fi
    
    # Check server readiness
    if ! grep -q "SERVER_READY" "${result_file}" 2>/dev/null; then
        print_result 1 "Server not ready for ${description} test"
        return 1
    fi
    
    # Check individual API tests
    local health_passed=false
    local info_passed=false
    local test_passed=false
    
    if grep -q "HEALTH_TEST_PASSED" "${result_file}" 2>/dev/null; then
        health_passed=true
    fi
    
    if grep -q "INFO_TEST_PASSED" "${result_file}" 2>/dev/null; then
        info_passed=true
    fi
    
    if grep -q "TEST_TEST_PASSED" "${result_file}" 2>/dev/null; then
        test_passed=true
    fi
    
    # Return results via global variables for detailed reporting
    HEALTH_TEST_RESULT=${health_passed}
    INFO_TEST_RESULT=${info_passed}
    TEST_TEST_RESULT=${test_passed}
    
    # Return success only if all tests passed
    if grep -q "ALL_API_TESTS_PASSED" "${result_file}" 2>/dev/null; then
        return 0
    else
        return 1
    fi
}

print_subtest "Locate Hydrogen Binary"

HYDROGEN_BIN=''
HYDROGEN_BIN_BASE=''
if find_hydrogen_binary "${PROJECT_DIR}"; then
    print_message "Using Hydrogen binary: ${HYDROGEN_BIN_BASE}"
    print_result 0 "Hydrogen binary found and validated"
    ((PASS_COUNT++))
else
    print_result 1 "Failed to find Hydrogen binary"
    EXIT_CODE=1
fi

print_subtest "Validate Configuration Files"

# Validate both configuration files
config_valid=true
for test_config in "${!API_TEST_CONFIGS[@]}"; do
    # Parse test configuration
    IFS=':' read -r config_file log_suffix api_prefix description <<< "${API_TEST_CONFIGS[${test_config}]}"
    
    if validate_config_file "${config_file}"; then
        port=$(get_webserver_port "${config_file}")
        print_message "${description} configuration will use port: ${port}"
    else
        config_valid=false
    fi
done

if [[ "${config_valid}" = true ]]; then
    print_result 0 "All configuration files validated successfully"
    ((PASS_COUNT++))
else
    print_result 1 "Configuration file validation failed"
    EXIT_CODE=1
fi

# Only proceed with API tests if binary and configs are available
if [[ "${EXIT_CODE}" -eq 0 ]]; then
    # Ensure clean state
    print_message "Ensuring no existing Hydrogen processes are running..."
    pkill -f "hydrogen.*json" 2>/dev/null || true
    
    print_message "Running API prefix tests in parallel for faster execution..."
    
    # Start all API prefix tests in parallel with job limiting
    for test_config in "${!API_TEST_CONFIGS[@]}"; do
        # shellcheck disable=SC2312 # Job control with wc -l is standard practice
        while (( $(jobs -r | wc -l) >= MAX_JOBS )); do
            wait -n  # Wait for any job to finish
        done
        
        # Parse test configuration
        IFS=':' read -r config_file log_suffix api_prefix description <<< "${API_TEST_CONFIGS[${test_config}]}"
        
        print_message "Starting parallel test: ${test_config} (${description})"
        
        # Run parallel API prefix test in background
        run_api_prefix_test_parallel "${test_config}" "${config_file}" "${log_suffix}" "${api_prefix}" "${description}" &
        PARALLEL_PIDS+=($!)
    done
    
    # Wait for all parallel tests to complete
    print_message "Waiting for all ${#API_TEST_CONFIGS[@]} parallel API prefix tests to complete..."
    for pid in "${PARALLEL_PIDS[@]}"; do
        wait "${pid}"
    done
    print_message "All parallel tests completed, analyzing results..."
    
    # Process results sequentially for clean output
    for test_config in "${!API_TEST_CONFIGS[@]}"; do
        # Parse test configuration
        IFS=':' read -r config_file log_suffix api_prefix description <<< "${API_TEST_CONFIGS[${test_config}]}"
        
        print_subtest "API Prefix Test: ${description} (${api_prefix})"
        
        if analyze_api_prefix_test_results "${test_config}" "${log_suffix}" "${api_prefix}" "${description}"; then
            # Test individual endpoint results for detailed feedback
            print_subtest "Health Endpoint - ${description}"
            if [[ "${HEALTH_TEST_RESULT}" = true ]]; then
                print_result 0 "Health endpoint test passed"
                ((PASS_COUNT++))
            else
                print_result 1 "Health endpoint test failed"
                EXIT_CODE=1
            fi
            
            print_subtest "Info Endpoint - ${description}"
            if [[ "${INFO_TEST_RESULT}" = true ]]; then
                print_result 0 "Info endpoint test passed"
                ((PASS_COUNT++))
            else
                print_result 1 "Info endpoint test failed"
                EXIT_CODE=1
            fi
            
            print_subtest "Test Endpoint - ${description}"
            if [[ "${TEST_TEST_RESULT}" = true ]]; then
                print_result 0 "Test endpoint test passed"
                ((PASS_COUNT++))
            else
                print_result 1 "Test endpoint test failed"
                EXIT_CODE=1
            fi
            
            print_message "${description}: All API endpoint tests passed"
        else
            print_result 1 "${description} test failed"
            EXIT_CODE=1
        fi
    done
    
    # Print summary
    successful_configs=0
    for test_config in "${!API_TEST_CONFIGS[@]}"; do
        IFS=':' read -r config_file log_suffix api_prefix description <<< "${API_TEST_CONFIGS[${test_config}]}"
        result_file="${LOG_PREFIX}test_${TEST_NUMBER}_${TIMESTAMP}_${log_suffix}.result"
        if [[ -f "${result_file}" ]] && grep -q "ALL_API_TESTS_PASSED" "${result_file}" 2>/dev/null; then
            ((successful_configs++))
        fi
    done
    
    print_message "Summary: ${successful_configs}/${#API_TEST_CONFIGS[@]} API prefix configurations passed all tests"
    print_message "Parallel execution completed - SO_REUSEADDR allows immediate port reuse"
else
    # Skip API tests if prerequisites failed
    print_result 1 "Skipping API prefix tests due to prerequisite failures"
    EXIT_CODE=1
fi

# Print test completion summary
print_test_completion "${TEST_NAME}" "${TEST_ABBR}" "${TEST_NUMBER}" "${TEST_VERSION}"

# Return status code if sourced, exit if run standalone
${ORCHESTRATION:-false} && return "${EXIT_CODE}" || exit "${EXIT_CODE}"
