#!/usr/bin/env bash

# Test: MailRelay Outbound
#
# Validates Hydrogen Mail Relay outbound delivery end-to-end using our own C
# mail validator (extras/mailval) as an SMTP sink. Hydrogen is started with
# MailRelay.Test.SendRawOnLaunch enabled, which fires one synchronous raw send
# during launch. The test asserts the message was captured by the sink.
#
# Two transport variants are exercised:
#   1. Plaintext SMTP  (mailval on 127.0.0.1:5570)
#   2. STARTTLS SMTP   (mailval on 127.0.0.1:5571, self-signed cert)
#
# Secrets (SMTP password, CA cert path) are supplied via environment variables
# resolved by the config loader, so no credentials live in the committed config.

# CHANGELOG
# 1.0.2 - 2026-07-07 - Updated SendRawOnLaunch log assertion to match async enqueue message.
# 1.0.1 - 2026-07-06 - Hardened sink readiness probe and stored-message capture selection.
# 1.0.0 - 2026-07-06 - Initial MailRelay outbound blackbox test (plaintext + TLS).

set -euo pipefail

# Test configuration
TEST_NAME="MailRelay Outbound"
TEST_ABBR="MRO"
TEST_NUMBER="57"
TEST_COUNTER=0
TEST_VERSION="1.0.2"

# shellcheck source=tests/lib/framework.sh # Reference framework directly
[[ -n "${FRAMEWORK_GUARD:-}" ]] || source "$(dirname "${BASH_SOURCE[0]}")/lib/framework.sh"
setup_test_environment

# Locate the prebuilt mailval binary. Override with MAILVAL_BIN if needed.
MAILVAL_DIR="${PROJECT_DIR}/extras/mailval"
MAILVAL_BIN="${MAILVAL_BIN:-${MAILVAL_DIR}/build/mailval}"
MAILVAL_CERT="${MAILVAL_DIR}/mailval.pem"
MAILVAL_KEY="${MAILVAL_DIR}/mailval.key"

# SMTP sink ports (dedicated 557x range from the plan).
PLAINTEXT_PORT=5570
TLS_PORT=5571

# Timeouts
STARTUP_TIMEOUT=25
SHUTDOWN_TIMEOUT=30
SHUTDOWN_ACTIVITY_TIMEOUT=5

# Track background PIDs for a safety-net cleanup trap.
HYDROGEN_PIDS=()
MAILVAL_PIDS=()
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
trap cleanup_processes EXIT

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

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Locate mailval Binary"
if [[ -x "${MAILVAL_BIN}" ]]; then
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Using mailval binary: ${MAILVAL_BIN}"
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "mailval binary found and executable"
    PASS_COUNT=$(( PASS_COUNT + 1 ))
else
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "mailval binary not found at ${MAILVAL_BIN} (build extras/mailval first)"
    EXIT_CODE=1
fi

# Validate both configuration files.
CONFIG_PLAIN="${SCRIPT_DIR}/configs/hydrogen_test_${TEST_NUMBER}_mailrelay_outbound.json"
CONFIG_TLS="${SCRIPT_DIR}/configs/hydrogen_test_${TEST_NUMBER}_mailrelay_outbound_tls.json"

for cfg in "${CONFIG_PLAIN}" "${CONFIG_TLS}"; do
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Validate Configuration File: $(basename "${cfg}")"
    # shellcheck disable=SC2310 # We want to continue even if the test fails
    if validate_config_file "${cfg}"; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Configuration file found: $(basename "${cfg}")"
        PASS_COUNT=$(( PASS_COUNT + 1 ))
    else
        EXIT_CODE=1
    fi
done

