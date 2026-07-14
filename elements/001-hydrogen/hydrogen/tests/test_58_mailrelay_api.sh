#!/usr/bin/env bash

# Test: MailRelay API - Multi-Engine Authenticated End-to-End
#
# Validates the Hydrogen Mail Relay REST API across all supported database engines.
# Each engine is exercised with both plaintext SMTP and STARTTLS SMTP delivery
# against the local C mail validator (extras/mailval). The test logs in as the
# mailadmin account, previews and sends the seeded mail.test template, and
# verifies the message was captured by the SMTP sink.
#
# Required environment variables:
#   HYDROGEN_MAILADMIN_NAME, HYDROGEN_MAILADMIN_PASS, HYDROGEN_MAILADMIN_EMAIL
#   HYDROGEN_DEMO_USER_NAME, HYDROGEN_DEMO_USER_PASS, HYDROGEN_DEMO_API_KEY
#   HYDROGEN_DEMO_JWT_KEY, PAYLOAD_KEY (via existing configs)

# FUNCTIONS
# api_request()
# extract_jwt()
# wait_for_http_ready()
# wait_for_ready_for_requests()
# start_mailval()
# stop_mailval()
# poll_mailval_capture()
# run_mailrelay_variant()
# run_engine_test()
# analyze_engine_results()

# CHANGELOG
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
TEST_VERSION="2.3.0"

# shellcheck source=tests/lib/framework.sh # Reference framework directly
[[ -n "${FRAMEWORK_GUARD:-}" ]] || source "$(dirname "${BASH_SOURCE[0]}")/lib/framework.sh"
setup_test_environment

# Parallel execution configuration
declare -a PARALLEL_PIDS
declare -A MAILRELAY_API_CONFIGS

# Engine configuration: result_suffix:engine_name:web_port:mailval_port:use_tls
# Each variant gets a unique result_suffix so parallel plaintext/STARTTLS runs
# for the same engine do not overwrite the same result file.
MAILRELAY_API_CONFIGS=(
    ["PostgreSQL-plaintext"]="postgres-plaintext:postgres:55800:55801:0"
    ["PostgreSQL-STARTTLS"]="postgres-starttls:postgres:55802:55803:1"
    ["MySQL-plaintext"]="mysql-plaintext:mysql:55804:55805:0"
    ["MySQL-STARTTLS"]="mysql-starttls:mysql:55806:55807:1"
    ["SQLite-plaintext"]="sqlite-plaintext:sqlite:55808:55809:0"
    ["SQLite-STARTTLS"]="sqlite-starttls:sqlite:55810:55811:1"
    ["DB2-plaintext"]="db2-plaintext:db2:55812:55813:0"
    ["DB2-STARTTLS"]="db2-starttls:db2:55814:55815:1"
    ["MariaDB-plaintext"]="mariadb-plaintext:mariadb:55816:55817:0"
    ["MariaDB-STARTTLS"]="mariadb-starttls:mariadb:55818:55819:1"
    ["CockroachDB-plaintext"]="cockroachdb-plaintext:cockroachdb:55820:55821:0"
    ["CockroachDB-STARTTLS"]="cockroachdb-starttls:cockroachdb:55822:55823:1"
    ["YugabyteDB-plaintext"]="yugabytedb-plaintext:yugabytedb:55824:55825:0"
    ["YugabyteDB-STARTTLS"]="yugabytedb-starttls:yugabytedb:55826:55827:1"
)

# Timeouts (seconds)
STARTUP_TIMEOUT=15
SHUTDOWN_TIMEOUT=30
SHUTDOWN_ACTIVITY_TIMEOUT=5
HTTP_READY_TIMEOUT=15
READY_TIMEOUT=30
MAILVAL_READY_TIMEOUT=5
CAPTURE_TIMEOUT=15

# OTP launch coverage (Seam A) — isolated SQLite variant on dedicated ports.
OTP_WEB_PORT=55830
OTP_MAILVAL_PORT=55831
OTP_RECIPIENT="mailrelay-otp-launch@hydrogen.local"
OTP_PURPOSE=1

# Mail validator paths
MAILVAL_DIR="${PROJECT_DIR}/extras/mailval"
MAILVAL_BIN="${MAILVAL_DIR}/build/mailval"
MAILVAL_CERT="${MAILVAL_DIR}/mailval.pem"
MAILVAL_KEY="${MAILVAL_DIR}/mailval.key"

# Baseline SQLite database for isolated copies
BASELINE_SQLITE="${PROJECT_DIR}/tests/artifacts/database/sqlite/hydrodemo.sqlite"

# Background PID collectors for the cleanup trap
MAILVAL_PIDS=()
HYDROGEN_PIDS=()

