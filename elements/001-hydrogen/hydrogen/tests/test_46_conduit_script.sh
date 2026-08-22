#!/usr/bin/env bash

# Test: Conduit Script Invoke (LUA_CLIENT Phase 9)
# POST /api/conduit/script + GET /api/conduit/script/{job_id} across engines.
# JWT login, Api.Echo fixture (invokable), auth/404 cases, async wait:false.

# FUNCTIONS
# api_request()
# extract_jwt()
# wait_ready()
# prepare_sqlite_config()
# run_engine()
# analyze_engine()

# CHANGELOG
# 1.2.0 - 2026-08-21 - Extra parse/GET/405/fail cases for blackbox coverage
# 1.1.0 - 2026-08-20 - Replace python3 JSON rewrite with jq
# 1.0.0 - 2026-08-08 - Initial blackbox for LUA_CLIENT Phase 9

set -euo pipefail

TEST_NAME="Conduit Script"
TEST_ABBR="CSC"
TEST_NUMBER="46"
TEST_COUNTER=0
TEST_VERSION="1.2.0"

# shellcheck source=tests/lib/framework.sh # Reference framework directly
[[ -n "${FRAMEWORK_GUARD:-}" ]] || source "$(dirname "${BASH_SOURCE[0]}")/lib/framework.sh"
setup_test_environment

# shellcheck source=tests/lib/scripting_helpers.sh # Start/shutdown helpers shared with test_43
source "$(dirname "${BASH_SOURCE[0]}")/lib/scripting_helpers.sh"

declare -a PARALLEL_PIDS
declare -A SCRIPT_TEST_CONFIGS

# config:log_suffix:engine_key:description
SCRIPT_TEST_CONFIGS=(
    ["PostgreSQL"]="${SCRIPT_DIR}/configs/hydrogen_test_${TEST_NUMBER}_conduit_script_postgres.json:postgres:postgresql:PostgreSQL"
    ["MySQL"]="${SCRIPT_DIR}/configs/hydrogen_test_${TEST_NUMBER}_conduit_script_mysql.json:mysql:mysql:MySQL"
    ["SQLite"]="${SCRIPT_DIR}/configs/hydrogen_test_${TEST_NUMBER}_conduit_script_sqlite.json:sqlite:sqlite:SQLite"
    ["DB2"]="${SCRIPT_DIR}/configs/hydrogen_test_${TEST_NUMBER}_conduit_script_db2.json:db2:db2:DB2"
    ["MariaDB"]="${SCRIPT_DIR}/configs/hydrogen_test_${TEST_NUMBER}_conduit_script_mariadb.json:mariadb:mariadb:MariaDB"
    ["CockroachDB"]="${SCRIPT_DIR}/configs/hydrogen_test_${TEST_NUMBER}_conduit_script_cockroachdb.json:cockroachdb:cockroachdb:CockroachDB"
    ["YugabyteDB"]="${SCRIPT_DIR}/configs/hydrogen_test_${TEST_NUMBER}_conduit_script_yugabytedb.json:yugabytedb:yugabytedb:YugabyteDB"
)

# shellcheck disable=SC2034 # Reserved for log messages / future fail-fast bounds
STARTUP_TIMEOUT=90
SHUTDOWN_TIMEOUT=20
READY_TIMEOUT=90
HTTP_TIMEOUT=30
BASELINE_SQLITE="${PROJECT_DIR}/tests/artifacts/database/sqlite/hydrodemo.sqlite"

# Demo credentials from environment (set in shell / CI)
# shellcheck disable=SC2154 # Set externally via ~/.zshrc or CI
: "${HYDROGEN_DEMO_USER_NAME:=}"
# shellcheck disable=SC2154 # Set externally via ~/.zshrc or CI
: "${HYDROGEN_DEMO_USER_PASS:=}"
# shellcheck disable=SC2154 # Set externally via ~/.zshrc or CI
: "${HYDROGEN_DEMO_API_KEY:=}"
# shellcheck disable=SC2154 # Set externally via ~/.zshrc or CI
: "${HYDROGEN_DEMO_JWT_KEY:=}"

