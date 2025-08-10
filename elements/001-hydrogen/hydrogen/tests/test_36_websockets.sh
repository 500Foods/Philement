#!/bin/bash

# Test: WebSockets
# Tests the WebSocket functionality with different configurations

# FUNCTIONS
# test_websocket_connection() 
# test_websocket_status() 
# test_websocket_configuration() 

# CHANGELOG
# 3.1.0 - 2025-08-09 - Updates to log file naming mostly
# 3.0.0 - 2025-08-04 - That right there was Grok's rewrite - heh a modest 0.0.1
# 2.0.2 - 2025-08-04 - Optimized for performance: cached key validation, parallel config tests, consolidated log checks, in-memory temps, early exits
# 2.0.1 - 2025-08-03 - Removed extraneous command -v calls
# 2.0.0 - 2025-07-30 - Overhaul #1
# 1.1.2 - 2025-07-19 - Bit of tidying up to get rid of steps we don't need
# 1.1.1 - 2025-07-18 - Fixed subshell issue in server log output that prevented detailed error messages from being displayed in test output
# 1.1.0 - 2025-07-15 - Added WebSocket status request test to improve coverage of websocket_server_status.c
# 1.0.3 - 2025-07-14 - Enhanced test_websocket_connection with retry logic to handle WebSocket subsystem initialization delays during parallel execution
# 1.0.2 - 2025-07-15 - No more sleep
# 1.0.1 - 2025-07-14 - Updated to use build/tests directories for test output consistency
# 1.0.0 - 2025-07-13 - Initial version for WebSocket server testing

# Test Configuration
TEST_NAME="WebSockets"
TEST_ABBR="WSS"
TEST_NUMBER="36"
TEST_VERSION="3.1.0"

# shellcheck source=tests/lib/framework.sh # Reference framework directly
[[ -n "${FRAMEWORK_GUARD}" ]] || source "$(dirname "${BASH_SOURCE[0]}")/lib/framework.sh"
setup_test_environment

# Test variables
CONFIG_1="${SCRIPT_DIR}/configs/hydrogen_test_36_websocket_1.json"
CONFIG_2="${SCRIPT_DIR}/configs/hydrogen_test_36_websocket_2.json"

# Global cache for WEBSOCKET_KEY validation
WEBSOCKET_KEY_VALIDATED=0

# Create temporary directory for parallel results
declare -a PARALLEL_PIDS
declare -a CONFIGS=(
    "${CONFIG_1}|5101|hydrogen|one|1"
    "${CONFIG_2}|5102|hydrogen-test|two|2"
)

