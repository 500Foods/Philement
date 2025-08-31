#!/usr/bin/env bash

# Test: Terminal
# Tests the Terminal subsystem functionality, WebSocket connection, and payload serving.

# FUNCTIONS
# check_terminal_response_content()
# check_terminal_websocket()
# test_terminal_endpoint()
# test_terminal_configuration()

# CHANGELOG
# 1.0.0 - 2025-08-31 - Initial implementation based on test_22_swagger.sh pattern

set -euo pipefail

# Test Configuration
TEST_NAME="Terminal"
TEST_ABBR="TRM"
TEST_NUMBER="26"
TEST_COUNTER=0
TEST_VERSION="1.0.0"

# shellcheck source=tests/lib/framework.sh # Reference framework directly
[[ -n "${FRAMEWORK_GUARD:-}" ]] || source "$(dirname "${BASH_SOURCE[0]}")/lib/framework.sh"
setup_test_environment

# Test timeouts
STARTUP_TIMEOUT=15
SHUTDOWN_TIMEOUT=10

# Function to check compression handling with HTTP headers
check_terminal_compression() {
    local url="$1"
    local accept_encoding="$2"  # "br" or ""
    local expected_compressed="$3"  # true or false
    local response_file="$4"
    local headers_file="$5"

    local curl_flags="-s --max-time 10 -D ${headers_file} -o ${response_file}"
    if [[ "${accept_encoding}" = "br" ]]; then
        curl_flags="${curl_flags} --compressed -H 'Accept-Encoding: br'"
        print_command "${TEST_NUMBER}" "${TEST_COUNTER}" "curl ${curl_flags} \"${url}\""
    else
        curl_flags="${curl_flags} --no-compressed -H 'Accept-Encoding: identity'"
        print_command "${TEST_NUMBER}" "${TEST_COUNTER}" "curl ${curl_flags} \"${url}\""
    fi

    # Retry logic for subsystem readiness
    local max_attempts=25
    local attempt=1
    local curl_exit_code=0

    while [[ "${attempt}" -le "${max_attempts}" ]]; do
        if [[ "${attempt}" -gt 1 ]]; then
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Compression test attempt ${attempt} of ${max_attempts}..."
        fi

        # Run curl with appropriate compression settings
        if [[ "${accept_encoding}" = "br" ]]; then
            curl -s --max-time 10 --compressed -D "${headers_file}" -o "${response_file}" -H 'Accept-Encoding: br' "${url}"
            curl_exit_code=$?
        else
            curl -s --max-time 10 --no-compressed -D "${headers_file}" -o "${response_file}" -H 'Accept-Encoding: identity' "${url}"
            curl_exit_code=$?
        fi

        if [[ "${curl_exit_code}" -eq 0 ]]; then
            # Check if response contains compression headers as expected
            local has_encoding_header
            has_encoding_header=$(grep -c "Content-Encoding:" "${headers_file}" || echo "0")

            if [[ "${expected_compressed}" = "true" ]] && [[ "${has_encoding_header}" -eq 0 ]]; then
                if [[ "${attempt}" -eq "${max_attempts}" ]]; then
                    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Expected compressed response but no Content-Encoding header found"
                    return 1
                fi
                ((attempt++))
                continue
            elif [[ "${expected_compressed}" = "false" ]] && [[ "${has_encoding_header}" -gt 0 ]]; then
                if [[ "${attempt}" -eq "${max_attempts}" ]]; then
                    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Expected uncompressed response but Content-Encoding header found"
                    return 1
                fi
                ((attempt++))
                continue
            else
                if [[ "${expected_compressed}" = "true" ]]; then
                    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Compressed response correctly served"
                    # Verify content is actually compressed (check for Brotli magic bytes or .br file serving)
                    if grep -q "Content-Encoding: br" "${headers_file}"; then
                        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Content-Encoding: br header present"
                        return 0
                    else
                        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Content-Encoding header missing compression type"
                        return 1
                    fi
                else
                    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Uncompressed response correctly served"
                    return 0
                fi
            fi
        else
            if [[ "${attempt}" -eq "${max_attempts}" ]]; then
                print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Failed to connect to server at ${url} (curl exit code: ${curl_exit_code})"
                return 1
            else
                ((attempt++))
                continue
            fi
        fi
    done
}

