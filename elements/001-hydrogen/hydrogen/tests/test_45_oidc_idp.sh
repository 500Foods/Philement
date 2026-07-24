#!/usr/bin/env bash

# Test: OIDC Identity Provider — multi-engine parallel blackbox
# Drives Hydrogen as IdP across 7 DBs: discovery, JWKS, authorize login,
# PKCE token, userinfo, refresh, error paths (inverse of Test 42).

# CHANGELOG
# 2.0.0 - 2026-07-23 - Multi-engine parallel (5450-5456), test_40 pattern
# 1.0.2 - 2026-07-23 - Phase 15: state required, bad scheme, missing nonce
# 1.0.1 - 2026-07-23 - Shellcheck clean
# 1.0.0 - 2026-07-23 - Initial Phase 14 SQLite-only

set -euo pipefail

TEST_NAME="OIDC Identity Provider"
TEST_ABBR="IDP"
TEST_NUMBER="45"
TEST_COUNTER=0
TEST_VERSION="2.0.0"

# shellcheck source=tests/lib/framework.sh # Resolve path at runtime via BASH_SOURCE
[[ -n "${FRAMEWORK_GUARD:-}" ]] || source "$(dirname "${BASH_SOURCE[0]}")/lib/framework.sh"
setup_test_environment

# shellcheck source=tests/lib/oidc_idp_helpers.sh # Helpers live next to other test libs
source "${SCRIPT_DIR}/lib/oidc_idp_helpers.sh"

CONFIG_DISABLED="${SCRIPT_DIR}/configs/hydrogen_test_${TEST_NUMBER}_oidc_idp_disabled.json"

CLIENT_ID="test-idp-public"
REDIRECT_URI="http://127.0.0.1:5459/cb"
DEMO_USER_NAME="${HYDROGEN_DEMO_USER_NAME:-}"
DEMO_USER_PASS="${HYDROGEN_DEMO_USER_PASS:-}"

STARTUP_TIMEOUT=20
SHUTDOWN_TIMEOUT=15
MIGRATION_TIMEOUT=180

declare -a PARALLEL_PIDS
declare -A IDP_TEST_CONFIGS

# format: "config_file:log_suffix:engine_name:description"
IDP_TEST_CONFIGS=(
    ["PostgreSQL"]="${SCRIPT_DIR}/configs/hydrogen_test_${TEST_NUMBER}_postgres.json:postgres:postgresql:PostgreSQL Engine"
    ["MySQL"]="${SCRIPT_DIR}/configs/hydrogen_test_${TEST_NUMBER}_mysql.json:mysql:mysql:MySQL Engine"
    ["SQLite"]="${SCRIPT_DIR}/configs/hydrogen_test_${TEST_NUMBER}_sqlite.json:sqlite:sqlite:SQLite Engine"
    ["DB2"]="${SCRIPT_DIR}/configs/hydrogen_test_${TEST_NUMBER}_db2.json:db2:db2:DB2 Engine"
    ["MariaDB"]="${SCRIPT_DIR}/configs/hydrogen_test_${TEST_NUMBER}_mariadb.json:mariadb:mariadb:MariaDB Engine"
    ["CockroachDB"]="${SCRIPT_DIR}/configs/hydrogen_test_${TEST_NUMBER}_cockroachdb.json:cockroachdb:cockroachdb:CockroachDB Engine"
    ["YugabyteDB"]="${SCRIPT_DIR}/configs/hydrogen_test_${TEST_NUMBER}_yugabytedb.json:yugabytedb:yugabytedb:YugabyteDB Engine"
)

pass_subtest() {
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "$1"
    PASS_COUNT=$(( PASS_COUNT + 1 ))
}

fail_subtest() {
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "$1"
    EXIT_CODE=1
}

wait_idp_ready_log() {
    local log_file="$1"
    local timeout="${2:-120}"
    local deadline now
    now="$(date +%s)"
    deadline=$(( now + timeout ))
    while true; do
        now="$(date +%s)"
        if [[ "${now}" -ge ${deadline} ]]; then
            return 1
        fi
        if "${GREP}" -q -E "READY FOR REQUESTS|Migration completed in|Migration Current:" "${log_file}" 2>/dev/null; then
            sleep 1
            return 0
        fi
        sleep 0.3
    done
}

