-- Migration: acuranzo_1192.lua
-- QueryRef #081 - OIDC RP: Link OIDC Identity (Phase 17)

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.0.0 - 2026-05-09 - Initial creation for OIDC Phase 17

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "queries"
cfg.MIGRATION = "1192"
cfg.QUERY_REF = "081"
cfg.QUERY_NAME = "OIDC RP: Link OIDC Identity"
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
table.insert(queries,{sql=[[

    INSERT INTO ${SCHEMA}${QUERIES} (
        ${QUERIES_INSERT}
    )
    WITH next_query_id AS (
        SELECT COALESCE(MAX(query_id), 0) + 1 AS new_query_id
        FROM ${SCHEMA}${QUERIES}
    )
    SELECT
        new_query_id                                                        AS query_id,
        ${MIGRATION}                                                        AS query_ref,
        ${STATUS_ACTIVE}                                                    AS query_status_a27,
        ${TYPE_FORWARD_MIGRATION}                                           AS query_type_a28,
        ${DIALECT}                                                          AS query_dialect_a30,
        ${QTC_SLOW}                                                         AS query_queue_a58,
        ${TIMEOUT}                                                          AS query_timeout,
        [=[
            INSERT INTO ${SCHEMA}${QUERIES} (
                ${QUERIES_INSERT}
            )
            WITH next_query_id AS (
                SELECT COALESCE(MAX(query_id), 0) + 1 AS new_query_id
                FROM ${SCHEMA}${QUERIES}
            )
            SELECT
                new_query_id                                                        AS query_id,
                ${QUERY_REF}                                                        AS query_ref,
                ${STATUS_ACTIVE}                                                    AS query_status_a27,
                ${TYPE_SQL}                                                         AS query_type_a28,
                ${DIALECT}                                                          AS query_dialect_a30,
                ${QTC_MEDIUM}                                                       AS query_queue_a58,
                ${TIMEOUT}                                                          AS query_timeout,
                [==[
                    ${INSERT_KEY_START} identity_id ${INSERT_KEY_END}
                        INSERT INTO ${SCHEMA}account_oidc_identities (
                            identity_id,
                            account_id,
                            issuer,
                            subject,
                            email,
                            email_verified,
                            last_seen_at,
                            ${COMMON_FIELDS}
                        )
                        WITH
                            next_identity_id AS (
                                SELECT COALESCE(MAX(identity_id), 0) + 1 AS new_identity_id
                                FROM ${SCHEMA}account_oidc_identities
                            ),
                            account_check AS (
                                SELECT account_id
                                FROM ${SCHEMA}accounts
                                WHERE account_id = :ACCOUNTID
                            )
                        SELECT
                            next_identity_id.new_identity_id,
                            account_check.account_id,
                            :ISSUER,
                            :SUBJECT,
                            :EMAIL,
                            :EMAILVERIFIED,
                            ${NOW},
                            '2025-01-01 00:00:00',
                            '2035-01-01 00:00:00',
                            0,
                            ${NOW},
                            0,
                            ${NOW}
                        FROM next_identity_id, account_check
                    ${INSERT_KEY_RETURN} identity_id
                    ;
                ]==]                                                                AS code,
                '${QUERY_NAME}'                                                     AS name,
                [==[
                    #  QueryRef #${QUERY_REF} - ${QUERY_NAME}

                    Links an existing Hydrogen account to an external OIDC identity for the
                    first time. Called when a user signs in via OIDC and their `(issuer,
                    subject)` is not yet in `account_oidc_identities` but their email
                    matches an existing account (the `match_email_only` and
                    `match_email_then_provision` strategies).

                    The INSERT uses a SELECT-then-INSERT CTE so the query fails silently
                    (returns zero rows) if the `account_id` does not exist — no SQL FK
                    constraint, but the application-layer integrity check is embedded here.

                    The UNIQUE constraint on `(issuer, subject)` means a duplicate insert
                    (race condition on concurrent sign-ins) raises a constraint violation
                    rather than creating a second row. The caller (Phase 18/19 linker)
                    handles this by treating a constraint-violation as "already linked" and
                    retrying with QueryRef #080.

                    ## Parameters

                    - `ACCOUNTID` (integer): The Hydrogen account to link.
                    - `ISSUER` (string): OIDC `iss` claim (byte-for-byte from ID token).
                    - `SUBJECT` (string): OIDC `sub` claim.
                    - `EMAIL` (string|null): Email from ID token at link time. May be NULL.
                    - `EMAILVERIFIED` (integer): 1 if IdP marked email verified, 0 if not.

                    ## Returns

                    - `identity_id` (integer): The newly created identity row's PK.
                      Returns zero rows if `account_id` does not exist.

                    ## Tables

                    - `${SCHEMA}account_oidc_identities`: OIDC identity link table (Phase 15).
                    - `${SCHEMA}accounts`: Read-only existence check (no SQL FK).

                    ## Notes

                    - `email_verified` is stored as INTEGER_SMALL (0/1), not BOOLEAN.
                    - `last_seen_at` is set to ${NOW} at link time; updated on each
                      subsequent sign-in via QueryRef #084.

                ]==]
                                                                                    AS summary,
                '{}'                                                                AS collection,
                ${COMMON_INSERT}
            FROM next_query_id;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Populate QueryRef #${QUERY_REF} - ${QUERY_NAME}'                   AS name,
        [=[
            # Forward Migration ${MIGRATION}: Populate QueryRef #${QUERY_REF} - ${QUERY_NAME}

            This migration creates the query for QueryRef #${QUERY_REF} - ${QUERY_NAME}
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
table.insert(queries,{sql=[[

    INSERT INTO ${SCHEMA}${QUERIES} (
        ${QUERIES_INSERT}
    )
    WITH next_query_id AS (
        SELECT COALESCE(MAX(query_id), 0) + 1 AS new_query_id
        FROM ${SCHEMA}${QUERIES}
    )
    SELECT
        new_query_id                                                        AS query_id,
        ${MIGRATION}                                                        AS query_ref,
        ${STATUS_ACTIVE}                                                    AS query_status_a27,
        ${TYPE_REVERSE_MIGRATION}                                           AS query_type_a28,
        ${DIALECT}                                                          AS query_dialect_a30,
        ${QTC_SLOW}                                                         AS query_queue_a58,
        ${TIMEOUT}                                                          AS query_timeout,
        [=[
            DELETE FROM ${SCHEMA}${TABLE}
            WHERE query_ref = ${QUERY_REF};

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Remove QueryRef #${QUERY_REF} - ${QUERY_NAME}'                     AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Remove QueryRef #${QUERY_REF} - ${QUERY_NAME}

            This is provided for completeness when testing the migration system
            to ensure that forward and reverse migrations are complete.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
return queries end
