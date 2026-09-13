#!/usr/bin/env bash

# Terminal Utilities Library
# Terminal-specific helpers for test_26_terminal.sh and future terminal tests:
# SQLite isolation, WebSocket config resolution, result-flag checking, redaction,
# HTTP content checks, login, and system-info authorization contract tests.

# LIBRARY FUNCTIONS
# prepare_sqlite_isolation()
# resolve_terminal_websocket_config()
# check_result_flag()
# redact_jwt_fingerprint()
# redact_sysinfo_body()
# check_terminal_response_content()
# login_for_terminal_tests()
# test_sysinfo_no_terminal_without_jwt()
# test_sysinfo_no_terminal_with_invalid_jwt()
# test_sysinfo_cors_origin_enforcement()
# test_sysinfo_terminal_with_valid_jwt()

# CHANGELOG
# 1.2.0 - 2026-09-12 - Resolve Terminal.Protocol and WEBSOCKET_TERMINAL_KEY
# 1.1.0 - 2026-09-12 - Added HTTP/sysinfo helpers from test_26_terminal.sh (1000-line cap)
# 1.0.0 - 2026-09-12 - Extracted from test_26_terminal.sh: SQLite isolation, WebSocket config,
#                      result-flag checking, JWT fingerprint, and sysinfo redaction helpers

# shellcheck disable=SC2154 # TEST_NUMBER, TEST_COUNTER, GREP, JQ come from framework/caller

# Check for required HYDROGEN_ROOT environment variable
if [[ -z "${HYDROGEN_ROOT:-}" ]]; then
    echo "❌ Error: HYDROGEN_ROOT environment variable is not set"
    echo "Please set HYDROGEN_ROOT to the Hydrogen project's root directory"
    exit 1
fi

# Check for required HELIUM_ROOT environment variable
if [[ -z "${HELIUM_ROOT:-}" ]]; then
    echo "❌ Error: HELIUM_ROOT environment variable is not set"
    echo "Please set HELIUM_ROOT to the Helium project's root directory"
    exit 1
fi

set -euo pipefail

# Guard clause to prevent multiple sourcing
[[ -n "${TERMINAL_UTILS_GUARD:-}" ]] && return 0
export TERMINAL_UTILS_GUARD="true"

# Library metadata
TERMINAL_UTILS_NAME="Terminal Utilities Library"
TERMINAL_UTILS_VERSION="1.2.0"

# Ensure framework is sourced (for GREP, JQ, etc.)
# shellcheck disable=SC1091,SC2154 # Normal framework sourcing
[[ -n "${FRAMEWORK_GUARD:-}" ]] || source "${LIB_DIR}/framework.sh"

# print_message is defined in log_output.sh which is sourced during
# setup_test_environment(); guard this call so the library can be
# sourced before the framework is fully initialized.
# shellcheck disable=SC2154 # normal framework sourcing
if declare -f print_message >/dev/null 2>&1; then
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${TERMINAL_UTILS_NAME} ${TERMINAL_UTILS_VERSION}" "info" 2> /dev/null || true
fi

# shellcheck disable=SC2154 # TEST_NUMBER, TEST_COUNTER, GREP, JQ come from framework/caller