wait_idp_http_ready() {
    local base_url="$1"
    local max_attempts="${2:-60}"
    local attempt=1
    local code
    while [[ "${attempt}" -le "${max_attempts}" ]]; do
        code="$(curl -sS -o /dev/null -w '%{http_code}' "${base_url}/.well-known/openid-configuration" 2>/dev/null || echo "000")"
        if [[ "${code}" == "200" ]]; then
            return 0
        fi
        sleep 0.25
        attempt=$(( attempt + 1 ))
    done
    return 1
}

# Run full IdP suite for one engine; write markers to result_file
run_idp_test_parallel() {
    local test_name="$1"
    local config_file="$2"
    local log_suffix="$3"
    local engine_name="$4"
    local description="$5"

    local log_file="${LOGS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}_${log_suffix}.log"
    local result_file="${LOG_PREFIX}${TIMESTAMP}_${log_suffix}.result"
    local prefix="${LOG_PREFIX}${TIMESTAMP}_${log_suffix}"

    true > "${result_file}"

    local server_port
    server_port=$(jq -r '.WebServer.Port' "${config_file}" 2>/dev/null || echo "0")
    local base_url="http://localhost:${server_port}"

    local actual_config_file="${config_file}"
    local sqlite_temp_file=""
    local sqlite_temp_config=""
    if [[ "${engine_name}" == "sqlite" ]]; then
        local baseline_sqlite
        baseline_sqlite=$(jq -r '.Databases.Connections[0].Database' "${config_file}" 2>/dev/null)
        if [[ -n "${baseline_sqlite}" && "${baseline_sqlite}" != "null" && -f "${baseline_sqlite}" ]]; then
            sqlite_temp_file="${DIAG_TEST_DIR}/hydrodemo_${TIMESTAMP}_${log_suffix}.sqlite"
            sqlite_temp_config="${DIAG_TEST_DIR}/hydrogen_test_${TEST_NUMBER}_sqlite_${TIMESTAMP}_${log_suffix}.json"
            if cp "${baseline_sqlite}" "${sqlite_temp_file}" 2>/dev/null && \
               jq --arg db "${sqlite_temp_file}" \
                  '.Databases.Connections[0].Database = $db' \
                  "${config_file}" > "${sqlite_temp_config}" 2>/dev/null; then
                actual_config_file="${sqlite_temp_config}"
            else
                sqlite_temp_file=""
                sqlite_temp_config=""
                actual_config_file="${config_file}"
            fi
        fi
    fi

    mkdir -p "tests/artifacts/oidc_idp_keys_45_${log_suffix}" 2>/dev/null || true

    HYDROGEN_LOG_LEVEL=STATE "${HYDROGEN_BIN}" "${actual_config_file}" > "${log_file}" 2>&1 &
    local hydrogen_pid=$!
    if declare -f register_hydrogen_pid >/dev/null 2>&1; then
        register_hydrogen_pid "${hydrogen_pid}"
    fi
    echo "PID=${hydrogen_pid}" >> "${result_file}"

    local startup_success=false
    local start_time
    start_time=${SECONDS}
    while true; do
        if [[ $((SECONDS - start_time)) -ge "${STARTUP_TIMEOUT}" ]]; then
            break
        fi
        if "${GREP}" -q "Startup elapsed time:" "${log_file}" 2>/dev/null; then
            startup_success=true
            break
        fi
        sleep 0.1
    done

    if [[ "${startup_success}" != true ]]; then
        echo "STARTUP_FAILED" >> "${result_file}"
        kill -9 "${hydrogen_pid}" 2>/dev/null || true
        rm -f "${sqlite_temp_file}" "${sqlite_temp_config}" 2>/dev/null || true
        return 0
    fi
    echo "STARTUP_SUCCESS" >> "${result_file}"

    local migration_start_time=${SECONDS}
    while true; do
        if [[ $((SECONDS - migration_start_time)) -ge ${MIGRATION_TIMEOUT} ]]; then
            break
        fi
        if "${GREP}" -q "Migration completed in" "${log_file}" 2>/dev/null; then
            break
        fi
        if "${GREP}" -q "Migration Current:" "${log_file}" 2>/dev/null; then
            break
        fi
        sleep 0.5
    done

    # shellcheck disable=SC2310 # Continue even if readiness fails
    if ! wait_idp_http_ready "${base_url}" 90; then
        echo "READY_FAILED" >> "${result_file}"
        if ps -p "${hydrogen_pid}" > /dev/null 2>&1; then
            kill -SIGINT "${hydrogen_pid}" 2>/dev/null || true
            sleep 1
            kill -9 "${hydrogen_pid}" 2>/dev/null || true
        fi
        rm -f "${sqlite_temp_file}" "${sqlite_temp_config}" 2>/dev/null || true
        return 0
    fi
    echo "READY_OK" >> "${result_file}"

    local code disc_file jwks_file
    disc_file="${prefix}_discovery.json"
    code="$(oidc_idp_fetch_discovery "${base_url}" "${disc_file}")"
    if [[ "${code}" == "200" ]] \
        && jq -e '.issuer and .authorization_endpoint and .token_endpoint and .jwks_uri' "${disc_file}" >/dev/null 2>&1 \
        && jq -e '.code_challenge_methods_supported | index("S256")' "${disc_file}" >/dev/null 2>&1 \
        && jq -e '.id_token_signing_alg_values_supported | index("RS256")' "${disc_file}" >/dev/null 2>&1 \
        && jq -e '.introspection_endpoint and .revocation_endpoint' "${disc_file}" >/dev/null 2>&1; then
        echo "DISCOVERY_OK" >> "${result_file}"
    else
        echo "DISCOVERY_FAIL" >> "${result_file}"
    fi

    jwks_file="${prefix}_jwks.json"
    code="$(oidc_idp_fetch_jwks "${base_url}" "${jwks_file}")"
    if [[ "${code}" == "200" ]] \
        && jq -e '.keys | length >= 1' "${jwks_file}" >/dev/null 2>&1 \
        && jq -e '.keys[0].n and .keys[0].e and .keys[0].kid' "${jwks_file}" >/dev/null 2>&1; then
        echo "JWKS_OK" >> "${result_file}"
    else
        echo "JWKS_FAIL" >> "${result_file}"
    fi

    code="$(curl -sS -o "${prefix}_auth_bad.json" -w '%{http_code}' \
        "${base_url}/oauth/authorize" 2>/dev/null || echo "000")"
    if [[ "${code}" != "200" ]] || grep -qi "error\|invalid" "${prefix}_auth_bad.json" 2>/dev/null; then
        echo "AUTH_BAD_PARAMS_OK" >> "${result_file}"
    else
        body="$(cat "${prefix}_auth_bad.json" 2>/dev/null || true)"
        if [[ -z "${body}" || "${body}" == *invalid* || "${body}" == *error* ]]; then
            echo "AUTH_BAD_PARAMS_OK" >> "${result_file}"
        else
            echo "AUTH_BAD_PARAMS_FAIL" >> "${result_file}"
        fi
    fi

    local auth_code="" access_token="" id_token="" refresh_token="" state nonce
    # shellcheck disable=SC2310 # PKCE helper may fail
    if oidc_idp_pkce_pair; then
        state="st-${log_suffix}-$(openssl rand -hex 4)"
        nonce="n-${log_suffix}-$(openssl rand -hex 4)"
        # shellcheck disable=SC2310 # authorize may fail per engine
        if oidc_idp_authorize_login "${base_url}" "${CLIENT_ID}" "${REDIRECT_URI}" \
            "openid offline_access" "${state}" "${nonce}" \
            "${OIDC_IDP_CODE_CHALLENGE}" "${DEMO_USER_NAME}" "${DEMO_USER_PASS}"; then
            auth_code="${OIDC_IDP_AUTH_CODE}"
            if [[ -n "${auth_code}" ]]; then
                echo "LOGIN_OK" >> "${result_file}"
                if [[ "${OIDC_IDP_STATE_OUT}" == "${state}" ]]; then
                    echo "STATE_OK" >> "${result_file}"
                else
                    echo "STATE_MISMATCH" >> "${result_file}"
                fi
            else
                echo "LOGIN_NO_CODE" >> "${result_file}"
            fi
        else
            echo "LOGIN_FAIL" >> "${result_file}"
        fi
    else
        echo "PKCE_FAIL" >> "${result_file}"
    fi

    if [[ -n "${auth_code}" ]]; then
        local tok_file="${prefix}_token1.json"
        code="$(oidc_idp_token_code "${base_url}" "${CLIENT_ID}" "${REDIRECT_URI}" \
            "${auth_code}" "${OIDC_IDP_CODE_VERIFIER}" "${tok_file}")"
        if [[ "${code}" == "200" ]] \
            && jq -e '.access_token and .id_token and .token_type' "${tok_file}" >/dev/null 2>&1; then
            access_token="$(jq -r '.access_token' "${tok_file}")"
            id_token="$(jq -r '.id_token' "${tok_file}")"
            refresh_token="$(jq -r '.refresh_token // empty' "${tok_file}")"
            echo "TOKEN_OK" >> "${result_file}"
        else
            echo "TOKEN_FAIL" >> "${result_file}"
        fi
    fi

    if [[ -n "${access_token}" ]]; then
        local ui_file="${prefix}_userinfo.json"
        code="$(oidc_idp_userinfo "${base_url}" "${access_token}" "${ui_file}")"
        if [[ "${code}" == "200" ]] && jq -e '.sub' "${ui_file}" >/dev/null 2>&1; then
            echo "USERINFO_OK" >> "${result_file}"
        else
            echo "USERINFO_FAIL" >> "${result_file}"
        fi
    fi

    if [[ -n "${id_token}" ]]; then
        local payload_json
        payload_json="$(oidc_idp_jwt_payload_json "${id_token}")"
        if printf '%s' "${payload_json}" | jq -e --arg n "${nonce}" \
            '.iss and .sub and .nonce == $n' >/dev/null 2>&1; then
            echo "ID_TOKEN_OK" >> "${result_file}"
        else
            echo "ID_TOKEN_FAIL" >> "${result_file}"
        fi
    fi

    if [[ -n "${refresh_token}" ]]; then
        local tok2="${prefix}_token2.json"
        code="$(oidc_idp_token_refresh "${base_url}" "${CLIENT_ID}" "${refresh_token}" "${tok2}")"
        local new_rt
        new_rt="$(jq -r '.refresh_token // empty' "${tok2}" 2>/dev/null || true)"
        if [[ "${code}" == "200" && -n "${new_rt}" && "${new_rt}" != "${refresh_token}" ]]; then
            echo "REFRESH_OK" >> "${result_file}"
            local tok3="${prefix}_token3.json"
            code="$(oidc_idp_token_refresh "${base_url}" "${CLIENT_ID}" "${refresh_token}" "${tok3}")"
            if [[ "${code}" != "200" ]] || jq -e '.error' "${tok3}" >/dev/null 2>&1; then
                echo "REFRESH_REUSE_OK" >> "${result_file}"
            else
                echo "REFRESH_REUSE_FAIL" >> "${result_file}"
            fi
        else
            echo "REFRESH_FAIL" >> "${result_file}"
        fi
    elif [[ -n "${access_token}" ]]; then
        echo "REFRESH_MISSING" >> "${result_file}"
    fi

    if [[ -n "${auth_code}" ]]; then
        local tok_reuse="${prefix}_token_reuse.json"
        code="$(oidc_idp_token_code "${base_url}" "${CLIENT_ID}" "${REDIRECT_URI}" \
            "${auth_code}" "${OIDC_IDP_CODE_VERIFIER}" "${tok_reuse}")"
        if [[ "${code}" != "200" ]] || jq -e '.error' "${tok_reuse}" >/dev/null 2>&1; then
            echo "CODE_REUSE_OK" >> "${result_file}"
        else
            echo "CODE_REUSE_FAIL" >> "${result_file}"
        fi
    fi

    # shellcheck disable=SC2310 # expect failure
    oidc_idp_pkce_pair || true
    # shellcheck disable=SC2310 # expect failure
    if oidc_idp_authorize_login "${base_url}" "${CLIENT_ID}" "http://evil.example/cb" \
        "openid" "s1" "n1" "${OIDC_IDP_CODE_CHALLENGE:-x}" \
        "${DEMO_USER_NAME}" "${DEMO_USER_PASS}"; then
        echo "BAD_REDIRECT_FAIL" >> "${result_file}"
    else
        echo "BAD_REDIRECT_OK" >> "${result_file}"
    fi

    # shellcheck disable=SC2310 # expect failure
    oidc_idp_pkce_pair || true
    # shellcheck disable=SC2310 # expect failure
    if oidc_idp_authorize_login "${base_url}" "${CLIENT_ID}" "javascript:alert(1)" \
        "openid" "s1" "n1" "${OIDC_IDP_CODE_CHALLENGE:-x}" \
        "${DEMO_USER_NAME}" "${DEMO_USER_PASS}"; then
        echo "JS_REDIRECT_FAIL" >> "${result_file}"
    else
        echo "JS_REDIRECT_OK" >> "${result_file}"
    fi

    # shellcheck disable=SC2310 # pkce optional
    oidc_idp_pkce_pair || true
    local miss_state_hdr="${prefix}_miss_state.hdr"
    curl -sS -D "${miss_state_hdr}" -o /dev/null \
        -X POST "${base_url}/oauth/authorize" \
        -H "Content-Type: application/x-www-form-urlencoded" \
        --data-urlencode "client_id=${CLIENT_ID}" \
        --data-urlencode "redirect_uri=${REDIRECT_URI}" \
        --data-urlencode "response_type=code" \
        --data-urlencode "scope=openid" \
        --data-urlencode "nonce=n-missing-state" \
        --data-urlencode "code_challenge=${OIDC_IDP_CODE_CHALLENGE:-x}" \
        --data-urlencode "code_challenge_method=S256" \
        --data-urlencode "username=${DEMO_USER_NAME}" \
        --data-urlencode "password=${DEMO_USER_PASS}" 2>/dev/null || true
    local loc_ms
    loc_ms="$(oidc_idp_location_from_headers "${miss_state_hdr}")"
    if [[ -z "${loc_ms}" || "${loc_ms}" != *code=* ]]; then
        echo "MISS_STATE_OK" >> "${result_file}"
    else
        echo "MISS_STATE_FAIL" >> "${result_file}"
    fi

    # shellcheck disable=SC2310 # pkce optional
    oidc_idp_pkce_pair || true
    local miss_nonce_hdr="${prefix}_miss_nonce.hdr"
    curl -sS -D "${miss_nonce_hdr}" -o /dev/null \
        -X POST "${base_url}/oauth/authorize" \
        -H "Content-Type: application/x-www-form-urlencoded" \
        --data-urlencode "client_id=${CLIENT_ID}" \
        --data-urlencode "redirect_uri=${REDIRECT_URI}" \
        --data-urlencode "response_type=code" \
        --data-urlencode "scope=openid" \
        --data-urlencode "state=st-no-nonce" \
        --data-urlencode "code_challenge=${OIDC_IDP_CODE_CHALLENGE:-x}" \
        --data-urlencode "code_challenge_method=S256" \
        --data-urlencode "username=${DEMO_USER_NAME}" \
        --data-urlencode "password=${DEMO_USER_PASS}" 2>/dev/null || true
    local loc_mn
    loc_mn="$(oidc_idp_location_from_headers "${miss_nonce_hdr}")"
    if [[ -z "${loc_mn}" || "${loc_mn}" != *code=* ]]; then
        echo "MISS_NONCE_OK" >> "${result_file}"
    else
        echo "MISS_NONCE_FAIL" >> "${result_file}"
    fi

    echo "IDP_TEST_COMPLETE" >> "${result_file}"

    if ps -p "${hydrogen_pid}" > /dev/null 2>&1; then
        kill -SIGINT "${hydrogen_pid}" 2>/dev/null || true
        local shutdown_start
        shutdown_start=${SECONDS}
        while ps -p "${hydrogen_pid}" > /dev/null 2>&1; do
            if [[ $((SECONDS - shutdown_start)) -ge "${SHUTDOWN_TIMEOUT}" ]]; then
                kill -9 "${hydrogen_pid}" 2>/dev/null || true
                break
            fi
            sleep 0.1
        done
    fi

    rm -f "${sqlite_temp_file}" "${sqlite_temp_config}" 2>/dev/null || true
}

