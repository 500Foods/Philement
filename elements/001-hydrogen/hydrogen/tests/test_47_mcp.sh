#!/usr/bin/env bash

# Test: MCP Streamable HTTP (MCP Phase 13)
# Real hydrogen, Hydrogen JWT, Lua Mcp.Server: initialize / tools / call / session.

# FUNCTIONS
# api_request()
# mcp_http()
# extract_jwt()
# header_value()
# wait_ready()
# b64url_encode()
# b64url_decode()
# mint_hijack_jwt()
# token_hash()
# prepare_sqlite_config()
# record_case()
# run_engine()
# analyze_engine()
# run_disabled()

# CHANGELOG
# 1.0.1 - 2026-08-27 - Skip comment: seeds expected applied (1283 hole closed)
# 1.0.0 - 2026-08-27 - Initial blackbox for MCP Phase 13 (Test 47)

set -euo pipefail

TEST_NAME="MCP Server"
TEST_ABBR="MCP"
TEST_NUMBER="47"
TEST_COUNTER=0
TEST_VERSION="1.0.1"

# shellcheck source=tests/lib/framework.sh # Reference framework directly
[[ -n "${FRAMEWORK_GUARD:-}" ]] || source "$(dirname "${BASH_SOURCE[0]}")/lib/framework.sh"
setup_test_environment

# shellcheck source=tests/lib/scripting_helpers.sh # Start/shutdown helpers shared with test_43
source "$(dirname "${BASH_SOURCE[0]}")/lib/scripting_helpers.sh"

declare -a PARALLEL_PIDS
declare -A SCRIPT_TEST_CONFIGS

# config:log_suffix:engine_key:description
SCRIPT_TEST_CONFIGS=(
    ["PostgreSQL"]="${SCRIPT_DIR}/configs/hydrogen_test_${TEST_NUMBER}_mcp_postgres.json:postgres:postgresql:PostgreSQL"
    ["MySQL"]="${SCRIPT_DIR}/configs/hydrogen_test_${TEST_NUMBER}_mcp_mysql.json:mysql:mysql:MySQL"
    ["SQLite"]="${SCRIPT_DIR}/configs/hydrogen_test_${TEST_NUMBER}_mcp_sqlite.json:sqlite:sqlite:SQLite"
    ["DB2"]="${SCRIPT_DIR}/configs/hydrogen_test_${TEST_NUMBER}_mcp_db2.json:db2:db2:DB2"
    ["MariaDB"]="${SCRIPT_DIR}/configs/hydrogen_test_${TEST_NUMBER}_mcp_mariadb.json:mariadb:mariadb:MariaDB"
    ["CockroachDB"]="${SCRIPT_DIR}/configs/hydrogen_test_${TEST_NUMBER}_mcp_cockroachdb.json:cockroachdb:cockroachdb:CockroachDB"
    ["YugabyteDB"]="${SCRIPT_DIR}/configs/hydrogen_test_${TEST_NUMBER}_mcp_yugabytedb.json:yugabytedb:yugabytedb:YugabyteDB"
)

DISABLED_CONFIG="${SCRIPT_DIR}/configs/hydrogen_test_${TEST_NUMBER}_mcp_disabled.json"

# shellcheck disable=SC2034 # Reserved for log messages / future fail-fast bounds
STARTUP_TIMEOUT=90
SHUTDOWN_TIMEOUT=25
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
    local max_retries=5
    local retry=1
    local http_status="000"

    while [[ "${retry}" -le "${max_retries}" ]]; do
        local curl_cmd=(curl -s -X "${method}" -H "Content-Type: application/json"
            --connect-timeout 10 --max-time "${max_time}" -w "%{http_code}" -o "${output_file}")
        if [[ -n "${jwt_token}" ]]; then
            curl_cmd+=(-H "Authorization: Bearer ${jwt_token}")
        fi
        if [[ "${method}" != "GET" && "${method}" != "DELETE" && -n "${data}" ]]; then
            curl_cmd+=(-d "${data}")
        fi
        curl_cmd+=("${url}")
        # shellcheck disable=SC2312 # Intentionally swallow curl exit code; we use the HTTP status
        http_status=$("${curl_cmd[@]}" 2>/dev/null || true)
        http_status="${http_status:-000}"

        if [[ "${http_status}" == "000" || \
              "${http_status}" == 5* || \
              "${http_status}" == "408" || \
              "${http_status}" == "429" ]]; then
            if [[ "${retry}" -eq "${max_retries}" ]]; then
                break
            fi
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "HTTP ${http_status} from ${method} ${url}, retrying (${retry}/${max_retries})..."
            sleep "${retry}"
            retry=$(( retry + 1 ))
            continue
        fi
        break
    done

    echo "${http_status}"
}

