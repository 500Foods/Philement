#!/usr/bin/env bash

# Test: MailRelay Inbound SMTP Listener (Phase 12)
#
# Validates Hydrogen's inbound SMTP relay listener end-to-end.
# Hydrogen is configured with MailRelay.InboundEnabled=true on a
# dedicated ListenPort. The test connects via SMTP, injects a message
# with MAIL FROM / RCPT TO / DATA, and verifies that Hydrogen
# accepts and queues the message (250 response) and that the message
# reaches the outbound SMTP sink (mailval).
#
# Secret config values are supplied via environment variables resolved
# by the config loader; no credentials are committed.

# CHANGELOG
# 1.0.0 - 2026-09-07 - Initial MailRelay inbound SMTP relay blackbox test.

set -euo pipefail

# Test configuration
TEST_NAME="MailRelay Inbound SMTP Listener"
TEST_ABBR="MRI"
TEST_NUMBER="61"
TEST_COUNTER=0
TEST_VERSION="1.0.0"

# shellcheck source=tests/lib/framework.sh # Reference framework directly
[[ -n "${FRAMEWORK_GUARD:-}" ]] || source "$(dirname "${BASH_SOURCE[0]}")/lib/framework.sh"
setup_test_environment

# Locate the prebuilt mailval binary. Override with MAILVAL_BIN if needed.
MAILVAL_DIR="${PROJECT_DIR}/extras/mailval"
MAILVAL_BIN="${MAILVAL_BIN:-${MAILVAL_DIR}/build/mailval}"
MAILVAL_CERT="${MAILVAL_DIR}/mailval.pem"
MAILVAL_KEY="${MAILVAL_DIR}/mailval.key"

# Dedicated test ports (from the MAILRELAY_PLAN port scheme).
SINK_PORT=5580
INBOUND_PORT=5581

# Track background PIDs for a safety-net cleanup trap.
HYDROGEN_PIDS=()
MAILVAL_PIDS=()
# shellcheck disable=SC2329 # invoked via trap EXIT
cleanup_processes() {
    local p
    for p in "${HYDROGEN_PIDS[@]:-}"; do
        kill -SIGINT "${p}" 2>/dev/null || true
    done
    for p in "${MAILVAL_PIDS[@]:-}"; do
        kill -INT "${p}" 2>/dev/null || true
    done
    if declare -f _hydrogen_owned_exit_trap >/dev/null 2>&1; then
        _hydrogen_owned_exit_trap
    fi
}
# shellcheck disable=SC2329 # function is invoked via trap
trap cleanup_processes EXIT

# --- Locate Hydrogen Binary ---
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

# --- Locate mailval Binary ---
print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Locate mailval Binary"
if [[ -x "${MAILVAL_BIN}" ]]; then
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Using mailval binary: ${MAILVAL_BIN}"
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "mailval binary found and executable"
    PASS_COUNT=$(( PASS_COUNT + 1 ))
else
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "mailval binary not found at ${MAILVAL_BIN} (build extras/mailval first)"
    EXIT_CODE=1
fi

# --- Validate Configuration File ---
CONFIG_FILE="${SCRIPT_DIR}/configs/hydrogen_test_${TEST_NUMBER}_mailrelay_inbound.json"
print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Validate Configuration File"
# shellcheck disable=SC2310 # We want to continue even if the test fails
if validate_config_file "${CONFIG_FILE}"; then
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Configuration file found: $(basename "${CONFIG_FILE}")"
    PASS_COUNT=$(( PASS_COUNT + 1 ))
else
    EXIT_CODE=1
fi

# --- Start mailval SMTP sink ---
print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Start mailval SMTP sink"
MAILDATA_DIR="${DIAG_TEST_DIR}/mailval_inbound"
mkdir -p "${MAILDATA_DIR}"
MAILVAL_LOG="${LOGS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}_mailval.log"
true > "${MAILVAL_LOG}"

