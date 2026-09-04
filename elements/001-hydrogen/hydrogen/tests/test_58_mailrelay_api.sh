#!/usr/bin/env bash

# Test: MailRelay API - Multi-Engine Authenticated End-to-End
#
# Validates the Hydrogen Mail Relay REST API across all supported database engines. Each engine is exercised with both plaintext SMTP and STARTTLS SMTP delivery
# against the local C mail validator (extras/mailval). The test logs in as the mailadmin account, previews and sends the seeded mail.test template, and
# verifies the message was captured by the SMTP sink.
#
# Required environment variables:
#   HYDROGEN_MAILADMIN_NAME, HYDROGEN_MAILADMIN_PASS, HYDROGEN_MAILADMIN_EMAIL
#   HYDROGEN_DEMO_USER_NAME, HYDROGEN_DEMO_USER_PASS, HYDROGEN_DEMO_API_KEY
#   HYDROGEN_DEMO_JWT_KEY, PAYLOAD_KEY (via existing configs)

# FUNCTIONS
# (Helpers live in tests/lib/mailrelay_api_helpers.sh)

# CHANGELOG
# 2.8.3 - 2026-09-04 - Pair TEST/PASS/FAIL; print_subtest owns TEST_COUNTER
# 2.8.2 - 2026-09-04 - Queue.Persist on for MySQL/MariaDB after LONGLONG bind fix.
# 2.8.1 - 2026-08-21 - STARTTLS CAPath written into runtime config.
# 2.8.0 - 2026-08-21 - Split helpers; sequential plaintext/STARTTLS per engine;
#                      capped parallelism and longer waits under full-suite load.
# 2.7.0 - 2026-08-21 - 11.4: second send with the same idempotency_key
#                      returns the first message_id and does not double-deliver
#                      when Queue.Persist is on (all engines).
# 2.6.0 - 2026-07-30 - MailRepoProbeOnLaunch: H.mail template/route/cleanup/event repo helpers via ephemeral Lua.
# 2.5.0 - 2026-07-29 - OTP launch seam: wrong-code (otp_increment_attempts) + max-attempts (otp_mark_max_attempts) markers and DB status checks.
# 2.4.0 - 2026-07-15 - Moved listeners from Linux ephemeral range 55800-55831 to dedicated 15800-15831 ports to prevent full-suite client connection collisions.
# 2.3.0 - 2026-07-14 - Added launch-time OTP send + self-verify coverage subtest (Seam A, SendOtpOnLaunch) asserting MAILRELAY_OTP_LAUNCH_SENT/VERIFIED markers and DB row consumption.
# 2.2.0 - 2026-07-09 - Parallel fail-soft: variant/engine helpers always return 0 after writing PASS/FAIL so set -e wait does not abort the suite mid-run; wait -n/wait pid tolerate non-zero children.
# 2.1.0 - 2026-07-09 - Replaced migration completion wait with canonical "READY FOR REQUESTS" signal; reduced STARTUP_TIMEOUT to 15s and migration wait to 30s READY_TIMEOUT for faster failure on unreachable engines.
# 2.0.0 - 2026-07-08 - Expanded to full 7-engine × plaintext/STARTTLS matrix; fixed parallel result-file race.
# 1.0.0 - 2026-07-08 - Initial SQLite-only API blackbox implementation.

set -euo pipefail

# Test configuration
TEST_NAME="MailRelay API"
TEST_ABBR="MRA"
TEST_NUMBER="58"
TEST_COUNTER=0
TEST_VERSION="2.8.3"

# shellcheck source=tests/lib/framework.sh # Reference framework directly
[[ -n "${FRAMEWORK_GUARD:-}" ]] || source "$(dirname "${BASH_SOURCE[0]}")/lib/framework.sh"
setup_test_environment

# shellcheck source=tests/lib/mailrelay_api_helpers.sh # Split for the 1000-line cap
source "$(dirname "${BASH_SOURCE[0]}")/lib/mailrelay_api_helpers.sh"

declare -a PARALLEL_PIDS
declare -A MAILRELAY_API_ENGINES