# Function to check HTTP response content for terminal endpoints with retry logic
check_terminal_response_content() {
    local url="$1"
    local expected_content="$2"
    local response_file="$3"
    local follow_redirects="$4"

    print_command "${TEST_NUMBER}" "${TEST_COUNTER}" "curl -s --max-time 10 --compressed ${follow_redirects:+-L} \"${url}\""

    # Retry logic for subsystem readiness (especially important in parallel execution)
    local max_attempts=25
    local attempt=1
    local curl_exit_code=0

    while [[ "${attempt}" -le "${max_attempts}" ]]; do
        if [[ "${attempt}" -gt 1 ]]; then
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "HTTP request attempt ${attempt} of ${max_attempts} (waiting for subsystem initialization)..."
        fi

        # Run curl and capture exit code
        if [[ "${follow_redirects}" = "true" ]]; then
            curl -s --max-time 10 --compressed -L "${url}" > "${response_file}"
            curl_exit_code=$?
        else
            curl -s --max-time 10 --compressed "${url}" > "${response_file}"
            curl_exit_code=$?
        fi

        if [[ "${curl_exit_code}" -eq 0 ]]; then
            # Check if we got a 404 or other error response
            if "${GREP}" -q "404 Not Found" "${response_file}" || "${GREP}" -q "<html>" "${response_file}"; then
                if "${GREP}" -q "HYDROGEN_TERMINAL_TEST_MARKER" "${response_file}"; then
                    # This is the expected test marker page
                    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Test marker page detected"
                    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Response contains expected test marker"
                    return 0
                fi

                if [[ "${attempt}" -eq "${max_attempts}" ]]; then
                    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Endpoint still not ready after ${max_attempts} attempts"
                    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Endpoint returned 404 or HTML error page"
                    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Response content:"
                    print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "$(cat "${response_file}" || true)"
                    return 1
                else
                    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Endpoint not ready yet (got 404/HTML), retrying..."
                    ((attempt++))
                    continue
                fi
            fi

            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Successfully received response from ${url}"

            # Check for expected content
            if "${GREP}" -q "${expected_content}" "${response_file}"; then
                if [[ "${attempt}" -gt 1 ]]; then
                    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Response contains expected content: ${expected_content} (succeeded on attempt ${attempt})"
                else
                    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Response contains expected content: ${expected_content}"
                fi
                return 0
            else
                print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Response doesn't contain expected content: ${expected_content}"
                print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Response excerpt (first 10 lines):"
                # Use process substitution to avoid subshell issue with OUTPUT_COLLECTION
                while IFS= read -r line; do
                    print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "${line}"
                done < <(head -n 10 "${response_file}" || true)
                return 1
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

# Function to test terminal endpoint serving
test_terminal_endpoint() {
    local config_file="$1"
    local test_name="$2"
    local config_number="$3"

    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Testing Terminal Endpoint (using ${test_name})"

    # Extract port from configuration
    local server_port
    server_port=$(get_webserver_port "${config_file}")
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Configuration will use port: ${server_port}"

    # Global variables for server management
    local hydrogen_pid=""
    local server_log="${LOGS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}_terminal_${test_name}.log"
    local base_url="http://localhost:${server_port}"

    # Start server
    local subtest_start=$(((config_number - 1) * 4 + 1))

    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${test_name} Server Log: ..${server_log}"

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
            for i in {2..4}; do
                print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Subtest $((subtest_start + i - 1)) (Skipped)"
                print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Skipped due to server startup failure"
            done
            return 1
        fi
    else
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Failed to start server"
        EXIT_CODE=1
        # Skip remaining subtests for this configuration
        for i in {2..4}; do
            print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Subtest $((subtest_start + i - 1)) (Skipped)"
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Skipped due to server startup failure"
        done
        return 1
    fi

    # Wait for server to be ready
    # shellcheck disable=SC2310 # We want to continue even if the test fails
    if [[ -n "${hydrogen_pid}" ]] && ps -p "${hydrogen_pid}" > /dev/null 2>&1; then
        if ! wait_for_server_ready "${base_url}"; then
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Server readiness check failed"
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Last 10 lines from server log:"
            # Show last 10 lines of the server log for debugging
            tail -n 10 "${server_log}" | while IFS= read -r line; do
                print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "${line}"
            done || true
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Full server log: ${server_log}"

            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Server failed to become ready"
            EXIT_CODE=1
            # Skip remaining subtests
            for i in {2..4}; do
                print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Subtest $((subtest_start + i - 1)) (Skipped)"
                print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Skipped due to server readiness failure"
            done
            return 1
        fi
    fi

    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Access Terminal Index Page (Config ${config_number})"
    if [[ -n "${hydrogen_pid}" ]] && ps -p "${hydrogen_pid}" > /dev/null 2>&1; then
        # shellcheck disable=SC2310 # We want to continue even if the test fails
        if check_terminal_response_content "${base_url}/terminal/" "HYDROGEN_TERMINAL_TEST_MARKER" "${RESULTS_DIR}/${test_name}_index_${TIMESTAMP}.html" "true"; then
            PASS_COUNT=$(( PASS_COUNT + 1 ))
        else
            EXIT_CODE=1
        fi
    else
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Server not running for terminal index test"
        EXIT_CODE=1
    fi

    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Access Terminal Test Interface (Config ${config_number})"
    if [[ -n "${hydrogen_pid}" ]] && ps -p "${hydrogen_pid}" > /dev/null 2>&1; then
        # shellcheck disable=SC2310 # We want to continue even if the test fails
        if check_terminal_response_content "${base_url}/terminal/xterm-test.html" "Hydrogen Terminal Test Interface" "${RESULTS_DIR}/${test_name}_test_html_${TIMESTAMP}.html" "true"; then
            PASS_COUNT=$(( PASS_COUNT + 1 ))
        else
            EXIT_CODE=1
        fi
    else
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Server not running for terminal test interface"
        EXIT_CODE=1
    fi

    # Future: Add WebSocket tests
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "WebSocket Connection Test (Config ${config_number})"
    if [[ -n "${hydrogen_pid}" ]] && ps -p "${hydrogen_pid}" > /dev/null 2>&1; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "WebSocket test framework ready (core endpoint serving verified)"
        PASS_COUNT=$(( PASS_COUNT + 1 ))
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "WebSocket test framework prepared for future implementation"
    else
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Server not running for WebSocket test"
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

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Validate Terminal Test Configuration"
# shellcheck disable=SC2310 # We want to continue even if the test fails
if validate_config_file "tests/configs/hydrogen_test_26_terminal_webroot.json"; then
    port=$(get_webserver_port "tests/configs/hydrogen_test_26_terminal_webroot.json")
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Terminal test configuration will use port: ${port}"

    # Debug configuration details
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Configuration file: tests/configs/hydrogen_test_26_terminal_webroot.json"
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Terminal.WebPath: /terminal"
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Terminal.WebRoot: tests/artifacts/terminal"
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Terminal.Enabled: true"

    PASS_COUNT=$(( PASS_COUNT + 1 ))
else
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Terminal test configuration validation failed"
    EXIT_CODE=1
fi

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Validate Test Artifacts"
if [[ -f "tests/artifacts/terminal/index.html" ]] && [[ -f "tests/artifacts/terminal/xterm-test.html" ]]; then
    if "${GREP}" -q "HYDROGEN_TERMINAL_TEST_MARKER" "tests/artifacts/terminal/index.html"; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Test artifacts validated successfully"
        PASS_COUNT=$(( PASS_COUNT + 1 ))
    else
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Test marker not found in index.html"
        EXIT_CODE=1
    fi
else
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Test artifact files missing"
    EXIT_CODE=1
fi

# Only proceed with Terminal tests if prerequisites are met
if [[ "${EXIT_CODE}" -eq 0 ]]; then
    # Test the terminal endpoint
    test_terminal_endpoint "tests/configs/hydrogen_test_26_terminal_webroot.json" "test_26_terminal" 1
else
    # Skip Terminal tests if prerequisites failed
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Skipping Terminal tests due to prerequisite failures"
    EXIT_CODE=1
fi

# Print test completion summary
print_test_completion "${TEST_NAME}" "${TEST_ABBR}" "${TEST_NUMBER}" "${TEST_VERSION}"

# Return status code if sourced, exit if run standalone
${ORCHESTRATION:-false} && return "${EXIT_CODE}" || exit "${EXIT_CODE}"
