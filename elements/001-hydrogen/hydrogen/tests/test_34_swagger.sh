#!/bin/bash

# Test: Swagger
# Tests the Swagger functionality, its presence in the payload, etc.

# FUNCTIONS
# check_response_content()
# check_redirect_response()
# check_swagger_json()
# run_swagger_test_parallel()
# analyze_swagger_test_results()
# test_swagger_configuration()

# CHANGELOG
# 5.1.0 - 2025-08-09 - Minor tweaks to log file names
# 5.0.0 - 2025-08-08 - Major refactor: Implemented parallel execution of Swagger tests following Test 13/20 patterns. 
#                    - Extracted modular functions run_swagger_test_parallel() and analyze_swagger_test_results(). 
#                    - Now runs both /swagger and /apidocs tests simultaneously instead of sequentially, significantly reducing execution time.
# 4.0.1 - 2025-08-03 - Removed extraneous command -v calls
# 4.0.0 - 2025-07-30 - Overhaul #1
# 3.1.4 - 2025-07-18 - Fixed subshell issue in response output that prevented detailed error messages from being displayed in test output
# 3.1.3 - 2025-07-14 - Enhanced HTTP request functions with retry logic to handle subsystem initialization delays during parallel execution
# 3.1.2 - 2025-07-15 - No more sleep
# 3.1.1 - 2025-07-14 - Updated to use build/tests directories for test output consistency
# 3.1.0 - 2025-07-13 - Added swagger.json file retrieval and validation testing
# 3.0.1 - 2025-07-06 - Added missing shellcheck justifications
# 3.0.0 - 2025-07-02 - Migrated to use lib/ scripts, following established test patterns
# 2.0.0 - 2025-06-17 - Major refactoring: fixed all shellcheck warnings, improved modularity, enhanced comments

# Test Configuration
TEST_NAME="Swagger"
TEST_ABBR="SWG"
TEST_NUMBER="34"
TEST_VERSION="5.0.0"

# shellcheck source=tests/lib/framework.sh # Reference framework directly
[[ -n "${FRAMEWORK_GUARD}" ]] || source "$(dirname "${BASH_SOURCE[0]}")/lib/framework.sh"
setup_test_environment

# Parallel execution configuration
declare -a PARALLEL_PIDS
declare -A SWAGGER_TEST_CONFIGS

# Swagger test configuration - format: "config_file:log_suffix:swagger_prefix:description"
SWAGGER_TEST_CONFIGS=(
    ["ONE"]="${SCRIPT_DIR}/configs/hydrogen_test_34_swagger_1.json:one:/swagger:Swagger Prefix One"
    ["TWO"]="${SCRIPT_DIR}/configs/hydrogen_test_34_swagger_2.json:two:/apidocs:Swagger Prefix Two"
)

# Test timeouts
STARTUP_TIMEOUT=15
SHUTDOWN_TIMEOUT=10

