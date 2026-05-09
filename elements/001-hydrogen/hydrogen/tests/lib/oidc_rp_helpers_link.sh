#!/usr/bin/env bash

# shellcheck disable=SC2154 # All globals (TEST_NUMBER, TEST_COUNTER, GREP, LOG_PREFIX, TIMESTAMP, etc.) are set by the test script via framework.sh before sourcing this lib
# shellcheck disable=SC2034 # EXIT_CODE is set by the test script and re-exported through these helpers
# shellcheck disable=SC2312 # Several diagnostic command substitutions intentionally swallow the inner exit code

# OIDC RP Account Linker Test Utilities (Phase 18 + Phase 19)
#
# Helpers for black-box testing the account-linker strategies:
#   Phase 18: match_sub_only   (QueryRefs #080, #084)
#   Phase 19: match_email_only (QueryRefs #080, #082, #081, #084)
#
# Requires the SQLite-backed Hydrogen instance plus the demo Acuranzo
# database at tests/artifacts/database/sqlite/hydrodemo.sqlite.
#
# Provides:
#   - seed_oidc_identity:       insert a row into account_oidc_identities
#   - unseed_oidc_identity:     delete the seeded row
#   - seed_email_contact:       insert a mock email into account_contacts
#   - unseed_email_contact:     delete the seeded email contact row
#   - seed_email_queryrefs:     ensure QueryRefs #081 and #082 are present
#   - test_oidc_link_match_sub_only_hit:    pre-seeded identity → JWT
#   - test_oidc_link_match_sub_only_miss:   no identity row → no_account
#   - test_oidc_link_match_email_only_hit:  email match → linked → JWT
#   - test_oidc_link_match_email_only_miss: no email contact → no_account
#   - test_oidc_link_match_email_only_ambiguous: two accounts, same email → email_ambiguous
#
# CHANGELOG
# 1.1.0 - 2026-05-09 - Phase 19: match_email_only helpers.
# 1.0.0 - 2026-05-09 - Initial creation for Phase 18 account linker.

# ---------------------------------------------------------------------------
# SQLite helpers
# ---------------------------------------------------------------------------

