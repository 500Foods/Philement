#!/usr/bin/env bash

# Test: OIDC Identity Provider — blackbox coverage
# Drives Hydrogen as IdP: discovery, JWKS, authorize (login), token (PKCE),
# userinfo, refresh, and basic error paths (inverse of Test 42 mock-IdP).

# CHANGELOG
# 1.0.0 - 2026-07-23 - Initial Phase 14: disabled gate, discovery/JWKS, happy path PKCE

set -euo pipefail

TEST_NAME="OIDC Identity Provider"
TEST_ABBR="IDP"
TEST_NUMBER="45"
TEST_COUNTER=0
TEST_VERSION="1.0.0"

# shellcheck source=tests/lib/framework.sh
[[ -n "${FRAMEWORK_GUARD:-}" ]] || source "$(dirname "${BASH_SOURCE[0]}")/lib/framework.sh"
setup_test_environment

# shellcheck source=tests/lib/oidc_idp_helpers.sh
source "${SCRIPT_DIR}/lib/oidc_idp_helpers.sh"

CONFIG_DISABLED="${SCRIPT_DIR}/configs/hydrogen_test_${TEST_NUMBER}_oidc_idp_disabled.json"
CONFIG_ENABLED="${SCRIPT_DIR}/configs/hydrogen_test_${TEST_NUMBER}_oidc_idp.json"

CLIENT_ID="test-idp-public"
REDIRECT_URI="http://127.0.0.1:5459/cb"
DEMO_USER_NAME="${HYDROGEN_DEMO_USER_NAME:-}"
DEMO_USER_PASS="${HYDROGEN_DEMO_USER_PASS:-}"

STARTUP_TIMEOUT=20
SHUTDOWN_TIMEOUT=15

HYDROGEN_PID=""
SERVER_LOG=""
BASE_URL=""

cleanup_server() {
    oidc_idp_cleanup_tmp
    if [[ -n "${HYDROGEN_PID}" ]] && ps -p "${HYDROGEN_PID}" > /dev/null 2>&1; then
        stop_hydrogen "${HYDROGEN_PID}" "${SERVER_LOG}" "${SHUTDOWN_TIMEOUT}" 5 "${RESULTS_DIR}" 2>/dev/null || true
        HYDROGEN_PID=""
    fi
}
trap 'cleanup_server; _hydrogen_owned_exit_trap 2>/dev/null || true' EXIT

pass_subtest() {
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "$1"
    PASS_COUNT=$(( PASS_COUNT + 1 ))
}

fail_subtest() {
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "$1"
    EXIT_CODE=1
}

wait_idp_ready() {
    local log_file="$1"
    local timeout="${2:-120}"
    local deadline
    deadline=$(( $(date +%s) + timeout ))
    while true; do
        if [[ $(date +%s) -ge ${deadline} ]]; then
            return 1
        fi
        if "${GREP}" -q -E "READY FOR REQUESTS|Migration completed in|Migration Current:" "${log_file}" 2>/dev/null; then
            sleep 1
            return 0
        fi
        sleep 0.3
    done
}

# --- Preflight ---

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Locate Hydrogen Binary"
HYDROGEN_BIN=''
HYDROGEN_BIN_BASE=''
# shellcheck disable=SC2310
if find_hydrogen_binary "${PROJECT_DIR}"; then
    pass_subtest "Hydrogen binary found: ${HYDROGEN_BIN_BASE}"
else
    fail_subtest "Failed to find Hydrogen binary"
fi

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Validate Configuration Files"
# shellcheck disable=SC2310
if validate_config_file "${CONFIG_DISABLED}" && validate_config_file "${CONFIG_ENABLED}"; then
    pass_subtest "Configs validated"
else
    fail_subtest "Config validation failed"
fi

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Demo credentials present"
if [[ -n "${DEMO_USER_NAME}" && -n "${DEMO_USER_PASS}" ]]; then
    pass_subtest "HYDROGEN_DEMO_USER_NAME/PASS set"
else
    fail_subtest "Missing HYDROGEN_DEMO_USER_NAME or HYDROGEN_DEMO_USER_PASS"
fi

