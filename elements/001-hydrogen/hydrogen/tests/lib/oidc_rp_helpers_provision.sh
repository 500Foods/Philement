#!/usr/bin/env bash

# shellcheck disable=SC2154 # All globals (TEST_NUMBER, TEST_COUNTER, GREP, LOG_PREFIX, TIMESTAMP, etc.) are set by the test script via framework.sh before sourcing this lib
# shellcheck disable=SC2034 # EXIT_CODE is set by the test script and re-exported through these helpers
# shellcheck disable=SC2312 # Several diagnostic command substitutions intentionally swallow the inner exit code

# OIDC RP Account Linker — Phase 20 (provision_only) Sub-Tests
#
# Provides the black-box sub-tests for the provision_only strategy:
#   - test_oidc_link_provision_only_happy:   provisions an account → JWT
#   - test_oidc_link_provision_only_blocked: domain rejected → provision_disallowed_email
#
# Depends on helpers from oidc_rp_helpers_link.sh:
#   - _drive_oidc_chain (drives /start -> /auth -> /callback)
#   - seed_oidc_queryref_seed_only (Phase 18 QueryRefs + table)
#   - seed_email_queryrefs (#081 + #082)
#   - seed_provision_queryrefs (#083 — Phase 20)
#   - get_max_account_id, delete_account, unseed_oidc_identity
#
# Phase 20 was split into its own file (separately from oidc_rp_helpers_link.sh)
# because the combined file was approaching the project-wide 1,000-line cap
# (LITHIUM-INS.md rule equivalent for Hydrogen). This split keeps every
# sourced library file comfortably under the cap.
#
# CHANGELOG
# 1.0.0 - 2026-05-09 - Initial creation, Phase 20 provision_only sub-tests.

# ---------------------------------------------------------------------------
# Phase 20 sub-tests
# ---------------------------------------------------------------------------