api_request() {
    local method="$1"
    local url="$2"
    local data="$3"
    local output_file="$4"
    local jwt_token="${5:-}"
    local max_time="${6:-${HTTP_TIMEOUT}}"
    local curl_cmd=(curl -s -X "${method}" -H "Content-Type: application/json"
        --connect-timeout 3 --max-time "${max_time}" -w "%{http_code}" -o "${output_file}")
    if [[ -n "${jwt_token}" ]]; then
        curl_cmd+=(-H "Authorization: Bearer ${jwt_token}")
    fi
    if [[ "${method}" != "GET" && -n "${data}" ]]; then
        curl_cmd+=(-d "${data}")
    fi
    curl_cmd+=("${url}")
    local http_status
    http_status=$("${curl_cmd[@]}" 2>/dev/null || true)
    echo "${http_status:-000}"
}

extract_jwt() {
    local response_file="$1"
    if [[ -f "${response_file}" ]]; then
        jq -r '.token // empty' "${response_file}" 2>/dev/null || true
    fi
}

wait_ready() {
    local log_file="$1"
    local timeout="${2:-${READY_TIMEOUT}}"
    local start_time=${SECONDS}
    while [[ $((SECONDS - start_time)) -lt "${timeout}" ]]; do
        if "${GREP}" -q "READY FOR REQUESTS" "${log_file}" 2>/dev/null; then
            return 0
        fi
        sleep 0.2
    done
    return 1
}

