#!/usr/bin/env bash

# shellcheck disable=SC2154 # All globals (TEST_NUMBER, TEST_COUNTER, GREP, LOG_PREFIX, TIMESTAMP, MOCK_KC_*, etc.) are set by the test script via framework.sh before sourcing this lib
# shellcheck disable=SC2034 # EXIT_CODE is set by the test script and re-exported through these helpers
# shellcheck disable=SC2312 # Several diagnostic command substitutions intentionally swallow the inner exit code; helpers either fall back gracefully or || true the outer call

# CHANGELOG
# 1.1.0 - 2026-06-20 - Added wait_for_migration_ready (canonical "READY FOR REQUESTS" signal) to
#                      replace the per-phase tail-offset migration-wait loops that timed out ~30s
#                      each (shared SERVER_LOG is truncated per instance, so stale offsets matched
#                      nothing). Cuts Test 42 runtime from ~250s to a few seconds of real work.
# 1.0.0 - 2026-05-09 - Initial extraction from test_42_oidc_rp.sh during Phase 13.

wait_for_migration_ready() {
    local server_log="$1"
    local timeout="${2:-30}"
    local deadline
    deadline=$(( $(date +%s) + timeout ))
    while true; do
        if [[ $(date +%s) -ge ${deadline} ]]; then
            return 1
        fi
        # shellcheck disable=SC2310 # Polling on success/timeout — non-zero grep is "not yet ready"
        if "${GREP}" -q -E "READY FOR REQUESTS|Migration completed in|Migration Current:" "${server_log}" 2>/dev/null; then
            # Brief settling pause so the QTC is fully usable by
            # handle_api_request paths (matches test_40's convention).
            sleep 1
            return 0
        fi
        sleep 0.2
    done
}

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