check_marker() {
    local result_file="$1"
    local ok_marker="$2"
    local fail_markers="$3"
    if "${GREP}" -q "${ok_marker}" "${result_file}" 2>/dev/null; then
        return 0
    fi
    local fm
    for fm in ${fail_markers}; do
        if "${GREP}" -q "${fm}" "${result_file}" 2>/dev/null; then
            return 1
        fi
    done
    return 1
}

analyze_idp_test_results() {
    local test_name="$1"
    local log_suffix="$2"
    local engine_name="$3"
    local description="$4"
    local result_file="${LOG_PREFIX}${TIMESTAMP}_${log_suffix}.result"
    local engine_ok=0

    if [[ ! -f "${result_file}" ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "No result file for ${test_name}"
        return 1
    fi

    if ! "${GREP}" -q "STARTUP_SUCCESS" "${result_file}" 2>/dev/null; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${description}: Failed to start Hydrogen"
        return 1
    fi

    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}: Discovery"
    if "${GREP}" -q "DISCOVERY_OK" "${result_file}" 2>/dev/null; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "${description}: Discovery OK"
        PASS_COUNT=$(( PASS_COUNT + 1 ))
    else
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${description}: Discovery failed"
        engine_ok=1
    fi

    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}: JWKS"
    if "${GREP}" -q "JWKS_OK" "${result_file}" 2>/dev/null; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "${description}: JWKS OK"
        PASS_COUNT=$(( PASS_COUNT + 1 ))
    else
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${description}: JWKS failed"
        engine_ok=1
    fi

    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}: Authorize bad params"
    if "${GREP}" -q "AUTH_BAD_PARAMS_OK" "${result_file}" 2>/dev/null; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "${description}: Bad params rejected"
        PASS_COUNT=$(( PASS_COUNT + 1 ))
    else
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${description}: Bad params check failed"
        engine_ok=1
    fi

    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}: Login + PKCE code"
    if "${GREP}" -q "LOGIN_OK" "${result_file}" 2>/dev/null; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "${description}: Login code OK"
        PASS_COUNT=$(( PASS_COUNT + 1 ))
    else
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${description}: Login failed"
        engine_ok=1
    fi

    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}: Token exchange"
    if "${GREP}" -q "TOKEN_OK" "${result_file}" 2>/dev/null; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "${description}: Token OK"
        PASS_COUNT=$(( PASS_COUNT + 1 ))
    else
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${description}: Token failed"
        engine_ok=1
    fi

    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}: Userinfo"
    if "${GREP}" -q "USERINFO_OK" "${result_file}" 2>/dev/null; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "${description}: Userinfo OK"
        PASS_COUNT=$(( PASS_COUNT + 1 ))
    else
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${description}: Userinfo failed"
        engine_ok=1
    fi

    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}: id_token claims"
    if "${GREP}" -q "ID_TOKEN_OK" "${result_file}" 2>/dev/null; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "${description}: id_token OK"
        PASS_COUNT=$(( PASS_COUNT + 1 ))
    else
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${description}: id_token failed"
        engine_ok=1
    fi

    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}: Refresh + reuse"
    if "${GREP}" -q "REFRESH_OK" "${result_file}" 2>/dev/null \
        && "${GREP}" -q "REFRESH_REUSE_OK" "${result_file}" 2>/dev/null; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "${description}: Refresh rotation OK"
        PASS_COUNT=$(( PASS_COUNT + 1 ))
    else
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${description}: Refresh failed"
        engine_ok=1
    fi

    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}: Code reuse"
    if "${GREP}" -q "CODE_REUSE_OK" "${result_file}" 2>/dev/null; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "${description}: Code reuse rejected"
        PASS_COUNT=$(( PASS_COUNT + 1 ))
    else
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${description}: Code reuse check failed"
        engine_ok=1
    fi

    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}: Redirect hardening"
    if "${GREP}" -q "BAD_REDIRECT_OK" "${result_file}" 2>/dev/null \
        && "${GREP}" -q "JS_REDIRECT_OK" "${result_file}" 2>/dev/null \
        && "${GREP}" -q "MISS_STATE_OK" "${result_file}" 2>/dev/null \
        && "${GREP}" -q "MISS_NONCE_OK" "${result_file}" 2>/dev/null; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "${description}: Hardening OK"
        PASS_COUNT=$(( PASS_COUNT + 1 ))
    else
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${description}: Hardening failed"
        engine_ok=1
    fi

    if [[ "${engine_ok}" -eq 0 ]] && "${GREP}" -q "IDP_TEST_COMPLETE" "${result_file}" 2>/dev/null; then
        return 0
    fi
    return 1
}

