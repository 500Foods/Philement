-- Migration: acuranzo_1301.lua
-- Creates the user_preferences table (Band L Phase 52)
--
-- Notification flags only. Identity/demographics stay on
-- user_registration_meta (acuranzo_1284). Design: FINISHLINE FL-51.

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.0.0 - 2026-08-12 - Create user_preferences (Phase 52 / FL-51)

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "user_preferences"
cfg.MIGRATION = "1301"
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
                pref_id                 ${INTEGER}          NOT NULL,
                account_id              ${INTEGER}          NOT NULL,
                notify_new_course       ${INTEGER_SMALL}    NOT NULL DEFAULT 0,
                notify_expiring         ${INTEGER_SMALL}    NOT NULL DEFAULT 1,
                notify_expired          ${INTEGER_SMALL}    NOT NULL DEFAULT 1,
                notify_weekly_summary   ${INTEGER_SMALL}    NOT NULL DEFAULT 1,
                ${COMMON_CREATE}
                ${PRIMARY}(pref_id),
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

            Band L Phase 52 — Reception notification preferences (FL-51).

            This table is **notification behavior only**. Do not add
            `currency`, `preferred_language`, or any Phase 27 registration
            attribute here — those live on `user_registration_meta`
            (acuranzo_1284) and stay there when Settings PATCHes them.

            ## Schema

            - **pref_id**: Surrogate primary key.
            - **account_id**: Foreign reference to ${SCHEMA}accounts (no SQL
              FK — same convention as user_registration_meta /
              account_oidc_identities). One row per account (1:1 via UNIQUE).
            - **notify_new_course**: Marketing / catalog announce mail.
              Default **0** (opt-in). CourseBuilder CB-29 Announce.
            - **notify_expiring**: Transactional mail N=7 days before
              enrollment expiry. Default **1** (opt-out). Phase 56 job.
            - **notify_expired**: Transactional mail when enrollment expires.
              Default **1**. Phase 56 job.
            - **notify_weekly_summary**: Weekly digest of active enrollments
              + expiration dates. Default **1**. Pref stored now; send job
              is not in Band L.

            Booleans are `INTEGER_SMALL` 0/1 (same as `courses.is_featured`).
            `COMMON_CREATE` supplies `created_at` / `updated_at` — do
            **not** add a second domain `created_at` (FL-29c).

            ## Defaults for a freshly provisioned account

            No provision-time INSERT and no trigger. QueryRef #143 /
            `oidc_rp_link_provision.c` are unchanged. A new account has
            **no row**. Phase 53 read path LEFT JOIN + COALESCE to the
            column defaults above. First PATCH INSERTs (own JWT
            `account_id` only).

            ## Indexes

            - PRIMARY KEY on `pref_id`.
            - UNIQUE on `account_id` — also the lookup index; no secondary
              account index (same as 1284).

            Additive migration. Existing `accounts` rows are not touched.
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
                                "name": "pref_id",
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
                                "name": "notify_new_course",
                                "datatype": "${INTEGER_SMALL}",
                                "nullable": false,
                                "primary_key": false,
                                "unique": false
                            },
                            {
                                "name": "notify_expiring",
                                "datatype": "${INTEGER_SMALL}",
                                "nullable": false,
                                "primary_key": false,
                                "unique": false
                            },
                            {
                                "name": "notify_expired",
                                "datatype": "${INTEGER_SMALL}",
                                "nullable": false,
                                "primary_key": false,
                                "unique": false
                            },
                            {
                                "name": "notify_weekly_summary",
                                "datatype": "${INTEGER_SMALL}",
                                "nullable": false,
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
