#!/usr/bin/env bash

# Test: MCP Streamable HTTP (MCP Phase 13)
# Real hydrogen, Hydrogen JWT, Lua Mcp.Server: initialize / tools / call / session.

# FUNCTIONS
# (Helpers live in tests/lib/mcp_helpers.sh)
# run_engine()
# analyze_engine()
# run_disabled()

# CHANGELOG
# 1.1.10 - 2026-09-02 - Assert System.Info in tools/list + tools/call returns system JSON
# 1.1.9 - 2026-08-29 - echo_ok via mcp_expect_jq (3 tries) like echostrict/resources/prompts;
#                      CockroachDB RequestTimeoutSeconds 4→15 for nested DB query headroom.
# 1.1.8 - 2026-08-27 - Score ping/echostrict/cursor/prompts via mcp_expect_jq (3 tries);
#                      overlap Echo retry on 401; drop 10–20s MCP max-times.
# 1.1.7 - 2026-08-27 - 90s HTTP wait, 000-only retry, INFO delay (group40_http).
# 1.1.6 - 2026-08-27 - Suite-load: HTTP warmup; login 8×; retry initialize on
#                      404/000; retry resources_unknown JSON-RPC.
# 1.1.5 - 2026-08-27 - Split HTTP/JWT/SQLite helpers into tests/lib/mcp_helpers.sh
# 1.1.4 - 2026-08-27 - Suite-load: LOGINMAXATTEMPTS 100000 + Fast/Medium/Cache queues (match test_40); retry login on 401/empty JWT so a 15m IP block from test 46 cannot fail the engine.  Retry tools/resources/prompts list (QueryRef 152 empty).
# 1.1.3 - 2026-08-27 - Extra auth/parse/list cases (malformed Bearer, nested _hydrogen in arguments, invalid JSON, tools/list cursor)
# 1.1.2 - 2026-08-27 - Fail engines that cannot load Mcp.Server (no skip-pass)
# 1.1.1 - 2026-08-27 - Phase 15 required on all engines (1375 applied)
# 1.1.0 - 2026-08-27 - Phase 15 resources/list+read, prompts/list+get
# 1.0.1 - 2026-08-27 - Skip comment: seeds expected applied (1283 hole closed)
# 1.0.0 - 2026-08-27 - Initial blackbox for MCP Phase 13 (Test 47)

set -euo pipefail

TEST_NAME="MCP Server"
TEST_ABBR="MCP"
TEST_NUMBER="47"
TEST_COUNTER=0
TEST_VERSION="1.1.10"

# shellcheck source=tests/lib/framework.sh # Reference framework directly
[[ -n "${FRAMEWORK_GUARD:-}" ]] || source "$(dirname "${BASH_SOURCE[0]}")/lib/framework.sh"
setup_test_environment