# Run one Mail Relay outbound subtest against the SMTP sink.
# Arguments: <label> <config> <port> <tls:0|1> <subject_marker>
run_mailrelay_outbound() {
    local label="$1"
    local config_file="$2"
    local port="$3"
    local use_tls="$4"
    local subject_marker="$5"

    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "${label}"

    local maildata_dir="${DIAG_TEST_DIR}/mailval_${port}"
    mkdir -p "${maildata_dir}"
    local mailval_log="${LOGS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}_mailval_${port}.log"
    local hydrogen_log="${LOGS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}_hydrogen_${port}.log"
    true > "${mailval_log}"

    # Supply secrets via env vars resolved by the config loader.
    export MAILRELAY_TEST_PASSWORD="${MAILRELAY_TEST_PASSWORD:-mailrelay-local-test-only}"
    if [[ "${use_tls}" -eq 1 ]]; then
        if [[ ! -f "${MAILVAL_CERT}" || ! -f "${MAILVAL_KEY}" ]]; then
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Generating mailval self-signed cert/key"
            bash "${MAILVAL_DIR}/gen_cert.sh" >/dev/null 2>&1 || true
        fi
        export MAILRELAY_MAILVAL_CERT="${MAILVAL_CERT}"
    fi

    # Start the SMTP sink.
    local mailval_args=( "--smtp-port" "${port}" "--data-dir" "${maildata_dir}" )
    if [[ "${use_tls}" -eq 1 ]]; then
        mailval_args+=( "--cert" "${MAILVAL_CERT}" "--key" "${MAILVAL_KEY}" )
    fi
    print_command "${TEST_NUMBER}" "${TEST_COUNTER}" "$(basename "${MAILVAL_BIN}") ${mailval_args[*]}"
    "${MAILVAL_BIN}" "${mailval_args[@]}" > "${mailval_log}" 2>&1 &
    local mailval_pid=$!
    MAILVAL_PIDS+=("${mailval_pid}")

    # Wait for the sink to accept connections.
    local ready=false
    for ((i = 0; i < 100; i++)); do
        if "${TIMEOUT}" 1 bash -c "</dev/tcp/127.0.0.1/${port}" 2>/dev/null; then
            ready=true
            break
        fi
        sleep 0.05
    done
    if [[ "${ready}" = false ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${label} - mailval did not start listening on ${port}"
        kill -INT "${mailval_pid}" 2>/dev/null || true
        return 1
    fi

    # Start Hydrogen; SendRawOnLaunch fires during launch (before STARTUP COMPLETE).
    local hydrogen_pid_var="MAILRELAY_HYDROGEN_PID_${port}"
    local hydrogen_pid=""
    eval "${hydrogen_pid_var}=''"
    # shellcheck disable=SC2310 # We want to continue even if the test fails
    if ! start_hydrogen_with_pid "${config_file}" "${hydrogen_log}" "${STARTUP_TIMEOUT}" "${HYDROGEN_BIN}" "${hydrogen_pid_var}"; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${label} - Hydrogen failed to start"
        kill -INT "${mailval_pid}" 2>/dev/null || true
        return 1
    fi
    hydrogen_pid=$(eval "echo \${${hydrogen_pid_var}}")
    if [[ -z "${hydrogen_pid}" ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${label} - Hydrogen PID was not captured"
        kill -INT "${mailval_pid}" 2>/dev/null || true
        return 1
    fi
    HYDROGEN_PIDS+=("${hydrogen_pid}")

    # Stop Hydrogen cleanly.
    # shellcheck disable=SC2310 # We want to continue even if the test fails
    if ! stop_hydrogen "${hydrogen_pid}" "${hydrogen_log}" "${SHUTDOWN_TIMEOUT}" "${SHUTDOWN_ACTIVITY_TIMEOUT}" "${DIAG_TEST_DIR}"; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${label} - Hydrogen shutdown failed"
        kill -INT "${mailval_pid}" 2>/dev/null || true
        return 1
    fi

    # Assert Hydrogen reported a successful SendRawOnLaunch enqueue.
    if ! "${GREP}" -q "SendRawOnLaunch smoke test enqueued" "${hydrogen_log}" 2>/dev/null; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${label} - Hydrogen log missing 'SendRawOnLaunch smoke test enqueued'"
        kill -INT "${mailval_pid}" 2>/dev/null || true
        return 1
    fi

    local capture_file=""
    local candidate=""
    local stored=""
    local subject=""
    for ((i = 0; i < 80; i++)); do
        for candidate in "${maildata_dir}"/mailval_smtp_*.json; do
            [[ -f "${candidate}" ]] || continue
            stored=$(jq -r '[.commands[]? | select(.dir=="meta" and .key=="stored_uid")] | .[0].value // empty' "${candidate}")
            subject=$(jq -r '[.commands[]? | select(.dir=="meta" and .key=="subject")] | .[0].value // empty' "${candidate}")
            if [[ "${stored}" = "yes" ]]; then
                capture_file="${candidate}"
                break
            fi
        done
        if [[ -n "${capture_file}" ]]; then
            break
        fi
        sleep 0.1
    done
    if [[ -z "${capture_file}" ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${label} - no stored SMTP capture transcript written by sink"
        kill -INT "${mailval_pid}" 2>/dev/null || true
        return 1
    fi

    if [[ "${stored}" != "yes" ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${label} - sink did not store message (stored_uid=${stored})"
        kill -INT "${mailval_pid}" 2>/dev/null || true
        return 1
    fi
    if [[ "${subject}" != *"${subject_marker}"* ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${label} - subject mismatch ('${subject}')"
        kill -INT "${mailval_pid}" 2>/dev/null || true
        return 1
    fi

    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${label} - captured: stored_uid=${stored} subject='${subject}'"
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "${label} - message delivered and captured (stored_uid=yes, subject matched)"
    PASS_COUNT=$(( PASS_COUNT + 1 ))

    # Stop the sink.
    kill -INT "${mailval_pid}" 2>/dev/null || true
    return 0
}

# Only proceed if prerequisites are present.
if [[ "${EXIT_CODE}" -eq 0 ]]; then
    # shellcheck disable=SC2310 # We want to continue even if the test fails
    run_mailrelay_outbound "MailRelay Outbound (plaintext SMTP)" "${CONFIG_PLAIN}" "${PLAINTEXT_PORT}" 0 "(plaintext)" || EXIT_CODE=1
    # shellcheck disable=SC2310 # We want to continue even if the test fails
    run_mailrelay_outbound "MailRelay Outbound (STARTTLS SMTP)" "${CONFIG_TLS}" "${TLS_PORT}" 1 "(tls)" || EXIT_CODE=1
else
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Skipping MailRelay outbound runs due to prerequisite failures"
fi

# Print test completion summary
print_test_completion "${TEST_NAME}" "${TEST_ABBR}" "${TEST_NUMBER}" "${TEST_VERSION}"

# Return status code if sourced, exit if run standalone
${ORCHESTRATION:-false} && return "${EXIT_CODE}" || exit "${EXIT_CODE}"