# Insert a row into account_oidc_identities using the demo SQLite fixture.
# Args:
#   $1  sqlite_db  path to the SQLite database file
#   $2  account_id integer
#   $3  issuer     string (must match Hydrogen's configured Issuer)
#   $4  subject    string (the IdP sub claim the mock will emit)
seed_oidc_identity() {
    local sqlite_db="$1"
    local account_id="$2"
    local issuer="$3"
    local subject="$4"

    if ! command -v sqlite3 >/dev/null 2>&1; then
        return 1
    fi

    # Ensure the table exists before inserting. The production schema
    # is created by acuranzo_1189.lua; for the test SQLite fixture
    # (which may not have had all migrations applied) we create it
    # inline with the same columns. CREATE TABLE IF NOT EXISTS is a
    # no-op when the table already exists, so this is safe to run
    # against any version of the fixture.
    sqlite3 "${sqlite_db}" <<'ENSURE_TABLE'
CREATE TABLE IF NOT EXISTS account_oidc_identities (
    identity_id   INTEGER PRIMARY KEY,
    account_id    INTEGER NOT NULL,
    issuer        TEXT NOT NULL,
    subject       TEXT NOT NULL,
    email         TEXT,
    email_verified INTEGER,
    last_seen_at  TEXT NOT NULL,
    created_at    TEXT NOT NULL,
    updated_at    TEXT NOT NULL,
    valid_after   TEXT NOT NULL,
    valid_until   TEXT NOT NULL,
    created_id    INTEGER,
    updated_id    INTEGER,
    UNIQUE (issuer, subject)
);
CREATE INDEX IF NOT EXISTS account_oidc_identities_idx_account
    ON account_oidc_identities(account_id);
ENSURE_TABLE

    # Ensure QueryRef #080 (lookup by issuer+subject) and #084 (touch)
    # exist in the queries table. The demo SQLite fixture was built before
    # Phase 17 migrations; Phase 18's linker needs these two QueryRefs
    # registered so execute_auth_query(80, ...) and execute_auth_query(84, ...)
    # succeed at runtime. We insert them with INSERT OR IGNORE so this
    # is idempotent whether or not AutoMigration has already applied them.
    #
    # Column semantics (from the queries schema):
    #   query_status_a27 = 1  (active)
    #   query_type_a28   = 1  (SQL — the operational query type)
    #   query_dialect_a30 = 2 (SQLite — matches the demo DB engine)
    #   query_queue_a58  = 1  (medium/slow queue — same as other auth queries)
    #   query_timeout    = 5000 (5 s — same as existing auth queries)
    sqlite3 "${sqlite_db}" <<'ENSURE_QUERYREFS'
INSERT OR IGNORE INTO queries (
    query_id, query_ref,
    query_status_a27, query_type_a28, query_dialect_a30,
    query_queue_a58, query_timeout,
    code, name, summary, collection,
    valid_after, valid_until, created_id, created_at, updated_id, updated_at
)
SELECT
    (SELECT COALESCE(MAX(query_id), 0) + 1 FROM queries),
    80, 1, 1, 2, 1, 5000,
    'SELECT identity_id, account_id, issuer, subject, email, email_verified, last_seen_at FROM account_oidc_identities WHERE (issuer = :ISSUER) AND (subject = :SUBJECT)',
    'OIDC RP: Lookup OIDC Identity',
    'Phase 18 seed: lookup (issuer, subject) -> account_id',
    '{}',
    datetime('now'), datetime('now', '+10 years'), 1, datetime('now'), 1, datetime('now')
WHERE NOT EXISTS (SELECT 1 FROM queries WHERE query_ref = 80 AND query_type_a28 = 1);

INSERT OR IGNORE INTO queries (
    query_id, query_ref,
    query_status_a27, query_type_a28, query_dialect_a30,
    query_queue_a58, query_timeout,
    code, name, summary, collection,
    valid_after, valid_until, created_id, created_at, updated_id, updated_at
)
SELECT
    (SELECT COALESCE(MAX(query_id), 0) + 1 FROM queries),
    84, 1, 1, 2, 1, 5000,
    'UPDATE account_oidc_identities SET last_seen_at = datetime(''now''), email = :EMAIL, email_verified = :EMAILVERIFIED, updated_at = datetime(''now'') WHERE identity_id = :IDENTITYID',
    'OIDC RP: Touch OIDC Identity',
    'Phase 18 seed: update last_seen_at, email, email_verified',
    '{}',
    datetime('now'), datetime('now', '+10 years'), 1, datetime('now'), 1, datetime('now')
WHERE NOT EXISTS (SELECT 1 FROM queries WHERE query_ref = 84 AND query_type_a28 = 1);
ENSURE_QUERYREFS

    sqlite3 "${sqlite_db}" <<EOF
INSERT OR REPLACE INTO account_oidc_identities (
    identity_id,
    account_id,
    issuer,
    subject,
    email,
    email_verified,
    last_seen_at,
    created_at,
    updated_at,
    valid_after,
    valid_until,
    created_id,
    updated_id
)
SELECT
    COALESCE(MAX(identity_id), 0) + 1,
    ${account_id},
    '${issuer}',
    '${subject}',
    NULL,
    0,
    datetime('now'),
    datetime('now'),
    datetime('now'),
    datetime('now'),
    datetime('now', '+10 years'),
    1,
    1
FROM account_oidc_identities;
EOF
}

# Delete all identity rows matching (issuer, subject) from the demo DB.
# Args:
#   $1  sqlite_db  path to the SQLite database file
#   $2  issuer     string
#   $3  subject    string
unseed_oidc_identity() {
    local sqlite_db="$1"
    local issuer="$2"
    local subject="$3"

    if ! command -v sqlite3 >/dev/null 2>&1; then
        return 1
    fi

    sqlite3 "${sqlite_db}" \
        "DELETE FROM account_oidc_identities WHERE issuer='${issuer}' AND subject='${subject}';"
}

