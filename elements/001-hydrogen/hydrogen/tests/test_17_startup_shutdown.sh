#!/usr/bin/env bash

# Test: Startup/Shutdown
# Tests startup and shutdown of the application with minimal and maximal configurations.
# Also exercises VictoriaLogs shipping via a local mock insert endpoint (env-driven).

# CHANGELOG
# 5.1.0 - 2026-07-28 - Enable VICTORIALOGS_URL against mock sink for blackbox victoria_logs.c coverage
# 5.0.0 - 2025-09-19 - Added server log output and elapsed time display in test name for both min/max configs
# 4.1.0 - 2025-08-08 - Reviewed. Changed JSON filenames.
# 4.0.0 - 2025-07-30 - Overhaul #1
# 3.0.1 - 2025-07-06 - Added missing shellcheck justifications
# 3.0.0 - 2025-07-02 - Complete rewrite to use new modular test libraries
# 2.0.0 - 2025-06-17 - Major refactoring: fixed all shellcheck warnings, improved modularity, enhanced comments
# 1.0.0 - Initial version

set -euo pipefail

# Test configuration
TEST_NAME="Startup/Shutdown"
TEST_ABBR="UPD"
TEST_NUMBER="17"
TEST_COUNTER=0
TEST_VERSION="5.1.0"

# shellcheck source=tests/lib/framework.sh # Reference framework directly
[[ -n "${FRAMEWORK_GUAR:-}" ]] || source "$(dirname "${BASH_SOURCE[0]}")/lib/framework.sh"
setup_test_environment

# Test configuration
MIN_CONFIG="${SCRIPT_DIR}/configs/hydrogen_test_${TEST_NUMBER}_startup_min.json"
MAX_CONFIG="${SCRIPT_DIR}/configs/hydrogen_test_${TEST_NUMBER}_startup_max.json"
STARTUP_TIMEOUT=10    # Seconds to wait for startup
SHUTDOWN_TIMEOUT=90   # Hard limit on shutdown time
SHUTDOWN_ACTIVITY_TIMEOUT=5  # Timeout if no new log activity
LOG_MIN="${LOGS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}_min.log"
LOG_MAX="${LOGS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}_max.log"

MOCK_VL_SCRIPT="${SCRIPT_DIR}/lib/mock_victoria_logs/server.js"
MOCK_VL_PID=""
MOCK_VL_PORT=""
MOCK_VL_LOG=""
MOCK_VL_RECEIPT=""

stop_mock_vl() {
    if [[ -z "${MOCK_VL_PID}" ]]; then
        return 0
    fi
    if ps -p "${MOCK_VL_PID}" >/dev/null 2>&1; then
        kill -TERM "${MOCK_VL_PID}" 2>/dev/null || true
        local waited=0
        while (( waited < 20 )) && ps -p "${MOCK_VL_PID}" >/dev/null 2>&1; do
            sleep 0.1
            waited=$((waited + 1))
        done
        if ps -p "${MOCK_VL_PID}" >/dev/null 2>&1; then
            kill -KILL "${MOCK_VL_PID}" 2>/dev/null || true
        fi
    fi
    wait "${MOCK_VL_PID}" 2>/dev/null || true
    MOCK_VL_PID=""
}

cleanup_mock_vl_env() {
    # shellcheck disable=SC2310 # Cleanup must continue
    stop_mock_vl || true
    unset VICTORIALOGS_URL VICTORIALOGS_LVL \
        K8S_NAMESPACE K8S_POD_NAME K8S_NODE_NAME K8S_CONTAINER_NAME 2>/dev/null || true
}
trap cleanup_mock_vl_env EXIT

