#!/bin/bash

# Test: System API Endpoints
# Tests all the system API endpoints:
# - /api/system/test: Tests request handling with various parameters
# - /api/system/health: Tests the health check endpoint
# - /api/system/info: Tests the system information endpoint
# - /api/system/config: Tests the configuration endpoint
# - /api/system/prometheus: Tests the Prometheus metrics endpoint
# - /api/system/recent: Tests the recent activity log endpoint
# - /api/system/appconfig: Tests the application configuration endpoint

# CHANGELOG
# 4.0.1 - 2025-08-03 - Removed extraneous command -v calls
# 4.0.0 - 2025-07-30 - Overhaul #1
# 3.2.1 - 2025-07-18 - Fixed subshell issue in response file output that prevented detailed error messages from being displayed in test output
# 3.2.0 - 2025-07-15 - Added tests for missing system endpoints: /api/system/prometheus, /api/system/recent, /api/system/appconfig to improve coverage
# 3.1.3 - 2025-07-14 - Enhanced validate_api_request with retry logic to handle API subsystem initialization delays during parallel execution
# 3.1.2 - 2025-07-15 - No more sleep
# 3.1.1 - 2025-07-14 - Updated to use build/tests directories for test output consistency
# 3.1.0 - 2025-07-14 - Major architectural restructure to modular approach for better parallel execution reliability
# 3.0.2 - 2025-07-14 - Improved error handling in validate_api_request function to better handle parallel test execution
# 3.0.1 - 2025-07-06 - Added missing shellcheck justifications
# 3.0.0 - 2025-07-02 - Migrated to use lib/ scripts, following established test patterns
# 2.0.0 - 2025-06-17 - Major refactoring: fixed all shellcheck warnings, improved modularity, enhanced comments

# Test Configuration
TEST_NAME="System API"
TEST_ABBR="SYS"
TEST_NUMBER="32"
TEST_VERSION="4.0.1"

# shellcheck source=tests/lib/framework.sh # Reference framework directly
[[ -n "${FRAMEWORK_GUARD}" ]] || source "$(dirname "${BASH_SOURCE[0]}")/lib/framework.sh"
setup_test_environment

# Test variables
HYDROGEN_DIR="${PROJECT_DIR}"
CONFIG_PATH="$(get_config_path "hydrogen_test_system_endpoints.json")"

