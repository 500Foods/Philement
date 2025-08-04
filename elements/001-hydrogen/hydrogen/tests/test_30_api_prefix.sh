#!/bin/bash

# Test: API Prefix
# Tests the API endpoints with different API prefix configurations using immediate restart approach:
# - Default "/api" prefix using standard hydrogen_test_api.json
# - Custom "/myapi" prefix using hydrogen_test_api_prefix.json
# - Uses immediate restart without waiting for TIME_WAIT (SO_REUSEADDR enabled)

# FUNCTIONS
# create_unique_config() 
# validate_api_request()

# CHANGELOG
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
TEST_VERSION="6.0.0"

# shellcheck source=tests/lib/framework.sh # Reference framework directly
[[ -n "${FRAMEWORK_GUARD}" ]] || source "$(dirname "${BASH_SOURCE[0]}")/lib/framework.sh"
setup_test_environment

# Test variables
HYDROGEN_DIR="${PROJECT_DIR}"

# Create unique configuration files for parallel execution
create_unique_config() {
    local base_config="$1"
    local unique_suffix="$2"
    local output_path="$3"
    
    # Read base config and modify paths to use build/tests and unique identifiers
    sed "s|\"./tests/hydrogen_test_api_test_|\"${RESULTS_DIR}/hydrogen_test_api_test_${unique_suffix}_|g; \
         s|\"./logs/hydrogen_test_api_test_|\"${RESULTS_DIR}/hydrogen_test_api_test_${unique_suffix}_|g" \
         "${base_config}" > "${output_path}"
}

# Configuration files with unique paths for parallel execution
BASE_DEFAULT_CONFIG="$(get_config_path "hydrogen_test_api_test_1.json")"
BASE_CUSTOM_CONFIG="$(get_config_path "hydrogen_test_api_test_2.json")"
DEFAULT_CONFIG_PATH="${RESULTS_DIR}/hydrogen_test_api_test_1_${TIMESTAMP}.json"
CUSTOM_CONFIG_PATH="${RESULTS_DIR}/hydrogen_test_api_test_2_${TIMESTAMP}.json"

# Create unique configs to avoid conflicts in parallel execution
create_unique_config "${BASE_DEFAULT_CONFIG}" "1" "${DEFAULT_CONFIG_PATH}"
create_unique_config "${BASE_CUSTOM_CONFIG}" "2" "${CUSTOM_CONFIG_PATH}"