# shellcheck source=tests/lib/scripting_helpers.sh # Start/shutdown helpers shared with test_43
source "$(dirname "${BASH_SOURCE[0]}")/lib/scripting_helpers.sh"
# shellcheck source=tests/lib/mcp_helpers.sh # Split for the 1000-line cap
source "$(dirname "${BASH_SOURCE[0]}")/lib/mcp_helpers.sh"

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
STARTUP_TIMEOUT="${GROUP40_STARTUP_TIMEOUT}"
SHUTDOWN_TIMEOUT="${GROUP40_SHUTDOWN_TIMEOUT}"
READY_TIMEOUT="${GROUP40_READY_TIMEOUT}"
HTTP_TIMEOUT="${GROUP40_HTTP_MAX_TIME}"
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
    # shellcheck disable=SC2310 # Continue; later HTTP retries still cover bind lag
    wait_http "${base_url}/api/version" "${GROUP40_READY_TIMEOUT}" || true
    # shellcheck disable=SC2310 # MCP bind can lag WebServer under suite load
    wait_http "${mcp_url}/healthz" "${GROUP40_READY_TIMEOUT}" || true

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

    # shellcheck disable=SC2312 # curl exit ignored; HTTP code is the signal
    http_st=$(curl -s -X POST "${mcp_url}" -H "Content-Type: application/json" \
        -H "Authorization: Basic abc" -d '{"jsonrpc":"2.0","id":1,"method":"ping"}' \
        --connect-timeout 10 --max-time 10 -o "${body}" -w "%{http_code}" 2>/dev/null || true)
    http_st="${http_st:-000}"
    if [[ "${http_st}" == "401" ]]; then
        record_case "${result_file}" "malformed_auth_401" 1
    else
        record_case "${result_file}" "malformed_auth_401" 0
        echo "MALFORMED_AUTH_HTTP=${http_st}" >> "${result_file}"
    fi

    # shellcheck disable=SC2312 # curl exit ignored; HTTP code is the signal
    http_st=$(curl -s -X POST "${mcp_url}" -H "Content-Type: application/json" \
        -H "Authorization: Bearer " -d '{"jsonrpc":"2.0","id":1,"method":"ping"}' \
        --connect-timeout 10 --max-time 10 -o "${body}" -w "%{http_code}" 2>/dev/null || true)
    http_st="${http_st:-000}"
    if [[ "${http_st}" == "401" ]]; then
        record_case "${result_file}" "empty_bearer_401" 1
    else
        record_case "${result_file}" "empty_bearer_401" 0
        echo "EMPTY_BEARER_HTTP=${http_st}" >> "${result_file}"
    fi

    # shellcheck disable=SC2312 # curl exit ignored; HTTP code is the signal
    http_st=$(curl -s -X POST "${mcp_url}" -H "Content-Type: application/json" \
        -H "Authorization: Bearer not-a-jwt" -d 'not-json' \
        --connect-timeout 10 --max-time 10 -o "${body}" -w "%{http_code}" 2>/dev/null || true)
    http_st="${http_st:-000}"
    if [[ "${http_st}" == "400" || "${http_st}" == "401" ]]; then
        record_case "${result_file}" "invalid_json_rejected" 1
    else
        record_case "${result_file}" "invalid_json_rejected" 0
        echo "INVALID_JSON_HTTP=${http_st}" >> "${result_file}"
    fi

    # --- login ---
    local login_file="${result_file}.login.json"
    local login_payload
    login_payload=$(jq -n \
        --arg login_id "${HYDROGEN_DEMO_USER_NAME}" \
        --arg password "${HYDROGEN_DEMO_USER_PASS}" \
        --arg api_key "${HYDROGEN_DEMO_API_KEY}" \
        '{database:"Acuranzo",login_id:$login_id,password:$password,api_key:$api_key,tz:"America/Vancouver"}')
    local jwt=""
    local login_try=1
    while [[ "${login_try}" -le 2 ]]; do
        # shellcheck disable=SC2312 # Intentionally swallow curl exit code; we use the HTTP status
        http_st=$(api_request "POST" "${base_url}/api/auth/login" "${login_payload}" "${login_file}" "" \
            "${GROUP40_HTTP_MAX_TIME}")
        jwt=$(extract_jwt "${login_file}")
        if [[ "${http_st}" == "200" && -n "${jwt}" ]]; then
            break
        fi
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" \
            "INFO delay ${description}: login HTTP ${http_st} (try ${login_try}/2)"
        sleep 2
        login_try=$(( login_try + 1 ))
    done
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
    local session=""
    local init_ok=0
    local init_try=1
    while [[ "${init_try}" -le 5 ]]; do
        http_st=$(mcp_http "POST" "${mcp_url}" "${init_body}" "${body}" "${hdr}" "${jwt}" "" "")
        session=$(header_value "${hdr}" "Mcp-Session-Id")
        init_ok=0
        if [[ "${http_st}" == "200" && -n "${session}" ]] \
            && jq -e '.result.serverInfo.name == "hydrogen" and (.result.instructions|type=="string")' \
                "${body}" >/dev/null 2>&1; then
            init_ok=1
            break
        fi
        if [[ "${http_st}" != "404" && "${http_st}" != "000" && "${http_st}" != 5* ]]; then
            break
        fi
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" \
            "${description}: initialize HTTP ${http_st} (try ${init_try}/5)"
        sleep "${init_try}"
        init_try=$(( init_try + 1 ))
    done
    if [[ "${init_ok}" -eq 1 ]]; then
        record_case "${result_file}" "initialize" 1
    else
        record_case "${result_file}" "initialize" 0
        echo "INIT_HTTP=${http_st}" >> "${result_file}"
        echo "INIT_SESSION=${session}" >> "${result_file}"
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
        "${body}" "${hdr}" "${jwt}" "${session}" "")
    if [[ "${http_st}" == "202" ]]; then
        record_case "${result_file}" "initialized_202" 1
    else
        record_case "${result_file}" "initialized_202" 0
        echo "INITIALIZED_HTTP=${http_st}" >> "${result_file}"
    fi

    # shellcheck disable=SC2310 # ping scored from helper
    if http_st=$(mcp_expect_jq "${mcp_url}" "${jwt}" "${session}" \
        '{"jsonrpc":"2.0","id":2,"method":"ping"}' \
        "${result_file}.ping.json" "${hdr}" \
        '.result != null and .error == null'); then
        record_case "${result_file}" "ping" 1
    else
        record_case "${result_file}" "ping" 0
        echo "PING_HTTP=${http_st}" >> "${result_file}"
    fi

    local list_ok=0
    local tools_filter='
            ([.result.tools[].name] | index("Mcp.Echo"))
            and ([.result.tools[].name] | index("Mcp.EchoStrict"))
            and ([.result.tools[].name] | index("System.Info"))
            and ([.result.tools[] | select(.name=="System.Info") | .inputSchema] | length > 0)
            and ([.result.tools[] | select(.name=="System.Info") | .annotations.title] | .[0] == "System.Info")
            and ([.result.tools[].name] | index("Mcp.Server") | not)
            and ([.result.tools[].name] | index("Mcp.Helpers") | not)
            and ([.result.tools[].name] | index("Mcp.Info") | not)
            and ([.result.tools[].name] | index("Mcp.Intro") | not)
        '
    # shellcheck disable=SC2310 # list_ok is scored from the helper return
    if http_st=$(mcp_expect_jq "${mcp_url}" "${jwt}" "${session}" \
        '{"jsonrpc":"2.0","id":3,"method":"tools/list","params":{}}' \
        "${body}" "${hdr}" "${tools_filter}"); then
        list_ok=1
    fi
    if [[ "${list_ok}" -eq 1 ]]; then
        record_case "${result_file}" "tools_list" 1
    else
        record_case "${result_file}" "tools_list" 0
        echo "LIST_HTTP=${http_st}" >> "${result_file}"
    fi

    # shellcheck disable=SC2310 # cursor scored from helper
    if http_st=$(mcp_expect_jq "${mcp_url}" "${jwt}" "${session}" \
        '{"jsonrpc":"2.0","id":3,"method":"tools/list","params":{"cursor":"1"}}' \
        "${result_file}.cursor.json" "${hdr}" \
        '.result.tools != null and .error == null'); then
        record_case "${result_file}" "tools_list_cursor" 1
    else
        record_case "${result_file}" "tools_list_cursor" 0
        echo "LIST_CURSOR_HTTP=${http_st}" >> "${result_file}"
    fi

    if [[ "${list_ok}" -ne 1 ]]; then
        echo "ENGINE_COMPLETE=1" >> "${result_file}"
        # shellcheck disable=SC2310 # Shutdown best-effort
        scripting_shutdown_instance "${hydrogen_pid}" "${SHUTDOWN_TIMEOUT}" || true
        return 0
    fi

    # shellcheck disable=SC2310 # echo_ok scored from helper
    if http_st=$(mcp_expect_jq "${mcp_url}" "${jwt}" "${session}" \
        '{"jsonrpc":"2.0","id":4,"method":"tools/call","params":{"name":"Mcp.Echo","arguments":{"message":"hello"}}}' \
        "${result_file}.echo.json" "${hdr}" \
        '.result.content != null and .result.isError != true'); then
        record_case "${result_file}" "echo_ok" 1
    else
        record_case "${result_file}" "echo_ok" 0
        echo "ECHO_HTTP=${http_st}" >> "${result_file}"
    fi

    # shellcheck disable=SC2310 # echostrict scored from helper
    if http_st=$(mcp_expect_jq "${mcp_url}" "${jwt}" "${session}" \
        '{"jsonrpc":"2.0","id":5,"method":"tools/call","params":{"name":"Mcp.EchoStrict","arguments":{"nope":true}}}' \
        "${result_file}.strict.json" "${hdr}" \
        '.result.isError == true and .error == null'); then
        record_case "${result_file}" "echostrict_iserror" 1
    else
        record_case "${result_file}" "echostrict_iserror" 0
        echo "STRICT_HTTP=${http_st}" >> "${result_file}"
    fi

    # shellcheck disable=SC2310 # unknown tool scored from helper
    if http_st=$(mcp_expect_jq "${mcp_url}" "${jwt}" "${session}" \
        '{"jsonrpc":"2.0","id":6,"method":"tools/call","params":{"name":"No.SuchTool","arguments":{}}}' \
        "${result_file}.unk_tool.json" "${hdr}" \
        '.result.isError == true and .error == null'); then
        record_case "${result_file}" "unknown_tool_hidden" 1
    else
        record_case "${result_file}" "unknown_tool_hidden" 0
        echo "UNKNOWN_TOOL_HTTP=${http_st}" >> "${result_file}"
    fi

    # shellcheck disable=SC2310 # non_mcp scored from helper
    if http_st=$(mcp_expect_jq "${mcp_url}" "${jwt}" "${session}" \
        '{"jsonrpc":"2.0","id":7,"method":"tools/call","params":{"name":"Orchestrators.Orchestrator","arguments":{}}}' \
        "${result_file}.nonmcp.json" "${hdr}" \
        '.result.isError == true and .error == null'); then
        record_case "${result_file}" "non_mcp_hidden" 1
    else
        record_case "${result_file}" "non_mcp_hidden" 0
        echo "NON_MCP_HTTP=${http_st}" >> "${result_file}"
    fi

    # shellcheck disable=SC2310 # systeminfo scored from helper
    if http_st=$(mcp_expect_jq "${mcp_url}" "${jwt}" "${session}" \
        '{"jsonrpc":"2.0","id":7,"method":"tools/call","params":{"name":"System.Info","arguments":{}}}' \
        "${result_file}.sysinfo.json" "${hdr}" \
        '.result.content != null and .result.structuredContent.version != null and .result.isError != true'); then
        record_case "${result_file}" "system_info_ok" 1
    else
        record_case "${result_file}" "system_info_ok" 0
        echo "SYSINFO_HTTP=${http_st}" >> "${result_file}"
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
        '{"jsonrpc":"2.0","id":8,"method":"tools/call","params":{"name":"Mcp.Echo","arguments":{"_hydrogen":{"x":1}}}}' \
        "${body}" "${hdr}" "${jwt}" "${session}" "" 10)
    if [[ "${http_st}" == "401" ]] \
        || { [[ "${http_st}" == "200" ]] && jq -e '.result.isError == true or .error != null' "${body}" >/dev/null 2>&1; }; then
        record_case "${result_file}" "hydrogen_args_rejected" 1
    else
        record_case "${result_file}" "hydrogen_args_rejected" 0
        echo "HYDROGEN_ARGS_HTTP=${http_st}" >> "${result_file}"
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
    local st_a st_b overlap_try=1 overlap_ok=0
    while [[ "${overlap_try}" -le 3 ]]; do
        ( mcp_http "POST" "${mcp_url}" \
            '{"jsonrpc":"2.0","id":11,"method":"tools/call","params":{"name":"Mcp.Echo","arguments":{"n":1}}}' \
            "${echo_a}" "${hdr_a}" "${jwt}" "${session}" "" \
            | tail -1 > "${result_file}.st_a" ) &
        local pid_a=$!
        ( mcp_http "POST" "${mcp_url}" \
            '{"jsonrpc":"2.0","id":12,"method":"tools/call","params":{"name":"Mcp.Echo","arguments":{"n":2}}}' \
            "${echo_b}" "${hdr_b}" "${jwt}" "${session}" "" \
            | tail -1 > "${result_file}.st_b" ) &
        local pid_b=$!
        wait "${pid_a}" || true
        wait "${pid_b}" || true
        st_a=$(tail -1 "${result_file}.st_a" 2>/dev/null || echo 000)
        st_b=$(tail -1 "${result_file}.st_b" 2>/dev/null || echo 000)
        if [[ "${st_a}" == "200" && "${st_b}" == "200" ]] \
            && jq -e '.result.isError != true' "${echo_a}" >/dev/null 2>&1 \
            && jq -e '.result.isError != true' "${echo_b}" >/dev/null 2>&1; then
            overlap_ok=1
            break
        fi
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" \
            "INFO delay overlap Echo HTTP ${st_a},${st_b} try ${overlap_try}/3"
        sleep 1
        overlap_try=$(( overlap_try + 1 ))
    done
    if [[ "${overlap_ok}" -eq 1 ]]; then
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

    # --- Phase 15 resources / prompts (1375 applied on all engines) ---
    # shellcheck disable=SC2310 # resources_list is scored from the helper return
    if http_st=$(mcp_expect_jq "${mcp_url}" "${jwt}" "${session}" \
        '{"jsonrpc":"2.0","id":20,"method":"resources/list","params":{}}' \
        "${body}" "${hdr}" \
        '([.result.resources[].uri] | index("hydrogen://mcp/info"))'); then
        record_case "${result_file}" "resources_list" 1
    else
        record_case "${result_file}" "resources_list" 0
        echo "RESOURCES_LIST_HTTP=${http_st}" >> "${result_file}"
    fi

    # shellcheck disable=SC2310 # resources_read scored from helper
    if http_st=$(mcp_expect_jq "${mcp_url}" "${jwt}" "${session}" \
        '{"jsonrpc":"2.0","id":21,"method":"resources/read","params":{"uri":"hydrogen://mcp/info"}}' \
        "${result_file}.res_read.json" "${hdr}" \
        '.result.contents[0].text != null and .error == null'); then
        record_case "${result_file}" "resources_read" 1
    else
        record_case "${result_file}" "resources_read" 0
        echo "RESOURCES_READ_HTTP=${http_st}" >> "${result_file}"
    fi

    # shellcheck disable=SC2310 # scored from helper return
    if http_st=$(mcp_expect_jq "${mcp_url}" "${jwt}" "${session}" \
        '{"jsonrpc":"2.0","id":22,"method":"resources/read","params":{"uri":"hydrogen://mcp/no-such"}}' \
        "${body}" "${hdr}" \
        '.error.code == -32602'); then
        record_case "${result_file}" "resources_unknown" 1
    else
        record_case "${result_file}" "resources_unknown" 0
        echo "RESOURCES_UNK_HTTP=${http_st}" >> "${result_file}"
    fi

    # shellcheck disable=SC2310 # prompts_list is scored from the helper return
    if http_st=$(mcp_expect_jq "${mcp_url}" "${jwt}" "${session}" \
        '{"jsonrpc":"2.0","id":23,"method":"prompts/list","params":{}}' \
        "${body}" "${hdr}" \
        '([.result.prompts[].name] | index("Mcp.Intro"))'); then
        record_case "${result_file}" "prompts_list" 1
    else
        record_case "${result_file}" "prompts_list" 0
        echo "PROMPTS_LIST_HTTP=${http_st}" >> "${result_file}"
    fi

    # shellcheck disable=SC2310 # prompts_get scored from helper
    if http_st=$(mcp_expect_jq "${mcp_url}" "${jwt}" "${session}" \
        '{"jsonrpc":"2.0","id":24,"method":"prompts/get","params":{"name":"Mcp.Intro","arguments":{"topic":"Echo"}}}' \
        "${result_file}.prompts_get.json" "${hdr}" \
        '.result.messages[0].content.text != null and .error == null'); then
        record_case "${result_file}" "prompts_get" 1
    else
        record_case "${result_file}" "prompts_get" 0
        echo "PROMPTS_GET_HTTP=${http_st}" >> "${result_file}"
    fi

    # shellcheck disable=SC2310 # prompts_unknown scored from helper
    if http_st=$(mcp_expect_jq "${mcp_url}" "${jwt}" "${session}" \
        '{"jsonrpc":"2.0","id":25,"method":"prompts/get","params":{"name":"No.SuchPrompt"}}' \
        "${result_file}.prompts_unk.json" "${hdr}" \
        '.error.code == -32602'); then
        record_case "${result_file}" "prompts_unknown" 1
    else
        record_case "${result_file}" "prompts_unknown" 0
        echo "PROMPTS_UNK_HTTP=${http_st}" >> "${result_file}"
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

    local pass_n fail_n
    pass_n=$("${GREP}" -c "^CASE_PASS=" "${result_file}" 2>/dev/null || echo 0)
    fail_n=$("${GREP}" -c "^CASE_FAIL=" "${result_file}" 2>/dev/null || echo 0)
    pass_n=${pass_n//[^0-9]/}
    fail_n=${fail_n//[^0-9]/}
    pass_n=${pass_n:-0}
    fail_n=${fail_n:-0}

    local min_pass=36
    if [[ "${description}" == "SQLite" ]]; then
        min_pass=37
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
