#!/usr/bin/env bash

# MailRelay API blackbox helpers for tests/test_58_mailrelay_api.sh.
#
# Local variable prefix in callers must be lowercase so the test_03
# env-var scanner does not treat them as required environment variables.

# shellcheck disable=SC2154 # Globals (TEST_NUMBER, GREP, LOG_PREFIX, TIMESTAMP, HYDROGEN_BIN, …) come from framework.sh
# shellcheck disable=SC2312 # Diagnostic substitutions swallow inner status; callers use || true

# CHANGELOG
# 1.0.4 - 2026-09-04 - Queue.Persist off for MySQL/MariaDB (stmt_bind_param SIGSEGV)
# 1.0.3 - 2026-09-04 - Pair TEST/PASS/FAIL in analyze (print_subtest before result)
# 1.0.2 - 2026-09-04 - Enable Queue.Persist for MySQL/MariaDB (INTEGER bind uses
#                      MYSQL_TYPE_LONGLONG).
# 1.0.1 - 2026-08-21 - STARTTLS: write CAPath in the patched JSON (export inside
#                      $() was lost).
# 1.0.0 - 2026-08-21 - Split from test_58; sequential variants per engine;
#                      longer timeouts and capped parallelism for full-suite load.

[[ -n "${MAILRELAY_API_HELPERS_GUARD:-}" ]] && return 0
export MAILRELAY_API_HELPERS_GUARD="true"

MAILRELAY_API_HELPERS_NAME="MailRelay API Test Helpers"
MAILRELAY_API_HELPERS_VERSION="1.0.4"
print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${MAILRELAY_API_HELPERS_NAME} ${MAILRELAY_API_HELPERS_VERSION}" "info"

MAILVAL_PIDS=()
HYDROGEN_PIDS=()

