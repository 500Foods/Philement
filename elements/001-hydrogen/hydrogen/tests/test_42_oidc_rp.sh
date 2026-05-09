#!/usr/bin/env bash

# Test: OIDC Relying Party — End-to-End Coverage
# Drives the OIDC RP endpoints through every gate the plan defines
# from Phase 6 through Phase 19:
#
#   - Disabled feature gate (Phase 6): all three endpoints respond
#     503 {"error":"oidc_disabled"} when OIDC_RP.Enabled = false.
#   - Method discrimination (Phase 6): wrong methods get 405.
#   - Mock IdP reachability + discovery/JWKS shape (Phase 9).
#   - Mock IdP /token endpoint (Phases 11/12).
#   - /oidc/start redirect to IdP with PKCE+state (Phase 10).
#   - /oidc/handoff exchange via debug-only injector (Phase 13).
#   - /oidc/callback full chain with stub linker (Phase 14).
#   - match_sub_only linker: hit/miss paths (Phase 18).
#   - match_email_only linker: hit/miss/ambiguous paths (Phase 19).
#
# The endpoint helper functions, mock-Keycloak lifecycle, and per-
# phase sub-test functions live in tests/lib/oidc_rp_helpers.sh; this
# script is the orchestrator (server lifecycle + sub-test invocation).

# FUNCTIONS
# (Helper functions live in tests/lib/oidc_rp_helpers.sh)

# CHANGELOG
# 1.8.0 - 2026-05-09 - Phase 19: match_email_only linker hit/miss/ambiguous sub-tests
# 1.7.0 - 2026-05-09 - Phase 18: match_sub_only linker hit/miss sub-tests
# 1.6.0 - 2026-05-09 - Phase 14: /oidc/callback end-to-end with stub linker
# 1.5.0 - 2026-05-09 - Phase 13: real /oidc/handoff exchange + debug-inject sub-tests
# 1.4.0 - 2026-05-09 - Phase 12: mock signs real RS256 JWTs; JWKS has real RSA keys; id_token shape sub-test
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
TEST_VERSION="1.8.0"

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
CONFIG_PATH_FULL="${SCRIPT_DIR}/configs/hydrogen_test_${TEST_NUMBER}_oidc_rp_full.json"
CONFIG_PATH_SUB="${SCRIPT_DIR}/configs/hydrogen_test_${TEST_NUMBER}_oidc_rp_sub.json"
CONFIG_PATH_EMAIL="${SCRIPT_DIR}/configs/hydrogen_test_${TEST_NUMBER}_oidc_rp_email.json"
MOCK_KC_SCRIPT="${SCRIPT_DIR}/lib/mock_keycloak/server.js"

# shellcheck source=tests/lib/oidc_rp_helpers.sh # Phase 13 split for code-size cap
source "$(dirname "${BASH_SOURCE[0]}")/lib/oidc_rp_helpers.sh"
# shellcheck source=tests/lib/oidc_rp_helpers_callback.sh # Phase 14 callback helpers
source "$(dirname "${BASH_SOURCE[0]}")/lib/oidc_rp_helpers_callback.sh"
# shellcheck source=tests/lib/oidc_rp_helpers_link.sh # Phase 18 account-linker helpers
source "$(dirname "${BASH_SOURCE[0]}")/lib/oidc_rp_helpers_link.sh"

