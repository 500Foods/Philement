#!/usr/bin/env bash

# Test: Conduit Webhook (LUA_CLIENT Phase 14)
# POST /api/conduit/webhook/{hook} + /webhook/{hook} alias across engines.
# HMAC verification (sha256, sha256-timestamp), all error branches, the
# alias path, and the webhooks-disabled / scripting-disabled 503 variants.
# No JWT: webhook ingress is unauthenticated by design.

# FUNCTIONS
# wh_sign()
# wh_post()
# wh_get()
# signed_webhook_case()
# wait_ready()
# wait_http()
# seed_sqlite_lua_client_fixture()
# prepare_sqlite_config()
# record_case()
# run_engine()
# analyze_engine()

# CHANGELOG
# 1.0.0 - 2026-10-14 - Initial blackbox for LUA_CLIENT Phase 14 (webhook.c coverage)

set -euo pipefail

TEST_NAME="Conduit Webhook"
TEST_ABBR="CWH"
TEST_NUMBER="63"
TEST_COUNTER=0
TEST_VERSION="1.0.0"

# shellcheck source=tests/lib/framework.sh # Reference framework directly
[[ -n "${FRAMEWORK_GUARD:-}" ]] || source "$(dirname "${BASH_SOURCE[0]}")/lib/framework.sh"
setup_test_environment

# shellcheck source=tests/lib/group40_http.sh # Shared 40-series HTTP timing
[[ -n "${GROUP40_HTTP_GUARD:-}" ]] || source "$(dirname "${BASH_SOURCE[0]}")/lib/group40_http.sh"
# shellcheck source=tests/lib/scripting_helpers.sh # Start/shutdown helpers shared with test_43/46
source "$(dirname "${BASH_SOURCE[0]}")/lib/scripting_helpers.sh"

declare -a PARALLEL_PIDS
declare -A WEBHOOK_TEST_CONFIGS

# config:log_suffix:engine_key:description:variant
# variant: full (all cases), wh_disabled (503), scripting_disabled (503)
WEBHOOK_TEST_CONFIGS=(
    ["PostgreSQL"]="${SCRIPT_DIR}/configs/hydrogen_test_${TEST_NUMBER}_conduit_webhook_postgres.json:postgres:postgresql:PostgreSQL:full"
    ["MySQL"]="${SCRIPT_DIR}/configs/hydrogen_test_${TEST_NUMBER}_conduit_webhook_mysql.json:mysql:mysql:MySQL:full"
    ["SQLite"]="${SCRIPT_DIR}/configs/hydrogen_test_${TEST_NUMBER}_conduit_webhook_sqlite.json:sqlite:sqlite:SQLite:full"
    ["DB2"]="${SCRIPT_DIR}/configs/hydrogen_test_${TEST_NUMBER}_conduit_webhook_db2.json:db2:db2:DB2:full"
    ["MariaDB"]="${SCRIPT_DIR}/configs/hydrogen_test_${TEST_NUMBER}_conduit_webhook_mariadb.json:mariadb:mariadb:MariaDB:full"
    ["Firebird"]="${SCRIPT_DIR}/configs/hydrogen_test_${TEST_NUMBER}_conduit_webhook_firebird.json:firebird:firebird:Firebird:full"
    ["YugabyteDB"]="${SCRIPT_DIR}/configs/hydrogen_test_${TEST_NUMBER}_conduit_webhook_yugabytedb.json:yugabytedb:yugabytedb:YugabyteDB:full"
    ["MSSQL"]="${SCRIPT_DIR}/configs/hydrogen_test_${TEST_NUMBER}_conduit_webhook_mssql.json:mssql:mssql:MSSQL:full"
    ["SQLite-WebhooksDisabled"]="${SCRIPT_DIR}/configs/hydrogen_test_${TEST_NUMBER}_conduit_webhook_sqlite_wh_disabled.json:sqlite_wh_disabled:sqlite:SQLite (webhooks disabled):wh_disabled"
    ["SQLite-ScriptingDisabled"]="${SCRIPT_DIR}/configs/hydrogen_test_${TEST_NUMBER}_conduit_webhook_sqlite_scripting_disabled.json:sqlite_scripting_disabled:sqlite:SQLite (scripting disabled):scripting_disabled"
)

# shellcheck disable=SC2034 # Reserved for log messages / future fail-fast bounds
STARTUP_TIMEOUT="${GROUP40_STARTUP_TIMEOUT}"
SHUTDOWN_TIMEOUT="${GROUP40_SHUTDOWN_TIMEOUT}"
READY_TIMEOUT="${GROUP40_READY_TIMEOUT}"
BASELINE_SQLITE="${PROJECT_DIR}/tests/artifacts/database/sqlite/hydrodemo.sqlite"