# display_name -> engine:web_plain:mail_plain:web_tls:mail_tls
MAILRELAY_API_ENGINES=(
    ["PostgreSQL"]="postgres:15800:15801:15802:15803"
    ["MySQL"]="mysql:15804:15805:15806:15807"
    ["SQLite"]="sqlite:15808:15809:15810:15811"
    ["DB2"]="db2:15812:15813:15814:15815"
    ["MariaDB"]="mariadb:15816:15817:15818:15819"
    ["CockroachDB"]="cockroachdb:15820:15821:15822:15823"
    ["YugabyteDB"]="yugabytedb:15824:15825:15826:15827"
)

# Timeouts (seconds) — slightly generous so the 50s batch does not starve SMTP/HTTP.
STARTUP_TIMEOUT=20
SHUTDOWN_TIMEOUT=30
SHUTDOWN_ACTIVITY_TIMEOUT=5
HTTP_READY_TIMEOUT=20
READY_TIMEOUT=45
MAILVAL_READY_TIMEOUT=10
CAPTURE_TIMEOUT=25
LIFECYCLE_CAPTURE_TIMEOUT=20
LIFECYCLE_SETTLE=4
IDEMPOTENCY_SETTLE=1
MAX_ENGINE_JOBS=4

OTP_WEB_PORT=15830
OTP_MAILVAL_PORT=15831
OTP_RECIPIENT="mailrelay-otp-launch@hydrogen.local"
OTP_MAX_RECIPIENT="mailrelay-otp-max@hydrogen.local"
OTP_PURPOSE=1
OTP_STATUS_CONSUMED=1
OTP_STATUS_MAX_ATTEMPTS=3

MAILVAL_DIR="${PROJECT_DIR}/extras/mailval"
MAILVAL_BIN="${MAILVAL_DIR}/build/mailval"
MAILVAL_CERT="${MAILVAL_DIR}/mailval.pem"
MAILVAL_KEY="${MAILVAL_DIR}/mailval.key"
BASELINE_SQLITE="${PROJECT_DIR}/tests/artifacts/database/sqlite/hydrodemo.sqlite"

trap mailrelay_api_cleanup EXIT

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Locate Hydrogen Binary"

HYDROGEN_BIN=''
HYDROGEN_BIN_BASE=''
# shellcheck disable=SC2310 # Continue even if binary lookup fails
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

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Validate Environment Variables"
env_vars_valid=true
for required_var in HYDROGEN_MAILADMIN_NAME HYDROGEN_MAILADMIN_PASS HYDROGEN_MAILADMIN_EMAIL \
                    HYDROGEN_DEMO_USER_NAME HYDROGEN_DEMO_USER_PASS HYDROGEN_DEMO_API_KEY; do
    if [[ -z "${!required_var:-}" ]]; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "ERROR: ${required_var} is not set"
        env_vars_valid=false
    fi
done
if [[ "${env_vars_valid}" = true ]]; then
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Required environment variables are set"
    PASS_COUNT=$(( PASS_COUNT + 1 ))
else
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Missing required environment variables"
    EXIT_CODE=1
fi

config_valid=true
print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Validate Configuration Files"
for display_name in "${!MAILRELAY_API_ENGINES[@]}"; do
    IFS=':' read -r engine_name web_plain mail_plain web_tls mail_tls <<< "${MAILRELAY_API_ENGINES[${display_name}]}"
    config_file="${SCRIPT_DIR}/configs/hydrogen_test_${TEST_NUMBER}_${engine_name}.json"
    # shellcheck disable=SC2310 # Continue even if validation fails
    if validate_config_file "${config_file}"; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${display_name} configuration will use web ${web_plain}/${web_tls}, mailval ${mail_plain}/${mail_tls}"
    else
        config_valid=false
        EXIT_CODE=1
    fi
done

if [[ "${config_valid}" = true ]]; then
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "All configuration files validated successfully"
    PASS_COUNT=$(( PASS_COUNT + 1 ))