cleanup_background_processes() {
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
trap cleanup_background_processes EXIT

# Generic HTTP request helper. Prints HTTP status code to stdout.
api_request() {
    local method="$1"
    local url="$2"
    local data="$3"
    local output_file="$4"
    local jwt_token="${5:-}"
    local max_retries=3
    local retry=1

    while [[ "${retry}" -le "${max_retries}" ]]; do
        local curl_cmd=(curl -s -X "${method}" -H "Content-Type: application/json")
        if [[ -n "${jwt_token}" ]]; then
            curl_cmd+=(-H "Authorization: Bearer ${jwt_token}")
        fi
        if [[ "${method}" != "GET" ]]; then
            curl_cmd+=(-d "${data}")
        fi
        curl_cmd+=(-w "%{http_code}" -o "${output_file}" --connect-timeout 3 --max-time 5 "${url}")

        local http_status
        http_status=$("${curl_cmd[@]}" 2>/dev/null || true)
        http_status=${http_status:-"000"}

        # Retry on transient network/server errors
        if [[ "${http_status}" == "200" ]] || \
           [[ "${http_status}" == "201" ]] || \
           [[ "${http_status}" == "400" ]] || \
           [[ "${http_status}" == "401" ]] || \
           [[ "${http_status}" == "403" ]] || \
           [[ "${http_status}" == "404" ]] || \
           [[ "${http_status}" == "429" ]] || \
           [[ "${http_status}" == "500" ]] || \
           [[ "${http_status}" == "503" ]]; then
            echo "${http_status}"
            return 0
        fi

        if [[ "${retry}" -eq "${max_retries}" ]]; then
            echo "${http_status}"
            return 0
        fi

        sleep 0.5
        retry=$(( retry + 1 ))
    done

    echo "000"
}

# Extract the JWT token from a login response file.
extract_jwt() {
    local response_file="$1"
    if [[ -f "${response_file}" ]]; then
        jq -r '.token // empty' "${response_file}" 2>/dev/null || true
    fi
}

# Wait for the Hydrogen web server to respond to /api/version.
wait_for_http_ready() {
    local base_url="$1"
    local timeout="${2:-${HTTP_READY_TIMEOUT}}"
    local start_time
    start_time=${SECONDS}

    while [[ $((SECONDS - start_time)) -lt "${timeout}" ]]; do
        local http_code
        http_code=$(curl -s -w "%{http_code}" -o /dev/null --connect-timeout 1 --max-time 2 "${base_url}/api/version" 2>/dev/null || true)
        http_code=${http_code:-"000"}
        if [[ "${http_code}" == "200" ]]; then
            return 0
        fi
        sleep 0.1
    done

    return 1
}

# Wait for the canonical "READY FOR REQUESTS" signal.
wait_for_ready_for_requests() {
    local log_file="$1"
    local timeout="${2:-${READY_TIMEOUT}}"
    local start_time
    start_time=${SECONDS}

    while [[ $((SECONDS - start_time)) -lt "${timeout}" ]]; do
        if "${GREP}" -q "READY FOR REQUESTS" "${log_file}" 2>/dev/null; then
            return 0
        fi
        sleep 0.1
    done

    return 1
}

# Start the C mail validator on a dedicated port.
start_mailval() {
    local port="$1"
    local use_tls="$2"
    local maildata_dir="$3"
    local mailval_log="$4"

    mkdir -p "${maildata_dir}"
    true > "${mailval_log}"

    local mailval_args=( "--smtp-port" "${port}" "--data-dir" "${maildata_dir}" )
    if [[ "${use_tls}" -eq 1 ]]; then
        mailval_args+=( "--cert" "${MAILVAL_CERT}" "--key" "${MAILVAL_KEY}" )
    fi

    "${MAILVAL_BIN}" "${mailval_args[@]}" > "${mailval_log}" 2>&1 &
    local mailval_pid=$!
    MAILVAL_PIDS+=("${mailval_pid}")

    # Wait for the sink to accept connections.
    local ready=false
    local start_time
    start_time=${SECONDS}
    while [[ $((SECONDS - start_time)) -lt "${MAILVAL_READY_TIMEOUT}" ]]; do
        if "${TIMEOUT}" 1 bash -c "</dev/tcp/127.0.0.1/${port}" 2>/dev/null; then
            ready=true
            break
        fi
        sleep 0.05
    done

    if [[ "${ready}" = false ]]; then
        kill -INT "${mailval_pid}" 2>/dev/null || true
        return 1
    fi

    echo "${mailval_pid}"
    return 0
}

# Stop a mailval instance by PID.
stop_mailval() {
    local pid="$1"
    kill -INT "${pid}" 2>/dev/null || true
}

# Poll the mailval capture directory for a stored message matching a subject
# marker. Prints the capture file path to stdout.
poll_mailval_capture() {
    local maildata_dir="$1"
    local subject_marker="$2"
    local timeout="${3:-${CAPTURE_TIMEOUT}}"
    local start_time
    start_time=${SECONDS}

    while [[ $((SECONDS - start_time)) -lt "${timeout}" ]]; do
        local candidate
        for candidate in "${maildata_dir}"/mailval_smtp_*.json; do
            [[ -f "${candidate}" ]] || continue
            local stored
            local subject
            stored=$(jq -r '[.commands[]? | select(.dir=="meta" and .key=="stored_uid")] | .[0].value // empty' "${candidate}" 2>/dev/null || true)
            subject=$(jq -r '[.commands[]? | select(.dir=="meta" and .key=="subject")] | .[0].value // empty' "${candidate}" 2>/dev/null || true)
            if [[ "${stored}" == "yes" ]] && [[ "${subject}" == *"${subject_marker}"* ]]; then
                echo "${candidate}"
                return 0
            fi
        done
        sleep 0.1
    done

    return 1
}

# Run one plaintext or STARTTLS API subtest for a single engine.
run_mailrelay_variant() {
    local engine_name="$1"
    local description="$2"
    local config_file="$3"
    local web_port="$4"
    local mailval_port="$5"
    local use_tls="$6"
    local result_file="$7"
    local base_url="http://127.0.0.1:${web_port}"
    local variant_label
    local variant_tag
    local maildata_dir
    local mailval_log
    local hydrogen_log
    local temp_config=""
    local sqlite_temp_file=""
    local sqlite_temp_config=""
    local actual_config_file
    local jq_patch
    local mailval_pid
    local hydrogen_pid_var
    local hydrogen_pid
    local variant_failed=false
    local response_dir
    local status_response
    local preview_response
    local send_response
    local login_mailadmin_response
    local login_demo_response
    local noauth_response
    local http_status
    local login_data
    local mailadmin_token=""
    local status_success
    local preview_success
    local preview_subject
    local macros_used
    local idempotency_key
    local send_data
    local send_success
    local demo_token
    local demo_data
    local unauthorized_send_response
    local capture_file

    if [[ "${use_tls}" -eq 1 ]]; then
        variant_label="STARTTLS"
    else
        variant_label="plaintext"
    fi

    # Unique working directories for this variant
    variant_tag="${engine_name}_${variant_label}_${TIMESTAMP}"
    maildata_dir="${DIAG_TEST_DIR}/mailval_${variant_tag}"
    mailval_log="${LOGS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}_mailval_${engine_name}_${variant_label}.log"
    hydrogen_log="${LOGS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}_hydrogen_${engine_name}_${variant_label}.log"
    temp_config=""
    sqlite_temp_file=""
    sqlite_temp_config=""

    # Build the runtime config. For SQLite, use an isolated copy of the baseline DB.
    actual_config_file="${config_file}"
    if [[ "${engine_name}" == "sqlite" ]]; then
        sqlite_temp_file="${DIAG_TEST_DIR}/hydrodemo_${variant_tag}.sqlite"
        sqlite_temp_config="${DIAG_TEST_DIR}/hydrogen_test_${TEST_NUMBER}_sqlite_${variant_tag}.json"
        if ! cp "${BASELINE_SQLITE}" "${sqlite_temp_file}" 2>/dev/null; then
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}: Failed to copy baseline SQLite database"
            echo "VARIANT_${variant_label}_FAIL" >> "${result_file}"
            # Always return 0 so parallel set -e wait does not abort the suite.
            return 0
        fi
        # Copy WAL-mode sidecar files if present; without them the database may
        # appear empty because the baseline may have uncheckpointed WAL data.
        if [[ -f "${BASELINE_SQLITE}-wal" ]]; then
            cp "${BASELINE_SQLITE}-wal" "${sqlite_temp_file}-wal" 2>/dev/null || true
        fi
        if [[ -f "${BASELINE_SQLITE}-shm" ]]; then
            cp "${BASELINE_SQLITE}-shm" "${sqlite_temp_file}-shm" 2>/dev/null || true
        fi
        if ! jq --arg db "${sqlite_temp_file}" \
                '.Databases.Connections[0].Database = $db' \
                "${config_file}" > "${sqlite_temp_config}" 2>/dev/null; then
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}: Failed to patch SQLite config"
            echo "VARIANT_${variant_label}_FAIL" >> "${result_file}"
            return 0
        fi
        actual_config_file="${sqlite_temp_config}"
    fi

    temp_config="${DIAG_TEST_DIR}/hydrogen_test_${TEST_NUMBER}_${engine_name}_${variant_label}_${TIMESTAMP}.json"
    if [[ "${use_tls}" -eq 1 ]]; then
        export MAILRELAY_MAILVAL_CERT="${MAILVAL_CERT}"
        jq_patch=$(jq --arg web_port "${web_port}" --arg port "${mailval_port}" \
                      '.WebServer.Port = ($web_port | tonumber) |
                       .MailRelay.Servers[0].Port = $port |
                       .MailRelay.Servers[0].UseTLS = true |
                       .MailRelay.Servers[0].TLSMode = 1 |
                       .MailRelay.Servers[0].CAPath = "${env.MAILRELAY_MAILVAL_CERT}"' \
                      "${actual_config_file}" 2>/dev/null) || true
    else
        jq_patch=$(jq --arg web_port "${web_port}" --arg port "${mailval_port}" \
                      '.WebServer.Port = ($web_port | tonumber) |
                       .MailRelay.Servers[0].Port = $port |
                       .MailRelay.Servers[0].UseTLS = false |
                       .MailRelay.Servers[0].TLSMode = 0 |
                       .MailRelay.Servers[0].CAPath = ""' \
                      "${actual_config_file}" 2>/dev/null) || true
    fi

    if [[ -z "${jq_patch}" ]]; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}: Failed to patch MailRelay server config"
        echo "VARIANT_${variant_label}_FAIL" >> "${result_file}"
        return 0
    fi
    echo "${jq_patch}" > "${temp_config}"

    # Generate mailval cert/key if needed for STARTTLS.
    if [[ "${use_tls}" -eq 1 ]]; then
        if [[ ! -f "${MAILVAL_CERT}" || ! -f "${MAILVAL_KEY}" ]]; then
            bash "${MAILVAL_DIR}/gen_cert.sh" >/dev/null 2>&1 || true
        fi
    fi

    # Start the SMTP sink.
    # shellcheck disable=SC2310 # Continue even if mailval startup fails
    mailval_pid=$(start_mailval "${mailval_port}" "${use_tls}" "${maildata_dir}" "${mailval_log}") || {
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}: mailval failed to start on port ${mailval_port}"
        echo "VARIANT_${variant_label}_FAIL" >> "${result_file}"
        rm -f "${temp_config}" "${sqlite_temp_file}" "${sqlite_temp_config}" "${sqlite_temp_file}-wal" "${sqlite_temp_file}-shm" 2>/dev/null || true
        return 0
    }

    # Start Hydrogen and wait for startup.
    hydrogen_pid_var="MAILRELAY_HYDROGEN_PID_${engine_name}_${variant_label}"
    hydrogen_pid=""
    eval "${hydrogen_pid_var}=''"
    # shellcheck disable=SC2310 # Continue even if startup fails
    if ! start_hydrogen_with_pid "${temp_config}" "${hydrogen_log}" "${STARTUP_TIMEOUT}" "${HYDROGEN_BIN}" "${hydrogen_pid_var}"; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}: Hydrogen failed to start"
        stop_mailval "${mailval_pid}"
        echo "VARIANT_${variant_label}_FAIL" >> "${result_file}"
        rm -f "${temp_config}" "${sqlite_temp_file}" "${sqlite_temp_config}" "${sqlite_temp_file}-wal" "${sqlite_temp_file}-shm" 2>/dev/null || true
        return 0
    fi
    hydrogen_pid=$(eval "echo \${${hydrogen_pid_var}}")
    if [[ -z "${hydrogen_pid}" ]]; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}: Hydrogen PID was not captured"
        stop_mailval "${mailval_pid}"
        echo "VARIANT_${variant_label}_FAIL" >> "${result_file}"
        rm -f "${temp_config}" "${sqlite_temp_file}" "${sqlite_temp_config}" "${sqlite_temp_file}-wal" "${sqlite_temp_file}-shm" 2>/dev/null || true
        return 0
    fi
    HYDROGEN_PIDS+=("${hydrogen_pid}")

    # Wait for HTTP readiness and the canonical READY FOR REQUESTS signal.
    variant_failed=false
    # shellcheck disable=SC2310 # Continue even if HTTP readiness check fails
    if ! wait_for_http_ready "${base_url}" "${HTTP_READY_TIMEOUT}"; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}: HTTP readiness check failed"
        variant_failed=true
    fi

    if [[ "${variant_failed}" = false ]]; then
        # shellcheck disable=SC2310 # Continue even if readiness check fails
        if ! wait_for_ready_for_requests "${hydrogen_log}" "${READY_TIMEOUT}"; then
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}: READY FOR REQUESTS signal not observed"
            variant_failed=true
        fi
    fi

    # Response files.
    response_dir="${DIAG_TEST_DIR}/responses_${variant_tag}"
    mkdir -p "${response_dir}"
    status_response="${response_dir}/status.json"
    preview_response="${response_dir}/preview.json"
    send_response="${response_dir}/send.json"
    login_mailadmin_response="${response_dir}/login_mailadmin.json"
    login_demo_response="${response_dir}/login_demo.json"
    noauth_response="${response_dir}/noauth.json"

    # Negative: status without JWT must fail.
    if [[ "${variant_failed}" = false ]]; then
        http_status=$(api_request "GET" "${base_url}/api/mailrelay/status" "" "${noauth_response}")
        if [[ "${http_status}" != "401" ]]; then
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}: Expected 401 for unauthenticated status, got ${http_status}"
            variant_failed=true
        fi
    fi

    # Login as mailadmin.
    mailadmin_token=""
    if [[ "${variant_failed}" = false ]]; then
        login_data=$(jq -n \
            --arg login_id "${HYDROGEN_MAILADMIN_NAME}" \
            --arg password "${HYDROGEN_MAILADMIN_PASS}" \
            --arg api_key "${HYDROGEN_DEMO_API_KEY}" \
            '{database: "Acuranzo", login_id: $login_id, password: $password, api_key: $api_key, tz: "America/Vancouver"}')
        http_status=$(api_request "POST" "${base_url}/api/auth/login" "${login_data}" "${login_mailadmin_response}")
        if [[ "${http_status}" != "200" ]]; then
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}: mailadmin login returned HTTP ${http_status}"
            variant_failed=true
        else
            mailadmin_token=$(extract_jwt "${login_mailadmin_response}")
            if [[ -z "${mailadmin_token}" ]]; then
                print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}: mailadmin login returned no JWT token"
                variant_failed=true
            fi
        fi
    fi

    # Positive: status with valid JWT.
    if [[ "${variant_failed}" = false ]] && [[ -n "${mailadmin_token}" ]]; then
        http_status=$(api_request "GET" "${base_url}/api/mailrelay/status" "" "${status_response}" "${mailadmin_token}")
        if [[ "${http_status}" != "200" ]]; then
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}: status endpoint returned HTTP ${http_status}"
            variant_failed=true
        else
            status_success=$(jq -r '.success // false' "${status_response}" 2>/dev/null || echo "false")
            if [[ "${status_success}" != "true" ]]; then
                print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}: status endpoint returned success=${status_success}"
                variant_failed=true
            fi
        fi
    fi

    # Positive: preview with valid JWT + mail_send role.
    if [[ "${variant_failed}" = false ]] && [[ -n "${mailadmin_token}" ]]; then
        local preview_data
        preview_data=$(jq -n \
            --arg name "MailRelayBlackbox" \
            '{template_key: "mail.test", params: {NAME: $name}}')
        http_status=$(api_request "POST" "${base_url}/api/mailrelay/preview" "${preview_data}" "${preview_response}" "${mailadmin_token}")
        if [[ "${http_status}" != "200" ]]; then
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}: preview endpoint returned HTTP ${http_status}"
            variant_failed=true
        else
            preview_success=$(jq -r '.success // false' "${preview_response}" 2>/dev/null || echo "false")
            if [[ "${preview_success}" != "true" ]]; then
                print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}: preview endpoint returned success=${preview_success}"
                variant_failed=true
            else
                preview_subject=$(jq -r '.subject // empty' "${preview_response}" 2>/dev/null || true)
                if [[ "${preview_subject}" != *"MailRelayBlackbox"* ]]; then
                    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}: preview subject missing marker ('${preview_subject}')"
                    variant_failed=true
                fi
                macros_used=$(jq -r '.macros_used // [] | map(. == "NAME") | any' "${preview_response}" 2>/dev/null || echo "false")
                if [[ "${macros_used}" != "true" ]]; then
                    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}: preview macros_used missing NAME"
                    variant_failed=true
                fi
            fi
        fi
    fi

    # Positive: send with valid JWT + mail_send role.
    idempotency_key="mailrelay-api-${engine_name}-${variant_label}-${TIMESTAMP}-${RANDOM}"
    if [[ "${variant_failed}" = false ]] && [[ -n "${mailadmin_token}" ]]; then
        send_data=$(jq -n \
            --arg template_key "mail.test" \
            --arg idempotency_key "${idempotency_key}" \
            --arg name "MailRelayBlackbox" \
            '{template_key: $template_key, to: ["sink@mailval.local"], idempotency_key: $idempotency_key, params: {NAME: $name}}')
        http_status=$(api_request "POST" "${base_url}/api/mailrelay/send" "${send_data}" "${send_response}" "${mailadmin_token}")
        if [[ "${http_status}" != "200" ]]; then
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}: send endpoint returned HTTP ${http_status}"
            variant_failed=true
        else
            send_success=$(jq -r '.success // false' "${send_response}" 2>/dev/null || echo "false")
            if [[ "${send_success}" != "true" ]]; then
                print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}: send endpoint returned success=${send_success}"
                variant_failed=true
            fi
        fi
    fi

    # Negative: send as demo user without mail_send role.
    if [[ "${variant_failed}" = false ]]; then
        demo_token=""
        demo_data=$(jq -n \
            --arg login_id "${HYDROGEN_DEMO_USER_NAME}" \
            --arg password "${HYDROGEN_DEMO_USER_PASS}" \
            --arg api_key "${HYDROGEN_DEMO_API_KEY}" \
            '{database: "Acuranzo", login_id: $login_id, password: $password, api_key: $api_key, tz: "America/Vancouver"}')
        http_status=$(api_request "POST" "${base_url}/api/auth/login" "${demo_data}" "${login_demo_response}")
        if [[ "${http_status}" != "200" ]]; then
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}: demo user login returned HTTP ${http_status}"
            variant_failed=true
        else
            demo_token=$(extract_jwt "${login_demo_response}")
            if [[ -z "${demo_token}" ]]; then
                print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}: demo user login returned no JWT token"
                variant_failed=true
            else
                unauthorized_send_response="${response_dir}/send_unauthorized.json"
                http_status=$(api_request "POST" "${base_url}/api/mailrelay/send" "${send_data}" "${unauthorized_send_response}" "${demo_token}")
                if [[ "${http_status}" != "401" ]]; then
                    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}: expected 401 for unauthorized send, got ${http_status}"
                    variant_failed=true
                fi
            fi
        fi
    fi

    # Stop Hydrogen cleanly.
    # shellcheck disable=SC2310 # Continue even if shutdown fails
    stop_hydrogen "${hydrogen_pid}" "${hydrogen_log}" "${SHUTDOWN_TIMEOUT}" "${SHUTDOWN_ACTIVITY_TIMEOUT}" "${DIAG_TEST_DIR}"

    # Poll the sink for the delivered message.
    if [[ "${variant_failed}" = false ]]; then
        # shellcheck disable=SC2310 # Capture failures are handled below
        capture_file=$(poll_mailval_capture "${maildata_dir}" "MailRelayBlackbox" "${CAPTURE_TIMEOUT}") || true
        if [[ -z "${capture_file}" ]]; then
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}: SMTP sink did not capture message with subject marker"
            variant_failed=true
        fi
    fi

    # Stop the sink and clean up temp files.
    stop_mailval "${mailval_pid}"
    rm -f "${temp_config}" "${sqlite_temp_file}" "${sqlite_temp_config}" "${sqlite_temp_file}-wal" "${sqlite_temp_file}-shm" 2>/dev/null || true

    if [[ "${variant_failed}" = false ]]; then
        echo "VARIANT_${variant_label}_PASS" >> "${result_file}"
    else
        echo "VARIANT_${variant_label}_FAIL" >> "${result_file}"
    fi
    return 0
}

