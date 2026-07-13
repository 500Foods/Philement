#!/usr/bin/env bash

# shellcheck disable=SC2154 # All globals (TEST_NUMBER, TEST_COUNTER, GREP, LOG_PREFIX, TIMESTAMP, etc.) are set by the test script via framework.sh before sourcing this lib
# shellcheck disable=SC2034 # EXIT_CODE is set by the test script and re-exported through these helpers
# shellcheck disable=SC2312 # Several diagnostic command substitutions intentionally swallow the inner exit code

# OIDC RP /callback Test Utilities (Phase 14)
#
# Helpers for testing the Phase 14 /api/auth/oidc/callback chain.
# Split out of tests/lib/oidc_rp_helpers.sh to keep both files under
# the project's 1,000-line cap.
#
# Provides:
#   - extract_oidc_error_param:           pull oidc_error from a 302
#   - test_oidc_callback_*_redirects_*:   typed-error redirect coverage
#   - test_oidc_callback_method_check_*:  enabled-mode method check
#   - test_oidc_callback_replay_returns_state_invalid: replay defence
#   - test_oidc_callback_happy_path_end_to_end: full /start -> handoff
#
# The full happy-path test requires the Phase 14 SQLite-backed config
# (`hydrogen_test_42_oidc_rp_full.json`) and the demo Acuranzo schema;
# the failure-path tests run against the no-DB enabled config.
#
# CHANGELOG
# 1.1.0 - 2026-07-13 - Phase 22: test_oidc_end_session_happy_path drives /end-session with the happy-path OIDC JWT; happy-path stashes token in OIDC_RP_LAST_JWT
# 1.0.0 - 2026-05-09 - Initial extraction during Phase 14 to keep
#                      oidc_rp_helpers.sh below the 1,000-line cap.

# ---------------------------------------------------------------------------
# Phase 14: /oidc/callback failure paths (enabled config, no DB needed)
# ---------------------------------------------------------------------------
#
# These exercise the typed error vocabulary described in
# oidc_rp_callback.h. Each one drives /callback directly with hand-
# crafted query parameters and verifies the resulting 302 carries the
# expected ?oidc_error=<name> token. None of them touch the database
# or mint a JWT; they fail before that step.
#
# The callback always 302s on failure (the user agent is the browser,
# not an SPA fetch). The Location header carries the error code.

# Extract the value of an `oidc_error=...` parameter from a curl -i
# response file. Returns the literal string (or empty if absent).
extract_oidc_error_param() {
    local headers_file="$1"
    "${GREP}" -i '^location:' "${headers_file}" 2>/dev/null \
        | sed 's/^[Ll]ocation:[[:space:]]*//' \
        | tr -d '\r' \
        | head -n 1 \
        | sed -n 's/.*[?&]oidc_error=\([^&]*\).*/\1/p'
}

