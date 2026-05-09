#!/usr/bin/env bash

# Test: OIDC Relying Party — Disabled Stub Endpoints
# Verifies that the three new OIDC RP endpoints lock in the routing
# contract before any real logic lands (Phases 7–22).
#
# With OIDC_RP.Enabled defaulting to false (no OIDC_RP block in the
# config), all three endpoints must respond 503 with the envelope
# {"error":"oidc_disabled"}. Method-mismatch must return 405 with
# {"error":"method_not_allowed"}. Paths outside the OIDC tree
# return the standard 404.

# FUNCTIONS
# validate_oidc_request()
# test_oidc_endpoint_disabled()
# test_oidc_endpoint_method_not_allowed()
# test_oidc_unknown_path_404()
# start_mock_keycloak()
# stop_mock_keycloak()
# test_mock_keycloak_reachable()
# test_mock_keycloak_discovery_doc()
# test_mock_keycloak_jwks_doc()
# test_mock_keycloak_token_happy_path()
# test_mock_keycloak_token_invalid_grant()
# test_mock_keycloak_token_unsupported_grant_type()
# test_oidc_start_redirects_to_idp()
# test_oidc_start_invalid_return_to_rejected()

# CHANGELOG
# 1.3.0 - 2026-05-09 - Phase 11: mock Keycloak /token endpoint sub-tests
# 1.2.0 - 2026-05-09 - Phase 10: real /oidc/start redirect against mock IdP
# 1.1.0 - 2026-05-09 - Phase 9: mock Keycloak reachability + discovery/JWKS
# 1.0.0 - 2026-05-08 - Initial Phase 6 implementation: disabled-stub coverage

set -euo pipefail

# Test Configuration
TEST_NAME="OIDC RP"
TEST_ABBR="OID"
TEST_NUMBER="42"
TEST_COUNTER=0
TEST_VERSION="1.3.0"

# Phase 9: mock Keycloak port. Picked outside the typical Hydrogen
# port range (5000s) and the test config's WebServer port (5242). If
# this collides on someone's machine, override via MOCK_KC_PORT.
MOCK_KC_PORT="${MOCK_KC_PORT:-7042}"
MOCK_KC_SCRIPT=""
MOCK_KC_PID=""
MOCK_KC_LOG=""

# shellcheck source=tests/lib/framework.sh # Reference framework directly
[[ -n "${FRAMEWORK_GUARD:-}" ]] || source "$(dirname "${BASH_SOURCE[0]}")/lib/framework.sh"
setup_test_environment

CONFIG_PATH="${SCRIPT_DIR}/configs/hydrogen_test_${TEST_NUMBER}_oidc_rp.json"
CONFIG_PATH_ENABLED="${SCRIPT_DIR}/configs/hydrogen_test_${TEST_NUMBER}_oidc_rp_enabled.json"
MOCK_KC_SCRIPT="${SCRIPT_DIR}/lib/mock_keycloak/server.js"

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

# Issues an HTTP request and verifies (a) the HTTP status and
# (b) that the response body is JSON containing the expected
# `error` key value.
#
# Arguments:
#   $1 url
#   $2 method
#   $3 expected_http_status
#   $4 expected_error_value     (the literal string to find under .error)
#   $5 response_file            (where to write the body for diagnostics)
#   $6 [optional body] for POST
#
# Returns 0 on match, 1 otherwise.
validate_oidc_request() {
    local url="$1"
    local method="$2"
    local expected_status="$3"
    local expected_error="$4"
    local response_file="$5"
    local body="${6:-}"

    local curl_cmd=(curl -s -X "${method}" -H "Content-Type: application/json")
    if [[ -n "${body}" ]]; then
        curl_cmd+=(-d "${body}")
    fi
    curl_cmd+=(-w "%{http_code}" -o "${response_file}" --max-time 5 "${url}")

    local http_status
    http_status=$("${curl_cmd[@]}" 2>/dev/null || echo "000")

    if [[ "${http_status}" != "${expected_status}" ]]; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Expected HTTP ${expected_status}, got ${http_status}"
        if [[ -s "${response_file}" ]]; then
            local body_excerpt
            # shellcheck disable=SC2312 # Diagnostic-only; head failure is acceptable
            body_excerpt=$(head -c 200 "${response_file}" 2>/dev/null || true)
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Body: ${body_excerpt}"
        fi
        return 1
    fi

    if ! command -v jq >/dev/null 2>&1; then
        # Fallback: substring match if jq unavailable.
        if "${GREP}" -q "\"error\"\\s*:\\s*\"${expected_error}\"" "${response_file}"; then
            return 0
        fi
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "jq missing and substring search failed for error=${expected_error}"
        return 1
    fi

    local actual_error
    actual_error=$(jq -r '.error // empty' "${response_file}" 2>/dev/null || echo "")
    if [[ "${actual_error}" == "${expected_error}" ]]; then
        return 0
    fi

    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Expected .error=${expected_error}, got .error=${actual_error}"
    return 1
}