# Run one plaintext or STARTTLS subtest for a single database engine.
run_engine_test() {
    local test_name="$1"
    local config_file="$2"
    local result_suffix="$3"
    local engine_name="$4"
    local description="$5"
    local web_port="$6"
    local mailval_port="$7"
    local use_tls="$8"
    local result_file="${LOG_PREFIX}_${result_suffix}.result"
    local variant_label

    if [[ "${use_tls}" -eq 1 ]]; then
        variant_label="STARTTLS"
    else
        variant_label="plaintext"
    fi

    true > "${result_file}"

    # shellcheck disable=SC2310 # Continue even if variant fails
    run_mailrelay_variant "${engine_name}" "${description}" "${config_file}" \
        "${web_port}" "${mailval_port}" "${use_tls}" "${result_file}"

    # Always return 0: pass/fail is encoded in the result file so parallel
    # set -e wait does not abort the parent before analyze_engine_results runs.
    if "${GREP}" -q "VARIANT_${variant_label}_PASS" "${result_file}" 2>/dev/null; then
        echo "ENGINE_TEST_COMPLETE" >> "${result_file}"
    else
        if ! "${GREP}" -q "VARIANT_${variant_label}_FAIL" "${result_file}" 2>/dev/null; then
            echo "VARIANT_${variant_label}_FAIL" >> "${result_file}"
        fi
        echo "ENGINE_TEST_FAILED" >> "${result_file}"
    fi
    return 0
}