# --- Subtest 1: Disabled → endpoints closed ---

if [[ "${EXIT_CODE}" -eq 0 ]]; then
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "OIDC disabled closes discovery"
    SERVER_LOG="${LOGS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}_disabled.log"
    HYDROGEN_PID=""
    HYDROGEN_PID_VAR="HYDROGEN_PID_$$"
    # shellcheck disable=SC2310
    if start_hydrogen_with_pid "${CONFIG_DISABLED}" "${SERVER_LOG}" "${STARTUP_TIMEOUT}" "${HYDROGEN_BIN}" "${HYDROGEN_PID_VAR}"; then
        HYDROGEN_PID=$(eval "echo \$${HYDROGEN_PID_VAR}")
        PORT_DIS=$(get_webserver_port "${CONFIG_DISABLED}")
        BASE_URL="http://localhost:${PORT_DIS}"
        # shellcheck disable=SC2310
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
        cleanup_server
    else
        fail_subtest "Failed to start disabled config"
    fi
fi

# --- Enabled server lifecycle ---

if [[ "${EXIT_CODE}" -eq 0 ]]; then
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Start OIDC-enabled Hydrogen"
    mkdir -p "${PROJECT_DIR}/tests/artifacts/oidc_idp_keys_45" \
             "${PROJECT_DIR}/tests/artifacts/database/sqlite" 2>/dev/null || true
    SERVER_LOG="${LOGS_DIR}/test_${TEST_NUMBER}_${TIMESTAMP}_enabled.log"
    HYDROGEN_PID=""
    HYDROGEN_PID_VAR="HYDROGEN_PID_$$"
    # shellcheck disable=SC2310
    if start_hydrogen_with_pid "${CONFIG_ENABLED}" "${SERVER_LOG}" "${STARTUP_TIMEOUT}" "${HYDROGEN_BIN}" "${HYDROGEN_PID_VAR}"; then
        HYDROGEN_PID=$(eval "echo \$${HYDROGEN_PID_VAR}")
        PORT_EN=$(get_webserver_port "${CONFIG_ENABLED}")
        BASE_URL="http://localhost:${PORT_EN}"
        # shellcheck disable=SC2310
        # shellcheck disable=SC2310
        if ! wait_for_server_ready "${BASE_URL}"; then
            fail_subtest "Enabled server HTTP not ready"
            cleanup_server
        else
            # Prefer READY/migration signal; fall back to discovery if OIDC already live
            # shellcheck disable=SC2310
            if wait_idp_ready "${SERVER_LOG}" 90 \
                || [[ "$(oidc_idp_fetch_discovery "${BASE_URL}" "${LOG_PREFIX}_ready_probe.json")" == "200" ]]; then
                pass_subtest "Server ready on ${BASE_URL}"
            else
                fail_subtest "Enabled server failed readiness/migration"
                print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Tail log: $(tail -20 "${SERVER_LOG}" 2>/dev/null || true)"
                cleanup_server
            fi
        fi
    else
        fail_subtest "Failed to start enabled config"
    fi
fi

# --- Discovery / JWKS ---