# Webhook HMAC secret. The configs name this variable in each hook's
# SecretEnv, so every Hydrogen instance getenv()s it at request time.
HYDROGEN_TEST_WEBHOOK_SECRET="conduit-webhook-blackbox-secret-63"
export HYDROGEN_TEST_WEBHOOK_SECRET

# Lowercase hex HMAC-SHA256 of <payload> under the webhook secret.
# Matches utils_hmac_sha256() + conduit_webhook_bytes_to_hex() server-side.
wh_sign() {
    local payload="$1"
    local hex=""
    hex=$(printf '%s' "${payload}" | openssl dgst -sha256 -hmac "${HYDROGEN_TEST_WEBHOOK_SECRET}" -hex 2>/dev/null | awk '{print $NF}' || true)
    printf '%s\n' "${hex}"
}

# POST helper: wh_post <url> <output_file> <data> [extra curl args...]
# An empty <data> sends Content-Length: 0 (empty-body webhook case).
wh_post() {
    local url="$1"
    local out_file="$2"
    local data="$3"
    shift 3
    local -a curl_args=(-X POST -H "Content-Type: application/json")
    if [[ -n "${data}" ]]; then
        curl_args+=(-d "${data}")
    fi
    if [[ "$#" -gt 0 ]]; then
        curl_args+=("$@")
    fi
    curl_args+=("${url}")
    group40_curl_to "${out_file}" "${curl_args[@]}"
}

# GET helper: wh_get <url> <output_file>
wh_get() {
    local url="$1"
    local out_file="$2"
    group40_curl_to "${out_file}" -X GET "${url}"
}

# One signed webhook delivery that must reach a completed Api.Echo job.
# Retries on load-induced timeouts (the C-side wait is a fixed 15s).
# signed_webhook_case <result_file> <case> <url> <body> <expect_hook> \
#                     <header_name> <header_value> <description>
signed_webhook_case() {
    local result_file="$1"
    local case_name="$2"
    local url="$3"
    local body="$4"
    local expect_hook="$5"
    local header_name="$6"
    local header_value="$7"
    local description="$8"
    local out_file="${result_file}.case.json"
    local http_st="000"
    local st=""
    local try=1
    local ok=0

    while [[ "${try}" -le 3 ]]; do
        http_st=$(wh_post "${url}" "${out_file}" "${body}" \
            -H "${header_name}: ${header_value}" \
            -H "X-Request-Id: webhook-blackbox-63")
        st=$(jq -r '.status // empty' "${out_file}" 2>/dev/null || true)
        if [[ "${http_st}" == "200" && "${st}" == "completed" ]]; then
            ok=1
            break
        fi
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" \
            "INFO ${description}: ${case_name} HTTP ${http_st} status ${st:-?} (try ${try}/3)"
        if [[ "${http_st}" != "200" && "${http_st}" != "000" && "${http_st}" != 5* ]]; then
            break
        fi
        sleep 1
        try=$(( try + 1 ))
    done

    if [[ "${ok}" -eq 1 ]] && jq -e \
        --arg hook "${expect_hook}" \
        --arg body "${body}" '
        (.script == "Api.Echo")
        and (.result.hook == $hook)
        and (.result.body == $body)
        and (.result.content_type == "application/json")
        and (.result.headers["X-Request-Id"] == "webhook-blackbox-63")
        ' "${out_file}" >/dev/null 2>&1; then
        record_case "${result_file}" "${case_name}" 1
    else
        record_case "${result_file}" "${case_name}" 0
        {
            echo "CASE_HTTP_${case_name}=${http_st}"
            echo "CASE_STATUS_${case_name}=${st}"
        } >> "${result_file}"
    fi
}

wait_ready() {
    local log_file="$1"
    local timeout="${2:-${READY_TIMEOUT}}"
    local start_time
    local now_secs
    start_time=$("${DATE}" +%s)
    while true; do
        if "${GREP}" -q "READY FOR REQUESTS" "${log_file}" 2>/dev/null; then
            return 0
        fi
        now_secs=$("${DATE}" +%s)
        if (( now_secs - start_time >= timeout )); then
            return 1
        fi
        sleep 0.2
    done
}

