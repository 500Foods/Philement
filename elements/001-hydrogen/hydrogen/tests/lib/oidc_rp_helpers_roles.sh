#!/usr/bin/env bash

# shellcheck disable=SC2154 # All globals (TEST_NUMBER, TEST_COUNTER, GREP, LOG_PREFIX, TIMESTAMP, etc.) are set by the test script via framework.sh before sourcing this lib
# shellcheck disable=SC2034 # EXIT_CODE is set by the test script and re-exported through these helpers
# shellcheck disable=SC2312 # Several diagnostic command substitutions intentionally swallow the inner exit code

# OIDC RP Role Mapping — Phase 22 Sub-Tests
#
# Provides three black-box sub-tests that verify the four
# RoleMapping.Source modes (database, idp_realm_roles, idp_client_roles,
# merge) by driving the full /start → /auth → /callback → /handoff chain.
#
# Sub-tests:
#
#   test_oidc_roles_database_source:
#     Config: RoleMapping.Source = "database" (port 5249 — default config).
#     Pre-seed an identity row for account_id=1. Drive the chain.
#     Verify the handoff envelope is 200 with `success=true`.
#     The `roles` field in the handoff response is "" (account_roles is
#     empty for account_id=1 in the demo DB). This confirms the DATABASE
#     path runs QueryRef #017 without blocking login.
#
#   test_oidc_roles_idp_realm_source:
#     Config: RoleMapping.Source = "idp_realm_roles" (port 5250).
#     The mock Keycloak emits realm_access.roles = ["test-role"].
#     Verify `roles` in the handoff response contains "test-role".
#
#   test_oidc_roles_merge_source:
#     Config: RoleMapping.Source = "merge", IdpRolePrefix = "kc:" (port 5251).
#     DB part: account_id=1 has no role rows → "" from #017.
#     IdP part: mock emits ["test-role"] → "kc:test-role" after prefixing.
#     Merged result: "kc:test-role".
#     Verify `roles` in the handoff response contains "kc:test-role".
#
# Depends on helpers from oidc_rp_helpers_link.sh:
#   - _drive_oidc_chain
#   - seed_oidc_identity, unseed_oidc_identity
#   - seed_oidc_queryref_seed_only
#   - seed_email_queryrefs
#   - seed_provision_queryrefs
#
# CHANGELOG
# 1.1.0 - 2026-06-20 - Speed-up: replace broken tail-offset migration-wait loops (which timed out
#                      ~30s each because the shared SERVER_LOG is truncated per instance) with
#                      wait_for_migration_ready keyed off the canonical "READY FOR REQUESTS" signal.
# 1.0.0 - 2026-05-09 - Initial creation, Phase 22 role-mapping sub-tests.

# ---------------------------------------------------------------------------
# seed_roles_queryref
#   Ensure QueryRef #017 "Get User Roles" is present in the demo SQLite DB.
#   QueryRef #017 is already present in the baseline demo DB (it is in
#   acuranzo_1108) but the demo DB may not have the SQL-type row if the
#   migration was never applied. This helper is idempotent.
#
# Args:
#   $1  sqlite_db   path to the SQLite database
# ---------------------------------------------------------------------------
seed_roles_queryref() {
    local sqlite_db="$1"
    if [[ ! -f "${sqlite_db}" ]]; then return 1; fi

    # Insert QueryRef #017 as a SQL-type query if it is not already present.
    # The query selects role_id from account_roles filtered by validity window.
    sqlite3 "${sqlite_db}" <<'SEED_017' 2>/dev/null || true
INSERT OR IGNORE INTO queries (
    query_id, query_ref, query_status_a27, query_type_a28,
    query_dialect_a30, query_queue_a58, query_timeout,
    name, code, summary, collection,
    created_id, created_at, updated_id, updated_at
)
SELECT
    (SELECT COALESCE(MAX(query_id), 0) + 1 FROM queries),
    '017',
    1,
    2,
    'sqlite',
    0,
    30,
    'Get User Roles',
    'SELECT role_id FROM account_roles WHERE (account_id = :ACCOUNTID) AND ((valid_after IS NULL) OR (valid_after < datetime(''now''))) AND ((valid_until IS NULL) OR (valid_until > datetime(''now'')));',
    'QueryRef #017 - Get User Roles',
    '{}',
    1, datetime('now'), 1, datetime('now')
WHERE NOT EXISTS (
    SELECT 1 FROM queries WHERE query_ref = '017' AND query_type_a28 = 2
);
SEED_017
}