start_mock_vl() {
    if ! command -v node >/dev/null 2>&1; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "node not available; cannot start mock VictoriaLogs"
        return 1
    fi
    if [[ ! -f "${MOCK_VL_SCRIPT}" ]]; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "mock VictoriaLogs script missing: ${MOCK_VL_SCRIPT}"
        return 1
    fi

    MOCK_VL_LOG="${LOG_PREFIX}${TIMESTAMP}_mock_vl.log"
    MOCK_VL_RECEIPT="${LOG_PREFIX}${TIMESTAMP}_mock_vl_receipt.txt"
    : >"${MOCK_VL_RECEIPT}"

    # port 0 = ephemeral; server prints READY <port>
    node "${MOCK_VL_SCRIPT}" 0 "${MOCK_VL_RECEIPT}" >"${MOCK_VL_LOG}" 2>&1 &
    MOCK_VL_PID=$!

    local waited=0
    while (( waited < 50 )); do
        if [[ -s "${MOCK_VL_LOG}" ]] && "${GREP}" -qE '^READY [0-9]+$' "${MOCK_VL_LOG}"; then
            MOCK_VL_PORT=$("${AWK}" "/^READY / {print \$2; exit}" "${MOCK_VL_LOG}")
            if [[ -n "${MOCK_VL_PORT}" ]]; then
                return 0
            fi
        fi
        if ! ps -p "${MOCK_VL_PID}" >/dev/null 2>&1; then
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "mock VictoriaLogs exited early; log: ${MOCK_VL_LOG}"
            MOCK_VL_PID=""
            return 1
        fi
        sleep 0.1
        waited=$((waited + 1))
    done
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "mock VictoriaLogs not READY within 5s"
    return 1
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

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Start mock VictoriaLogs sink"
# shellcheck disable=SC2310 # Non-fatal: lifecycle still runs without VL
if start_mock_vl; then
    export VICTORIALOGS_URL="http://127.0.0.1:${MOCK_VL_PORT}/insert/jsonline?_stream_fields=app,kubernetes_namespace,kubernetes_pod_name,kubernetes_container_name"
    export VICTORIALOGS_LVL="DEBUG"
    export K8S_NAMESPACE="bb-test-17"
    export K8S_POD_NAME="hydrogen-test-17"
    export K8S_NODE_NAME="bb-node-17"
    export K8S_CONTAINER_NAME="hydrogen"
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "VICTORIALOGS_URL=${VICTORIALOGS_URL}"
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Mock VictoriaLogs listening on 127.0.0.1:${MOCK_VL_PORT}"
    PASS_COUNT=$((PASS_COUNT + 1))
else
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Failed to start mock VictoriaLogs"
    EXIT_CODE=1
fi

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Validate Minimum Configuration File"

# shellcheck disable=SC2310 # We want to continue even if the test fails
if validate_config_file "${MIN_CONFIG}"; then
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Validated Minimum Configuration File"
else
    EXIT_CODE=1
fi

# Test minimal configuration (with VL env if mock started)
config_name=$(basename "${MIN_CONFIG}" .json)
run_lifecycle_test "${MIN_CONFIG}" "${config_name}" "${DIAG_TEST_DIR}" "${STARTUP_TIMEOUT}" "${SHUTDOWN_TIMEOUT}" "${SHUTDOWN_ACTIVITY_TIMEOUT}" "${HYDROGEN_BIN}" "${LOG_MIN}" "PASS_COUNT" "EXIT_CODE"

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Validate Maximum Configuration File"

# shellcheck disable=SC2310 # We want to continue even if the test fails
if validate_config_file "${MAX_CONFIG}"; then
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Validated Maximum Configuration File"
else
    EXIT_CODE=1
fi

# Test maximal configuration
config_name=$(basename "${MAX_CONFIG}" .json)
run_lifecycle_test "${MAX_CONFIG}" "${config_name}" "${DIAG_TEST_DIR}" "${STARTUP_TIMEOUT}" "${SHUTDOWN_TIMEOUT}" "${SHUTDOWN_ACTIVITY_TIMEOUT}" "${HYDROGEN_BIN}" "${LOG_MAX}" "PASS_COUNT" "EXIT_CODE"

# Confirm the mock received at least one insert (happy-path HTTP shipping)
print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "VictoriaLogs mock received inserts"
if [[ -n "${MOCK_VL_RECEIPT}" && -f "${MOCK_VL_RECEIPT}" ]]; then
    receipt_lines=$(wc -l <"${MOCK_VL_RECEIPT}" | tr -d ' ')
    if [[ "${receipt_lines}" -gt 0 ]]; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Mock receipt lines: ${receipt_lines} (file: ${MOCK_VL_RECEIPT})"
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "VictoriaLogs sink accepted ${receipt_lines} POST(s)"
        PASS_COUNT=$((PASS_COUNT + 1))
    else
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Mock VictoriaLogs received no POSTs"
        EXIT_CODE=1
    fi
else
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "No mock VictoriaLogs receipt file"
    EXIT_CODE=1
fi

# Add server logs to output
print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Min Config Server Log: ..${LOG_MIN}"
print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Max Config Server Log: ..${LOG_MAX}"
if [[ -n "${MOCK_VL_LOG}" ]]; then
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Mock VictoriaLogs Log: ..${MOCK_VL_LOG}"
fi

# Extract elapsed times from logs and update test name
min_elapsed="" max_elapsed=""
if [[ -f "${LOG_MIN}" ]]; then
    min_elapsed=$("${AWK}" "/Total elapsed time:/ {print \$NF}" "${LOG_MIN}" 2>/dev/null | tail -1 || true)
fi
if [[ -f "${LOG_MAX}" ]]; then
    max_elapsed=$("${AWK}" "/Total elapsed time:/ {print \$NF}" "${LOG_MAX}" 2>/dev/null | tail -1 || true)
fi

# Update test name with timing info
if [[ -n "${min_elapsed}" || -n "${max_elapsed}" ]]; then
    timing_info=""
    if [[ -n "${min_elapsed}" ]]; then
        timing_info="${min_elapsed} / "
    fi
    if [[ -n "${max_elapsed}" ]]; then
        if [[ -n "${timing_info}" ]]; then
            timing_info="${timing_info} ${max_elapsed}"
        else
            timing_info="Max: ${max_elapsed}"
        fi
    fi
    TEST_NAME="${TEST_NAME}  {BLUE}cycle: ${timing_info}{RESET}"
fi

# Print completion table
print_test_completion "${TEST_NAME}" "${TEST_ABBR}" "${TEST_NUMBER}" "${TEST_VERSION}"

# Return status code if sourced, exit if run standalone
${ORCHESTRATION:-false} && return "${EXIT_CODE}" || exit "${EXIT_CODE}"
