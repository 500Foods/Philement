#!/usr/bin/env bash

# Test: Swagger
# Tests the Swagger functionality, its presence in the payload, etc.

# FUNCTIONS (others moved to lib/ files)
# run_swagger_test_parallel()
# analyze_swagger_test_results()
# test_swagger_configuration()

# CHANGELOG
# 7.4.0 - 2026-01-06 - Added custom headers test to verify WebServer.Headers configuration is applied to responses.
#                    - Tests both configurations: one with simple headers and one with CORS headers.
#                    - Verifies headers match patterns (wildcard * and extension-specific like .js).
# 7.3.0 - 2025-09-19 - Major refactoring to reduce script size from 951 to ~750 lines.
#                    - Moved timing functions (handle_timing_file, collect_timing_data, test_file_download) to file_utils.sh.
#                    - Moved HTTP functions (check_response_content, check_redirect_response, check_swagger_json) to network_utils.sh.
#                    - Updated function references and library sourcing.
#                    - Maintained all existing functionality and test coverage.
# 7.2.0 - 2025-09-19 - Moved curl_with_retry to network_utils.sh library for reuse across tests.
#                    - Fixed shellcheck errors and improved code quality.
#                    - Maintained all existing functionality and test coverage.
# 7.0.0 - 2025-09-19 - Major refactoring to reduce code size and eliminate duplication.
#                    - Created curl_with_retry() function to consolidate retry logic across all HTTP functions.
#                    - Added handle_timing_file() for standardized timing data processing.
#                    - Added test_file_download() and collect_timing_data() helper functions.
#                    - Refactored check_response_content(), check_redirect_response(), and check_swagger_json() to use new helpers.
#                    - Reduced script size from 1047 to 971 lines (7.3% reduction) to meet 1000-line build limit.
#                    - Maintained all existing functionality and test coverage.
# 6.0.0 - 2025-09-19 - Added average response time calculation and display in test name
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

set -euo pipefail

# Test Configuration
TEST_NAME="Swagger"
TEST_ABBR="SWG"
TEST_NUMBER="22"
TEST_COUNTER=0
TEST_VERSION="7.4.0"

# shellcheck source=tests/lib/framework.sh # Reference framework directly
[[ -n "${FRAMEWORK_GUARD:-}" ]] || source "$(dirname "${BASH_SOURCE[0]}")/lib/framework.sh"
setup_test_environment

# Parallel execution configuration
declare -a PARALLEL_PIDS
declare -A SWAGGER_TEST_CONFIGS

# Swagger test configuration - format: "config_file:log_suffix:swagger_prefix:description"
SWAGGER_TEST_CONFIGS=(
    ["ONE"]="${SCRIPT_DIR}/configs/hydrogen_test_${TEST_NUMBER}_swagger_1.json:one:/swagger:Swagger Prefix One"
    ["TWO"]="${SCRIPT_DIR}/configs/hydrogen_test_${TEST_NUMBER}_swagger_2.json:two:/apidocs:Swagger Prefix Two"
)

# Test timeouts
STARTUP_TIMEOUT=15
SHUTDOWN_TIMEOUT=10