# Sub-test: provision_only with the email domain on the allow-list.
# The mock IdP emits email='mockuser@example.com' and the config sets
# AllowedEmailDomains=["example.com"]. The linker should:
#   1. Fail #080 (no prior identity link).
#   2. Validate prerequisites (Enabled, email present, domain allowed).
#   3. Provision a new accounts row via #083.
#   4. Link the new account via #081.
#   5. Touch via #084.
#   6. Mint a JWT and return the handoff.
#
# The test captures the pre-provision MAX(account_id), drives the chain,
# verifies the JWT envelope, and asserts that exactly one new account row
# was created. Cleanup deletes the provisioned account so subsequent runs
# start from a clean state.
#
# Args:
#   $1  base_url    Hydrogen base URL (the provision-config instance)
#   $2  sqlite_db   path to the SQLite database
#   $3  issuer      OIDC issuer string matching the config
#   $4  subject     the mock IdP's sub claim (used for cleanup)
#   $5  email       the email address the mock IdP emits
test_oidc_link_provision_only_happy() {
    local base_url="$1"
    local sqlite_db="$2"
    local issuer="$3"
    local subject="$4"
    local email="$5"

    local headers_file="${LOG_PREFIX}${TIMESTAMP}_link_provision_happy_headers.txt"
    local exchange_file="${LOG_PREFIX}${TIMESTAMP}_link_provision_happy_exchange.json"
    OIDC_CHAIN_HANDOFF=""
    OIDC_CHAIN_ERROR=""
    OIDC_CHAIN_STATUS=""

    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" \
        "Phase 20: provision_only with allowed-domain email -> JWT envelope (account provisioned)"

    if ! command -v jq >/dev/null 2>&1; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" \
            "jq missing — Phase 20 sub-test requires it"
        return
    fi

    if ! command -v sqlite3 >/dev/null 2>&1; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" \
            "sqlite3 missing — skipping Phase 20 sub-test"
        return
    fi

    # Clean state: ensure no prior identity link exists for this (iss, sub).
    unseed_oidc_identity "${sqlite_db}" "${issuer}" "${subject}" || true

    # Seed all three QueryRefs the provision flow needs (#080, #084 from
    # Phase 18; #081 from Phase 19; #083 new in Phase 20). Idempotent.
    seed_oidc_queryref_seed_only "${sqlite_db}" || true
    seed_email_queryrefs "${sqlite_db}" || true
    seed_provision_queryrefs "${sqlite_db}" || true

    # Capture pre-provision MAX(account_id). The new account will be
    # max+1.
    local before_max
    before_max=$(get_max_account_id "${sqlite_db}")

    # Drive the OIDC chain.
    _drive_oidc_chain "${base_url}" "${headers_file}"

    if [[ -z "${OIDC_CHAIN_HANDOFF}" ]]; then
        # On failure, surface the error and try to find any provisioned row
        # so cleanup can remove it.
        local post_max
        post_max=$(get_max_account_id "${sqlite_db}")
        if [[ "${post_max}" -gt "${before_max}" ]]; then
            delete_account "${sqlite_db}" "${post_max}" || true
        fi
        unseed_oidc_identity "${sqlite_db}" "${issuer}" "${subject}" || true
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 \
            "No handoff code (oidc_error=${OIDC_CHAIN_ERROR:-none}, status=${OIDC_CHAIN_STATUS})"
        EXIT_CODE=1
        return
    fi

    # Exchange the handoff.
    local body exchange_status
    body=$(printf '{"handoff":"%s"}' "${OIDC_CHAIN_HANDOFF}")
    exchange_status=$(curl -s -X POST -H "Content-Type: application/json" \
        -d "${body}" -w "%{http_code}" -o "${exchange_file}" \
        --max-time 5 "${base_url}/api/auth/oidc/handoff" \
        2>/dev/null || echo "000")

    # Determine the new account_id (max+1) for cleanup.
    local after_max
    after_max=$(get_max_account_id "${sqlite_db}")

    # Cleanup — always run, even if assertions below fail.
    if [[ "${after_max}" -gt "${before_max}" ]]; then
        delete_account "${sqlite_db}" "${after_max}" || true
    fi
    unseed_oidc_identity "${sqlite_db}" "${issuer}" "${subject}" || true

    if [[ "${exchange_status}" != "200" ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 \
            "/handoff exchange returned ${exchange_status} (expected 200)"
        EXIT_CODE=1
        return
    fi

    local token user_id success
    token=$(jq -r   '.token   // empty' "${exchange_file}" 2>/dev/null || echo "")
    user_id=$(jq -r '.user_id // empty' "${exchange_file}" 2>/dev/null || echo "")
    success=$(jq -r '.success // empty' "${exchange_file}" 2>/dev/null || echo "")

    if [[ "${success}" != "true" ]] \
        || [[ -z "${token}" ]] \
        || [[ "${token}" != *"."*"."* ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 \
            "Envelope malformed: success=${success} token=${token:0:32}..."
        EXIT_CODE=1
        return
    fi

    # The provisioned account_id should be exactly one greater than the
    # pre-flight max.
    local expected_id=$(( before_max + 1 ))
    if [[ "${user_id}" != "${expected_id}" ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 \
            "Expected user_id=${expected_id} (max+1), got user_id=${user_id} (before=${before_max} after=${after_max})"
        EXIT_CODE=1
        return
    fi

    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 \
        "provision_only happy: JWT envelope user_id=${user_id} (account provisioned, email=${email#*@})"
    PASS_COUNT=$(( PASS_COUNT + 1 ))
}

# Sub-test: provision_only with the email domain NOT on the allow-list.
# This sub-test runs against a config whose AllowedEmailDomains=["philement.com"]
# but the mock IdP emits 'mockuser@example.com' — domain mismatch.
#
# Expected: 302 redirect with ?oidc_error=provision_disallowed_email.
# The accounts table must NOT grow.
#
# Args:
#   $1  base_url    Hydrogen base URL (the provision-blocked-config instance)
#   $2  sqlite_db   path to the SQLite database
#   $3  issuer      OIDC issuer string matching the config
#   $4  subject     the mock IdP's sub claim (used for cleanup)
test_oidc_link_provision_only_blocked() {
    local base_url="$1"
    local sqlite_db="$2"
    local issuer="$3"
    local subject="$4"

    local headers_file="${LOG_PREFIX}${TIMESTAMP}_link_provision_blocked_headers.txt"
    OIDC_CHAIN_HANDOFF=""
    OIDC_CHAIN_ERROR=""
    OIDC_CHAIN_STATUS=""

    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" \
        "Phase 20: provision_only with disallowed-domain email -> 302 oidc_error=provision_disallowed_email"

    if ! command -v sqlite3 >/dev/null 2>&1; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" \
            "sqlite3 missing — skipping Phase 20 blocked sub-test"
        return
    fi

    # Ensure no prior identity link.
    unseed_oidc_identity "${sqlite_db}" "${issuer}" "${subject}" || true

    # Seed QueryRefs (idempotent). #083 must exist even though the linker
    # will refuse to call it — Hydrogen validates the queries table at
    # bootstrap regardless of whether we expect a domain mismatch.
    seed_oidc_queryref_seed_only "${sqlite_db}" || true
    seed_email_queryrefs "${sqlite_db}" || true
    seed_provision_queryrefs "${sqlite_db}" || true

    # Capture pre-flight max so we can verify NO account was provisioned.
    local before_max
    before_max=$(get_max_account_id "${sqlite_db}")

    # Drive the chain.
    _drive_oidc_chain "${base_url}" "${headers_file}"

    # Verify accounts table did not grow.
    local after_max
    after_max=$(get_max_account_id "${sqlite_db}")
    if [[ "${after_max}" -ne "${before_max}" ]]; then
        # Defensive cleanup just in case.
        delete_account "${sqlite_db}" "${after_max}" || true
        unseed_oidc_identity "${sqlite_db}" "${issuer}" "${subject}" || true
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 \
            "Account row was provisioned despite domain mismatch (before=${before_max} after=${after_max})"
        EXIT_CODE=1
        return
    fi

    if [[ "${OIDC_CHAIN_ERROR}" != "provision_disallowed_email" ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 \
            "Expected oidc_error=provision_disallowed_email, got '${OIDC_CHAIN_ERROR:-none}' (handoff=${OIDC_CHAIN_HANDOFF:-none})"
        EXIT_CODE=1
        return
    fi

    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 \
        "provision_only blocked: provision_disallowed_email as expected (no account row created)"
    PASS_COUNT=$(( PASS_COUNT + 1 ))
}

# Internal helper: ensure the Phase 18 QueryRefs (#080, #084) and the
# account_oidc_identities table exist. Phase 20 doesn't seed an identity
# row, but the linker still needs the table + #080 to perform the fast-path
# lookup, and #084 to touch after a re-sign-in.
#
# Implemented by piggybacking on seed_oidc_identity (which always creates
# both the table and the QueryRefs), then immediately deleting any seeded
# row. This avoids duplicating the schema/QueryRef seed SQL across
# Phase 18, 19, and 20.
seed_oidc_queryref_seed_only() {
    local sqlite_db="$1"

    # Use a sentinel pair that won't collide with real test values.
    seed_oidc_identity "${sqlite_db}" 1 \
        "__phase20_queryref_seed__" "__phase20_queryref_seed__" || return 1

    # Remove the sentinel row immediately — we only wanted the side
    # effects (table + QueryRefs).
    sqlite3 2> /dev/null "${sqlite_db}" \
        "DELETE FROM account_oidc_identities WHERE issuer='__phase20_queryref_seed__' AND subject='__phase20_queryref_seed__';"
}