# ---------------------------------------------------------------------------
# prepare_sqlite_isolation
#
# Copies the SQLite database referenced by a config file to a per-run work
# directory, checkpointing WAL mode first so the copy is self-contained.
# Updates the config's SQLite Connection.Database paths to point at the copy
# with AutoMigration=false.
#
# Usage:
#   prepare_sqlite_isolation <config_file> <sqlite_work_dir>
#
# Sets the following globals on success:
#   SQLITE_ISOLATION_CONFIG    - path to the (possibly rewritten) config file
#   SQLITE_ISOLATION_WORK_DIR  - the work directory (empty if no SQLite was found)
#
# Returns 0 on success (even if SQLite isolation was not needed), 1 on copy failure.
# ---------------------------------------------------------------------------
prepare_sqlite_isolation() {
    local config_file="$1"
    local sqlite_work_dir="$2"

    SQLITE_ISOLATION_CONFIG="${config_file}"
    SQLITE_ISOLATION_WORK_DIR=""
    export SQLITE_ISOLATION_CONFIG SQLITE_ISOLATION_WORK_DIR

    local sqlite_db_path
    sqlite_db_path=$(jq -r '.Databases.Connections[0].Database' "${config_file}" 2>/dev/null || echo "")

    if [[ -z "${sqlite_db_path}" || "${sqlite_db_path}" == "null" ]]; then
        return 0
    fi

    SQLITE_ISOLATION_WORK_DIR="${sqlite_work_dir}"
    local sqlite_copy="${sqlite_work_dir}/hydrodemo.sqlite"
    mkdir -p "${sqlite_work_dir}" 2>/dev/null || return 1

    local sqlite_checkpointed=false
    if command -v sqlite3 >/dev/null 2>&1; then
        sqlite3 "${sqlite_db_path}" "PRAGMA wal_checkpoint(TRUNCATE);" >/dev/null 2>/dev/null && sqlite_checkpointed=true
    fi

    if ! cp "${sqlite_db_path}" "${sqlite_copy}" 2>/dev/null; then
        rm -rf "${sqlite_work_dir}" 2>/dev/null || true
        SQLITE_ISOLATION_WORK_DIR=""
        SQLITE_ISOLATION_CONFIG="${config_file}"
        export SQLITE_ISOLATION_CONFIG SQLITE_ISOLATION_WORK_DIR
        return 1
    fi

    if [[ "${sqlite_checkpointed}" != "true" ]]; then
        for _ext in -wal -shm; do
            if [[ -f "${sqlite_db_path}${_ext}" ]]; then
                cp "${sqlite_db_path}${_ext}" "${sqlite_copy}${_ext}" 2>/dev/null || true
            fi
        done
    fi

    # Rewrite the config to use the copied database
    local actual_config_file="${sqlite_work_dir}/config.json"
    if jq --arg db "${sqlite_copy}" \
        '.Databases.Connections |= map(
            if ((.Engine // "") | ascii_downcase) == "sqlite" then
                .Database = $db | .AutoMigration = false
            else . end
         )' "${config_file}" > "${actual_config_file}" 2>/dev/null; then
        SQLITE_ISOLATION_CONFIG="${actual_config_file}"
    else
        rm -rf "${sqlite_work_dir}" 2>/dev/null || true
        SQLITE_ISOLATION_WORK_DIR=""
        SQLITE_ISOLATION_CONFIG="${config_file}"
        export SQLITE_ISOLATION_CONFIG SQLITE_ISOLATION_WORK_DIR
    fi

    return 0
}

# ---------------------------------------------------------------------------
# resolve_terminal_websocket_config
#
# Determines the WebSocket URL, protocol, and key for terminal tests.
# Prefers values obtained from an authorized /api/system/info response
# (TERMINAL_WS_URL, TERMINAL_WS_PROTOCOL, TERMINAL_WS_KEY globals).
# Falls back to config file values or sensible test defaults.
#
# Usage:
#   resolve_terminal_websocket_config <config_file>
#
# Sets globals: RESOLVED_WS_URL, RESOLVED_WS_PROTOCOL, RESOLVED_WS_KEY
# ---------------------------------------------------------------------------
resolve_terminal_websocket_config() {
    local config_file="$1"

    local ws_url="${TERMINAL_WS_URL:-}"
    if [[ -z "${ws_url}" ]]; then
        local ws_port
        ws_port=$(jq -r '.WebSocketServer.Port // 5261' "${config_file}" 2>/dev/null || echo "5261")
        ws_url="ws://localhost:${ws_port}"
    fi

    # Strip wss:// -> ws:// for the test server (plain WebSocket, no TLS)
    ws_url="${ws_url/wss:\/\//ws:\/\/}"

    # Append the terminal WebSocket path if not already present
    if [[ "${ws_url}" != *"/terminal/ws"* ]]; then
        ws_url="${ws_url}/terminal/ws"
    fi

    local websocket_protocol="${TERMINAL_WS_PROTOCOL:-}"
    if [[ -z "${websocket_protocol}" ]]; then
        websocket_protocol=$(jq -r '.Terminal.Protocol // "terminal"' "${config_file}" 2>/dev/null || echo "terminal")
    fi

    # Resolve the terminal WebSocket key — prefer the sysinfo-provided key,
    # then WEBSOCKET_TERMINAL_KEY, never the chat key (WEBSOCKET_KEY).
    local terminal_key="${TERMINAL_WS_KEY:-${WEBSOCKET_TERMINAL_KEY:-}}"

    RESOLVED_WS_URL="${ws_url}"
    RESOLVED_WS_PROTOCOL="${websocket_protocol}"
    RESOLVED_WS_KEY="${terminal_key}"
    export RESOLVED_WS_URL RESOLVED_WS_PROTOCOL RESOLVED_WS_KEY
}

# ---------------------------------------------------------------------------
# check_result_flag
#
# Checks whether a result file contains a specific flag token.
# Replaces the repeated 'grep -q "FLAG" result_file' pattern throughout test_26.
#
# Usage:
#   check_result_flag <result_file> <flag>
#
# Returns 0 if the flag is found, 1 otherwise.
# ---------------------------------------------------------------------------
check_result_flag() {
    local result_file="$1"
    local flag="$2"

    if [[ ! -f "${result_file}" ]]; then
        return 1
    fi

    # shellcheck disable=SC2154 # GREP is defined in framework.sh
    "${GREP}" -q "${flag}" "${result_file}" 2>/dev/null
}

# ---------------------------------------------------------------------------
# redact_jwt_fingerprint
#
# Computes a redacted SHA-256 fingerprint for a JWT (first 16 hex chars).
# Used in test output instead of printing the full JWT.
#
# Usage:
#   redact_jwt_fingerprint <jwt>
#   Prints: "<first-16-chars-of-sha256>"
# ---------------------------------------------------------------------------
redact_jwt_fingerprint() {
    local jwt="$1"
    echo -n "${jwt}" | sha256sum | cut -c1-16 || echo "unknown"
}

# ---------------------------------------------------------------------------
# redact_sysinfo_body
#
# Takes raw JSON from /api/system/info and produces a redacted version
# safe for test output: the terminal.key is replaced with "REDACTED".
#
# Usage:
#   redact_sysinfo_body <raw_json>
#   Prints: redacted JSON string
# ---------------------------------------------------------------------------
redact_sysinfo_body() {
    local raw_json="$1"
    echo "${raw_json}" | jq -c '.terminal = "REDACTED"' 2>/dev/null || echo "${raw_json:0:200}"
}

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
                    attempt=$(( attempt + 1 ))
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
                attempt=$(( attempt + 1 ))
                continue
            fi
        fi
    done

    return 1
}