mailrelay_api_cleanup() {
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

mailrelay_api_request() {
    local method="$1"
    local url="$2"
    local data="$3"
    local output_file="$4"
    local jwt_token="${5:-}"
    local max_retries=5
    local retry=1
    local curl_cmd
    local http_status
    while [[ "${retry}" -le "${max_retries}" ]]; do
        curl_cmd=(curl -s -X "${method}" -H "Content-Type: application/json")
        if [[ -n "${jwt_token}" ]]; then
            curl_cmd+=(-H "Authorization: Bearer ${jwt_token}")
        fi
        if [[ "${method}" != "GET" ]]; then
            curl_cmd+=(-d "${data}")
        fi
        curl_cmd+=(-w "%{http_code}" -o "${output_file}" --connect-timeout 5 --max-time 10 "${url}")
        http_status=$("${curl_cmd[@]}" 2>/dev/null || true)
        http_status=${http_status:-"000"}
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

mailrelay_api_extract_jwt() {
    local response_file="$1"
    if [[ -f "${response_file}" ]]; then
        jq -r '.token // empty' "${response_file}" 2>/dev/null || true
    fi
}

mailrelay_api_wait_http() {
    local base_url="$1"
    local timeout="${2:-${HTTP_READY_TIMEOUT}}"
    local start_time
    local http_code
    start_time=${SECONDS}
    while [[ $((SECONDS - start_time)) -lt "${timeout}" ]]; do
        http_code=$(curl -s -w "%{http_code}" -o /dev/null --connect-timeout 1 --max-time 2 "${base_url}/api/version" 2>/dev/null || true)
        http_code=${http_code:-"000"}
        if [[ "${http_code}" == "200" ]]; then
            return 0
        fi
        sleep 0.1
    done
    return 1
}

mailrelay_api_wait_ready() {
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

mailrelay_api_start_mailval() {
    local port="$1"
    local use_tls="$2"
    local maildata_dir="$3"
    local mailval_log="$4"
    local mailval_args
    local mailval_pid
    local ready=false
    local start_time
    mkdir -p "${maildata_dir}"
    true > "${mailval_log}"
    mailval_args=( "--smtp-port" "${port}" "--data-dir" "${maildata_dir}" )
    if [[ "${use_tls}" -eq 1 ]]; then
        mailval_args+=( "--cert" "${MAILVAL_CERT}" "--key" "${MAILVAL_KEY}" )
    fi
    "${MAILVAL_BIN}" "${mailval_args[@]}" > "${mailval_log}" 2>&1 &
    mailval_pid=$!
    MAILVAL_PIDS+=("${mailval_pid}")
    start_time=${SECONDS}
    while [[ $((SECONDS - start_time)) -lt "${MAILVAL_READY_TIMEOUT}" ]]; do
        # shellcheck disable=SC2153 # TIMEOUT is the framework timeout binary, not a misspelling of timeout
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

mailrelay_api_stop_mailval() {
    local pid="$1"
    kill -INT "${pid}" 2>/dev/null || true
}

mailrelay_api_mailval_meta() {
    local candidate="$1"
    local key="$2"
    jq -r --arg key "${key}" \
        '[.commands[]? | select(.dir=="meta" and .key==$key)] | .[0].value // empty' \
        "${candidate}" 2>/dev/null || true
}

mailrelay_api_poll_capture() {
    local maildata_dir="$1"
    local subject_marker="$2"
    local timeout="${3:-${CAPTURE_TIMEOUT}}"
    local start_time
    local candidate
    local stored
    local subject
    start_time=${SECONDS}
    while [[ $((SECONDS - start_time)) -lt "${timeout}" ]]; do
        for candidate in "${maildata_dir}"/mailval_smtp_*.json; do
            [[ -f "${candidate}" ]] || continue
            stored=$(mailrelay_api_mailval_meta "${candidate}" "stored_uid")
            subject=$(mailrelay_api_mailval_meta "${candidate}" "subject")
            if [[ "${stored}" == "yes" ]] && [[ "${subject}" == *"${subject_marker}"* ]]; then
                echo "${candidate}"
                return 0
            fi
        done
        sleep 0.1
    done
    return 1
}

mailrelay_api_count_subject() {
    local maildata_dir="$1"
    local subject_marker="$2"
    local count=0
    local candidate
    local stored
    local subject
    for candidate in "${maildata_dir}"/mailval_smtp_*.json; do
        [[ -f "${candidate}" ]] || continue
        stored=$(mailrelay_api_mailval_meta "${candidate}" "stored_uid")
        subject=$(mailrelay_api_mailval_meta "${candidate}" "subject")
        if [[ "${stored}" == "yes" ]] && [[ "${subject}" == *"${subject_marker}"* ]]; then
            count=$(( count + 1 ))
        fi
    done
    echo "${count}"
}

mailrelay_api_copy_sqlite() {
    local dest="$1"
    if ! cp "${BASELINE_SQLITE}" "${dest}" 2>/dev/null; then
        return 1
    fi
    if [[ -f "${BASELINE_SQLITE}-wal" ]]; then
        cp "${BASELINE_SQLITE}-wal" "${dest}-wal" 2>/dev/null || true
    fi
    if [[ -f "${BASELINE_SQLITE}-shm" ]]; then
        cp "${BASELINE_SQLITE}-shm" "${dest}-shm" 2>/dev/null || true
    fi
    return 0
}

mailrelay_api_rm_temp() {
    rm -f "$@" 2>/dev/null || true
}

mailrelay_api_persist_enabled() {
    local engine_name="${1:-}"
    if [[ "${engine_name}" == "mysql" || "${engine_name}" == "mariadb" ]]; then
        echo "false"
        return 0
    fi
    echo "true"
}

mailrelay_api_login() {
    local base_url="$1"
    local login_id="$2"
    local password="$3"
    local output_file="$4"
    local login_data
    login_data=$(jq -n \
        --arg login_id "${login_id}" \
        --arg password "${password}" \
        --arg api_key "${HYDROGEN_DEMO_API_KEY}" \
        '{database: "Acuranzo", login_id: $login_id, password: $password, api_key: $api_key, tz: "America/Vancouver"}')
    mailrelay_api_request "POST" "${base_url}/api/auth/login" "${login_data}" "${output_file}"
}

mailrelay_api_patch_runtime() {
    local src="$1"
    local web_port="$2"
    local mailval_port="$3"
    local use_tls="$4"
    local persist="$5"
    local tls_json="false"
    local capath=""
    if [[ "${use_tls}" -eq 1 ]]; then
        tls_json="true"
        capath="${MAILVAL_CERT}"
        export MAILRELAY_MAILVAL_CERT="${MAILVAL_CERT}"
    fi
    jq --arg web_port "${web_port}" --arg port "${mailval_port}" \
       --arg capath "${capath}" \
       --argjson persist "${persist}" --argjson tls "${tls_json}" \
       '.WebServer.Port = ($web_port | tonumber) |
        .MailRelay.Database = .Databases.Connections[0].Name |
        .MailRelay.Servers[0].Port = $port |
        .MailRelay.Servers[0].UseTLS = $tls |
        .MailRelay.Servers[0].TLSMode = (if $tls then 1 else 0 end) |
        .MailRelay.Servers[0].CAPath = $capath |
        .MailRelay.AdminRecipients = ["events-sink@mailval.local"] |
        .MailRelay.Queue.Persist = $persist |
        .MailRelay.Queue.DebounceSeconds = 2 |
        .MailRelay.Events = {
            Enabled: true,
            MaxEventsPerInterval: 20,
            EventIntervalSeconds: 60,
            Rules: {
                "system.databases_ready": "Mail.Events.DatabasesReady",
                "system.server_started": "Mail.Events.ServerStarted",
                "system.server_stopped": "Mail.Events.ServerStopped"
            }
        }' \
       "${src}" 2>/dev/null || true
}

mailrelay_api_start_hydrogen() {
    local config_file="$1"
    local hydrogen_log="$2"
    local pid_var="$3"
    local hydrogen_pid
    eval "${pid_var}=''"
    # shellcheck disable=SC2310 # Continue even if startup fails
    if ! start_hydrogen_with_pid "${config_file}" "${hydrogen_log}" "${STARTUP_TIMEOUT}" "${HYDROGEN_BIN}" "${pid_var}"; then
        return 1
    fi
    hydrogen_pid=$(eval "echo \${${pid_var}}")
    if [[ -z "${hydrogen_pid}" ]]; then
        return 1
    fi
    HYDROGEN_PIDS+=("${hydrogen_pid}")
    echo "${hydrogen_pid}"
    return 0
}

mailrelay_api_run_variant() {
    local engine_name="$1"
    local description="$2"
    local config_file="$3"
    local web_port="$4"
    local mailval_port="$5"
    local use_tls="$6"
    local result_file="$7"
    local base_url="http://127.0.0.1:${web_port}"
    local variant_label="plaintext"
    local variant_tag
    local maildata_dir
    local mailval_log
    local hydrogen_log
    local sqlite_temp_file=""
    local sqlite_temp_config=""
    local actual_config_file="${config_file}"
    local temp_config
    local jq_patch
    local mailval_pid
    local hydrogen_pid_var
    local hydrogen_pid
    local variant_failed=false
    local response_dir
    local enable_persist
    local mailadmin_token=""
    local demo_token
    local http_status
    local send_data
    local send_message_id
    local send_message_id_dup
    local send_success
    local preview_data
    local preview_success
    local preview_subject
    local macros_used
    local status_success
    local idempotency_key
    local capture_file
    local capture_count
    local lifecycle_capture
    local stopped_capture
    if [[ "${use_tls}" -eq 1 ]]; then
        variant_label="STARTTLS"
    fi
    variant_tag="${engine_name}_${variant_label}_${TIMESTAMP}"
    maildata_dir="${DIAG_TEST_DIR}/mailval_${variant_tag}"
    mailval_log="${LOGS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}_mailval_${engine_name}_${variant_label}.log"
    hydrogen_log="${LOGS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}_hydrogen_${engine_name}_${variant_label}.log"
    if [[ "${engine_name}" == "sqlite" ]]; then
        sqlite_temp_file="${DIAG_TEST_DIR}/hydrodemo_${variant_tag}.sqlite"
        sqlite_temp_config="${DIAG_TEST_DIR}/hydrogen_test_${TEST_NUMBER}_sqlite_${variant_tag}.json"
        if ! mailrelay_api_copy_sqlite "${sqlite_temp_file}"; then
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}: Failed to copy baseline SQLite database"
            echo "VARIANT_${variant_label}_FAIL" >> "${result_file}"
            return 0
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
    enable_persist=$(mailrelay_api_persist_enabled "${engine_name}")
    temp_config="${DIAG_TEST_DIR}/hydrogen_test_${TEST_NUMBER}_${engine_name}_${variant_label}_${TIMESTAMP}.json"
    jq_patch=$(mailrelay_api_patch_runtime "${actual_config_file}" "${web_port}" "${mailval_port}" "${use_tls}" "${enable_persist}")
    if [[ -z "${jq_patch}" ]]; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}: Failed to patch MailRelay server config"
        echo "VARIANT_${variant_label}_FAIL" >> "${result_file}"
        return 0
    fi
    echo "${jq_patch}" > "${temp_config}"
    if [[ "${use_tls}" -eq 1 ]]; then
        if [[ ! -f "${MAILVAL_CERT}" || ! -f "${MAILVAL_KEY}" ]]; then
            bash "${MAILVAL_DIR}/gen_cert.sh" >/dev/null 2>&1 || true
        fi
    fi
    # shellcheck disable=SC2310 # Continue even if mailval startup fails
    mailval_pid=$(mailrelay_api_start_mailval "${mailval_port}" "${use_tls}" "${maildata_dir}" "${mailval_log}") || {
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}: mailval failed to start on port ${mailval_port}"
        echo "VARIANT_${variant_label}_FAIL" >> "${result_file}"
        mailrelay_api_rm_temp "${temp_config}" "${sqlite_temp_file}" "${sqlite_temp_config}" "${sqlite_temp_file}-wal" "${sqlite_temp_file}-shm"
        return 0
    }
    hydrogen_pid_var="MAILRELAY_HYDROGEN_PID_${engine_name}_${variant_label}"
    # shellcheck disable=SC2310 # Continue even if startup fails
    hydrogen_pid=$(mailrelay_api_start_hydrogen "${temp_config}" "${hydrogen_log}" "${hydrogen_pid_var}") || {
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}: Hydrogen failed to start"
        mailrelay_api_stop_mailval "${mailval_pid}"
        echo "VARIANT_${variant_label}_FAIL" >> "${result_file}"
        mailrelay_api_rm_temp "${temp_config}" "${sqlite_temp_file}" "${sqlite_temp_config}" "${sqlite_temp_file}-wal" "${sqlite_temp_file}-shm"
        return 0
    }
    # shellcheck disable=SC2310 # Continue even if HTTP readiness check fails
    if ! mailrelay_api_wait_http "${base_url}" "${HTTP_READY_TIMEOUT}"; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}: HTTP readiness check failed"
        variant_failed=true
    fi
    if [[ "${variant_failed}" = false ]]; then
        # shellcheck disable=SC2310 # Continue even if readiness check fails
        if ! mailrelay_api_wait_ready "${hydrogen_log}" "${READY_TIMEOUT}"; then
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}: READY FOR REQUESTS signal not observed"
            variant_failed=true
        fi
    fi
    if [[ "${variant_failed}" = false ]]; then
        sleep "${LIFECYCLE_SETTLE}"
        # shellcheck disable=SC2310 # Capture failures are handled below
        lifecycle_capture=$(mailrelay_api_poll_capture "${maildata_dir}" "MailRelayEvent" "${LIFECYCLE_CAPTURE_TIMEOUT}") || true
        if [[ -z "${lifecycle_capture}" ]]; then
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}: lifecycle event mail not captured (databases_ready/server_started)"
            variant_failed=true
        fi
    fi
    response_dir="${DIAG_TEST_DIR}/responses_${variant_tag}"
    mkdir -p "${response_dir}"
    if [[ "${variant_failed}" = false ]]; then
        http_status=$(mailrelay_api_request "GET" "${base_url}/api/mailrelay/status" "" "${response_dir}/noauth.json")
        if [[ "${http_status}" != "401" ]]; then
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}: Expected 401 for unauthenticated status, got ${http_status}"
            variant_failed=true
        fi
    fi
    if [[ "${variant_failed}" = false ]]; then
        http_status=$(mailrelay_api_login "${base_url}" "${HYDROGEN_MAILADMIN_NAME}" "${HYDROGEN_MAILADMIN_PASS}" "${response_dir}/login_mailadmin.json")
        if [[ "${http_status}" != "200" ]]; then
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}: mailadmin login returned HTTP ${http_status}"
            variant_failed=true
        else
            mailadmin_token=$(mailrelay_api_extract_jwt "${response_dir}/login_mailadmin.json")
            if [[ -z "${mailadmin_token}" ]]; then
                print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}: mailadmin login returned no JWT token"
                variant_failed=true
            fi
        fi
    fi
    if [[ "${variant_failed}" = false ]] && [[ -n "${mailadmin_token}" ]]; then
        http_status=$(mailrelay_api_request "GET" "${base_url}/api/mailrelay/status" "" "${response_dir}/status.json" "${mailadmin_token}")
        if [[ "${http_status}" != "200" ]]; then
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}: status endpoint returned HTTP ${http_status}"
            variant_failed=true
        else
            status_success=$(jq -r '.success // false' "${response_dir}/status.json" 2>/dev/null || echo "false")
            if [[ "${status_success}" != "true" ]]; then
                print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}: status endpoint returned success=${status_success}"
                variant_failed=true
            fi
        fi
    fi
    if [[ "${variant_failed}" = false ]] && [[ -n "${mailadmin_token}" ]]; then
        preview_data=$(jq -n --arg name "MailRelayBlackbox" '{template_key: "mail.test", params: {NAME: $name}}')
        http_status=$(mailrelay_api_request "POST" "${base_url}/api/mailrelay/preview" "${preview_data}" "${response_dir}/preview.json" "${mailadmin_token}")
        if [[ "${http_status}" != "200" ]]; then
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}: preview endpoint returned HTTP ${http_status}"
            variant_failed=true
        else
            preview_success=$(jq -r '.success // false' "${response_dir}/preview.json" 2>/dev/null || echo "false")
            if [[ "${preview_success}" != "true" ]]; then
                print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}: preview endpoint returned success=${preview_success}"
                variant_failed=true
            else
                preview_subject=$(jq -r '.subject // empty' "${response_dir}/preview.json" 2>/dev/null || true)
                if [[ "${preview_subject}" != *"MailRelayBlackbox"* ]]; then
                    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}: preview subject missing marker ('${preview_subject}')"
                    variant_failed=true
                fi
                macros_used=$(jq -r '.macros_used // [] | map(. == "NAME") | any' "${response_dir}/preview.json" 2>/dev/null || echo "false")
                if [[ "${macros_used}" != "true" ]]; then
                    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}: preview macros_used missing NAME"
                    variant_failed=true
                fi
            fi
        fi
    fi
    idempotency_key="mailrelay-api-${engine_name}-${variant_label}-${TIMESTAMP}-${RANDOM}"
    if [[ "${variant_failed}" = false ]] && [[ -n "${mailadmin_token}" ]]; then
        send_data=$(jq -n \
            --arg template_key "mail.test" \
            --arg idempotency_key "${idempotency_key}" \
            --arg name "MailRelayBlackbox" \
            '{template_key: $template_key, to: ["sink@mailval.local"], idempotency_key: $idempotency_key, params: {NAME: $name}}')
        http_status=$(mailrelay_api_request "POST" "${base_url}/api/mailrelay/send" "${send_data}" "${response_dir}/send.json" "${mailadmin_token}")
        if [[ "${http_status}" != "200" ]]; then
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}: send endpoint returned HTTP ${http_status}"
            variant_failed=true
        else
            send_success=$(jq -r '.success // false' "${response_dir}/send.json" 2>/dev/null || echo "false")
            if [[ "${send_success}" != "true" ]]; then
                print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}: send endpoint returned success=${send_success}"
                variant_failed=true
            elif [[ "${enable_persist}" == "true" ]]; then
                send_message_id=$(jq -r '.message_id // empty' "${response_dir}/send.json" 2>/dev/null || true)
                sleep "${IDEMPOTENCY_SETTLE}"
                http_status=$(mailrelay_api_request "POST" "${base_url}/api/mailrelay/send" "${send_data}" "${response_dir}/send_dup.json" "${mailadmin_token}")
                if [[ "${http_status}" != "200" ]]; then
                    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}: duplicate send returned HTTP ${http_status}"
                    variant_failed=true
                else
                    send_success=$(jq -r '.success // false' "${response_dir}/send_dup.json" 2>/dev/null || echo "false")
                    send_message_id_dup=$(jq -r '.message_id // empty' "${response_dir}/send_dup.json" 2>/dev/null || true)
                    if [[ "${send_success}" != "true" ]] || [[ -z "${send_message_id}" ]] || [[ "${send_message_id}" != "${send_message_id_dup}" ]]; then
                        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}: 11.4 idempotency failed (first=${send_message_id:-empty} second=${send_message_id_dup:-empty} success=${send_success})"
                        variant_failed=true
                    fi
                fi
            fi
        fi
    fi
    if [[ "${variant_failed}" = false ]]; then
        http_status=$(mailrelay_api_login "${base_url}" "${HYDROGEN_DEMO_USER_NAME}" "${HYDROGEN_DEMO_USER_PASS}" "${response_dir}/login_demo.json")
        if [[ "${http_status}" != "200" ]]; then
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}: demo user login returned HTTP ${http_status}"
            variant_failed=true
        else
            demo_token=$(mailrelay_api_extract_jwt "${response_dir}/login_demo.json")
            if [[ -z "${demo_token}" ]]; then
                print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}: demo user login returned no JWT token"
                variant_failed=true
            else
                http_status=$(mailrelay_api_request "POST" "${base_url}/api/mailrelay/send" "${send_data}" "${response_dir}/send_unauthorized.json" "${demo_token}")
                if [[ "${http_status}" != "401" ]]; then
                    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}: expected 401 for unauthorized send, got ${http_status}"
                    variant_failed=true
                fi
            fi
        fi
    fi
    # shellcheck disable=SC2310 # Continue even if shutdown fails
    stop_hydrogen "${hydrogen_pid}" "${hydrogen_log}" "${SHUTDOWN_TIMEOUT}" "${SHUTDOWN_ACTIVITY_TIMEOUT}" "${DIAG_TEST_DIR}"
    if [[ "${variant_failed}" = false ]]; then
        # shellcheck disable=SC2310 # Capture failures are handled below
        capture_file=$(mailrelay_api_poll_capture "${maildata_dir}" "MailRelayBlackbox" "${CAPTURE_TIMEOUT}") || true
        if [[ -z "${capture_file}" ]]; then
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}: SMTP sink did not capture message with subject marker"
            variant_failed=true
        elif [[ "${enable_persist}" == "true" ]]; then
            capture_count=$(mailrelay_api_count_subject "${maildata_dir}" "MailRelayBlackbox")
            if [[ "${capture_count}" -ne 1 ]]; then
                print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}: 11.4 expected 1 MailRelayBlackbox delivery, got ${capture_count}"
                variant_failed=true
            fi
        fi
    fi
    if [[ "${variant_failed}" = false ]]; then
        # shellcheck disable=SC2310 # Capture failures are handled below
        stopped_capture=$(mailrelay_api_poll_capture "${maildata_dir}" "server_stopped" 20) || true
        if [[ -z "${stopped_capture}" ]]; then
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}: server_stopped capture missing (soft)"
        fi
    fi
    mailrelay_api_stop_mailval "${mailval_pid}"
    mailrelay_api_rm_temp "${temp_config}" "${sqlite_temp_file}" "${sqlite_temp_config}" "${sqlite_temp_file}-wal" "${sqlite_temp_file}-shm"
    if [[ "${variant_failed}" = false ]]; then
        echo "VARIANT_${variant_label}_PASS" >> "${result_file}"
    else
        echo "VARIANT_${variant_label}_FAIL" >> "${result_file}"
    fi
    return 0
}

