-- Migration: acuranzo_1285.lua
-- QueryRef #143 - OIDC RP: Insert User Registration Meta (Phase 29)

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.0.0 - 2026-08-08 - Initial creation for Phase 29 first-OIDC provision

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "queries"
cfg.MIGRATION = "1285"
cfg.QUERY_REF = "143"
cfg.QUERY_NAME = "OIDC RP: Insert User Registration Meta"
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
                    ${INSERT_KEY_START} meta_id ${INSERT_KEY_END}
                        INSERT INTO ${SCHEMA}user_registration_meta (
                            meta_id,
                            account_id,
                            currency,
                            preferred_language,
                            referral_source,
                            learner_type,
                            country,
                            age_band,
                            ${COMMON_FIELDS}
                        )
                        WITH
                            next_meta_id AS (
                                SELECT COALESCE(MAX(meta_id), 0) + 1 AS new_meta_id
                                FROM ${SCHEMA}user_registration_meta
                            ),
                            account_check AS (
                                SELECT account_id
                                FROM ${SCHEMA}accounts
                                WHERE account_id = :ACCOUNTID
                                  AND NOT EXISTS (
                                      SELECT 1
                                      FROM ${SCHEMA}user_registration_meta urm
                                      WHERE urm.account_id = :ACCOUNTID
                                  )
                            )
                        SELECT
                            next_meta_id.new_meta_id,
                            account_check.account_id,
                            :CURRENCY,
                            :PREFERREDLANGUAGE,
                            :REFERRALSOURCE,
                            :LEARNERTYPE,
                            :COUNTRY,
                            :AGEBAND,
                            '2025-01-01 00:00:00',
                            '2035-01-01 00:00:00',
                            0,
                            ${NOW},
                            0,
                            ${NOW}
                        FROM next_meta_id, account_check
                    ${INSERT_KEY_RETURN} meta_id
                    ;
                ]==]                                                                AS code,
                '${QUERY_NAME}'                                                     AS name,
                [==[
                    #  QueryRef #${QUERY_REF} - ${QUERY_NAME}

                    Writes the one-time `user_registration_meta` row for a newly
                    provisioned Hydrogen account (Phase 29). Called from the OIDC
                    RP linker immediately after QueryRef #083/#081 on first
                    provision. Subsequent logins must NOT re-sync from Keycloak
                    (Phase 27 §8 ownership); this query is therefore idempotent:
                    if a row already exists for `account_id`, the INSERT returns
                    zero rows (WHERE NOT EXISTS) rather than raising UNIQUE.

                    ## Parameters

                    - `ACCOUNTID` (integer): Hydrogen account PK from #083.
                    - `CURRENCY` (string): ISO-4217 code from IdP claim; caller
                      falls back to `CAD` when the claim is missing (Phase 29 §5).
                    - `PREFERREDLANGUAGE` (string): Locale from IdP claim; caller
                      falls back to `en` when missing.
                    - `REFERRALSOURCE` (string|null): Optional claim; NULL if skipped.
                    - `LEARNERTYPE` (string|null): Optional claim; NULL if skipped.
                    - `COUNTRY` (string|null): Optional claim; NULL if skipped.
                    - `AGEBAND` (string|null): Optional claim (`under_18` /
                      `18_24` / `25_34` / `35_49` / `50_plus`); NULL if skipped.

                    ## Returns

                    - `meta_id` (integer): PK of the new row.
                      Zero rows if `account_id` does not exist or a meta row
                      already exists for that account.

                    ## Tables

                    - `${SCHEMA}user_registration_meta` (acuranzo_1284).
                    - `${SCHEMA}accounts`: existence check only (no SQL FK).

                    ## Notes

                    - QueryRef number is **#143** (next free after #142). Do not
                      reuse #085/#086 — those are Cap-protected form inserts.
                    - Caller owns claim parsing + CAD/en fallbacks; this query
                      stores whatever strings it is given.
                    - UNIQUE(`account_id`) is the durable 1:1 guard; the
                      NOT EXISTS clause avoids noisy constraint errors on
                      retries after a partial first-login path.

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