# Function to check HTTP response content with retry logic for subsystem readiness
check_response_content() {
    local url="$1"
    local expected_content="$2"
    local response_file="$3"
    local follow_redirects="$4"
    
    local curl_cmd="curl -s --max-time 10"
    if [[ "${follow_redirects}" = "true" ]]; then
        curl_cmd="${curl_cmd} -L"
    fi
    curl_cmd="${curl_cmd} --compressed"
    
    print_command "${curl_cmd} \"${url}\""
    
    # Retry logic for subsystem readiness (especially important in parallel execution)
    local max_attempts=25
    local attempt=1
    local curl_exit_code=0
    
    while [[ "${attempt}" -le "${max_attempts}" ]]; do
        if [[ "${attempt}" -gt 1 ]]; then
            print_message "HTTP request attempt ${attempt} of ${max_attempts} (waiting for subsystem initialization)..."
            # sleep 0.05  # Brief delay between attempts for subsystem initialization
        fi
        
        # Run curl and capture exit code
        eval "${curl_cmd} \"${url}\" > \"${response_file}\""
        curl_exit_code=$?
        
        if [[ "${curl_exit_code}" -eq 0 ]]; then
            # Check if we got a 404 or other error response
            if grep -q "404 Not Found" "${response_file}" || grep -q "<html>" "${response_file}"; then
                if [[ "${attempt}" -eq "${max_attempts}" ]]; then
                    print_message "Endpoint still not ready after ${max_attempts} attempts"
                    print_result 1 "Endpoint returned 404 or HTML error page"
                    print_message "Response content:"
                    print_output "$(cat "${response_file}" || true)"
                    return 1
                else
                    print_message "Endpoint not ready yet (got 404/HTML), retrying..."
                    ((attempt++))
                    continue
                fi
            fi
            
            print_message "Successfully received response from ${url}"
            
            # Show response excerpt
            local line_count
            line_count=$(wc -l < "${response_file}")
            print_message "Response contains ${line_count} lines"
            
            # Check for expected content
            if grep -q "${expected_content}" "${response_file}"; then
                if [[ "${attempt}" -gt 1 ]]; then
                    print_result 0 "Response contains expected content: ${expected_content} (succeeded on attempt ${attempt})"
                else
                    print_result 0 "Response contains expected content: ${expected_content}"
                fi
                return 0
            else
                print_result 1 "Response doesn't contain expected content: ${expected_content}"
                print_message "Response excerpt (first 10 lines):"
                # Use process substitution to avoid subshell issue with OUTPUT_COLLECTION
                while IFS= read -r line; do
                    print_output "${line}"
                done < <(head -n 10 "${response_file}" || true)
                return 1
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

# Function to check HTTP redirect with retry logic for subsystem readiness
check_redirect_response() {
    local url="$1"
    local expected_location="$2"
    local redirect_file="$3"
    
    print_command "curl -v -s --max-time 10 -o /dev/null \"${url}\""
    
    # Retry logic for subsystem readiness (especially important in parallel execution)
    local max_attempts=25
    local attempt=1
    local curl_exit_code=0
    
    while [[ "${attempt}" -le "${max_attempts}" ]]; do
        if [[ "${attempt}" -gt 1 ]]; then
            print_message "Redirect check attempt ${attempt} of ${max_attempts} (waiting for subsystem initialization)..."
            # sleep 0.05  # Brief delay between attempts for subsystem initialization
        fi
        
        # Run curl and capture exit code
        curl -v -s --max-time 10 -o /dev/null "${url}" 2> "${redirect_file}"
        curl_exit_code=$?
        
        if [[ "${curl_exit_code}" -eq 0 ]]; then
            # Check if we got a 404 or connection error
            if grep -q "404 Not Found" "${redirect_file}"; then
                if [[ "${attempt}" -eq "${max_attempts}" ]]; then
                    print_message "Endpoint still not ready after ${max_attempts} attempts"
                    print_result 1 "Endpoint returned 404 error"
                    print_message "Response headers:"
                    # Use process substitution to avoid subshell issue with OUTPUT_COLLECTION
                    while IFS= read -r line; do
                        print_output "${line}"
                    done < <(grep -E "< HTTP/|< Location:" "${redirect_file}" || true)
                    return 1
                else
                    print_message "Endpoint not ready yet (got 404), retrying..."
                    ((attempt++))
                    continue
                fi
            fi
            
            print_message "Successfully received response from ${url}"
            
            # Check for redirect
            if grep -q "< HTTP/1.1 301" "${redirect_file}" && grep -q "< Location: ${expected_location}" "${redirect_file}"; then
                if [[ "${attempt}" -gt 1 ]]; then
                    print_result 0 "Response is a 301 redirect to ${expected_location} (succeeded on attempt ${attempt})"
                else
                    print_result 0 "Response is a 301 redirect to ${expected_location}"
                fi
                return 0
            else
                print_result 1 "Response is not a redirect to ${expected_location}"
                print_message "Response headers:"
                # Use process substitution to avoid subshell issue with OUTPUT_COLLECTION
                while IFS= read -r line; do
                    print_output "${line}"
                done < <(grep -E "< HTTP/|< Location:" "${redirect_file}" || true)
                return 1
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

# Function to check swagger.json file content with retry logic for subsystem readiness
check_swagger_json() {
    local url="$1"
    local response_file="$2"
    
    print_command "curl -s --max-time 10 \"${url}\""
    
    # Retry logic for subsystem readiness (especially important in parallel execution)
    local max_attempts=25
    local attempt=1
    local curl_exit_code=0
    
    while [[ "${attempt}" -le "${max_attempts}" ]]; do
        if [[ "${attempt}" -gt 1 ]]; then
            print_message "Swagger JSON request attempt ${attempt} of ${max_attempts} (waiting for subsystem initialization)..."
            # sleep 0.05  # Brief delay between attempts for subsystem initialization
        fi
        
        # Run curl and capture exit code
        curl -s --max-time 10 "${url}" > "${response_file}"
        curl_exit_code=$?
        
        if [[ "${curl_exit_code}" -eq 0 ]]; then
            # Check if we got a 404 or other error response
            if grep -q "404 Not Found" "${response_file}" || grep -q "<html>" "${response_file}"; then
                if [[ "${attempt}" -eq "${max_attempts}" ]]; then
                    print_message "Swagger endpoint still not ready after ${max_attempts} attempts"
                    print_result 1 "Swagger endpoint returned 404 or HTML error page"
                    print_message "Response content:"
                    print_output "$(cat "${response_file}" || true)"
                    return 1
                else
                    print_message "Swagger endpoint not ready yet (got 404/HTML), retrying..."
                    ((attempt++))
                    continue
                fi
            fi
            
            print_message "Successfully received swagger.json from ${url}"
            
            # Check if it's valid JSON and contains expected swagger content
            # Use jq to validate JSON and check for required fields
            if jq -e '.openapi // .swagger' "${response_file}" >/dev/null 2>&1; then
                local openapi_version
                openapi_version=$(jq -r '.openapi // .swagger // "unknown"' "${response_file}")
                print_message "Valid OpenAPI/Swagger specification found (version: ${openapi_version})"
                
                # Check for required Hydrogen API components
                if jq -e '.info.title' "${response_file}" >/dev/null 2>&1; then
                    local api_title
                    api_title=$(jq -r '.info.title' "${response_file}")
                    print_message "API Title: ${api_title}"
                    
                    if [[ "${api_title}" == *"Hydrogen"* ]]; then
                        if [[ "${attempt}" -gt 1 ]]; then
                            print_result 0 "swagger.json contains valid Hydrogen API specification (succeeded on attempt ${attempt})"
                        else
                            print_result 0 "swagger.json contains valid Hydrogen API specification"
                        fi
                        return 0
                    else
                        print_result 1 "swagger.json doesn't appear to be for Hydrogen API (title: ${api_title})"
                        return 1
                    fi
                else
                    print_result 1 "swagger.json missing required 'info.title' field"
                    return 1
                fi
            else
                print_result 1 "swagger.json contains invalid JSON or missing OpenAPI/Swagger version"
                return 1
            fi
        else
            if [[ "${attempt}" -eq "${max_attempts}" ]]; then
                print_result 1 "Failed to retrieve swagger.json from ${url} (curl exit code: ${curl_exit_code})"
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

# Function to test Swagger UI configuration in parallel
run_swagger_test_parallel() {
    local test_name="$1"
    local config_file="$2"
    local log_suffix="$3"
    local swagger_prefix="$4"
    local description="$5"
    
    local log_file="${LOGS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}_${log_suffix}.log"
    local result_file="${LOG_PREFIX}${TIMESTAMP}_${log_suffix}.result"
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
            
            local base_url="http://localhost:${port}"
            local all_tests_passed=true
            
            # Test Swagger UI with trailing slash
            local trailing_file="${RESULTS_DIR}/${log_suffix}_trailing_slash_${TIMESTAMP}.html"
            if check_response_content "${base_url}${swagger_prefix}/" "swagger-ui" "${trailing_file}" "true"; then
                echo "TRAILING_SLASH_TEST_PASSED" >> "${result_file}"
            else
                echo "TRAILING_SLASH_TEST_FAILED" >> "${result_file}"
                all_tests_passed=false
            fi
            
            # Test redirect without trailing slash
            local redirect_file="${RESULTS_DIR}/${log_suffix}_redirect_${TIMESTAMP}.txt"
            if check_redirect_response "${base_url}${swagger_prefix}" "${swagger_prefix}/" "${redirect_file}"; then
                echo "REDIRECT_TEST_PASSED" >> "${result_file}"
            else
                echo "REDIRECT_TEST_FAILED" >> "${result_file}"
                all_tests_passed=false
            fi
            
            # Test Swagger UI content
            local content_file="${RESULTS_DIR}/${log_suffix}_content_${TIMESTAMP}.html"
            if check_response_content "${base_url}${swagger_prefix}/" "swagger-ui" "${content_file}" "true"; then
                echo "CONTENT_TEST_PASSED" >> "${result_file}"
            else
                echo "CONTENT_TEST_FAILED" >> "${result_file}"
                all_tests_passed=false
            fi
            
            # Test JavaScript initializer
            local js_file="${RESULTS_DIR}/${log_suffix}_initializer_${TIMESTAMP}.js"
            if check_response_content "${base_url}${swagger_prefix}/swagger-initializer.js" "window.ui = SwaggerUIBundle" "${js_file}" "true"; then
                echo "JAVASCRIPT_TEST_PASSED" >> "${result_file}"
            else
                echo "JAVASCRIPT_TEST_FAILED" >> "${result_file}"
                all_tests_passed=false
            fi
            
            # Test swagger.json
            local swagger_json_file="${RESULTS_DIR}/${log_suffix}_swagger_json_${TIMESTAMP}.json"
            if check_swagger_json "${base_url}${swagger_prefix}/swagger.json" "${swagger_json_file}"; then
                echo "SWAGGER_JSON_TEST_PASSED" >> "${result_file}"
            else
                echo "SWAGGER_JSON_TEST_FAILED" >> "${result_file}"
                all_tests_passed=false
            fi
            
            if [[ "${all_tests_passed}" = true ]]; then
                echo "ALL_SWAGGER_TESTS_PASSED" >> "${result_file}"
            else
                echo "SOME_SWAGGER_TESTS_FAILED" >> "${result_file}"
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

# Function to analyze results from parallel Swagger test execution
analyze_swagger_test_results() {
    local test_name="$1"
    local log_suffix="$2"
    local swagger_prefix="$3"
    local description="$4"
    local result_file="${LOG_PREFIX}${TIMESTAMP}_${log_suffix}.result"

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
    
    # Check individual Swagger tests
    local trailing_slash_passed=false
    local redirect_passed=false
    local content_passed=false
    local javascript_passed=false
    local swagger_json_passed=false
    
    if grep -q "TRAILING_SLASH_TEST_PASSED" "${result_file}" 2>/dev/null; then
        trailing_slash_passed=true
    fi
    
    if grep -q "REDIRECT_TEST_PASSED" "${result_file}" 2>/dev/null; then
        redirect_passed=true
    fi
    
    if grep -q "CONTENT_TEST_PASSED" "${result_file}" 2>/dev/null; then
        content_passed=true
    fi
    
    if grep -q "JAVASCRIPT_TEST_PASSED" "${result_file}" 2>/dev/null; then
        javascript_passed=true
    fi
    
    if grep -q "SWAGGER_JSON_TEST_PASSED" "${result_file}" 2>/dev/null; then
        swagger_json_passed=true
    fi
    
    # Return results via global variables for detailed reporting
    TRAILING_SLASH_TEST_RESULT=${trailing_slash_passed}
    REDIRECT_TEST_RESULT=${redirect_passed}
    CONTENT_TEST_RESULT=${content_passed}
    JAVASCRIPT_TEST_RESULT=${javascript_passed}
    SWAGGER_JSON_TEST_RESULT=${swagger_json_passed}
    
    # Return success only if all tests passed
    if grep -q "ALL_SWAGGER_TESTS_PASSED" "${result_file}" 2>/dev/null; then
        return 0
    else
        return 1
    fi
}

# Function to test Swagger UI with a specific configuration (kept for compatibility)
test_swagger_configuration() {
    local config_file="$1"
    local swagger_prefix="$2"
    local test_name="$3"
    local config_number="$4"
    
    print_message "Testing Swagger UI: ${swagger_prefix} (using ${test_name})"
    
    # Extract port from configuration
    local server_port
    server_port=$(get_webserver_port "${config_file}")
    print_message "Configuration will use port: ${server_port}"
    
    # Global variables for server management
    local hydrogen_pid=""
    local server_log="${RESULTS_DIR}/swagger_${test_name}_${TIMESTAMP}.log"
    local base_url="http://localhost:${server_port}"
    
    # Start server
    local subtest_start=$(((config_number - 1) * 6 + 1))
    
    print_subtest "Start Hydrogen Server (Config ${config_number})"
    
    # Use a temporary variable name that won't conflict
    local temp_pid_var="HYDROGEN_PID_$$"
    if start_hydrogen_with_pid "${config_file}" "${server_log}" 15 "${HYDROGEN_BIN}" "${temp_pid_var}"; then
        # Get the PID from the temporary variable
        hydrogen_pid=$(eval "echo \$${temp_pid_var}")
        if [[ -n "${hydrogen_pid}" ]]; then
            print_result 0 "Server started successfully with PID: ${hydrogen_pid}"
            ((PASS_COUNT++))
        else
            print_result 1 "Failed to start server - no PID returned"
            EXIT_CODE=1
            # Skip remaining subtests for this configuration
            for i in {2..6}; do

                print_subtest "Subtest $((subtest_start + i - 1)) (Skipped)"
                print_result 1 "Skipped due to server startup failure"

            done
            return 1
        fi
    else
        print_result 1 "Failed to start server"
        EXIT_CODE=1
        # Skip remaining subtests for this configuration
        for i in {2..6}; do

            print_subtest "Subtest $((subtest_start + i - 1)) (Skipped)"
            print_result 1 "Skipped due to server startup failure"

        done
        return 1
    fi
    
    # Wait for server to be ready
    if [[ -n "${hydrogen_pid}" ]] && ps -p "${hydrogen_pid}" > /dev/null 2>&1; then
        if ! wait_for_server_ready "${base_url}"; then
            print_result 1 "Server failed to become ready"
            EXIT_CODE=1
            # Skip remaining subtests
            for i in {2..6}; do

                print_subtest "Subtest $((subtest_start + i - 1)) (Skipped)"
                print_result 1 "Skipped due to server readiness failure"

            done
            return 1
        fi
    fi
    
    print_subtest "Access Swagger UI with trailing slash (Config ${config_number})"

    if [[ -n "${hydrogen_pid}" ]] && ps -p "${hydrogen_pid}" > /dev/null 2>&1; then
        if check_response_content "${base_url}${swagger_prefix}/" "swagger-ui" "${RESULTS_DIR}/${test_name}_trailing_slash_${TIMESTAMP}.html" "true"; then
            ((PASS_COUNT++))
        else
            EXIT_CODE=1
        fi
    else
        print_result 1 "Server not running for trailing slash test"
        EXIT_CODE=1
    fi
    
    print_subtest "Access Swagger UI without trailing slash (Config ${config_number})"

    if [[ -n "${hydrogen_pid}" ]] && ps -p "${hydrogen_pid}" > /dev/null 2>&1; then
        if check_redirect_response "${base_url}${swagger_prefix}" "${swagger_prefix}/" "${RESULTS_DIR}/${test_name}_redirect_${TIMESTAMP}.txt"; then
            ((PASS_COUNT++))
        else
            EXIT_CODE=1
        fi
    else
        print_result 1 "Server not running for redirect test"
        EXIT_CODE=1
    fi
    
    print_subtest "Verify Swagger UI content loads (Config ${config_number})"

    if [[ -n "${hydrogen_pid}" ]] && ps -p "${hydrogen_pid}" > /dev/null 2>&1; then
        if check_response_content "${base_url}${swagger_prefix}/" "swagger-ui" "${RESULTS_DIR}/${test_name}_content_${TIMESTAMP}.html" "true"; then
            ((PASS_COUNT++))
        else
            EXIT_CODE=1
        fi
    else
        print_result 1 "Server not running for content test"
        EXIT_CODE=1
    fi
    
    print_subtest "Verify JavaScript files load (Config ${config_number})"

    if [[ -n "${hydrogen_pid}" ]] && ps -p "${hydrogen_pid}" > /dev/null 2>&1; then
        if check_response_content "${base_url}${swagger_prefix}/swagger-initializer.js" "window.ui = SwaggerUIBundle" "${RESULTS_DIR}/${test_name}_initializer_${TIMESTAMP}.js" "true"; then
            ((PASS_COUNT++))
        else
            EXIT_CODE=1
        fi
    else
        print_result 1 "Server not running for JavaScript test"
        EXIT_CODE=1
    fi
    
    print_subtest "Verify swagger.json file loads and contains valid content (Config ${config_number})"

    if [[ -n "${hydrogen_pid}" ]] && ps -p "${hydrogen_pid}" > /dev/null 2>&1; then
        if check_swagger_json "${base_url}${swagger_prefix}/swagger.json" "${RESULTS_DIR}/${test_name}_swagger_json_${TIMESTAMP}.json"; then
            ((PASS_COUNT++))
        else
            EXIT_CODE=1
        fi
    else
        print_result 1 "Server not running for swagger.json test"
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
for test_config in "${!SWAGGER_TEST_CONFIGS[@]}"; do
    # Parse test configuration
    IFS=':' read -r config_file log_suffix swagger_prefix description <<< "${SWAGGER_TEST_CONFIGS[${test_config}]}"
    
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

# Only proceed with Swagger tests if prerequisites are met
if [[ "${EXIT_CODE}" -eq 0 ]]; then
    # Ensure clean state
    print_message "Ensuring no existing Hydrogen processes are running..."
    pkill -f "hydrogen.*json" 2>/dev/null || true
    
    print_message "Running Swagger tests in parallel for faster execution..."
    
    # Start all Swagger tests in parallel with job limiting
    for test_config in "${!SWAGGER_TEST_CONFIGS[@]}"; do
        # shellcheck disable=SC2312 # Job control with wc -l is standard practice
        while (( $(jobs -r | wc -l) >= CORES )); do
            wait -n  # Wait for any job to finish
        done
        
        # Parse test configuration
        IFS=':' read -r config_file log_suffix swagger_prefix description <<< "${SWAGGER_TEST_CONFIGS[${test_config}]}"
        
        print_message "Starting parallel test: ${test_config} (${description})"
        
        # Run parallel Swagger test in background
        run_swagger_test_parallel "${test_config}" "${config_file}" "${log_suffix}" "${swagger_prefix}" "${description}" &
        PARALLEL_PIDS+=($!)
    done
    
    # Wait for all parallel tests to complete
    print_message "Waiting for all ${#SWAGGER_TEST_CONFIGS[@]} parallel Swagger tests to complete..."
    for pid in "${PARALLEL_PIDS[@]}"; do
        wait "${pid}"
    done
    print_message "All parallel tests completed, analyzing results..."
    
    # Process results sequentially for clean output
    for test_config in "${!SWAGGER_TEST_CONFIGS[@]}"; do
        # Parse test configuration
        IFS=':' read -r config_file log_suffix swagger_prefix description <<< "${SWAGGER_TEST_CONFIGS[${test_config}]}"
        
        print_subtest "Swagger UI Test: ${description} (${swagger_prefix})"
        
        if analyze_swagger_test_results "${test_config}" "${log_suffix}" "${swagger_prefix}" "${description}"; then
            # Test individual endpoint results for detailed feedback
            print_subtest "Trailing Slash Access - ${description}"
            if [[ "${TRAILING_SLASH_TEST_RESULT}" = true ]]; then
                print_result 0 "Swagger UI with trailing slash test passed"
                ((PASS_COUNT++))
            else
                print_result 1 "Swagger UI with trailing slash test failed"
                EXIT_CODE=1
            fi
            
            print_subtest "Redirect Test - ${description}"
            if [[ "${REDIRECT_TEST_RESULT}" = true ]]; then
                print_result 0 "Swagger UI redirect test passed"
                ((PASS_COUNT++))
            else
                print_result 1 "Swagger UI redirect test failed"
                EXIT_CODE=1
            fi
            
            print_subtest "Content Verification - ${description}"
            if [[ "${CONTENT_TEST_RESULT}" = true ]]; then
                print_result 0 "Swagger UI content test passed"
                ((PASS_COUNT++))
            else
                print_result 1 "Swagger UI content test failed"
                EXIT_CODE=1
            fi
            
            print_subtest "JavaScript Loading - ${description}"
            if [[ "${JAVASCRIPT_TEST_RESULT}" = true ]]; then
                print_result 0 "JavaScript initializer test passed"
                ((PASS_COUNT++))
            else
                print_result 1 "JavaScript initializer test failed"
                EXIT_CODE=1
            fi
            
            print_subtest "Swagger JSON Validation - ${description}"
            if [[ "${SWAGGER_JSON_TEST_RESULT}" = true ]]; then
                print_result 0 "Swagger JSON test passed"
                ((PASS_COUNT++))
            else
                print_result 1 "Swagger JSON test failed"
                EXIT_CODE=1
            fi
            
            print_message "${description}: All Swagger tests passed"
        else
            print_result 1 "${description} test failed"
            EXIT_CODE=1
        fi
    done
    
    # Print summary
    successful_configs=0
    for test_config in "${!SWAGGER_TEST_CONFIGS[@]}"; do
        IFS=':' read -r config_file log_suffix swagger_prefix description <<< "${SWAGGER_TEST_CONFIGS[${test_config}]}"
        result_file="${LOG_PREFIX}${TIMESTAMP}_${log_suffix}.result"
        if [[ -f "${result_file}" ]] && grep -q "ALL_SWAGGER_TESTS_PASSED" "${result_file}" 2>/dev/null; then
            ((successful_configs++))
        fi
    done
    
    print_message "Summary: ${successful_configs}/${#SWAGGER_TEST_CONFIGS[@]} Swagger configurations passed all tests"
    print_message "Parallel execution completed - SO_REUSEADDR allows immediate port reuse"
    
else
    # Skip Swagger tests if prerequisites failed
    print_message "Skipping Swagger tests due to prerequisite failures"
    EXIT_CODE=1
fi

# Clean up response files but preserve logs if test failed
rm -f "${RESULTS_DIR}"/*_trailing_slash_*.html "${RESULTS_DIR}"/*_redirect_*.txt "${RESULTS_DIR}"/*_content_*.html "${RESULTS_DIR}"/*_initializer_*.js "${RESULTS_DIR}"/*_swagger_json_*.json

# Print test completion summary
print_test_completion "${TEST_NAME}" "${TEST_ABBR}" "${TEST_NUMBER}" "${TEST_VERSION}"

# Return status code if sourced, exit if run standalone
${ORCHESTRATION:-false} && return "${EXIT_CODE}" || exit "${EXIT_CODE}"
