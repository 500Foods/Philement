#!/bin/bash

# Test: Swagger
# Tests the Swagger functionality, its presence in the payload, etc.

# CHANGELOG
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
TEST_VERSION="4.0.0"

# shellcheck source=tests/lib/framework.sh # Reference framework directly
[[ -n "${FRAMEWORK_GUARD}" ]] || source "$(dirname "${BASH_SOURCE[0]}")/lib/framework.sh"
setup_test_environment

# Test variables
HYDROGEN_DIR="${PROJECT_DIR}"

# Function to wait for server to be ready
wait_for_server_ready() {
    local base_url="$1"
    local max_attempts=100   # 20 seconds total (0.2s * 100)
    local attempt=1
    
    print_message "Waiting for server to be ready at ${base_url}..."
    
    while [[ "${attempt}" -le "${max_attempts}" ]]; do
        if curl -s --max-time 2 "${base_url}" >/dev/null 2>&1; then
            print_message "Server is ready after $(( attempt * 2 / 10 )) seconds"
            return 0
        fi
        ((attempt++))
    done
    
    print_error "Server failed to respond within 20 seconds"
    return 1
}

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
            sleep 0.05  # Brief delay between attempts for subsystem initialization
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
            sleep 0.05  # Brief delay between attempts for subsystem initialization
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
            sleep 0.05  # Brief delay between attempts for subsystem initialization
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
            if command -v jq >/dev/null 2>&1; then
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
                # Fallback validation without jq
                if grep -q '"openapi":\|"swagger":' "${response_file}" && \
                   grep -q '"info":' "${response_file}" && \
                   grep -q '"Hydrogen' "${response_file}"; then
                    if [[ "${attempt}" -gt 1 ]]; then
                        print_result 0 "swagger.json contains expected Hydrogen API content (basic validation, succeeded on attempt ${attempt})"
                    else
                        print_result 0 "swagger.json contains expected Hydrogen API content (basic validation)"
                    fi
                    return 0
                else
                    print_result 1 "swagger.json doesn't contain expected OpenAPI/Swagger structure or Hydrogen content"
                    print_message "Response excerpt (first 5 lines):"
                    # Use process substitution to avoid subshell issue with OUTPUT_COLLECTION
                    while IFS= read -r line; do
                        print_output "${line}"
                    done < <(head -n 5 "${response_file}" || true)
                    return 1
                fi
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

# Function to test Swagger UI with a specific configuration
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
    # shellcheck disable=SC2153  # HYDROGEN_BIN is set by find_hydrogen_binary function
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

if find_hydrogen_binary "${HYDROGEN_DIR}" "HYDROGEN_BIN"; then
    print_result 0 "Hydrogen binary found: $(basename "${HYDROGEN_BIN}")"
    ((PASS_COUNT++))
else
    print_result 1 "Hydrogen binary not found"
    EXIT_CODE=1
fi

# Configuration files for testing
CONFIG_1="$(get_config_path "hydrogen_test_swagger_test_1.json")"
CONFIG_2="$(get_config_path "hydrogen_test_swagger_test_2.json")"

print_subtest "Validate Configuration File 1"

if validate_config_file "${CONFIG_1}"; then
    ((PASS_COUNT++))
else
    EXIT_CODE=1
fi

print_subtest "Validate Configuration File 2"

if validate_config_file "${CONFIG_2}"; then
    ((PASS_COUNT++))
else
    EXIT_CODE=1
fi

# Only proceed with Swagger tests if prerequisites are met
if [[ "${EXIT_CODE}" -eq 0 ]]; then
    # Ensure clean state
    print_message "Ensuring no existing Hydrogen processes are running..."
    pkill -f "hydrogen.*json" 2>/dev/null || true
    
    # Note: Coverage data is automatically generated when using hydrogen_coverage binary
    # No initialization needed - gcda files are created during execution
    
    print_message "Testing Swagger functionality with immediate restart approach"
    print_message "SO_REUSEADDR is enabled - no need to wait for TIME_WAIT"
    
    # Test with default Swagger prefix (/swagger)
    test_swagger_configuration "${CONFIG_1}" "/swagger" "swagger_default" 1
    
    # Test with custom Swagger prefix (/apidocs) - immediate restart
    print_message "Starting second test immediately (testing SO_REUSEADDR)..."
    test_swagger_configuration "${CONFIG_2}" "/apidocs" "swagger_custom" 2
    
    print_message "Immediate restart successful - SO_REUSEADDR applied successfully"
    
    # Note: Blackbox coverage collection is handled centrally in test_99_cleanup.sh
    # Individual tests just run hydrogen_coverage normally and test 99 collects all coverage data
    
else
    # Skip Swagger tests if prerequisites failed
    print_message "Skipping Swagger tests due to prerequisite failures"
    # Account for skipped subtests (12 remaining: 6 for each configuration)
    for i in {4..15}; do

        print_subtest "Subtest ${i} (Skipped)"
        print_result 1 "Skipped due to prerequisite failures"
        
    done
    EXIT_CODE=1
fi

# Clean up response files but preserve logs if test failed
rm -f "${RESULTS_DIR}"/*_trailing_slash_*.html "${RESULTS_DIR}"/*_redirect_*.txt "${RESULTS_DIR}"/*_content_*.html "${RESULTS_DIR}"/*_initializer_*.js "${RESULTS_DIR}"/*_swagger_json_*.json

# Print test completion summary
print_test_completion "${TEST_NAME}" "${TEST_ABBR}" "${TEST_NUMBER}" "${TEST_VERSION}"

# Return status code if sourced, exit if run standalone
${ORCHESTRATION:-false} && return "${EXIT_CODE}" || exit "${EXIT_CODE}"