# Trap to make sure we do not leak a node process if the test script
# fails between mock start and stop. The functions live in
# lib/oidc_rp_helpers.sh; the trap is registered in this file because
# bash traps belong to the script not the sourced library.
trap 'stop_mock_keycloak || true' EXIT


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
            # Phase 12: signed id_token shape check
            test_mock_keycloak_id_token_header_and_claims
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

                # ---- Phase 13: /oidc/handoff exchange via debug injector ----
                test_oidc_handoff_happy_path "${ENABLED_BASE_URL}"
                test_oidc_handoff_replay_returns_401 "${ENABLED_BASE_URL}"
                test_oidc_handoff_unknown_code_returns_401 "${ENABLED_BASE_URL}"
                test_oidc_handoff_missing_field_returns_401 "${ENABLED_BASE_URL}"
                test_oidc_handoff_method_check_still_works_when_enabled "${ENABLED_BASE_URL}"

                # ---- Phase 14: /oidc/callback failure paths (no DB needed) ----
                test_oidc_callback_missing_params_redirects_state_invalid "${ENABLED_BASE_URL}"
                test_oidc_callback_unknown_state_redirects_state_invalid "${ENABLED_BASE_URL}"
                test_oidc_callback_idp_error_redirects_idp_error "${ENABLED_BASE_URL}"
                test_oidc_callback_method_check_still_works_when_enabled "${ENABLED_BASE_URL}"
                test_oidc_callback_replay_returns_state_invalid "${ENABLED_BASE_URL}"
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

    # ---- Phase 14: full happy-path against SQLite-backed Hydrogen ----
    # The mock Keycloak is still running. Start a third Hydrogen
    # instance with the demo SQLite Acuranzo schema attached so
    # /callback can resolve account_id=1 (adminuser) via the stub
    # linker, mint a real Hydrogen JWT, and put a handoff record.
    # Skipped cleanly when the demo SQLite db, the demo API key env
    # var, or jq are missing.
    if [[ "${EXIT_CODE}" -eq 0 ]] \
        && [[ "${MOCK_KC_STARTED:-0}" -eq 1 ]] \
        && [[ -n "${HYDROGEN_DEMO_API_KEY:-}" ]] \
        && [[ -f "${PROJECT_DIR}/tests/artifacts/database/sqlite/hydrodemo.sqlite" ]] \
        && command -v jq >/dev/null 2>&1; then

        print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Validate full-config (SQLite + OIDC RP)"
        # shellcheck disable=SC2310 # We want to continue even if the test fails
        if validate_config_file "${CONFIG_PATH_FULL}"; then
            FULL_PORT=$(get_webserver_port "${CONFIG_PATH_FULL}")
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Full config will use port: ${FULL_PORT}"
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Full-config validated"
            PASS_COUNT=$(( PASS_COUNT + 1 ))

            print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Start Hydrogen Server (full config, SQLite)"
            FULL_HYDROGEN_PID=""
            FULL_PID_VAR="FULL_HYDROGEN_PID_$$"
            FULL_BASE_URL="http://localhost:${FULL_PORT}"
            # Generous timeout — SQLite queries cache initialization
            # plus stmt cache prewarm is heavier than the no-DB cases.
            # shellcheck disable=SC2310 # We want to continue even if the test fails
            if start_hydrogen_with_pid "${CONFIG_PATH_FULL}" "${SERVER_LOG}" 30 "${HYDROGEN_BIN}" "${FULL_PID_VAR}"; then
                FULL_HYDROGEN_PID=$(eval "echo \$${FULL_PID_VAR}")
                if [[ -n "${FULL_HYDROGEN_PID}" ]] && ps -p "${FULL_HYDROGEN_PID}" > /dev/null 2>&1; then
                    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Server started with PID ${FULL_HYDROGEN_PID}"
                    PASS_COUNT=$(( PASS_COUNT + 1 ))
                else
                    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Full-config server PID missing after start"
                    EXIT_CODE=1
                fi
            else
                print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Failed to start Hydrogen with full config"
                EXIT_CODE=1
            fi

            if [[ "${EXIT_CODE}" -eq 0 ]]; then
                # shellcheck disable=SC2310 # We want to continue even if the test fails
                if ! wait_for_server_ready "${FULL_BASE_URL}"; then
                    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Full-config server failed to become ready"
                    EXIT_CODE=1
                fi
            fi

            if [[ "${EXIT_CODE}" -eq 0 ]]; then
                # The HTTP server becomes ready before the database queue
                # bootstrap finishes loading the QTC. Poll the server log
                # for the "Migration completed in" / "Migration Current"
                # signal that all auth queries (e.g. #008 lookup_account)
                # are ready to dispatch. 30s is plenty for SQLite +
                # bootstrap-query path; YugabyteDB-style slowness does
                # not apply here.
                MIGRATION_NOW=$(date +%s)
                MIGRATION_DEADLINE=$(( MIGRATION_NOW + 30 ))
                while true; do
                    MIGRATION_NOW=$(date +%s)
                    if [[ ${MIGRATION_NOW} -ge ${MIGRATION_DEADLINE} ]]; then
                        break
                    fi
                    # shellcheck disable=SC2310 # We poll on success/timeout — non-zero grep is "not yet ready"
                    if "${GREP}" -q -E "Migration completed in|Migration Current:" \
                        "${SERVER_LOG}" 2>/dev/null; then
                        break
                    fi
                    sleep 0.2
                done
                # Brief settling pause for the QTC to be fully usable
                # by handle_api_request paths (matches test_40's
                # convention).
                sleep 1

                # The actual Phase 14 happy-path black-box test.
                test_oidc_callback_happy_path_end_to_end "${FULL_BASE_URL}"
            fi

            # ---- Shutdown full-config Hydrogen ----
            if [[ -n "${FULL_HYDROGEN_PID}" ]] && ps -p "${FULL_HYDROGEN_PID}" > /dev/null 2>&1; then
                print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Stop Hydrogen Server (full config)"
                # shellcheck disable=SC2310 # We want to continue even if the test fails
                if stop_hydrogen "${FULL_HYDROGEN_PID}" "${SERVER_LOG}" 10 5 "${RESULTS_DIR}"; then
                    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Full-config server stopped cleanly"
                    PASS_COUNT=$(( PASS_COUNT + 1 ))
                else
                    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Full-config server shutdown had issues"
                    EXIT_CODE=1
                fi
                # shellcheck disable=SC2310 # Diagnostic-only; non-zero result must not abort the test
                check_time_wait_sockets "${FULL_PORT}" || true
            fi
        else
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Full-config validation failed"
            EXIT_CODE=1
        fi
    elif [[ "${MOCK_KC_STARTED:-0}" -eq 1 ]]; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" \
            "Skipping Phase 14 happy-path: missing demo SQLite db, HYDROGEN_DEMO_API_KEY, or jq"
    fi

    # ---- Phase 18: match_sub_only linker hit/miss sub-tests ----
    # Spin up a fourth Hydrogen instance with the sub-only config
    # (strategy="match_sub_only"). Uses the same SQLite demo db as Phase 14.
    # Skipped when the prerequisites are absent (same guards as Phase 14).
    if [[ "${EXIT_CODE}" -eq 0 ]] \
        && [[ "${MOCK_KC_STARTED:-0}" -eq 1 ]] \
        && [[ -n "${HYDROGEN_DEMO_API_KEY:-}" ]] \
        && [[ -f "${PROJECT_DIR}/tests/artifacts/database/sqlite/hydrodemo.sqlite" ]] \
        && command -v jq >/dev/null 2>&1 \
        && command -v sqlite3 >/dev/null 2>&1; then

        # Phase 18 seeds required rows (account_oidc_identities table +
        # QueryRefs #080 and #084) directly into the demo DB via sqlite3
        # in seed_oidc_identity, so AutoMigration is not needed. The seed
        # helper uses INSERT OR IGNORE so it is idempotent.
        DEMO_SQLITE="${PROJECT_DIR}/tests/artifacts/database/sqlite/hydrodemo.sqlite"

        # Issuer must match the "Issuer" field in hydrogen_test_42_oidc_rp_sub.json.
        MOCK_ISSUER="http://localhost:${MOCK_KC_PORT}/realms/test"
        # Subject value that the mock Keycloak will embed in the id_token
        # (see mock_keycloak/server.js line ~291: `sub: 'mock-sub-12345'`).
        # We seed this exact value so the match_sub_only linker finds the row.
        MOCK_SUBJECT="mock-sub-12345"

        print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Validate sub-config (match_sub_only)"
        # shellcheck disable=SC2310 # We want to continue even if the test fails
        if validate_config_file "${CONFIG_PATH_SUB}"; then
            SUB_PORT=$(get_webserver_port "${CONFIG_PATH_SUB}")
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Sub config will use port: ${SUB_PORT}"
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Sub-config validated"
            PASS_COUNT=$(( PASS_COUNT + 1 ))

            print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Start Hydrogen Server (sub config, match_sub_only)"
            SUB_HYDROGEN_PID=""
            SUB_PID_VAR="SUB_HYDROGEN_PID_$$"
            SUB_BASE_URL="http://localhost:${SUB_PORT}"
            # Capture the current log end so the migration-wait below
            # only searches log lines added by THIS Hydrogen instance.
            SUB_LOG_OFFSET=$(( $(wc -l < "${SERVER_LOG}" 2>/dev/null || echo 0) + 1 ))
            # shellcheck disable=SC2310 # We want to continue even if the test fails
            if start_hydrogen_with_pid "${CONFIG_PATH_SUB}" "${SERVER_LOG}" 30 "${HYDROGEN_BIN}" "${SUB_PID_VAR}"; then
                SUB_HYDROGEN_PID=$(eval "echo \$${SUB_PID_VAR}")
                if [[ -n "${SUB_HYDROGEN_PID}" ]] && ps -p "${SUB_HYDROGEN_PID}" > /dev/null 2>&1; then
                    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Server started with PID ${SUB_HYDROGEN_PID}"
                    PASS_COUNT=$(( PASS_COUNT + 1 ))
                else
                    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Sub-config server PID missing after start"
                    EXIT_CODE=1
                fi
            else
                print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Failed to start Hydrogen with sub config"
                EXIT_CODE=1
            fi

            if [[ "${EXIT_CODE}" -eq 0 ]]; then
                # shellcheck disable=SC2310 # We want to continue even if the test fails
                if ! wait_for_server_ready "${SUB_BASE_URL}"; then
                    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Sub-config server failed to become ready"
                    EXIT_CODE=1
                fi
            fi

            if [[ "${EXIT_CODE}" -eq 0 ]]; then
                # Wait for QTC bootstrap. AutoMigration is disabled (Phase 18
                # seeds rows directly), so we just wait for "Migration Current:"
                # which Hydrogen logs after the bootstrap query populates the QTC
                # from the existing queries table. Use SUB_LOG_OFFSET to avoid
                # matching migration log lines from earlier Hydrogen instances.
                MIGRATION_NOW=$(date +%s)
                MIGRATION_DEADLINE=$(( MIGRATION_NOW + 30 ))
                while true; do
                    MIGRATION_NOW=$(date +%s)
                    if [[ ${MIGRATION_NOW} -ge ${MIGRATION_DEADLINE} ]]; then
                        break
                    fi
                    # shellcheck disable=SC2310 # Polling on success/timeout — non-zero grep is "not yet ready"
                    if tail -n +"${SUB_LOG_OFFSET}" "${SERVER_LOG}" 2>/dev/null \
                        | "${GREP}" -q -E "Migration completed in|Migration Current:"; then
                        break
                    fi
                    sleep 0.2
                done
                sleep 1

                # Phase 18 happy path: pre-seed identity, drive chain, get JWT.
                # shellcheck disable=SC2310 # We want to continue even if the test fails
                test_oidc_link_match_sub_only_hit \
                    "${SUB_BASE_URL}" "${DEMO_SQLITE}" "${MOCK_ISSUER}" "${MOCK_SUBJECT}"

                # Phase 18 miss path: no identity row → no_account redirect.
                # shellcheck disable=SC2310 # We want to continue even if the test fails
                test_oidc_link_match_sub_only_miss \
                    "${SUB_BASE_URL}" "${DEMO_SQLITE}" "${MOCK_ISSUER}" "${MOCK_SUBJECT}"
            fi

            # Shutdown sub-config Hydrogen.
            if [[ -n "${SUB_HYDROGEN_PID:-}" ]] && ps -p "${SUB_HYDROGEN_PID}" > /dev/null 2>&1; then
                print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Stop Hydrogen Server (sub config)"
                # shellcheck disable=SC2310 # We want to continue even if the test fails
                if stop_hydrogen "${SUB_HYDROGEN_PID}" "${SERVER_LOG}" 10 5 "${RESULTS_DIR}"; then
                    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Sub-config server stopped cleanly"
                    PASS_COUNT=$(( PASS_COUNT + 1 ))
                else
                    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Sub-config server shutdown had issues"
                    EXIT_CODE=1
                fi
                # shellcheck disable=SC2310 # Diagnostic-only; non-zero result must not abort the test
                check_time_wait_sockets "${SUB_PORT}" || true
            fi
            # No DB copy to clean up — Phase 18 uses the shared demo DB
            # with idempotent INSERT OR IGNORE for the seeded rows.
        else
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Sub-config validation failed"
            EXIT_CODE=1
        fi
    elif [[ "${MOCK_KC_STARTED:-0}" -eq 1 ]]; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" \
            "Skipping Phase 18 sub-tests: missing demo SQLite db, HYDROGEN_DEMO_API_KEY, sqlite3, or jq"
    fi

    # ---- Phase 19: match_email_only linker hit/miss/ambiguous sub-tests ----
    # Spin up a fifth Hydrogen instance with the email-only config
    # (strategy="match_email_only"). Uses the same SQLite demo db.
    # Skipped when the prerequisites are absent (same guards as Phase 18).
    if [[ "${EXIT_CODE}" -eq 0 ]] \
        && [[ "${MOCK_KC_STARTED:-0}" -eq 1 ]] \
        && [[ -n "${HYDROGEN_DEMO_API_KEY:-}" ]] \
        && [[ -f "${PROJECT_DIR}/tests/artifacts/database/sqlite/hydrodemo.sqlite" ]] \
        && command -v jq >/dev/null 2>&1 \
        && command -v sqlite3 >/dev/null 2>&1; then

        DEMO_SQLITE="${PROJECT_DIR}/tests/artifacts/database/sqlite/hydrodemo.sqlite"
        MOCK_ISSUER="http://localhost:${MOCK_KC_PORT}/realms/test"
        # Subject emitted by the mock IdP (see mock_keycloak/server.js).
        MOCK_SUBJECT="mock-sub-12345"
        # Email emitted by the mock IdP.
        MOCK_EMAIL="mockuser@example.com"

        print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Validate email-config (match_email_only)"
        # shellcheck disable=SC2310 # We want to continue even if the test fails
        if validate_config_file "${CONFIG_PATH_EMAIL}"; then
            EMAIL_PORT=$(get_webserver_port "${CONFIG_PATH_EMAIL}")
            print_message "${TEST_NUMBER}" "${TEST_COUNTER}" "Email config will use port: ${EMAIL_PORT}"
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Email-config validated"
            PASS_COUNT=$(( PASS_COUNT + 1 ))

            print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Start Hydrogen Server (email config, match_email_only)"
            EMAIL_HYDROGEN_PID=""
            EMAIL_PID_VAR="EMAIL_HYDROGEN_PID_$$"
            EMAIL_BASE_URL="http://localhost:${EMAIL_PORT}"
            EMAIL_LOG_OFFSET=$(( $(wc -l < "${SERVER_LOG}" 2>/dev/null || echo 0) + 1 ))
            # shellcheck disable=SC2310 # We want to continue even if the test fails
            if start_hydrogen_with_pid "${CONFIG_PATH_EMAIL}" "${SERVER_LOG}" 30 "${HYDROGEN_BIN}" "${EMAIL_PID_VAR}"; then
                EMAIL_HYDROGEN_PID=$(eval "echo \$${EMAIL_PID_VAR}")
                if [[ -n "${EMAIL_HYDROGEN_PID}" ]] && ps -p "${EMAIL_HYDROGEN_PID}" > /dev/null 2>&1; then
                    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Server started with PID ${EMAIL_HYDROGEN_PID}"
                    PASS_COUNT=$(( PASS_COUNT + 1 ))
                else
                    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Email-config server PID missing after start"
                    EXIT_CODE=1
                fi
            else
                print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Failed to start Hydrogen with email config"
                EXIT_CODE=1
            fi

            if [[ "${EXIT_CODE}" -eq 0 ]]; then
                # shellcheck disable=SC2310 # We want to continue even if the test fails
                if ! wait_for_server_ready "${EMAIL_BASE_URL}"; then
                    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Email-config server failed to become ready"
                    EXIT_CODE=1
                fi
            fi

            if [[ "${EXIT_CODE}" -eq 0 ]]; then
                # Wait for QTC bootstrap (same pattern as Phase 18).
                EMAIL_NOW=$(date +%s)
                EMAIL_DEADLINE=$(( EMAIL_NOW + 30 ))
                while true; do
                    EMAIL_NOW=$(date +%s)
                    if [[ ${EMAIL_NOW} -ge ${EMAIL_DEADLINE} ]]; then
                        break
                    fi
                    # shellcheck disable=SC2310 # Polling on success/timeout
                    if tail -n +"${EMAIL_LOG_OFFSET}" "${SERVER_LOG}" 2>/dev/null \
                        | "${GREP}" -q -E "Migration completed in|Migration Current:"; then
                        break
                    fi
                    sleep 0.2
                done
                sleep 1

                # Phase 19 hit path: seed email, drive chain, get JWT.
                # shellcheck disable=SC2310 # We want to continue even if the test fails
                test_oidc_link_match_email_only_hit \
                    "${EMAIL_BASE_URL}" "${DEMO_SQLITE}" \
                    "${MOCK_ISSUER}" "${MOCK_SUBJECT}" "${MOCK_EMAIL}"

                # Phase 19 miss path: no email contact → no_account.
                # shellcheck disable=SC2310 # We want to continue even if the test fails
                test_oidc_link_match_email_only_miss \
                    "${EMAIL_BASE_URL}" "${DEMO_SQLITE}" \
                    "${MOCK_ISSUER}" "${MOCK_SUBJECT}" "${MOCK_EMAIL}"

                # Phase 19 ambiguous path: email on two accounts → email_ambiguous.
                # shellcheck disable=SC2310 # We want to continue even if the test fails
                test_oidc_link_match_email_only_ambiguous \
                    "${EMAIL_BASE_URL}" "${DEMO_SQLITE}" \
                    "${MOCK_ISSUER}" "${MOCK_SUBJECT}" "${MOCK_EMAIL}"
            fi

            # Shutdown email-config Hydrogen.
            if [[ -n "${EMAIL_HYDROGEN_PID:-}" ]] && ps -p "${EMAIL_HYDROGEN_PID}" > /dev/null 2>&1; then
                print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Stop Hydrogen Server (email config)"
                # shellcheck disable=SC2310 # We want to continue even if the test fails
                if stop_hydrogen "${EMAIL_HYDROGEN_PID}" "${SERVER_LOG}" 10 5 "${RESULTS_DIR}"; then
                    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Email-config server stopped cleanly"
                    PASS_COUNT=$(( PASS_COUNT + 1 ))
                else
                    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Email-config server shutdown had issues"
                    EXIT_CODE=1
                fi
                # shellcheck disable=SC2310 # Diagnostic-only; non-zero result must not abort the test
                check_time_wait_sockets "${EMAIL_PORT}" || true
            fi
        else
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Email-config validation failed"
            EXIT_CODE=1
        fi
    elif [[ "${MOCK_KC_STARTED:-0}" -eq 1 ]]; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" \
            "Skipping Phase 19 sub-tests: missing demo SQLite db, HYDROGEN_DEMO_API_KEY, sqlite3, or jq"
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