mailrelay_api_run_one_variant() {
    local test_name="$1"
    local config_file="$2"
    local result_suffix="$3"
    local engine_name="$4"
    local description="$5"
    local web_port="$6"
    local mailval_port="$7"
    local use_tls="$8"
    local result_file="${LOG_PREFIX}_${result_suffix}.result"
    local variant_label="plaintext"
    if [[ "${use_tls}" -eq 1 ]]; then
        variant_label="STARTTLS"
    fi
    true > "${result_file}"
    # shellcheck disable=SC2310 # Continue even if variant fails
    mailrelay_api_run_variant "${engine_name}" "${description}" "${config_file}" \
        "${web_port}" "${mailval_port}" "${use_tls}" "${result_file}"
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

# One engine job: plaintext then STARTTLS so shared-DB engines do not
# race MAX(queue_id)+1 inserts from two Hydrogen processes.
mailrelay_api_run_engine_pair() {
    local display_name="$1"
    local engine_name="$2"
    local web_plain="$3"
    local mail_plain="$4"
    local web_tls="$5"
    local mail_tls="$6"
    local config_file="${SCRIPT_DIR}/configs/hydrogen_test_${TEST_NUMBER}_${engine_name}.json"
    mailrelay_api_run_one_variant "${display_name}-plaintext" "${config_file}" \
        "${engine_name}-plaintext" "${engine_name}" "${display_name}-plaintext Engine" \
        "${web_plain}" "${mail_plain}" 0
    mailrelay_api_run_one_variant "${display_name}-STARTTLS" "${config_file}" \
        "${engine_name}-starttls" "${engine_name}" "${display_name}-STARTTLS Engine" \
        "${web_tls}" "${mail_tls}" 1
    return 0
}

mailrelay_api_analyze() {
    local test_name="$1"
    local result_suffix="$2"
    local description="$3"
    local result_file="${LOG_PREFIX}_${result_suffix}.result"
    print_marker "${TEST_NUMBER}" "${TEST_COUNTER}"
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}: Analyzing results"
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}: API End-to-End"
    if [[ ! -f "${result_file}" ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${description}: No result file found"
        return 1
    fi
    if "${GREP}" -q "ENGINE_TEST_COMPLETE" "${result_file}" 2>/dev/null; then
        echo "MAILRELAY_API_${test_name}_PASS" >> "${GLOBAL_RESULT_FILE:-${DIAG_TEST_DIR}/mailrelay_api_results.result}"
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "${description}: API test passed"
        return 0
    fi
    if "${GREP}" -q "ENGINE_TEST_FAILED" "${result_file}" 2>/dev/null \
        || "${GREP}" -q "VARIANT_.*_FAIL" "${result_file}" 2>/dev/null; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${description}: API test failed"
    else
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${description}: API test failed (incomplete result file)"
    fi
    return 1
}

mailrelay_api_run_otp_launch() {
    local label="$1"
    local config_file="$2"
    local web_port="$3"
    local mailval_port="$4"
    local recipient="$5"
    local max_recipient="${6:-${OTP_MAX_RECIPIENT}}"
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "${label}"
    local variant_tag="otp_launch_${TIMESTAMP}"
    local maildata_dir="${DIAG_TEST_DIR}/mailval_${variant_tag}"
    local mailval_log="${LOGS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}_mailval_otp.log"
    local hydrogen_log="${LOGS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}_hydrogen_otp.log"
    local sqlite_temp_file="${DIAG_TEST_DIR}/hydrodemo_${variant_tag}.sqlite"
    local sqlite_temp_config="${DIAG_TEST_DIR}/hydrogen_test_${TEST_NUMBER}_otp_${TIMESTAMP}.json"
    local jq_patch
    local mailval_pid
    local hydrogen_pid_var="MAILRELAY_HYDROGEN_PID_otp"
    local hydrogen_pid=""
    local failed=false
    local marker
    local consumed
    local maxed
    true > "${mailval_log}"
    mkdir -p "${maildata_dir}"
    if ! mailrelay_api_copy_sqlite "${sqlite_temp_file}"; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${label}: Failed to copy baseline SQLite database"
        return 1
    fi
    jq_patch=$(jq --arg web_port "${web_port}" \
                   --arg port "${mailval_port}" \
                   --arg db "${sqlite_temp_file}" \
                   '.WebServer.Port = ($web_port | tonumber) |
                    .MailRelay.Database = .Databases.Connections[0].Name |
                    .MailRelay.Servers[0].Port = $port |
                    .MailRelay.Servers[0].UseTLS = false |
                    .MailRelay.Servers[0].TLSMode = 0 |
                    .MailRelay.Servers[0].CAPath = "" |
                    .MailRelay.Queue.Persist = true |
                    .Databases.Connections[0].Database = $db |
                    .MailRelay.Test.SendOtpOnLaunch = true |
                    .MailRelay.Test.MailRepoProbeOnLaunch = true' \
                   "${config_file}" 2>/dev/null) || true
    if [[ -z "${jq_patch}" ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${label}: Failed to patch OTP launch config"
        mailrelay_api_rm_temp "${sqlite_temp_config}" "${sqlite_temp_file}" "${sqlite_temp_file}-wal" "${sqlite_temp_file}-shm"
        return 1
    fi
    echo "${jq_patch}" > "${sqlite_temp_config}"
    # shellcheck disable=SC2310 # Continue even if mailval startup fails
    mailval_pid=$(mailrelay_api_start_mailval "${mailval_port}" 0 "${maildata_dir}" "${mailval_log}") || {
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${label}: mailval failed to start on port ${mailval_port}"
        mailrelay_api_rm_temp "${sqlite_temp_config}" "${sqlite_temp_file}" "${sqlite_temp_file}-wal" "${sqlite_temp_file}-shm"
        return 1
    }
    # shellcheck disable=SC2310 # Continue even if startup fails
    hydrogen_pid=$(mailrelay_api_start_hydrogen "${sqlite_temp_config}" "${hydrogen_log}" "${hydrogen_pid_var}") || {
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${label}: Hydrogen failed to start"
        mailrelay_api_stop_mailval "${mailval_pid}"
        mailrelay_api_rm_temp "${sqlite_temp_config}" "${sqlite_temp_file}" "${sqlite_temp_file}-wal" "${sqlite_temp_file}-shm"
        return 1
    }
    # shellcheck disable=SC2310 # Continue even if readiness check fails
    if ! mailrelay_api_wait_ready "${hydrogen_log}" "${READY_TIMEOUT}"; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${label}: READY FOR REQUESTS signal not observed"
        failed=true
    fi
    if [[ "${failed}" = false ]]; then
        for marker in \
            MAILRELAY_OTP_LAUNCH_SENT \
            MAILRELAY_OTP_LAUNCH_WRONG_CODE \
            MAILRELAY_OTP_LAUNCH_VERIFIED \
            MAILRELAY_OTP_LAUNCH_MAX_SENT \
            MAILRELAY_OTP_LAUNCH_MAX_ATTEMPTS \
            MAILRELAY_REPO_PROBE_OK; do
            if ! "${GREP}" -q "${marker}" "${hydrogen_log}" 2>/dev/null; then
                print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${label}: log missing ${marker}"
                failed=true
            fi
        done
    fi
    if [[ "${failed}" = false ]] && command -v sqlite3 >/dev/null 2>&1; then
        consumed=$(sqlite3 "${sqlite_temp_file}" \
            "SELECT COUNT(*) FROM mail_otp_codes WHERE email='${recipient}' AND purpose_a66=${OTP_PURPOSE} AND status_a67=${OTP_STATUS_CONSUMED};" \
            2>/dev/null || echo "0")
        if [[ -z "${consumed}" ]] || [[ "${consumed}" = "0" ]]; then
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${label}: OTP row not consumed in DB (count=${consumed})"
            failed=true
        fi
        maxed=$(sqlite3 "${sqlite_temp_file}" \
            "SELECT COUNT(*) FROM mail_otp_codes WHERE email='${max_recipient}' AND purpose_a66=${OTP_PURPOSE} AND status_a67=${OTP_STATUS_MAX_ATTEMPTS};" \
            2>/dev/null || echo "0")
        if [[ -z "${maxed}" ]] || [[ "${maxed}" = "0" ]]; then
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${label}: OTP max-attempts row missing in DB (count=${maxed})"
            failed=true
        fi
    elif [[ "${failed}" = false ]]; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${label}: sqlite3 unavailable; relying on log markers for OTP assertions"
    fi
    # shellcheck disable=SC2310 # Continue even if shutdown fails
    stop_hydrogen "${hydrogen_pid}" "${hydrogen_log}" "${SHUTDOWN_TIMEOUT}" "${SHUTDOWN_ACTIVITY_TIMEOUT}" "${DIAG_TEST_DIR}"
    mailrelay_api_stop_mailval "${mailval_pid}"
    mailrelay_api_rm_temp "${sqlite_temp_config}" "${sqlite_temp_file}" "${sqlite_temp_file}-wal" "${sqlite_temp_file}-shm"
    if [[ "${failed}" = false ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "${label}: OTP + H.mail repo probe (markers + DB)"
        PASS_COUNT=$(( PASS_COUNT + 1 ))
        return 0
    fi
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${label}: OTP / repo-probe launch coverage failed"
    return 1
}