# ---------------------------------------------------------------------------
# seed_role_row
#   Insert an account_roles row for account_id=1, role_id=42.
#   Used to verify the DATABASE source returns non-empty roles.
#   Idempotent.
#
# Args:
#   $1  sqlite_db   path to the SQLite database
# ---------------------------------------------------------------------------
seed_role_row() {
    local sqlite_db="$1"
    if [[ ! -f "${sqlite_db}" ]]; then return 1; fi

    sqlite3 "${sqlite_db}" <<'SEED_ROLE' 2>/dev/null || true
INSERT OR IGNORE INTO account_roles (
    account_id, role_id, system_id, status_a37,
    created_id, created_at, updated_id, updated_at
) VALUES (
    1, 42, 1, 1,
    1, datetime('now'), 1, datetime('now')
);
SEED_ROLE
}

# ---------------------------------------------------------------------------
# unseed_role_row
#   Remove the test account_roles row for account_id=1, role_id=42.
#
# Args:
#   $1  sqlite_db   path to the SQLite database
# ---------------------------------------------------------------------------
unseed_role_row() {
    local sqlite_db="$1"
    if [[ ! -f "${sqlite_db}" ]]; then return 0; fi

    sqlite3 "${sqlite_db}" \
        "DELETE FROM account_roles WHERE account_id=1 AND role_id=42 AND system_id=1;" \
        2>/dev/null || true
}