else
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Configuration file validation failed"
    EXIT_CODE=1
fi

variant_count=$(( ${#MAILRELAY_API_ENGINES[@]} * 2 ))
TEST_NAME="MailRelay API  {BLUE}variants: ${variant_count}{RESET}"

GLOBAL_RESULT_FILE="${DIAG_TEST_DIR}/mailrelay_api_results.result"
true > "${GLOBAL_RESULT_FILE}"

if [[ "${EXIT_CODE}" -eq 0 ]]; then
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Running MailRelay API tests in parallel"

    for display_name in "${!MAILRELAY_API_ENGINES[@]}"; do
        # shellcheck disable=SC2312 # Job control with wc -l is standard practice
        while (( $(jobs -r | wc -l) >= MAX_ENGINE_JOBS )); do
            # shellcheck disable=SC2310 # Non-zero child exit must not abort the suite
            wait -n || true
        done

        IFS=':' read -r engine_name web_plain mail_plain web_tls mail_tls <<< "${MAILRELAY_API_ENGINES[${display_name}]}"
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Starting engine pair: ${display_name} (plain ${web_plain}/${mail_plain}, tls ${web_tls}/${mail_tls})"
        mailrelay_api_run_engine_pair "${display_name}" "${engine_name}" \
            "${web_plain}" "${mail_plain}" "${web_tls}" "${mail_tls}" &
        PARALLEL_PIDS+=($!)
    done

    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Waiting for ${#MAILRELAY_API_ENGINES[@]} engine pairs to complete"
    for pid in "${PARALLEL_PIDS[@]}"; do
        # shellcheck disable=SC2310 # Pass/fail is in result files; do not abort on child status
        wait "${pid}" || true
    done
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "All parallel tests completed"

    for display_name in "${!MAILRELAY_API_ENGINES[@]}"; do
        IFS=':' read -r engine_name web_plain mail_plain web_tls mail_tls <<< "${MAILRELAY_API_ENGINES[${display_name}]}"
        # shellcheck disable=SC2310 # Continue even if analysis fails
        if mailrelay_api_analyze "${display_name}-plaintext" "${engine_name}-plaintext" "${display_name}-plaintext Engine"; then
            PASS_COUNT=$(( PASS_COUNT + 1 ))
        else
            EXIT_CODE=1
        fi
        # shellcheck disable=SC2310 # Continue even if analysis fails
        if mailrelay_api_analyze "${display_name}-STARTTLS" "${engine_name}-starttls" "${display_name}-STARTTLS Engine"; then
            PASS_COUNT=$(( PASS_COUNT + 1 ))
        else
            EXIT_CODE=1
        fi
    done

    # shellcheck disable=SC2310 # Continue even if the OTP subtest fails
    if ! mailrelay_api_run_otp_launch "MailRelay OTP + Repo Probe Launch" \
            "${SCRIPT_DIR}/configs/hydrogen_test_${TEST_NUMBER}_sqlite.json" \
            "${OTP_WEB_PORT}" "${OTP_MAILVAL_PORT}" "${OTP_RECIPIENT}" "${OTP_MAX_RECIPIENT}"; then
        EXIT_CODE=1
    fi

    successful_engines=0
    if [[ -f "${GLOBAL_RESULT_FILE}" ]]; then
        successful_engines=$("${GREP}" -c "MAILRELAY_API_.*_PASS" "${GLOBAL_RESULT_FILE}" 2>/dev/null || echo "0")
    fi

    print_marker "${TEST_NUMBER}" "${TEST_COUNTER}"
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Summary: ${successful_engines}/${variant_count} engine/transport variants passed all checks"
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Parallel execution completed - MailRelay API validated across ${variant_count} engine/transport variants"
else
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Skipping MailRelay API tests due to prerequisite failures"
    EXIT_CODE=1
fi

print_test_completion "${TEST_NAME}" "${TEST_ABBR}" "${TEST_NUMBER}" "${TEST_VERSION}"

${ORCHESTRATION:-false} && return "${EXIT_CODE}" || exit "${EXIT_CODE}"
