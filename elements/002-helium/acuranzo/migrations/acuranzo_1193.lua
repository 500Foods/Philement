-- Migration: acuranzo_1193.lua
-- QueryRef #082 - OIDC RP: Lookup Account by Email (Phase 17)

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.1.0 - 2026-07-09 - Use <=/>= for valid_after/valid_until so same-second rows match on SQLite second-resolution timestamps
-- 1.0.0 - 2026-05-09 - Initial creation for OIDC Phase 17

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "queries"
cfg.MIGRATION = "1193"
cfg.QUERY_REF = "082"
cfg.QUERY_NAME = "OIDC RP: Lookup Account by Email"
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
                    SELECT
                        DISTINCT ac.account_id
                    FROM
                        ${SCHEMA}account_contacts ac
                    WHERE
                        (LOWER(ac.contact) = LOWER(:EMAIL))
                        AND (ac.contact_type_a18 = 0)
                        AND (
                            (ac.valid_after IS NULL)
                            OR (ac.valid_after <= ${NOW})
                        )
                        AND (
                            (ac.valid_until IS NULL)
                            OR (ac.valid_until >= ${NOW})
                        )
                ]==]                                                                AS code,
                '${QUERY_NAME}'                                                     AS name,
                [==[
                    #  QueryRef #${QUERY_REF} - ${QUERY_NAME}

                    Looks up the account ID(s) associated with a given email address via the
                    `account_contacts` table. Used by the OIDC RP account linker for the
                    `match_email_only` and `match_email_then_provision` strategies when a
                    `(issuer, subject)` lookup (QueryRef #080) returns no rows.

                    The caller MUST only call this query when the ID token's `email_verified`
                    claim is true (`RequireEmailVerified = true` in config). Linking by an
                    unverified email would allow an attacker to impersonate any account by
                    registering with a third-party IdP using that email address.

                    ## Parameters

                    - `EMAIL` (string): The email address from the validated ID token's
                      `email` claim. Case-insensitive comparison via LOWER().

                    ## Returns

                    Returns zero, one, or more rows:
                    - `account_id` (integer): A Hydrogen account that has the given email
                      as a contact (`contact_type_a18 = 0` means email contact).

                    Zero rows: no account with this email → proceed to provisioning or fail.
                    One row: unambiguous match → link it via QueryRef #081 and proceed.
                    Two or more rows: ambiguous (two accounts share the same email) → the
                    caller must reject with `email_ambiguous` and log for operator action.

                    ## Tables

                    - `${SCHEMA}account_contacts`: Contact lookup table; `contact_type_a18 = 0`
                      identifies email-type contacts (same as QueryRef #011 / QueryRef #008).

                    ## Notes

                    - Only considers currently-valid contacts (valid_after/valid_until check).
                    - Does NOT check `accounts.status_a16` — that check happens later when
                      minting the JWT (same as the password login path).
                    - The caller must handle the "two or more rows" case explicitly.

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
