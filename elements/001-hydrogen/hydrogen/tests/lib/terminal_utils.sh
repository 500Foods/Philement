#!/usr/bin/env bash

# Terminal Utilities Library
# Terminal-specific helpers for test_26_terminal.sh and future terminal tests:
# SQLite isolation, WebSocket config resolution, result-flag checking, and redaction.

# LIBRARY FUNCTIONS
# prepare_sqlite_isolation()
# resolve_terminal_websocket_config()
# check_result_flag()
# redact_jwt_fingerprint()
# redact_sysinfo_body()

# CHANGELOG
# 1.0.0 - 2026-09-12 - Extracted from test_26_terminal.sh: SQLite isolation, WebSocket config,
#                      result-flag checking, JWT fingerprint, and sysinfo redaction helpers

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
TERMINAL_UTILS_VERSION="1.0.0"

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
        websocket_protocol=$(jq -r '.WebSocketServer.Protocol // "terminal"' "${config_file}" 2>/dev/null || echo "terminal")
    fi

    local websockets_key="${TERMINAL_WS_KEY:-${WEBSOCKET_KEY:-}}"

    RESOLVED_WS_URL="${ws_url}"
    RESOLVED_WS_PROTOCOL="${websocket_protocol}"
    RESOLVED_WS_KEY="${websockets_key}"
    export WEBSOCKET_KEY="${websockets_key}"
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
