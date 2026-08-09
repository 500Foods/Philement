-- Migration: acuranzo_1283.lua
-- Creates the account_canvas_links table (Band E/F Phase 23/30)

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.0.0 - 2026-08-08 - Initial creation for Phase 30 Canvas user create/link

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "account_canvas_links"
cfg.MIGRATION = "1283"
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
                link_id                 ${INTEGER}          NOT NULL,
                account_id              ${INTEGER}          NOT NULL,
                canvas_user_id          ${INTEGER}          NOT NULL,
                canvas_email            ${VARCHAR_500}              ,
                last_seen_at            ${TIMESTAMP_TZ}     NOT NULL,
                ${COMMON_CREATE}
                ${PRIMARY}(link_id),
                ${UNIQUE}(account_id),
                ${UNIQUE}(canvas_user_id)
            );

            ${SUBQUERY_DELIMITER}

            CREATE INDEX ${TABLE}_idx_email
                ON ${SCHEMA}${TABLE}(canvas_email);

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

            Durable Hydrogen ↔ Canvas identity link for Band E/F (Phase 23
            design, Phase 30 implementation). Mirrors
            `account_oidc_identities`: one row caches the Canvas `user_id`
            after the first successful `ensure_canvas_user` so later phases
            (progress sync Phase 60, renewals) do not re-query Canvas by
            email on every request.

            ## Schema

            - **link_id**: Surrogate primary key.
            - **account_id**: Foreign reference to ${SCHEMA}accounts (no SQL
              FK — same convention as account_oidc_identities / account_roles).
              UNIQUE 1:1 with the Hydrogen account.
            - **canvas_user_id**: Canvas LMS numeric user id. UNIQUE so two
              Hydrogen accounts cannot claim the same Canvas user.
            - **canvas_email**: Email used at link time (matches Canvas
              `login_attribute=email` and Hydrogen `match_email_only`).
            - **last_seen_at**: Updated on each successful Canvas ensure /
              Dashboard path (best-effort touch). Row birth time is the
              standard `${COMMON_CREATE}` `created_at` — do **not** add a
              second domain `created_at` (SQLite duplicate-column failure).

            ## Indexes

            - PRIMARY KEY on `link_id`.
            - UNIQUE on `account_id` — one Canvas user per Hydrogen account.
            - UNIQUE on `canvas_user_id` — one Hydrogen account per Canvas user.
            - INDEX on `canvas_email` — supports email-based recovery / admin
              lookup when the durable id is missing.

            ## Mapping contract (FL-23)

            v1 resolution is still email-match at Canvas; this table is the
            durable cache written once `ensure_canvas_user` succeeds. On
            `email_ambiguous` (multiple Canvas users share the email) the
            caller must fail closed and must not insert a row.

            ## Notes

            Additive migration. Existing `accounts` rows are not touched.
            QueryRefs that read/write this table land in later migrations
            (1286–1288).
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
            -- DROP INDEX ${TABLE}_idx_email;

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
                                "name": "link_id",
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
                                "name": "canvas_user_id",
                                "datatype": "${INTEGER}",
                                "nullable": false,
                                "primary_key": false,
                                "unique": true
                            },
                            {
                                "name": "canvas_email",
                                "datatype": "${VARCHAR_500}",
                                "nullable": true,
                                "primary_key": false,
                                "unique": false
                            },
                            {
                                "name": "last_seen_at",
                                "datatype": "${TIMESTAMP_TZ}",
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