# Note: the EXIT trap that calls stop_mock_keycloak is registered in
# the test script (test_42_oidc_rp.sh) rather than this library,
# because bash traps belong to the executing process. Sourcing this
# file does not register the trap on its own.

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
        local key_count first_kid first_n_len
        key_count=$(jq '.keys | length' "${response_file}" 2>/dev/null || echo "0")
        first_kid=$(jq -r '.keys[0].kid // empty' "${response_file}" 2>/dev/null || echo "")
        # Phase 12: the JWKS now ships a real RSA modulus. The base64url
        # of a 2048-bit modulus is ~342 chars; treat anything < 100 as
        # the old placeholder.
        first_n_len=$(jq -r '.keys[0].n // empty' "${response_file}" 2>/dev/null | wc -c | tr -d ' ')
        if [[ "${key_count}" -ge 1 ]] && [[ -n "${first_kid}" ]] && [[ "${first_n_len}" -ge 100 ]]; then
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "JWKS has ${key_count} key(s); kid=${first_kid}; modulus length=${first_n_len} chars"
            PASS_COUNT=$(( PASS_COUNT + 1 ))
        else
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "JWKS missing required fields (count=${key_count} kid=${first_kid} n_len=${first_n_len})"
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
# expires_in. Phase 12 expectation: id_token is a real RS256-signed
# JWT (3 segments separated by '.', each base64url) — not the Phase 11
# placeholder string.
test_mock_keycloak_token_happy_path() {
    local response_file="${LOG_PREFIX}${TIMESTAMP}_mock_kc_token_ok.json"
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" \
        "Mock Keycloak /token returns 200 + signed JWT for test-code-ok"

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
        local id_tok acc_tok tok_type seg_count
        id_tok=$(jq -r '.id_token // empty' "${response_file}" 2>/dev/null || echo "")
        acc_tok=$(jq -r '.access_token // empty' "${response_file}" 2>/dev/null || echo "")
        tok_type=$(jq -r '.token_type // empty' "${response_file}" 2>/dev/null || echo "")
        # Phase 12: id_token must be a 3-segment compact JWS.
        if [[ -n "${id_tok}" ]]; then
            seg_count=$(awk -F. '{print NF}' <<< "${id_tok}")
        else
            seg_count=0
        fi
        if [[ -n "${id_tok}" ]] && [[ -n "${acc_tok}" ]] && [[ "${tok_type}" == "Bearer" ]] && [[ "${seg_count}" -eq 3 ]]; then
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 \
                "Token endpoint returned signed id_token (3 segments) + access_token + Bearer"
            PASS_COUNT=$(( PASS_COUNT + 1 ))
        else
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 \
                "Token endpoint response invalid (id_seg=${seg_count} acc=${acc_tok:0:8}.. type=${tok_type})"
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

# Phase 12 sub-test: verify that the mock's signed id_token has
# Keycloak-shaped header (alg=RS256, kid=test-key-1) and required
# OIDC claims (iss, sub, aud, exp, iat, nonce). This is a black-box
# sanity check on the mock; Hydrogen-side validation has full
# coverage in the Unity tests.
test_mock_keycloak_id_token_header_and_claims() {
    local response_file="${LOG_PREFIX}${TIMESTAMP}_mock_kc_token_jwt.json"
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" \
        "Mock Keycloak id_token header + claims match expected shape"

    local body="grant_type=authorization_code"
    body="${body}&code=test-code-ok"
    body="${body}&redirect_uri=http%3A%2F%2Flocalhost%3A5243%2Fcb"
    body="${body}&code_verifier=v"
    body="${body}&client_id=lithium-web"
    body="${body}&nonce=phase12-nonce"

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

    if ! command -v jq >/dev/null 2>&1; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "jq not available; skipping JWT decode sub-test"
        return
    fi

    local id_tok header_b64 payload_b64
    id_tok=$(jq -r '.id_token // empty' "${response_file}" 2>/dev/null || echo "")
    header_b64=$(awk -F. '{print $1}' <<< "${id_tok}")
    payload_b64=$(awk -F. '{print $2}' <<< "${id_tok}")

    if [[ -z "${header_b64}" ]] || [[ -z "${payload_b64}" ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "id_token has empty header/payload segments"
        EXIT_CODE=1
        return
    fi

    # base64url-decode helper: pad, swap chars, decode.
    decode_b64u() {
        local s="${1//-/+}"
        s="${s//_//}"
        local pad=$(( (4 - ${#s} % 4) % 4 ))
        local i
        for (( i = 0; i < pad; i++ )); do s="${s}="; done
        printf '%s' "${s}" | base64 -d 2>/dev/null
    }

    local header_json payload_json
    header_json=$(decode_b64u "${header_b64}")
    payload_json=$(decode_b64u "${payload_b64}")

    local alg kid iss sub aud exp iat nonce_claim
    alg=$(jq -r '.alg // empty' <<< "${header_json}" 2>/dev/null || echo "")
    kid=$(jq -r '.kid // empty' <<< "${header_json}" 2>/dev/null || echo "")
    iss=$(jq -r '.iss // empty' <<< "${payload_json}" 2>/dev/null || echo "")
    sub=$(jq -r '.sub // empty' <<< "${payload_json}" 2>/dev/null || echo "")
    aud=$(jq -r '.aud // empty' <<< "${payload_json}" 2>/dev/null || echo "")
    exp=$(jq -r '.exp // empty' <<< "${payload_json}" 2>/dev/null || echo "")
    iat=$(jq -r '.iat // empty' <<< "${payload_json}" 2>/dev/null || echo "")
    nonce_claim=$(jq -r '.nonce // empty' <<< "${payload_json}" 2>/dev/null || echo "")

    if [[ "${alg}" == "RS256" ]] \
        && [[ "${kid}" == "test-key-1" ]] \
        && [[ -n "${iss}" ]] \
        && [[ -n "${sub}" ]] \
        && [[ "${aud}" == "lithium-web" ]] \
        && [[ -n "${exp}" ]] \
        && [[ -n "${iat}" ]] \
        && [[ "${nonce_claim}" == "phase12-nonce" ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 \
            "id_token header alg=${alg} kid=${kid}; aud=${aud} nonce=${nonce_claim}"
        PASS_COUNT=$(( PASS_COUNT + 1 ))
    else
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 \
            "id_token shape mismatch (alg=${alg} kid=${kid} iss=${iss:0:20}.. aud=${aud} nonce=${nonce_claim})"
        EXIT_CODE=1
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
# Phase 13: /oidc/handoff exchange (with debug injector)
# ---------------------------------------------------------------------------
#
# These sub-tests exercise the real handoff endpoint end-to-end. The
# debug injector (`POST /api/auth/oidc/_inject_handoff`) is compiled
# in only when NDEBUG is undefined; the regular build used by the
# test harness includes it. The release build excludes it.
#
# The injection endpoint takes operator-supplied JWT/account_id/etc.
# (because Phase 14 hasn't wired /callback yet), inserts a record
# into the handoff store, and returns the generated handoff code.
# The black-box flow is then: inject → exchange → assert envelope.
#
# Note: the enabled-config has BindHandoffToIp=false, so localhost
# self-IP-bind variations don't perturb these tests. IP-bind logic
# is exercised by the Unity tests indirectly (via the store) and by
# code inspection of `oidc_rp_handoff.c`.

# Inject a handoff record via the debug endpoint and echo the code
# on stdout. Returns 0 on success, non-zero on failure.
inject_handoff() {
    local base_url="$1"
    local jwt="$2"
    local account_id="$3"
    local username="${4:-}"
    local roles="${5:-}"
    local response_file="$6"

    local body
    if [[ -n "${username}" ]] && [[ -n "${roles}" ]]; then
        body=$(printf '{"jwt":"%s","account_id":%s,"username":"%s","roles":"%s","expires_at":1893456000,"ttl_seconds":60}' \
            "${jwt}" "${account_id}" "${username}" "${roles}")
    else
        body=$(printf '{"jwt":"%s","account_id":%s,"expires_at":1893456000,"ttl_seconds":60}' \
            "${jwt}" "${account_id}")
    fi

    local http_status
    http_status=$(curl -s -X POST -H "Content-Type: application/json" \
        -d "${body}" -w "%{http_code}" -o "${response_file}" \
        --max-time 5 "${base_url}/api/auth/oidc/_inject_handoff" \
        2>/dev/null || echo "000")
    if [[ "${http_status}" != "200" ]]; then
        return 1
    fi
    return 0
}

# Sub-test: happy path. Inject a record, exchange the code, verify
# the success envelope mirrors /auth/login (token + user_id +
# username + roles + expires_at).
test_oidc_handoff_happy_path() {
    local base_url="$1"
    local inject_file="${LOG_PREFIX}${TIMESTAMP}_handoff_inject_happy.json"
    local exchange_file="${LOG_PREFIX}${TIMESTAMP}_handoff_exchange_happy.json"

    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" \
        "Phase 13: inject handoff, exchange, expect 200 envelope"

    # Step 1: inject.
    # shellcheck disable=SC2310 # We want to continue even if the test fails
    if ! inject_handoff "${base_url}" "fake.jwt.token" "42" "alice" "admin,user" \
        "${inject_file}"; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 \
            "inject_handoff failed (debug endpoint unreachable?)"
        EXIT_CODE=1
        return
    fi

    if ! command -v jq >/dev/null 2>&1; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" \
            "jq missing — Phase 13 happy-path test requires it for assertions"
        # Soft-skip; not a hard failure if jq is unavailable.
        return
    fi

    local handoff_code
    handoff_code=$(jq -r '.handoff // empty' "${inject_file}" 2>/dev/null || echo "")
    if [[ -z "${handoff_code}" ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 \
            "Inject response missing .handoff field"
        EXIT_CODE=1
        return
    fi

    # Step 2: exchange.
    local body
    body=$(printf '{"handoff":"%s"}' "${handoff_code}")
    local http_status
    http_status=$(curl -s -X POST -H "Content-Type: application/json" \
        -d "${body}" -w "%{http_code}" -o "${exchange_file}" \
        --max-time 5 "${base_url}/api/auth/oidc/handoff" \
        2>/dev/null || echo "000")

    if [[ "${http_status}" != "200" ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 \
            "Exchange returned ${http_status} (expected 200)"
        EXIT_CODE=1
        return
    fi

    # Step 3: assert envelope.
    local token user_id username roles success
    token=$(jq -r '.token // empty' "${exchange_file}" 2>/dev/null || echo "")
    user_id=$(jq -r '.user_id // empty' "${exchange_file}" 2>/dev/null || echo "")
    username=$(jq -r '.username // empty' "${exchange_file}" 2>/dev/null || echo "")
    roles=$(jq -r '.roles // empty' "${exchange_file}" 2>/dev/null || echo "")
    success=$(jq -r '.success // empty' "${exchange_file}" 2>/dev/null || echo "")

    if [[ "${token}" == "fake.jwt.token" ]] \
        && [[ "${user_id}" == "42" ]] \
        && [[ "${username}" == "alice" ]] \
        && [[ "${roles}" == "admin,user" ]] \
        && [[ "${success}" == "true" ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 \
            "200 envelope OK (token, user_id, username, roles all match)"
        PASS_COUNT=$(( PASS_COUNT + 1 ))
    else
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 \
            "Envelope mismatch: token=${token}, user_id=${user_id}, username=${username}, roles=${roles}, success=${success}"
        EXIT_CODE=1
    fi
}

# Sub-test: replay. Inject, exchange (consumes), then exchange the
# same code again — must return 401 handoff_invalid (atomic take).
test_oidc_handoff_replay_returns_401() {
    local base_url="$1"
    local inject_file="${LOG_PREFIX}${TIMESTAMP}_handoff_inject_replay.json"
    local first_file="${LOG_PREFIX}${TIMESTAMP}_handoff_exchange_first.json"
    local replay_file="${LOG_PREFIX}${TIMESTAMP}_handoff_exchange_replay.json"

    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" \
        "Phase 13: replay of consumed handoff returns 401 handoff_invalid"

    # shellcheck disable=SC2310 # We want to continue even if the test fails
    if ! inject_handoff "${base_url}" "replay.jwt" "7" "" "" "${inject_file}"; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "inject failed"
        EXIT_CODE=1
        return
    fi

    if ! command -v jq >/dev/null 2>&1; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "jq missing — soft skip"
        return
    fi

    local handoff_code
    handoff_code=$(jq -r '.handoff // empty' "${inject_file}" 2>/dev/null || echo "")
    if [[ -z "${handoff_code}" ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 \
            "Inject response missing .handoff field"
        EXIT_CODE=1
        return
    fi

    local body first_status replay_status
    body=$(printf '{"handoff":"%s"}' "${handoff_code}")

    first_status=$(curl -s -X POST -H "Content-Type: application/json" \
        -d "${body}" -w "%{http_code}" -o "${first_file}" \
        --max-time 5 "${base_url}/api/auth/oidc/handoff" \
        2>/dev/null || echo "000")
    replay_status=$(curl -s -X POST -H "Content-Type: application/json" \
        -d "${body}" -w "%{http_code}" -o "${replay_file}" \
        --max-time 5 "${base_url}/api/auth/oidc/handoff" \
        2>/dev/null || echo "000")

    if [[ "${first_status}" == "200" ]] && [[ "${replay_status}" == "401" ]]; then
        local replay_error
        replay_error=$(jq -r '.error // empty' "${replay_file}" 2>/dev/null || echo "")
        if [[ "${replay_error}" == "handoff_invalid" ]]; then
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 \
                "First exchange 200, replay returns 401 handoff_invalid"
            PASS_COUNT=$(( PASS_COUNT + 1 ))
        else
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 \
                "Replay returned 401 but error='${replay_error}' (expected handoff_invalid)"
            EXIT_CODE=1
        fi
    else
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 \
            "Bad status sequence: first=${first_status}, replay=${replay_status}"
        EXIT_CODE=1
    fi
}

# Sub-test: unknown code (never injected) returns 401.
test_oidc_handoff_unknown_code_returns_401() {
    local base_url="$1"
    local response_file="${LOG_PREFIX}${TIMESTAMP}_handoff_unknown.json"
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" \
        "Phase 13: unknown handoff code returns 401 handoff_invalid"
    # shellcheck disable=SC2310 # We want to continue even if the test fails
    if validate_oidc_request "${base_url}/api/auth/oidc/handoff" "POST" "401" \
        "handoff_invalid" "${response_file}" \
        '{"handoff":"never-was-inserted-anywhere-deadbeef"}'; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 \
            "Unknown handoff returns 401 handoff_invalid"
        PASS_COUNT=$(( PASS_COUNT + 1 ))
    else
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 \
            "Unknown handoff did not return 401 handoff_invalid"
        EXIT_CODE=1
    fi
}

# Sub-test: missing/empty `handoff` field in body returns 401.
test_oidc_handoff_missing_field_returns_401() {
    local base_url="$1"
    local response_file="${LOG_PREFIX}${TIMESTAMP}_handoff_missing.json"
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" \
        "Phase 13: missing handoff field returns 401 handoff_invalid"
    # shellcheck disable=SC2310 # We want to continue even if the test fails
    if validate_oidc_request "${base_url}/api/auth/oidc/handoff" "POST" "401" \
        "handoff_invalid" "${response_file}" '{"not_a_handoff":"x"}'; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 \
            "Missing handoff field returns 401 handoff_invalid"
        PASS_COUNT=$(( PASS_COUNT + 1 ))
    else
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 \
            "Missing handoff field did not return 401 handoff_invalid"
        EXIT_CODE=1
    fi
}

# Sub-test: GET on /handoff still returns 405 when feature is enabled.
# (Method check has higher priority than the feature gate, so disabled
# config also returns 405; this test confirms the same in enabled mode.)
test_oidc_handoff_method_check_still_works_when_enabled() {
    local base_url="$1"
    test_oidc_endpoint_method_not_allowed "${base_url}" "handoff" "GET" \
        "GET /api/auth/oidc/handoff (enabled config, still 405)"
}


# ---------------------------------------------------------------------------
# Phase 14: /oidc/callback helpers
# ---------------------------------------------------------------------------
#
# Phase 14's failure-path + happy-path coverage lives in a sibling
# library (tests/lib/oidc_rp_helpers_callback.sh) so this file stays
# below the project's 1,000-line cap. The orchestrator
# (tests/test_42_oidc_rp.sh) sources both libraries.