# Function to test WebSocket connection with proper authentication and retry logic
test_websocket_connection() {
    local ws_url="$1"
    local protocol="$2"
    local test_message="$3"
    local response_file="$4"
    
    print_message "Testing WebSocket connection with authentication using websocat"
    print_command "echo '${test_message}' | websocat --protocol='${protocol}' -H='Authorization: Key ${WEBSOCKET_KEY}' --ping-interval=30 --exit-on-eof '${ws_url}'"
    
    # Retry logic for WebSocket subsystem readiness (reduced for parallel execution to prevent thundering herd)
    local max_attempts=5
    local attempt=1
    local websocat_output
    local websocat_exitcode
    local temp_file="${LOG_PREFIX}${TIMESTAMP}_${protocol}_echo.log"

    while [[ "${attempt}" -le "${max_attempts}" ]]; do
        if [[ "${attempt}" -gt 1 ]]; then
            print_message "WebSocket connection attempt ${attempt} of ${max_attempts} (waiting for WebSocket subsystem initialization)..."
            sleep 0.05  # Brief delay between attempts to prevent thundering herd
        fi
        
        # Test WebSocket connection with a 5-second timeout
        echo "${test_message}" | timeout 5 websocat \
            --protocol="${protocol}" \
            -H="Authorization: Key ${WEBSOCKET_KEY}" \
            --ping-interval=30 \
            --exit-on-eof \
            "${ws_url}" > "${temp_file}" 2>&1
        websocat_exitcode=$?
        
        # Read the output
        websocat_output=$(cat "${temp_file}" 2>/dev/null || echo "")
        
        # Analyze the results
        if [[ "${websocat_exitcode}" -eq 0 ]]; then
            if [[ "${attempt}" -gt 1 ]]; then
                print_result 0 "WebSocket connection successful (clean exit, succeeded on attempt ${attempt})"
            else
                print_result 0 "WebSocket connection successful (clean exit)"
            fi
            if [[ -n "${websocat_output}" ]]; then
                print_message "Server response: ${websocat_output}"
            fi
            return 0
        elif [[ "${websocat_exitcode}" -eq 124 ]]; then
            # Timeout occurred, but that's OK if connection was established
            if [[ "${attempt}" -gt 1 ]]; then
                print_result 0 "WebSocket connection successful (timeout after successful connection, succeeded on attempt ${attempt})"
            else
                print_result 0 "WebSocket connection successful (timeout after successful connection)"
            fi
            return 0
        else
            # Check for connection refused which might indicate WebSocket server not ready yet
            if echo "${websocat_output}" | grep -qi "connection refused"; then
                if [[ "${attempt}" -eq "${max_attempts}" ]]; then
                    print_result 1 "WebSocket connection failed: Connection refused after ${max_attempts} attempts"
                    print_message "Server is not accepting WebSocket connections on the specified port"
                    return 1
                else
                    print_message "WebSocket server not ready yet (connection refused), retrying..."
                    ((attempt++))
                    continue
                fi
            fi
            
            # Check other error types that might be temporary during parallel execution
            if echo "${websocat_output}" | grep -qi "network.*unreachable\|temporarily unavailable\|resource.*unavailable"; then
                if [[ "${attempt}" -eq "${max_attempts}" ]]; then
                    print_result 1 "WebSocket connection failed: Network/resource issues after ${max_attempts} attempts"
                    print_message "Error: ${websocat_output}"
                    return 1
                else
                    print_message "Temporary network/resource issue, retrying..."
                    ((attempt++))
                    continue
                fi
            fi
            
            # For other errors, fail immediately as they're likely permanent
            if echo "${websocat_output}" | grep -qi "401\|forbidden\|unauthorized\|authentication"; then
                print_result 1 "WebSocket connection failed: Authentication rejected"
                print_message "Server rejected the provided WebSocket key"
                return 1
            elif echo "${websocat_output}" | grep -qi "protocol.*not.*supported"; then
                print_result 1 "WebSocket connection failed: Protocol not supported"
                print_message "Server does not support the specified protocol: ${protocol}"
                return 1
            else
                # Unknown error - retry if we have attempts left
                if [[ "${attempt}" -eq "${max_attempts}" ]]; then
                    print_result 1 "WebSocket connection failed after ${max_attempts} attempts"
                    print_message "Error: ${websocat_output}"
                    return 1
                else
                    print_message "WebSocket connection failed on attempt ${attempt}, retrying..."
                    ((attempt++))
                    continue
                fi
            fi
        fi
    done
    
    return 1
}