# Generate cert if needed
if [[ ! -f "${MAILVAL_CERT}" || ! -f "${MAILVAL_KEY}" ]]; then
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Generating mailval self-signed cert/key"
    bash "${MAILVAL_DIR}/gen_cert.sh" >/dev/null 2>&1 || true
fi

print_command "${TEST_NUMBER}" "${TEST_COUNTER}" "$(basename "${MAILVAL_BIN}") --smtp-port ${SINK_PORT} --data-dir ${MAILDATA_DIR}"
"${MAILVAL_BIN}" --smtp-port "${SINK_PORT}" --data-dir "${MAILDATA_DIR}" > "${MAILVAL_LOG}" 2>&1 &
local_mailval_pid=$!
MAILVAL_PIDS+=("${local_mailval_pid}")

# Wait for the sink to be ready.
sink_ready=false
for _ in $(seq 1 100); do
    if "${TIMEOUT}" 1 bash -c "</dev/tcp/127.0.0.1/${SINK_PORT}" 2>/dev/null; then
        sink_ready=true
        break
    fi
    sleep 0.1
done

if [[ "${sink_ready}" == "true" ]]; then
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "mailval SMTP sink is accepting connections on port ${SINK_PORT}"
    PASS_COUNT=$(( PASS_COUNT + 1 ))
else
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "mailval SMTP sink failed to start on port ${SINK_PORT}"
    EXIT_CODE=1
fi

# --- Start Hydrogen with InboundEnabled ---
print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Start Hydrogen with inbound SMTP listener"
HYDROGEN_LOG="${LOGS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}_hydrogen.log"
true > "${HYDROGEN_LOG}"

print_command "${TEST_NUMBER}" "${TEST_COUNTER}" "$(basename "${HYDROGEN_BIN}") --config ${CONFIG_FILE}"
export MAILRELAY_INBOUND_TEST_MODE=1
"${HYDROGEN_BIN}" --config "${CONFIG_FILE}" --port "${HYDROGEN_PORT:-0}" > "${HYDROGEN_LOG}" 2>&1 &
HYDROGEN_PID=$!
HYDROGEN_PIDS+=("${HYDROGEN_PID}")

# Wait for Hydrogen to be ready (listen on inbound port).
hydrogen_ready=false
for _ in $(seq 1 250); do
    if "${TIMEOUT}" 1 bash -c "</dev/tcp/127.0.0.1/${INBOUND_PORT}" 2>/dev/null; then
        hydrogen_ready=true
        break
    fi
    # Check if process died
    if ! kill -0 "${HYDROGEN_PID}" 2>/dev/null; then
        break
    fi
    sleep 0.1
done

if [[ "${hydrogen_ready}" == "true" ]]; then
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Hydrogen is accepting SMTP connections on inbound port ${INBOUND_PORT}"
    PASS_COUNT=$(( PASS_COUNT + 1 ))
else
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Hydrogen did not start inbound SMTP listener on port ${INBOUND_PORT}"
    EXIT_CODE=1
fi

# --- SMTP Client: inject a message ---
print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Inject SMTP message through inbound listener"
SMTP_INJECT_LOG="${LOGS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}_inject.log"
injected_ok=false