mcp_http() {
    local method="$1"
    local url="$2"
    local data="$3"
    local output_file="$4"
    local header_file="$5"
    local jwt_token="${6:-}"
    local session_id="${7:-}"
    local origin="${8:-}"
    local max_time="${9:-${HTTP_TIMEOUT}}"
    local max_retries=5
    local retry=1
    local http_status="000"

    while [[ "${retry}" -le "${max_retries}" ]]; do
        local curl_cmd=(curl -s -X "${method}" -H "Content-Type: application/json"
            --connect-timeout 10 --max-time "${max_time}"
            -D "${header_file}" -o "${output_file}" -w "%{http_code}")
        if [[ -n "${jwt_token}" ]]; then
            curl_cmd+=(-H "Authorization: Bearer ${jwt_token}")
        fi
        if [[ -n "${session_id}" ]]; then
            curl_cmd+=(-H "Mcp-Session-Id: ${session_id}")
        fi
        if [[ -n "${origin}" ]]; then
            curl_cmd+=(-H "Origin: ${origin}")
        fi
        if [[ "${method}" != "GET" && "${method}" != "DELETE" && -n "${data}" ]]; then
            curl_cmd+=(-d "${data}")
        fi
        curl_cmd+=("${url}")
        # shellcheck disable=SC2312 # Intentionally swallow curl exit code; we use the HTTP status
        http_status=$("${curl_cmd[@]}" 2>/dev/null || true)
        http_status="${http_status:-000}"

        if [[ "${http_status}" == "000" || \
              "${http_status}" == 5* || \
              "${http_status}" == "408" || \
              "${http_status}" == "429" ]]; then
            if [[ "${retry}" -eq "${max_retries}" ]]; then
                break
            fi
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "HTTP ${http_status} from ${method} ${url}, retrying (${retry}/${max_retries})..."
            sleep "${retry}"
            retry=$(( retry + 1 ))
            continue
        fi
        break
    done

    echo "${http_status}"
}

extract_jwt() {
    local response_file="$1"
    if [[ -f "${response_file}" ]]; then
        jq -r '.token // empty' "${response_file}" 2>/dev/null || true
    fi
}