# Analyze results for one engine variant and print subtest output.
analyze_engine_results() {
    local test_name="$1"
    local result_suffix="$2"
    local engine_name="$3"
    local description="$4"
    local result_file="${LOG_PREFIX}_${result_suffix}.result"

    if [[ ! -f "${result_file}" ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${description}: No result file found"
        return 1
    fi

    print_marker "${TEST_NUMBER}" "${TEST_COUNTER}"
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}: Analyzing results"

    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}: API End-to-End"
    if "${GREP}" -q "ENGINE_TEST_COMPLETE" "${result_file}" 2>/dev/null; then
        echo "MAILRELAY_API_${test_name}_PASS" >> "${GLOBAL_RESULT_FILE:-${DIAG_TEST_DIR}/mailrelay_api_results.result}"
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "${description}: API test passed"
        return 0
    fi

    # Prefer an explicit FAIL marker; fall back to missing COMPLETE.
    if "${GREP}" -q "ENGINE_TEST_FAILED" "${result_file}" 2>/dev/null \
        || "${GREP}" -q "VARIANT_.*_FAIL" "${result_file}" 2>/dev/null; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${description}: API test failed"
    else
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${description}: API test failed (incomplete result file)"
    fi
    return 1
}

# Run the launch-time OTP send + self-verify coverage subtest (Seam A).
# Starts Hydrogen with MailRelay.Test.SendOtpOnLaunch enabled, which fires a deterministic OTP send through the real templated worker path during launch
# and then verifies it. Asserts the MAILRELAY_OTP_LAUNCH_SENT and MAILRELAY_OTP_LAUNCH_VERIFIED log markers and that the OTP row was consumed
# in the database (mail_otp_codes.status_a67 = 1). Uses an isolated SQLite copy so the baseline database is not mutated.
run_mailrelay_otp_launch() {
    local label="$1"
    local config_file="$2"
    local web_port="$3"
    local mailval_port="$4"
    local recipient="$5"

    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "${label}"

    local variant_tag="otp_launch_${TIMESTAMP}"
    local maildata_dir="${DIAG_TEST_DIR}/mailval_${variant_tag}"
    local mailval_log="${LOGS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}_mailval_otp.log"
    local hydrogen_log="${LOGS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}_hydrogen_otp.log"
    true > "${mailval_log}"

    mkdir -p "${maildata_dir}"

    # Isolated SQLite DB copy so we do not mutate the baseline.
    local sqlite_temp_file="${DIAG_TEST_DIR}/hydrodemo_${variant_tag}.sqlite"
    local sqlite_temp_config="${DIAG_TEST_DIR}/hydrogen_test_${TEST_NUMBER}_otp_${TIMESTAMP}.json"
    if ! cp "${BASELINE_SQLITE}" "${sqlite_temp_file}" 2>/dev/null; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${label}: Failed to copy baseline SQLite database"
        return 1
    fi
    if [[ -f "${BASELINE_SQLITE}-wal" ]]; then
        cp "${BASELINE_SQLITE}-wal" "${sqlite_temp_file}-wal" 2>/dev/null || true
    fi
    if [[ -f "${BASELINE_SQLITE}-shm" ]]; then
        cp "${BASELINE_SQLITE}-shm" "${sqlite_temp_file}-shm" 2>/dev/null || true
    fi

    # Patch config: web port, mailval port, isolated DB, SendOtpOnLaunch.
    local jq_patch
    jq_patch=$(jq --arg web_port "${web_port}" \
                   --arg port "${mailval_port}" \
                   --arg db "${sqlite_temp_file}" \
                   '.WebServer.Port = ($web_port | tonumber) |
                    .MailRelay.Servers[0].Port = $port |
                    .MailRelay.Servers[0].UseTLS = false |
                    .MailRelay.Servers[0].TLSMode = 0 |
                    .MailRelay.Servers[0].CAPath = "" |
                    .Databases.Connections[0].Database = $db |
                    .MailRelay.Test.SendOtpOnLaunch = true' \
                   "${config_file}" 2>/dev/null) || true
    if [[ -z "${jq_patch}" ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${label}: Failed to patch OTP launch config"
        rm -f "${sqlite_temp_config}" "${sqlite_temp_file}" "${sqlite_temp_file}-wal" "${sqlite_temp_file}-shm" 2>/dev/null || true
        return 1
    fi
    echo "${jq_patch}" > "${sqlite_temp_config}"

    # Start the SMTP sink so the OTP email send has a destination.
    local mailval_pid
    # shellcheck disable=SC2310 # Continue even if mailval startup fails
    mailval_pid=$(start_mailval "${mailval_port}" 0 "${maildata_dir}" "${mailval_log}") || {
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${label}: mailval failed to start on port ${mailval_port}"
        rm -f "${sqlite_temp_config}" "${sqlite_temp_file}" "${sqlite_temp_file}-wal" "${sqlite_temp_file}-shm" 2>/dev/null || true
        return 1
    }

    # Start Hydrogen; the OTP seam runs during launch (before READY FOR REQUESTS).
    local hydrogen_pid_var="MAILRELAY_HYDROGEN_PID_otp"
    local hydrogen_pid=""
    eval "${hydrogen_pid_var}=''"
    # shellcheck disable=SC2310 # Continue even if startup fails
    if ! start_hydrogen_with_pid "${sqlite_temp_config}" "${hydrogen_log}" "${STARTUP_TIMEOUT}" "${HYDROGEN_BIN}" "${hydrogen_pid_var}"; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${label}: Hydrogen failed to start"
        stop_mailval "${mailval_pid}"
        rm -f "${sqlite_temp_config}" "${sqlite_temp_file}" "${sqlite_temp_file}-wal" "${sqlite_temp_file}-shm" 2>/dev/null || true
        return 1
    fi
    hydrogen_pid=$(eval "echo \${${hydrogen_pid_var}}")
    if [[ -z "${hydrogen_pid}" ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${label}: Hydrogen PID was not captured"
        stop_mailval "${mailval_pid}"
        rm -f "${sqlite_temp_config}" "${sqlite_temp_file}" "${sqlite_temp_file}-wal" "${sqlite_temp_file}-shm" 2>/dev/null || true
        return 1
    fi
    HYDROGEN_PIDS+=("${hydrogen_pid}")

    local failed=false

    # Launch must complete (READY FOR REQUESTS implies the OTP seam ran).
    # shellcheck disable=SC2310 # Continue even if readiness check fails
    if ! wait_for_ready_for_requests "${hydrogen_log}" "${READY_TIMEOUT}"; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${label}: READY FOR REQUESTS signal not observed"
        failed=true
    fi

    if [[ "${failed}" = false ]]; then
        if ! "${GREP}" -q "MAILRELAY_OTP_LAUNCH_SENT" "${hydrogen_log}" 2>/dev/null; then
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${label}: log missing MAILRELAY_OTP_LAUNCH_SENT"
            failed=true
        fi
        if ! "${GREP}" -q "MAILRELAY_OTP_LAUNCH_VERIFIED" "${hydrogen_log}" 2>/dev/null; then
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${label}: log missing MAILRELAY_OTP_LAUNCH_VERIFIED"
            failed=true
        fi
    fi

    # DB consumed check: the launched OTP row must be marked consumed (status_a67 = 1).
    if [[ "${failed}" = false ]] && command -v sqlite3 >/dev/null 2>&1; then
        local consumed
        consumed=$(sqlite3 "${sqlite_temp_file}" "SELECT COUNT(*) FROM mail_otp_codes WHERE email='${recipient}' AND purpose_a66=${OTP_PURPOSE} AND status_a67=1;" 2>/dev/null || echo "0")
        if [[ -z "${consumed}" ]] || [[ "${consumed}" = "0" ]]; then
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${label}: OTP row not consumed in DB (count=${consumed})"
            failed=true
        fi
    elif [[ "${failed}" = false ]]; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${label}: sqlite3 unavailable; relying on MAILRELAY_OTP_LAUNCH_VERIFIED marker for consume assertion"
    fi

    # Stop Hydrogen and the sink.
    # shellcheck disable=SC2310 # Continue even if shutdown fails
    stop_hydrogen "${hydrogen_pid}" "${hydrogen_log}" "${SHUTDOWN_TIMEOUT}" "${SHUTDOWN_ACTIVITY_TIMEOUT}" "${DIAG_TEST_DIR}"
    stop_mailval "${mailval_pid}"
    rm -f "${sqlite_temp_config}" "${sqlite_temp_file}" "${sqlite_temp_file}-wal" "${sqlite_temp_file}-shm" 2>/dev/null || true

    if [[ "${failed}" = false ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "${label}: OTP sent + self-verified (SENT + VERIFIED markers, DB row consumed)"
        PASS_COUNT=$(( PASS_COUNT + 1 ))
        return 0
    fi
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${label}: OTP launch coverage failed"
    return 1
}

# Main test flow

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

# Validate required environment variables.
print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Validate Environment Variables"
env_vars_valid=true
if [[ -z "${HYDROGEN_MAILADMIN_NAME:-}" ]]; then
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "ERROR: HYDROGEN_MAILADMIN_NAME is not set"
    env_vars_valid=false
fi
if [[ -z "${HYDROGEN_MAILADMIN_PASS:-}" ]]; then
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "ERROR: HYDROGEN_MAILADMIN_PASS is not set"
    env_vars_valid=false
fi
if [[ -z "${HYDROGEN_MAILADMIN_EMAIL:-}" ]]; then
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "ERROR: HYDROGEN_MAILADMIN_EMAIL is not set"
    env_vars_valid=false
fi
if [[ -z "${HYDROGEN_DEMO_USER_NAME:-}" ]]; then
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "ERROR: HYDROGEN_DEMO_USER_NAME is not set"
    env_vars_valid=false
fi
if [[ -z "${HYDROGEN_DEMO_USER_PASS:-}" ]]; then
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "ERROR: HYDROGEN_DEMO_USER_PASS is not set"
    env_vars_valid=false
fi
if [[ -z "${HYDROGEN_DEMO_API_KEY:-}" ]]; then
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "ERROR: HYDROGEN_DEMO_API_KEY is not set"
    env_vars_valid=false
fi

if [[ "${env_vars_valid}" = true ]]; then
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Required environment variables are set"
    PASS_COUNT=$(( PASS_COUNT + 1 ))
else
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Missing required environment variables"
    EXIT_CODE=1
fi

# Validate all configuration files.
config_valid=true
for test_config in "${!MAILRELAY_API_CONFIGS[@]}"; do
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Validate Configuration File: ${test_config}"

    IFS=':' read -r result_suffix engine_name web_port mailval_port use_tls <<< "${MAILRELAY_API_CONFIGS[${test_config}]}"
    config_file="${SCRIPT_DIR}/configs/hydrogen_test_${TEST_NUMBER}_${engine_name}.json"

    # shellcheck disable=SC2310 # Continue even if validation fails
    if validate_config_file "${config_file}"; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${test_config} configuration will use web port: ${web_port}, mailval port: ${mailval_port}"
    else
        config_valid=false
        EXIT_CODE=1
    fi
done

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Validate Configuration Files"
if [[ "${config_valid}" = true ]]; then
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "All configuration files validated successfully"
    PASS_COUNT=$(( PASS_COUNT + 1 ))
else
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Configuration file validation failed"
    EXIT_CODE=1
fi

TEST_NAME="MailRelay API  {BLUE}variants: ${#MAILRELAY_API_CONFIGS[@]}{RESET}"

# Global result aggregator file
GLOBAL_RESULT_FILE="${DIAG_TEST_DIR}/mailrelay_api_results.result"
true > "${GLOBAL_RESULT_FILE}"

# Only proceed if prerequisites are met
if [[ "${EXIT_CODE}" -eq 0 ]]; then
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Running MailRelay API tests in parallel"

    # Start all engine tests in parallel with job limiting
    for test_config in "${!MAILRELAY_API_CONFIGS[@]}"; do
        # shellcheck disable=SC2312 # Job control with wc -l is standard practice
        while (( $(jobs -r | wc -l) >= CORES )); do
            # shellcheck disable=SC2310 # Non-zero child exit must not abort the suite
            wait -n || true
        done

        IFS=':' read -r result_suffix engine_name web_port mailval_port use_tls <<< "${MAILRELAY_API_CONFIGS[${test_config}]}"
        config_file="${SCRIPT_DIR}/configs/hydrogen_test_${TEST_NUMBER}_${engine_name}.json"

        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Starting parallel test: ${test_config} (web=${web_port}, mailval=${mailval_port}, tls=${use_tls})"

        run_engine_test "${test_config}" "${config_file}" "${result_suffix}" \
            "${engine_name}" "${test_config} Engine" "${web_port}" "${mailval_port}" "${use_tls}" &
        PARALLEL_PIDS+=($!)
    done

    # Wait for all parallel tests to complete
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Waiting for ${#MAILRELAY_API_CONFIGS[@]} parallel MailRelay API tests to complete"
    for pid in "${PARALLEL_PIDS[@]}"; do
        # shellcheck disable=SC2310 # Pass/fail is in result files; do not abort on child status
        wait "${pid}" || true
    done
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "All parallel tests completed"

    # Process results sequentially for clean output
    for test_config in "${!MAILRELAY_API_CONFIGS[@]}"; do
        IFS=':' read -r result_suffix engine_name web_port mailval_port use_tls <<< "${MAILRELAY_API_CONFIGS[${test_config}]}"

        # shellcheck disable=SC2310 # Continue even if analysis fails
        if analyze_engine_results "${test_config}" "${result_suffix}" "${engine_name}" "${test_config} Engine"; then
            PASS_COUNT=$(( PASS_COUNT + 1 ))
        else
            EXIT_CODE=1
        fi
    done

    # OTP launch coverage subtest (Seam A) — isolated SQLite variant.
    # shellcheck disable=SC2310 # Continue even if the OTP subtest fails
    if ! run_mailrelay_otp_launch "MailRelay OTP Launch (send + self-verify)" \
            "${SCRIPT_DIR}/configs/hydrogen_test_${TEST_NUMBER}_sqlite.json" \
            "${OTP_WEB_PORT}" "${OTP_MAILVAL_PORT}" "${OTP_RECIPIENT}"; then
        EXIT_CODE=1
    fi

    # Print summary
    successful_engines=0
    if [[ -f "${GLOBAL_RESULT_FILE}" ]]; then
        successful_engines=$("${GREP}" -c "MAILRELAY_API_.*_PASS" "${GLOBAL_RESULT_FILE}" 2>/dev/null || echo "0")
    fi

    print_marker "${TEST_NUMBER}" "${TEST_COUNTER}"
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Summary: ${successful_engines}/${#MAILRELAY_API_CONFIGS[@]} engine/transport variants passed all checks"
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Parallel execution completed - MailRelay API validated across ${#MAILRELAY_API_CONFIGS[@]} engine/transport variants"
else
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Skipping MailRelay API tests due to prerequisite failures"
    EXIT_CODE=1
fi

# Print test completion summary
print_test_completion "${TEST_NAME}" "${TEST_ABBR}" "${TEST_NUMBER}" "${TEST_VERSION}"

# Return status code if sourced, exit if run standalone
${ORCHESTRATION:-false} && return "${EXIT_CODE}" || exit "${EXIT_CODE}"