# Function to test Swagger UI configuration in parallel
run_swagger_test_parallel() {
    local test_name="$1"
    local config_file="$2"
    local log_suffix="$3"
    local swagger_prefix="$4"
    local description="$5"
    
    local log_file="${LOGS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}_${log_suffix}.log"
    local result_file="${LOG_PREFIX}_${log_suffix}.result"
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
        
        if "${GREP}" -q "STARTUP COMPLETE" "${log_file}" 2>/dev/null; then
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
            
            local base_url="http://localhost:${port}"
            local all_tests_passed=true
            
            # Test Swagger UI with trailing slash
            local trailing_file="${LOG_PREFIX}_${log_suffix}_trailing_slash.html"
            local trailing_timing_file="${LOG_PREFIX}_${log_suffix}_trailing_slash.timing"
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Testing Swagger UI with trailing slash: ${base_url}${swagger_prefix}/"
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Output file: ${trailing_file}"
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Timing file: ${trailing_timing_file}"
            # shellcheck disable=SC2310 # We want to continue even if the test fails
            if check_response_content "${base_url}${swagger_prefix}/" "swagger-ui" "${trailing_file}" "true" "${trailing_timing_file}"; then
                echo "TRAILING_SLASH_TEST_PASSED" >> "${result_file}"
                print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "✓ Trailing slash test PASSED"
            else
                echo "TRAILING_SLASH_TEST_FAILED" >> "${result_file}"
                print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "✗ Trailing slash test FAILED"
                all_tests_passed=false
            fi
            
            # Test redirect without trailing slash
            local redirect_file="${LOG_PREFIX}_${log_suffix}_redirect.txt"
            local redirect_timing_file="${LOG_PREFIX}_${log_suffix}_redirect.timing"
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Testing redirect without trailing slash: ${base_url}${swagger_prefix}"
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Expected redirect to: ${swagger_prefix}/"
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Output file: ${redirect_file}"
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Timing file: ${redirect_timing_file}"
            # shellcheck disable=SC2310 # We want to continue even if the test fails
            if check_redirect_response "${base_url}${swagger_prefix}" "${swagger_prefix}/" "${redirect_file}" "${redirect_timing_file}"; then
                echo "REDIRECT_TEST_PASSED" >> "${result_file}"
                print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "✓ Redirect test PASSED"
            else
                echo "REDIRECT_TEST_FAILED" >> "${result_file}"
                print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "✗ Redirect test FAILED"
                all_tests_passed=false
            fi

            # Test exact prefix redirect (coverage: lines 163-174) - critical for redirect logic
            local exact_redirect_file="${LOG_PREFIX}_${log_suffix}_exact_redirect.txt"
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Testing exact prefix redirect: ${base_url}${swagger_prefix}"
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Output file: ${exact_redirect_file}"
            print_command "${TEST_NUMBER}" "${TEST_COUNTER}" "curl -v -s --max-time 10 -w '%{http_code}' -o /dev/null \"${base_url}${swagger_prefix}\""
            local http_code
            http_code=$(curl -v -s --max-time 10 -w '%{http_code}' -o /dev/null "${base_url}${swagger_prefix}" 2>"${exact_redirect_file}")
            if [[ "${http_code}" == "301" ]] && "${GREP}" -q "< Location: ${swagger_prefix}/" "${exact_redirect_file}"; then
                echo "EXACT_REDIRECT_TEST_PASSED" >> "${result_file}"
                print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "✓ Exact prefix redirect working correctly (301 -> ${swagger_prefix}/)"
            else
                echo "EXACT_REDIRECT_TEST_FAILED" >> "${result_file}"
                print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "✗ Expected 301 redirect for exact prefix, got HTTP ${http_code}"
                if [[ -f "${exact_redirect_file}" ]]; then
                    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Response headers:"
                    while IFS= read -r line; do
                        print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "${line}"
                    done < <("${GREP}" -E "< HTTP/|< Location:" "${exact_redirect_file}" || true)
                fi
                all_tests_passed=false
            fi
            
            # Test Swagger UI content
            local content_file="${LOG_PREFIX}_${log_suffix}_content.html"
            local content_timing_file="${LOG_PREFIX}_${log_suffix}_content.timing"
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Testing Swagger UI content: ${base_url}${swagger_prefix}/"
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Looking for content: swagger-ui"
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Output file: ${content_file}"
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Timing file: ${content_timing_file}"
            # shellcheck disable=SC2310 # We want to continue even if the test fails
            if check_response_content "${base_url}${swagger_prefix}/" "swagger-ui" "${content_file}" "true" "${content_timing_file}"; then
                echo "CONTENT_TEST_PASSED" >> "${result_file}"
                print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "✓ Content test PASSED"
            else
                echo "CONTENT_TEST_FAILED" >> "${result_file}"
                print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "✗ Content test FAILED"
                all_tests_passed=false
            fi
            
            # Test JavaScript initializer
            local js_file="${LOG_PREFIX}_${log_suffix}_initializer.js"
            local js_timing_file="${LOG_PREFIX}_${log_suffix}_initializer.timing"
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Testing JavaScript initializer: ${base_url}${swagger_prefix}/swagger-initializer.js"
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Looking for content: window.ui = SwaggerUIBundle"
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Output file: ${js_file}"
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Timing file: ${js_timing_file}"
            # shellcheck disable=SC2310 # We want to continue even if the test fails
            if check_response_content "${base_url}${swagger_prefix}/swagger-initializer.js" "window.ui = SwaggerUIBundle" "${js_file}" "true" "${js_timing_file}"; then
                echo "JAVASCRIPT_TEST_PASSED" >> "${result_file}"
                print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "✓ JavaScript test PASSED"
                
                # Check for custom headers by fetching head of JavaScript file
                # Use the already-passed config_file from function parameter
                if command -v jq >/dev/null 2>&1 && [[ -f "${config_file}" ]]; then
                    local expected_headers_count
                    expected_headers_count=$(jq -r '.WebServer.Headers // [] | length' "${config_file}" 2>/dev/null || echo "0")
                    
                    if [[ "${expected_headers_count}" -gt 0 ]]; then
                        # Fetch headers for the JavaScript file (use -i with GET instead of -I HEAD to ensure we get the response with headers)
                        local headers_output
                        headers_output=$(curl -i -X GET --silent --max-time 10 "${base_url}${swagger_prefix}/swagger-initializer.js" 2>&1 | head -30 || true)
                        
                        # Check each header rule from config
                        local custom_headers_found=0
                        # shellcheck disable=SC2312 # this works and when it doesn't, well, we've got bigger problems
                        while IFS= read -r header_rule; do
                            local pattern header_name header_value
                            pattern=$(echo "${header_rule}" | jq -r '.[0]' 2>/dev/null || echo "")
                            header_name=$(echo "${header_rule}" | jq -r '.[1]' 2>/dev/null || echo "")
                            header_value=$(echo "${header_rule}" | jq -r '.[2]' 2>/dev/null || echo "")
                            
                            # Check if pattern matches .js files (either "*" or ".js")
                            if [[ "${pattern}" == "*" ]] || [[ "${pattern}" == ".js" ]]; then
                                # Check if header is in response
                                local received_value
                                received_value=$(echo "${headers_output}" | "${GREP}" -i "^${header_name}:" | head -1 | sed 's/^[^:]*:[[:space:]]*//' | tr -d '\r' || echo "")
                                if [[ -n "${received_value}" ]]; then
                                    # Do case-insensitive comparison after trimming whitespace
                                    if echo "${received_value}" | "${GREP}" -qixF "${header_value}"; then
                                        custom_headers_found=$((custom_headers_found + 1))
                                        echo "CUSTOM_HEADERS_TEST_PASSED" >> "${result_file}"
                                        # Save header check result for analysis phase
                                        echo "HEADER_CHECK:${header_name}:${header_value}:FOUND:${received_value}" >> "${result_file}"
                                    else
                                        # Save header check result for analysis phase
                                        echo "HEADER_CHECK:${header_name}:${header_value}:MISMATCH:${received_value}" >> "${result_file}"
                                    fi
                                else
                                    # Save header check result for analysis phase
                                    echo "HEADER_CHECK:${header_name}:${header_value}:NOT_FOUND:(not found)" >> "${result_file}"
                                fi
                            fi
                        done < <(jq -c '.WebServer.Headers[]' "${config_file}" 2>/dev/null || echo "")
                        
                        # Mark test as failed if we expected headers but didn't find any
                        if [[ "${custom_headers_found}" -eq 0 ]] && [[ "${expected_headers_count}" -gt 0 ]]; then
                            echo "CUSTOM_HEADERS_TEST_FAILED" >> "${result_file}"
                        elif [[ "${custom_headers_found}" -gt 0 ]]; then
                            # Already marked as passed above
                            true
                        else
                            # No headers expected and none found
                            echo "CUSTOM_HEADERS_TEST_PASSED" >> "${result_file}"
                        fi
                    else
                        # No custom headers configured
                        echo "CUSTOM_HEADERS_TEST_PASSED" >> "${result_file}"
                    fi
                else
                    # jq not available or config not found
                    echo "CUSTOM_HEADERS_TEST_PASSED" >> "${result_file}"
                fi
            else
                echo "JAVASCRIPT_TEST_FAILED" >> "${result_file}"
                print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "✗ JavaScript test FAILED"
                all_tests_passed=false
            fi
            
            # Test swagger.json
            local swagger_json_file="${LOG_PREFIX}_${log_suffix}_swagger_json_.json"
            local swagger_json_timing_file="${LOG_PREFIX}_${log_suffix}_swagger_json.timing"
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Testing swagger.json: ${base_url}${swagger_prefix}/swagger.json"
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Output file: ${swagger_json_file}"
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Timing file: ${swagger_json_timing_file}"
            # shellcheck disable=SC2310 # We want to continue even if the test fails
            if check_swagger_json "${base_url}${swagger_prefix}/swagger.json" "${swagger_json_file}" "${swagger_json_timing_file}"; then
                echo "SWAGGER_JSON_TEST_PASSED" >> "${result_file}"
                print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "✓ Swagger JSON test PASSED"

                # Validate swagger.json format using jsonlint (test_93)
                if command -v jsonlint >/dev/null 2>&1; then
                    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Running jsonlint validation on swagger.json"
                    if jsonlint -q "${swagger_json_file}" >/dev/null 2>&1; then
                        echo "SWAGGER_JSON_LINT_PASSED" >> "${result_file}"
                        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "✓ JSON lint validation PASSED"
                    else
                        echo "SWAGGER_JSON_LINT_FAILED" >> "${result_file}"
                        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "✗ JSON lint validation FAILED"
                        all_tests_passed=false
                    fi
                else
                    echo "JSONLINT_NOT_AVAILABLE" >> "${result_file}"
                    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "! jsonlint not available, skipping validation"
                fi
            else
                echo "SWAGGER_JSON_TEST_FAILED" >> "${result_file}"
                print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "✗ Swagger JSON test FAILED"
                all_tests_passed=false
            fi

            # Test file downloads using helper function
            test_file_download "${base_url}${swagger_prefix}/swagger-ui.css" "${LOG_PREFIX}_${log_suffix}_swagger_css.css" "CSS" "CSS_FILE_TEST" "${result_file}" || all_tests_passed=false
            test_file_download "${base_url}${swagger_prefix}/favicon-32x32.png" "${LOG_PREFIX}_${log_suffix}_swagger_png.png" "PNG" "PNG_FILE_TEST" "${result_file}" || all_tests_passed=false

            # Test Brotli compressed file handling (coverage: lines 463-464)
            local br_file="${LOG_PREFIX}_${log_suffix}_swagger_br.br"
            print_command "${TEST_NUMBER}" "${TEST_COUNTER}" "curl -s -H \"Accept-Encoding: br\" --max-time 10 \"${base_url}${swagger_prefix}/swagger-ui.css.br\""
            if curl -s -H "Accept-Encoding: br" --max-time 10 "${base_url}${swagger_prefix}/swagger-ui.css.br" > "${br_file}"; then
                if [[ -s "${br_file}" ]]; then
                    echo "BR_FILE_TEST_PASSED" >> "${result_file}"
                    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Successfully downloaded Brotli compressed file ($(wc -c < "${br_file}" || true) bytes)"
                else
                    echo "BR_FILE_TEST_FAILED" >> "${result_file}"
                    all_tests_passed=false
                fi
            else
                echo "BR_FILE_TEST_FAILED" >> "${result_file}"
                all_tests_passed=false
            fi

            # Test 404 error handling for non-existent files
            # local notfound_file="${LOG_PREFIX}${TIMESTAMP}_${log_suffix}_404.txt"
            print_command "${TEST_NUMBER}" "${TEST_COUNTER}" "curl --silent --max-time 10 --write-out '%{http_code}' \"${base_url}${swagger_prefix}/nonexistent.file\""

            # Use curl with set +e/-e to avoid script exit on failure
            local http_code=""
            local curl_exit_code=0
            set +e
            http_code=$(curl --silent --max-time 10 --write-out '%{http_code}' "${base_url}${swagger_prefix}/nonexistent.file" 2>/dev/null)
            curl_exit_code=$?
            set -e

            # Check if we got a response or if connection was rejected
            if [[ "${curl_exit_code}" -eq 0 ]] && [[ "${http_code}" != "000" ]]; then
                # Connection succeeded and we got a valid HTTP response
                if [[ "${http_code}" == "404" ]] || [[ "${http_code}" == "500" ]]; then
                    echo "NOT_FOUND_TEST_PASSED" >> "${result_file}"
                    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "404/500 error handling working correctly for non-existent files (got HTTP ${http_code})"
                else
                    echo "NOT_FOUND_TEST_FAILED" >> "${result_file}"
                    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Expected 404/500 for non-existent file, but got HTTP ${http_code}"
                    all_tests_passed=false
                fi
            elif [[ "${curl_exit_code}" -ne 0 ]] || [[ "${http_code}" == "000" ]]; then
                # Curl exited with error or got HTTP 000 - application likely rejected the request properly
                echo "NOT_FOUND_TEST_PASSED" >> "${result_file}"
                print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Application correctly rejected invalid file request (connection closed/failed as expected)"
            else
                # Unexpected state - treat as failure
                echo "NOT_FOUND_TEST_FAILED" >> "${result_file}"
                print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Unexpected response for non-existent file (HTTP ${http_code}, exit ${curl_exit_code})"
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

# Function to analyze results from parallel Swagger test execution
analyze_swagger_test_results() {
    local test_name="$1"
    local log_suffix="$2"
    local swagger_prefix="$3"
    local description="$4"
    local result_file="${LOG_PREFIX}_${log_suffix}.result"

    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Analyzing results for ${description}"
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Result file: ${result_file}"

    if [[ ! -f "${result_file}" ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "No result file found for ${test_name}"
        return 1
    fi

    # print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Result file contents:"
    # while IFS= read -r line; do
    #     print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "${line}"
    # done < "${result_file}"

    # Check startup
    if ! "${GREP}" -q "STARTUP_SUCCESS" "${result_file}" 2>/dev/null; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Failed to start Hydrogen for ${description} test"
        return 1
    fi

    # Check server readiness
    if ! "${GREP}" -q "SERVER_READY" "${result_file}" 2>/dev/null; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Server not ready for ${description} test"
        return 1
    fi

    # Check individual Swagger tests
    local trailing_slash_passed=false
    local redirect_passed=false
    local content_passed=false
    local javascript_passed=false
    local swagger_json_passed=false
    local custom_headers_passed=false

    # print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Checking individual test results:"

    if "${GREP}" -q "TRAILING_SLASH_TEST_PASSED" "${result_file}" 2>/dev/null; then
        trailing_slash_passed=true
        # print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "✓ Trailing slash test: PASSED"
    # else
        # print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "✗ Trailing slash test: FAILED"
    fi

    if "${GREP}" -q "REDIRECT_TEST_PASSED" "${result_file}" 2>/dev/null; then
        redirect_passed=true
        # print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "✓ Redirect test: PASSED"
    # else
        # print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "✗ Redirect test: FAILED"
    fi

    if "${GREP}" -q "CONTENT_TEST_PASSED" "${result_file}" 2>/dev/null; then
        content_passed=true
        # print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "✓ Content test: PASSED"
    # else
        # print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "✗ Content test: FAILED"
    fi

    if "${GREP}" -q "JAVASCRIPT_TEST_PASSED" "${result_file}" 2>/dev/null; then
        javascript_passed=true
        # print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "✓ JavaScript test: PASSED"
    # else
        # print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "✗ JavaScript test: FAILED"
    fi

    if "${GREP}" -q "SWAGGER_JSON_TEST_PASSED" "${result_file}" 2>/dev/null; then
        swagger_json_passed=true
        # print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "✓ Swagger JSON test: PASSED"
    # else
        # print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "✗ Swagger JSON test: FAILED"
    fi

    # Check custom headers test
    if "${GREP}" -q "CUSTOM_HEADERS_TEST_PASSED" "${result_file}" 2>/dev/null; then
        custom_headers_passed=true
        # print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "✓ Custom headers test: PASSED"
    # else
        # print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "✗ Custom headers test: FAILED"
    fi

    # Show diagnostic file links
    # print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Diagnostic files for ${description}:"
    # print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "  HTML content: ${LOG_PREFIX}_${log_suffix}_content.html"
    # print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "  JavaScript: ${LOG_PREFIX}_${log_suffix}_initializer.js"
    # print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "  Swagger JSON: ${LOG_PREFIX}_${log_suffix}_swagger_json_.json"
    # print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "  Timing data: ${LOG_PREFIX}_${log_suffix}_*.timing"
    
    # Return results via global variables for detailed reporting
    TRAILING_SLASH_TEST_RESULT=${trailing_slash_passed}
    REDIRECT_TEST_RESULT=${redirect_passed}
    CONTENT_TEST_RESULT=${content_passed}
    JAVASCRIPT_TEST_RESULT=${javascript_passed}
    SWAGGER_JSON_TEST_RESULT=${swagger_json_passed}
    CUSTOM_HEADERS_TEST_RESULT=${custom_headers_passed}
    
    # Return success only if all tests passed
    if "${GREP}" -q "ALL_SWAGGER_TESTS_PASSED" "${result_file}" 2>/dev/null; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "✓ All Swagger tests PASSED for ${description}"
        return 0
    else
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "✗ Some Swagger tests FAILED for ${description}"
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Check the diagnostic files above for detailed error information"
        return 1
    fi
}

# Function to test Swagger UI with a specific configuration (kept for compatibility)
test_swagger_configuration() {
    local config_file="$1"
    local swagger_prefix="$2"
    local test_name="$3"
    local config_number="$4"
    
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Testing Swagger UI: ${swagger_prefix} (using ${test_name})"
    
    # Extract port from configuration
    local server_port
    server_port=$(get_webserver_port "${config_file}")
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Configuration will use port: ${server_port}"
    
    # Global variables for server management
    local hydrogen_pid=""
    local server_log="${LOGS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}_swagger_${test_name}.log"
    local base_url="http://localhost:${server_port}"
    
    # Start server
    local subtest_start=$(((config_number - 1) * 6 + 1))
    
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Start Hydrogen Server (Config ${config_number})"
    
    # Use a temporary variable name that won't conflict
    local temp_pid_var="HYDROGEN_PID_$$"
    # shellcheck disable=SC2310 # We want to continue even if the test fails
    if start_hydrogen_with_pid "${config_file}" "${server_log}" 15 "${HYDROGEN_BIN}" "${temp_pid_var}"; then
        # Get the PID from the temporary variable
        hydrogen_pid=$(eval "echo \$${temp_pid_var}")
        if [[ -n "${hydrogen_pid}" ]]; then
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Server started successfully with PID: ${hydrogen_pid}"
            PASS_COUNT=$(( PASS_COUNT + 1 ))
        else
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Failed to start server - no PID returned"
            EXIT_CODE=1
            # Skip remaining subtests for this configuration
            for i in {2..6}; do

                print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Subtest $((subtest_start + i - 1)) (Skipped)"
                print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Skipped due to server startup failure"

            done
            return 1
        fi
    else
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Failed to start server"
        EXIT_CODE=1
        # Skip remaining subtests for this configuration
        for i in {2..6}; do

            print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Subtest $((subtest_start + i - 1)) (Skipped)"
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Skipped due to server startup failure"

        done
        return 1
    fi
    
    # Wait for server to be ready
    # shellcheck disable=SC2310 # We want to continue even if the test fails
    if [[ -n "${hydrogen_pid}" ]] && ps -p "${hydrogen_pid}" > /dev/null 2>&1; then
        if ! wait_for_server_ready "${base_url}"; then
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Server failed to become ready"
            EXIT_CODE=1
            # Skip remaining subtests
            for i in {2..6}; do

                print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Subtest $((subtest_start + i - 1)) (Skipped)"
                print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Skipped due to server readiness failure"

            done
            return 1
        fi
    fi
    
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Access Swagger UI with trailing slash (Config ${config_number})"

    if [[ -n "${hydrogen_pid}" ]] && ps -p "${hydrogen_pid}" > /dev/null 2>&1; then
        # shellcheck disable=SC2310 # We want to continue even if the test fails
        if check_response_content "${base_url}${swagger_prefix}/" "swagger-ui" "${RESULTS_DIR}/${test_name}_trailing_slash_${TIMESTAMP}.html" "true"; then
            PASS_COUNT=$(( PASS_COUNT + 1 ))
        else
            EXIT_CODE=1
        fi
    else
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Server not running for trailing slash test"
        EXIT_CODE=1
    fi
    
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Access Swagger UI without trailing slash (Config ${config_number})"

    if [[ -n "${hydrogen_pid}" ]] && ps -p "${hydrogen_pid}" > /dev/null 2>&1; then
        # shellcheck disable=SC2310 # We want to continue even if the test fails
        if check_redirect_response "${base_url}${swagger_prefix}" "${swagger_prefix}/" "${RESULTS_DIR}/${test_name}_redirect_${TIMESTAMP}.txt"; then
            PASS_COUNT=$(( PASS_COUNT + 1 ))
        else
            EXIT_CODE=1
        fi
    else
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Server not running for redirect test"
        EXIT_CODE=1
    fi
    
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Verify Swagger UI content loads (Config ${config_number})"

    if [[ -n "${hydrogen_pid}" ]] && ps -p "${hydrogen_pid}" > /dev/null 2>&1; then
        # shellcheck disable=SC2310 # We want to continue even if the test fails
        if check_response_content "${base_url}${swagger_prefix}/" "swagger-ui" "${RESULTS_DIR}/${test_name}_content_${TIMESTAMP}.html" "true"; then
            PASS_COUNT=$(( PASS_COUNT + 1 ))
        else
            EXIT_CODE=1
        fi
    else
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Server not running for content test"
        EXIT_CODE=1
    fi
    
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Verify JavaScript files load (Config ${config_number})"

    if [[ -n "${hydrogen_pid}" ]] && ps -p "${hydrogen_pid}" > /dev/null 2>&1; then
        # shellcheck disable=SC2310 # We want to continue even if the test fails
        if check_response_content "${base_url}${swagger_prefix}/swagger-initializer.js" "window.ui = SwaggerUIBundle" "${RESULTS_DIR}/${test_name}_initializer_${TIMESTAMP}.js" "true"; then
            PASS_COUNT=$(( PASS_COUNT + 1 ))
        else
            EXIT_CODE=1
        fi
    else
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Server not running for JavaScript test"
        EXIT_CODE=1
    fi
    
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Verify swagger.json file loads and contains valid content (Config ${config_number})"

    if [[ -n "${hydrogen_pid}" ]] && ps -p "${hydrogen_pid}" > /dev/null 2>&1; then
        # shellcheck disable=SC2310 # We want to continue even if the test fails
        if check_swagger_json "${base_url}${swagger_prefix}/swagger.json" "${RESULTS_DIR}/${test_name}_swagger_json_${TIMESTAMP}.json"; then
            PASS_COUNT=$(( PASS_COUNT + 1 ))
        else
            EXIT_CODE=1
        fi
    else
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Server not running for swagger.json test"
        EXIT_CODE=1
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
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Using Hydrogen binary: ${HYDROGEN_BIN_BASE}"
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Hydrogen binary found and validated"
    PASS_COUNT=$(( PASS_COUNT + 1 ))
else
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Failed to find Hydrogen binary"
    EXIT_CODE=1
fi

# Validate both configuration files
config_valid=true
for test_config in "${!SWAGGER_TEST_CONFIGS[@]}"; do
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Validate Configuration File: ${test_config}"

    # Parse test configuration
    IFS=':' read -r config_file log_suffix swagger_prefix description <<< "${SWAGGER_TEST_CONFIGS[${test_config}]}"
    
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
    PASS_COUNT=$(( PASS_COUNT + 1 ))
else
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Configuration file validation failed"
    EXIT_CODE=1
fi

# Only proceed with Swagger tests if prerequisites are met
if [[ "${EXIT_CODE}" -eq 0 ]]; then
   
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Running Swagger tests in parallel"
    
    # Start all Swagger tests in parallel with job limiting
    for test_config in "${!SWAGGER_TEST_CONFIGS[@]}"; do
        while true; do
            running_jobs=$(jobs -r | wc -l)
            if (( running_jobs < CORES )); then
                break
            fi
            wait -n  # Wait for any job to finish
        done
        
        # Parse test configuration
        IFS=':' read -r config_file log_suffix swagger_prefix description <<< "${SWAGGER_TEST_CONFIGS[${test_config}]}"
        
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Starting parallel test: ${test_config} (${description})"
        
        # Run parallel Swagger test in background
        run_swagger_test_parallel "${test_config}" "${config_file}" "${log_suffix}" "${swagger_prefix}" "${description}" &
        PARALLEL_PIDS+=($!)
    done
    
    # Wait for all parallel tests to complete
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Waiting for ${#SWAGGER_TEST_CONFIGS[@]} parallel Swagger tests to complete"
    for pid in "${PARALLEL_PIDS[@]}"; do
        wait "${pid}"
    done
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "All parallel tests completed, analyzing results"
    
    # Process results sequentially for clean output
    for test_config in "${!SWAGGER_TEST_CONFIGS[@]}"; do
        # Parse test configuration
        IFS=':' read -r config_file log_suffix swagger_prefix description <<< "${SWAGGER_TEST_CONFIGS[${test_config}]}"
        
        #print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Swagger UI Test: ${description} (${swagger_prefix})"
        log_file="${LOGS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}_${log_suffix}.log"
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${test_config} Server Log: ..${log_file}" 

        # shellcheck disable=SC2310 # We want to continue even if the test fails
        if analyze_swagger_test_results "${test_config}" "${log_suffix}" "${swagger_prefix}" "${description}"; then
            # Test individual endpoint results for detailed feedback
            print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Trailing Slash Access - ${description}"
            if [[ "${TRAILING_SLASH_TEST_RESULT}" = true ]]; then
                trailing_timing_ms=$(handle_timing_file "${LOG_PREFIX}_${log_suffix}_trailing_slash.timing" "trailing slash")
                print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Swagger UI with trailing slash test passed (${trailing_timing_ms}ms)"
                PASS_COUNT=$(( PASS_COUNT + 1 ))
            else
                print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Swagger UI with trailing slash test failed"
                EXIT_CODE=1
            fi

            print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Redirect Test - ${description}"
            if [[ "${REDIRECT_TEST_RESULT}" = true ]]; then
                redirect_timing_ms=$(handle_timing_file "${LOG_PREFIX}_${log_suffix}_redirect.timing" "redirect")
                print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Swagger UI redirect test passed (${redirect_timing_ms}ms)"
                PASS_COUNT=$(( PASS_COUNT + 1 ))
            else
                print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Swagger UI redirect test failed"
                EXIT_CODE=1
            fi

            print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Content Verification - ${description}"
            if [[ "${CONTENT_TEST_RESULT}" = true ]]; then
                content_timing_ms=$(handle_timing_file "${LOG_PREFIX}_${log_suffix}_content.timing" "content")
                print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Swagger UI content test passed (${content_timing_ms}ms)"
                PASS_COUNT=$(( PASS_COUNT + 1 ))
            else
                print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Swagger UI content test failed"
                EXIT_CODE=1
            fi

            print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "JavaScript Loading - ${description}"
            if [[ "${JAVASCRIPT_TEST_RESULT}" = true ]]; then
                js_timing_ms=$(handle_timing_file "${LOG_PREFIX}_${log_suffix}_initializer.timing" "JavaScript")
                print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "JavaScript initializer test passed (${js_timing_ms}ms)"
                PASS_COUNT=$(( PASS_COUNT + 1 ))
            else
                print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "JavaScript initializer test failed"
                EXIT_CODE=1
            fi

            print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Swagger JSON Validation - ${description}"
            if [[ "${SWAGGER_JSON_TEST_RESULT}" = true ]]; then
                print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Swagger JSON test passed"
                PASS_COUNT=$(( PASS_COUNT + 1 ))
            else
                print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Swagger JSON test failed"
                EXIT_CODE=1
            fi

            print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Custom Headers Test - ${description}"
            if [[ "${CUSTOM_HEADERS_TEST_RESULT}" = true ]]; then
                # Display header check results from result file
                # Need to define result_file based on log_suffix
                header_result_file="${LOG_PREFIX}_${log_suffix}.result"
                while IFS=: read -r marker header_name header_value status received_value; do
                    if [[ "${marker}" == "HEADER_CHECK" ]]; then
                        if [[ "${status}" == "FOUND" ]]; then
                            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Expected \"${header_name}\":\"${header_value}\" was found (received: ${received_value})"
                        elif [[ "${status}" == "MISMATCH" ]]; then
                            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Expected \"${header_name}\":\"${header_value}\" but received \"${received_value}\""
                        else
                            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Expected \"${header_name}\":\"${header_value}\" was not found"
                        fi
                    fi
                done < <("${GREP}" "^HEADER_CHECK:" "${header_result_file}" 2>/dev/null || true)
                print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Custom headers test passed"
                PASS_COUNT=$(( PASS_COUNT + 1 ))
            else
                print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Custom headers test failed"
                EXIT_CODE=1
            fi
            
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}: All Swagger tests passed"
        else
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${description} test failed"
            EXIT_CODE=1
        fi
    done
    
    # Print summary
    successful_configs=0
    for test_config in "${!SWAGGER_TEST_CONFIGS[@]}"; do
        IFS=':' read -r config_file log_suffix swagger_prefix description <<< "${SWAGGER_TEST_CONFIGS[${test_config}]}"
        result_file="${LOG_PREFIX}_${log_suffix}.result"
        if [[ -f "${result_file}" ]] && "${GREP}" -q "ALL_SWAGGER_TESTS_PASSED" "${result_file}" 2>/dev/null; then
            successful_configs=$(( successful_configs + 1 ))
        fi
    done

    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Summary: ${successful_configs}/${#SWAGGER_TEST_CONFIGS[@]} Swagger configurations passed all tests"
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Parallel execution completed - SO_REUSEADDR allows immediate port reuse"

    # Calculate average response time from actual timing data
    total_response_time="0"
    timing_count=0
    for test_config in "${!SWAGGER_TEST_CONFIGS[@]}"; do
        IFS=':' read -r config_file log_suffix swagger_prefix description <<< "${SWAGGER_TEST_CONFIGS[${test_config}]}"
        result_file="${LOG_PREFIX}_${log_suffix}.result"
        if [[ -f "${result_file}" ]] && "${GREP}" -q "ALL_SWAGGER_TESTS_PASSED" "${result_file}" 2>/dev/null; then
            config_total="0"
            config_count=0
            collect_timing_data "${LOG_PREFIX}" "${log_suffix}" "config_total" "config_count"
            total_response_time=$(echo "scale=6; ${total_response_time} + ${config_total}" | bc 2>/dev/null || echo "${total_response_time}")
            timing_count=$((timing_count + config_count))
        fi
    done

    # Calculate average response time from actual timing data
    if [[ ${timing_count} -gt 0 ]]; then
        avg_response_time=$(echo "scale=6; ${total_response_time} / ${timing_count}" | bc 2>/dev/null || echo "0.000500")
        # Convert to milliseconds for more readable display
        # shellcheck disable=SC2312 # We want to continue even if printf fails
        avg_response_time_ms=$(printf "%.3f" "$(echo "scale=6; ${avg_response_time} * 1000" | bc 2>/dev/null || echo "0.500")" 2>/dev/null || echo "0.500")
        TEST_NAME="${TEST_NAME}  {BLUE}avg: ${avg_response_time_ms}ms{RESET}"
    fi

else
    # Skip Swagger tests if prerequisites failed
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Skipping Swagger tests due to prerequisite failures"
    EXIT_CODE=1
fi

# Print test completion summary
print_test_completion "${TEST_NAME}" "${TEST_ABBR}" "${TEST_NUMBER}" "${TEST_VERSION}"

# Return status code if sourced, exit if run standalone
${ORCHESTRATION:-false} && return "${EXIT_CODE}" || exit "${EXIT_CODE}"

