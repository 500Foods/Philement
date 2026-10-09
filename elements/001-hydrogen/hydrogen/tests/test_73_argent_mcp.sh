#!/usr/bin/env bash

# Test: Argent MCP tools (Argent Phases 11, 12, and 14)
# Eight engines. PAYLOAD:acuranzo+argent on the demo connections.
# AutoMigration applies argent_2030.lua–argent_2048.lua when the payload has them.

# FUNCTIONS
# (Tool calls live in tests/lib/argent_mcp_helpers.sh)
# run_engine()
# analyze_engine()

# CHANGELOG
# 1.0.0 - 2026-10-07 - Argent MCP tools and validation variants on 8 engines
# 1.0.1 - 2026-10-07 - Report the root tool error instead of every prereq miss
# 1.0.2 - 2026-10-07 - One result per config file, so validation does not warn
# 1.0.3 - 2026-10-08 - Confirm tokens, edits, rescinds, statements, and reconciliation
# 1.0.4 - 2026-10-08 - Helper retries one HTTP 503
# 1.0.5 - 2026-10-08 - Schedules, reserved rows, and a down calendar host

set -euo pipefail

TEST_NAME="Argent MCP"
TEST_ABBR="ARG"
TEST_NUMBER="73"
TEST_COUNTER=0
TEST_VERSION="1.0.5"

# shellcheck source=tests/lib/framework.sh # Reference framework directly
[[ -n "${FRAMEWORK_GUARD:-}" ]] || source "$(dirname "${BASH_SOURCE[0]}")/lib/framework.sh"
setup_test_environment

# shellcheck source=tests/lib/scripting_helpers.sh # Start/shutdown helpers shared with test_43
source "$(dirname "${BASH_SOURCE[0]}")/lib/scripting_helpers.sh"
# shellcheck source=tests/lib/mcp_helpers.sh # HTTP, JWT, and case recording
source "$(dirname "${BASH_SOURCE[0]}")/lib/mcp_helpers.sh"
# shellcheck source=tests/lib/argent_mcp_helpers.sh # Argent tool calls
source "$(dirname "${BASH_SOURCE[0]}")/lib/argent_mcp_helpers.sh"

declare -a PARALLEL_PIDS
declare -A SCRIPT_TEST_CONFIGS

# config:log_suffix:engine_key:description
SCRIPT_TEST_CONFIGS=(
    ["PostgreSQL"]="${SCRIPT_DIR}/configs/hydrogen_test_${TEST_NUMBER}_argent_mcp_postgres.json:postgres:postgresql:PostgreSQL"
    ["MySQL"]="${SCRIPT_DIR}/configs/hydrogen_test_${TEST_NUMBER}_argent_mcp_mysql.json:mysql:mysql:MySQL"
    ["SQLite"]="${SCRIPT_DIR}/configs/hydrogen_test_${TEST_NUMBER}_argent_mcp_sqlite.json:sqlite:sqlite:SQLite"
    ["DB2"]="${SCRIPT_DIR}/configs/hydrogen_test_${TEST_NUMBER}_argent_mcp_db2.json:db2:db2:DB2"
    ["MariaDB"]="${SCRIPT_DIR}/configs/hydrogen_test_${TEST_NUMBER}_argent_mcp_mariadb.json:mariadb:mariadb:MariaDB"
    ["Firebird"]="${SCRIPT_DIR}/configs/hydrogen_test_${TEST_NUMBER}_argent_mcp_firebird.json:firebird:firebird:Firebird"
    ["YugabyteDB"]="${SCRIPT_DIR}/configs/hydrogen_test_${TEST_NUMBER}_argent_mcp_yugabytedb.json:yugabytedb:yugabytedb:YugabyteDB"
    ["MSSQL"]="${SCRIPT_DIR}/configs/hydrogen_test_${TEST_NUMBER}_argent_mcp_mssql.json:mssql:mssql:MSSQL"
)

# Applying the Argent pack on a demo database can outlast the group-40 ready wait.
READY_TIMEOUT=300
# shellcheck disable=SC2034 # Reserved for log messages / future fail-fast bounds
STARTUP_TIMEOUT="${GROUP40_STARTUP_TIMEOUT}"
SHUTDOWN_TIMEOUT="${GROUP40_SHUTDOWN_TIMEOUT}"
HTTP_TIMEOUT="${GROUP40_HTTP_MAX_TIME}"
BASELINE_SQLITE="${PROJECT_DIR}/tests/artifacts/database/sqlite/hydrodemo.sqlite"

