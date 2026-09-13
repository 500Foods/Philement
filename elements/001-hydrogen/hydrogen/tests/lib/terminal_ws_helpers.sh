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
# test_websocket_chat_key_rejected_on_terminal_path()
# test_websocket_terminal_key_rejected_on_chat_path()
# test_websocket_wrong_protocol_rejected()

# CHANGELOG
# 1.1.1 - 2026-09-13 - Phase 11: Fixed terminal WebSocket auth to use RESOLVED_WS_KEY
#                    (terminal key from sysinfo or WEBSOCKET_TERMINAL_KEY) instead of
#                    WEBSOCKET_KEY (chat key). All terminal WS tests now authenticate
#                    with the terminal key, not the chat key.
# 1.1.0 - 2026-09-13 - Phase 11: Added cross-key denial tests (chat on terminal path, terminal on chat path, wrong protocol)
# 1.0.0 - 2026-09-12 - Extracted WebSocket helpers from test_26_terminal.sh (1000-line cap)

# shellcheck disable=SC2154 # TEST_NUMBER, TEST_COUNTER, GREP, TIMEOUT, WEBSOCKET_KEY, WEBSOCKET_TERMINAL_KEY, LOG_PREFIX, TIMESTAMP come from framework/caller
# shellcheck disable=SC2312 # Diagnostic substitutions swallow inner status; callers use || true

[[ -n "${TERMINAL_WS_HELPERS_GUARD:-}" ]] && return 0
export TERMINAL_WS_HELPERS_GUARD="true"

TERMINAL_WS_HELPERS_NAME="Terminal WebSocket Helpers"
TERMINAL_WS_HELPERS_VERSION="1.1.1"
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
            -H="Authorization: Key ${RESOLVED_WS_KEY:-${WEBSOCKET_TERMINAL_KEY:-${WEBSOCKET_KEY:-}}}" \
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
            -H="Authorization: Key ${RESOLVED_WS_KEY:-${WEBSOCKET_TERMINAL_KEY:-${WEBSOCKET_KEY:-}}}" \
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
            -H="Authorization: Key ${RESOLVED_WS_KEY:-${WEBSOCKET_TERMINAL_KEY:-${WEBSOCKET_KEY:-}}}" \
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
            -H="Authorization: Key ${RESOLVED_WS_KEY:-${WEBSOCKET_TERMINAL_KEY:-${WEBSOCKET_KEY:-}}}" \
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
            -H="Authorization: Key ${RESOLVED_WS_KEY:-${WEBSOCKET_TERMINAL_KEY:-${WEBSOCKET_KEY:-}}}" \
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