if [[ "${EXIT_CODE}" -eq 0 && -n "${HYDROGEN_PID}" ]]; then
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Discovery document shape"
    disc_file="${LOG_PREFIX}_discovery.json"
    code="$(oidc_idp_fetch_discovery "${BASE_URL}" "${disc_file}")"
    if [[ "${code}" == "200" ]] \
        && jq -e '.issuer and .authorization_endpoint and .token_endpoint and .jwks_uri' "${disc_file}" >/dev/null 2>&1 \
        && jq -e '.code_challenge_methods_supported | index("S256")' "${disc_file}" >/dev/null 2>&1 \
        && jq -e '.id_token_signing_alg_values_supported | index("RS256")' "${disc_file}" >/dev/null 2>&1 \
        && jq -e '.introspection_endpoint and .revocation_endpoint' "${disc_file}" >/dev/null 2>&1; then
        pass_subtest "Discovery OK (issuer, endpoints, S256, RS256)"
    else
        fail_subtest "Discovery missing required fields (HTTP ${code})"
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Body: $(head -c 400 "${disc_file}" 2>/dev/null || true)"
    fi

    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "JWKS has RSA key"
    jwks_file="${LOG_PREFIX}_jwks.json"
    code="$(oidc_idp_fetch_jwks "${BASE_URL}" "${jwks_file}")"
    if [[ "${code}" == "200" ]] \
        && jq -e '.keys | length >= 1' "${jwks_file}" >/dev/null 2>&1 \
        && jq -e '.keys[0].n and .keys[0].e and .keys[0].kid' "${jwks_file}" >/dev/null 2>&1; then
        pass_subtest "JWKS has usable RSA key"
    else
        fail_subtest "JWKS invalid (HTTP ${code})"
    fi

    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Authorize missing params → error"
    code="$(curl -sS -o "${LOG_PREFIX}_auth_bad.json" -w '%{http_code}' \
        "${BASE_URL}/oauth/authorize" 2>/dev/null || echo "000")"
    # Expect redirect or 400-class, not 200 login form without client
    if [[ "${code}" != "200" ]] || "${GREP}" -q "error" "${LOG_PREFIX}_auth_bad.json" 2>/dev/null; then
        pass_subtest "Authorize without params rejected (HTTP ${code})"
    else
        # GET without params may still 200 empty form error via oauth_error redirect fail
        body="$(cat "${LOG_PREFIX}_auth_bad.json" 2>/dev/null || true)"
        if [[ "${body}" == *invalid* || "${body}" == *error* || -z "${body}" ]]; then
            pass_subtest "Authorize missing params not a success login"
        else
            fail_subtest "Authorize missing params unexpectedly succeeded"
        fi
    fi
fi

# --- Happy path: login → code → tokens ---

ACCESS_TOKEN=""
ID_TOKEN=""
REFRESH_TOKEN=""
AUTH_CODE=""

if [[ "${EXIT_CODE}" -eq 0 && -n "${HYDROGEN_PID}" ]]; then
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Happy path: authorize login + PKCE code"
    oidc_idp_pkce_pair
    STATE="st-$(openssl rand -hex 8)"
    NONCE="n-$(openssl rand -hex 8)"
    # shellcheck disable=SC2310
    if oidc_idp_authorize_login "${BASE_URL}" "${CLIENT_ID}" "${REDIRECT_URI}" \
        "openid offline_access" "${STATE}" "${NONCE}" \
        "${OIDC_IDP_CODE_CHALLENGE}" "${DEMO_USER_NAME}" "${DEMO_USER_PASS}"; then
        AUTH_CODE="${OIDC_IDP_AUTH_CODE}"
        if [[ -n "${AUTH_CODE}" ]]; then
            pass_subtest "Received authorization code"
        else
            fail_subtest "Login redirect missing code"
        fi
    else
        fail_subtest "Authorize login failed (check credentials/DB/seed client)"
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "See log ${SERVER_LOG}"
    fi
fi

if [[ "${EXIT_CODE}" -eq 0 && -n "${AUTH_CODE}" ]]; then
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Token exchange code+PKCE"
    tok_file="${LOG_PREFIX}_token1.json"
    code="$(oidc_idp_token_code "${BASE_URL}" "${CLIENT_ID}" "${REDIRECT_URI}" \
        "${AUTH_CODE}" "${OIDC_IDP_CODE_VERIFIER}" "${tok_file}")"
    if [[ "${code}" == "200" ]] \
        && jq -e '.access_token and .id_token and .token_type' "${tok_file}" >/dev/null 2>&1; then
        ACCESS_TOKEN="$(jq -r '.access_token' "${tok_file}")"
        ID_TOKEN="$(jq -r '.id_token' "${tok_file}")"
        REFRESH_TOKEN="$(jq -r '.refresh_token // empty' "${tok_file}")"
        pass_subtest "Token response has access_token and id_token"
    else
        fail_subtest "Token exchange failed (HTTP ${code}): $(head -c 300 "${tok_file}" 2>/dev/null || true)"
    fi
fi