# ---------------------------------------------------------------------------
# Per-endpoint sub-tests
# ---------------------------------------------------------------------------

test_oidc_endpoint_disabled() {
    local base_url="$1"
    local endpoint="$2"      # e.g. start, callback, handoff
    local method="$3"        # the *correct* method for the endpoint
    local description="$4"

    local response_file="${LOG_PREFIX}${TIMESTAMP}_${endpoint}_disabled.json"
    local body=""
    if [[ "${method}" == "POST" ]]; then
        body='{"handoff":"unused-in-phase-6"}'
    fi

    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}: ${method} returns 503 oidc_disabled"
    # shellcheck disable=SC2310 # We want to continue even if the test fails
    if validate_oidc_request "${base_url}/api/auth/oidc/${endpoint}" \
        "${method}" "503" "oidc_disabled" "${response_file}" "${body}"; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "${description}: 503 oidc_disabled envelope correct"
        PASS_COUNT=$(( PASS_COUNT + 1 ))
    else
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${description}: did not return 503 oidc_disabled"
        EXIT_CODE=1
    fi
}

test_oidc_endpoint_method_not_allowed() {
    local base_url="$1"
    local endpoint="$2"      # e.g. start, callback, handoff
    local wrong_method="$3"  # the *wrong* method to use
    local description="$4"

    local response_file="${LOG_PREFIX}${TIMESTAMP}_${endpoint}_${wrong_method}_405.json"
    local body=""
    if [[ "${wrong_method}" == "POST" ]]; then
        body='{}'
    fi

    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}: ${wrong_method} returns 405 method_not_allowed"
    # shellcheck disable=SC2310 # We want to continue even if the test fails
    if validate_oidc_request "${base_url}/api/auth/oidc/${endpoint}" \
        "${wrong_method}" "405" "method_not_allowed" "${response_file}" "${body}"; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "${description}: 405 method_not_allowed envelope correct"
        PASS_COUNT=$(( PASS_COUNT + 1 ))
    else
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${description}: did not return 405 method_not_allowed"
        EXIT_CODE=1
    fi
}