# --- Preflight ---

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Locate Hydrogen Binary"
HYDROGEN_BIN=''
HYDROGEN_BIN_BASE=''
# shellcheck disable=SC2310 # Continue suite even if binary lookup fails
if find_hydrogen_binary "${PROJECT_DIR}"; then
    pass_subtest "Hydrogen binary found: ${HYDROGEN_BIN_BASE}"
else
    fail_subtest "Failed to find Hydrogen binary"
fi

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Demo credentials present"
if [[ -n "${DEMO_USER_NAME}" && -n "${DEMO_USER_PASS}" ]]; then
    pass_subtest "HYDROGEN_DEMO_USER_NAME/PASS set"
else
    fail_subtest "Missing HYDROGEN_DEMO_USER_NAME or HYDROGEN_DEMO_USER_PASS"
fi

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Validate disabled config"
# shellcheck disable=SC2310 # Continue even if validation fails
if validate_config_file "${CONFIG_DISABLED}"; then
    pass_subtest "Disabled config OK"
else
    fail_subtest "Disabled config invalid"
fi

config_valid=true
for test_config in "${!IDP_TEST_CONFIGS[@]}"; do
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Validate config: ${test_config}"
    IFS=':' read -r config_file log_suffix engine_name description <<< "${IDP_TEST_CONFIGS[${test_config}]}"
    # shellcheck disable=SC2310 # Continue even if one config fails
    if validate_config_file "${config_file}"; then
        port=$(get_webserver_port "${config_file}")
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${description} port ${port}"
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "${test_config} config valid"
        PASS_COUNT=$(( PASS_COUNT + 1 ))
    else
        config_valid=false
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${test_config} config invalid"
        EXIT_CODE=1
    fi