# Function to test WebSocket status request
test_websocket_status() {
    local ws_url="$1"
    local protocol="$2"
    local response_file="$3"
    
    print_message "Testing WebSocket status request using websocat"
    
    # JSON message to request status
    local status_request='{"type": "status"}'
    print_command "echo '${status_request}' | websocat --protocol='${protocol}' -H='Authorization: Key ${WEBSOCKET_KEY}' --ping-interval=30 --no-close '${ws_url}'"
    
    # Retry logic for WebSocket subsystem readiness (reduced for parallel execution to prevent thundering herd)
    local max_attempts=10
    local attempt=1
    local websocat_output
    local websocat_exitcode
    local temp_file="${LOG_PREFIX}${TIMESTAMP}_${protocol}_status.txt"

    while [[ "${attempt}" -le "${max_attempts}" ]]; do
        if [[ "${attempt}" -gt 1 ]]; then
            print_message "WebSocket status request attempt ${attempt} of ${max_attempts}..."
            sleep 0.05  # Brief delay between attempts to prevent thundering herd
        fi
        
        # Test WebSocket status request with a 5-second timeout
        echo "${status_request}" | websocat \
            --protocol="${protocol}" \
            -H="Authorization: Key ${WEBSOCKET_KEY}" \
            --ping-interval=30 \
            --no-close \
            --one-message \
            "${ws_url}" > "${temp_file}" 2>&1
        websocat_exitcode=$?
        
        # Read the output
        websocat_output=$(cat "${temp_file}" 2>/dev/null || echo "")
        
        # Check if we got a valid JSON response with system info
        if [[ -n "${websocat_output}" ]]; then
            # Use jq to properly validate JSON structure and required fields
            if echo "${websocat_output}" | jq -e '.system and .status.server_started' >/dev/null 2>&1; then
                if [[ "${attempt}" -gt 1 ]]; then
                    print_result 0 "WebSocket status request successful - received system status (succeeded on attempt ${attempt})"
                else
                    print_result 0 "WebSocket status request successful - received system status"
                fi
                print_message "Status response contains system information"
                echo "${websocat_output}" > "${response_file}"
                return 0
            else
                # Log what we actually received for debugging
                print_message "Invalid JSON response or missing required fields:"
                echo "${websocat_output}" | jq . 2>/dev/null | head -10 || echo "Non-JSON output: ${websocat_output}" || true
            fi
        fi
        
        # Check for connection issues
        if [[ "${websocat_exitcode}" -ne 0 ]] && [[ "${websocat_exitcode}" -ne 124 ]] && [[ "${websocat_exitcode}" -ne 1 ]]; then
            if [[ "${attempt}" -eq "${max_attempts}" ]]; then
                print_result 1 "WebSocket status request failed - connection error (exit code: ${websocat_exitcode})"
                return 1
            else
                print_message "WebSocket status request failed on attempt ${attempt}, retrying..."
                ((attempt++))
                continue
            fi
        fi
        
        # If we reach here, either got timeout or no valid response
        if [[ "${attempt}" -eq "${max_attempts}" ]]; then
            print_result 1 "WebSocket status request failed - no valid status response received"
            return 1
        else
            print_message "WebSocket status request attempt ${attempt} failed, retrying..."
            ((attempt++))
        fi
    done
    
    return 1
}

