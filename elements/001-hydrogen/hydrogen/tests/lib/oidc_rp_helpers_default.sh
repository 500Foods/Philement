#!/usr/bin/env bash

# shellcheck disable=SC2154 # All globals (TEST_NUMBER, TEST_COUNTER, GREP, LOG_PREFIX, TIMESTAMP, etc.) are set by the test script via framework.sh before sourcing this lib
# shellcheck disable=SC2034 # EXIT_CODE is set by the test script and re-exported through these helpers
# shellcheck disable=SC2312 # Several diagnostic command substitutions intentionally swallow the inner exit code

# OIDC RP Account Linker — Phase 21 (match_email_then_provision) Sub-Tests
#
# Provides three black-box sub-tests against a single Hydrogen instance
# configured with Strategy="match_email_then_provision":
#
#   test_oidc_link_default_sub_hit:
#     Pre-seed an identity row. Drive /start→/auth→/callback. Expect JWT
#     envelope with the seeded account_id (sub-match fast-path used).
#
#   test_oidc_link_default_email_link:
#     No identity row, but a matching email contact. Drive chain. Expect JWT
#     with the account whose email matches. The identity row is created in
#     account_oidc_identities as a side-effect; we clean it up afterwards.
#
#   test_oidc_link_default_provision:
#     No identity row, no email contact. Domain matches AllowedEmailDomains.
#     Drive chain. Expect JWT envelope with a newly provisioned account_id
#     (max+1). Clean up the new account row and identity row.
#
# All four sub-paths of the default strategy are covered by the three
# sub-tests above (sub-fast-path, email-link, provision) plus the
# email-ambiguous path which is tested by the Phase 19 helper as a
# cross-cutting concern (the same #082 ambiguous path applies).
#
# Depends on helpers from oidc_rp_helpers_link.sh and
# oidc_rp_helpers_provision.sh:
#   - _drive_oidc_chain
#   - seed_oidc_identity, unseed_oidc_identity
#   - seed_email_queryrefs, seed_oidc_queryref_seed_only
#   - seed_provision_queryrefs
#   - get_max_account_id, delete_account
#   - seed_email_contact, unseed_email_contact
#
# Phase 21 was split into its own file (separately from
# oidc_rp_helpers_provision.sh) because each lib file must stay under the
# project-wide 1,000-line cap (LITHIUM-INS.md rule equivalent for Hydrogen).
#
# CHANGELOG
# 1.0.0 - 2026-05-09 - Initial creation, Phase 21 default-strategy sub-tests.

# ---------------------------------------------------------------------------
# Phase 21 sub-tests
# ---------------------------------------------------------------------------