# Insert a mock email address into account_contacts for the given account_id.
# The mock IdP sends email='mockuser@example.com'; seeding this contact lets
# the match_email_only strategy resolve the email to the account.
#
# Args:
#   $1  sqlite_db  path to the SQLite database file
#   $2  account_id integer
#   $3  email      the email address to seed (must match what the mock IdP sends)
seed_email_contact() {
    local sqlite_db="$1"
    local account_id="$2"
    local email="$3"

    if ! command -v sqlite3 >/dev/null 2>&1; then
        return 1
    fi

    # contact_type_a18 = 0 means email (per QueryRef #082 / #011 contract).
    # contact_seq=0, authenticate_a19=1, status_a20=1 match existing email rows.
    # INSERT OR IGNORE: idempotent; won't fail if the row already exists.
    sqlite3 "${sqlite_db}" <<EOF
INSERT OR IGNORE INTO account_contacts (
    contact_id,
    account_id,
    contact_seq,
    contact_type_a18,
    authenticate_a19,
    status_a20,
    contact,
    valid_after,
    valid_until,
    created_at,
    updated_at,
    created_id,
    updated_id
)
SELECT
    COALESCE(MAX(contact_id), 0) + 1,
    ${account_id},
    0,
    0,
    1,
    1,
    '${email}',
    datetime('now', '-1 year'),
    datetime('now', '+10 years'),
    datetime('now'),
    datetime('now'),
    1,
    1
FROM account_contacts
WHERE NOT EXISTS (
    SELECT 1 FROM account_contacts
    WHERE account_id = ${account_id}
      AND contact = '${email}'
      AND contact_type_a18 = 0
);
EOF
}

# Delete the seeded email contact row for the given account/email.
# Args:
#   $1  sqlite_db  path to the SQLite database file
#   $2  account_id integer
#   $3  email      string
unseed_email_contact() {
    local sqlite_db="$1"
    local account_id="$2"
    local email="$3"

    if ! command -v sqlite3 >/dev/null 2>&1; then
        return 1
    fi

    sqlite3 "${sqlite_db}" \
        "DELETE FROM account_contacts WHERE account_id=${account_id} AND contact='${email}' AND contact_type_a18=0;"
}

# Seed a second account with the same email to trigger email_ambiguous.
# We use account_id=2 which exists in the demo DB as 'demouser'.
seed_ambiguous_email_contact() {
    local sqlite_db="$1"
    local email="$2"

    seed_email_contact "${sqlite_db}" 2 "${email}"
}

unseed_ambiguous_email_contact() {
    local sqlite_db="$1"
    local email="$2"

    unseed_email_contact "${sqlite_db}" 2 "${email}"
}