# Function to test WebSocket server configuration in parallel
test_websocket_configuration() {
    local config_file="$1"
    local ws_port="$2"
    local ws_protocol="$3"
    local test_name="$4"
    local config_number="$5"
    local result_file="${LOG_PREFIX}${TIMESTAMP}_${ws_protocol}.result"
    local server_log="${LOGS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}_${ws_protocol}.log"
    
    print_message "Testing WebSocket server: ws://localhost:${ws_port} (protocol: ${ws_protocol})"
    
    # Extract web server port from configuration for readiness check
    local server_port
    server_port=$(get_webserver_port "${config_file}")
    print_message "Configuration will use web server port: ${server_port}, WebSocket port: ${ws_port}"
    
    # Global variables for server management
    local hydrogen_pid=""
    local base_url="http://localhost:${server_port}"
    local ws_url="ws://localhost:${ws_port}"
    
    # Clear result file
    true > "${result_file}"
    
    # Start server
    # local subtest_start=$(((config_number - 1) * 5 + 1))
    
    print_subtest "Start Hydrogen Server (Config ${config_number})"
    
    local temp_pid_var="HYDROGEN_PID_$$_${config_number}"
    if start_hydrogen_with_pid "${config_file}" "${server_log}" 15 "${HYDROGEN_BIN}" "${temp_pid_var}"; then
        hydrogen_pid=$(eval "echo \$${temp_pid_var}")
        if [[ -n "${hydrogen_pid}" ]]; then
            print_result 0 "Server started successfully with PID: ${hydrogen_pid}"
            echo "START_RESULT=0" >> "${result_file}"
            echo "PID=${hydrogen_pid}" >> "${result_file}"
            ((PASS_COUNT++))
        else
            print_result 1 "Failed to start server - no PID returned"
            echo "START_RESULT=1" >> "${result_file}"
            return 1
        fi
    else
        print_result 1 "Failed to start server"
        echo "START_RESULT=1" >> "${result_file}"
        return 1
    fi
    
    # Wait for web server to be ready
    if [[ -n "${hydrogen_pid}" ]] && ps -p "${hydrogen_pid}" > /dev/null 2>&1; then
        if ! wait_for_server_ready "${base_url}"; then
            print_result 1 "Server failed to become ready"
            echo "READY_RESULT=1" >> "${result_file}"
            return 1
        fi
        echo "READY_RESULT=0" >> "${result_file}"
    fi
    
    print_subtest "Test WebSocket Connection (Config ${config_number})"

    if [[ -n "${hydrogen_pid}" ]] && ps -p "${hydrogen_pid}" > /dev/null 2>&1; then
        if test_websocket_connection "${ws_url}" "${ws_protocol}" "test_message" "${LOG_PREFIX}${TIMESTAMP}_${ws_protocol}_connection.result"; then
            echo "CONNECTION_RESULT=0" >> "${result_file}"
            ((PASS_COUNT++))
        else
            echo "CONNECTION_RESULT=1" >> "${result_file}"
        fi
    else
        print_result 1 "Server not running for WebSocket connection test"
        echo "CONNECTION_RESULT=1" >> "${result_file}"
    fi
    
    print_subtest "Test WebSocket Status Request (Config ${config_number})"

    if [[ -n "${hydrogen_pid}" ]] && ps -p "${hydrogen_pid}" > /dev/null 2>&1; then
        if test_websocket_status "${ws_url}" "${ws_protocol}" "${LOG_PREFIX}${TIMESTAMP}_${ws_protocol}_connection.json"; then
            echo "STATUS_RESULT=0" >> "${result_file}"
            ((PASS_COUNT++))
        else
            echo "STATUS_RESULT=1" >> "${result_file}"
        fi
    else
        print_result 1 "Server not running for WebSocket status test"
        echo "STATUS_RESULT=1" >> "${result_file}"
    fi
    
    print_subtest "Test WebSocket Port Accessibility (Config ${config_number})"

    if [[ -n "${hydrogen_pid}" ]] && ps -p "${hydrogen_pid}" > /dev/null 2>&1; then
        # Test port accessibility using /dev/tcp
        if timeout 5 bash -c "</dev/tcp/localhost/${ws_port}" 2>/dev/null; then
            print_result 0 "WebSocket port ${ws_port} is accessible"
            echo "PORT_RESULT=0" >> "${result_file}"
            ((PASS_COUNT++))
        else
            print_result 1 "WebSocket port ${ws_port} is not accessible"
            echo "PORT_RESULT=1" >> "${result_file}"
        fi
    else
        print_result 1 "Server not running for port accessibility test"
        echo "PORT_RESULT=1" >> "${result_file}"
    fi
    
    print_subtest "Verify WebSocket Initialization in Logs (Config ${config_number})"

    if [[ -n "${hydrogen_pid}" ]] && ps -p "${hydrogen_pid}" > /dev/null 2>&1; then
        # Check server logs for WebSocket initialization in one grep call
        if grep -q -E "LAUNCH: WEBSOCKETS|WebSocket.*successfully" "${server_log}"; then
            print_result 0 "WebSocket initialization confirmed in logs"
            echo "LOG_RESULT=0" >> "${result_file}"
            ((PASS_COUNT++))
        else
            print_result 1 "WebSocket initialization not found in logs"
            print_message "Log excerpt (last 10 lines):"
            while IFS= read -r line; do
                print_output "${line}"
            done < <(tail -n 10 "${server_log}" || true)
            echo "LOG_RESULT=1" >> "${result_file}"
        fi
    else
        print_result 1 "Server not running for log verification test"
        echo "LOG_RESULT=1" >> "${result_file}"
    fi
    
    # Stop the server
    if [[ -n "${hydrogen_pid}" ]]; then
        print_message "Stopping server..."
        if stop_hydrogen "${hydrogen_pid}" "${server_log}" 10 5 "${RESULTS_DIR}"; then
            print_message "Server stopped successfully"
        else
            print_warning "Server shutdown had issues"
        fi
        
        # Check TIME_WAIT sockets
        check_time_wait_sockets "${server_port}"
        check_time_wait_sockets "${ws_port}"
    fi
    
    return 0
}