# Sub-test: match_email_then_provision — sub fast-path.
# Pre-seed an identity row for the mock IdP's sub claim. The linker should
# resolve via #080 without touching #082 or #083.
#
# Args:
#   $1  base_url    Hydrogen base URL (default-config instance)
#   $2  sqlite_db   path to the SQLite database
#   $3  issuer      OIDC issuer string matching the config
#   $4  subject     the mock IdP's sub claim
#   $5  account_id  existing account_id to seed the identity row against
test_oidc_link_default_sub_hit() {
    local base_url="$1"
    local sqlite_db="$2"
    local issuer="$3"
    local subject="$4"
    local account_id="$5"

    local headers_file="${LOG_PREFIX}${TIMESTAMP}_default_sub_hit_headers.txt"
    local exchange_file="${LOG_PREFIX}${TIMESTAMP}_default_sub_hit_exchange.json"
    OIDC_CHAIN_HANDOFF=""
    OIDC_CHAIN_ERROR=""
    OIDC_CHAIN_STATUS=""

    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" \
        "Phase 21: default strategy sub fast-path -> JWT with seeded account_id"

    if ! command -v jq >/dev/null 2>&1; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" \
            "jq missing — Phase 21 sub-test requires it"
        return
    fi

    if ! command -v sqlite3 >/dev/null 2>&1; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" \
            "sqlite3 missing — skipping Phase 21 sub-test"
        return
    fi

    # Ensure seeds are present.
    seed_oidc_queryref_seed_only "${sqlite_db}" || true
    seed_email_queryrefs "${sqlite_db}" || true
    seed_provision_queryrefs "${sqlite_db}" || true

    # Seed the identity row so the sub fast-path fires.
    unseed_oidc_identity "${sqlite_db}" "${issuer}" "${subject}" || true
    seed_oidc_identity "${sqlite_db}" "${account_id}" "${issuer}" "${subject}" || {
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 \
            "Failed to seed identity row"
        EXIT_CODE=1
        return
    }

    _drive_oidc_chain "${base_url}" "${headers_file}"

    # Cleanup always runs.
    unseed_oidc_identity "${sqlite_db}" "${issuer}" "${subject}" || true

    if [[ -z "${OIDC_CHAIN_HANDOFF}" ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 \
            "No handoff code (oidc_error=${OIDC_CHAIN_ERROR:-none}, status=${OIDC_CHAIN_STATUS})"
        EXIT_CODE=1
        return
    fi

    local body exchange_status
    body=$(printf '{"handoff":"%s"}' "${OIDC_CHAIN_HANDOFF}")
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

    local user_id success token
    user_id=$(jq -r '.user_id // empty' "${exchange_file}" 2>/dev/null || echo "")
    success=$(jq -r '.success // empty' "${exchange_file}" 2>/dev/null || echo "")
    token=$(jq -r   '.token   // empty' "${exchange_file}" 2>/dev/null || echo "")

    if [[ "${success}" != "true" ]] \
        || [[ -z "${token}" ]] \
        || [[ "${token}" != *"."*"."* ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 \
            "Envelope malformed: success=${success} token=${token:0:32}..."
        EXIT_CODE=1
        return
    fi

    if [[ "${user_id}" != "${account_id}" ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 \
            "Expected user_id=${account_id} (seeded), got user_id=${user_id}"
        EXIT_CODE=1
        return
    fi

    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 \
        "default sub fast-path: JWT envelope user_id=${user_id} (sub-match used)"
    PASS_COUNT=$(( PASS_COUNT + 1 ))
}

# Sub-test: match_email_then_provision — provision path.
# No identity row exists, no email contact exists. The mock IdP emits
# mockuser@example.com and AllowedEmailDomains=["example.com"]. The linker
# should provision a new accounts row.
#
# Args:
#   $1  base_url    Hydrogen base URL (default-config instance)
#   $2  sqlite_db   path to the SQLite database
#   $3  issuer      OIDC issuer string matching the config
#   $4  subject     the mock IdP's sub claim (used for cleanup)
#   $5  email       the email address the mock IdP emits
test_oidc_link_default_provision() {
    local base_url="$1"
    local sqlite_db="$2"
    local issuer="$3"
    local subject="$4"
    local email="$5"

    local headers_file="${LOG_PREFIX}${TIMESTAMP}_default_provision_headers.txt"
    local exchange_file="${LOG_PREFIX}${TIMESTAMP}_default_provision_exchange.json"
    OIDC_CHAIN_HANDOFF=""
    OIDC_CHAIN_ERROR=""
    OIDC_CHAIN_STATUS=""

    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" \
        "Phase 21: default strategy provision path -> JWT with new account_id"

    if ! command -v jq >/dev/null 2>&1; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" \
            "jq missing — Phase 21 provision sub-test requires it"
        return
    fi

    if ! command -v sqlite3 >/dev/null 2>&1; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" \
            "sqlite3 missing — skipping Phase 21 provision sub-test"
        return
    fi

    # Clean state.
    unseed_oidc_identity "${sqlite_db}" "${issuer}" "${subject}" || true

    # Seed QueryRefs.
    seed_oidc_queryref_seed_only "${sqlite_db}" || true
    seed_email_queryrefs "${sqlite_db}" || true
    seed_provision_queryrefs "${sqlite_db}" || true

    # Capture pre-provision max.
    local before_max
    before_max=$(get_max_account_id "${sqlite_db}")

    _drive_oidc_chain "${base_url}" "${headers_file}"

    local after_max
    after_max=$(get_max_account_id "${sqlite_db}")

    # Cleanup always runs.
    if [[ "${after_max}" -gt "${before_max}" ]]; then
        delete_account "${sqlite_db}" "${after_max}" || true
    fi
    unseed_oidc_identity "${sqlite_db}" "${issuer}" "${subject}" || true

    if [[ -z "${OIDC_CHAIN_HANDOFF}" ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 \
            "No handoff code (oidc_error=${OIDC_CHAIN_ERROR:-none}, status=${OIDC_CHAIN_STATUS})"
        EXIT_CODE=1
        return
    fi

    local body exchange_status
    body=$(printf '{"handoff":"%s"}' "${OIDC_CHAIN_HANDOFF}")
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

    local user_id success token
    user_id=$(jq -r '.user_id // empty' "${exchange_file}" 2>/dev/null || echo "")
    success=$(jq -r '.success // empty' "${exchange_file}" 2>/dev/null || echo "")
    token=$(jq -r   '.token   // empty' "${exchange_file}" 2>/dev/null || echo "")

    if [[ "${success}" != "true" ]] \
        || [[ -z "${token}" ]] \
        || [[ "${token}" != *"."*"."* ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 \
            "Envelope malformed: success=${success} token=${token:0:32}..."
        EXIT_CODE=1
        return
    fi

    local expected_id=$(( before_max + 1 ))
    if [[ "${user_id}" != "${expected_id}" ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 \
            "Expected user_id=${expected_id} (provisioned max+1), got user_id=${user_id}"
        EXIT_CODE=1
        return
    fi

    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 \
        "default provision path: JWT envelope user_id=${user_id} (new account provisioned, email=${email#*@})"
    PASS_COUNT=$(( PASS_COUNT + 1 ))
}

# ---------------------------------------------------------------------------
# Phase 21 lifecycle block
# ---------------------------------------------------------------------------
# Handles the full Hydrogen instance lifecycle for the default-strategy tests:
# validate config, seed QueryRefs, start Hydrogen, wait for QTC, run sub-tests,
# stop Hydrogen. Called from test_42_oidc_rp.sh to keep the orchestrator under
# the 1,000-line cap.
#
# Args:
#   $1  config_path         Path to the default-strategy config file
#   $2  sqlite_db           Path to the demo SQLite database
#   $3  mock_issuer         OIDC issuer URL of the mock Keycloak
#   $4  mock_subject        sub claim value the mock IdP emits
#   $5  mock_email          email the mock IdP emits
#   $6  hydrogen_bin        Path to the Hydrogen binary
#   $7  server_log          Path to the shared server log file
#   $8  results_dir         Path to the results directory
run_phase21_default_tests() {
    local config_path="$1"
    local sqlite_db="$2"
    local mock_issuer="$3"
    local mock_subject="$4"
    local mock_email="$5"
    local hydrogen_bin="$6"
    local server_log="$7"
    local results_dir="$8"

    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Validate default-config (match_email_then_provision)"
    # shellcheck disable=SC2310 # We want to continue even if the test fails
    if ! validate_config_file "${config_path}"; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Default-config validation failed"
        EXIT_CODE=1
        return
    fi

    local default_port
    default_port=$(get_webserver_port "${config_path}")
    print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Default config will use port: ${default_port}"
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Default-config validated"
    PASS_COUNT=$(( PASS_COUNT + 1 ))

    # Seed QueryRefs BEFORE Hydrogen starts (idempotent).
    seed_oidc_queryref_seed_only "${sqlite_db}" || true
    seed_email_queryrefs "${sqlite_db}" || true
    seed_provision_queryrefs "${sqlite_db}" || true

    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Start Hydrogen Server (default config)"
    local default_hydrogen_pid=""
    local default_pid_var="DEFAULT_HYDROGEN_PID_$$"
    local default_base_url="http://localhost:${default_port}"
    local default_log_offset
    default_log_offset=$(( $(wc -l < "${server_log}" 2>/dev/null || echo 0) + 1 ))

    # shellcheck disable=SC2310 # We want to continue even if the test fails
    if start_hydrogen_with_pid "${config_path}" "${server_log}" 30 "${hydrogen_bin}" "${default_pid_var}"; then
        default_hydrogen_pid=$(eval "echo \$${default_pid_var}")
        if [[ -n "${default_hydrogen_pid}" ]] && ps -p "${default_hydrogen_pid}" > /dev/null 2>&1; then
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Server started with PID ${default_hydrogen_pid}"
            PASS_COUNT=$(( PASS_COUNT + 1 ))
        else
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Default-config server PID missing after start"
            EXIT_CODE=1
        fi
    else
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Failed to start Hydrogen with default config"
        EXIT_CODE=1
    fi

    if [[ "${EXIT_CODE}" -eq 0 ]]; then
        # shellcheck disable=SC2310 # We want to continue even if the test fails
        if ! wait_for_server_ready "${default_base_url}"; then
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Default-config server failed to become ready"
            EXIT_CODE=1
        fi
    fi

    if [[ "${EXIT_CODE}" -eq 0 ]]; then
        local default_now default_deadline
        default_now=$(date +%s)
        default_deadline=$(( default_now + 30 ))
        while true; do
            default_now=$(date +%s)
            if [[ ${default_now} -ge ${default_deadline} ]]; then
                break
            fi
            # shellcheck disable=SC2310 # Polling on success/timeout
            if tail -n +"${default_log_offset}" "${server_log}" 2>/dev/null \
                | "${GREP}" -q -E "Migration completed in|Migration Current:"; then
                break
            fi
            sleep 0.2
        done
        sleep 1

        # Phase 21 sub-path (a): sub fast-path.
        # shellcheck disable=SC2310 # We want to continue even if the test fails
        test_oidc_link_default_sub_hit \
            "${default_base_url}" "${sqlite_db}" \
            "${mock_issuer}" "${mock_subject}" 1

        # Phase 21 sub-path (b): provision path.
        # shellcheck disable=SC2310 # We want to continue even if the test fails
        test_oidc_link_default_provision \
            "${default_base_url}" "${sqlite_db}" \
            "${mock_issuer}" "${mock_subject}" "${mock_email}"
    fi

    # Shutdown default-config Hydrogen.
    if [[ -n "${default_hydrogen_pid:-}" ]] && ps -p "${default_hydrogen_pid}" > /dev/null 2>&1; then
        print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Stop Hydrogen Server (default config)"
        # shellcheck disable=SC2310 # We want to continue even if the test fails
        if stop_hydrogen "${default_hydrogen_pid}" "${server_log}" 10 5 "${results_dir}"; then
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Default-config server stopped cleanly"
            PASS_COUNT=$(( PASS_COUNT + 1 ))
        else
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Default-config server shutdown had issues"
            EXIT_CODE=1
        fi
        # shellcheck disable=SC2310 # Diagnostic-only; non-zero result must not abort the test
        check_time_wait_sockets "${default_port}" || true
    fi
}