if [[ "${EXIT_CODE}" -eq 0 && -n "${ACCESS_TOKEN}" ]]; then
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Userinfo with access token"
    ui_file="${LOG_PREFIX}_userinfo.json"
    code="$(oidc_idp_userinfo "${BASE_URL}" "${ACCESS_TOKEN}" "${ui_file}")"
    if [[ "${code}" == "200" ]] && jq -e '.sub' "${ui_file}" >/dev/null 2>&1; then
        pass_subtest "Userinfo returned sub"
    else
        fail_subtest "Userinfo failed (HTTP ${code})"
    fi
fi

if [[ "${EXIT_CODE}" -eq 0 && -n "${ID_TOKEN}" ]]; then
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "id_token payload has iss/sub/nonce"
    payload_json="$(oidc_idp_jwt_payload_json "${ID_TOKEN}")"
    if printf '%s' "${payload_json}" | jq -e --arg n "${NONCE}" \
        '.iss and .sub and .nonce == $n' >/dev/null 2>&1; then
        pass_subtest "id_token claims OK"
    else
        fail_subtest "id_token claims missing: ${payload_json:0:200}"
    fi
fi

if [[ "${EXIT_CODE}" -eq 0 && -n "${REFRESH_TOKEN}" ]]; then
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Refresh token rotation"
    tok2="${LOG_PREFIX}_token2.json"
    code="$(oidc_idp_token_refresh "${BASE_URL}" "${CLIENT_ID}" "${REFRESH_TOKEN}" "${tok2}")"
    new_rt="$(jq -r '.refresh_token // empty' "${tok2}" 2>/dev/null || true)"
    if [[ "${code}" == "200" && -n "${new_rt}" && "${new_rt}" != "${REFRESH_TOKEN}" ]]; then
        pass_subtest "Refresh rotated"
        # reuse old refresh should fail
        print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Refresh reuse fails"
        tok3="${LOG_PREFIX}_token3.json"
        code2="$(oidc_idp_token_refresh "${BASE_URL}" "${CLIENT_ID}" "${REFRESH_TOKEN}" "${tok3}")"
        if [[ "${code2}" != "200" ]] || jq -e '.error' "${tok3}" >/dev/null 2>&1; then
            pass_subtest "Reuse rejected"
        else
            fail_subtest "Reuse of old refresh succeeded"
        fi
    else
        fail_subtest "Refresh failed (HTTP ${code})"
    fi
elif [[ "${EXIT_CODE}" -eq 0 && -n "${ACCESS_TOKEN}" ]]; then
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Refresh token present"
    fail_subtest "No refresh_token in code exchange (seed client grant_types?)"
fi

if [[ "${EXIT_CODE}" -eq 0 && -n "${AUTH_CODE}" ]]; then
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Auth code reuse fails"
    tok_reuse="${LOG_PREFIX}_token_reuse.json"
    code="$(oidc_idp_token_code "${BASE_URL}" "${CLIENT_ID}" "${REDIRECT_URI}" \
        "${AUTH_CODE}" "${OIDC_IDP_CODE_VERIFIER}" "${tok_reuse}")"
    if [[ "${code}" != "200" ]] || jq -e '.error' "${tok_reuse}" >/dev/null 2>&1; then
        pass_subtest "Code reuse rejected"
    else
        fail_subtest "Code reuse succeeded"
    fi
fi

if [[ "${EXIT_CODE}" -eq 0 && -n "${HYDROGEN_PID}" ]]; then
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Bad redirect_uri rejected"
    oidc_idp_pkce_pair
    # shellcheck disable=SC2310
    if oidc_idp_authorize_login "${BASE_URL}" "${CLIENT_ID}" "http://evil.example/cb" \
        "openid" "s1" "n1" "${OIDC_IDP_CODE_CHALLENGE}" \
        "${DEMO_USER_NAME}" "${DEMO_USER_PASS}"; then
        fail_subtest "Bad redirect_uri issued a code"
    else
        pass_subtest "Bad redirect_uri rejected"
    fi
fi

cleanup_server
oidc_idp_cleanup_tmp

print_test_completion "${TEST_NAME}" "${TEST_ABBR}" "${TEST_NUMBER}" "${TEST_VERSION}"

${ORCHESTRATION:-false} && return "${EXIT_CODE}" || exit "${EXIT_CODE}"