done

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "All engine configs"
if [[ "${config_valid}" = true ]]; then
    pass_subtest "All ${#IDP_TEST_CONFIGS[@]} engine configs valid"
else
    fail_subtest "One or more engine configs invalid"
fi

TEST_NAME="OIDC IdP  {BLUE}databases: ${#IDP_TEST_CONFIGS[@]}{RESET}"

# --- Disabled gate (sequential, port 5457) ---

if [[ "${EXIT_CODE}" -eq 0 ]]; then
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "OIDC disabled closes discovery"
    SERVER_LOG="${LOGS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}_disabled.log"
    HYDROGEN_PID=""
    HYDROGEN_PID_VAR="HYDROGEN_PID_$$"
    # shellcheck disable=SC2310 # Continue even if startup fails
    if start_hydrogen_with_pid "${CONFIG_DISABLED}" "${SERVER_LOG}" "${STARTUP_TIMEOUT}" "${HYDROGEN_BIN}" "${HYDROGEN_PID_VAR}"; then
        HYDROGEN_PID=$(eval "echo \$${HYDROGEN_PID_VAR}")
        PORT_DIS=$(get_webserver_port "${CONFIG_DISABLED}")
        BASE_URL="http://localhost:${PORT_DIS}"
        # shellcheck disable=SC2310 # Continue even if readiness fails
        if wait_for_server_ready "${BASE_URL}"; then
            code="$(oidc_idp_fetch_discovery "${BASE_URL}" "${LOG_PREFIX}_disc_off.json")"
            if [[ "${code}" != "200" ]]; then
                pass_subtest "Discovery not 200 when disabled (HTTP ${code})"
            else
                fail_subtest "Discovery returned 200 while OIDC disabled"
            fi
        else
            fail_subtest "Disabled server not ready"
        fi
        # shellcheck disable=SC2310 # cleanup best-effort
        stop_hydrogen "${HYDROGEN_PID}" "${SERVER_LOG}" "${SHUTDOWN_TIMEOUT}" 5 "${RESULTS_DIR}" 2>/dev/null || true
        HYDROGEN_PID=""
    else
        fail_subtest "Failed to start disabled config"
    fi