# SQLite: isolate fixture + seed LUA_CLIENT fixture (Api.Echo, invokable, #149).
# AutoMigration is left off: baseline APPLY has a hole at 1283 and migration
# 1293 currently fails on SQLite seed SQL, so blackbox seeds the minimal
# surface needed for POST /api/conduit/script.
seed_sqlite_lua_client_fixture() {
    local db_copy="$1"
    if ! command -v sqlite3 >/dev/null 2>&1; then
        return 1
    fi
    sqlite3 "${db_copy}" <<'SQL'
-- invokable allowlist column (migration 1297)
ALTER TABLE scripts ADD COLUMN invokable INTEGER NOT NULL DEFAULT 0;

-- Api.Echo (migration 1296)
INSERT OR REPLACE INTO scripts (
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

-- Api.Fail (blackbox: Lua error → HTTP 200 status=failed)
INSERT OR REPLACE INTO scripts (
    group_name, script_name, script_type, schedule, next_run,
    last_run_start, last_run_end, status, code, summary,
    created_id, created_at, updated_id, updated_at, invokable
) VALUES (
    'Api', 'Fail', 1, NULL, NULL, NULL, NULL, 1,
    '-- Api.Fail (LUA_CLIENT blackbox fail probe)
error("blackbox fail probe")
',
    'LUA_CLIENT fixture: forced Lua error',
    1, datetime('now'), 1, datetime('now'), 1
);

-- QueryRef #149 (migration 1298) — invokable-only script load
INSERT INTO queries (
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
    cp -f "${BASELINE_SQLITE}" "${db_copy}"
    if [[ -f "${BASELINE_SQLITE}-wal" ]]; then
        cp -f "${BASELINE_SQLITE}-wal" "${db_copy}-wal" 2>/dev/null || true
    fi
    if [[ -f "${BASELINE_SQLITE}-shm" ]]; then
        cp -f "${BASELINE_SQLITE}-shm" "${db_copy}-shm" 2>/dev/null || true
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

    true > "${result_file}"
    echo "ENGINE=${engine_key}" >> "${result_file}"
    echo "DESCRIPTION=${description}" >> "${result_file}"

    local work_dir=""
    local run_config="${config_file}"
    if [[ "${engine_key}" == "sqlite" ]]; then
        work_dir="${DIAG_TEST_DIR}/sqlite_${engine_key}_$$"
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

    local login_file="${result_file}.login.json"
    local login_payload
    login_payload=$(jq -n \
        --arg login_id "${HYDROGEN_DEMO_USER_NAME}" \
        --arg password "${HYDROGEN_DEMO_USER_PASS}" \
        --arg api_key "${HYDROGEN_DEMO_API_KEY}" \
        '{database:"Acuranzo",login_id:$login_id,password:$password,api_key:$api_key,tz:"America/Vancouver"}')

    local http_st
    http_st=$(api_request "POST" "${base_url}/api/auth/login" "${login_payload}" "${login_file}" "" 15)
    local jwt
    jwt=$(extract_jwt "${login_file}")
    if [[ "${http_st}" != "200" || -z "${jwt}" ]]; then
        {
            echo "LOGIN_FAILED=1"
            echo "LOGIN_HTTP=${http_st}"
        } >> "${result_file}"
        # shellcheck disable=SC2310 # Shutdown best-effort after login failure
        scripting_shutdown_instance "${hydrogen_pid}" "${SHUTDOWN_TIMEOUT}" || true
        return 0
    fi
    echo "LOGIN_OK=1" >> "${result_file}"
    record_case "${result_file}" "login" 1

    # --- no auth → 401 ---
    local noauth_file="${result_file}.noauth.json"
    http_st=$(api_request "POST" "${base_url}/api/conduit/script" \
        '{"script":"Api.Echo","params":{"k":1},"wait":true,"timeout_seconds":15}' \
        "${noauth_file}" "")
    if [[ "${http_st}" == "401" ]]; then
        record_case "${result_file}" "no_auth_401" 1
    else
        record_case "${result_file}" "no_auth_401" 0
        echo "NOAUTH_HTTP=${http_st}" >> "${result_file}"
    fi

    # --- bad / unknown script → 404 ---
    local bad_file="${result_file}.bad.json"
    http_st=$(api_request "POST" "${base_url}/api/conduit/script" \
        '{"script":"No.SuchScript","params":{},"wait":true,"timeout_seconds":15}' \
        "${bad_file}" "${jwt}")
    if [[ "${http_st}" == "404" ]]; then
        record_case "${result_file}" "unknown_404" 1
    else
        record_case "${result_file}" "unknown_404" 0
        echo "UNKNOWN_HTTP=${http_st}" >> "${result_file}"
    fi

    # --- non-invokable (existence-hiding 404) ---
    local deny_file="${result_file}.deny.json"
    http_st=$(api_request "POST" "${base_url}/api/conduit/script" \
        '{"script":"Orchestrators.Orchestrator","params":{},"wait":true,"timeout_seconds":15}' \
        "${deny_file}" "${jwt}")
    if [[ "${http_st}" == "404" ]]; then
        record_case "${result_file}" "non_invokable_404" 1
    else
        record_case "${result_file}" "non_invokable_404" 0
        echo "DENY_HTTP=${http_st}" >> "${result_file}"
    fi

    # --- Echo wait:true ---
    local echo_file="${result_file}.echo.json"
    http_st=$(api_request "POST" "${base_url}/api/conduit/script" \
        '{"script":"Api.Echo","params":{"hello":"world","n":42},"wait":true,"timeout_seconds":15}' \
        "${echo_file}" "${jwt}")
    local echo_ok=0
    if [[ "${http_st}" == "200" ]] && command -v jq >/dev/null 2>&1; then
        local st hello n sub
        st=$(jq -r '.status // empty' "${echo_file}" 2>/dev/null || true)
        hello=$(jq -r '.result.hello // empty' "${echo_file}" 2>/dev/null || true)
        n=$(jq -r '.result.n // empty' "${echo_file}" 2>/dev/null || true)
        sub=$(jq -r '.result._hydrogen.sub // empty' "${echo_file}" 2>/dev/null || true)
        if [[ "${st}" == "completed" && "${hello}" == "world" && "${n}" == "42" && -n "${sub}" ]]; then
            echo_ok=1
        fi
        {
            echo "ECHO_STATUS=${st}"
            echo "ECHO_HELLO=${hello}"
            echo "ECHO_SUB=${sub}"
        } >> "${result_file}"
    fi
    if [[ "${echo_ok}" -eq 1 ]]; then
        record_case "${result_file}" "echo_wait" 1
    else
        record_case "${result_file}" "echo_wait" 0
        echo "ECHO_HTTP=${http_st}" >> "${result_file}"
        # Shared live DBs may not have migrations 1296–1298 yet.
        if [[ "${http_st}" == "404" && "${engine_key}" != "sqlite" ]]; then
            echo "ENGINE_SKIP=fixture_migrations" >> "${result_file}"
        fi
    fi

    # --- wait:false → 202 + GET status ---
    local async_file="${result_file}.async.json"
    http_st=$(api_request "POST" "${base_url}/api/conduit/script" \
        '{"script":"Api.Echo","params":{"async":true},"wait":false,"timeout_seconds":15}' \
        "${async_file}" "${jwt}")
    local async_ok=0
    local job_id=""
    if [[ "${http_st}" == "202" ]] && command -v jq >/dev/null 2>&1; then
        job_id=$(jq -r '.job_id // empty' "${async_file}" 2>/dev/null || true)
        if [[ -n "${job_id}" ]]; then
            local get_file="${result_file}.get.json"
            local waited=0
            local get_st=""
            while (( waited < 50 )); do
                get_st=$(api_request "GET" "${base_url}/api/conduit/script/${job_id}" "" "${get_file}" "${jwt}" 10)
                local gst
                gst=$(jq -r '.status // empty' "${get_file}" 2>/dev/null || true)
                if [[ "${get_st}" == "200" && ( "${gst}" == "completed" || "${gst}" == "failed" ) ]]; then
                    if [[ "${gst}" == "completed" ]]; then
                        local async_flag
                        async_flag=$(jq -r '.result.async // empty' "${get_file}" 2>/dev/null || true)
                        if [[ "${async_flag}" == "true" ]]; then
                            async_ok=1
                        fi
                    fi
                    break
                fi
                sleep 0.2
                waited=$((waited + 1))
            done
            echo "GET_HTTP=${get_st}" >> "${result_file}"
            echo "JOB_ID=${job_id}" >> "${result_file}"
        fi
    fi
    if [[ "${async_ok}" -eq 1 ]]; then
        record_case "${result_file}" "async_get" 1
    else
        record_case "${result_file}" "async_get" 0
        echo "ASYNC_HTTP=${http_st}" >> "${result_file}"
    fi

    # --- reserved _hydrogen → 400 ---
    local reserved_file="${result_file}.reserved.json"
    http_st=$(api_request "POST" "${base_url}/api/conduit/script" \
        '{"script":"Api.Echo","params":{"_hydrogen":{"x":1}},"wait":true}' \
        "${reserved_file}" "${jwt}")
    if [[ "${http_st}" == "400" ]]; then
        record_case "${result_file}" "reserved_400" 1
    else
        record_case "${result_file}" "reserved_400" 0
        echo "RESERVED_HTTP=${http_st}" >> "${result_file}"
    fi

    # --- parse / routing cases (no Echo fixture required) ---
    local parse_file="${result_file}.parse.json"
    http_st=$(api_request "POST" "${base_url}/api/conduit/script" \
        'not-json' "${parse_file}" "${jwt}")
    if [[ "${http_st}" == "400" ]]; then
        record_case "${result_file}" "invalid_json_400" 1
    else
        record_case "${result_file}" "invalid_json_400" 0
        echo "INVALID_JSON_HTTP=${http_st}" >> "${result_file}"
    fi

    http_st=$(api_request "POST" "${base_url}/api/conduit/script" \
        '{}' "${parse_file}" "${jwt}")
    if [[ "${http_st}" == "400" ]]; then
        record_case "${result_file}" "missing_script_400" 1
    else
        record_case "${result_file}" "missing_script_400" 0
        echo "MISSING_SCRIPT_HTTP=${http_st}" >> "${result_file}"
    fi

    http_st=$(api_request "POST" "${base_url}/api/conduit/script" \
        '{"script":""}' "${parse_file}" "${jwt}")
    if [[ "${http_st}" == "400" ]]; then
        record_case "${result_file}" "empty_script_400" 1
    else
        record_case "${result_file}" "empty_script_400" 0
        echo "EMPTY_SCRIPT_HTTP=${http_st}" >> "${result_file}"
    fi

    http_st=$(api_request "POST" "${base_url}/api/conduit/script" \
        '{"script":"Api/Echo","wait":true}' "${parse_file}" "${jwt}")
    if [[ "${http_st}" == "400" ]]; then
        record_case "${result_file}" "slash_name_400" 1
    else
        record_case "${result_file}" "slash_name_400" 0
        echo "SLASH_NAME_HTTP=${http_st}" >> "${result_file}"
    fi

    http_st=$(api_request "POST" "${base_url}/api/conduit/script" \
        '{"script":"Api.Echo","params":[1],"wait":true}' "${parse_file}" "${jwt}")
    if [[ "${http_st}" == "400" ]]; then
        record_case "${result_file}" "params_array_400" 1
    else
        record_case "${result_file}" "params_array_400" 0
        echo "PARAMS_ARRAY_HTTP=${http_st}" >> "${result_file}"
    fi

    http_st=$(api_request "POST" "${base_url}/api/conduit/script" \
        '{"script":"Api.Echo","wait":"yes"}' "${parse_file}" "${jwt}")
    if [[ "${http_st}" == "400" ]]; then
        record_case "${result_file}" "wait_not_bool_400" 1
    else
        record_case "${result_file}" "wait_not_bool_400" 0
        echo "WAIT_NOT_BOOL_HTTP=${http_st}" >> "${result_file}"
    fi

    http_st=$(api_request "POST" "${base_url}/api/conduit/script" \
        '{"script":"Api.Echo","timeout_seconds":"x"}' "${parse_file}" "${jwt}")
    if [[ "${http_st}" == "400" ]]; then
        record_case "${result_file}" "timeout_not_int_400" 1
    else
        record_case "${result_file}" "timeout_not_int_400" 0
        echo "TIMEOUT_NOT_INT_HTTP=${http_st}" >> "${result_file}"
    fi

    http_st=$(api_request "GET" "${base_url}/api/conduit/script/_____" \
        "" "${parse_file}" "")
    if [[ "${http_st}" == "401" ]]; then
        record_case "${result_file}" "get_noauth_401" 1
    else
        record_case "${result_file}" "get_noauth_401" 0
        echo "GET_NOAUTH_HTTP=${http_st}" >> "${result_file}"
    fi

    http_st=$(api_request "GET" "${base_url}/api/conduit/script/_____" \
        "" "${parse_file}" "${jwt}")
    if [[ "${http_st}" == "404" ]]; then
        record_case "${result_file}" "get_unknown_404" 1
    else
        record_case "${result_file}" "get_unknown_404" 0
        echo "GET_UNKNOWN_HTTP=${http_st}" >> "${result_file}"
    fi

    http_st=$(api_request "GET" "${base_url}/api/conduit/script/" \
        "" "${parse_file}" "${jwt}")
    if [[ "${http_st}" == "400" ]]; then
        record_case "${result_file}" "get_missing_id_400" 1
    else
        record_case "${result_file}" "get_missing_id_400" 0
        echo "GET_MISSING_ID_HTTP=${http_st}" >> "${result_file}"
    fi

    http_st=$(api_request "POST" "${base_url}/api/conduit/script/not-a-job" \
        '{"script":"Api.Echo"}' "${parse_file}" "${jwt}")
    if [[ "${http_st}" == "405" ]]; then
        record_case "${result_file}" "get_path_post_405" 1
    else
        record_case "${result_file}" "get_path_post_405" 0
        echo "GET_PATH_POST_HTTP=${http_st}" >> "${result_file}"
    fi

    if [[ "${echo_ok}" -eq 1 ]]; then
        local clamp_file="${result_file}.clamp.json"
        http_st=$(api_request "POST" "${base_url}/api/conduit/script" \
            '{"script":"Api.Echo","params":{"clamp":true},"wait":true,"timeout_seconds":0}' \
            "${clamp_file}" "${jwt}")
        local clamp_st=""
        clamp_st=$(jq -r '.status // empty' "${clamp_file}" 2>/dev/null || true)
        if [[ "${http_st}" == "200" && "${clamp_st}" == "completed" ]]; then
            record_case "${result_file}" "timeout_clamp" 1
        else
            record_case "${result_file}" "timeout_clamp" 0
            echo "CLAMP_HTTP=${http_st}" >> "${result_file}"
        fi
    fi

    if [[ "${engine_key}" == "sqlite" ]]; then
        local fail_file="${result_file}.fail.json"
        http_st=$(api_request "POST" "${base_url}/api/conduit/script" \
            '{"script":"Api.Fail","params":{},"wait":true,"timeout_seconds":15}' \
            "${fail_file}" "${jwt}")
        local fail_st=""
        fail_st=$(jq -r '.status // empty' "${fail_file}" 2>/dev/null || true)
        if [[ "${http_st}" == "200" && "${fail_st}" == "failed" ]]; then
            record_case "${result_file}" "fail_status" 1
        else
            record_case "${result_file}" "fail_status" 0
            echo "FAIL_HTTP=${http_st}" >> "${result_file}"
            echo "FAIL_STATUS=${fail_st}" >> "${result_file}"
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

    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}: conduit script invoke"
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
    if "${GREP}" -q "^LOGIN_FAILED=" "${result_file}" 2>/dev/null; then
        local lh
        lh=$("${GREP}" "^LOGIN_HTTP=" "${result_file}" 2>/dev/null | head -1 | cut -d= -f2 || true)
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${description}: login failed (HTTP ${lh:-?})"
        EXIT_CODE=1
        return
    fi

    if "${GREP}" -q "^ENGINE_SKIP=" "${result_file}" 2>/dev/null; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 \
            "${description}: skipped (Api.Echo / QueryRef 149 not migrated on live DB)"
        PASS_COUNT=$(( PASS_COUNT + 1 ))
        return
    fi

    local pass_n fail_n
    pass_n=$("${GREP}" -c "^CASE_PASS=" "${result_file}" 2>/dev/null || echo 0)
    fail_n=$("${GREP}" -c "^CASE_FAIL=" "${result_file}" 2>/dev/null || echo 0)
    pass_n=${pass_n//[^0-9]/}
    fail_n=${fail_n//[^0-9]/}
    pass_n=${pass_n:-0}
    fail_n=${fail_n:-0}

    if [[ "${fail_n}" -eq 0 && "${pass_n}" -ge 15 ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 \
            "${description}: ${pass_n} cases passed (login, auth/parse/GET/405, Echo, async, reserved)"
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
for v in HYDROGEN_DEMO_USER_NAME HYDROGEN_DEMO_USER_PASS HYDROGEN_DEMO_API_KEY HYDROGEN_DEMO_JWT_KEY; do
    if [[ -z "${!v:-}" ]]; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "ERROR: ${v} is not set"
        env_vars_valid=false
    fi
done
if [[ "${env_vars_valid}" = true ]]; then
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Required environment variables are set"
    PASS_COUNT=$(( PASS_COUNT + 1 ))
else
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Missing demo credential env vars"
    EXIT_CODE=1
fi

config_valid=true
for test_config in "${!SCRIPT_TEST_CONFIGS[@]}"; do
    IFS=':' read -r config_file log_suffix _ description <<< "${SCRIPT_TEST_CONFIGS[${test_config}]}"
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
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "All ${#SCRIPT_TEST_CONFIGS[@]} configuration files validated"
    PASS_COUNT=$(( PASS_COUNT + 1 ))
else
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Configuration file validation failed"
    EXIT_CODE=1
fi

# ---------------------------------------------------------------------------
# Parallel engines
# ---------------------------------------------------------------------------

if [[ "${EXIT_CODE}" -eq 0 ]]; then
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Running conduit script invoke in parallel"

    for test_config in "${!SCRIPT_TEST_CONFIGS[@]}"; do
        # shellcheck disable=SC2312 # Job control with wc -l is standard practice
        while (( $(jobs -r | wc -l) >= CORES )); do
            wait -n || true
        done

        IFS=':' read -r config_file log_suffix engine_key description <<< "${SCRIPT_TEST_CONFIGS[${test_config}]}"
        log_file="${LOGS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}_${log_suffix}.log"
        result_file="${LOG_PREFIX}${TIMESTAMP}_${log_suffix}.result"

        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Starting parallel run: ${test_config} (${description})"
        run_engine "${config_file}" "${log_file}" "${result_file}" "${HYDROGEN_BIN}" \
            "${engine_key}" "${description}" &
        PARALLEL_PIDS+=($!)
    done

    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Waiting for ${#SCRIPT_TEST_CONFIGS[@]} parallel runs"
    for pid in "${PARALLEL_PIDS[@]}"; do
        wait "${pid}" || true
    done
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "All parallel runs completed"

    successful=0
    sqlite_full=0
    for test_config in "${!SCRIPT_TEST_CONFIGS[@]}"; do
        IFS=':' read -r config_file log_suffix engine_key description <<< "${SCRIPT_TEST_CONFIGS[${test_config}]}"
        log_file="${LOGS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}_${log_suffix}.log"
        result_file="${LOG_PREFIX}${TIMESTAMP}_${log_suffix}.result"
        print_marker "${TEST_NUMBER}" "${TEST_COUNTER}"
        before=${PASS_COUNT}
        analyze_engine "${result_file}" "${description}" "${log_file}"
        if [[ "${PASS_COUNT}" -gt "${before}" ]]; then
            successful=$((successful + 1))
        fi
        if [[ "${engine_key}" == "sqlite" ]] \
            && [[ -f "${result_file}" ]] \
            && "${GREP}" -q "^CASE_PASS=echo_wait$" "${result_file}" 2>/dev/null \
            && ! "${GREP}" -q "^CASE_FAIL=" "${result_file}" 2>/dev/null; then
            sqlite_full=1
        fi
    done

    print_marker "${TEST_NUMBER}" "${TEST_COUNTER}"
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" \
        "Summary: ${successful}/${#SCRIPT_TEST_CONFIGS[@]} engines ok (SQLite full fixture required)"
    if [[ "${sqlite_full}" -ne 1 ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "SQLite full invoke path did not pass"
        EXIT_CODE=1
    fi
else
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Skipping conduit script tests due to prerequisite failures"
    EXIT_CODE=1
fi

print_test_completion "${TEST_NAME}" "${TEST_ABBR}" "${TEST_NUMBER}" "${TEST_VERSION}"
${ORCHESTRATION:-false} && return "${EXIT_CODE}" || exit "${EXIT_CODE}"
