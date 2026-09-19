-- Migration: acuranzo_1284.lua
-- Creates the user_registration_meta table (Band F Phase 29)

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.0.0 - 2026-08-08 - Initial creation for Phase 29 first-OIDC provision
-- 1.0.1 - 2026-08-08 - Align with account_oidc_identities conventions: drop
--                      redundant account_id secondary index (UNIQUE already
--                      covers it); fix column alignment; clarify QueryRef
--                      follow-ups use free refs starting at #143 (not #085 —
--                      Cap owns #085/#086).

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "user_registration_meta"
cfg.MIGRATION = "1284"
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
            CREATE TABLE ${SCHEMA}${TABLE}
            (
                meta_id                 ${INTEGER}          NOT NULL,
                account_id              ${INTEGER}          NOT NULL,
                currency                ${VARCHAR_20}               ,
                preferred_language      ${VARCHAR_50}               ,
                referral_source         ${VARCHAR_100}              ,
                learner_type            ${VARCHAR_100}              ,
                country                 ${VARCHAR_100}              ,
                age_band                ${VARCHAR_20}               ,
                ${COMMON_CREATE}
                ${PRIMARY}(meta_id),
                ${UNIQUE}(account_id)
            );

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Create ${TABLE} Table'                                             AS name,
        [=[
            # Forward Migration ${MIGRATION}: Create ${TABLE} Table

            This migration creates the ${TABLE} table for Band F new-user
            provisioning (Phase 29). It captures the registration-time
            attributes collected once at Keycloak signup (Phase 27/28) and
            seeded into Hydrogen on the first successful OIDC login.

            ## Schema

            - **meta_id**: Surrogate primary key.
            - **account_id**: Foreign reference to ${SCHEMA}accounts (no SQL
              FK constraint — same convention as account_roles /
              account_oidc_identities). One row per account (1:1 via UNIQUE).
            - **currency**: ISO-4217 currency code (e.g. CAD/USD/EUR/GBP).
              Required at Keycloak registration (Phase 28); provision path
              falls back to `CAD` if the claim is missing.
            - **preferred_language**: Locale string (e.g. `en`). Required at
              Keycloak registration; falls back to `en` if missing.
              Passthrough to Canvas/Keycloak locale only (not SPA text —
              see Band N).
            - **referral_source**: Optional ("how did you hear about us").
              NULL when the user skipped it at registration.
            - **learner_type**: Optional (student/teacher/content
              creator/life-long learner). NULL when skipped.
            - **country**: Optional free-text country. NULL when skipped.
            - **age_band**: Optional demographic band (under_18 / 18_24 /
              25_34 / 35_49 / 50_plus). NULL when skipped.
            - Provision-time stamp is the standard `COMMON_CREATE`
              `created_at` — do **not** add a second domain `created_at`
              (collides with COMMON; SQLite fails with duplicate column).

            ## Indexes

            - PRIMARY KEY on `meta_id`.
            - UNIQUE on `account_id` — one registration-meta row per account
              (also serves as the lookup index; no separate secondary index).

            ## Ownership (Phase 27 §8)

            This row is the Hydrogen/Helium-owned copy of the registration
            attributes. Keycloak's copy is a one-time seed only; it is never
            written back. Later edits flow through the Phase 53/54 PATCH
            endpoint (which writes this table), not through Keycloak.

            ## Notes

            Additive migration. Existing `accounts` rows are not touched.
            The provision path inserts exactly one row per account on first
            OIDC login (QueryRef #143 in acuranzo_1285); subsequent logins
            do not re-sync from Keycloak. VARCHAR_3 is not a dialect macro
            — currency uses VARCHAR_20 (sufficient for ISO-4217 codes).
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
            ${DROP_CHECK};

            ${SUBQUERY_DELIMITER}

            DROP TABLE ${SCHEMA}${TABLE};

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Drop ${TABLE} Table'                                               AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Drop ${TABLE} Table

            Provided for completeness when testing the migration system to
            ensure forward and reverse migrations are complete. Production
            rollback is documented in the FINISHLINE plan but not automated.
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
        ${TYPE_DIAGRAM_MIGRATION}                                           AS query_type_a28,
        ${DIALECT}                                                          AS query_dialect_a30,
        ${QTC_SLOW}                                                         AS query_queue_a58,
        ${TIMEOUT}                                                          AS query_timeout,
        'JSON Table Definition in collection'                               AS code,
        'Diagram Tables: ${SCHEMA}${TABLE}'                                 AS name,
        [=[
            # Diagram Migration ${MIGRATION}

            ## Diagram Tables: ${SCHEMA}${TABLE}

            This is the JSON Diagram code for the ${TABLE} table.
        ]=]
                                                                            AS summary,
                                                                            -- DIAGRAM_START
        ${JSON_INGEST_START}
        [=[
            {
                "diagram": [
                    {
                        "object_type": "table",
                        "object_id": "table.${TABLE}",
                        "object_ref": "${MIGRATION}",
                        "table": [
                            {
                                "name": "meta_id",
                                "datatype": "${INTEGER}",
                                "nullable": false,
                                "primary_key": true,
                                "unique": true
                            },
                            {
                                "name": "account_id",
                                "datatype": "${INTEGER}",
                                "nullable": false,
                                "primary_key": false,
                                "unique": true
                            },
                            {
                                "name": "currency",
                                "datatype": "${VARCHAR_20}",
                                "nullable": true,
                                "primary_key": false,
                                "unique": false
                            },
                            {
                                "name": "preferred_language",
                                "datatype": "${VARCHAR_50}",
                                "nullable": true,
                                "primary_key": false,
                                "unique": false
                            },
                            {
                                "name": "referral_source",
                                "datatype": "${VARCHAR_100}",
                                "nullable": true,
                                "primary_key": false,
                                "unique": false
                            },
                            {
                                "name": "learner_type",
                                "datatype": "${VARCHAR_100}",
                                "nullable": true,
                                "primary_key": false,
                                "unique": false
                            },
                            {
                                "name": "country",
                                "datatype": "${VARCHAR_100}",
                                "nullable": true,
                                "primary_key": false,
                                "unique": false
                            },
                            {
                                "name": "age_band",
                                "datatype": "${VARCHAR_20}",
                                "nullable": true,
                                "primary_key": false,
                                "unique": false
                            },
                            ${COMMON_DIAGRAM}
                        ]
                    }
                ]
            }
        ]=]
        ${JSON_INGEST_END}
                                                                            -- DIAGRAM_END
                                                                            AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
return queries end
