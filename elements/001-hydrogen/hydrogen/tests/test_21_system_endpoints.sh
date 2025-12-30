#!/usr/bin/env bash

# Test: System API Endpoints
# Tests all the system API endpoints:
# - /api/system/test: Tests request handling with various parameters
# - /api/system/health: Tests the health check endpoint
# - /api/system/info: Tests the system information endpoint
# - /api/system/config: Tests the configuration endpoint
# - /api/system/prometheus: Tests the Prometheus metrics endpoint
# - /api/system/recent: Tests the recent activity log endpoint
# - /api/system/appconfig: Tests the application configuration endpoint

# FUNCTIONS
# validate_api_request()
# validate_json_response()
# validate_line_count()
# validate_prometheus_format()
# check_server_logs()
# run_endpoint_test_parallel()
# analyze_endpoint_test_results()
# test_system_endpoints()

# CHANGELOG
# 6.0.0 - 2025-09-19 - Added average response time calculation and display in test name for all system endpoints
# 5.1.0 - 2025-08-09 - Mopping up after major factor, tweaking names of log files mostly
# 5.0.0 - 2025-08-08 - Major refactor: Implemented parallel execution of endpoint requests against single server.
#                    - Extracted modular functions run_endpoint_test_parallel() and analyze_endpoint_test_results(). Now tests all 7 system endpoints simultaneously instead of sequentially, significantly reducing execution time while maintaining single server approach.
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

set -euo pipefail

# Test Configuration
TEST_NAME="Endpoints"
TEST_ABBR="SYS"
TEST_NUMBER="21"
TEST_COUNTER=0
TEST_VERSION="6.0.0"

# shellcheck source=tests/lib/framework.sh # Reference framework directly
[[ -n "${FRAMEWORK_GUARD:-}" ]] || source "$(dirname "${BASH_SOURCE[0]}")/lib/framework.sh"
setup_test_environment

# Test variables
CONFIG_PATH="${SCRIPT_DIR}/configs/hydrogen_test_${TEST_NUMBER}_system_endpoints.json"

# Curl timing format for measuring response times
CURL_FORMAT='     time_namelookup:  %{time_namelookup}\n        time_connect:  %{time_connect}\n     time_appconnect:  %{time_appconnect}\n    time_pretransfer:  %{time_pretransfer}\n       time_redirect:  %{time_redirect}\n  time_starttransfer:  %{time_starttransfer}\n                     total:  %{time_total}\n'

# Parallel execution configuration for endpoint requests
declare -A ENDPOINT_TEST_CONFIGS

# Endpoint test configuration - format: "endpoint:expected_content:description"
ENDPOINT_TEST_CONFIGS=(
    ["HEALTH"]="health:Yes, I'm alive, thanks!:Health Check"
    ["INFO"]="info:system:System Information"
    ["CONFIG"]="config:ServerName:Configuration Data"
    ["TEST"]="basic_get:client_ip:System Test"
    ["PROMETHEUS"]="prometheus:system_info:Prometheus Metrics"
    ["RECENT"]="recent:\\[:Recent Activity Logs"
    ["APPCONFIG"]="appconfig:APPCONFIG:Application Configuration"
    ["VERSION"]="version:OctoPrint:Version Information"
)