# Ensure QueryRefs #081 and #082 are registered in the queries table.
# Phase 19's match_email_only linker calls both; the demo fixture may not
# have them if AutoMigration has not run.
# Args:
#   $1  sqlite_db  path to the SQLite database file
seed_email_queryrefs() {
    local sqlite_db="$1"

    if ! command -v sqlite3 >/dev/null 2>&1; then
        return 1
    fi

    sqlite3 "${sqlite_db}" <<'ENSURE_EMAIL_QUERYREFS'
INSERT OR IGNORE INTO queries (
    query_id, query_ref,
    query_status_a27, query_type_a28, query_dialect_a30,
    query_queue_a58, query_timeout,
    code, name, summary, collection,
    valid_after, valid_until, created_id, created_at, updated_id, updated_at
)
SELECT
    (SELECT COALESCE(MAX(query_id), 0) + 1 FROM queries),
    81, 1, 1, 2, 1, 5000,
    'INSERT INTO account_oidc_identities (identity_id, account_id, issuer, subject, email, email_verified, last_seen_at, created_at, updated_at, valid_after, valid_until, created_id, updated_id) WITH next_identity_id AS (SELECT COALESCE(MAX(identity_id), 0) + 1 AS new_identity_id FROM account_oidc_identities), account_check AS (SELECT account_id FROM accounts WHERE account_id = :ACCOUNTID) SELECT next_identity_id.new_identity_id, account_check.account_id, :ISSUER, :SUBJECT, :EMAIL, :EMAILVERIFIED, datetime(''now''), datetime(''now''), datetime(''now''), datetime(''now''), datetime(''now'', ''+10 years''), 0, 0 FROM next_identity_id, account_check RETURNING identity_id',
    'OIDC RP: Link OIDC Identity',
    'Phase 19 seed: link (issuer,subject) to account_id via email match',
    '{}',
    datetime('now'), datetime('now', '+10 years'), 1, datetime('now'), 1, datetime('now')
WHERE NOT EXISTS (SELECT 1 FROM queries WHERE query_ref = 81 AND query_type_a28 = 1);

INSERT OR IGNORE INTO queries (
    query_id, query_ref,
    query_status_a27, query_type_a28, query_dialect_a30,
    query_queue_a58, query_timeout,
    code, name, summary, collection,
    valid_after, valid_until, created_id, created_at, updated_id, updated_at
)
SELECT
    (SELECT COALESCE(MAX(query_id), 0) + 1 FROM queries),
    82, 1, 1, 2, 1, 5000,
    'SELECT DISTINCT ac.account_id FROM account_contacts ac WHERE (LOWER(ac.contact) = LOWER(:EMAIL)) AND (ac.contact_type_a18 = 0) AND ((ac.valid_after IS NULL) OR (ac.valid_after < datetime(''now''))) AND ((ac.valid_until IS NULL) OR (ac.valid_until > datetime(''now'')))',
    'OIDC RP: Lookup Account by Email',
    'Phase 19 seed: lookup account_id by email (account_contacts)',
    '{}',
    datetime('now'), datetime('now', '+10 years'), 1, datetime('now'), 1, datetime('now')
WHERE NOT EXISTS (SELECT 1 FROM queries WHERE query_ref = 82 AND query_type_a28 = 1);
ENSURE_EMAIL_QUERYREFS
}

# ---------------------------------------------------------------------------
# Phase 18 sub-tests
# ---------------------------------------------------------------------------