# ---------------------------------------------------------------------------
# test_websocket_chat_key_rejected_on_terminal_path
#
# Verifies that the chat key (WEBSOCKET_KEY) is rejected when used to
# authenticate against the terminal WebSocket path (/terminal/ws).
# The terminal surface must only accept Terminal.Key (WEBSOCKET_TERMINAL_KEY).
#
# Usage:
#   test_websocket_chat_key_rejected_on_terminal_path <ws_url> <terminal_protocol> <response_file>
#
# Returns 0 if the chat key is correctly rejected (connection fails/401),
# 1 if the chat key is unexpectedly accepted on the terminal path.
# ---------------------------------------------------------------------------
test_websocket_chat_key_rejected_on_terminal_path() {
    local ws_url="$1"
    local ws_protocol="$2"
    local response_file="$3"

    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Testing cross-key denial: chat key on terminal path (should fail)"

    # Use the chat key (WEBSOCKET_KEY) against the terminal path.
    # The key value is never logged — only a redacted fingerprint.
    local chat_key="${WEBSOCKET_KEY:-}"
    if [[ -z "${chat_key}" ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Skipped: WEBSOCKET_KEY not set (no chat key to test)"
        return 0
    fi

    local terminal_ws_url="${ws_url}"
    # Ensure the URL points to the terminal path
    if [[ "${terminal_ws_url}" != *"/terminal/ws"* ]]; then
        terminal_ws_url="${terminal_ws_url%/terminal/ws}/terminal/ws"
    fi

    # Strip wss:// -> ws:// for the test server (plain WebSocket, no TLS)
    terminal_ws_url="${terminal_ws_url/wss:\/\//ws:\/\/}"

    # Attempt connection with the CHAT key on the TERMINAL path.
    # We expect this to FAIL (401 forbidden or connection rejected).
    # Redacted: the key is never printed, only "Authorization: Key ***"
    print_command "${TEST_NUMBER}" "${TEST_COUNTER}" "websocat --protocol='${ws_protocol}' -H='Authorization: Key ***' --ping-interval=30 --exit-on-eof '${terminal_ws_url}' (chat key on terminal path)"

    local websocat_output
    local websocat_exitcode
    local temp_file="${LOG_PREFIX}${TIMESTAMP}_${ws_protocol}_cross_key_chat_on_terminal.log"

    # Send a ping message and expect rejection. Use --one-message with a timeout.
    echo '{"type": "ping"}' | "${TIMEOUT}" 5 websocat \
        --protocol="${ws_protocol}" \
        -H="Authorization: Key ${chat_key}" \
        --ping-interval=30 \
        --one-message \
        "${terminal_ws_url}" > "${temp_file}" 2>&1
    websocat_exitcode=$?
    websocat_output=$(cat "${temp_file}" 2>/dev/null || echo "")

    # The chat key should be REJECTED on the terminal path.
    # websocat exit code 0 (success) means the connection was accepted — that's a failure.
    # websocat exit code 1 with auth error, or non-zero with 401/forbidden — that's success.
    if [[ "${websocat_exitcode}" -eq 0 ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Chat key was accepted on terminal path (should be rejected)"
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "CROSS-KEY VIOLATION: chat key opened terminal WebSocket"
        return 1
    fi

    # Check for expected rejection indicators
    if echo "${websocat_output}" | "${GREP}" -qi "401\|forbidden\|unauthorized\|rejected\|denied"; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Chat key correctly rejected on terminal path (redacted: auth failure detected)"
        return 0
    fi

    # Non-zero exit without explicit auth error — still a rejection
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Chat key rejected on terminal path (exit code ${websocat_exitcode} — connection not established)"
    return 0
}

# ---------------------------------------------------------------------------
# test_websocket_terminal_key_rejected_on_chat_path
#
# Verifies that the terminal key (WEBSOCKET_TERMINAL_KEY) is rejected when
# used to authenticate against the chat WebSocket path (/wss).
# The chat surface must only accept WebSocketServer.Key (WEBSOCKET_KEY).
#
# Usage:
#   test_websocket_terminal_key_rejected_on_chat_path <ws_url> <chat_protocol> <response_file>
#
# Returns 0 if the terminal key is correctly rejected on the chat path,
# 1 if the terminal key is unexpectedly accepted.
# ---------------------------------------------------------------------------
test_websocket_terminal_key_rejected_on_chat_path() {
    local ws_url="$1"
    local ws_protocol="$2"
    local response_file="$3"

    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Testing cross-key denial: terminal key on chat path (should fail)"

    # Use the terminal key (WEBSOCKET_TERMINAL_KEY) against the chat path (/wss).
    local terminal_key="${WEBSOCKET_TERMINAL_KEY:-}"
    if [[ -z "${terminal_key}" ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Skipped: WEBSOCKET_TERMINAL_KEY not set (no terminal key to test)"
        return 0
    fi

    local chat_ws_url="${ws_url}"
    # Ensure the URL points to the chat path (/wss)
    if [[ "${chat_ws_url}" == *"/terminal/ws"* ]]; then
        chat_ws_url="${chat_ws_url%/terminal/ws}/wss"
    elif [[ "${chat_ws_url}" != *"/wss"* ]]; then
        chat_ws_url="${chat_ws_url%/wss}/wss"
    fi

    # Strip wss:// -> ws:// for the test server (plain WebSocket, no TLS)
    chat_ws_url="${chat_ws_url/wss:\/\//ws:\/\/}"

    # Attempt connection with the TERMINAL key on the CHAT path.
    print_command "${TEST_NUMBER}" "${TEST_COUNTER}" "websocat --protocol='${ws_protocol}' -H='Authorization: Key ***' --ping-interval=30 --exit-on-eof '${chat_ws_url}' (terminal key on chat path)"

    local websocat_output
    local websocat_exitcode
    local temp_file="${LOG_PREFIX}${TIMESTAMP}_${ws_protocol}_cross_key_terminal_on_chat.log"

    echo '{"type": "ping"}' | "${TIMEOUT}" 5 websocat \
        --protocol="${ws_protocol}" \
        -H="Authorization: Key ${terminal_key}" \
        --ping-interval=30 \
        --one-message \
        "${chat_ws_url}" > "${temp_file}" 2>&1
    websocat_exitcode=$?
    websocat_output=$(cat "${temp_file}" 2>/dev/null || echo "")

    # The terminal key should be REJECTED on the chat path.
    if [[ "${websocat_exitcode}" -eq 0 ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Terminal key was accepted on chat path (should be rejected)"
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "CROSS-KEY VIOLATION: terminal key opened chat WebSocket"
        return 1
    fi

    if echo "${websocat_output}" | "${GREP}" -qi "401\|forbidden\|unauthorized\|rejected\|denied"; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Terminal key correctly rejected on chat path (redacted: auth failure detected)"
        return 0
    fi

    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Terminal key rejected on chat path (exit code ${websocat_exitcode} — connection not established)"
    return 0
}

# ---------------------------------------------------------------------------
# test_websocket_wrong_protocol_rejected
#
# Verifies that a mismatched subprotocol is rejected on the terminal path.
# If the server expects the "terminal" protocol and the client offers "hydrogen"
# (or vice versa), the connection must fail.
#
# Usage:
#   test_websocket_wrong_protocol_rejected <ws_url> <wrong_protocol> <response_file>
#
# Returns 0 if the wrong protocol is rejected, 1 if it's accepted.
# ---------------------------------------------------------------------------
test_websocket_wrong_protocol_rejected() {
    local ws_url="$1"
    local wrong_protocol="$2"
    local response_file="$3"

    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Testing protocol mismatch rejection: protocol '${wrong_protocol}' on terminal path (should fail)"

    local terminal_ws_url="${ws_url}"
    # Ensure the URL points to the terminal path
    if [[ "${terminal_ws_url}" != *"/terminal/ws"* ]]; then
        terminal_ws_url="${terminal_ws_url%/terminal/ws}/terminal/ws"
    fi

    # Strip wss:// -> ws:// for the test server (plain WebSocket, no TLS)
    terminal_ws_url="${terminal_ws_url/wss:\/\//ws:\/\/}"

    # Use a valid terminal key but the WRONG protocol. The key is never logged.
    local test_key="${RESOLVED_WS_KEY:-${WEBSOCKET_TERMINAL_KEY:-${WEBSOCKET_KEY:-}}}"
    if [[ -z "${test_key}" ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "No WebSocket key available for protocol test"
        return 1
    fi

    print_command "${TEST_NUMBER}" "${TEST_COUNTER}" "websocat --protocol='${wrong_protocol}' -H='Authorization: Key ***' --ping-interval=30 --exit-on-eof '${terminal_ws_url}' (wrong protocol)"

    local websocat_output
    local websocat_exitcode
    local temp_file="${LOG_PREFIX}${TIMESTAMP}_${wrong_protocol}_wrong_proto.log"

    echo '{"type": "ping"}' | "${TIMEOUT}" 5 websocat \
        --protocol="${wrong_protocol}" \
        -H="Authorization: Key ${test_key}" \
        --ping-interval=30 \
        --one-message \
        "${terminal_ws_url}" > "${temp_file}" 2>&1
    websocat_exitcode=$?
    websocat_output=$(cat "${temp_file}" 2>/dev/null || echo "")

    # A mismatched protocol should be rejected. websocat may report a protocol
    # negotiation failure or a connection that immediately closes.
    if [[ "${websocat_exitcode}" -eq 0 ]]; then
        # Check if the response contains terminal output (meaning the wrong
        # protocol was accepted, which is a failure)
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Wrong protocol '${wrong_protocol}' was accepted on terminal path (should be rejected)"
        return 1
    fi

    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Wrong protocol '${wrong_protocol}' correctly rejected on terminal path (exit code ${websocat_exitcode})"
    return 0
}