# Login to obtain a JWT for terminal authorization tests.
# Performs retry logic for readiness under parallel load.
# Sets the global variable TERMINAL_LOGIN_JWT on success.
# Returns 0 on success, 1 on failure.
login_for_terminal_tests() {
    local base_url="$1"

    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Terminal login for JWT (pre-sysinfo step)"

    TERMINAL_LOGIN_JWT=""

    local demo_user="${HYDROGEN_DEMO_USER_NAME:-}"
    local demo_pass="${HYDROGEN_DEMO_USER_PASS:-}"
    local demo_api_key="${HYDROGEN_DEMO_API_KEY:-}"

    local login_response=""
    local login_http_code
    for attempt in 1 2 3 4 5; do
        if [[ "${attempt}" -gt 1 ]]; then
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Login retry ${attempt}/5 (server under parallel load)..."
            sleep 1
        fi
        login_http_code=$(curl -s -o /tmp/test26_login_$$.json -w "%{http_code}" --max-time 15 \
            -X POST "${base_url}/api/auth/login" \
            -H "Content-Type: application/json" \
            -d "{\"login_id\":\"${demo_user}\",\"password\":\"${demo_pass}\",\"api_key\":\"${demo_api_key}\",\"tz\":\"UTC\",\"database\":\"Acuranzo\"}" \
            2>/dev/null || echo "000")
        if [[ "${login_http_code}" == "200" ]]; then
            login_response=$(cat /tmp/test26_login_$$.json 2>/dev/null || echo "")
            rm -f /tmp/test26_login_$$.json 2>/dev/null || true
            break
        fi
        rm -f /tmp/test26_login_$$.json 2>/dev/null || true
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Login HTTP ${login_http_code} (attempt ${attempt}), retrying..."
    done

    if [[ -z "${login_response}" || "${login_response}" == *"error"* ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Failed to obtain JWT via login"
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Login response:"
        print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "${login_response}"
        return 1
    fi

    TERMINAL_LOGIN_JWT=$(echo "${login_response}" | jq -r '.token // .access_token // .jwt // empty' 2>/dev/null || echo "")

    if [[ -z "${TERMINAL_LOGIN_JWT}" || "${TERMINAL_LOGIN_JWT}" == "null" ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "No JWT in login response"
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Login response:"
        print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "${login_response}"
        return 1
    fi

    local jwt_fingerprint
    jwt_fingerprint=$(redact_jwt_fingerprint "${TERMINAL_LOGIN_JWT}")
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Login succeeded, JWT obtained (fp: ${jwt_fingerprint})"
    return 0
}

# System-info authorization contract tests
# Verify that /api/system/info omits the terminal object when no valid
# JWT with terminal role is present.

# Test: no JWT -> no terminal object
test_sysinfo_no_terminal_without_jwt() {
    local base_url="$1"
    local output_file="$2"

    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "System-info: no JWT returns no terminal object"

    local response
    response=$(curl -s -o /dev/null -w "%{http_code}" "${base_url}/api/system/info" 2>/dev/null || echo "000")

    if [[ "${response}" != "200" ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Expected HTTP 200 from /api/system/info without JWT, got ${response}"
        return 1
    fi

    # Fetch the body and check for absence of terminal authorization object
    local body
    body=$(curl -s "${base_url}/api/system/info" 2>/dev/null || echo "")

    # The response may contain a generic "terminal" key under services.status,
    # but the authorization block has "url", "protocol", and "key" fields.
    # We check for the presence of the authorization-specific fields.
    if echo "${body}" | jq -e '.terminal.url' >/dev/null 2>&1; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "terminal authorization object (url) present in /api/system/info without JWT (should be omitted)"
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Response body (redacted):"
        local redacted
        redacted=$(redact_sysinfo_body "${body}")
        print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "${redacted}"
        return 1
    fi

    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "No terminal authorization object in /api/system/info without JWT"
    local redacted_body
    redacted_body=$(redact_sysinfo_body "${body}")
    echo "SYSINFO_NOJWT_BODY=${redacted_body}" >> "${output_file}"
    return 0
}

# Test: invalid JWT -> no terminal object
test_sysinfo_no_terminal_with_invalid_jwt() {
    local base_url="$1"
    local output_file="$2"

    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "System-info: invalid JWT returns no terminal object"

    local fake_jwt="eyJhbGci.eyJzdWIi.inZhbGlk"

    # Fetch the body and check — the terminal authorization block has "url",
    # "protocol", and "key" fields. Check for the URL specifically.
    local body
    body=$(curl -s -H "Authorization: Bearer ${fake_jwt}" "${base_url}/api/system/info" 2>/dev/null || echo "")

    if echo "${body}" | jq -e '.terminal.url' >/dev/null 2>&1; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "terminal authorization object (url) present in /api/system/info with invalid JWT (should be omitted)"
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Response body (redacted):"
        local redacted
        redacted=$(redact_sysinfo_body "${body}")
        print_output "${TEST_NUMBER}" "${TEST_COUNTER}" "${redacted}"
        return 1
    fi

    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "No terminal authorization object in /api/system/info with invalid JWT"
    local redacted_body
    redacted_body=$(redact_sysinfo_body "${body}")
    echo "SYSINFO_BADJWT_BODY=${redacted_body}" >> "${output_file}"
    return 0
}

# Test: CORS origin enforcement on /api/system/info
# When terminal is enabled, the CORS allowlist should include the configured origin.
# Without terminal role, CORS headers should not expose terminal-specific origins.
test_sysinfo_cors_origin_enforcement() {
    local base_url="$1"
    local output_file="$2"

    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "System-info: CORS origin enforcement"

    local cors_origin
    cors_origin=$(curl -s -D - -o /dev/null -H "Origin: http://example.com" "${base_url}/api/system/info" 2>/dev/null | tr -d '\r' || echo "")

    # Without terminal role, the response should not contain Access-Control-Allow-Origin
    # pointing to a terminal-specific origin. It may be "*"" (default) or the request origin.
    if echo "${cors_origin}" | grep -qi "access-control-allow-origin: http://localhost"; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "CORS origin exposes localhost terminal origin without terminal role"
        return 1
    fi

    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "CORS origin not exposing terminal-specific origin without terminal role"
    echo "SYSINFO_CORS_HEADERS=${cors_origin}" >> "${output_file}"
    return 0
}

# Test: valid terminal-role JWT -> terminal object present with key match
# Uses a pre-obtained JWT (from login_for_terminal_tests) rather than logging in again.
# Sets global variables TERMINAL_WS_URL, TERMINAL_WS_PROTOCOL, and TERMINAL_WS_KEY
# from the sysinfo response so WebSocket tests can use them.
# Requires HYDROGEN_DEMO_USER_NAME, HYDROGEN_DEMO_USER_PASS, HYDROGEN_DEMO_API_KEY
test_sysinfo_terminal_with_valid_jwt() {
    local base_url="$1"
    local jwt="$2"
    local output_file="$3"

    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "System-info: valid terminal-role JWT returns terminal object"

    if [[ -z "${jwt}" || "${jwt}" == "null" ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "No JWT available (login was not performed)"
        return 1
    fi

    # Redacted fingerprint of the JWT
    local jwt_fingerprint
    jwt_fingerprint=$(redact_jwt_fingerprint "${jwt}")

    # Fetch system/info with the JWT (retry under parallel load — the system
    # status JSON queries the database, which can be slow when two servers
    # and WebSocket stress tests are running simultaneously).
    local sysinfo_body=""
    local sysinfo_http_code
    for attempt in 1 2 3 4 5; do
        if [[ "${attempt}" -gt 1 ]]; then
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "System-info retry ${attempt}/5 (server under parallel load)..."
            sleep 1
        fi
        sysinfo_http_code=$(curl -s -o /tmp/test26_sysinfo_$$.json -w "%{http_code}" --max-time 15 \
            -H "Authorization: Bearer ${jwt}" "${base_url}/api/system/info" 2>/dev/null || echo "000")
        if [[ "${sysinfo_http_code}" == "200" ]]; then
            sysinfo_body=$(cat /tmp/test26_sysinfo_$$.json 2>/dev/null || echo "")
            rm -f /tmp/test26_sysinfo_$$.json 2>/dev/null || true
            break
        fi
        rm -f /tmp/test26_sysinfo_$$.json 2>/dev/null || true
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "System-info HTTP ${sysinfo_http_code} (attempt ${attempt}), retrying..."
    done

    echo "SYSINFO_VALIDJWT_JWT_FP=${jwt_fingerprint}" >> "${output_file}"
    echo "SYSINFO_VALIDJWT_BODY=$(echo "${sysinfo_body}" | jq -c '.terminal // "omitted" | .key = "REDACTED"' 2>/dev/null || echo "omitted")" >> "${output_file}" || true
    if ! echo "${sysinfo_body}" | jq -e '.terminal.url' >/dev/null 2>&1; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "No terminal authorization object in /api/system/info with valid terminal-role JWT"
        return 1
    fi

    # Extract terminal URL, protocol, and key from the sysinfo response.
    # These are used by the WebSocket tests so they connect using the
    # server-provided endpoint rather than reading config directly.
    TERMINAL_WS_URL=$(echo "${sysinfo_body}" | jq -r '.terminal.url // empty' 2>/dev/null || echo "")
    TERMINAL_WS_PROTOCOL=$(echo "${sysinfo_body}" | jq -r '.terminal.protocol // empty' 2>/dev/null || echo "")
    TERMINAL_WS_KEY=$(echo "${sysinfo_body}" | jq -r '.terminal.key // empty' 2>/dev/null || echo "")

    # Verify the terminal object has a non-empty key
    if [[ -n "${TERMINAL_WS_KEY}" ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Terminal authorization object present with key (redacted fp: ${jwt_fingerprint})"
        return 0
    else
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Terminal authorization object present but key is empty"
        return 1
    fi
}