# ---------------------------------------------------------------------------
# test_oidc_roles_database_source
#   Verify that RoleMapping.Source = "database" works end-to-end.
#   Accounts with roles produce the role_id list in the handoff response.
#
# Args:
#   $1  base_url    Hydrogen base URL (port 5249 — default config)
#   $2  sqlite_db   path to the SQLite database
#   $3  issuer      OIDC issuer string matching the config
#   $4  subject     the mock IdP's sub claim
#   $5  account_id  existing account_id to seed the identity row against
# ---------------------------------------------------------------------------
test_oidc_roles_database_source() {
    local base_url="$1"
    local sqlite_db="$2"
    local issuer="$3"
    local subject="$4"
    local account_id="$5"

    local headers_file="${LOG_PREFIX}${TIMESTAMP}_roles_db_headers.txt"
    local exchange_file="${LOG_PREFIX}${TIMESTAMP}_roles_db_exchange.json"
    OIDC_CHAIN_HANDOFF=""
    OIDC_CHAIN_ERROR=""
    OIDC_CHAIN_STATUS=""

    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" \
        "Phase 22: RoleMapping.Source=database -> roles='42' in JWT"

    if ! command -v jq >/dev/null 2>&1; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" \
            "jq missing — Phase 22 database-roles sub-test requires it"
        return
    fi

    if ! command -v sqlite3 >/dev/null 2>&1; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" \
            "sqlite3 missing — skipping Phase 22 database-roles sub-test"
        return
    fi

    # Ensure seeds are present.
    seed_oidc_queryref_seed_only "${sqlite_db}" || true
    seed_email_queryrefs "${sqlite_db}" || true
    seed_provision_queryrefs "${sqlite_db}" || true
    seed_roles_queryref "${sqlite_db}" || true

    # Seed role row for account_id=1 so DATABASE source returns "42".
    unseed_role_row "${sqlite_db}" || true
    seed_role_row "${sqlite_db}" || true

    # Seed the identity row so the sub fast-path fires (no provisioning noise).
    unseed_oidc_identity "${sqlite_db}" "${issuer}" "${subject}" || true
    seed_oidc_identity "${sqlite_db}" "${account_id}" "${issuer}" "${subject}" || {
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 \
            "Failed to seed identity row"
        EXIT_CODE=1
        unseed_role_row "${sqlite_db}" || true
        return
    }

    _drive_oidc_chain "${base_url}" "${headers_file}"

    # Cleanup always runs.
    unseed_oidc_identity "${sqlite_db}" "${issuer}" "${subject}" || true
    unseed_role_row "${sqlite_db}" || true

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

    local success roles
    success=$(jq -r '.success // empty' "${exchange_file}" 2>/dev/null || echo "")
    roles=$(jq -r   '.roles   // empty' "${exchange_file}" 2>/dev/null || echo "")

    if [[ "${success}" != "true" ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 \
            "Envelope: success=${success}"
        EXIT_CODE=1
        return
    fi

    # roles should contain "42" (the seeded role_id).
    if [[ "${roles}" != *"42"* ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 \
            "Expected roles to contain '42', got: '${roles}'"
        EXIT_CODE=1
        return
    fi

    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 \
        "database role-mapping: roles='${roles}' (role_id 42 present)"
    PASS_COUNT=$(( PASS_COUNT + 1 ))
}

# ---------------------------------------------------------------------------
# test_oidc_roles_idp_realm_source
#   Verify that RoleMapping.Source = "idp_realm_roles" works end-to-end.
#   The mock emits realm_access.roles = ["test-role"]; expect roles="test-role".
#
# Args:
#   $1  base_url    Hydrogen base URL (idp-roles config, port 5250)
#   $2  sqlite_db   path to the SQLite database
#   $3  issuer      OIDC issuer string
#   $4  subject     mock sub claim
#   $5  account_id  account_id to seed identity row against
# ---------------------------------------------------------------------------
test_oidc_roles_idp_realm_source() {
    local base_url="$1"
    local sqlite_db="$2"
    local issuer="$3"
    local subject="$4"
    local account_id="$5"

    local headers_file="${LOG_PREFIX}${TIMESTAMP}_roles_idp_headers.txt"
    local exchange_file="${LOG_PREFIX}${TIMESTAMP}_roles_idp_exchange.json"
    OIDC_CHAIN_HANDOFF=""
    OIDC_CHAIN_ERROR=""
    OIDC_CHAIN_STATUS=""

    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" \
        "Phase 22: RoleMapping.Source=idp_realm_roles -> roles='test-role'"

    if ! command -v jq >/dev/null 2>&1; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" \
            "jq missing — Phase 22 idp-realm-roles sub-test requires it"
        return
    fi

    if ! command -v sqlite3 >/dev/null 2>&1; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" \
            "sqlite3 missing — skipping Phase 22 idp-realm-roles sub-test"
        return
    fi

    # Seeds.
    seed_oidc_queryref_seed_only "${sqlite_db}" || true
    seed_email_queryrefs "${sqlite_db}" || true
    seed_provision_queryrefs "${sqlite_db}" || true

    # Seed identity row so sub fast-path fires.
    unseed_oidc_identity "${sqlite_db}" "${issuer}" "${subject}" || true
    seed_oidc_identity "${sqlite_db}" "${account_id}" "${issuer}" "${subject}" || {
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 \
            "Failed to seed identity row"
        EXIT_CODE=1
        return
    }

    _drive_oidc_chain "${base_url}" "${headers_file}"

    # Cleanup.
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

    local success roles
    success=$(jq -r '.success // empty' "${exchange_file}" 2>/dev/null || echo "")
    roles=$(jq -r   '.roles   // empty' "${exchange_file}" 2>/dev/null || echo "")

    if [[ "${success}" != "true" ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 \
            "Envelope: success=${success}"
        EXIT_CODE=1
        return
    fi

    if [[ "${roles}" != *"test-role"* ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 \
            "Expected roles to contain 'test-role', got: '${roles}'"
        EXIT_CODE=1
        return
    fi

    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 \
        "idp_realm_roles mapping: roles='${roles}'"
    PASS_COUNT=$(( PASS_COUNT + 1 ))
}

# ---------------------------------------------------------------------------
# test_oidc_roles_merge_source
#   Verify that RoleMapping.Source = "merge" + IdpRolePrefix = "kc:" works.
#   DB: no account_roles rows → empty from #017.
#   IdP: realm_access.roles = ["test-role"] → "kc:test-role".
#   Merged: "kc:test-role".
#
# Args:
#   $1  base_url    Hydrogen base URL (merge config, port 5251)
#   $2  sqlite_db   path to the SQLite database
#   $3  issuer      OIDC issuer string
#   $4  subject     mock sub claim
#   $5  account_id  account_id to seed identity row against
# ---------------------------------------------------------------------------
test_oidc_roles_merge_source() {
    local base_url="$1"
    local sqlite_db="$2"
    local issuer="$3"
    local subject="$4"
    local account_id="$5"

    local headers_file="${LOG_PREFIX}${TIMESTAMP}_roles_merge_headers.txt"
    local exchange_file="${LOG_PREFIX}${TIMESTAMP}_roles_merge_exchange.json"
    OIDC_CHAIN_HANDOFF=""
    OIDC_CHAIN_ERROR=""
    OIDC_CHAIN_STATUS=""

    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" \
        "Phase 22: RoleMapping.Source=merge IdpRolePrefix=kc: -> roles='kc:test-role'"

    if ! command -v jq >/dev/null 2>&1; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" \
            "jq missing — Phase 22 merge-roles sub-test requires it"
        return
    fi

    if ! command -v sqlite3 >/dev/null 2>&1; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" \
            "sqlite3 missing — skipping Phase 22 merge-roles sub-test"
        return
    fi

    # Seeds.
    seed_oidc_queryref_seed_only "${sqlite_db}" || true
    seed_email_queryrefs "${sqlite_db}" || true
    seed_provision_queryrefs "${sqlite_db}" || true
    seed_roles_queryref "${sqlite_db}" || true

    # Ensure no role rows for account_id=1 so DB half is empty.
    unseed_role_row "${sqlite_db}" || true

    # Seed identity row so sub fast-path fires.
    unseed_oidc_identity "${sqlite_db}" "${issuer}" "${subject}" || true
    seed_oidc_identity "${sqlite_db}" "${account_id}" "${issuer}" "${subject}" || {
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 \
            "Failed to seed identity row"
        EXIT_CODE=1
        return
    }

    _drive_oidc_chain "${base_url}" "${headers_file}"

    # Cleanup.
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

    local success roles
    success=$(jq -r '.success // empty' "${exchange_file}" 2>/dev/null || echo "")
    roles=$(jq -r   '.roles   // empty' "${exchange_file}" 2>/dev/null || echo "")

    if [[ "${success}" != "true" ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 \
            "Envelope: success=${success}"
        EXIT_CODE=1
        return
    fi

    # Merged roles: DB empty + IdP "kc:test-role" → expect "kc:test-role".
    if [[ "${roles}" != *"kc:test-role"* ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 \
            "Expected roles to contain 'kc:test-role', got: '${roles}'"
        EXIT_CODE=1
        return
    fi

    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 \
        "merge role-mapping: roles='${roles}' (kc:test-role present)"
    PASS_COUNT=$(( PASS_COUNT + 1 ))
}

# ---------------------------------------------------------------------------
# run_phase22_roles_tests
#   Full lifecycle block for Phase 22 role-mapping tests. Starts three
#   Hydrogen instances:
#     - default-config (port 5249) for the database-source sub-test.
#     - idp-roles-config (port 5250) for the idp_realm_roles sub-test.
#     - merge-config (port 5251) for the merge sub-test.
#
# Args:
#   $1  default_base_url_unused  (kept for interface symmetry; not used
#                                  since we start our own default instance)
#   $2  config_path_idp          Path to the idp_realm_roles config file
#   $3  config_path_merge        Path to the merge config file
#   $4  sqlite_db                Path to the demo SQLite database
#   $5  mock_issuer              OIDC issuer URL of the mock Keycloak
#   $6  mock_subject             sub claim value the mock IdP emits
#   $7  hydrogen_bin             Path to the Hydrogen binary
#   $8  server_log               Path to the shared server log file
#   $9  results_dir              Path to the results directory
# ---------------------------------------------------------------------------
run_phase22_roles_tests() {
    # $1 is kept for interface symmetry but not used here.
    local config_path_idp="$2"
    local config_path_merge="$3"
    local sqlite_db="$4"
    local mock_issuer="$5"
    local mock_subject="$6"
    local hydrogen_bin="$7"
    local server_log="$8"
    local results_dir="$9"

    local account_id=1  # adminuser in the demo DB

    # ---- Start default-config Hydrogen (port 5249) for database source ----
    local p22_default_config
    p22_default_config="$(dirname "${server_log}")/../configs/hydrogen_test_42_oidc_rp_default.json"
    # Fall back to relative path from SCRIPT_DIR if available.
    if [[ -n "${SCRIPT_DIR:-}" ]]; then
        p22_default_config="${SCRIPT_DIR}/configs/hydrogen_test_42_oidc_rp_default.json"
    fi

    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Validate default-config for Phase 22 (database source)"
    # shellcheck disable=SC2310 # We want to continue even if the test fails
    if ! validate_config_file "${p22_default_config}"; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Phase 22 default-config validation failed"
        EXIT_CODE=1
        return
    fi
    local p22_default_port
    p22_default_port=$(get_webserver_port "${p22_default_config}")
    local p22_default_base_url="http://localhost:${p22_default_port}"
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Phase 22 default-config validated (port ${p22_default_port})"
    PASS_COUNT=$(( PASS_COUNT + 1 ))

    # Seed QueryRefs BEFORE Hydrogen starts.
    seed_oidc_queryref_seed_only "${sqlite_db}" || true
    seed_email_queryrefs "${sqlite_db}" || true
    seed_provision_queryrefs "${sqlite_db}" || true
    seed_roles_queryref "${sqlite_db}" || true

    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Start Hydrogen Server (Phase 22 default config)"
    local p22_default_pid=""
    local p22_default_pid_var="P22_DEFAULT_HYDROGEN_PID_$$"

    # shellcheck disable=SC2310 # We want to continue even if the test fails
    if start_hydrogen_with_pid "${p22_default_config}" "${server_log}" 30 "${hydrogen_bin}" "${p22_default_pid_var}"; then
        p22_default_pid=$(eval "echo \$${p22_default_pid_var}")
        if [[ -n "${p22_default_pid}" ]] && ps -p "${p22_default_pid}" > /dev/null 2>&1; then
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Phase 22 default server started with PID ${p22_default_pid}"
            PASS_COUNT=$(( PASS_COUNT + 1 ))
        else
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Phase 22 default server PID missing after start"
            EXIT_CODE=1
        fi
    else
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Failed to start Hydrogen with Phase 22 default config"
        EXIT_CODE=1
    fi

    if [[ "${EXIT_CODE}" -eq 0 ]]; then
        # shellcheck disable=SC2310 # We want to continue even if the test fails
        if ! wait_for_server_ready "${p22_default_base_url}"; then
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Phase 22 default server failed to become ready"
            EXIT_CODE=1
        fi
    fi

    if [[ "${EXIT_CODE}" -eq 0 ]]; then
        # shellcheck disable=SC2310 # Diagnostic-only; proceed even on timeout
        wait_for_migration_ready "${server_log}" 30 || true

        # Phase 22 sub-test (a): database source.
        # shellcheck disable=SC2310 # We want to continue even if the test fails
        test_oidc_roles_database_source \
            "${p22_default_base_url}" "${sqlite_db}" \
            "${mock_issuer}" "${mock_subject}" "${account_id}"
    fi

    if [[ -n "${p22_default_pid:-}" ]] && ps -p "${p22_default_pid}" > /dev/null 2>&1; then
        print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Stop Hydrogen Server (Phase 22 default config)"
        # shellcheck disable=SC2310 # We want to continue even if the test fails
        if stop_hydrogen "${p22_default_pid}" "${server_log}" 10 5 "${results_dir}"; then
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "Phase 22 default server stopped cleanly"
            PASS_COUNT=$(( PASS_COUNT + 1 ))
        else
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Phase 22 default server shutdown had issues"
            EXIT_CODE=1
        fi
        # shellcheck disable=SC2310 # Diagnostic-only
        check_time_wait_sockets "${p22_default_port}" || true
    fi

    # ---- Start idp-roles-config Hydrogen (port 5250) ----
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Validate idp-roles-config (idp_realm_roles)"
    # shellcheck disable=SC2310 # We want to continue even if the test fails
    if ! validate_config_file "${config_path_idp}"; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "idp-roles-config validation failed"
        EXIT_CODE=1
        return
    fi
    local idp_port
    idp_port=$(get_webserver_port "${config_path_idp}")
    local idp_base_url="http://localhost:${idp_port}"
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "idp-roles-config validated (port ${idp_port})"
    PASS_COUNT=$(( PASS_COUNT + 1 ))

    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Start Hydrogen Server (idp-roles config)"
    local idp_hydrogen_pid=""
    local idp_pid_var="IDP_ROLES_HYDROGEN_PID_$$"

    # shellcheck disable=SC2310 # We want to continue even if the test fails
    if start_hydrogen_with_pid "${config_path_idp}" "${server_log}" 30 "${hydrogen_bin}" "${idp_pid_var}"; then
        idp_hydrogen_pid=$(eval "echo \$${idp_pid_var}")
        if [[ -n "${idp_hydrogen_pid}" ]] && ps -p "${idp_hydrogen_pid}" > /dev/null 2>&1; then
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "idp-roles server started with PID ${idp_hydrogen_pid}"
            PASS_COUNT=$(( PASS_COUNT + 1 ))
        else
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "idp-roles server PID missing after start"
            EXIT_CODE=1
        fi
    else
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Failed to start Hydrogen with idp-roles config"
        EXIT_CODE=1
    fi

    if [[ "${EXIT_CODE}" -eq 0 ]]; then
        # shellcheck disable=SC2310 # We want to continue even if the test fails
        if ! wait_for_server_ready "${idp_base_url}"; then
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "idp-roles server failed to become ready"
            EXIT_CODE=1
        fi
    fi

    if [[ "${EXIT_CODE}" -eq 0 ]]; then
        # shellcheck disable=SC2310 # Diagnostic-only; proceed even on timeout
        wait_for_migration_ready "${server_log}" 30 || true

        # Phase 22 sub-test (b): idp_realm_roles source.
        # shellcheck disable=SC2310 # We want to continue even if the test fails
        test_oidc_roles_idp_realm_source \
            "${idp_base_url}" "${sqlite_db}" \
            "${mock_issuer}" "${mock_subject}" "${account_id}"
    fi

    if [[ -n "${idp_hydrogen_pid:-}" ]] && ps -p "${idp_hydrogen_pid}" > /dev/null 2>&1; then
        print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Stop Hydrogen Server (idp-roles config)"
        # shellcheck disable=SC2310 # We want to continue even if the test fails
        if stop_hydrogen "${idp_hydrogen_pid}" "${server_log}" 10 5 "${results_dir}"; then
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "idp-roles server stopped cleanly"
            PASS_COUNT=$(( PASS_COUNT + 1 ))
        else
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "idp-roles server shutdown had issues"
            EXIT_CODE=1
        fi
        # shellcheck disable=SC2310 # Diagnostic-only
        check_time_wait_sockets "${idp_port}" || true
    fi

    # ---- Start merge-config Hydrogen (port 5251) ----
    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Validate merge-config (merge + IdpRolePrefix=kc:)"
    # shellcheck disable=SC2310 # We want to continue even if the test fails
    if ! validate_config_file "${config_path_merge}"; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "merge-config validation failed"
        EXIT_CODE=1
        return
    fi
    local merge_port
    merge_port=$(get_webserver_port "${config_path_merge}")
    local merge_base_url="http://localhost:${merge_port}"
    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "merge-config validated (port ${merge_port})"
    PASS_COUNT=$(( PASS_COUNT + 1 ))

    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Start Hydrogen Server (merge config)"
    local merge_hydrogen_pid=""
    local merge_pid_var="MERGE_HYDROGEN_PID_$$"

    # shellcheck disable=SC2310 # We want to continue even if the test fails
    if start_hydrogen_with_pid "${config_path_merge}" "${server_log}" 30 "${hydrogen_bin}" "${merge_pid_var}"; then
        merge_hydrogen_pid=$(eval "echo \$${merge_pid_var}")
        if [[ -n "${merge_hydrogen_pid}" ]] && ps -p "${merge_hydrogen_pid}" > /dev/null 2>&1; then
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "merge server started with PID ${merge_hydrogen_pid}"
            PASS_COUNT=$(( PASS_COUNT + 1 ))
        else
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "merge server PID missing after start"
            EXIT_CODE=1
        fi
    else
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "Failed to start Hydrogen with merge config"
        EXIT_CODE=1
    fi

    if [[ "${EXIT_CODE}" -eq 0 ]]; then
        # shellcheck disable=SC2310 # We want to continue even if the test fails
        if ! wait_for_server_ready "${merge_base_url}"; then
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "merge server failed to become ready"
            EXIT_CODE=1
        fi
    fi

    if [[ "${EXIT_CODE}" -eq 0 ]]; then
        # shellcheck disable=SC2310 # Diagnostic-only; proceed even on timeout
        wait_for_migration_ready "${server_log}" 30 || true

        # Phase 22 sub-test (c): merge source.
        # shellcheck disable=SC2310 # We want to continue even if the test fails
        test_oidc_roles_merge_source \
            "${merge_base_url}" "${sqlite_db}" \
            "${mock_issuer}" "${mock_subject}" "${account_id}"
    fi

    if [[ -n "${merge_hydrogen_pid:-}" ]] && ps -p "${merge_hydrogen_pid}" > /dev/null 2>&1; then
        print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" "Stop Hydrogen Server (merge config)"
        # shellcheck disable=SC2310 # We want to continue even if the test fails
        if stop_hydrogen "${merge_hydrogen_pid}" "${server_log}" 10 5 "${results_dir}"; then
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 "merge server stopped cleanly"
            PASS_COUNT=$(( PASS_COUNT + 1 ))
        else
            print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 "merge server shutdown had issues"
            EXIT_CODE=1
        fi
        # shellcheck disable=SC2310 # Diagnostic-only
        check_time_wait_sockets "${merge_port}" || true
    fi
}