test_oidc_unknown_path_404() {
    local base_url="$1"
    local path="$2"          # e.g. auth/oidc/nope, auth/oidc
    local description="$3"

    local safe_label
    safe_label=$(echo "${path}" | tr '/' '_')
    local response_file="${LOG_PREFIX}${TIMESTAMP}_${safe_label}_404.json"

    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}: returns 404"

    local http_status
    http_status=$(curl -s -X GET -w "%{http_code}" -o "${response_file}" \
        --max-time 5 "${base_url}/api/${path}" 2>/dev/null || echo "000")

    if [[ "${http_status}" == "404" ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "${description}: 404 returned as expected"
        PASS_COUNT=$(( PASS_COUNT + 1 ))
    else
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "${description}: expected 404, got ${http_status}"
        EXIT_CODE=1
    fi
}

# ---------------------------------------------------------------------------
# Mock Keycloak (Phase 9)
# ---------------------------------------------------------------------------

# Start the mock Keycloak server in the background. Sets MOCK_KC_PID
# on success; on failure leaves it empty and returns non-zero. Waits
# until the script prints "READY <port>" or 5 s elapses.
start_mock_keycloak() {
    if ! command -v node >/dev/null 2>&1; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "node not available; skipping mock Keycloak sub-tests"
        return 1
    fi
    if [[ ! -f "${MOCK_KC_SCRIPT}" ]]; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "mock Keycloak script missing: ${MOCK_KC_SCRIPT}"
        return 1
    fi

    MOCK_KC_LOG="${LOG_PREFIX}${TIMESTAMP}_mock_keycloak.log"
    # Background the node process; capture stdout+stderr.
    node "${MOCK_KC_SCRIPT}" "${MOCK_KC_PORT}" >"${MOCK_KC_LOG}" 2>&1 &
    MOCK_KC_PID=$!

    # Wait for the READY line (max 5 s).
    local waited=0
    while (( waited < 50 )); do
        if [[ -s "${MOCK_KC_LOG}" ]] && "${GREP}" -q "^READY ${MOCK_KC_PORT}\$" "${MOCK_KC_LOG}"; then
            return 0
        fi
        if ! ps -p "${MOCK_KC_PID}" > /dev/null 2>&1; then
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "mock Keycloak exited prematurely; log: ${MOCK_KC_LOG}"
            MOCK_KC_PID=""
            return 1
        fi
        sleep 0.1
        waited=$(( waited + 1 ))
    done
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "mock Keycloak did not become READY within 5s"
    return 1
}

# Stop the mock Keycloak server. No-op if MOCK_KC_PID is empty.
stop_mock_keycloak() {
    if [[ -z "${MOCK_KC_PID}" ]]; then
        return 0
    fi
    if ps -p "${MOCK_KC_PID}" > /dev/null 2>&1; then
        kill -TERM "${MOCK_KC_PID}" 2>/dev/null || true
        # Give it a beat to exit; kill -9 if needed.
        local waited=0
        while (( waited < 20 )) && ps -p "${MOCK_KC_PID}" > /dev/null 2>&1; do
            sleep 0.1
            waited=$(( waited + 1 ))
        done
        if ps -p "${MOCK_KC_PID}" > /dev/null 2>&1; then
            kill -KILL "${MOCK_KC_PID}" 2>/dev/null || true
        fi
    fi
    wait "${MOCK_KC_PID}" 2>/dev/null || true
    MOCK_KC_PID=""
}

# Trap to make sure we do not leak a node process if the test script
# fails between mock start and stop.
trap 'stop_mock_keycloak || true' EXIT

# Sub-test: mock Keycloak responds 200 ok at /health.
test_mock_keycloak_reachable() {
    local response_file="${LOG_PREFIX}${TIMESTAMP}_mock_kc_health.txt"
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Mock Keycloak /health returns 200 ok"

    local http_status body=""
    http_status=$(curl -s -X GET -w "%{http_code}" -o "${response_file}" \
        --max-time 5 "http://127.0.0.1:${MOCK_KC_PORT}/health" 2>/dev/null || echo "000")
    if [[ -f "${response_file}" ]]; then
        body=$(<"${response_file}")
    fi

    if [[ "${http_status}" == "200" ]] && [[ "${body}" == "ok" ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Mock Keycloak reachable"
        PASS_COUNT=$(( PASS_COUNT + 1 ))
    else
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Mock Keycloak unreachable (status=${http_status})"
        EXIT_CODE=1
    fi
}

# Sub-test: mock discovery doc has required OIDC fields.
test_mock_keycloak_discovery_doc() {
    local response_file="${LOG_PREFIX}${TIMESTAMP}_mock_kc_discovery.json"
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Mock Keycloak discovery doc has required fields"

    local http_status
    http_status=$(curl -s -X GET -w "%{http_code}" -o "${response_file}" \
        --max-time 5 \
        "http://127.0.0.1:${MOCK_KC_PORT}/realms/test/.well-known/openid-configuration" \
        2>/dev/null || echo "000")

    if [[ "${http_status}" != "200" ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Discovery doc fetch failed (status=${http_status})"
        EXIT_CODE=1
        return
    fi

    if command -v jq >/dev/null 2>&1; then
        local issuer auth_ep token_ep jwks_uri
        issuer=$(jq -r '.issuer // empty' "${response_file}" 2>/dev/null || echo "")
        auth_ep=$(jq -r '.authorization_endpoint // empty' "${response_file}" 2>/dev/null || echo "")
        token_ep=$(jq -r '.token_endpoint // empty' "${response_file}" 2>/dev/null || echo "")
        jwks_uri=$(jq -r '.jwks_uri // empty' "${response_file}" 2>/dev/null || echo "")

        if [[ -n "${issuer}" ]] && [[ -n "${auth_ep}" ]] && [[ -n "${token_ep}" ]] && [[ -n "${jwks_uri}" ]]; then
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Discovery doc has issuer/auth/token/jwks fields"
            PASS_COUNT=$(( PASS_COUNT + 1 ))
        else
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Discovery doc missing one or more required fields"
            EXIT_CODE=1
        fi
    else
        # jq missing — substring sanity check.
        if "${GREP}" -q '"issuer"' "${response_file}" \
            && "${GREP}" -q '"jwks_uri"' "${response_file}"; then
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Discovery doc has required fields (substring match)"
            PASS_COUNT=$(( PASS_COUNT + 1 ))
        else
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Discovery doc substring check failed"
            EXIT_CODE=1
        fi
    fi
}

# Sub-test: mock JWKS doc has at least one key with kid.
test_mock_keycloak_jwks_doc() {
    local response_file="${LOG_PREFIX}${TIMESTAMP}_mock_kc_jwks.json"
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Mock Keycloak JWKS doc has at least one keyed entry"

    local http_status
    http_status=$(curl -s -X GET -w "%{http_code}" -o "${response_file}" \
        --max-time 5 \
        "http://127.0.0.1:${MOCK_KC_PORT}/realms/test/protocol/openid-connect/certs" \
        2>/dev/null || echo "000")

    if [[ "${http_status}" != "200" ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "JWKS doc fetch failed (status=${http_status})"
        EXIT_CODE=1
        return
    fi

    if command -v jq >/dev/null 2>&1; then
        local key_count first_kid
        key_count=$(jq '.keys | length' "${response_file}" 2>/dev/null || echo "0")
        first_kid=$(jq -r '.keys[0].kid // empty' "${response_file}" 2>/dev/null || echo "")
        if [[ "${key_count}" -ge 1 ]] && [[ -n "${first_kid}" ]]; then
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "JWKS has ${key_count} key(s); first kid=${first_kid}"
            PASS_COUNT=$(( PASS_COUNT + 1 ))
        else
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "JWKS has no keyed entries"
            EXIT_CODE=1
        fi
    else
        if "${GREP}" -q '"kid"' "${response_file}"; then
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "JWKS has at least one kid (substring match)"
            PASS_COUNT=$(( PASS_COUNT + 1 ))
        else
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "JWKS substring check failed"
            EXIT_CODE=1
        fi
    fi
}

# ---------------------------------------------------------------------------
# Phase 11: mock Keycloak /token endpoint
# ---------------------------------------------------------------------------

# Sub-test: POST /token with the canned `test-code-ok` returns 200
# and a JSON bundle containing id_token + access_token + token_type +
# expires_in. The id_token is intentionally a placeholder (not a real
# JWT); Phase 12 will swap in a real signed token.
test_mock_keycloak_token_happy_path() {
    local response_file="${LOG_PREFIX}${TIMESTAMP}_mock_kc_token_ok.json"
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" \
        "Mock Keycloak /token returns 200 + tokens for test-code-ok"

    # Build the same x-www-form-urlencoded body Hydrogen will send.
    # `code_verifier` is verified by neither the mock nor by RFC at
    # this layer (the IdP simply trusts whatever Hydrogen sent), so
    # any non-empty value works.
    local body="grant_type=authorization_code"
    body="${body}&code=test-code-ok"
    body="${body}&redirect_uri=http%3A%2F%2Flocalhost%3A5243%2Fapi%2Fauth%2Foidc%2Fcallback"
    body="${body}&code_verifier=fake-pkce-verifier-only-the-mock-sees-this"

    local http_status
    http_status=$(curl -s -X POST -w "%{http_code}" -o "${response_file}" \
        -H "Content-Type: application/x-www-form-urlencoded" \
        --max-time 5 \
        --data "${body}" \
        "http://127.0.0.1:${MOCK_KC_PORT}/realms/test/protocol/openid-connect/token" \
        2>/dev/null || echo "000")

    if [[ "${http_status}" != "200" ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 \
            "Token endpoint returned ${http_status} (expected 200)"
        EXIT_CODE=1
        return
    fi

    if command -v jq >/dev/null 2>&1; then
        local id_tok acc_tok tok_type
        id_tok=$(jq -r '.id_token // empty' "${response_file}" 2>/dev/null || echo "")
        acc_tok=$(jq -r '.access_token // empty' "${response_file}" 2>/dev/null || echo "")
        tok_type=$(jq -r '.token_type // empty' "${response_file}" 2>/dev/null || echo "")
        if [[ -n "${id_tok}" ]] && [[ -n "${acc_tok}" ]] && [[ "${tok_type}" == "Bearer" ]]; then
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 \
                "Token endpoint returned id_token + access_token + Bearer"
            PASS_COUNT=$(( PASS_COUNT + 1 ))
        else
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 \
                "Token endpoint response missing fields (id=${id_tok:0:8}.. acc=${acc_tok:0:8}.. type=${tok_type})"
            EXIT_CODE=1
        fi
    else
        # Fallback: substring sanity if jq missing.
        if "${GREP}" -q '"id_token"' "${response_file}" \
            && "${GREP}" -q '"access_token"' "${response_file}"; then
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 \
                "Token endpoint returned id_token + access_token (substring match)"
            PASS_COUNT=$(( PASS_COUNT + 1 ))
        else
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 \
                "Token endpoint substring check failed"
            EXIT_CODE=1
        fi
    fi
}

# Sub-test: POST /token with an unknown code returns 400 invalid_grant
# (RFC 6749 §5.2). The mock intentionally accepts only the canned
# `test-code-ok`; any other code yields invalid_grant.
test_mock_keycloak_token_invalid_grant() {
    local response_file="${LOG_PREFIX}${TIMESTAMP}_mock_kc_token_invalid_grant.json"
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" \
        "Mock Keycloak /token returns 400 invalid_grant for unknown code"

    local body="grant_type=authorization_code"
    body="${body}&code=this-code-was-never-issued"
    body="${body}&redirect_uri=http%3A%2F%2Flocalhost%3A5243%2Fcb"
    body="${body}&code_verifier=v"

    local http_status
    http_status=$(curl -s -X POST -w "%{http_code}" -o "${response_file}" \
        -H "Content-Type: application/x-www-form-urlencoded" \
        --max-time 5 \
        --data "${body}" \
        "http://127.0.0.1:${MOCK_KC_PORT}/realms/test/protocol/openid-connect/token" \
        2>/dev/null || echo "000")

    if [[ "${http_status}" != "400" ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 \
            "Expected 400, got ${http_status}"
        EXIT_CODE=1
        return
    fi

    local actual_error=""
    if command -v jq >/dev/null 2>&1; then
        actual_error=$(jq -r '.error // empty' "${response_file}" 2>/dev/null || echo "")
    fi

    if [[ "${actual_error}" == "invalid_grant" ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 \
            "Unknown code rejected with 400 invalid_grant"
        PASS_COUNT=$(( PASS_COUNT + 1 ))
    elif [[ -z "${actual_error}" ]] && "${GREP}" -q '"invalid_grant"' "${response_file}"; then
        # jq missing fallback.
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 \
            "Unknown code rejected with 400 invalid_grant (substring match)"
        PASS_COUNT=$(( PASS_COUNT + 1 ))
    else
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 \
            "Expected error=invalid_grant, got error=${actual_error}"
        EXIT_CODE=1
    fi
}

# Sub-test: POST /token with a wrong grant_type returns 400
# unsupported_grant_type. Guards against accidentally accepting
# password grants or anything else not specified by the plan.
test_mock_keycloak_token_unsupported_grant_type() {
    local response_file="${LOG_PREFIX}${TIMESTAMP}_mock_kc_token_unsupported.json"
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" \
        "Mock Keycloak /token returns 400 unsupported_grant_type for client_credentials"

    local body="grant_type=client_credentials&code=ignored"

    local http_status
    http_status=$(curl -s -X POST -w "%{http_code}" -o "${response_file}" \
        -H "Content-Type: application/x-www-form-urlencoded" \
        --max-time 5 \
        --data "${body}" \
        "http://127.0.0.1:${MOCK_KC_PORT}/realms/test/protocol/openid-connect/token" \
        2>/dev/null || echo "000")

    if [[ "${http_status}" != "400" ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 \
            "Expected 400, got ${http_status}"
        EXIT_CODE=1
        return
    fi

    if "${GREP}" -q "unsupported_grant_type" "${response_file}"; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 \
            "Wrong grant_type rejected with 400 unsupported_grant_type"
        PASS_COUNT=$(( PASS_COUNT + 1 ))
    else
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 \
            "Response missing unsupported_grant_type marker"
        EXIT_CODE=1
    fi
}

# ---------------------------------------------------------------------------
# Phase 10: /oidc/start redirect against mock IdP
# ---------------------------------------------------------------------------

# Sub-test: GET /api/auth/oidc/start returns 302 with a Location header
# pointing at the configured mock IdP authorization endpoint, with the
# expected query parameters present (response_type, client_id, scope,
# state, nonce, code_challenge, code_challenge_method=S256).
test_oidc_start_redirects_to_idp() {
    local base_url="$1"
    local response_file="${LOG_PREFIX}${TIMESTAMP}_start_redirect.headers"

    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" \
        "GET /api/auth/oidc/start returns 302 to IdP with PKCE+state"

    # -i prints headers with the body so we can grep the Location header.
    # -L is OFF — we explicitly do NOT want curl to follow the redirect
    #    because the mock IdP's /auth endpoint returns 404 (Phase 11+).
    local http_status
    http_status=$(curl -s -i -o "${response_file}" -w "%{http_code}" \
        --max-time 5 \
        "${base_url}/api/auth/oidc/start" 2>/dev/null || echo "000")

    if [[ "${http_status}" != "302" ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 \
            "Expected 302, got ${http_status}"
        EXIT_CODE=1
        return
    fi

    # Extract Location header (case-insensitive match; some servers emit
    # "Location:", others "location:"). Strip CR.
    local location
    location=$("${GREP}" -i '^location:' "${response_file}" 2>/dev/null \
        | sed 's/^[Ll]ocation:[[:space:]]*//' \
        | tr -d '\r' \
        | head -n 1)

    if [[ -z "${location}" ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 \
            "302 returned but no Location header found"
        EXIT_CODE=1
        return
    fi

    # Verify the Location starts with the mock authorization endpoint.
    local expected_prefix="http://localhost:${MOCK_KC_PORT}/realms/test/protocol/openid-connect/auth?"
    if [[ "${location}" != "${expected_prefix}"* ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 \
            "Location does not start with ${expected_prefix} (got: ${location:0:120}...)"
        EXIT_CODE=1
        return
    fi

    # Required query params per OIDC + plan:
    local missing=""
    local p
    for p in response_type=code \
             client_id= \
             redirect_uri= \
             scope= \
             state= \
             nonce= \
             code_challenge= \
             code_challenge_method=S256; do
        if [[ "${location}" != *"${p}"* ]]; then
            missing="${missing} ${p}"
        fi
    done

    if [[ -n "${missing}" ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 \
            "Location missing required params:${missing}"
        EXIT_CODE=1
        return
    fi

    # Verify Cache-Control: no-store is set on the redirect.
    if ! "${GREP}" -iq '^cache-control:[[:space:]]*no-store' "${response_file}"; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 \
            "Redirect missing Cache-Control: no-store header"
        EXIT_CODE=1
        return
    fi

    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 \
        "Redirect to IdP has all required params + Cache-Control: no-store"
    PASS_COUNT=$(( PASS_COUNT + 1 ))
}

# Sub-test: GET /api/auth/oidc/start with an unsafe return_to is
# rejected with 400 invalid_return_to before any state is generated.
test_oidc_start_invalid_return_to_rejected() {
    local base_url="$1"
    local response_file="${LOG_PREFIX}${TIMESTAMP}_start_bad_return_to.json"

    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" \
        "GET /api/auth/oidc/start with unsafe return_to returns 400"

    # The %2F%2Fevil.com encoded form decodes to //evil.com — protocol-
    # relative URL. Must be rejected.
    local http_status
    http_status=$(curl -s -X GET -w "%{http_code}" -o "${response_file}" \
        --max-time 5 \
        "${base_url}/api/auth/oidc/start?return_to=%2F%2Fevil.com" 2>/dev/null \
        || echo "000")

    if [[ "${http_status}" != "400" ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 \
            "Expected 400, got ${http_status}"
        EXIT_CODE=1
        return
    fi

    if command -v jq >/dev/null 2>&1; then
        local err
        err=$(jq -r '.error // empty' "${response_file}" 2>/dev/null || echo "")
        if [[ "${err}" != "invalid_return_to" ]]; then
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 \
                "Expected error=invalid_return_to, got error=${err}"
            EXIT_CODE=1
            return
        fi
    fi

    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 \
        "Unsafe return_to rejected with 400 invalid_return_to"
    PASS_COUNT=$(( PASS_COUNT + 1 ))
}

# Sub-test: GET /api/auth/oidc/start with the WRONG method on the
# enabled config still returns 405 (the method check fires before the
# feature gate). This guards against accidentally swapping the order
# of checks in a future refactor.
test_oidc_start_method_check_still_works_when_enabled() {
    local base_url="$1"
    test_oidc_endpoint_method_not_allowed "${base_url}" "start" "POST" \
        "POST /api/auth/oidc/start (enabled config, still 405)"
}

# ---------------------------------------------------------------------------
# Pre-flight: locate binary, validate config
# ---------------------------------------------------------------------------

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Locate Hydrogen Binary"

HYDROGEN_BIN=''
HYDROGEN_BIN_BASE=''
# shellcheck disable=SC2310 # We want to continue even if the test fails
if find_hydrogen_binary "${PROJECT_DIR}"; then
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Using Hydrogen binary: ${HYDROGEN_BIN_BASE}"
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Hydrogen binary found and validated"
    PASS_COUNT=$(( PASS_COUNT + 1 ))
else
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Failed to find Hydrogen binary"
    EXIT_CODE=1
fi

print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Validate Configuration File"
# shellcheck disable=SC2310 # We want to continue even if the test fails
if validate_config_file "${CONFIG_PATH}"; then
    SERVER_PORT=$(get_webserver_port "${CONFIG_PATH}")
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Configuration will use port: ${SERVER_PORT}"
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Configuration file validated"
    PASS_COUNT=$(( PASS_COUNT + 1 ))
else
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Configuration file validation failed"
    EXIT_CODE=1
fi

# ---------------------------------------------------------------------------
# Server lifecycle + endpoint sub-tests
# ---------------------------------------------------------------------------

if [[ "${EXIT_CODE}" -eq 0 ]]; then
    SERVER_LOG="${LOG_FILE}"
    BASE_URL="http://localhost:${SERVER_PORT}"

    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Start Hydrogen Server"
    HYDROGEN_PID=""
    HYDROGEN_PID_VAR="HYDROGEN_PID_$$"
    # shellcheck disable=SC2310 # We want to continue even if the test fails
    if start_hydrogen_with_pid "${CONFIG_PATH}" "${SERVER_LOG}" 15 "${HYDROGEN_BIN}" "${HYDROGEN_PID_VAR}"; then
        HYDROGEN_PID=$(eval "echo \$${HYDROGEN_PID_VAR}")
        if [[ -n "${HYDROGEN_PID}" ]] && ps -p "${HYDROGEN_PID}" > /dev/null 2>&1; then
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Server started with PID ${HYDROGEN_PID}"
            PASS_COUNT=$(( PASS_COUNT + 1 ))
        else
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Server PID missing after start"
            EXIT_CODE=1
        fi
    else
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Failed to start Hydrogen"
        EXIT_CODE=1
    fi

    if [[ "${EXIT_CODE}" -eq 0 ]]; then
        # shellcheck disable=SC2310 # We want to continue even if the test fails
        if ! wait_for_server_ready "${BASE_URL}"; then
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Server failed to become ready"
            EXIT_CODE=1
        fi
    fi

    if [[ "${EXIT_CODE}" -eq 0 ]]; then
        # ---- Disabled-stub envelope (3 endpoints × correct method) ----
        test_oidc_endpoint_disabled "${BASE_URL}" "start"    "GET"  "GET /api/auth/oidc/start"
        test_oidc_endpoint_disabled "${BASE_URL}" "callback" "GET"  "GET /api/auth/oidc/callback"
        test_oidc_endpoint_disabled "${BASE_URL}" "handoff"  "POST" "POST /api/auth/oidc/handoff"

        # ---- Method-not-allowed (one wrong method per endpoint) ----
        # /start is GET-only, /callback is GET-only, /handoff is POST-only
        test_oidc_endpoint_method_not_allowed "${BASE_URL}" "start"    "POST" "POST /api/auth/oidc/start (wrong)"
        test_oidc_endpoint_method_not_allowed "${BASE_URL}" "callback" "POST" "POST /api/auth/oidc/callback (wrong)"
        test_oidc_endpoint_method_not_allowed "${BASE_URL}" "handoff"  "GET"  "GET /api/auth/oidc/handoff (wrong)"

        # ---- 404 for paths outside the OIDC contract ----
        test_oidc_unknown_path_404 "${BASE_URL}" "auth/oidc/nope"     "Unknown OIDC sub-path"
        test_oidc_unknown_path_404 "${BASE_URL}" "auth/oidc/start/x"  "OIDC start with extra segment"

        # ---- Phase 9: mock Keycloak reachability + discovery/JWKS ----
        # The mock is a tiny Node script that serves canned discovery
        # and JWKS docs. The Hydrogen-side code that consumes those is
        # exercised by the Unity tests (oidc_rp_discovery_test_cache,
        # oidc_rp_jwks_test_cache) via the http test seam. These
        # sub-tests just prove the mock works end-to-end against a
        # real HTTP client (curl), so future phases that wire the
        # real flow have a known-good fixture.
        #
        # Phase 10 also relies on the mock IdP being reachable for the
        # /oidc/start redirect test below (Hydrogen fetches discovery
        # from the mock when serving /start with Enabled=true).
        MOCK_KC_STARTED=0
        print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Start mock Keycloak"
        # shellcheck disable=SC2310 # We want to continue even if the test fails
        if start_mock_keycloak; then
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Mock Keycloak started on port ${MOCK_KC_PORT} (PID ${MOCK_KC_PID})"
            PASS_COUNT=$(( PASS_COUNT + 1 ))
            MOCK_KC_STARTED=1

            test_mock_keycloak_reachable
            test_mock_keycloak_discovery_doc
            test_mock_keycloak_jwks_doc
            test_mock_keycloak_token_happy_path
            test_mock_keycloak_token_invalid_grant
            test_mock_keycloak_token_unsupported_grant_type
        else
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Mock Keycloak failed to start"
            # Do not fail the whole test if node is missing — the
            # disabled-stub coverage above is still valid. Phase 9 + 10
            # sub-tests gracefully degrade.
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Skipping mock-Keycloak + Phase 10 sub-tests"
        fi
    fi

    # ---- Shutdown disabled-config Hydrogen before Phase 10 ----
    if [[ -n "${HYDROGEN_PID}" ]] && ps -p "${HYDROGEN_PID}" > /dev/null 2>&1; then
        print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Stop Hydrogen Server (disabled config)"
        # shellcheck disable=SC2310 # We want to continue even if the test fails
        if stop_hydrogen "${HYDROGEN_PID}" "${SERVER_LOG}" 10 5 "${RESULTS_DIR}"; then
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Server stopped cleanly"
            PASS_COUNT=$(( PASS_COUNT + 1 ))
        else
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Server shutdown had issues"
            EXIT_CODE=1
        fi
        # shellcheck disable=SC2310 # Diagnostic-only; non-zero result must not abort the test
        check_time_wait_sockets "${SERVER_PORT}" || true
        HYDROGEN_PID=""
    fi

    # ---- Phase 10: /oidc/start redirect against mock IdP ----
    # Bring up a second Hydrogen instance with OIDC_RP.Enabled=true,
    # pointing at the mock Keycloak we already started above. Skip
    # cleanly if the mock didn't come up (e.g. node missing).
    if [[ "${EXIT_CODE}" -eq 0 ]] && [[ "${MOCK_KC_STARTED:-0}" -eq 1 ]]; then
        print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Validate enabled-mode config"
        # shellcheck disable=SC2310 # We want to continue even if the test fails
        if validate_config_file "${CONFIG_PATH_ENABLED}"; then
            ENABLED_PORT=$(get_webserver_port "${CONFIG_PATH_ENABLED}")
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Enabled config will use port: ${ENABLED_PORT}"
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Enabled config validated"
            PASS_COUNT=$(( PASS_COUNT + 1 ))
        else
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Enabled config validation failed"
            EXIT_CODE=1
        fi

        if [[ "${EXIT_CODE}" -eq 0 ]]; then
            print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Start Hydrogen Server (enabled config)"
            ENABLED_HYDROGEN_PID=""
            ENABLED_PID_VAR="ENABLED_HYDROGEN_PID_$$"
            ENABLED_BASE_URL="http://localhost:${ENABLED_PORT}"
            # shellcheck disable=SC2310 # We want to continue even if the test fails
            if start_hydrogen_with_pid "${CONFIG_PATH_ENABLED}" "${SERVER_LOG}" 15 "${HYDROGEN_BIN}" "${ENABLED_PID_VAR}"; then
                ENABLED_HYDROGEN_PID=$(eval "echo \$${ENABLED_PID_VAR}")
                if [[ -n "${ENABLED_HYDROGEN_PID}" ]] && ps -p "${ENABLED_HYDROGEN_PID}" > /dev/null 2>&1; then
                    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Server started with PID ${ENABLED_HYDROGEN_PID}"
                    PASS_COUNT=$(( PASS_COUNT + 1 ))
                else
                    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Server PID missing after start"
                    EXIT_CODE=1
                fi
            else
                print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Failed to start Hydrogen with enabled config"
                EXIT_CODE=1
            fi

            if [[ "${EXIT_CODE}" -eq 0 ]]; then
                # shellcheck disable=SC2310 # We want to continue even if the test fails
                if ! wait_for_server_ready "${ENABLED_BASE_URL}"; then
                    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Enabled-config server failed to become ready"
                    EXIT_CODE=1
                fi
            fi

            if [[ "${EXIT_CODE}" -eq 0 ]]; then
                # ---- The actual Phase 10 sub-tests ----
                test_oidc_start_redirects_to_idp "${ENABLED_BASE_URL}"
                test_oidc_start_invalid_return_to_rejected "${ENABLED_BASE_URL}"
                test_oidc_start_method_check_still_works_when_enabled "${ENABLED_BASE_URL}"
            fi

            # ---- Shutdown enabled-config Hydrogen ----
            if [[ -n "${ENABLED_HYDROGEN_PID}" ]] && ps -p "${ENABLED_HYDROGEN_PID}" > /dev/null 2>&1; then
                print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Stop Hydrogen Server (enabled config)"
                # shellcheck disable=SC2310 # We want to continue even if the test fails
                if stop_hydrogen "${ENABLED_HYDROGEN_PID}" "${SERVER_LOG}" 10 5 "${RESULTS_DIR}"; then
                    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Enabled-config server stopped cleanly"
                    PASS_COUNT=$(( PASS_COUNT + 1 ))
                else
                    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Enabled-config server shutdown had issues"
                    EXIT_CODE=1
                fi
                # shellcheck disable=SC2310 # Diagnostic-only; non-zero result must not abort the test
                check_time_wait_sockets "${ENABLED_PORT}" || true
            fi
        fi
    fi

    # ---- Stop the mock now that all dependent tests are done ----
    if [[ "${MOCK_KC_STARTED:-0}" -eq 1 ]]; then
        print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Stop mock Keycloak"
        stop_mock_keycloak
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Mock Keycloak stopped"
        PASS_COUNT=$(( PASS_COUNT + 1 ))
    fi
else
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Skipping endpoint tests due to prerequisite failures"
fi

# Print test completion summary
print_test_completion "${TEST_NAME}" "${TEST_ABBR}" "${TEST_NUMBER}" "${TEST_VERSION}"

# Return status code if sourced, exit if run standalone
${ORCHESTRATION:-false} && return "${EXIT_CODE}" || exit "${EXIT_CODE}"
