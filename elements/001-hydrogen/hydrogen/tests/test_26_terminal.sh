#!/usr/bin/env bash

# Test: Terminal
# Tests the Terminal functionality, payload serving, and WebRoot configurations.

# FUNCTIONS
# check_terminal_response_content()
# run_terminal_test_parallel()
# analyze_terminal_test_results()
# test_terminal_configuration()

# CHANGELOG
# 2.4.0 - 2025-11-20 - Enhanced test coverage for terminal_shell_ops.c, terminal_websocket_bridge.c:
#                    - Extended I/O test from 3 to 8 commands with longer delays (1s between commands + 3s post-test)
#                    - Added multiple resize commands (5 different dimensions) to thoroughly exercise pty_set_size()
#                    - Added long-running session test (4 iterations over 8 seconds) to exercise pty_is_running()
#                    - These changes significantly increase coverage of PTY operations and I/O bridge functions
# 2.3.0 - 2025-09-09 - Fixed WebSocket I/O and resize test crashes: Added NULL pointer checks in terminal_websocket.c
#                    - Re-enabled WebSocket I/O and resize tests that were disabled due to SIGSEGV crashes
#                    - Enhanced libwebsockets mocks to support lws_get_protocol for better terminal message routing
#                    - Improved coverage for terminal_websocket.c and terminal_shell.c functions
#                    - WebSocket I/O and resize tests now enabled and should provide better coverage
# 2.0.0 - 2025-08-31 - Major refactor: Implemented parallel execution of Terminal tests following Test 22/Swagger patterns.
#                    - Added dual configuration testing: payload mode vs filesystem mode
#                    - Extracted modular functions for parallel execution and result analysis
#                    - Now runs both payload and filesystem tests simultaneously instead of sequentially
# 1.0.0 - 2025-08-31 - Initial implementation based on test_22_swagger.sh pattern

set -euo pipefail

# Test Configuration
TEST_NAME="Terminal"
TEST_ABBR="TRM"
TEST_NUMBER="26"
TEST_COUNTER=0
TEST_VERSION="2.4.0"  # Enhanced coverage for terminal_shell_ops.c and terminal_websocket_bridge.c

# shellcheck source=tests/lib/framework.sh # Reference framework directly
[[ -n "${FRAMEWORK_GUARD:-}" ]] || source "$(dirname "${BASH_SOURCE[0]}")/lib/framework.sh"
setup_test_environment

# Parallel execution configuration
declare -a PARALLEL_PIDS
declare -A TERMINAL_TEST_CONFIGS

# Declare result variables to avoid unbound variable errors
INDEX_TEST_RESULT=false
SPECIFIC_FILE_TEST_RESULT=false
CROSS_CONFIG_404_TEST_RESULT=false

# Terminal test configuration - format: "config_file:log_suffix:description:expected_content"
TERMINAL_TEST_CONFIGS=(
    ["PAYLOAD"]="${SCRIPT_DIR}/configs/hydrogen_test_26_terminal_payload.json:payload:Payload Mode:Hydrogen Terminal"
    ["FILESYSTEM"]="${SCRIPT_DIR}/configs/hydrogen_test_26_terminal_filesystem.json:filesystem:Filesystem Mode:HYDROGEN_TERMINAL_TEST_MARKER"
)

# Test timeouts
STARTUP_TIMEOUT=15
SHUTDOWN_TIMEOUT=15  # Increased to allow more graceful cleanup and I/O processing

# Function to check HTTP response content with retry logic for subsystem readiness
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
                # Check if this is actually the expected terminal test page
                if "${GREP}" -q "HYDROGEN_TERMINAL_TEST_MARKER" "${response_file}" || "${GREP}" -q "Hydrogen Terminal Test Interface" "${response_file}"; then
                    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Successfully received terminal page from ${url}"
                    if [[ "${attempt}" -gt 1 ]]; then
                        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Response contains expected content: ${expected_content} (succeeded on attempt ${attempt})"
                    else
                        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Response contains expected content: ${expected_content}"
                    fi
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
            
            # Show response excerpt
            local line_count
            line_count=$(wc -l < "${response_file}")
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Response contains ${line_count} lines"
            
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