# Fixed cases recorded in this file: login, api_status, initialize, initialized_202, shutdown_clean.
ARGENT_FIXED_CASES=5

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
    if [[ "${engine_key}" == "sqlite" ]]; then
        work_dir="${DIAG_TEST_DIR}/sqlite_${engine_key}_$$"
        # shellcheck disable=SC2310 # Capture STARTUP_FAILED when SQLite copy fails
        run_config=$(argent_prepare_sqlite "${config_file}" "${work_dir}") || {
            echo "STARTUP_FAILED=1" >> "${result_file}"
            echo "REASON=sqlite_copy" >> "${result_file}"
            return 0
        }
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
    # shellcheck disable=SC2310 # Later HTTP retries still cover bind lag
    wait_http "${base_url}/api/version" "${GROUP40_READY_TIMEOUT}" || true
    # shellcheck disable=SC2310 # MCP bind can lag WebServer under suite load
    wait_http "${mcp_url}/healthz" "${GROUP40_READY_TIMEOUT}" || true

    local body hdr http_st
    body="${result_file}.body"
    hdr="${result_file}.hdr"

    local login_file="${result_file}.login.json"
    local login_payload
    login_payload=$(jq -n \
        --arg login_id "${HYDROGEN_DEMO_USER_NAME}" \
        --arg password "${HYDROGEN_DEMO_USER_PASS}" \
        --arg api_key "${HYDROGEN_DEMO_API_KEY}" \
        '{database:"Acuranzo",login_id:$login_id,password:$password,api_key:$api_key,tz:"America/Vancouver"}')
    local jwt=""
    local login_try=1
    local login_max=2
    while [[ "${login_try}" -le "${login_max}" ]]; do
        # shellcheck disable=SC2312 # curl exit ignored; HTTP status is the signal
        http_st=$(api_request "POST" "${base_url}/api/auth/login" "${login_payload}" "${login_file}" "" \
            "${GROUP40_HTTP_MAX_TIME}")
        jwt=$(extract_jwt "${login_file}")
        if [[ "${http_st}" == "200" && -n "${jwt}" ]]; then
            break
        fi
        if [[ "${http_st}" == "503" && "${login_max}" -lt 4 ]]; then
            login_max=4
        fi
        if [[ "${login_try}" -ge "${login_max}" ]]; then
            break
        fi
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" \
            "INFO delay ${description}: login HTTP ${http_st} (try ${login_try}/${login_max})"
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

    local init_body='{"jsonrpc":"2.0","id":1,"method":"initialize","params":{"protocolVersion":"2025-06-18","capabilities":{},"clientInfo":{"name":"test73","version":"1.0.0"}}}'
    local session=""
    local init_ok=0
    local init_try=1
    while [[ "${init_try}" -le 5 ]]; do
        http_st=$(mcp_http "POST" "${mcp_url}" "${init_body}" "${body}" "${hdr}" "${jwt}" "" "")
        session=$(header_value "${hdr}" "Mcp-Session-Id")
        init_ok=0
        if [[ "${http_st}" == "200" && -n "${session}" ]] \
            && jq -e '.result.serverInfo.name == "hydrogen"' "${body}" >/dev/null 2>&1; then
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
    fi

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

    argent_mcp_exercise "${result_file}" "${mcp_url}" "${jwt}" "${session}" "${hdr}" "${engine_key}"

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
    local tool_n pass_n fail_n expected root_fail prereq_n

    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}: Argent MCP tools"
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

    tool_n=$("${GREP}" "^EXPECTED_TOOL_CASES=" "${result_file}" 2>/dev/null | head -1 | cut -d= -f2 || true)
    tool_n=${tool_n//[^0-9]/}
    pass_n=$("${GREP}" -c "^CASE_PASS=" "${result_file}" 2>/dev/null || echo 0)
    fail_n=$("${GREP}" -c "^CASE_FAIL=" "${result_file}" 2>/dev/null || echo 0)
    pass_n=${pass_n//[^0-9]/}
    fail_n=${fail_n//[^0-9]/}
    pass_n=${pass_n:-0}
    fail_n=${fail_n:-0}
    tool_n=${tool_n:-0}
    expected=$(( tool_n + ARGENT_FIXED_CASES ))

    if [[ "${tool_n}" -gt 0 && "${fail_n}" -eq 0 && "${pass_n}" -eq "${expected}" ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 \
            "${description}: ${pass_n} cases passed"
        PASS_COUNT=$(( PASS_COUNT + 1 ))
    else
        root_fail=$("${GREP}" "^FAIL_" "${result_file}" 2>/dev/null | "${GREP}" -v '=prereq$' | head -1 | tr '\n' ' ' || true)
        prereq_n=$("${GREP}" -c "=prereq$" "${result_file}" 2>/dev/null || echo 0)
        prereq_n=${prereq_n//[^0-9]/}
        prereq_n=${prereq_n:-0}
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 \
            "${description}: pass=${pass_n} fail=${fail_n} expected=${expected} prereq=${prereq_n} ${root_fail}"
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
    if [[ -f "${config_file}" ]]; then
        port=$(get_webserver_port "${config_file}")
        mcp_port=$(jq -r '.MCP.Port // empty' "${config_file}")
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 \
            "${description} config valid (web ${port} mcp ${mcp_port})"
        PASS_COUNT=$(( PASS_COUNT + 1 ))
    else
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${description} config not found: ${config_file}"
        config_valid=false
        EXIT_CODE=1
    fi
done

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Validate Configuration Files"
if [[ "${config_valid}" = true ]]; then
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "All ${#SCRIPT_TEST_CONFIGS[@]} engine configs validated"
    PASS_COUNT=$(( PASS_COUNT + 1 ))
else
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Configuration file validation failed"
    EXIT_CODE=1
fi

if [[ "${EXIT_CODE}" -eq 0 ]]; then
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Running Argent MCP blackbox in parallel"

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
    done

    print_marker "${TEST_NUMBER}" "${TEST_COUNTER}"
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "All eight engines"
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" \
        "Summary: ${successful}/${#SCRIPT_TEST_CONFIGS[@]} engines passed every Argent tool case"
    if [[ "${successful}" -eq "${#SCRIPT_TEST_CONFIGS[@]}" ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "All eight engines passed"
        PASS_COUNT=$(( PASS_COUNT + 1 ))
    else
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 \
            "${successful}/${#SCRIPT_TEST_CONFIGS[@]} engines passed"
        EXIT_CODE=1
    fi
else
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Skipping Argent MCP tests due to prerequisite failures"
    EXIT_CODE=1
fi

print_test_completion "${TEST_NAME}" "${TEST_ABBR}" "${TEST_NUMBER}" "${TEST_VERSION}"
${ORCHESTRATION:-false} && return "${EXIT_CODE}" || exit "${EXIT_CODE}"