# Sub-test: /callback with no query parameters at all redirects with
# oidc_error=state_invalid (because state is required).
test_oidc_callback_missing_params_redirects_state_invalid() {
    local base_url="$1"
    local response_file="${LOG_PREFIX}${TIMESTAMP}_callback_no_params.headers"
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" \
        "Phase 14: /callback with no params -> 302 oidc_error=state_invalid"

    local http_status
    http_status=$(curl -s -i -o "${response_file}" -w "%{http_code}" \
        --max-time 5 "${base_url}/api/auth/oidc/callback" 2>/dev/null \
        || echo "000")

    if [[ "${http_status}" != "302" ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 \
            "Expected 302, got ${http_status}"
        EXIT_CODE=1
        return
    fi

    local err_param
    err_param=$(extract_oidc_error_param "${response_file}")
    if [[ "${err_param}" != "state_invalid" ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 \
            "Expected oidc_error=state_invalid, got '${err_param}'"
        EXIT_CODE=1
        return
    fi

    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 \
        "302 with oidc_error=state_invalid (missing code/state)"
    PASS_COUNT=$(( PASS_COUNT + 1 ))
}

# Sub-test: /callback with an unknown state value redirects with
# oidc_error=state_invalid (state lookup fails atomically).
test_oidc_callback_unknown_state_redirects_state_invalid() {
    local base_url="$1"
    local response_file="${LOG_PREFIX}${TIMESTAMP}_callback_unknown_state.headers"
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" \
        "Phase 14: /callback with unknown state -> 302 oidc_error=state_invalid"

    local http_status
    http_status=$(curl -s -i -o "${response_file}" -w "%{http_code}" \
        --max-time 5 \
        "${base_url}/api/auth/oidc/callback?code=any&state=never-was-issued-deadbeef" \
        2>/dev/null || echo "000")

    if [[ "${http_status}" != "302" ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 \
            "Expected 302, got ${http_status}"
        EXIT_CODE=1
        return
    fi

    local err_param
    err_param=$(extract_oidc_error_param "${response_file}")
    if [[ "${err_param}" != "state_invalid" ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 \
            "Expected oidc_error=state_invalid, got '${err_param}'"
        EXIT_CODE=1
        return
    fi

    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 \
        "302 with oidc_error=state_invalid (unknown state)"
    PASS_COUNT=$(( PASS_COUNT + 1 ))
}

# Sub-test: /callback with an IdP-supplied ?error= passes it through
# as oidc_error=idp_error (we never echo the IdP's error string).
test_oidc_callback_idp_error_redirects_idp_error() {
    local base_url="$1"
    local response_file="${LOG_PREFIX}${TIMESTAMP}_callback_idp_error.headers"
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" \
        "Phase 14: /callback with IdP error -> 302 oidc_error=idp_error"

    local http_status
    http_status=$(curl -s -i -o "${response_file}" -w "%{http_code}" \
        --max-time 5 \
        "${base_url}/api/auth/oidc/callback?error=access_denied&error_description=User%20canceled" \
        2>/dev/null || echo "000")

    if [[ "${http_status}" != "302" ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 \
            "Expected 302, got ${http_status}"
        EXIT_CODE=1
        return
    fi

    local err_param
    err_param=$(extract_oidc_error_param "${response_file}")
    if [[ "${err_param}" != "idp_error" ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 \
            "Expected oidc_error=idp_error, got '${err_param}'"
        EXIT_CODE=1
        return
    fi

    # Belt-and-suspenders: confirm the raw error_description string did
    # NOT leak into the redirect URL. Echoing it would be an injection
    # surface.
    local location
    location=$("${GREP}" -i '^location:' "${response_file}" 2>/dev/null \
        | sed 's/^[Ll]ocation:[[:space:]]*//' \
        | tr -d '\r' \
        | head -n 1)
    if [[ "${location}" == *"User"* ]] || [[ "${location}" == *"canceled"* ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 \
            "IdP error_description leaked into Location header"
        EXIT_CODE=1
        return
    fi

    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 \
        "302 with oidc_error=idp_error (IdP description not echoed)"
    PASS_COUNT=$(( PASS_COUNT + 1 ))
}

# Sub-test: /callback method discrimination still fires when the
# feature is enabled (POST on a GET-only endpoint -> 405).
test_oidc_callback_method_check_still_works_when_enabled() {
    local base_url="$1"
    test_oidc_endpoint_method_not_allowed "${base_url}" "callback" "POST" \
        "POST /api/auth/oidc/callback (enabled, still 405)"
}

# ---------------------------------------------------------------------------
# Phase 14: full happy-path against SQLite-backed Hydrogen
# ---------------------------------------------------------------------------
#
# The happy path needs all five moving parts:
#   1. Mock IdP (already running from the Phase 9-13 block).
#   2. SQLite-backed Hydrogen with OIDC_RP enabled and the demo
#      Acuranzo schema, with an identity row pre-seeded for
#      adminuser (account_id=1) so the sub fast-path fires (Phase 21).
#   3. The mock's /auth endpoint redirecting back to Hydrogen's
#      /callback with code=test-code-ok (Phase 14 mock change).
#   4. /callback minting a real Hydrogen JWT and putting a handoff
#      record using the real linker (stub removed in Phase 21).
#   5. /handoff exchanging the code for the JWT envelope (Phase 13).
#
# The driver curl follows the redirect chain (start -> auth -> callback)
# with -L, capturing the final URL so we can extract the handoff
# parameter.

# Drive the full /start -> /auth -> /callback chain with curl following
# all 302s, capture the final URL, then exchange the handoff. Asserts
# the SPA-shaped envelope (token + user_id + username + roles).
test_oidc_callback_happy_path_end_to_end() {
    local base_url="$1"
    local trace_file="${LOG_PREFIX}${TIMESTAMP}_callback_happy_trace.txt"
    local exchange_file="${LOG_PREFIX}${TIMESTAMP}_callback_happy_exchange.json"

    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" \
        "Phase 14: /start -> /auth -> /callback -> handoff -> JWT envelope"

    if ! command -v jq >/dev/null 2>&1; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" \
            "jq missing — Phase 14 happy-path requires it"
        return
    fi

    # Step 1: hit /start, follow redirects through /auth to /callback,
    # but STOP before the final /callback -> /login redirect. The SPA
    # /login URL is on the Hydrogen origin (because our test has no
    # SPA listener); following it would 404. --max-redirs 2 lets curl
    # follow /start->/auth->/callback (two hops) and capture the
    # /callback's Location header in -i output, which carries our
    # handoff code.
    local headers_file="${LOG_PREFIX}${TIMESTAMP}_callback_happy_headers.txt"
    local http_status
    http_status=$(curl -s -i -L --max-redirs 2 \
        -o "${headers_file}" -w '%{http_code}' \
        --max-time 15 \
        "${base_url}/api/auth/oidc/start" 2>/dev/null || echo "000")

    # Save trace for diagnostics
    cp "${headers_file}" "${trace_file}" 2>/dev/null || true

    # The final response is /callback's 302 -> /login... We grep all
    # Location headers and pick the LAST one (the SPA-bound redirect).
    local final_location
    final_location=$("${GREP}" -i '^location:' "${headers_file}" 2>/dev/null \
        | sed 's/^[Ll]ocation:[[:space:]]*//' \
        | tr -d '\r' \
        | tail -n 1)

    if [[ -z "${final_location}" ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 \
            "No Location header in chain (status=${http_status})"
        EXIT_CODE=1
        return
    fi

    local handoff_code
    handoff_code=$(printf '%s' "${final_location}" \
        | sed -n 's/.*[?&]handoff=\([^&]*\).*/\1/p')

    if [[ -z "${handoff_code}" ]]; then
        local err_param
        err_param=$(printf '%s' "${final_location}" \
            | sed -n 's/.*[?&]oidc_error=\([^&]*\).*/\1/p')
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 \
            "Final Location had no handoff (oidc_error=${err_param:-none}, status=${http_status}, loc=${final_location:0:160})"
        EXIT_CODE=1
        return
    fi

    # Step 2: exchange the handoff code at /handoff.
    local body
    body=$(printf '{"handoff":"%s"}' "${handoff_code}")
    local exchange_status
    exchange_status=$(curl -s -X POST -H "Content-Type: application/json" \
        -d "${body}" -w "%{http_code}" -o "${exchange_file}" \
        --max-time 5 "${base_url}/api/auth/oidc/handoff" \
        2>/dev/null || echo "000")

    if [[ "${exchange_status}" != "200" ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 \
            "/handoff exchange returned ${exchange_status} (expected 200)"
        EXIT_CODE=1
        return
    fi

    # Step 3: assert envelope shape — must be indistinguishable from
    # the password-login response.
    local token user_id username success
    token=$(jq -r '.token // empty'    "${exchange_file}" 2>/dev/null || echo "")
    user_id=$(jq -r '.user_id // empty' "${exchange_file}" 2>/dev/null || echo "")
    username=$(jq -r '.username // empty' "${exchange_file}" 2>/dev/null || echo "")
    success=$(jq -r '.success // empty' "${exchange_file}" 2>/dev/null || echo "")

    # Stash the freshly-minted JWT so the RP-initiated logout step
    # (test_oidc_end_session_happy_path) can drive /end-session with it.
    OIDC_RP_LAST_JWT="${token}"

    # The JWT is opaque — assert it has the JWS three-segment shape
    # (header.payload.signature) without trying to decode it.
    if [[ "${success}" != "true" ]] \
        || [[ -z "${token}" ]] \
        || [[ "${token}" != *"."*"."* ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 \
            "Envelope malformed: success=${success}, token=${token:0:32}..."
        EXIT_CODE=1
        return
    fi

     # The pre-seeded identity links to account_id=1 (adminuser). The
     # username in the handoff envelope now comes from the IdP's
     # preferred_username claim (real linker, Phase 21 — no longer the
     # stub's fixed "adminuser"). Assert user_id=1 and non-empty username.
     if [[ "${user_id}" != "1" ]]; then
         print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 \
             "Expected user_id=1 (seeded identity for adminuser), got user_id=${user_id}"
         EXIT_CODE=1
         return
     fi

     if [[ -z "${username}" ]]; then
         print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 \
             "Expected non-empty username from IdP claims, got empty string"
         EXIT_CODE=1
         return
     fi

    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 \
        "Full chain OK: JWT envelope user_id=${user_id} username=${username} (real linker, sub fast-path)"
    PASS_COUNT=$(( PASS_COUNT + 1 ))
}

# RP-initiated logout (Phase 22): POST the JWT obtained during the
# happy-path login to /api/auth/oidc/end-session. The endpoint validates
# the Hydrogen JWT, deletes it from storage, and — because the JWT carries
# OIDC context (id_token + idp_provider) and the active provider's
# discovery doc advertises an end_session_endpoint — returns a
# redirect_url that points at the IdP's logout endpoint. This exercises
# the previously-uncovered handler in src/api/auth/oidc_rp/oidc_rp_end_session.c.
#
# Relies on OIDC_RP_LAST_JWT being populated by
# test_oidc_callback_happy_path_end_to_end (same server / login session).
test_oidc_end_session_happy_path() {
    local base_url="$1"
    local es_file="${LOG_PREFIX}${TIMESTAMP}_end_session.json"

    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" \
        "Phase 22: RP-initiated logout /end-session with OIDC JWT"

    if ! command -v jq >/dev/null 2>&1; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" \
            "jq missing — end-session coverage requires it"
        return
    fi

    if [[ -z "${OIDC_RP_LAST_JWT:-}" ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 \
            "No JWT captured from happy-path login (OIDC_RP_LAST_JWT empty)"
        EXIT_CODE=1
        return
    fi

    # Method check: /end-session is POST-only. A GET must be rejected
    # with 405 without consuming the body.
    local get_status
    get_status=$(curl -s -o /dev/null -w '%{http_code}' --max-time 5 \
        "${base_url}/api/auth/oidc/end-session" 2>/dev/null || echo "000")
    if [[ "${get_status}" != "405" ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 \
            "/end-session GET returned ${get_status} (expected 405)"
        EXIT_CODE=1
        return
    fi

    # Happy path: POST the Bearer JWT. Expect 200 + JSON body whose
    # redirect_url is a non-empty IdP logout URL.
    local es_status
    es_status=$(curl -s -X POST \
        -H "Authorization: Bearer ${OIDC_RP_LAST_JWT}" \
        -w '%{http_code}' -o "${es_file}" \
        --max-time 10 \
        "${base_url}/api/auth/oidc/end-session" 2>/dev/null || echo "000")

    if [[ "${es_status}" != "200" ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 \
            "/end-session POST returned ${es_status} (expected 200)"
        EXIT_CODE=1
        return
    fi

    local redirect_url
    redirect_url=$(jq -r '.redirect_url // empty' "${es_file}" 2>/dev/null || echo "")

    if [[ -z "${redirect_url}" ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 \
            "/end-session returned empty redirect_url (expected IdP logout URL)"
        EXIT_CODE=1
        return
    fi

    # The URL must be the IdP's end_session_endpoint with the
    # id_token_hint + post_logout_redirect_uri + client_id query params.
    if [[ "${redirect_url}" != *"id_token_hint="* ]] \
        || [[ "${redirect_url}" != *"post_logout_redirect_uri="* ]] \
        || [[ "${redirect_url}" != *"client_id="* ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 \
            "/end-session redirect_url missing expected params: ${redirect_url:0:200}"
        EXIT_CODE=1
        return
    fi

    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 \
        "RP-initiated logout OK: redirect_url present (${redirect_url:0:96}...)"
    PASS_COUNT=$(( PASS_COUNT + 1 ))
}

# Replay defence at the /callback layer: a successfully-consumed
# state cannot be re-used. Driving /start twice produces two distinct
# (state, code) pairs, but driving /callback twice with the same
# captured state must fail the second time. Easiest way to test this
# without recovering Hydrogen's internal state is to drive the chain
# once successfully (above), then re-issue the SAME /callback URL
# (preserved in the trace) and verify state_invalid.
#
# Implementation note: this captures the callback URL during the
# happy path. We don't repeat the full chain here; instead we use
# a fresh /start, capture its /callback URL by reading the last
# redirect target, and replay it.
test_oidc_callback_replay_returns_state_invalid() {
    local base_url="$1"
    local first_trace="${LOG_PREFIX}${TIMESTAMP}_callback_replay_first.txt"
    local replay_headers="${LOG_PREFIX}${TIMESTAMP}_callback_replay_second.headers"

    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" \
        "Phase 14: replaying a consumed state -> 302 oidc_error=state_invalid"

    # Run /start to /callback chain once. We don't care about the
    # outcome here — we just need a state to consume. Use --max-redirs 4
    # to avoid following the SPA-bound redirect.
    curl -s -o "${first_trace}" --max-redirs 4 \
        --max-time 10 \
        "${base_url}/api/auth/oidc/start" >/dev/null 2>&1 || true

    # Run /start again, but stop at /auth (--max-redirs 1) so we can
    # capture the auth URL with the state. Then call /callback
    # ourselves with that state, twice.
    local start_headers="${LOG_PREFIX}${TIMESTAMP}_callback_replay_start.headers"
    curl -s -i -o "${start_headers}" --max-time 5 \
        "${base_url}/api/auth/oidc/start" >/dev/null 2>&1 || true

    local auth_url
    auth_url=$("${GREP}" -i '^location:' "${start_headers}" 2>/dev/null \
        | sed 's/^[Ll]ocation:[[:space:]]*//' \
        | tr -d '\r' \
        | head -n 1)

    local state
    state=$(printf '%s' "${auth_url}" \
        | sed -n 's/.*[?&]state=\([^&]*\).*/\1/p')

    if [[ -z "${state}" ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 \
            "Could not extract state from /start redirect"
        EXIT_CODE=1
        return
    fi

    # First callback consumes the state (we don't care about the
    # outcome — happy or sad — only that the state is GONE after).
    curl -s -o /dev/null --max-time 5 \
        "${base_url}/api/auth/oidc/callback?code=test-code-ok&state=${state}" \
        >/dev/null 2>&1 || true

    # Second callback with the SAME state must redirect with
    # state_invalid (state was atomically consumed by the first).
    local http_status
    http_status=$(curl -s -i -o "${replay_headers}" -w "%{http_code}" \
        --max-time 5 \
        "${base_url}/api/auth/oidc/callback?code=test-code-ok&state=${state}" \
        2>/dev/null || echo "000")

    if [[ "${http_status}" != "302" ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 \
            "Expected 302 on replay, got ${http_status}"
        EXIT_CODE=1
        return
    fi

    local err_param
    err_param=$(extract_oidc_error_param "${replay_headers}")
    if [[ "${err_param}" != "state_invalid" ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 \
            "Expected oidc_error=state_invalid on replay, got '${err_param}'"
        EXIT_CODE=1
        return
    fi

    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 \
        "Consumed state cannot be replayed (state_invalid)"
    PASS_COUNT=$(( PASS_COUNT + 1 ))
}