# Sub-test: with a pre-seeded identity row, the match_sub_only strategy
# resolves to an account and the full /start->callback->handoff chain
# returns a valid JWT envelope.
#
# Args:
#   $1  base_url    Hydrogen base URL (the sub-config instance)
#   $2  sqlite_db   path to the SQLite database
#   $3  issuer      OIDC issuer string matching the config
#   $4  subject     the subject to seed and that the mock IdP will emit
test_oidc_link_match_sub_only_hit() {
    local base_url="$1"
    local sqlite_db="$2"
    local issuer="$3"
    local subject="$4"

    local headers_file="${LOG_PREFIX}${TIMESTAMP}_link_sub_hit_headers.txt"
    local exchange_file="${LOG_PREFIX}${TIMESTAMP}_link_sub_hit_exchange.json"

    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" \
        "Phase 18: match_sub_only with seeded identity -> JWT envelope"

    if ! command -v jq >/dev/null 2>&1; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" \
            "jq missing — Phase 18 sub-test requires it"
        return
    fi

    if ! command -v sqlite3 >/dev/null 2>&1; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" \
            "sqlite3 missing — skipping Phase 18 seed/unseed"
        return
    fi

    # Seed: insert (issuer, subject) linked to account_id=1 (adminuser).
    # shellcheck disable=SC2310 # We want to continue even if the seed fails
    if ! seed_oidc_identity "${sqlite_db}" 1 "${issuer}" "${subject}"; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 \
            "Failed to seed identity row into ${sqlite_db}"
        EXIT_CODE=1
        return
    fi

    # Drive /start -> /auth -> /callback (stop before SPA /login).
    local http_status
    http_status=$(curl -s -i -L --max-redirs 2 \
        -o "${headers_file}" -w '%{http_code}' \
        --max-time 15 \
        "${base_url}/api/auth/oidc/start" 2>/dev/null || echo "000")

    # Pull the handoff code from the last Location header.
    local final_location
    final_location=$("${GREP}" -i '^location:' "${headers_file}" 2>/dev/null \
        | sed 's/^[Ll]ocation:[[:space:]]*//' \
        | tr -d '\r' \
        | tail -n 1)

    local handoff_code
    handoff_code=$(printf '%s' "${final_location}" \
        | sed -n 's/.*[?&]handoff=\([^&]*\).*/\1/p')

    if [[ -z "${handoff_code}" ]]; then
        local err_param
        err_param=$(printf '%s' "${final_location}" \
            | sed -n 's/.*[?&]oidc_error=\([^&]*\).*/\1/p')
        # Clean up before failing.
        unseed_oidc_identity "${sqlite_db}" "${issuer}" "${subject}" || true
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 \
            "No handoff code (oidc_error=${err_param:-none}, status=${http_status}, loc=${final_location:0:160})"
        EXIT_CODE=1
        return
    fi

    # Exchange the handoff.
    local body exchange_status
    body=$(printf '{"handoff":"%s"}' "${handoff_code}")
    exchange_status=$(curl -s -X POST -H "Content-Type: application/json" \
        -d "${body}" -w "%{http_code}" -o "${exchange_file}" \
        --max-time 5 "${base_url}/api/auth/oidc/handoff" \
        2>/dev/null || echo "000")

    # Clean up the seeded row now that the login attempt is done.
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

    if [[ "${user_id}" != "1" ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 \
            "Expected user_id=1 (seeded to account_id=1), got user_id=${user_id}"
        EXIT_CODE=1
        return
    fi

    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 \
        "match_sub_only hit: JWT envelope user_id=${user_id} (seeded identity resolved)"
    PASS_COUNT=$(( PASS_COUNT + 1 ))
}

# Sub-test: without a pre-seeded identity row, match_sub_only must
# redirect back to the SPA with ?oidc_error=no_account.
#
# Args:
#   $1  base_url    Hydrogen base URL (the sub-config instance)
#   $2  sqlite_db   path to the SQLite database
#   $3  issuer      OIDC issuer string matching the config
#   $4  subject     a subject that has NOT been seeded
test_oidc_link_match_sub_only_miss() {
    local base_url="$1"
    local sqlite_db="$2"
    local issuer="$3"
    local subject="$4"

    local headers_file="${LOG_PREFIX}${TIMESTAMP}_link_sub_miss_headers.txt"

    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" \
        "Phase 18: match_sub_only with no identity row -> 302 oidc_error=no_account"

    # Guarantee the row does not exist.
    if command -v sqlite3 >/dev/null 2>&1; then
        unseed_oidc_identity "${sqlite_db}" "${issuer}" "${subject}" || true
    fi

    # Drive the full /start -> /auth -> /callback chain.
    local http_status
    http_status=$(curl -s -i -L --max-redirs 2 \
        -o "${headers_file}" -w '%{http_code}' \
        --max-time 15 \
        "${base_url}/api/auth/oidc/start" 2>/dev/null || echo "000")

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

    local err_param
    err_param=$(printf '%s' "${final_location}" \
        | sed -n 's/.*[?&]oidc_error=\([^&]*\).*/\1/p')

    # We expect no_account (linker found no identity row for this sub).
    if [[ "${err_param}" != "no_account" ]]; then
        local handoff_code
        handoff_code=$(printf '%s' "${final_location}" \
            | sed -n 's/.*[?&]handoff=\([^&]*\).*/\1/p')
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 \
            "Expected oidc_error=no_account, got '${err_param}' (handoff=${handoff_code:-none})"
        EXIT_CODE=1
        return
    fi

    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 \
        "match_sub_only miss: no_account as expected (no identity row)"
    PASS_COUNT=$(( PASS_COUNT + 1 ))
}

# ---------------------------------------------------------------------------
# Phase 19 sub-tests
# ---------------------------------------------------------------------------

# Shared helper: drive /start -> /auth -> /callback and return the result.
# Sets OIDC_CHAIN_HANDOFF, OIDC_CHAIN_ERROR, and OIDC_CHAIN_STATUS.
# Args:
#   $1  base_url
#   $2  headers_file  output file for curl -i output
_drive_oidc_chain() {
    local base_url="$1"
    local headers_file="$2"

    local http_status
    http_status=$(curl -s -i -L --max-redirs 2 \
        -o "${headers_file}" -w '%{http_code}' \
        --max-time 15 \
        "${base_url}/api/auth/oidc/start" 2>/dev/null || echo "000")
    OIDC_CHAIN_STATUS="${http_status}"

    local final_location
    final_location=$("${GREP}" -i '^location:' "${headers_file}" 2>/dev/null \
        | sed 's/^[Ll]ocation:[[:space:]]*//' \
        | tr -d '\r' \
        | tail -n 1)

    OIDC_CHAIN_HANDOFF=$(printf '%s' "${final_location}" \
        | sed -n 's/.*[?&]handoff=\([^&]*\).*/\1/p')
    OIDC_CHAIN_ERROR=$(printf '%s' "${final_location}" \
        | sed -n 's/.*[?&]oidc_error=\([^&]*\).*/\1/p')
}

# Sub-test: match_email_only with a seeded email contact for account_id=1.
# The mock IdP emits email='mockuser@example.com'. The linker should:
#   1. Fail #080 (no prior (iss,sub) link).
#   2. Find account_id=1 via #082.
#   3. Link via #081.
#   4. Touch via #084.
#   5. Mint a JWT and return the handoff.
#
# Args:
#   $1  base_url    Hydrogen base URL (the email-config instance)
#   $2  sqlite_db   path to the SQLite database
#   $3  issuer      OIDC issuer string matching the config
#   $4  subject     the mock IdP's sub claim (to clean up after)
#   $5  email       the email address to seed (must match mock IdP)
test_oidc_link_match_email_only_hit() {
    local base_url="$1"
    local sqlite_db="$2"
    local issuer="$3"
    local subject="$4"
    local email="$5"

    local headers_file="${LOG_PREFIX}${TIMESTAMP}_link_email_hit_headers.txt"
    local exchange_file="${LOG_PREFIX}${TIMESTAMP}_link_email_hit_exchange.json"
    OIDC_CHAIN_HANDOFF=""
    OIDC_CHAIN_ERROR=""
    OIDC_CHAIN_STATUS=""

    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" \
        "Phase 19: match_email_only with seeded email contact -> JWT envelope"

    if ! command -v jq >/dev/null 2>&1; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" \
            "jq missing — Phase 19 sub-test requires it"
        return
    fi

    if ! command -v sqlite3 >/dev/null 2>&1; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" \
            "sqlite3 missing — skipping Phase 19 seed/unseed"
        return
    fi

    # Ensure no prior identity link exists (so #080 misses and #082/#081 fire).
    unseed_oidc_identity "${sqlite_db}" "${issuer}" "${subject}" || true

    # Seed the email contact so #082 can find it.
    # shellcheck disable=SC2310 # We want to continue even if the seed fails
    if ! seed_email_contact "${sqlite_db}" 1 "${email}"; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 \
            "Failed to seed email contact into ${sqlite_db}"
        EXIT_CODE=1
        return
    fi

    # Seed QueryRefs #081 and #082 if not already present.
    seed_email_queryrefs "${sqlite_db}" || true

    # Drive the OIDC chain.
    _drive_oidc_chain "${base_url}" "${headers_file}"

    # Clean up: remove the newly-linked identity row (created by #081)
    # and the email contact seed.
    unseed_oidc_identity "${sqlite_db}" "${issuer}" "${subject}" || true
    unseed_email_contact "${sqlite_db}" 1 "${email}" || true

    if [[ -z "${OIDC_CHAIN_HANDOFF}" ]]; then
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

    if [[ "${user_id}" != "1" ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 \
            "Expected user_id=1 (email seeded for account_id=1), got user_id=${user_id}"
        EXIT_CODE=1
        return
    fi

    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 \
        "match_email_only hit: JWT envelope user_id=${user_id} (email linked)"
    PASS_COUNT=$(( PASS_COUNT + 1 ))
}

# Sub-test: match_email_only with no email contact seeded.
# The #082 lookup should return zero rows → no_account.
#
# Args:
#   $1  base_url    Hydrogen base URL (the email-config instance)
#   $2  sqlite_db   path to the SQLite database
#   $3  issuer      OIDC issuer string
#   $4  subject     the mock IdP's sub claim
#   $5  email       the email address (ensure it is NOT in account_contacts)
test_oidc_link_match_email_only_miss() {
    local base_url="$1"
    local sqlite_db="$2"
    local issuer="$3"
    local subject="$4"
    local email="$5"

    local headers_file="${LOG_PREFIX}${TIMESTAMP}_link_email_miss_headers.txt"
    OIDC_CHAIN_HANDOFF=""
    OIDC_CHAIN_ERROR=""
    OIDC_CHAIN_STATUS=""

    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" \
        "Phase 19: match_email_only with no email contact -> 302 oidc_error=no_account"

    if ! command -v sqlite3 >/dev/null 2>&1; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" \
            "sqlite3 missing — skipping Phase 19 miss sub-test"
        return
    fi

    # Guarantee no identity link and no email contact exist.
    unseed_oidc_identity "${sqlite_db}" "${issuer}" "${subject}" || true
    unseed_email_contact "${sqlite_db}" 1 "${email}" || true
    # Also clear any seeded ambiguous contact from a prior run.
    unseed_ambiguous_email_contact "${sqlite_db}" "${email}" || true

    # Ensure QueryRefs are present.
    seed_email_queryrefs "${sqlite_db}" || true

    # Drive the chain.
    _drive_oidc_chain "${base_url}" "${headers_file}"

    if [[ "${OIDC_CHAIN_ERROR}" != "no_account" ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 \
            "Expected oidc_error=no_account, got '${OIDC_CHAIN_ERROR:-none}' (handoff=${OIDC_CHAIN_HANDOFF:-none})"
        EXIT_CODE=1
        return
    fi

    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 \
        "match_email_only miss: no_account as expected (no email contact)"
    PASS_COUNT=$(( PASS_COUNT + 1 ))
}

# Sub-test: match_email_only with email seeded for TWO accounts.
# The #082 lookup returns two rows → email_ambiguous.
#
# Args:
#   $1  base_url    Hydrogen base URL
#   $2  sqlite_db   path to the SQLite database
#   $3  issuer      OIDC issuer string
#   $4  subject     the mock IdP's sub claim
#   $5  email       the email address to seed for both accounts
test_oidc_link_match_email_only_ambiguous() {
    local base_url="$1"
    local sqlite_db="$2"
    local issuer="$3"
    local subject="$4"
    local email="$5"

    local headers_file="${LOG_PREFIX}${TIMESTAMP}_link_email_amb_headers.txt"
    OIDC_CHAIN_HANDOFF=""
    OIDC_CHAIN_ERROR=""
    OIDC_CHAIN_STATUS=""

    print_subtest "${TEST_NUMBER}" "${TEST_COUNTER}" \
        "Phase 19: match_email_only with email on two accounts -> 302 oidc_error=email_ambiguous"

    if ! command -v sqlite3 >/dev/null 2>&1; then
        print_message "${TEST_NUMBER}" "${TEST_COUNTER}" \
            "sqlite3 missing — skipping Phase 19 ambiguous sub-test"
        return
    fi

    # Ensure no prior identity link.
    unseed_oidc_identity "${sqlite_db}" "${issuer}" "${subject}" || true

    # Seed the email for both account_id=1 and account_id=2.
    seed_email_contact "${sqlite_db}" 1 "${email}" || true
    seed_ambiguous_email_contact "${sqlite_db}" "${email}" || true

    # Ensure QueryRefs are present.
    seed_email_queryrefs "${sqlite_db}" || true

    # Drive the chain.
    _drive_oidc_chain "${base_url}" "${headers_file}"

    # Cleanup.
    unseed_email_contact "${sqlite_db}" 1 "${email}" || true
    unseed_ambiguous_email_contact "${sqlite_db}" "${email}" || true

    if [[ "${OIDC_CHAIN_ERROR}" != "email_ambiguous" ]]; then
        print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 1 \
            "Expected oidc_error=email_ambiguous, got '${OIDC_CHAIN_ERROR:-none}' (handoff=${OIDC_CHAIN_HANDOFF:-none})"
        EXIT_CODE=1
        return
    fi

    print_result "${TEST_NUMBER}" "${TEST_COUNTER}" 0 \
        "match_email_only ambiguous: email_ambiguous as expected (two accounts)"
    PASS_COUNT=$(( PASS_COUNT + 1 ))
}