# Function to make a request and validate response with retry logic for API readiness
validate_api_request() {
    local request_name="$1"
    local url="$2"
    local expected_field="$3"
    
    # Generate unique ID once and store it
    local unique_id="${TIMESTAMP}_$${_}${RANDOM}"
    local response_file="${RESULTS_DIR}/response_${request_name}_${unique_id}.json"
    
    print_command "curl -s --max-time 10 --compressed \"${url}\""
    
    # Retry logic for API readiness (especially important in parallel execution)
    local max_attempts=25
    local attempt=1
    local curl_exit_code=0
    local curl_error_file="${RESULTS_DIR}/curl_error_${unique_id}.txt"
        
    while [[ "${attempt}" -le "${max_attempts}" ]]; do
        if [[ "${attempt}" -gt 1 ]]; then
            print_message "API request attempt ${attempt} of ${max_attempts} (waiting for API subsystem initialization)..."
            sleep 0.05 
        fi
        
        # Run curl and capture both exit code and any error output
        curl -s --max-time 10 --compressed "${url}" > "${response_file}" 2>"${curl_error_file}"
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
                        rm -f "${curl_error_file}" 2>/dev/null
                        return 1
                    else
                        print_message "API endpoint not ready yet (got 404/HTML), retrying..."
                        ((attempt++))
                        continue
                    fi
                fi
                
                print_message "Request successful, checking response content"
                
                # Clean up error file if not needed
                rm -f "${curl_error_file}" 2>/dev/null
                
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
                # Show curl error if available
                if [[ -f "${curl_error_file}" ]] && [[ -s "${curl_error_file}" ]]; then
                    print_message "Curl error output:"
                    print_output "$(cat "${curl_error_file}" || true)"
                fi
                # Clean up error file
                rm -f "${curl_error_file}" 2>/dev/null
                return 1
            fi
        else
            if [[ "${attempt}" -eq "${max_attempts}" ]]; then
                print_result 1 "Failed to connect to server (curl exit code: ${curl_exit_code})"
                # Show curl error if available
                if [[ -f "${curl_error_file}" ]] && [[ -s "${curl_error_file}" ]]; then
                    print_message "Curl error output:"
                    print_output "$(cat "${curl_error_file}" || true)"
                fi
                # Clean up error file
                rm -f "${curl_error_file}" 2>/dev/null
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

print_subtest "Locate Hydrogen Binary"

# shellcheck disable=SC2153  # HYDROGEN_BIN is set by find_hydrogen_binary function
if find_hydrogen_binary "${HYDROGEN_DIR}" "HYDROGEN_BIN"; then
    print_result 0 "Hydrogen binary found: $(basename "${HYDROGEN_BIN}")"
    ((PASS_COUNT++))
else
    print_result 1 "Hydrogen binary not found"
    EXIT_CODE=1
fi

print_subtest "Validate Configuration Files"

if [[ -f "${DEFAULT_CONFIG_PATH}" ]] && [[ -f "${CUSTOM_CONFIG_PATH}" ]]; then
    print_result 0 "Both configuration files found"
    ((PASS_COUNT++))
    
    # Extract and display ports
    DEFAULT_PORT=$(get_webserver_port "${DEFAULT_CONFIG_PATH}")
    CUSTOM_PORT=$(get_webserver_port "${CUSTOM_CONFIG_PATH}")
    print_message "Default configuration will use port: ${DEFAULT_PORT}"
    print_message "Custom configuration will use port: ${CUSTOM_PORT}"
else
    print_result 1 "One or more configuration files not found"
    EXIT_CODE=1
fi

# Only proceed with API tests if binary and configs are available
if [[ "${EXIT_CODE}" -eq 0 ]]; then
    # Ensure clean state
    print_message "Ensuring no existing Hydrogen processes are running..."
    pkill -f "hydrogen.*json" 2>/dev/null || true
    
    print_message "Testing API prefix functionality with immediate restart approach"
    print_message "SO_REUSEADDR is enabled - no need to wait for TIME_WAIT"
    
    # Global variables for server management
    HYDROGEN_PID=""
    DEFAULT_PORT=$(get_webserver_port "${DEFAULT_CONFIG_PATH}")
    CUSTOM_PORT=$(get_webserver_port "${CUSTOM_CONFIG_PATH}")
    
    print_subtest "Start Server with Default API Prefix (/api)"

    DEFAULT_LOG="${RESULTS_DIR}/api_prefixes_default_server_${TIMESTAMP}_${RANDOM}.log"
    if start_hydrogen_with_pid "${DEFAULT_CONFIG_PATH}" "${DEFAULT_LOG}" 15 "${HYDROGEN_BIN}" "HYDROGEN_PID" && [[ -n "${HYDROGEN_PID}" ]]; then
        print_result 0 "Server started successfully with PID: ${HYDROGEN_PID}"
        ((PASS_COUNT++))
    else
        print_result 1 "Failed to start server with default configuration"
        EXIT_CODE=1
    fi
    
    print_subtest "Test Default API Health Endpoint"

    if [[ -n "${HYDROGEN_PID}" ]] && ps -p "${HYDROGEN_PID}" > /dev/null 2>&1; then
        if wait_for_server_ready "http://localhost:${DEFAULT_PORT}"; then
            if validate_api_request "default_health" "http://localhost:${DEFAULT_PORT}/api/system/health" "Yes, I'm alive, thanks!"; then
                ((PASS_COUNT++))
            else
                EXIT_CODE=1
            fi
        else
            print_result 1 "Server not ready for health endpoint test"
            EXIT_CODE=1
        fi
    else
        print_result 1 "Server not running for health endpoint test"
        EXIT_CODE=1
    fi
    
    print_subtest "Test Default API Info Endpoint"

    if [[ -n "${HYDROGEN_PID}" ]] && ps -p "${HYDROGEN_PID}" > /dev/null 2>&1; then
        if validate_api_request "default_info" "http://localhost:${DEFAULT_PORT}/api/system/info" "system"; then
            ((PASS_COUNT++))
        else
            EXIT_CODE=1
        fi
    else
        print_result 1 "Server not running for info endpoint test"
        EXIT_CODE=1
    fi
    
    print_subtest "Test Default API Test Endpoint"

    if [[ -n "${HYDROGEN_PID}" ]] && ps -p "${HYDROGEN_PID}" > /dev/null 2>&1; then
        if validate_api_request "default_test" "http://localhost:${DEFAULT_PORT}/api/system/test" "client_ip"; then
            ((PASS_COUNT++))
        else
            EXIT_CODE=1
        fi
    else
        print_result 1 "Server not running for test endpoint test"
        EXIT_CODE=1
    fi
    
    # Stop the default server
    if [[ -n "${HYDROGEN_PID}" ]]; then
        print_message "Stopping default server..."
        if stop_hydrogen "${HYDROGEN_PID}" "${DEFAULT_LOG}" 10 5 "${RESULTS_DIR}"; then
            print_message "Default server stopped successfully"
        else
            print_warning "Default server shutdown had issues"
        fi
        check_time_wait_sockets "${DEFAULT_PORT}"
        HYDROGEN_PID=""
    fi
    
    # Start second test immediately
    print_message "Starting second test immediately (testing SO_REUSEADDR)..."
    
    print_subtest "Start Server with Custom API Prefix (/myapi)"

    CUSTOM_LOG="${RESULTS_DIR}/api_prefixes_custom_server_${TIMESTAMP}_${RANDOM}.log"
    if start_hydrogen_with_pid "${CUSTOM_CONFIG_PATH}" "${CUSTOM_LOG}" 15 "${HYDROGEN_BIN}" "HYDROGEN_PID" && [[ -n "${HYDROGEN_PID}" ]]; then
        print_result 0 "Server started successfully with PID: ${HYDROGEN_PID}"
        ((PASS_COUNT++))
    else
        print_result 1 "Failed to start server with custom configuration"
        EXIT_CODE=1
    fi
    
    print_subtest "Test Custom API Health Endpoint"

    if [[ -n "${HYDROGEN_PID}" ]] && ps -p "${HYDROGEN_PID}" > /dev/null 2>&1; then
        if wait_for_server_ready "http://localhost:${CUSTOM_PORT}"; then
            if validate_api_request "custom_health" "http://localhost:${CUSTOM_PORT}/myapi/system/health" "Yes, I'm alive, thanks!"; then
                ((PASS_COUNT++))
            else
                EXIT_CODE=1
            fi
        else
            print_result 1 "Server not ready for health endpoint test"
            EXIT_CODE=1
        fi
    else
        print_result 1 "Server not running for health endpoint test"
        EXIT_CODE=1
    fi
    
    print_subtest "Test Custom API Info Endpoint"

    if [[ -n "${HYDROGEN_PID}" ]] && ps -p "${HYDROGEN_PID}" > /dev/null 2>&1; then
        if validate_api_request "custom_info" "http://localhost:${CUSTOM_PORT}/myapi/system/info" "system"; then
            ((PASS_COUNT++))
        else
            EXIT_CODE=1
        fi
    else
        print_result 1 "Server not running for info endpoint test"
        EXIT_CODE=1
    fi
    
    print_subtest "Test Custom API Test Endpoint"

    if [[ -n "${HYDROGEN_PID}" ]] && ps -p "${HYDROGEN_PID}" > /dev/null 2>&1; then
        if validate_api_request "custom_test" "http://localhost:${CUSTOM_PORT}/myapi/system/test" "client_ip"; then
            ((PASS_COUNT++))
        else
            EXIT_CODE=1
        fi
    else
        print_result 1 "Server not running for test endpoint test"
        EXIT_CODE=1
    fi
    
    # Stop the custom server
    if [[ -n "${HYDROGEN_PID}" ]]; then
        print_message "Stopping custom server..."
        if stop_hydrogen "${HYDROGEN_PID}" "${CUSTOM_LOG}" 10 5 "${RESULTS_DIR}"; then
            print_message "Custom server stopped successfully"
        else
            print_warning "Custom server shutdown had issues"
        fi
        check_time_wait_sockets "${CUSTOM_PORT}"
        HYDROGEN_PID=""
    fi
    
    print_message "Immediate restart successful - SO_REUSEADDR applied successfully"
else
    # Skip API tests if prerequisites failed
    print_result 1 "Skipping API prefix tests due to prerequisite failures"
    # Account for skipped subtests
    for i in {3..10}; do
        print_result 1 "Subtest ${i} skipped due to prerequisite failures"
    done
    EXIT_CODE=1
fi

# Clean up response files and temporary configs
rm -f "${RESULTS_DIR}"/response_*.json
rm -f "${DEFAULT_CONFIG_PATH}" "${CUSTOM_CONFIG_PATH}"

# Print test completion summary
print_test_completion "${TEST_NAME}" "${TEST_ABBR}" "${TEST_NUMBER}" "${TEST_VERSION}"

# Return status code if sourced, exit if run standalone
${ORCHESTRATION:-false} && return "${EXIT_CODE}" || exit "${EXIT_CODE}"
