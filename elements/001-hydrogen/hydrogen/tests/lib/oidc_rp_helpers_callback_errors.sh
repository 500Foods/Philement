#!/usr/bin/env bash

# shellcheck disable=SC2154 # All globals (TEST_NUMBER, TEST_COUNTER, GREP, LOG_PREFIX, TIMESTAMP, MOCK_KC_PORT, etc.) are set by the test script via framework.sh before sourcing this lib
# shellcheck disable=SC2034 # EXIT_CODE is set by the test script and re-exported through these helpers
# shellcheck disable=SC2312 # Several diagnostic command substitutions intentionally swallow the inner exit code

# OIDC RP /callback deep-error sub-tests (Phase 23)
#
# These drive the early /callback failure branches that live *after*
# the no-DB gate but *before* the linker, by forcing the mock Keycloak
# into an error mode via `mock_keycloak_set_mode` and then running the
# full /start -> /auth -> /callback chain (the server POSTs to the
# mock's /token on our behalf). They exercise lines that unit tests
# cannot reach without a live MHD connection + global subsystem state:
#
#   - token_<x>            (oidc_rp_callback.c ~376-393): /token returns
#                          a 400 token-error (e.g. invalid_grant).
#   - id_token_<x>         (oidc_rp_callback.c ~406-420): /token returns a
#                          200 bundle whose id_token fails signature
#                          validation (signed by an unknown key).
#   - server_error (missing id_token, oidc_rp_callback.c ~433-440): /token
#                          returns a 200 bundle without any id_token.
#
# The token-exchange / id-token-validation failures all short-circuit
# before the account linker, so they run against the no-DB enabled
# config instance (port 5243). A subsequent sub-test restores the
# happy path so later phases are unaffected.
#
# CHANGELOG
# 1.0.0 - 2026-07-17 - Phase 23: token-error / id-token-invalid /
#                      id-token-missing deep-error coverage for /callback.

# Drive the /start -> /auth -> /callback chain with the mock in the
# given error mode and assert the final SPA redirect carries the
# expected oidc_error token. Resets the mock mode afterwards.
#
# Arguments:
#   $1 base_url          — the enabled-config Hydrogen instance.
#   $2 token_error       — mock /token forced error ("" = happy).
#   $3 no_id_token       — "1" if /token should omit id_token.
#   $4 id_token_invalid  — "1" if /token should sign an untrusted id_token.
#   $5 expected_error    — the oidc_error= token we expect on the 302.
#   $6 description       — human-readable sub-test label.
oidc_callback_error_subtest() {
    local base_url="$1"
    local token_error="$2"
    local no_id_token="$3"
    local id_token_invalid="$4"
    local expected_error="$5"
    local description="$6"

    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}"

    if [[ -z "${MOCK_KC_PORT:-}" ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 \
            "Mock Keycloak port unknown — cannot drive /token error mode"
        EXIT_CODE=1
        return
    fi

    mock_keycloak_set_mode "${token_error}" "${no_id_token}" "${id_token_invalid}"

    local headers_file="${LOG_PREFIX}${TIMESTAMP}_callback_err_${expected_error}.headers"
    # Follow /start -> /auth -> /callback (two 302 hops) and capture the
    # /callback Location header (the SPA-bound redirect carrying the error).
    curl -s -i -L --max-redirs 2 \
        -o "${headers_file}" \
        --max-time 15 "${base_url}/api/auth/oidc/start" 2>/dev/null || true

    # Always restore the happy path so subsequent phases are unaffected.
    mock_keycloak_reset_mode

    # Derive the HTTP status from the last "HTTP/1.x" status line in the
    # captured headers (more robust than -w with -L + max-redirs).
    local http_status
    http_status=$("${GREP}" -i '^HTTP/' "${headers_file}" 2>/dev/null \
        | tail -n 1 \
        | awk '{print $2}')

    if [[ "${http_status}" != "302" ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 \
            "Expected 302, got ${http_status:-unknown}"
        EXIT_CODE=1
        return
    fi

    local final_location
    final_location=$("${GREP}" -i '^location:' "${headers_file}" 2>/dev/null \
        | sed 's/^[Ll]ocation:[[:space:]]*//' \
        | tr -d '\r' \
        | tail -n 1)

    local err_param
    err_param=$(printf '%s' "${final_location}" \
        | sed -n 's/.*[?&]oidc_error=\([^&]*\).*/\1/p')

    if [[ "${err_param}" != "${expected_error}" ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 \
            "Expected oidc_error=${expected_error}, got '${err_param:-none}' (loc=${final_location:0:160})"
        EXIT_CODE=1
        return
    fi

    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 \
        "302 with oidc_error=${err_param} (${description})"
    PASS_COUNT=$(( PASS_COUNT + 1 ))
}

# /token returns 400 invalid_grant → /callback oidc_error=token_invalid_grant.
test_oidc_callback_token_invalid_grant() {
    oidc_callback_error_subtest "$1" "invalid_grant" "" "" \
        "token_invalid_grant" \
        "Phase 23: /token invalid_grant -> 302 oidc_error=token_invalid_grant"
}

# /token returns a 200 bundle whose id_token is signed under an unknown
# kid → Hydrogen's validator rejects it with oidc_error=id_token_kid_unknown
# (the id_token validation failure branch, oidc_rp_callback.c ~406-420).
test_oidc_callback_id_token_invalid_signature() {
    oidc_callback_error_subtest "$1" "" "" "1" \
        "id_token_kid_unknown" \
        "Phase 23: untrusted id_token -> 302 oidc_error=id_token_kid_unknown"
}

# Drive /start -> /callback against a SQLite-backed instance whose
# provider resolves a real account but is missing (or has a rejected)
# SystemApiKey. The chain succeeds through the linker, then the
# server-side API-key check fails and /callback 302s with
# oidc_error=no_api_key (oidc_rp_callback.c ~500-522).
#
# Arguments:
#   $1 base_url   — the DB-backed Hydrogen instance.
#   $2 description — human-readable sub-test label.
test_oidc_callback_no_api_key() {
    local base_url="$1"
    local description="$2"
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "${description}"

    local headers_file="${LOG_PREFIX}${TIMESTAMP}_callback_no_api_key.headers"
    curl -s -i -L --max-redirs 2 \
        -o "${headers_file}" \
        --max-time 15 "${base_url}/api/auth/oidc/start" 2>/dev/null || true

    local http_status
    http_status=$("${GREP}" -i '^HTTP/' "${headers_file}" 2>/dev/null \
        | tail -n 1 | awk '{print $2}')

    if [[ "${http_status}" != "302" ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 \
            "Expected 302, got ${http_status:-unknown}"
        EXIT_CODE=1
        return
    fi

    local final_location
    final_location=$("${GREP}" -i '^location:' "${headers_file}" 2>/dev/null \
        | sed 's/^[Ll]ocation:[[:space:]]*//' | tr -d '\r' | tail -n 1)

    local err_param
    err_param=$(printf '%s' "${final_location}" \
        | sed -n 's/.*[?&]oidc_error=\([^&]*\).*/\1/p')

    if [[ "${err_param}" != "no_api_key" ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 \
            "Expected oidc_error=no_api_key, got '${err_param:-none}' (loc=${final_location:0:160})"
        EXIT_CODE=1
        return
    fi

    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 \
        "302 with oidc_error=no_api_key (${description})"
    PASS_COUNT=$(( PASS_COUNT + 1 ))
}