# Function to make a request and validate response with retry logic for API readiness
validate_api_request() {
    local request_name="$1"
    local url="$2"
    local expected_field="$3"
    
    # Generate unique ID once and store it
    local unique_id="${TIMESTAMP}_$${_}${RANDOM}"
    local response_file="${RESULTS_DIR}/response_${request_name}_${unique_id}"
    
    # Add appropriate extension based on endpoint type
    if [[ "${request_name}" == "prometheus" ]] || [[ "${request_name}" == "recent" ]] || [[ "${request_name}" == "appconfig" ]]; then
        response_file="${response_file}.txt"
    else
        response_file="${response_file}.json"
    fi
    
    # Store the actual filename for later access
    declare -g "RESPONSE_FILE_${request_name^^}=${response_file}"
    
    print_command "curl -s --max-time 10 --compressed \"${url}\""
    
    # Retry logic for API readiness (especially important in parallel execution)
    local max_attempts=25
    local attempt=1
    local curl_exit_code=0
    
    while [[ "${attempt}" -le "${max_attempts}" ]]; do
        if [[ "${attempt}" -gt 1 ]]; then
            print_message "API request attempt ${attempt} of ${max_attempts} (waiting for API subsystem initialization)..."
            sleep 0.05  # Brief delay between attempts for API initialization
        fi
        
        # Run curl and capture exit code
        curl -s --max-time 10 --compressed "${url}" > "${response_file}"
        curl_exit_code=$?
        
        if [[ "${curl_exit_code}" -eq 0 ]]; then
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
            
            print_message "Successfully received response from ${url}"
            
            # Show response excerpt
            local line_count
            line_count=$(wc -l < "${response_file}")
            print_message "Response contains ${line_count} lines"
            
            # Check for expected content based on endpoint type
            if [[ "${request_name}" == "recent" ]]; then
                # Use fixed string search for recent endpoint
                if grep -F -q "[" "${response_file}"; then
                    if [[ "${attempt}" -gt 1 ]]; then
                        print_result 0 "Response contains expected field: log entry (succeeded on attempt ${attempt})"
                    else
                        print_result 0 "Response contains expected field: log entry"
                    fi
                    return 0
                else
                    print_result 1 "Response doesn't contain expected field: log entry"
                    return 1
                fi
            else
                # Normal pattern search for other endpoints
                if grep -q "${expected_field}" "${response_file}"; then
                    if [[ "${attempt}" -gt 1 ]]; then
                        print_result 0 "Response contains expected content: ${expected_field} (succeeded on attempt ${attempt})"
                    else
                        print_result 0 "Response contains expected content: ${expected_field}"
                    fi
                    return 0
                else
                    print_result 1 "Response doesn't contain expected content: ${expected_field}"
                    print_message "Response excerpt (first 10 lines):"
                    # Use process substitution to avoid subshell issue with OUTPUT_COLLECTION
                    while IFS= read -r line; do
                        print_output "${line}"
                    done < <(head -n 10 "${response_file}" || true)
                    return 1
                fi
            fi
        else
            if [[ "${attempt}" -eq "${max_attempts}" ]]; then
                print_result 1 "Failed to connect to server at ${url} (curl exit code: ${curl_exit_code})"
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

# Function to validate JSON syntax
validate_json_response() {
    local file="$1"
    local endpoint_name="$2"
    
    # Use jq if available for proper JSON validation
    if jq . "${file}" > /dev/null 2>&1; then
        print_result 0 "${endpoint_name} endpoint returns valid JSON"
        return 0
    else
        print_result 1 "${endpoint_name} endpoint returns invalid JSON"
        return 1
    fi
}

# Function to validate line count in text response
validate_line_count() {
    local file="$1"
    local min_lines="$2"
    local endpoint_name="$3"
    local actual_lines
    actual_lines=$(wc -l < "${file}")
    
    if [[ "${actual_lines}" -gt "${min_lines}" ]]; then
        print_result 0 "${endpoint_name} endpoint contains ${actual_lines} lines (more than ${min_lines} required)"
        return 0
    else
        print_result 1 "${endpoint_name} endpoint contains only ${actual_lines} lines (${min_lines} required)"
        return 1
    fi
}

# Function to validate Prometheus format
validate_prometheus_format() {
    local file="$1"
    
    # Check content type
    if ! grep -q "Content-Type: text/plain" "${file}"; then
        print_result 1 "Prometheus endpoint has incorrect content type"
        return 1
    fi
    
    # Check for required Prometheus format elements
    local prometheus_format_ok=1
    
    # Check for TYPE definitions
    if ! grep -q "^# TYPE" "${file}"; then
        print_result 1 "Prometheus endpoint missing TYPE definitions"
        prometheus_format_ok=0
    fi
    
    # Check for specific metrics
    local required_metrics=(
        "system_info"
        "memory_total_bytes"
        "cpu_usage_total"
        "service_threads"
    )
    
    local metric
    for metric in "${required_metrics[@]}"; do
        if ! grep -q "^${metric}" "${file}"; then
            print_result 1 "Prometheus endpoint missing required metric: ${metric}"
            prometheus_format_ok=0
        fi
    done
    
    if [[ "${prometheus_format_ok}" -eq 1 ]]; then
        print_result 0 "Prometheus endpoint contains valid format and metrics"
        return 0
    else
        return 1
    fi
}

# Function to check server logs for compression and errors
check_server_logs() {
    local log_file="$1"
    local timestamp="$2"
    
    # Check server logs for API-related errors - filter irrelevant messages
    print_message "Checking server logs for API-related errors..."
    if grep -i "error\|warn\|fatal\|segmentation" "${log_file}" | grep -i "API\|System\|SystemTest\|SystemService\|Endpoint\|api" || true > "${RESULTS_DIR}/system_test_errors_${timestamp}.log"; then
        if [[ -s "${RESULTS_DIR}/system_test_errors_${timestamp}.log" ]]; then
            print_warning "API-related warning/error messages found in logs:"
            # Process each line individually for clean formatting
            while IFS= read -r line; do
                print_output "${line}"
            done < "${RESULTS_DIR}/system_test_errors_${timestamp}.log"
        fi
    fi

    # Check for Brotli compression logs
    print_message "Checking for Brotli compression logs..."
    if grep -i "Brotli" "${log_file}" > "${RESULTS_DIR}/brotli_compression_${timestamp}.log" 2>/dev/null; then
        if [[ -s "${RESULTS_DIR}/brotli_compression_${timestamp}.log" ]]; then
            print_message "Brotli compression logs found:"
            # Process each line individually for clean formatting
            while IFS= read -r line; do
                print_output "${line}"
            done < "${RESULTS_DIR}/brotli_compression_${timestamp}.log"
            # Check for compression metrics with level information
            if grep -q "Brotli(level=[0-9]\+).*bytes.*ratio.*compression.*time:" "${RESULTS_DIR}/brotli_compression_${timestamp}.log"; then
                print_result 0 "Compression logs contain detailed metrics with compression level"
                return 0
            else
                print_warning "Compression logs found but missing metrics details"
                return 1
            fi
        else
            print_warning "No Brotli compression logs found"
            return 1
        fi
    else
        print_warning "No Brotli compression logs found"
        return 1
    fi
}

# Function to test system endpoints with better isolation (based on test_34 pattern)
test_system_endpoints() {
    local test_name="$1"
    
    print_message "Testing System API Endpoints: ${test_name}"
    
    # Extract port from configuration
    local server_port
    server_port=$(get_webserver_port "${CONFIG_PATH}")
    print_message "Configuration will use port: ${server_port}"
    
    # Local variables for server management
    local hydrogen_pid=""
    local server_log="${RESULTS_DIR}/system_endpoints_${test_name}_${TIMESTAMP}.log"
    local base_url="http://localhost:${server_port}"
    
    print_subtest "Start Hydrogen Server (${test_name})"
    
    # Use a temporary variable name that won't conflict
    local temp_pid_var="HYDROGEN_PID_$$"
    # shellcheck disable=SC2153  # HYDROGEN_BIN is set by find_hydrogen_binary function
    if start_hydrogen_with_pid "${CONFIG_PATH}" "${server_log}" 15 "${HYDROGEN_BIN}" "${temp_pid_var}"; then
        # Get the PID from the temporary variable
        hydrogen_pid=$(eval "echo \$${temp_pid_var}")
        if [[ -n "${hydrogen_pid}" ]]; then
            print_result 0 "Server started successfully with PID: ${hydrogen_pid}"
            ((PASS_COUNT++))
        else
            print_result 1 "Failed to start server - no PID returned"
            EXIT_CODE=1
            return 1
        fi
    else
        print_result 1 "Failed to start server"
        EXIT_CODE=1
        return 1
    fi
    
    # Wait for server to be ready
    if [[ -n "${hydrogen_pid}" ]] && ps -p "${hydrogen_pid}" > /dev/null 2>&1; then
        if ! wait_for_server_ready "${base_url}"; then
            print_result 1 "Server failed to become ready"
            EXIT_CODE=1
            return 1
        fi
    fi
    
    print_subtest "Test Health Endpoint (${test_name})"

    if [[ -n "${hydrogen_pid}" ]] && ps -p "${hydrogen_pid}" > /dev/null 2>&1; then
        if validate_api_request "health" "${base_url}/api/system/health" "Yes, I'm alive, thanks!"; then
            ((PASS_COUNT++))
        else
            EXIT_CODE=1
        fi
    else
        print_result 1 "Server not running for health endpoint test"
        EXIT_CODE=1
    fi
    
    print_subtest "Test Info Endpoint (${test_name})"

    if [[ -n "${hydrogen_pid}" ]] && ps -p "${hydrogen_pid}" > /dev/null 2>&1; then
        if validate_api_request "info" "${base_url}/api/system/info" "system"; then
            ((PASS_COUNT++))
        else
            EXIT_CODE=1
        fi
    else
        print_result 1 "Server not running for info endpoint test"
        EXIT_CODE=1
    fi
    
    print_subtest "Test Config Endpoint (${test_name})"

    if [[ -n "${hydrogen_pid}" ]] && ps -p "${hydrogen_pid}" > /dev/null 2>&1; then
        if validate_api_request "config" "${base_url}/api/system/config" "ServerName"; then
            ((PASS_COUNT++))
        else
            EXIT_CODE=1
        fi
    else
        print_result 1 "Server not running for config endpoint test"
        EXIT_CODE=1
    fi
    
    print_subtest "Test System Test Endpoint (${test_name})"

    if [[ -n "${hydrogen_pid}" ]] && ps -p "${hydrogen_pid}" > /dev/null 2>&1; then
        if validate_api_request "basic_get" "${base_url}/api/system/test" "client_ip"; then
            ((PASS_COUNT++))
        else
            EXIT_CODE=1
        fi
    else
        print_result 1 "Server not running for system test endpoint"
        EXIT_CODE=1
    fi
    
    print_subtest "Test Prometheus Endpoint (${test_name})"

    if [[ -n "${hydrogen_pid}" ]] && ps -p "${hydrogen_pid}" > /dev/null 2>&1; then
        if validate_api_request "prometheus" "${base_url}/api/system/prometheus" "system_info"; then
            ((PASS_COUNT++))
        else
            EXIT_CODE=1
        fi
    else
        print_result 1 "Server not running for prometheus endpoint test"
        EXIT_CODE=1
    fi
    
    print_subtest "Test Recent Logs Endpoint (${test_name})"

    if [[ -n "${hydrogen_pid}" ]] && ps -p "${hydrogen_pid}" > /dev/null 2>&1; then
        if validate_api_request "recent" "${base_url}/api/system/recent" "\\["; then
            ((PASS_COUNT++))
        else
            EXIT_CODE=1
        fi
    else
        print_result 1 "Server not running for recent endpoint test"
        EXIT_CODE=1
    fi
    
    print_subtest "Test AppConfig Endpoint (${test_name})"

    if [[ -n "${hydrogen_pid}" ]] && ps -p "${hydrogen_pid}" > /dev/null 2>&1; then
        if validate_api_request "appconfig" "${base_url}/api/system/appconfig" "APPCONFIG"; then
            ((PASS_COUNT++))
        else
            EXIT_CODE=1
        fi
    else
        print_result 1 "Server not running for appconfig endpoint test"
        EXIT_CODE=1
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
    fi
    
    return 0
}

# Handle cleanup on script interruption (not normal exit)
# shellcheck disable=SC2317  # Function is invoked indirectly via trap
cleanup() {
    print_message "Cleaning up any remaining Hydrogen processes..."
    pkill -f "hydrogen.*json" 2>/dev/null || true
    exit "${EXIT_CODE}"
}

# Set up trap for interruption only (not normal exit)
trap cleanup SIGINT SIGTERM

print_subtest "Locate Hydrogen Binary"

# shellcheck disable=SC2153  # HYDROGEN_BIN is set by find_hydrogen_binary function
if find_hydrogen_binary "${HYDROGEN_DIR}" "HYDROGEN_BIN"; then
    print_result 0 "Hydrogen binary found: $(basename "${HYDROGEN_BIN}")"
    ((PASS_COUNT++))
else
    print_result 1 "Hydrogen binary not found"
    EXIT_CODE=1
fi

print_subtest "Validate Configuration File"

if validate_config_file "${CONFIG_PATH}"; then
    ((PASS_COUNT++))
    
    # Extract and display port
    SERVER_PORT=$(get_webserver_port "${CONFIG_PATH}")
    print_message "Configuration will use port: ${SERVER_PORT}"
else
    EXIT_CODE=1
fi

# Only proceed with API tests if binary and config are available
if [[ "${EXIT_CODE}" -eq 0 ]]; then
    # Ensure clean state
    print_message "Ensuring no existing Hydrogen processes are running..."
    pkill -f "hydrogen.*json" 2>/dev/null || true
    
    print_message "Testing System API Endpoints with modular approach"
    print_message "SO_REUSEADDR is enabled - no need to wait for TIME_WAIT"
    
    # Test system endpoints using modular approach
    test_system_endpoints "core_endpoints"
    
else
    # Skip API tests if prerequisites failed
    print_message "Skipping API endpoint tests due to prerequisite failures"
    # Account for skipped subtests
    for i in {3..9}; do

        print_subtest "Subtest ${i} (Skipped)"
        print_result 1 "Skipped due to prerequisite failures"

    done
    EXIT_CODE=1
fi

# Print test completion summary
print_test_completion "${TEST_NAME}" "${TEST_ABBR}" "${TEST_NUMBER}" "${TEST_VERSION}"

# Return status code if sourced, exit if run standalone
${ORCHESTRATION:-false} && return "${EXIT_CODE}" || exit "${EXIT_CODE}"