wait_http() {
    local url="$1"
    local timeout="${2:-30}"
    local start_time now_secs http_st
    start_time=$("${DATE}" +%s)
    while true; do
        # shellcheck disable=SC2312 # curl exit ignored; HTTP code is the signal
        http_st=$(curl -s -o /dev/null -w "%{http_code}" \
            --connect-timeout "${GROUP40_CONNECT_TIMEOUT}" --max-time 15 \
            "${url}" 2>/dev/null || true)
        http_st="${http_st:-000}"
        if [[ "${http_st}" != "000" && "${http_st}" != "" ]]; then
            return 0
        fi
        now_secs=$("${DATE}" +%s)
        if (( now_secs - start_time >= timeout )); then
            return 1
        fi
        sleep 0.2
    done
}

# SQLite: isolate fixture + seed the LUA_CLIENT fixture (Api.Echo, invokable,
# QueryRef #149). AutoMigration is left off, exactly like test_46: the baseline
# APPLY has a hole at 1283 and migration 1293 fails on SQLite seed SQL, so the
# blackbox seeds the minimal surface the webhook path needs.
seed_sqlite_lua_client_fixture() {
    local db_copy="$1"
    if ! command -v sqlite3 >/dev/null 2>&1; then
        return 1
    fi

    # Idempotently ensure the invokable allowlist column exists (migration 1297
    # may already have applied it on a fresh baseline). SQLite's ALTER TABLE ...
    # ADD COLUMN has no IF NOT EXISTS clause, so check pragma_table_info first.
    local invokable_present
    invokable_present=$(sqlite3 "${db_copy}" \
        "SELECT COUNT(*) FROM pragma_table_info('scripts') WHERE name='invokable';" 2>/dev/null || echo 0)
    if [[ "${invokable_present}" -eq 0 ]]; then
        sqlite3 "${db_copy}" "ALTER TABLE scripts ADD COLUMN invokable INTEGER NOT NULL DEFAULT 0;" || return 1
    fi

    # All inserts use INSERT OR IGNORE so this function is idempotent: if
    # migrations 1296/1298 already seeded Api.Echo / QueryRef #149 into the
    # baseline, those rows are preserved (never downgraded).
    sqlite3 "${db_copy}" <<'SQL'
-- Api.Echo (migration 1296) — skip if the baseline already provides it
INSERT OR IGNORE INTO scripts (
    group_name, script_name, script_type, schedule, next_run,
    last_run_start, last_run_end, status, code, summary,
    created_id, created_at, updated_id, updated_at, invokable
) VALUES (
    'Api', 'Echo', 1, NULL, NULL, NULL, NULL, 1,
    '-- Api.Echo (LUA_CLIENT Phase 6 fixture)
local out = {}
if type(params) == "table" then
    for k, v in pairs(params) do
        out[k] = v
    end
end
H.set_result_json(out)
return 0
',
    'LUA_CLIENT fixture: echo params',
    1, datetime('now'), 1, datetime('now'), 1
);

-- QueryRef #149 (migration 1298) — invokable-only script load; skip if present
INSERT OR IGNORE INTO queries (
    query_id, query_ref, query_status_a27, query_type_a28, query_dialect_a30,
    query_queue_a58, query_timeout, code, name, summary, collection,
    created_id, created_at, updated_id, updated_at
)
SELECT
    (SELECT COALESCE(MAX(query_id), 0) + 1 FROM queries),
    149, 1, 1, query_dialect_a30, 2, 5000,
    'SELECT group_name, script_name, script_type, schedule, next_run,
            last_run_start, last_run_end, status, code, summary, invokable
       FROM scripts
      WHERE (group_name = :GROUP_NAME)
        AND (script_name = :SCRIPT_NAME)
        AND (invokable <> 0)',
    'Get Invokable Script by Group/Name',
    'LUA_CLIENT blackbox seed QueryRef #149',
    '{}',
    1, datetime('now'), 1, datetime('now')
FROM queries
WHERE query_ref = 87 AND query_type_a28 = 1
LIMIT 1;
SQL
}