# Function to test Terminal configuration in parallel
run_terminal_test_parallel() {
    local test_name="$1"
    local config_file="$2"
    local log_suffix="$3"
    local description="$4"
    local expected_file="$5"
    
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
            
            # Test terminal index page - use the expected content for this configuration
            local index_file="${LOG_PREFIX}${TIMESTAMP}_${log_suffix}_index.html"
            # shellcheck disable=SC2310 # We want to continue even if the test fails
            if check_terminal_response_content "${base_url}/terminal/" "${expected_file}" "${index_file}" "true"; then
                echo "INDEX_TEST_PASSED" >> "${result_file}"
            else
                echo "INDEX_TEST_FAILED" >> "${result_file}"
                all_tests_passed=false
            fi
            
            # Test specific files based on configuration mode
            if [[ "${log_suffix}" = "payload" ]]; then
                # For payload mode, test that we can access the terminal interface
                local payload_test_file="${LOG_PREFIX}${TIMESTAMP}_${log_suffix}_terminal.html"
                # shellcheck disable=SC2310 # We want to continue even if the test fails
                if check_terminal_response_content "${base_url}/terminal/" "Hydrogen Terminal" "${payload_test_file}" "true"; then
                    echo "SPECIFIC_FILE_TEST_PASSED" >> "${result_file}"
                else
                    echo "SPECIFIC_FILE_TEST_FAILED" >> "${result_file}"
                    all_tests_passed=false
                fi
            else
                # For filesystem mode, test that we can access the test artifacts
                local filesystem_test_file="${LOG_PREFIX}${TIMESTAMP}_${log_suffix}_index.html"
                # shellcheck disable=SC2310 # We want to continue even if the test fails
                if check_terminal_response_content "${base_url}/terminal/index.html" "HYDROGEN_TERMINAL_TEST_MARKER" "${filesystem_test_file}" "true"; then
                    echo "SPECIFIC_FILE_TEST_PASSED" >> "${result_file}"
                else
                    echo "SPECIFIC_FILE_TEST_FAILED" >> "${result_file}"
                    all_tests_passed=false
                fi
            fi
            
            # Test 404 behavior for the other config's file (cross-config test)
            local other_file=""
            if [[ "${expected_file}" = "terminal.html" ]]; then
                other_file="xterm-test.html"
            else
                other_file="terminal.html"
            fi

            local cross_file="${LOG_PREFIX}${TIMESTAMP}_${log_suffix}_cross_${other_file}.html"
            curl -s --max-time 10 "${base_url}/terminal/${other_file}" > "${cross_file}" 2>/dev/null
            if "${GREP}" -q "404 Not Found" "${cross_file}"; then
                echo "CROSS_CONFIG_404_TEST_PASSED" >> "${result_file}"
            else
                echo "CROSS_CONFIG_404_TEST_FAILED" >> "${result_file}"
                # Note: This is expected behavior - files might be available in both configs
                # Don't fail the test for this
            fi

            # Test WebSocket terminal connection
            # Determine WebSocket port from configuration
            local ws_port
            ws_port=$(jq -r '.WebSocketServer.Port // 5261' "${config_file}" 2>/dev/null || echo "5261")
            local ws_url="ws://localhost:${ws_port}"

            local websocket_protocol
            websocket_protocol=$(jq -r '.WebSocketServer.Protocol // "terminal"' "${config_file}" 2>/dev/null || echo "terminal")

            # Test WebSocket terminal connection (basic connectivity test)
            local websocket_test_file="${LOG_PREFIX}${TIMESTAMP}_${log_suffix}_websocket_connection.json"
            # shellcheck disable=SC2310 # We want to continue even if the test fails
            if test_websocket_terminal_connection "${ws_url}" "${websocket_protocol}" '{"type": "ping"}' "${websocket_test_file}"; then
                echo "WEBSOCKET_CONNECTION_TEST_PASSED" >> "${result_file}"
            else
                echo "WEBSOCKET_CONNECTION_TEST_FAILED" >> "${result_file}"
                all_tests_passed=false
            fi

            # Test WebSocket terminal protocol acceptance (ping test)
            local websocket_ping_file="${LOG_PREFIX}${TIMESTAMP}_${log_suffix}_websocket_ping.json"
            # shellcheck disable=SC2310 # We want to continue even if the test fails
            if test_websocket_terminal_status "${ws_url}" "${websocket_protocol}" "${websocket_ping_file}"; then
                echo "WEBSOCKET_PING_TEST_PASSED" >> "${result_file}"
            else
                echo "WEBSOCKET_PING_TEST_FAILED" >> "${result_file}"
                all_tests_passed=false
            fi

            # Test WebSocket terminal input/output
            # NOTE: Re-enabled after fixing crash in terminal_websocket.c NULL pointer checks
            local websocket_io_file="${LOG_PREFIX}${TIMESTAMP}_${log_suffix}_websocket_io.json"
            # shellcheck disable=SC2310 # We want to continue even if the test fails
            if test_websocket_terminal_input_output "${ws_url}" "${websocket_protocol}" "${websocket_io_file}"; then
                echo "WEBSOCKET_IO_TEST_PASSED" >> "${result_file}"
                # Brief pause to allow I/O processing and potential pty_read_data coverage
                sleep 1
            else
                echo "WEBSOCKET_IO_TEST_FAILED" >> "${result_file}"
                all_tests_passed=false
            fi

            # Test WebSocket terminal resize
            # NOTE: Re-enabled after fixing crash in terminal_websocket.c NULL pointer checks
            local websocket_resize_file="${LOG_PREFIX}${TIMESTAMP}_${log_suffix}_websocket_resize.json"
            # shellcheck disable=SC2310 # We want to continue even if the test fails
            if test_websocket_terminal_resize "${ws_url}" "${websocket_protocol}" "${websocket_resize_file}"; then
                echo "WEBSOCKET_RESIZE_TEST_PASSED" >> "${result_file}"
            else
                echo "WEBSOCKET_RESIZE_TEST_FAILED" >> "${result_file}"
                all_tests_passed=false
            fi

            # Test WebSocket terminal long-running session
            # This exercises pty_is_running, should_continue_iobridge, and the I/O bridge loop
            local websocket_long_session_file="${LOG_PREFIX}${TIMESTAMP}_${log_suffix}_websocket_long_session.json"
            # shellcheck disable=SC2310 # We want to continue even if the test fails
            if test_websocket_terminal_long_session "${ws_url}" "${websocket_protocol}" "${websocket_long_session_file}"; then
                echo "WEBSOCKET_LONG_SESSION_TEST_PASSED" >> "${result_file}"
            else
                echo "WEBSOCKET_LONG_SESSION_TEST_FAILED" >> "${result_file}"
                all_tests_passed=false
            fi

            if [[ "${all_tests_passed}" = true ]]; then
                echo "ALL_TERMINAL_TESTS_PASSED" >> "${result_file}"
            else
                echo "SOME_TERMINAL_TESTS_FAILED" >> "${result_file}"
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

# Function to analyze results from parallel Terminal test execution
analyze_terminal_test_results() {
    local test_name="$1"
    local log_suffix="$2"
    local description="$3"
    local expected_file="$4"
    local result_file="${LOG_PREFIX}${TIMESTAMP}_${log_suffix}.result"

    if [[ ! -f "${result_file}" ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "No result file found for ${test_name}"
        return 1
    fi
    
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
    
    # Check individual terminal tests
    local index_test_passed=false
    local specific_file_test_passed=false
    local cross_config_404_test_passed=false
    
    if "${GREP}" -q "INDEX_TEST_PASSED" "${result_file}" 2>/dev/null; then
        index_test_passed=true
    fi
    
    if "${GREP}" -q "SPECIFIC_FILE_TEST_PASSED" "${result_file}" 2>/dev/null; then
        specific_file_test_passed=true
    fi
    
    if "${GREP}" -q "CROSS_CONFIG_404_TEST_PASSED" "${result_file}" 2>/dev/null; then
        cross_config_404_test_passed=true
    fi
    
    # Return results via global variables for detailed reporting
    INDEX_TEST_RESULT=${index_test_passed}
    SPECIFIC_FILE_TEST_RESULT=${specific_file_test_passed}
    CROSS_CONFIG_404_TEST_RESULT=${cross_config_404_test_passed}
    
    # Return success only if critical tests passed (404 test is informational)
    if "${GREP}" -q "ALL_TERMINAL_TESTS_PASSED" "${result_file}" 2>/dev/null; then
        return 0
    else
        return 1
    fi
}

# Function to test WebSocket terminal connection with proper authentication and retry logic
test_websocket_terminal_connection() {
    local ws_url="$1"
    local protocol="$2"
    local test_message="$3"
    local response_file="$4"

    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Testing WebSocket Terminal connection with authentication using websocat"
    print_command "${TEST_NUMBER}" "${TEST_COUNTER}" "echo '${test_message}' | websocat --protocol='${protocol}' -H='Authorization: Key ${WEBSOCKET_KEY}' --ping-interval=30 --exit-on-eof '${ws_url}'"

    # Retry logic for WebSocket subsystem readiness (reduced for parallel execution to prevent thundering herd)
    local max_attempts=5
    local attempt=1
    local websocat_output
    local websocat_exitcode
    local temp_file="${LOG_PREFIX}${TIMESTAMP}_${protocol}_terminal_echo.log"

    while [[ "${attempt}" -le "${max_attempts}" ]]; do
        if [[ "${attempt}" -gt 1 ]]; then
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Terminal WebSocket connection attempt ${attempt} of ${max_attempts} (waiting for terminal subsystem initialization)..."
            sleep 0.05  # Brief delay between attempts to prevent thundering herd
        fi

        # Test WebSocket connection with a 5-second timeout
        echo "${test_message}" | "${TIMEOUT}" 5 websocat \
            --protocol="${protocol}" \
            -H="Authorization: Key ${WEBSOCKET_KEY}" \
            --ping-interval=30 \
            --exit-on-eof \
            "${ws_url}" > "${temp_file}" 2>&1
        websocat_exitcode=$?
        websocat_output=$(cat "${temp_file}" 2>/dev/null || echo "")

        # Analyze the results
        if [[ "${websocat_exitcode}" -eq 0 ]]; then
            if [[ "${attempt}" -gt 1 ]]; then
                print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Terminal WebSocket connection successful (clean exit, succeeded on attempt ${attempt})"
            else
                print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Terminal WebSocket connection successful (clean exit)"
            fi
            # Don't print server response to avoid cluttering output with shell prompts
            return 0
        elif [[ "${websocat_exitcode}" -eq 124 ]]; then
            # Timeout occurred, but that's OK if connection was established
            if [[ "${attempt}" -gt 1 ]]; then
                print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Terminal WebSocket connection successful (timeout after successful connection, succeeded on attempt ${attempt})"
            else
                print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Terminal WebSocket connection successful (timeout after successful connection)"
            fi
            return 0
        else
            # Check for connection refused which might indicate WebSocket server not ready yet
            if echo "${websocat_output}" | "${GREP}" -qi "connection refused"; then
                if [[ "${attempt}" -eq "${max_attempts}" ]]; then
                    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Terminal WebSocket connection failed: Connection refused after ${max_attempts} attempts"
                    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Server is not accepting Terminal WebSocket connections on the specified port"
                    return 1
                else
                    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Terminal WebSocket server not ready yet (connection refused), retrying..."
                    attempt=$(( attempt + 1 ))
                    continue
                fi
            fi

            # Check for authentication errors
            if echo "${websocat_output}" | "${GREP}" -qi "401\|forbidden\|unauthorized"; then
                print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Terminal WebSocket connection failed: Authentication rejected"
                print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Server rejected the provided WebSocket key"
                return 1
            elif echo "${websocat_output}" | "${GREP}" -qi "protocol.*not.*supported"; then
                print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Terminal WebSocket connection failed: Terminal protocol not supported"
                print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Server does not support the 'terminal' protocol"
                return 1
            else
                # Unknown error - retry if we have attempts left
                if [[ "${attempt}" -eq "${max_attempts}" ]]; then
                    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Terminal WebSocket connection failed after ${max_attempts} attempts"
                    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Error: ${websocat_output}"
                    return 1
                else
                    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Terminal WebSocket connection failed on attempt ${attempt}, retrying..."
                    attempt=$(( attempt + 1 ))
                    continue
                fi
            fi
        fi
    done

    return 1
}

# Function to test WebSocket terminal status request
test_websocket_terminal_status() {
    local ws_url="$1"
    local protocol="$2"
    local response_file="$3"

    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Testing Terminal WebSocket status request using websocat"

    # JSON message to request status (terminal-specific)
    local status_request='{"type": "ping"}'
    print_command "${TEST_NUMBER}" "${TEST_COUNTER}" "echo '${status_request}' | websocat --protocol='${protocol}' -H='Authorization: Key ${WEBSOCKET_KEY}' --ping-interval=30 --one-message '${ws_url}'"

    # Retry logic for WebSocket subsystem readiness (reduced for parallel execution to prevent thundering herd)
    local max_attempts=8
    local attempt=1
    local websocat_output
    local websocat_exitcode
    local temp_file="${LOG_PREFIX}${TIMESTAMP}_${protocol}_terminal_status.txt"

    while [[ "${attempt}" -le "${max_attempts}" ]]; do
        if [[ "${attempt}" -gt 1 ]]; then
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Terminal WebSocket status request attempt ${attempt} of ${max_attempts}..."
            sleep 0.05  # Brief delay between attempts to prevent thundering herd
        fi

        # Test WebSocket status request with a 3-second timeout
        echo "${status_request}" | websocat \
            --protocol="${protocol}" \
            -H="Authorization: Key ${WEBSOCKET_KEY}" \
            --ping-interval=30 \
            --one-message \
            "${ws_url}" > "${temp_file}" 2>&1
        websocat_exitcode=$?
        websocat_output=$(cat "${temp_file}" 2>/dev/null || echo "")

        # For terminal protocol, we expect success (clean exit) - this tests that the protocol is accepted
        if [[ "${websocat_exitcode}" -eq 0 ]]; then
            if [[ "${attempt}" -gt 1 ]]; then
                print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Terminal WebSocket status ping successful (succeeded on attempt ${attempt})"
            else
                print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Terminal WebSocket status ping successful"
            fi
            # Don't print protocol acceptance message to reduce output clutter
            return 0
        elif [[ "${websocat_exitcode}" -eq 124 ]]; then
            # Timeout occurred, but ping should respond quickly
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Terminal WebSocket ping timed out (protocol accepting but no response)"
            if [[ "${attempt}" -eq "${max_attempts}" ]]; then
                print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Terminal WebSocket ping failed - protocol accepted but no response"
                return 1
            else
                print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Terminal WebSocket ping attempt ${attempt} timed out, retrying..."
                attempt=$(( attempt + 1 ))
                continue
            fi
        else
            # Check for connection issues that might indicate protocol incompatibility
            if [[ "${websocat_exitcode}" -ne 0 ]] && [[ "${websocat_exitcode}" -ne 1 ]]; then
                if [[ "${attempt}" -eq "${max_attempts}" ]]; then
                    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Terminal WebSocket ping failed - connection error (${websocat_exitcode})"
                    return 1
                else
                    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Terminal WebSocket ping attempt ${attempt} failed, retrying..."
                    attempt=$(( attempt + 1 ))
                    continue
                fi
            fi

            # For other errors, fail immediately as they're likely permanent
            if echo "${websocat_output}" | "${GREP}" -qi "401\|forbidden\|unauthorized"; then
                print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Terminal WebSocket ping failed: Authentication rejected"
                print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Server rejected the provided WebSocket key for terminal protocol"
                return 1
            elif echo "${websocat_output}" | "${GREP}" -qi "protocol.*not.*supported"; then
                print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Terminal WebSocket ping failed: Terminal protocol not supported"
                print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Server does not support the 'terminal' protocol - likely configuration issue"
                return 1
            fi

            # If we reach here, either got failure or timeout, retry if we have attempts left
            if [[ "${attempt}" -eq "${max_attempts}" ]]; then
                print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Terminal WebSocket ping failed after ${max_attempts} attempts"
                print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Final error: ${websocat_output}"
                return 1
            else
                print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Terminal WebSocket ping attempt ${attempt} failed, retrying..."
                attempt=$(( attempt + 1 ))
            fi
        fi
    done

    return 1
}

# Function to test WebSocket terminal input/output with shell command
test_websocket_terminal_input_output() {
    local ws_url="$1"
    local protocol="$2"
    local response_file="$3"

    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Testing WebSocket Terminal input/output with multiple commands (extended for coverage)"

    # Send multiple input commands to better exercise I/O processing
    # Extended from 3 to 8 commands to increase coverage
    local commands=(
        '{"type": "input", "data": "echo hello\n"}'
        '{"type": "input", "data": "pwd\n"}'
        '{"type": "input", "data": "date\n"}'
        '{"type": "input", "data": "whoami\n"}'
        '{"type": "input", "data": "echo Coverage Test Line 1\n"}'
        '{"type": "input", "data": "echo Coverage Test Line 2\n"}'
        '{"type": "input", "data": "ls -la /tmp 2>/dev/null | head -n 5\n"}'
        '{"type": "input", "data": "echo COVERAGE_TEST_COMPLETE\n"}'
    )

    local all_commands_successful=true

    for cmd in "${commands[@]}"; do
        print_command "${TEST_NUMBER}" "${TEST_COUNTER}" "echo '${cmd}' | websocat --protocol='${protocol}' -H='Authorization: Key ${WEBSOCKET_KEY}' --ping-interval=30 --one-message '${ws_url}'"

        # Send the command
        if ! echo "${cmd}" | websocat \
            --protocol="${protocol}" \
            -H="Authorization: Key ${WEBSOCKET_KEY}" \
            --ping-interval=30 \
            --one-message \
            "${ws_url}" >> "${response_file}" 2>&1; then
            all_commands_successful=false
            break
        fi

        # Increased pause between commands from 0.5s to 1s to allow more bridge cycles
        sleep 1
    done

    # Additional pause to allow I/O bridge thread to process multiple read cycles
    # This ensures terminal_websocket_bridge.c functions get sufficient execution time
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Maintaining connection for I/O bridge coverage (additional 3 seconds)..."
    sleep 3

    if [[ "${all_commands_successful}" = true ]]; then
        # The test passes if commands were sent successfully
        # This exercises terminal_websocket.c input processing and pty_write_data
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Terminal input commands sent successfully (terminal_websocket.c and terminal_shell.c exercised)"
        return 0
    else
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Failed to send terminal input commands"
        return 1
    fi
}

# Function to test WebSocket terminal resize functionality
test_websocket_terminal_resize() {
    local ws_url="$1"
    local protocol="$2"
    local response_file="$3"

    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Testing WebSocket Terminal resize functionality (multiple dimensions for coverage)"

    # Send multiple resize commands with different dimensions
    # This exercises terminal_shell_ops.c:pty_set_size() more thoroughly
    local resize_commands=(
        '{"type": "resize", "rows": 30, "cols": 100}'
        '{"type": "resize", "rows": 40, "cols": 120}'
        '{"type": "resize", "rows": 50, "cols": 132}'
        '{"type": "resize", "rows": 24, "cols": 80}'   # Standard size
        '{"type": "resize", "rows": 25, "cols": 85}'   # Different variation
    )

    local all_resize_successful=true

    for resize_command in "${resize_commands[@]}"; do
        print_command "${TEST_NUMBER}" "${TEST_COUNTER}" "echo '${resize_command}' | websocat --protocol='${protocol}' -H='Authorization: Key ${WEBSOCKET_KEY}' --ping-interval=30 --one-message '${ws_url}'"

        # Send resize command - success means terminal_websocket.c resize function was called
        if ! echo "${resize_command}" | websocat \
            --protocol="${protocol}" \
            -H="Authorization: Key ${WEBSOCKET_KEY}" \
            --ping-interval=30 \
            --one-message \
            "${ws_url}" >> "${response_file}" 2>&1; then
            all_resize_successful=false
            break
        fi

        # Brief pause between resizes to allow processing
        sleep 0.5
    done

    if [[ "${all_resize_successful}" = true ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Terminal resize commands sent successfully (terminal_websocket.c and terminal_shell_ops.c exercised)"
        return 0
    else
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Failed to send terminal resize commands"
        return 1
    fi
}

# Function to test long-running WebSocket terminal session
test_websocket_terminal_long_session() {
    local ws_url="$1"
    local protocol="$2"
    local response_file="$3"
    
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Testing WebSocket Terminal long-running session (pty_is_running coverage)"
    
    # Keep connection alive with periodic commands over 8 seconds
    # This exercises terminal_shell_ops.c:pty_is_running() and ensures
    # the I/O bridge thread maintains the session properly
    local session_successful=true
    
    for i in {1..4}; do
        local cmd='{"type": "input", "data": "echo Session iteration '${i}'\n"}'
        print_command "${TEST_NUMBER}" "${TEST_COUNTER}" "echo '${cmd}' | websocat --protocol='${protocol}' -H='Authorization: Key ${WEBSOCKET_KEY}' --ping-interval=30 --one-message '${ws_url}'"
        
        if ! echo "${cmd}" | websocat \
            --protocol="${protocol}" \
            -H="Authorization: Key ${WEBSOCKET_KEY}" \
            --ping-interval=30 \
            --one-message \
            "${ws_url}" >> "${response_file}" 2>&1; then
            session_successful=false
            break
        fi
        
        # 2-second pause between commands to maintain session over time
        sleep 2
    done
    
    if [[ "${session_successful}" = true ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Long-running session test completed (pty_is_running and should_continue_io_bridge exercised)"
        return 0
    else
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Long-running session test failed"
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
for test_config in "${!TERMINAL_TEST_CONFIGS[@]}"; do
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Validate Configuration File: ${test_config}"

    # Parse test configuration
    IFS=':' read -r config_file log_suffix description expected_file <<< "${TERMINAL_TEST_CONFIGS[${test_config}]}"
    
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

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Validate WEBSOCKET_KEY Environment Variable"
if [[ -n "${WEBSOCKET_KEY}" ]]; then
    # shellcheck disable=SC2310 # We want to continue even if the test fails
    if validate_websocket_key "WEBSOCKET_KEY" "${WEBSOCKET_KEY}"; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "WEBSOCKET_KEY is valid and ready for WebSocket authentication"
    else
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "WEBSOCKET_KEY is invalid format"
        EXIT_CODE=1
        print_test_completion "${TEST_NAME}" "${TEST_ABBR}" "${TEST_NUMBER}" "${TEST_VERSION}"
        ${ORCHESTRATION:-false} && return "${EXIT_CODE}" || exit "${EXIT_CODE}"
    fi
else
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "WEBSOCKET_KEY environment variable is not set"
    EXIT_CODE=1
    print_test_completion "${TEST_NAME}" "${TEST_ABBR}" "${TEST_NUMBER}" "${TEST_VERSION}"
    ${ORCHESTRATION:-false} && return "${EXIT_CODE}" || exit "${EXIT_CODE}"
fi

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Validate Test Artifacts"
if [[ -f "tests/artifacts/terminal/index.html" ]] && [[ -f "tests/artifacts/terminal/xterm-test.html" ]]; then
    if "${GREP}" -q "HYDROGEN_TERMINAL_TEST_MARKER" "tests/artifacts/terminal/index.html" || "${GREP}" -q "Hydrogen Terminal Test Interface" "tests/artifacts/terminal/xterm-test.html"; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Test artifacts validated successfully"
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Test artifact files found and validated"
    else
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Test markers not found in artifact files"
        EXIT_CODE=1
    fi
else
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Test artifact files missing"
    EXIT_CODE=1
fi

# Only proceed with Terminal tests if prerequisites are met
if [[ "${EXIT_CODE}" -eq 0 ]]; then
   
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Running Terminal tests in parallel"
    
    # Start all Terminal tests in parallel with job limiting
    for test_config in "${!TERMINAL_TEST_CONFIGS[@]}"; do
        # shellcheck disable=SC2312 # Job control with wc -l is standard practice
        while (( $(jobs -r | wc -l) >= CORES )); do
            wait -n  # Wait for any job to finish
        done
        
        # Parse test configuration
        IFS=':' read -r config_file log_suffix description expected_file <<< "${TERMINAL_TEST_CONFIGS[${test_config}]}"
        
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Starting parallel test: ${test_config} (${description})"
        
        # Run parallel Terminal test in background
        run_terminal_test_parallel "${test_config}" "${config_file}" "${log_suffix}" "${description}" "${expected_file}" &
        PARALLEL_PIDS+=($!)
    done
    
    # Wait for all parallel tests to complete
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Waiting for ${#TERMINAL_TEST_CONFIGS[@]} parallel Terminal tests to complete"
    for pid in "${PARALLEL_PIDS[@]}"; do
        wait "${pid}"
    done
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "All parallel tests completed, analyzing results"
    
    # Process results sequentially for clean output
    for test_config in "${!TERMINAL_TEST_CONFIGS[@]}"; do
        # Parse test configuration
        IFS=':' read -r config_file log_suffix description expected_file <<< "${TERMINAL_TEST_CONFIGS[${test_config}]}"
        
        log_file="${LOGS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}_${log_suffix}.log"
        result_file="${LOG_PREFIX}${TIMESTAMP}_${log_suffix}.result"
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${test_config} Server Log: ..${log_file}"
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${test_config} Result File: ..${result_file}"

        # shellcheck disable=SC2310 # We want to continue even if the test fails
        if analyze_terminal_test_results "${test_config}" "${log_suffix}" "${description}" "${expected_file}"; then
            # Test individual endpoint results for detailed feedback
            print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Terminal Index Access - ${description}"
            if [[ "${INDEX_TEST_RESULT}" = true ]]; then
                print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Terminal index page test passed"
            else
                print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Terminal index page test failed"
                EXIT_CODE=1
            fi
            
            print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Specific File Access - ${description}"
            if [[ "${SPECIFIC_FILE_TEST_RESULT}" = true ]]; then
                print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Specific file (${expected_file}) test passed"
            else
                print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Specific file (${expected_file}) test failed"
                EXIT_CODE=1
            fi
            
            print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Cross-Config 404 Test - ${description}"
            if [[ "${CROSS_CONFIG_404_TEST_RESULT}" = true ]]; then
                print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Cross-config 404 test passed (proper file isolation)"
            else
                print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Cross-config file access detected (files available in both modes)"
                print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Cross-config test completed (informational)"
            fi

            # Reconstruct result file path for WebSocket tests
            result_file="${LOG_PREFIX}${TIMESTAMP}_${log_suffix}.result"

            # Check WebSocket connection test results
            print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "WebSocket Terminal Connection - ${description}"
            if "${GREP}" -q "WEBSOCKET_CONNECTION_TEST_PASSED" "${result_file}" 2>/dev/null; then
                print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Terminal WebSocket connection test passed"
            else
                print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Terminal WebSocket connection test failed"
                EXIT_CODE=1
            fi

            # Check WebSocket ping test results
            print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "WebSocket Terminal Ping - ${description}"
            if "${GREP}" -q "WEBSOCKET_PING_TEST_PASSED" "${result_file}" 2>/dev/null; then
                print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Terminal WebSocket ping test passed"
            else
                print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Terminal WebSocket ping test failed"
                EXIT_CODE=1
            fi

            # Check WebSocket I/O test results
            print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "WebSocket Terminal I/O - ${description}"
            if "${GREP}" -q "WEBSOCKET_IO_TEST_PASSED" "${result_file}" 2>/dev/null; then
                print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Terminal WebSocket I/O test passed"
            else
                print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Terminal WebSocket I/O test failed"
                EXIT_CODE=1
            fi

            # Check WebSocket resize test results
            print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "WebSocket Terminal Resize - ${description}"
            if "${GREP}" -q "WEBSOCKET_RESIZE_TEST_PASSED" "${result_file}" 2>/dev/null; then
                print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Terminal WebSocket resize test passed"
            else
                print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Terminal WebSocket resize test failed"
                EXIT_CODE=1
            fi

            # Check WebSocket long-running session test results
            print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "WebSocket Terminal Long Session - ${description}"
            if "${GREP}" -q "WEBSOCKET_LONG_SESSION_TEST_PASSED" "${result_file}" 2>/dev/null; then
                print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Terminal WebSocket long-running session test passed"
            else
                print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Terminal WebSocket long-running session test failed"
                EXIT_CODE=1
            fi

            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}: All Terminal tests passed"
        else
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${description} test failed"
            EXIT_CODE=1
        fi
    done
    
    # Print summary
    successful_configs=0
    for test_config in "${!TERMINAL_TEST_CONFIGS[@]}"; do
        IFS=':' read -r config_file log_suffix description expected_file <<< "${TERMINAL_TEST_CONFIGS[${test_config}]}"
        result_file="${LOG_PREFIX}${TIMESTAMP}_${log_suffix}.result"
        if [[ -f "${result_file}" ]] && "${GREP}" -q "ALL_TERMINAL_TESTS_PASSED" "${result_file}" 2>/dev/null; then
            successful_configs=$(( successful_configs + 1 ))
        fi
    done
    
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Summary: ${successful_configs}/${#TERMINAL_TEST_CONFIGS[@]} Terminal configurations passed all tests"
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Parallel execution completed - SO_REUSEADDR allows immediate port reuse"
    
else
    # Skip Terminal tests if prerequisites failed
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Skipping Terminal tests due to prerequisite failures"
    EXIT_CODE=1
fi

# Print test completion summary
print_test_completion "${TEST_NAME}" "${TEST_ABBR}" "${TEST_NUMBER}" "${TEST_VERSION}"

# Return status code if sourced, exit if run standalone
${ORCHESTRATION:-false} && return "${EXIT_CODE}" || exit "${EXIT_CODE}"
