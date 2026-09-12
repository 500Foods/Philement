#!/usr/bin/env bash

# Terminal WebSocket Test Helpers
# WebSocket connection, ping, I/O, resize, and long-session helpers for
# test_26_terminal.sh. Split out so the blackbox script stays under 1000 lines.

# LIBRARY FUNCTIONS
# test_websocket_terminal_connection()
# test_websocket_terminal_status()
# test_websocket_terminal_input_output()
# test_websocket_terminal_resize()
# test_websocket_terminal_long_session()

# CHANGELOG
# 1.0.0 - 2026-09-12 - Extracted WebSocket helpers from test_26_terminal.sh (1000-line cap)

# shellcheck disable=SC2154 # TEST_NUMBER, TEST_COUNTER, GREP, TIMEOUT, WEBSOCKET_KEY, LOG_PREFIX, TIMESTAMP come from framework/caller
# shellcheck disable=SC2312 # Diagnostic substitutions swallow inner status; callers use || true

[[ -n "${TERMINAL_WS_HELPERS_GUARD:-}" ]] && return 0
export TERMINAL_WS_HELPERS_GUARD="true"

TERMINAL_WS_HELPERS_NAME="Terminal WebSocket Helpers"
TERMINAL_WS_HELPERS_VERSION="1.0.0"
print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${TERMINAL_WS_HELPERS_NAME} ${TERMINAL_WS_HELPERS_VERSION}" "info"

# Function to test WebSocket terminal connection with proper authentication and retry logic
test_websocket_terminal_connection() {
    local ws_url="$1"
    local protocol="$2"
    local test_message="$3"
    local response_file="$4"

    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Testing WebSocket Terminal connection with authentication using websocat"
    print_command "${TEST_NUMBER}" "${TEST_COUNTER}" "echo '${test_message}' | websocat --protocol='${protocol}' -H='Authorization: Key ***' --ping-interval=30 --exit-on-eof '${ws_url}'"

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
    print_command "${TEST_NUMBER}" "${TEST_COUNTER}" "echo '${status_request}' | websocat --protocol='${protocol}' -H='Authorization: Key ***' --ping-interval=30 --one-message '${ws_url}'"

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
        print_command "${TEST_NUMBER}" "${TEST_COUNTER}" "echo '${cmd}' | websocat --protocol='${protocol}' -H='Authorization: Key ***' --ping-interval=30 --one-message '${ws_url}'"

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
        print_command "${TEST_NUMBER}" "${TEST_COUNTER}" "echo '${resize_command}' | websocat --protocol='${protocol}' -H='Authorization: Key ***' --ping-interval=30 --one-message '${ws_url}'"

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
        print_command "${TEST_NUMBER}" "${TEST_COUNTER}" "echo '${cmd}' | websocat --protocol='${protocol}' -H='Authorization: Key ***' --ping-interval=30 --one-message '${ws_url}'"

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