fi

# --- Parallel multi-engine ---

if [[ "${EXIT_CODE}" -eq 0 ]]; then
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Running OIDC IdP tests in parallel"
    mkdir -p "${PROJECT_DIR}/tests/artifacts/database/sqlite" 2>/dev/null || true

    for test_config in "${!IDP_TEST_CONFIGS[@]}"; do
        # shellcheck disable=SC2312 # Job control with wc -l is standard practice
        while (( $(jobs -r | wc -l) >= CORES )); do
            wait -n
        done
        IFS=':' read -r config_file log_suffix engine_name description <<< "${IDP_TEST_CONFIGS[${test_config}]}"
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Starting parallel: ${test_config} (${description})"
        run_idp_test_parallel "${test_config}" "${config_file}" "${log_suffix}" "${engine_name}" "${description}" &
        PARALLEL_PIDS+=($!)
    done

    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Waiting for ${#IDP_TEST_CONFIGS[@]} parallel IdP tests"
    for pid in "${PARALLEL_PIDS[@]}"; do
        wait "${pid}"
    done
    pass_subtest "All parallel IdP engine runs finished"

    engines_pass=0
    for test_config in "${!IDP_TEST_CONFIGS[@]}"; do
        IFS=':' read -r config_file log_suffix engine_name description <<< "${IDP_TEST_CONFIGS[${test_config}]}"
        print_marker "${TEST_NUMBER}" "${TEST_COUNTER}"
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}: Analyzing results"
        log_file="${LOGS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}_${log_suffix}.log"
        result_file="${LOG_PREFIX}${TIMESTAMP}_${log_suffix}.result"
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${test_config}: ${TESTS_DIR}/logs/${log_file##*/}"
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "${test_config}: ${DIAG_TEST_DIR}/${result_file##*/}"
        # shellcheck disable=SC2310 # Continue analyzing all engines
        if analyze_idp_test_results "${test_config}" "${log_suffix}" "${engine_name}" "${description}"; then
            engines_pass=$(( engines_pass + 1 ))
        else
            EXIT_CODE=1
        fi
    done

    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Multi-engine summary"
    if [[ "${engines_pass}" -eq "${#IDP_TEST_CONFIGS[@]}" ]]; then
        pass_subtest "All ${engines_pass}/${#IDP_TEST_CONFIGS[@]} engines passed IdP suite"
    else
        fail_subtest "${engines_pass}/${#IDP_TEST_CONFIGS[@]} engines fully passed"
    fi
fi

oidc_idp_cleanup_tmp

print_test_completion "${TEST_NAME}" "${TEST_ABBR}" "${TEST_NUMBER}" "${TEST_VERSION}"

${ORCHESTRATION:-false} && return "${EXIT_CODE}" || exit "${EXIT_CODE}"