# Function to make a request and validate response with retry logic for API readiness
validate_api_request() {
    local request_name="$1"
    local url="$2"
    local expected_field="$3"
    local timing_file="$4"  # New parameter for timing data
    local response_file="${LOG_PREFIX}${TIMESTAMP}_${request_name}"
    
    # Add appropriate extension based on endpoint type
    if [[ "${request_name}" == "prometheus" ]] || [[ "${request_name}" == "recent" ]] || [[ "${request_name}" == "appconfig" ]]; then
        response_file="${response_file}.txt"
    else
        response_file="${response_file}.json"
    fi

    # Store the actual filename for later access
    declare -g "RESPONSE_FILE_${request_name^^}=${response_file}"
    
    print_command "${TEST_NUMBER}" "${TEST_COUNTER}" "curl -s --max-time 10 --compressed \"${url}\""
    
    # Retry logic for API readiness (reduced for parallel execution to prevent thundering herd)
    local max_attempts=10
    local attempt=1
    local curl_exit_code=0
    local response_time="0.000"

    while [[ "${attempt}" -le "${max_attempts}" ]]; do
        if [[ "${attempt}" -gt 1 ]]; then
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "API request attempt ${attempt} of ${max_attempts} (waiting for API subsystem initialization)..."
            sleep 0.05  # Brief delay between attempts for API initialization
        fi

        # Run curl with timing and capture exit code
        local timing_output
        timing_output=$(curl -s --max-time 10 --compressed -w "${CURL_FORMAT}" -o "${response_file}" "${url}" 2>/dev/null)
        curl_exit_code=$?

        # Extract total time from timing output
        if [[ -n "${timing_output}" ]]; then
            response_time=$(echo "${timing_output}" | grep "total:" | awk '{print $2}' | sed 's/s$//' || echo "0.000")
        fi
        
        if [[ "${curl_exit_code}" -eq 0 ]]; then
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
            
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Successfully received response from ${url}"
            
            # Show response excerpt
            local line_count
            line_count=$(wc -l < "${response_file}")
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Response contains ${line_count} lines"
            
            # Check for expected content based on endpoint type
            if [[ "${request_name}" == "recent" ]]; then
                # Use fixed string search for recent endpoint
                if "${GREP}" -F -q "[" "${response_file}"; then
                    # Store timing data if file provided
                    if [[ -n "${timing_file}" ]]; then
                        echo "${response_time}" > "${timing_file}"
                    fi
                    # Format timing for display
                    local timing_display=""
                    if [[ -n "${timing_file}" ]]; then
                        local timing_ms
                        # shellcheck disable=SC2312 # We want to continue even if printf fails
                        timing_ms=$(printf "%.3f" "$(echo "scale=6; ${response_time} * 1000" | bc 2>/dev/null || echo "0.000")" 2>/dev/null || echo "0.000")
                        timing_display=" (${timing_ms}ms)"
                    fi
                    if [[ "${attempt}" -gt 1 ]]; then
                        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Response contains expected field: log entry${timing_display} (succeeded on attempt ${attempt})"
                    else
                        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Response contains expected field: log entry${timing_display}"
                    fi
                    return 0
                else
                    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Response doesn't contain expected field: log entry"
                    return 1
                fi
            else
                # Normal pattern search for other endpoints
                if "${GREP}" -q "${expected_field}" "${response_file}"; then
                    # Store timing data if file provided
                    if [[ -n "${timing_file}" ]]; then
                        echo "${response_time}" > "${timing_file}"
                    fi
                    # Format timing for display
                    local timing_display=""
                    if [[ -n "${timing_file}" ]]; then
                        local timing_ms
                        # shellcheck disable=SC2312 # We want to continue even if printf fails
                        timing_ms=$(printf "%.3f" "$(echo "scale=6; ${response_time} * 1000" | bc 2>/dev/null || echo "0.000")" 2>/dev/null || echo "0.000")
                        timing_display=" (${timing_ms}ms)"
                    fi
                    if [[ "${attempt}" -gt 1 ]]; then
                        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Response contains expected content: ${expected_field}${timing_display} (succeeded on attempt ${attempt})"
                    else
                        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Response contains expected content: ${expected_field}${timing_display}"
                    fi
                    return 0
                else
                    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Response doesn't contain expected content: ${expected_field}"
                    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Response excerpt (first 10 lines):"
                    # Use process substitution to avoid subshell issue with OUTPUT_COLLECTION
                    while IFS= read -r line; do
                        print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "${line}"
                    done < <(head -n 10 "${response_file}" || true)
                    return 1
                fi
            fi
        else
            if [[ "${attempt}" -eq "${max_attempts}" ]]; then
                print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Failed to connect to server at ${url} (curl exit code: ${curl_exit_code})"
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

# Function to validate JSON syntax
validate_json_response() {
    local file="$1"
    local endpoint_name="$2"
    
    # Use jq if available for proper JSON validation
    if jq . "${file}" > /dev/null 2>&1; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "${endpoint_name} endpoint returns valid JSON"
        return 0
    else
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${endpoint_name} endpoint returns invalid JSON"
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
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "${endpoint_name} endpoint contains ${actual_lines} lines (more than ${min_lines} required)"
        return 0
    else
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${endpoint_name} endpoint contains only ${actual_lines} lines (${min_lines} required)"
        return 1
    fi
}

# Function to validate Prometheus format
validate_prometheus_format() {
    local file="$1"
    
    # Check content type
    if ! "${GREP}" -q "Content-Type: text/plain" "${file}"; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Prometheus endpoint has incorrect content type"
        return 1
    fi
    
    # Check for required Prometheus format elements
    local prometheus_format_ok=1
    
    # Check for TYPE definitions
    if ! "${GREP}" -q "^# TYPE" "${file}"; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Prometheus endpoint missing TYPE definitions"
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
        if ! "${GREP}" -q "^${metric}" "${file}"; then
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Prometheus endpoint missing required metric: ${metric}"
            prometheus_format_ok=0
        fi
    done
    
    if [[ "${prometheus_format_ok}" -eq 1 ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Prometheus endpoint contains valid format and metrics"
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
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Checking server logs for API-related errors..."
    if "${GREP}" -i "error\|warn\|fatal\|segmentation" "${log_file}" | "${GREP}" -i "API\|System\|SystemTest\|SystemService\|Endpoint\|api" || true > "${RESULTS_DIR}/system_test_errors_${timestamp}.log"; then
        if [[ -s "${RESULTS_DIR}/system_test_errors_${timestamp}.log" ]]; then
            print_warning "${TEST_NUMBER}" "${TEST_COUNTER}" "API-related warning/error messages found in logs:"
            # Process each line individually for clean formatting
            while IFS= read -r line; do
                print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "${line}"
            done < "${RESULTS_DIR}/system_test_errors_${timestamp}.log"
        fi
    fi

    # Check for Brotli compression logs
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Checking for Brotli compression logs..."
    if "${GREP}" -i "Brotli" "${log_file}" > "${RESULTS_DIR}/brotli_compression_${timestamp}.log" 2>/dev/null; then
        if [[ -s "${RESULTS_DIR}/brotli_compression_${timestamp}.log" ]]; then
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Brotli compression logs found:"
            # Process each line individually for clean formatting
            while IFS= read -r line; do
                print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "${line}"
            done < "${RESULTS_DIR}/brotli_compression_${timestamp}.log"
            # Check for compression metrics with level information
            if "${GREP}" -q "Brotli(level=[0-9]\+).*bytes.*ratio.*compression.*time:" "${RESULTS_DIR}/brotli_compression_${timestamp}.log"; then
                print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Compression logs contain detailed metrics with compression level"
                return 0
            else
                print_warning "${TEST_NUMBER}" "${TEST_COUNTER}" "Compression logs found but missing metrics details"
                return 1
            fi
        else
            print_warning "${TEST_NUMBER}" "${TEST_COUNTER}" "No Brotli compression logs found"
            return 1
        fi
    else
        print_warning "${TEST_NUMBER}" "${TEST_COUNTER}" "No Brotli compression logs found"
        return 1
    fi
}

# Function to test a single endpoint in parallel
run_endpoint_test_parallel() {
    local test_name="$1"
    local base_url="$2"
    local endpoint_path="$3"
    local expected_content="$4"
    local description="$5"
    
    local result_file="${LOG_PREFIX}${TIMESTAMP}_${test_name}.result"
    
    # Clear result file
    true > "${result_file}"
    
    # Create timing file for this endpoint
    local timing_file="${LOG_PREFIX}${TIMESTAMP}_${test_name}.timing"

    # Use the existing validate_api_request function with unique naming
    # shellcheck disable=SC2310 # We want to continue even if the test fails
    if validate_api_request "${test_name}" "${base_url}${endpoint_path}" "${expected_content}" "${timing_file}"; then
        echo "ENDPOINT_TEST_PASSED" >> "${result_file}"
    else
        echo "ENDPOINT_TEST_FAILED" >> "${result_file}"
    fi
    
    echo "TEST_COMPLETE" >> "${result_file}"
}

# Function to analyze results from parallel endpoint test execution
analyze_endpoint_test_results() {
    local test_name="$1"
    local description="$2"
    local result_file="${LOG_PREFIX}${TIMESTAMP}_${test_name}.result"

    if [[ ! -f "${result_file}" ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "No result file found for ${test_name}"
        return 1
    fi

    # Check endpoint test result
    if "${GREP}" -q "ENDPOINT_TEST_PASSED" "${result_file}" 2>/dev/null; then
        # Read timing data if available
        local timing_display=""
        local timing_file="${LOG_PREFIX}${TIMESTAMP}_${test_name}.timing"
        if [[ -f "${timing_file}" ]]; then
            local timing_value
            timing_value=$(cat "${timing_file}" 2>/dev/null || echo "0.000")
            if [[ -n "${timing_value}" ]] && [[ "${timing_value}" =~ ^[0-9]+\.[0-9]+$ ]]; then
                # shellcheck disable=SC2312 # We want to continue even if printf fails
                local timing_ms
                # shellcheck disable=SC2312 # We want to continue even if printf fails
                timing_ms=$(printf "%.3f" "$(echo "scale=6; ${timing_value} * 1000" | bc 2>/dev/null || echo "0.000")" 2>/dev/null || echo "0.000")
                timing_display=" (${timing_ms}ms)"
            fi
        fi
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "${description} endpoint test passed${timing_display}"
        return 0
    else
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${description} endpoint test failed"
        return 1
    fi
}

# Function to test system endpoints with parallel requests
test_system_endpoints() {
    local test_name="$1"
    
    # Extract port from configuration
    local server_port
    server_port=$(get_webserver_port "${CONFIG_PATH}")
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Configuration uses port: ${server_port}"
    
    # Local variables for server management
    local hydrogen_pid=""
    local server_log="${LOG_FILE}"
    local base_url="http://localhost:${server_port}"
    
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Start Hydrogen Server (${test_name})"
    
    # Use a temporary variable name that won't conflict
    local temp_pid_var="HYDROGEN_PID_$$"
    # shellcheck disable=SC2310 # We want to continue even if the test fails
    if start_hydrogen_with_pid "${CONFIG_PATH}" "${server_log}" 15 "${HYDROGEN_BIN}" "${temp_pid_var}"; then
        # Get the PID from the temporary variable
        hydrogen_pid=$(eval "echo \$${temp_pid_var}")
        if [[ -n "${hydrogen_pid}" ]]; then
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Server started successfully with PID: ${hydrogen_pid}"
        else
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Failed to start server - no PID returned"
            EXIT_CODE=1
            return 1
        fi
    else
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Failed to start server"
        EXIT_CODE=1
        return 1
    fi
    
    # Wait for server to be ready
    if [[ -n "${hydrogen_pid}" ]] && ps -p "${hydrogen_pid}" > /dev/null 2>&1; then
        # shellcheck disable=SC2310 # We want to continue even if the test fails
        if ! wait_for_server_ready "${base_url}"; then
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Server failed to become ready"
            EXIT_CODE=1
            return 1
        fi
    fi
    
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Running endpoint tests in parallel for faster execution..."
    
    # Start all endpoint tests in parallel with job limiting
    local endpoint_parallel_pids=()
    for test_config in "${!ENDPOINT_TEST_CONFIGS[@]}"; do
        # shellcheck disable=SC2312 # Job control with wc -l is standard practice
        while (( $(jobs -r | wc -l) >= CORES )); do
            wait -n  # Wait for any job to finish
        done
        
        # Parse test configuration
        IFS=':' read -r endpoint_name expected_content description <<< "${ENDPOINT_TEST_CONFIGS[${test_config}]}"
        
        # Map endpoint names to API paths
        local endpoint_path
        case "${endpoint_name}" in
            "health") endpoint_path="/api/system/health" ;;
            "info") endpoint_path="/api/system/info" ;;
            "config") endpoint_path="/api/system/config" ;;
            "basic_get") endpoint_path="/api/system/test" ;;
            "prometheus") endpoint_path="/api/system/prometheus" ;;
            "recent") endpoint_path="/api/system/recent" ;;
            "appconfig") endpoint_path="/api/system/appconfig" ;;
            *) endpoint_path="/api/system/${endpoint_name}" ;;
        esac
        
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Starting parallel test: ${test_config} (${description})"
        
        # Run parallel endpoint test in background
        run_endpoint_test_parallel "${endpoint_name}" "${base_url}" "${endpoint_path}" "${expected_content}" "${description}" &
        endpoint_parallel_pids+=($!)
    done
    
    # Wait for all parallel endpoint tests to complete
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Waiting for all ${#ENDPOINT_TEST_CONFIGS[@]} parallel endpoint tests to complete..."
    for pid in "${endpoint_parallel_pids[@]}"; do
        wait "${pid}"
    done
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "All parallel endpoint tests completed, analyzing results..."
    
    # Process results sequentially for clean output
    for test_config in "${!ENDPOINT_TEST_CONFIGS[@]}"; do
        # Parse test configuration
        IFS=':' read -r endpoint_name expected_content description <<< "${ENDPOINT_TEST_CONFIGS[${test_config}]}"
        
        print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "${description} Endpoint"
        
        # shellcheck disable=SC2310 # We want to continue even if the test fails
        if analyze_endpoint_test_results "${endpoint_name}" "${description}"; then
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Analysis complete"
        else
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Analysis failed"
            EXIT_CODE=1
        fi
    done
    
    # Print summary
    local successful_endpoints=0
    for test_config in "${!ENDPOINT_TEST_CONFIGS[@]}"; do
        IFS=':' read -r endpoint_name expected_content description <<< "${ENDPOINT_TEST_CONFIGS[${test_config}]}"
        result_file="${LOG_PREFIX}${TIMESTAMP}_${endpoint_name}.result"
        if [[ -f "${result_file}" ]] && "${GREP}" -q "ENDPOINT_TEST_PASSED" "${result_file}" 2>/dev/null; then
            successful_endpoints=$(( successful_endpoints + 1 ))
        fi
    done
    
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Summary: ${successful_endpoints}/${#ENDPOINT_TEST_CONFIGS[@]} system endpoints passed all tests"
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Parallel endpoint execution completed"

    # Calculate average response time from actual timing data
    local total_response_time="0"
    local timing_count=0
    for test_config in "${!ENDPOINT_TEST_CONFIGS[@]}"; do
        IFS=':' read -r endpoint_name expected_content description <<< "${ENDPOINT_TEST_CONFIGS[${test_config}]}"
        result_file="${LOG_PREFIX}${TIMESTAMP}_${endpoint_name}.result"
        if [[ -f "${result_file}" ]] && "${GREP}" -q "ENDPOINT_TEST_PASSED" "${result_file}" 2>/dev/null; then
            # Collect timing data from timing files for this configuration
            timing_file="${LOG_PREFIX}${TIMESTAMP}_${endpoint_name}.timing"
            if [[ -f "${timing_file}" ]]; then
                timing_value=$(tr -d '\n' < "${timing_file}" 2>/dev/null || echo "0")
                # Check if timing value is a valid number greater than 0
                if [[ -n "${timing_value}" ]] && [[ "${timing_value}" =~ ^[0-9]+\.[0-9]+$ ]]; then
                    # shellcheck disable=SC2312 # We want to continue even if bc fails
                    if (( $(echo "${timing_value} > 0" | bc -l 2>/dev/null || echo "0") )); then
                        total_response_time=$(echo "scale=6; ${total_response_time} + ${timing_value}" | bc 2>/dev/null || echo "${total_response_time}")
                        timing_count=$((timing_count + 1))
                    fi
                fi
            fi
        fi
    done

    # Calculate average response time from actual timing data
    if [[ ${timing_count} -gt 0 ]]; then
        local avg_response_time
        avg_response_time=$(echo "scale=6; ${total_response_time} / ${timing_count}" | bc 2>/dev/null || echo "0.000500")
        # Convert to milliseconds for more readable display
        # shellcheck disable=SC2312 # We want to continue even if printf fails
        avg_response_time_ms=$(printf "%.3f" "$(echo "scale=6; ${avg_response_time} * 1000" | bc 2>/dev/null || echo "0.500")" 2>/dev/null || echo "0.500")
        TEST_NAME="${TEST_NAME}  {BLUE}avg: ${avg_response_time_ms}ms{RESET}"
    fi

    # Stop the server
    if [[ -n "${hydrogen_pid}" ]]; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Stopping server..."
        # shellcheck disable=SC2310 # We want to continue even if the test fails
        if stop_hydrogen "${hydrogen_pid}" "${server_log}" 10 5 "${RESULTS_DIR}"; then
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Server stopped successfully"
        else
            print_warning "${TEST_NUMBER}" "${TEST_COUNTER}" "Server shutdown had issues"
        fi
        
        # Check TIME_WAIT sockets
        check_time_wait_sockets "${server_port}"
    fi
    
    return 0
}

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Locate Hydrogen Binary"

HYDROGEN_BIN=''
HYDROGEN_BIN_BASE=''
# shellcheck disable=SC2310 # We want to continue even if the test fails
if find_hydrogen_binary "${PROJECT_DIR}"; then
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}"4 "Using Hydrogen binary: ${HYDROGEN_BIN_BASE}"
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Hydrogen binary found and validated"
else
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Failed to find Hydrogen binary"
    EXIT_CODE=1
fi

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Validate Configuration File"

# shellcheck disable=SC2310 # We want to continue even if the test fails
if validate_config_file "${CONFIG_PATH}"; then
    # Extract and display port
    SERVER_PORT=$(get_webserver_port "${CONFIG_PATH}")
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Configuration will use port: ${SERVER_PORT}"
else
    EXIT_CODE=1
fi

# Only proceed with API tests if binary and config are available
if [[ "${EXIT_CODE}" -eq 0 ]]; then
    # Test system endpoints using modular approach
    test_system_endpoints "core_endpoints"
    
else
    # Skip API tests if prerequisites failed
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Skipping API endpoint tests due to prerequisite failures"
    # Account for skipped subtests
    for i in {3..9}; do

        print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Subtest ${i} (Skipped)"
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Skipped due to prerequisite failures"

    done
    EXIT_CODE=1
fi

# Print test completion summary
print_test_completion "${TEST_NAME}" "${TEST_ABBR}" "${TEST_NUMBER}" "${TEST_VERSION}"

# Return status code if sourced, exit if run standalone
${ORCHESTRATION:-false} && return "${EXIT_CODE}" || exit "${EXIT_CODE}"