prepare_sqlite_config() {
    local src_config="$1"
    local work_dir="$2"
    local out_config="${work_dir}/config.json"
    local db_copy="${work_dir}/hydrodemo.sqlite"
    mkdir -p "${work_dir}"
    # shellcheck disable=SC2310 # backup failure is fatal for the SQLite engine run
    if ! sqlite_online_backup "${BASELINE_SQLITE}" "${db_copy}"; then
        return 1
    fi
    # shellcheck disable=SC2310 # Seed failure is fatal for the SQLite engine run
    if ! seed_sqlite_lua_client_fixture "${db_copy}"; then
        return 1
    fi
    jq --arg db "${db_copy}" '
        .Databases.Connections |= map(
            if ((.Engine // "") | ascii_downcase) == "sqlite" then
                .Database = $db | .AutoMigration = false
            else . end
        )
    ' "${src_config}" > "${out_config}"
    echo "${out_config}"
}

# Record one pass/fail line into result file; always return 0 for parallel set -e.
record_case() {
    local result_file="$1"
    local name="$2"
    local ok="$3"
    if [[ "${ok}" == "1" ]]; then
        echo "CASE_PASS=${name}" >> "${result_file}"
    else
        echo "CASE_FAIL=${name}" >> "${result_file}"
    fi
}

run_engine() {
    local config_file="$1"
    local log_file="$2"
    local result_file="$3"
    local hydrogen_bin="$4"
    local engine_key="$5"
    local description="$6"
    local variant="$7"

    {
        echo "ENGINE=${engine_key}"
        echo "VARIANT=${variant}"
        echo "DESCRIPTION=${description}"

        # Full engines run the whole battery (19 cases); SQLite adds the
        # oversize-body case (20). The 503 variants run exactly one case.
        local min_cases=19
        if [[ "${variant}" != "full" ]]; then
            min_cases=1
        elif [[ "${engine_key}" == "sqlite" ]]; then
            min_cases=20
        fi
        echo "MIN_CASES=${min_cases}"
    } > "${result_file}"

    local work_dir=""
    local run_config="${config_file}"
    if [[ "${engine_key}" == "sqlite" ]]; then
        work_dir="${DIAG_TEST_DIR}/sqlite_${variant}_$$"
        # shellcheck disable=SC2310 # Capture STARTUP_FAILED when SQLite copy fails
        run_config=$(prepare_sqlite_config "${config_file}" "${work_dir}") || {
            echo "STARTUP_FAILED=1" >> "${result_file}"
            echo "REASON=sqlite_copy" >> "${result_file}"
            return 0
        }
    fi

    local port
    port=$(get_webserver_port "${run_config}")
    local base_url="http://127.0.0.1:${port}"
    echo "PORT=${port}" >> "${result_file}"
    echo "BASE_URL=${base_url}" >> "${result_file}"

    local hydrogen_pid=""
    # shellcheck disable=SC2310 # Continue writing STARTUP_FAILED when start returns non-zero
    if ! scripting_start_instance "${run_config}" "${log_file}" "${hydrogen_bin}" hydrogen_pid; then
        echo "STARTUP_FAILED=1" >> "${result_file}"
        return 0
    fi

    # shellcheck disable=SC2310 # Continue writing NOT_READY when wait returns non-zero
    if ! wait_ready "${log_file}" "${READY_TIMEOUT}"; then
        echo "NOT_READY=1" >> "${result_file}"
        # shellcheck disable=SC2310 # Shutdown best-effort after timeout
        scripting_shutdown_instance "${hydrogen_pid}" "${SHUTDOWN_TIMEOUT}" || true
        return 0
    fi
    echo "READY=1" >> "${result_file}"
    # Log READY can precede MHD bind under suite load (HTTP 000).
    # shellcheck disable=SC2310 # Continue; request retries still cover bind lag
    wait_http "${base_url}/api/version" "${GROUP40_READY_TIMEOUT}" || true

    local http_st=""
    local out_file="${result_file}.case.json"
    local sig=""
    local body=""
    local ts=""

    if [[ "${variant}" == "wh_disabled" ]]; then
        # Webhooks disabled: rejected before any signature/scripting work.
        http_st=$(wh_post "${base_url}/api/conduit/webhook/echo" "${out_file}" \
            '{"k":1}' -H "X-Signature-256: v1=deadbeef")
        if [[ "${http_st}" == "503" ]] && jq -e '.error == "webhooks_disabled"' \
            "${out_file}" >/dev/null 2>&1; then
            record_case "${result_file}" "webhooks_disabled_503" 1
        else
            record_case "${result_file}" "webhooks_disabled_503" 0
            echo "WEBHOOKS_DISABLED_HTTP=${http_st}" >> "${result_file}"
        fi
    elif [[ "${variant}" == "scripting_disabled" ]]; then
        # Webhooks on, Scripting off: 503 before the hook lookup runs.
        http_st=$(wh_post "${base_url}/api/conduit/webhook/echo" "${out_file}" \
            '{"k":1}' -H "X-Signature-256: v1=deadbeef")
        if [[ "${http_st}" == "503" ]] && jq -e '.error == "scripting_disabled"' \
            "${out_file}" >/dev/null 2>&1; then
            record_case "${result_file}" "scripting_disabled_503" 1
        else
            record_case "${result_file}" "scripting_disabled_503" 0
            echo "SCRIPTING_DISABLED_HTTP=${http_st}" >> "${result_file}"
        fi
    else
        # --- 405: GET on the endpoint ---
        http_st=$(wh_get "${base_url}/api/conduit/webhook/echo" "${out_file}")
        if [[ "${http_st}" == "405" ]] && jq -e '.error == "method_not_allowed"' \
            "${out_file}" >/dev/null 2>&1; then
            record_case "${result_file}" "get_405" 1
        else
            record_case "${result_file}" "get_405" 0
            echo "GET_HTTP=${http_st}" >> "${result_file}"
        fi

        # --- 404: hook path carries an extra slash (neither extractor yields a name) ---
        http_st=$(wh_post "${base_url}/api/conduit/webhook/a/b" "${out_file}" \
            '{"k":1}' -H "X-Signature-256: v1=deadbeef")
        if [[ "${http_st}" == "404" ]] && jq -e '.error == "hook_not_found"' \
            "${out_file}" >/dev/null 2>&1; then
            record_case "${result_file}" "slash_hook_404" 1
        else
            record_case "${result_file}" "slash_hook_404" 0
            echo "SLASH_HOOK_HTTP=${http_st}" >> "${result_file}"
        fi

        # --- 404: empty hook name (trailing slash only) ---
        http_st=$(wh_post "${base_url}/api/conduit/webhook/" "${out_file}" \
            '{"k":1}' -H "X-Signature-256: v1=deadbeef")
        if [[ "${http_st}" == "404" ]] && jq -e '.error == "hook_not_found"' \
            "${out_file}" >/dev/null 2>&1; then
            record_case "${result_file}" "empty_hook_404" 1
        else
            record_case "${result_file}" "empty_hook_404" 0
            echo "EMPTY_HOOK_HTTP=${http_st}" >> "${result_file}"
        fi

        # --- 404: well-formed signature, unknown hook name ---
        sig=$(wh_sign '{"k":1}')
        http_st=$(wh_post "${base_url}/api/conduit/webhook/nope" "${out_file}" \
            '{"k":1}' -H "X-Signature-256: v1=${sig}")
        if [[ "${http_st}" == "404" ]] && jq -e '.error == "hook_not_found"' \
            "${out_file}" >/dev/null 2>&1; then
            record_case "${result_file}" "unknown_hook_404" 1
        else
            record_case "${result_file}" "unknown_hook_404" 0
            echo "UNKNOWN_HOOK_HTTP=${http_st}" >> "${result_file}"
        fi

        # --- 401: no signature header at all ---
        http_st=$(wh_post "${base_url}/api/conduit/webhook/echo" "${out_file}" '{"k":1}')
        if [[ "${http_st}" == "401" ]] && jq -e '.error == "invalid_signature"' \
            "${out_file}" >/dev/null 2>&1; then
            record_case "${result_file}" "missing_sig_401" 1
        else
            record_case "${result_file}" "missing_sig_401" 0
            echo "MISSING_SIG_HTTP=${http_st}" >> "${result_file}"
        fi

        # --- 401: short (wrong-length) signature token ---
        http_st=$(wh_post "${base_url}/api/conduit/webhook/echo" "${out_file}" \
            '{"k":1}' -H "X-Signature-256: v1=deadbeef")
        if [[ "${http_st}" == "401" ]] && jq -e '.error == "invalid_signature"' \
            "${out_file}" >/dev/null 2>&1; then
            record_case "${result_file}" "bad_sig_401" 1
        else
            record_case "${result_file}" "bad_sig_401" 0
            echo "BAD_SIG_HTTP=${http_st}" >> "${result_file}"
        fi

        # --- 401: full-length but wrong signature token ---
        http_st=$(wh_post "${base_url}/api/conduit/webhook/echo" "${out_file}" \
            '{"k":1}' -H "X-Signature-256: v1=0000000000000000000000000000000000000000000000000000000000000000")
        if [[ "${http_st}" == "401" ]] && jq -e '.error == "invalid_signature"' \
            "${out_file}" >/dev/null 2>&1; then
            record_case "${result_file}" "wrong_sig_401" 1
        else
            record_case "${result_file}" "wrong_sig_401" 0
            echo "WRONG_SIG_HTTP=${http_st}" >> "${result_file}"
        fi

        # --- 401: SecretEnv names an environment variable that is not set ---
        sig=$(wh_sign '{"k":1}')
        http_st=$(wh_post "${base_url}/api/conduit/webhook/noenv" "${out_file}" \
            '{"k":1}' -H "X-Signature-256: v1=${sig}")
        if [[ "${http_st}" == "401" ]] && jq -e '.error == "invalid_signature"' \
            "${out_file}" >/dev/null 2>&1; then
            record_case "${result_file}" "noenv_401" 1
        else
            record_case "${result_file}" "noenv_401" 0
            echo "NOENV_HTTP=${http_st}" >> "${result_file}"
        fi

        # --- 401: hook configures no SignatureHeader ---
        sig=$(wh_sign '{"k":1}')
        http_st=$(wh_post "${base_url}/api/conduit/webhook/nosig" "${out_file}" \
            '{"k":1}' -H "X-Signature-256: v1=${sig}")
        if [[ "${http_st}" == "401" ]] && jq -e '.error == "invalid_signature"' \
            "${out_file}" >/dev/null 2>&1; then
            record_case "${result_file}" "nosig_401" 1
        else
            record_case "${result_file}" "nosig_401" 0
            echo "NOSIG_HTTP=${http_st}" >> "${result_file}"
        fi

        # --- 200: stripe-style v1= token, params echoed by Api.Echo ---
        body='{"hello":"world"}'
        sig=$(wh_sign "${body}")
        signed_webhook_case "${result_file}" "echo_v1_200" \
            "${base_url}/api/conduit/webhook/echo" "${body}" "echo" \
            "X-Signature-256" "v1=${sig}" "${description}"

        # --- 200: sha256= prefixed token ---
        body='{"prefixed":true}'
        sig=$(wh_sign "${body}")
        signed_webhook_case "${result_file}" "echo_prefixed_200" \
            "${base_url}/api/conduit/webhook/echo" "${body}" "echo" \
            "X-Signature-256" "sha256=${sig}" "${description}"

        # --- 200: bare hex token ---
        body='{"bare":true}'
        sig=$(wh_sign "${body}")
        signed_webhook_case "${result_file}" "echo_bare_200" \
            "${base_url}/api/conduit/webhook/echo" "${body}" "echo" \
            "X-Signature-256" "${sig}" "${description}"

        # --- 200: empty body signed over the empty payload ---
        sig=$(wh_sign "")
        signed_webhook_case "${result_file}" "echo_empty_200" \
            "${base_url}/api/conduit/webhook/echo" "" "echo" \
            "X-Signature-256" "${sig}" "${description}"

        # --- 200: sha256-timestamp algorithm (signed payload is "<ts>.<body>") ---
        ts=$("${DATE}" +%s)
        body='{"ts":true}'
        sig=$(wh_sign "${ts}.${body}")
        signed_webhook_case "${result_file}" "ts_200" \
            "${base_url}/api/conduit/webhook/ts" "${body}" "ts" \
            "X-Timestamp-Signature" "t=${ts},v1=${sig}" "${description}"

        # --- 401: sha256-timestamp hook whose header carries no t= ---
        sig=$(wh_sign '{"k":1}')
        http_st=$(wh_post "${base_url}/api/conduit/webhook/ts" "${out_file}" \
            '{"k":1}' -H "X-Timestamp-Signature: v1=${sig}")
        if [[ "${http_st}" == "401" ]] && jq -e '.error == "invalid_signature"' \
            "${out_file}" >/dev/null 2>&1; then
            record_case "${result_file}" "ts_no_timestamp_401" 1
        else
            record_case "${result_file}" "ts_no_timestamp_401" 0
            echo "TS_NO_TIMESTAMP_HTTP=${http_st}" >> "${result_file}"
        fi

        # --- 405: GET on the /webhook/{hook} alias ---
        http_st=$(wh_get "${base_url}/webhook/echo" "${out_file}")
        if [[ "${http_st}" == "405" ]] && jq -e '.error == "method_not_allowed"' \
            "${out_file}" >/dev/null 2>&1; then
            record_case "${result_file}" "alias_get_405" 1
        else
            record_case "${result_file}" "alias_get_405" 0
            echo "ALIAS_GET_HTTP=${http_st}" >> "${result_file}"
        fi

        # --- 200: signed POST on the /webhook/{hook} alias ---
        body='{"alias":true}'
        sig=$(wh_sign "${body}")
        signed_webhook_case "${result_file}" "alias_200" \
            "${base_url}/webhook/echo" "${body}" "echo" \
            "X-Signature-256" "v1=${sig}" "${description}"

        # --- 404: hook maps to a script that does not exist ---
        body='{"k":1}'
        sig=$(wh_sign "${body}")
        http_st=""
        local try=1
        while [[ "${try}" -le 3 ]]; do
            http_st=$(wh_post "${base_url}/api/conduit/webhook/missing" "${out_file}" \
                "${body}" -H "X-Signature-256: v1=${sig}")
            if [[ "${http_st}" == "404" || ( "${http_st}" != "000" && "${http_st}" != 5* ) ]]; then
                break
            fi
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" \
                "INFO ${description}: missing_script HTTP ${http_st} (try ${try}/3)"
            sleep 1
            try=$(( try + 1 ))
        done
        if [[ "${http_st}" == "404" ]] && jq -e '.error == "script_not_found"' \
            "${out_file}" >/dev/null 2>&1; then
            record_case "${result_file}" "missing_script_404" 1
        else
            record_case "${result_file}" "missing_script_404" 0
            echo "MISSING_SCRIPT_HTTP=${http_st}" >> "${result_file}"
        fi

        # --- 500: reserved _hydrogen smuggled in through a header value ---
        # collect_headers() copies X-Request-Id into the params JSON, and the
        # dumped params then literally contain the reserved key "_hydrogen".
        body='{"k":1}'
        sig=$(wh_sign "${body}")
        http_st=$(wh_post "${base_url}/api/conduit/webhook/echo" "${out_file}" \
            "${body}" -H "X-Signature-256: v1=${sig}" \
            -H "X-Request-Id: _hydrogen")
        if [[ "${http_st}" == "500" ]] && jq -e '.error == "internal_error"' \
            "${out_file}" >/dev/null 2>&1; then
            record_case "${result_file}" "reserved_500" 1
        else
            record_case "${result_file}" "reserved_500" 0
            echo "RESERVED_HTTP=${http_st}" >> "${result_file}"
        fi

        # --- 500 (SQLite only): body larger than API_MAX_POST_SIZE ---
        if [[ "${engine_key}" == "sqlite" ]]; then
            local big_file="${work_dir}/oversize_body.bin"
            if head -c 11534336 /dev/zero | tr '\0' 'a' > "${big_file}"; then
                http_st=$(group40_curl_to "${out_file}" -X POST \
                    -H "Content-Type: application/json" \
                    --data-binary "@${big_file}" \
                    "${base_url}/api/conduit/webhook/echo")
                if [[ "${http_st}" == "500" ]] || "${GREP}" -q -F "POST body too large" \
                    "${log_file}" 2>/dev/null; then
                    record_case "${result_file}" "oversize_500" 1
                else
                    record_case "${result_file}" "oversize_500" 0
                    echo "OVERSIZE_HTTP=${http_st}" >> "${result_file}"
                fi
            fi
        fi
    fi

    echo "ENGINE_COMPLETE=1" >> "${result_file}"
    # shellcheck disable=SC2310 # Shutdown best-effort at end of engine run
    scripting_shutdown_instance "${hydrogen_pid}" "${SHUTDOWN_TIMEOUT}" || true
    return 0
}

analyze_engine() {
    local result_file="$1"
    local description="$2"
    local log_file="$3"

    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}: conduit webhook"
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "log ..${log_file##*/}"

    if [[ ! -f "${result_file}" ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${description}: no result file"
        EXIT_CODE=1
        return
    fi

    if "${GREP}" -q "^STARTUP_FAILED=" "${result_file}" 2>/dev/null; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${description}: startup failed"
        EXIT_CODE=1
        return
    fi
    if "${GREP}" -q "^NOT_READY=" "${result_file}" 2>/dev/null; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${description}: not READY within timeout"
        EXIT_CODE=1
        return
    fi

    local pass_n fail_n min_cases
    pass_n=$("${GREP}" -c "^CASE_PASS=" "${result_file}" 2>/dev/null || echo 0)
    fail_n=$("${GREP}" -c "^CASE_FAIL=" "${result_file}" 2>/dev/null || echo 0)
    pass_n=${pass_n//[^0-9]/}
    fail_n=${fail_n//[^0-9]/}
    pass_n=${pass_n:-0}
    fail_n=${fail_n:-0}
    min_cases=$("${GREP}" "^MIN_CASES=" "${result_file}" 2>/dev/null | head -1 | cut -d= -f2 || true)
    min_cases=${min_cases:-18}

    if [[ "${fail_n}" -eq 0 && "${pass_n}" -ge "${min_cases}" ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 \
            "${description}: ${pass_n} cases passed (method/hook/signature errors, HMAC variants, alias, reserved, disabled)"
        PASS_COUNT=$(( PASS_COUNT + 1 ))
    else
        local fails
        fails=$("${GREP}" "^CASE_FAIL=" "${result_file}" 2>/dev/null | cut -d= -f2 | tr '\n' ',' || true)
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 \
            "${description}: pass=${pass_n} fail=${fail_n} (${fails%,})"
        EXIT_CODE=1
    fi
}

# ---------------------------------------------------------------------------
# Pre-flight
# ---------------------------------------------------------------------------

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Locate Hydrogen Binary"
HYDROGEN_BIN=''
HYDROGEN_BIN_BASE=''
# shellcheck disable=SC2310 # Continue with EXIT_CODE=1 when binary missing
if find_hydrogen_binary "${PROJECT_DIR}"; then
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Using Hydrogen binary: ${HYDROGEN_BIN_BASE}"
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Hydrogen binary found and validated"
    PASS_COUNT=$(( PASS_COUNT + 1 ))
else
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Failed to find Hydrogen binary"
    EXIT_CODE=1
fi

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Validate Environment Variables"
env_vars_valid=true
for v in PAYLOAD_KEY HYDROGEN_DEMO_USER_NAME HYDROGEN_DEMO_JWT_KEY; do
    if [[ -z "${!v:-}" ]]; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "ERROR: ${v} is not set"
        env_vars_valid=false
    fi
done
if [[ "${env_vars_valid}" = true ]]; then
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Required environment variables are set"
    PASS_COUNT=$(( PASS_COUNT + 1 ))
else
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Missing required env vars"
    EXIT_CODE=1
fi

config_valid=true
for test_config in "${!WEBHOOK_TEST_CONFIGS[@]}"; do
    IFS=':' read -r config_file log_suffix _ description variant <<< "${WEBHOOK_TEST_CONFIGS[${test_config}]}"
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Validate Configuration File: ${test_config}"
    # shellcheck disable=SC2310 # Mark config_valid false without aborting the loop
    if validate_config_file "${config_file}"; then
        port=$(get_webserver_port "${config_file}")
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${description} will use port: ${port}"
    else
        config_valid=false
        EXIT_CODE=1
    fi
done

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Validate Configuration Files"
if [[ "${config_valid}" = true ]]; then
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "All ${#WEBHOOK_TEST_CONFIGS[@]} configuration files validated"
    PASS_COUNT=$(( PASS_COUNT + 1 ))
else
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Configuration file validation failed"
    EXIT_CODE=1
fi

# ---------------------------------------------------------------------------
# Parallel engines
# ---------------------------------------------------------------------------

if [[ "${EXIT_CODE}" -eq 0 ]]; then
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Running conduit webhook in parallel"

    for test_config in "${!WEBHOOK_TEST_CONFIGS[@]}"; do
        # shellcheck disable=SC2312 # Job control with wc -l is standard practice
        while (( $(jobs -r | wc -l) >= CORES )); do
            wait -n || true
        done

        IFS=':' read -r config_file log_suffix engine_key description variant <<< "${WEBHOOK_TEST_CONFIGS[${test_config}]}"
        log_file="${LOGS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}_${log_suffix}.log"
        result_file="${LOG_PREFIX}${TIMESTAMP}_${log_suffix}.result"

        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Starting parallel run: ${test_config} (${description})"
        run_engine "${config_file}" "${log_file}" "${result_file}" "${HYDROGEN_BIN}" \
            "${engine_key}" "${description}" "${variant}" &
        PARALLEL_PIDS+=($!)
    done

    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Waiting for ${#WEBHOOK_TEST_CONFIGS[@]} parallel runs"
    for pid in "${PARALLEL_PIDS[@]}"; do
        wait "${pid}" || true
    done
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "All parallel runs completed"

    successful=0
    for test_config in "${!WEBHOOK_TEST_CONFIGS[@]}"; do
        IFS=':' read -r config_file log_suffix engine_key description variant <<< "${WEBHOOK_TEST_CONFIGS[${test_config}]}"
        log_file="${LOGS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}_${log_suffix}.log"
        result_file="${LOG_PREFIX}${TIMESTAMP}_${log_suffix}.result"
        print_marker "${TEST_NUMBER}" "${TEST_COUNTER}"
        before=${PASS_COUNT}
        analyze_engine "${result_file}" "${description}" "${log_file}"
        if [[ "${PASS_COUNT}" -gt "${before}" ]]; then
            successful=$((successful + 1))
        fi
    done

    print_marker "${TEST_NUMBER}" "${TEST_COUNTER}"
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" \
        "Summary: ${successful}/${#WEBHOOK_TEST_CONFIGS[@]} instances ok"
else
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Skipping conduit webhook tests due to prerequisite failures"
    EXIT_CODE=1
fi

print_test_completion "${TEST_NAME}" "${TEST_ABBR}" "${TEST_NUMBER}" "${TEST_VERSION}"
${ORCHESTRATION:-false} && return "${EXIT_CODE}" || exit "${EXIT_CODE}"