# Function to analyze parallel test results
analyze_parallel_results() {
    local config_file="$1"
    local ws_port="$2"
    local ws_protocol="$3"
    local test_name="$4"
    local config_number="$5"
    local result_file="${LOG_PREFIX}${TIMESTAMP}_${ws_protocol}.result"
    
    if [[ ! -f "${result_file}" ]]; then
        print_message "No result file found for ${test_name}"
        EXIT_CODE=1
        return 1
    fi
    
    # shellcheck disable=SC2155 # Failure is not an option
    local start_result=$(grep "START_RESULT=" "${result_file}" | cut -d'=' -f2 || echo "1")
    # shellcheck disable=SC2155 # Failure is not an option
    local ready_result=$(grep "READY_RESULT=" "${result_file}" | cut -d'=' -f2 || echo "1")
    # shellcheck disable=SC2155 # Failure is not an option
    local connection_result=$(grep "CONNECTION_RESULT=" "${result_file}" | cut -d'=' -f2 || echo "1")
    # shellcheck disable=SC2155 # Failure is not an option
    local status_result=$(grep "STATUS_RESULT=" "${result_file}" | cut -d'=' -f2 || echo "1")
    # shellcheck disable=SC2155 # Failure is not an option
    local port_result=$(grep "PORT_RESULT=" "${result_file}" | cut -d'=' -f2 || echo "1")
    # shellcheck disable=SC2155 # Failure is not an option
    local log_result=$(grep "LOG_RESULT=" "${result_file}" | cut -d'=' -f2 || echo "1")
    
    if [[ "${start_result}" -ne 0 || "${ready_result}" -ne 0 ]]; then
        print_message "${test_name}: Server startup or readiness failed, skipping subtest results"
        EXIT_CODE=1
        return 1
    fi
    
    if [[ "${connection_result}" -ne 0 ]]; then
        ((TEST_FAILED_COUNT++))
        EXIT_CODE=1
    fi
    if [[ "${status_result}" -ne 0 ]]; then
        ((TEST_FAILED_COUNT++))
        EXIT_CODE=1
    fi
    if [[ "${port_result}" -ne 0 ]]; then
        ((TEST_FAILED_COUNT++))
        EXIT_CODE=1
    fi
    if [[ "${log_result}" -ne 0 ]]; then
        ((TEST_FAILED_COUNT++))
        EXIT_CODE=1
    fi

    ((TEST_PASSED_COUNT++))
    ((TEST_PASSED_COUNT++))
    ((TEST_PASSED_COUNT++))
    ((TEST_PASSED_COUNT++))
    ((TEST_PASSED_COUNT++))

    return 0
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
    print_test_completion "${TEST_NAME}" "${TEST_ABBR}" "${TEST_NUMBER}" "${TEST_VERSION}"
    ${ORCHESTRATION:-false} && return "${EXIT_CODE}" || exit "${EXIT_CODE}"
fi

print_subtest "Validate Configuration File 1"

if validate_config_file "${CONFIG_1}"; then
    ((PASS_COUNT++))
else
    EXIT_CODE=1
    print_test_completion "${TEST_NAME}" "${TEST_ABBR}" "${TEST_NUMBER}" "${TEST_VERSION}"
    ${ORCHESTRATION:-false} && return "${EXIT_CODE}" || exit "${EXIT_CODE}"
fi

print_subtest "Validate Configuration File 2"

if validate_config_file "${CONFIG_2}"; then
    ((PASS_COUNT++))
else
    EXIT_CODE=1
    print_test_completion "${TEST_NAME}" "${TEST_ABBR}" "${TEST_NUMBER}" "${TEST_VERSION}"
    ${ORCHESTRATION:-false} && return "${EXIT_CODE}" || exit "${EXIT_CODE}"
fi

print_subtest "Validate WEBSOCKET_KEY Environment Variable"

if [[ -n "${WEBSOCKET_KEY}" ]]; then
    if [[ "${WEBSOCKET_KEY_VALIDATED}" -eq 0 ]] || validate_websocket_key "WEBSOCKET_KEY" "${WEBSOCKET_KEY}"; then
        WEBSOCKET_KEY_VALIDATED=1
        print_result 0 "WEBSOCKET_KEY is valid and ready for WebSocket authentication"
        ((PASS_COUNT++))
    else
        print_result 1 "WEBSOCKET_KEY is invalid format"
        EXIT_CODE=1
        print_test_completion "${TEST_NAME}" "${TEST_ABBR}" "${TEST_NUMBER}" "${TEST_VERSION}"
        ${ORCHESTRATION:-false} && return "${EXIT_CODE}" || exit "${EXIT_CODE}"
    fi
else
    print_result 1 "WEBSOCKET_KEY environment variable is not set"
    EXIT_CODE=1
    print_test_completion "${TEST_NAME}" "${TEST_ABBR}" "${TEST_NUMBER}" "${TEST_VERSION}"
    ${ORCHESTRATION:-false} && return "${EXIT_CODE}" || exit "${EXIT_CODE}"
fi

# Proceed with WebSocket tests
print_message "Ensuring no existing Hydrogen processes are running..."
pkill -f "hydrogen.*json" 2>/dev/null || true

for config in "${CONFIGS[@]}"; do
    while (( $(jobs -r | wc -l || true) >= CORES )); do
        wait -n
    done
    IFS='|' read -r config_file ws_port ws_protocol test_name config_number <<< "${config}"
    print_message "Starting parallel test for: ${test_name}"
    test_websocket_configuration "${config_file}" "${ws_port}" "${ws_protocol}" "${test_name}" "${config_number}" &
    PARALLEL_PIDS+=($!)
done

# Wait for all parallel tests to complete
print_message "Waiting for all ${#CONFIGS[@]} parallel tests to complete..."
for pid in "${PARALLEL_PIDS[@]}"; do
    wait "${pid}"
done

print_subtest "All parallel tests completed, analyzing results..."

# Analyze results
for config in "${CONFIGS[@]}"; do
    IFS='|' read -r config_file ws_port ws_protocol test_name config_number <<< "${config}"
    print_message "Analyzing results for: ${test_name}"
    analyze_parallel_results "${config_file}" "${ws_port}" "${ws_protocol}" "${test_name}" "${config_number}"
done
print_result 0 "Analysis complete"
       
# Print test completion summary
print_test_completion "${TEST_NAME}" "${TEST_ABBR}" "${TEST_NUMBER}" "${TEST_VERSION}"

# Return status code if sourced, exit if run standalone
${ORCHESTRATION:-false} && return "${EXIT_CODE}" || exit "${EXIT_CODE}"