# Use a bash TCP connection to send the SMTP dialogue.
{
    exec 3<>"/dev/tcp/127.0.0.1/${INBOUND_PORT}"

    # Read greeting
    read -r -t 5 -u 3 greeting
    echo "GREETING: ${greeting}" >> "${SMTP_INJECT_LOG}"

    # EHLO
    echo "EHLO testrunner.example.com" >&3
    read -r -t 5 -u 3 ehlo_resp
    echo "EHLO RESP: ${ehlo_resp}" >> "${SMTP_INJECT_LOG}"
    while [[ "${ehlo_resp:0:4}" == "250-" ]]; do
        read -r -t 5 -u 3 ehlo_resp
        echo "EHLO RESP: ${ehlo_resp}" >> "${SMTP_INJECT_LOG}"
    done

    # MAIL FROM
    echo "MAIL FROM:<alice@testrunner.example.com>" >&3
    read -r -t 5 -u 3 mail_resp
    echo "MAIL RESP: ${mail_resp}" >> "${SMTP_INJECT_LOG}"

    # RCPT TO — points at the mailval sink
    echo "RCPT TO:<bob@example.com>" >&3
    read -r -t 5 -u 3 rcpt_resp
    echo "RCPT RESP: ${rcpt_resp}" >> "${SMTP_INJECT_LOG}"

    # DATA
    SUBJECT="Test inbound SMTP relay $(( RANDOM % 100000 ))"
    echo "DATA" >&3
    read -r -t 5 -u 3 data_resp
    echo "DATA RESP: ${data_resp}" >> "${SMTP_INJECT_LOG}"

    # Message body
    printf 'From: alice@testrunner.example.com\r\n' >&3
    printf 'To: bob@example.com\r\n' >&3
    printf 'Subject: %s\r\n' "${SUBJECT}" >&3
    printf '\r\n' >&3
    printf 'This is a test message via inbound SMTP relay.\r\n' >&3
    printf '.\r\n' >&3
    read -r -t 10 -u 3 final_resp
    echo "FINAL RESP: ${final_resp}" >> "${SMTP_INJECT_LOG}"

    # QUIT
    echo "QUIT" >&3
    read -r -t 5 -u 3 quit_resp
    echo "QUIT RESP: ${quit_resp}" >> "${SMTP_INJECT_LOG}"

    exec 3>&-

    # Check for 250 OK response on final
    # shellcheck disable=SC2086 # intentional: checking first 3 chars only
    if [[ "${final_resp:0:3}" == "250" ]]; then
        injected_ok=true
    fi
} 2>> "${SMTP_INJECT_LOG}" || true

if [[ "${injected_ok}" == "true" ]]; then
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "SMTP message injected and accepted (250 OK)"
    PASS_COUNT=$(( PASS_COUNT + 1 ))
else
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "SMTP message injection failed"
    EXIT_CODE=1
fi

# --- Wait for message to reach mailval sink ---
print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Wait for message delivery to mailval sink"
delivered=false
for _ in $(seq 1 100); do
    # Check mailval transcript for captured message
    if compgen -G "${MAILDATA_DIR}/session_*.json" > /dev/null 2>&1; then
        if jq -e '.commands[]? | select(.cmd == "DATA")' "${MAILDATA_DIR}"/session_*.json >/dev/null 2>&1; then
            delivered=true
            break
        fi
    fi
    sleep 0.2
done

if [[ "${delivered}" == "true" ]]; then
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Message delivered to mailval sink"
    PASS_COUNT=$(( PASS_COUNT + 1 ))
else
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Message was not delivered to mailval sink within timeout"
    EXIT_CODE=1
fi

# --- Shutdown ---
print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Shutdown Hydrogen"
if [[ "${HYDROGEN_PID}" -gt 0 ]] && kill -0 "${HYDROGEN_PID}" 2>/dev/null; then
    kill -SIGINT "${HYDROGEN_PID}" 2>/dev/null || true
    # Wait for graceful shutdown
    for _ in $(seq 1 30); do
        if ! kill -0 "${HYDROGEN_PID}" 2>/dev/null; then
            break
        fi
        sleep 1
    done
    # Force kill if still running
    if kill -0 "${HYDROGEN_PID}" 2>/dev/null; then
        kill -KILL "${HYDROGEN_PID}" 2>/dev/null || true
    fi
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Hydrogen shut down"
    PASS_COUNT=$(( PASS_COUNT + 1 ))
else
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Hydrogen already shut down"
    PASS_COUNT=$(( PASS_COUNT + 1 ))
fi

# Clean up mailval
for p in "${MAILVAL_PIDS[@]:-}"; do
    kill -INT "${p}" 2>/dev/null || true
done

# --- Final Result ---
print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Test Summary"
print_result "${TEST_NUMBER}" "${TEST_COUNTER}" "${EXIT_CODE}" "Inbound SMTP relay test: ${PASS_COUNT} sub-tests passed"

exit "${EXIT_CODE}"