header_value() {
    local header_file="$1"
    local name="$2"
    if [[ -f "${header_file}" ]]; then
        "${GREP}" -i "^${name}:" "${header_file}" 2>/dev/null | head -1 | cut -d: -f2- | tr -d '\r' | sed 's/^[[:space:]]*//' || true
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

b64url_encode() {
    openssl base64 -e -A | tr '+/' '-_' | tr -d '='
}

b64url_decode() {
    local s="$1"
    local mod=$(( ${#s} % 4 ))
    if [[ "${mod}" -eq 2 ]]; then
        s="${s}=="
    elif [[ "${mod}" -eq 3 ]]; then
        s="${s}="
    fi
    printf '%s' "${s}" | tr -- '-_' '+/' | openssl base64 -d -A 2>/dev/null || true
}

token_hash() {
    local token="$1"
    printf '%s' "${token}" | openssl dgst -sha256 -binary | b64url_encode
}

# Rebuild JWT A with a different sub so session bind rejects it (sqlite only).
mint_hijack_jwt() {
    local src_jwt="$1"
    local secret="$2"
    local new_sub="$3"
    local payload_b64 header_b64 payload_json new_payload unsigned sig
    header_b64=$(printf '%s' "${src_jwt}" | cut -d. -f1)
    payload_b64=$(printf '%s' "${src_jwt}" | cut -d. -f2)
    payload_json=$(b64url_decode "${payload_b64}")
    if [[ -z "${payload_json}" ]]; then
        return 1
    fi
    new_payload=$(printf '%s' "${payload_json}" | jq -c --arg sub "${new_sub}" '.sub = $sub')
    payload_b64=$(printf '%s' "${new_payload}" | b64url_encode)
    unsigned="${header_b64}.${payload_b64}"
    sig=$(printf '%s' "${unsigned}" | openssl dgst -sha256 -hmac "${secret}" -binary | b64url_encode)
    printf '%s' "${unsigned}.${sig}"
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
    local seed_n
    seed_n=$(sqlite3 "${db_copy}" \
        "SELECT COUNT(*) FROM scripts WHERE group_name='Mcp' AND script_name='Server' AND mcp_access<>0;" \
        2>/dev/null || echo 0)
    if [[ "${seed_n}" -lt 1 ]]; then
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
    local db_copy=""
    if [[ "${engine_key}" == "sqlite" ]]; then
        work_dir="${DIAG_TEST_DIR}/sqlite_${engine_key}_$$"
        # shellcheck disable=SC2310 # Capture STARTUP_FAILED when SQLite copy fails
        run_config=$(prepare_sqlite_config "${config_file}" "${work_dir}") || {
            echo "STARTUP_FAILED=1" >> "${result_file}"
            echo "REASON=sqlite_copy" >> "${result_file}"
            return 0
        }
        db_copy="${work_dir}/hydrodemo.sqlite"
    fi

    local web_port mcp_port
    web_port=$(get_webserver_port "${run_config}")
    mcp_port=$(jq -r '.MCP.Port // empty' "${run_config}" 2>/dev/null || true)
    local base_url="http://127.0.0.1:${web_port}"
    local mcp_url="http://127.0.0.1:${mcp_port}/mcp"
    echo "PORT=${web_port}" >> "${result_file}"
    echo "MCP_PORT=${mcp_port}" >> "${result_file}"

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

    local body hdr http_st
    body="${result_file}.body"
    hdr="${result_file}.hdr"

    # --- unauthenticated healthz / PRM ---
    http_st=$(mcp_http "GET" "${mcp_url}/healthz" "" "${body}" "${hdr}" "" "" "" 10)
    if [[ "${http_st}" == "200" ]] && jq -e '.status == "ok"' "${body}" >/dev/null 2>&1; then
        record_case "${result_file}" "healthz" 1
    else
        record_case "${result_file}" "healthz" 0
        echo "HEALTHZ_HTTP=${http_st}" >> "${result_file}"
    fi

    http_st=$(mcp_http "GET" "http://127.0.0.1:${mcp_port}/.well-known/oauth-protected-resource" \
        "" "${body}" "${hdr}" "" "" "" 10)
    if [[ "${http_st}" == "200" ]] && jq -e '.resource != null' "${body}" >/dev/null 2>&1; then
        record_case "${result_file}" "prm" 1
    else
        record_case "${result_file}" "prm" 0
        echo "PRM_HTTP=${http_st}" >> "${result_file}"
    fi

    http_st=$(mcp_http "GET" "http://127.0.0.1:${mcp_port}/.well-known/oauth-protected-resource/mcp" \
        "" "${body}" "${hdr}" "" "" "" 10)
    if [[ "${http_st}" == "200" ]]; then
        record_case "${result_file}" "prm_path" 1
    else
        record_case "${result_file}" "prm_path" 0
        echo "PRM_PATH_HTTP=${http_st}" >> "${result_file}"
    fi

    http_st=$(mcp_http "POST" "${mcp_url}" \
        '{"jsonrpc":"2.0","id":1,"method":"initialize","params":{}}' \
        "${body}" "${hdr}" "" "" "" 10)
    local www
    www=$(header_value "${hdr}" "WWW-Authenticate")
    if [[ "${http_st}" == "401" && "${www}" == *resource_metadata=* ]]; then
        record_case "${result_file}" "unauth_401_www" 1
    else
        record_case "${result_file}" "unauth_401_www" 0
        echo "UNAUTH_HTTP=${http_st}" >> "${result_file}"
        echo "UNAUTH_WWW=${www}" >> "${result_file}"
    fi

    http_st=$(mcp_http "POST" "${mcp_url}" \
        '{"jsonrpc":"2.0","id":1,"method":"ping"}' \
        "${body}" "${hdr}" "" "" "http://evil.example" 10)
    if [[ "${http_st}" == "403" ]]; then
        record_case "${result_file}" "origin_403" 1
    else
        record_case "${result_file}" "origin_403" 0
        echo "ORIGIN_HTTP=${http_st}" >> "${result_file}"
    fi

    http_st=$(mcp_http "GET" "${mcp_url}" "" "${body}" "${hdr}" "" "" "" 10)
    if [[ "${http_st}" == "405" ]]; then
        record_case "${result_file}" "get_path_405" 1
    else
        record_case "${result_file}" "get_path_405" 0
        echo "GET_PATH_HTTP=${http_st}" >> "${result_file}"
    fi

    # --- login ---
    local login_file="${result_file}.login.json"
    local login_payload
    login_payload=$(jq -n \
        --arg login_id "${HYDROGEN_DEMO_USER_NAME}" \
        --arg password "${HYDROGEN_DEMO_USER_PASS}" \
        --arg api_key "${HYDROGEN_DEMO_API_KEY}" \
        '{database:"Acuranzo",login_id:$login_id,password:$password,api_key:$api_key,tz:"America/Vancouver"}')
    http_st=$(api_request "POST" "${base_url}/api/auth/login" "${login_payload}" "${login_file}" "" 45)
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

    http_st=$(api_request "GET" "${base_url}/api/mcp/status" "" "${body}" "${jwt}" 15)
    if [[ "${http_st}" == "200" ]] && jq -e '.enabled == true' "${body}" >/dev/null 2>&1; then
        record_case "${result_file}" "api_status" 1
    else
        record_case "${result_file}" "api_status" 0
        echo "STATUS_HTTP=${http_st}" >> "${result_file}"
    fi

    # --- initialize ---
    local init_body='{"jsonrpc":"2.0","id":1,"method":"initialize","params":{"protocolVersion":"2025-06-18","capabilities":{},"clientInfo":{"name":"test47","version":"1.0.0"}}}'
    http_st=$(mcp_http "POST" "${mcp_url}" "${init_body}" "${body}" "${hdr}" "${jwt}" "" "" 30)
    local session
    session=$(header_value "${hdr}" "Mcp-Session-Id")
    local init_ok=0
    if [[ "${http_st}" == "200" && -n "${session}" ]] \
        && jq -e '.result.serverInfo.name == "hydrogen" and (.result.instructions|type=="string")' \
            "${body}" >/dev/null 2>&1; then
        init_ok=1
    fi
    if [[ "${init_ok}" -eq 1 ]]; then
        record_case "${result_file}" "initialize" 1
    else
        record_case "${result_file}" "initialize" 0
        echo "INIT_HTTP=${http_st}" >> "${result_file}"
        echo "INIT_SESSION=${session}" >> "${result_file}"
        # Non-SQLite: skip remaining cases if Protocol Lua did not load.
        if [[ "${engine_key}" != "sqlite" ]]; then
            echo "ENGINE_SKIP=fixture_migrations" >> "${result_file}"
        fi
    fi
    echo "SESSION=${session}" >> "${result_file}"

    if [[ "${init_ok}" -ne 1 ]]; then
        echo "ENGINE_COMPLETE=1" >> "${result_file}"
        # shellcheck disable=SC2310 # Shutdown best-effort
        scripting_shutdown_instance "${hydrogen_pid}" "${SHUTDOWN_TIMEOUT}" || true
        return 0
    fi

    http_st=$(mcp_http "POST" "${mcp_url}" \
        '{"jsonrpc":"2.0","method":"notifications/initialized"}' \
        "${body}" "${hdr}" "${jwt}" "${session}" "" 10)
    if [[ "${http_st}" == "202" ]]; then
        record_case "${result_file}" "initialized_202" 1
    else
        record_case "${result_file}" "initialized_202" 0
        echo "INITIALIZED_HTTP=${http_st}" >> "${result_file}"
    fi

    http_st=$(mcp_http "POST" "${mcp_url}" \
        '{"jsonrpc":"2.0","id":2,"method":"ping"}' \
        "${body}" "${hdr}" "${jwt}" "${session}" "" 15)
    if [[ "${http_st}" == "200" ]] && jq -e '.result != null and .error == null' "${body}" >/dev/null 2>&1; then
        record_case "${result_file}" "ping" 1
    else
        record_case "${result_file}" "ping" 0
        echo "PING_HTTP=${http_st}" >> "${result_file}"
    fi

    http_st=$(mcp_http "POST" "${mcp_url}" \
        '{"jsonrpc":"2.0","id":3,"method":"tools/list","params":{}}' \
        "${body}" "${hdr}" "${jwt}" "${session}" "" 20)
    local list_ok=0
    if [[ "${http_st}" == "200" ]] \
        && jq -e '
            ([.result.tools[].name] | index("Mcp.Echo"))
            and ([.result.tools[].name] | index("Mcp.EchoStrict"))
            and ([.result.tools[] | select(.name=="Mcp.Echo") | .inputSchema] | length > 0)
            and ([.result.tools[].name] | index("Mcp.Server") | not)
            and ([.result.tools[].name] | index("Mcp.Helpers") | not)
        ' "${body}" >/dev/null 2>&1; then
        list_ok=1
    fi
    if [[ "${list_ok}" -eq 1 ]]; then
        record_case "${result_file}" "tools_list" 1
    else
        record_case "${result_file}" "tools_list" 0
        echo "LIST_HTTP=${http_st}" >> "${result_file}"
        if [[ "${engine_key}" != "sqlite" ]]; then
            echo "ENGINE_SKIP=fixture_migrations" >> "${result_file}"
        fi
    fi

    if [[ "${list_ok}" -ne 1 ]]; then
        echo "ENGINE_COMPLETE=1" >> "${result_file}"
        # shellcheck disable=SC2310 # Shutdown best-effort
        scripting_shutdown_instance "${hydrogen_pid}" "${SHUTDOWN_TIMEOUT}" || true
        return 0
    fi

    http_st=$(mcp_http "POST" "${mcp_url}" \
        '{"jsonrpc":"2.0","id":4,"method":"tools/call","params":{"name":"Mcp.Echo","arguments":{"message":"hello"}}}' \
        "${body}" "${hdr}" "${jwt}" "${session}" "" 20)
    if [[ "${http_st}" == "200" ]] \
        && jq -e '.result.content != null and .result.isError != true' "${body}" >/dev/null 2>&1; then
        record_case "${result_file}" "echo_ok" 1
    else
        record_case "${result_file}" "echo_ok" 0
        echo "ECHO_HTTP=${http_st}" >> "${result_file}"
    fi

    http_st=$(mcp_http "POST" "${mcp_url}" \
        '{"jsonrpc":"2.0","id":5,"method":"tools/call","params":{"name":"Mcp.EchoStrict","arguments":{"nope":true}}}' \
        "${body}" "${hdr}" "${jwt}" "${session}" "" 20)
    if [[ "${http_st}" == "200" ]] \
        && jq -e '.result.isError == true and .error == null' "${body}" >/dev/null 2>&1; then
        record_case "${result_file}" "echostrict_iserror" 1
    else
        record_case "${result_file}" "echostrict_iserror" 0
        echo "STRICT_HTTP=${http_st}" >> "${result_file}"
    fi

    http_st=$(mcp_http "POST" "${mcp_url}" \
        '{"jsonrpc":"2.0","id":6,"method":"tools/call","params":{"name":"No.SuchTool","arguments":{}}}' \
        "${body}" "${hdr}" "${jwt}" "${session}" "" 20)
    if [[ "${http_st}" == "200" ]] \
        && jq -e '.result.isError == true and .error == null' "${body}" >/dev/null 2>&1; then
        record_case "${result_file}" "unknown_tool_hidden" 1
    else
        record_case "${result_file}" "unknown_tool_hidden" 0
        echo "UNKNOWN_TOOL_HTTP=${http_st}" >> "${result_file}"
    fi

    http_st=$(mcp_http "POST" "${mcp_url}" \
        '{"jsonrpc":"2.0","id":7,"method":"tools/call","params":{"name":"Orchestrators.Orchestrator","arguments":{}}}' \
        "${body}" "${hdr}" "${jwt}" "${session}" "" 20)
    if [[ "${http_st}" == "200" ]] \
        && jq -e '.result.isError == true and .error == null' "${body}" >/dev/null 2>&1; then
        record_case "${result_file}" "non_mcp_hidden" 1
    else
        record_case "${result_file}" "non_mcp_hidden" 0
        echo "NON_MCP_HTTP=${http_st}" >> "${result_file}"
    fi

    http_st=$(mcp_http "POST" "${mcp_url}" \
        '{"jsonrpc":"2.0","id":8,"method":"ping","_hydrogen":{"x":1}}' \
        "${body}" "${hdr}" "${jwt}" "${session}" "" 10)
    if [[ "${http_st}" == "401" ]]; then
        record_case "${result_file}" "hydrogen_rejected" 1
    else
        record_case "${result_file}" "hydrogen_rejected" 0
        echo "HYDROGEN_HTTP=${http_st}" >> "${result_file}"
    fi

    http_st=$(mcp_http "POST" "${mcp_url}" \
        '{"jsonrpc":"2.0","id":9,"method":"ping"}' \
        "${body}" "${hdr}" "${jwt}" "not-a-session" "" 10)
    if [[ "${http_st}" == "404" ]]; then
        record_case "${result_file}" "unknown_session_404" 1
    else
        record_case "${result_file}" "unknown_session_404" 0
        echo "UNK_SESSION_HTTP=${http_st}" >> "${result_file}"
    fi

    # Session hijack: sqlite-only (mint JWT with different sub + insert tokens row)
    if [[ "${engine_key}" == "sqlite" && -n "${db_copy}" && -n "${HYDROGEN_DEMO_JWT_KEY}" ]]; then
        local hijack_jwt old_hash new_hash
        # shellcheck disable=SC2310 # Mint failure falls back to empty token
        hijack_jwt=$(mint_hijack_jwt "${jwt}" "${HYDROGEN_DEMO_JWT_KEY}" "hijack-user-b") || hijack_jwt=""
        if [[ -n "${hijack_jwt}" ]]; then
            old_hash=$(token_hash "${jwt}")
            new_hash=$(token_hash "${hijack_jwt}")
            sqlite3 "${db_copy}" \
                "INSERT INTO tokens (token_hash, account_id, system_id, app_id, app_version, ip_address, valid_after, valid_until)
                 SELECT '${new_hash}', account_id, system_id, app_id, app_version, ip_address, valid_after, valid_until
                   FROM tokens WHERE token_hash = '${old_hash}' LIMIT 1;" >/dev/null 2>&1 || true
            http_st=$(mcp_http "POST" "${mcp_url}" \
                '{"jsonrpc":"2.0","id":10,"method":"ping"}' \
                "${body}" "${hdr}" "${hijack_jwt}" "${session}" "" 10)
            if [[ "${http_st}" == "401" ]]; then
                record_case "${result_file}" "hijack_401" 1
            else
                record_case "${result_file}" "hijack_401" 0
                echo "HIJACK_HTTP=${http_st}" >> "${result_file}"
            fi
        else
            record_case "${result_file}" "hijack_401" 0
            echo "HIJACK_MINT=fail" >> "${result_file}"
        fi
    fi

    # Two overlapping Echo calls (WorkerCount=2 must not deadlock)
    local echo_a="${result_file}.echo_a"
    local echo_b="${result_file}.echo_b"
    local hdr_a="${result_file}.hdr_a"
    local hdr_b="${result_file}.hdr_b"
    local st_a st_b
    ( mcp_http "POST" "${mcp_url}" \
        '{"jsonrpc":"2.0","id":11,"method":"tools/call","params":{"name":"Mcp.Echo","arguments":{"n":1}}}' \
        "${echo_a}" "${hdr_a}" "${jwt}" "${session}" "" 20 | tail -1 > "${result_file}.st_a" ) &
    local pid_a=$!
    ( mcp_http "POST" "${mcp_url}" \
        '{"jsonrpc":"2.0","id":12,"method":"tools/call","params":{"name":"Mcp.Echo","arguments":{"n":2}}}' \
        "${echo_b}" "${hdr_b}" "${jwt}" "${session}" "" 20 | tail -1 > "${result_file}.st_b" ) &
    local pid_b=$!
    wait "${pid_a}" || true
    wait "${pid_b}" || true
    st_a=$(tail -1 "${result_file}.st_a" 2>/dev/null || echo 000)
    st_b=$(tail -1 "${result_file}.st_b" 2>/dev/null || echo 000)
    if [[ "${st_a}" == "200" && "${st_b}" == "200" ]] \
        && jq -e '.result.isError != true' "${echo_a}" >/dev/null 2>&1 \
        && jq -e '.result.isError != true' "${echo_b}" >/dev/null 2>&1; then
        record_case "${result_file}" "overlap_echo" 1
    else
        record_case "${result_file}" "overlap_echo" 0
        echo "OVERLAP_HTTP=${st_a},${st_b}" >> "${result_file}"
    fi

    # Sleep timeout + cancelled (cancel is C 202; timeout yields -32603)
    local sleep_body="${result_file}.sleep"
    local sleep_hdr="${result_file}.sleep.hdr"
    ( mcp_http "POST" "${mcp_url}" \
        '{"jsonrpc":"2.0","id":13,"method":"tools/call","params":{"name":"Mcp.Sleep","arguments":{"seconds":30}}}' \
        "${sleep_body}" "${sleep_hdr}" "${jwt}" "${session}" "" 20 | tail -1 > "${result_file}.sleep.st" ) &
    local sleep_pid=$!
    sleep 0.3
    http_st=$(mcp_http "POST" "${mcp_url}" \
        '{"jsonrpc":"2.0","method":"notifications/cancelled","params":{"requestId":13}}' \
        "${body}" "${hdr}" "${jwt}" "${session}" "" 10)
    if [[ "${http_st}" == "202" ]]; then
        record_case "${result_file}" "cancelled_202" 1
    else
        record_case "${result_file}" "cancelled_202" 0
        echo "CANCEL_HTTP=${http_st}" >> "${result_file}"
    fi
    wait "${sleep_pid}" || true
    local sleep_st
    sleep_st=$(cat "${result_file}.sleep.st" 2>/dev/null || echo 000)
    if [[ "${sleep_st}" == "200" ]] \
        && jq -e '.error.code == -32603' "${sleep_body}" >/dev/null 2>&1; then
        record_case "${result_file}" "sleep_timeout" 1
    else
        record_case "${result_file}" "sleep_timeout" 0
        echo "SLEEP_HTTP=${sleep_st}" >> "${result_file}"
    fi

    http_st=$(mcp_http "DELETE" "${mcp_url}" "" "${body}" "${hdr}" "${jwt}" "${session}" "" 10)
    if [[ "${http_st}" == "204" ]]; then
        record_case "${result_file}" "delete_204" 1
    else
        record_case "${result_file}" "delete_204" 0
        echo "DELETE_HTTP=${http_st}" >> "${result_file}"
    fi

    http_st=$(mcp_http "POST" "${mcp_url}" \
        '{"jsonrpc":"2.0","id":14,"method":"ping"}' \
        "${body}" "${hdr}" "${jwt}" "${session}" "" 10)
    if [[ "${http_st}" == "404" ]]; then
        record_case "${result_file}" "delete_reuse_404" 1
    else
        record_case "${result_file}" "delete_reuse_404" 0
        echo "REUSE_HTTP=${http_st}" >> "${result_file}"
    fi

    echo "ENGINE_COMPLETE=1" >> "${result_file}"
    local shut_ok=1
    # shellcheck disable=SC2310 # Shutdown is a scored case
    if ! scripting_shutdown_instance "${hydrogen_pid}" "${SHUTDOWN_TIMEOUT}"; then
        shut_ok=0
    fi
    record_case "${result_file}" "shutdown_clean" "${shut_ok}"
    return 0
}

analyze_engine() {
    local result_file="$1"
    local description="$2"
    local log_file="$3"

    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}: MCP Streamable HTTP"
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
            "${description}: skipped (MCP seeds / QueryRef 152-153 not applied on live DB)"
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

    local min_pass=20
    if [[ "${description}" == "SQLite" ]]; then
        min_pass=22
    fi

    if [[ "${fail_n}" -eq 0 && "${pass_n}" -ge "${min_pass}" ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 \
            "${description}: ${pass_n} cases passed (healthz/PRM/auth/session/tools/timeout)"
        PASS_COUNT=$(( PASS_COUNT + 1 ))
    else
        local fails
        fails=$("${GREP}" "^CASE_FAIL=" "${result_file}" 2>/dev/null | cut -d= -f2 | tr '\n' ',' || true)
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 \
            "${description}: pass=${pass_n} fail=${fail_n} (${fails%,})"
        EXIT_CODE=1
    fi
}

run_disabled() {
    local hydrogen_bin="$1"
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Disabled MCP does not bind"
    local work_dir="${DIAG_TEST_DIR}/disabled_$$"
    local run_config log_file
    # shellcheck disable=SC2310 # Disabled run uses isolated sqlite copy
    run_config=$(prepare_sqlite_config "${DISABLED_CONFIG}" "${work_dir}") || {
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Disabled config sqlite copy failed"
        EXIT_CODE=1
        return
    }
    log_file="${LOGS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}_disabled.log"
    local hydrogen_pid=""
    # shellcheck disable=SC2310 # Start failure is a scored result
    if ! scripting_start_instance "${run_config}" "${log_file}" "${hydrogen_bin}" hydrogen_pid; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Disabled MCP instance failed to start"
        EXIT_CODE=1
        return
    fi
    # shellcheck disable=SC2310 # Ready failure is a scored result
    if ! wait_ready "${log_file}" "${READY_TIMEOUT}"; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Disabled MCP instance not READY"
        scripting_shutdown_instance "${hydrogen_pid}" "${SHUTDOWN_TIMEOUT}" || true
        EXIT_CODE=1
        return
    fi
    local mcp_port http_st
    mcp_port=$(jq -r '.MCP.Port // empty' "${run_config}")
    # shellcheck disable=SC2312 # curl exit is ignored; HTTP code (or empty) is the signal
    http_st=$(curl -s -o /dev/null -w "%{http_code}" --connect-timeout 2 --max-time 3 \
        "http://127.0.0.1:${mcp_port}/mcp/healthz" 2>/dev/null || true)
    http_st="${http_st:-000}"
    # shellcheck disable=SC2310 # Shutdown best-effort after the bind check
    scripting_shutdown_instance "${hydrogen_pid}" "${SHUTDOWN_TIMEOUT}" || true
    if [[ "${http_st}" != "200" ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "MCP port ${mcp_port} refused when Enabled=false"
        PASS_COUNT=$(( PASS_COUNT + 1 ))
    else
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "MCP port ${mcp_port} answered HTTP ${http_st} while disabled"
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
        mcp_port=$(jq -r '.MCP.Port // empty' "${config_file}")
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${description} will use web ${port} mcp ${mcp_port}"
    else
        config_valid=false
        EXIT_CODE=1
    fi
done

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Validate Configuration Files"
# shellcheck disable=SC2310 # Disabled config validation is part of the if condition
if [[ "${config_valid}" = true ]] && validate_config_file "${DISABLED_CONFIG}"; then
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "All ${#SCRIPT_TEST_CONFIGS[@]} engine configs plus disabled validated"
    PASS_COUNT=$(( PASS_COUNT + 1 ))
else
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Configuration file validation failed"
    config_valid=false
    EXIT_CODE=1
fi

# ---------------------------------------------------------------------------
# Disabled bind + parallel engines
# ---------------------------------------------------------------------------

if [[ "${EXIT_CODE}" -eq 0 ]]; then
    run_disabled "${HYDROGEN_BIN}"

    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Running MCP blackbox in parallel"

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
            && "${GREP}" -q "^CASE_PASS=echo_ok$" "${result_file}" 2>/dev/null \
            && ! "${GREP}" -q "^CASE_FAIL=" "${result_file}" 2>/dev/null; then
            sqlite_full=1
        fi
    done

    print_marker "${TEST_NUMBER}" "${TEST_COUNTER}"
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" \
        "Summary: ${successful}/${#SCRIPT_TEST_CONFIGS[@]} engines ok (SQLite full fixture required)"
    if [[ "${sqlite_full}" -ne 1 ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "SQLite full MCP path did not pass"
        EXIT_CODE=1
    fi
else
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Skipping MCP tests due to prerequisite failures"
    EXIT_CODE=1
fi

print_test_completion "${TEST_NAME}" "${TEST_ABBR}" "${TEST_NUMBER}" "${TEST_VERSION}"
${ORCHESTRATION:-false} && return "${EXIT_CODE}" || exit "${EXIT_CODE}"
