-- Migration: acuranzo_1405.lua
-- QueryRef #075 - Create Account Settings (fix VALUES parentheses)
--
-- acuranzo_1188.lua installed INSERT ... VALUES without a parenthesized
-- row list. PostgreSQL rejects that (`syntax error at or near …`).
-- Profile photo / settings sync calls 074 then 075; create always failed.
-- Rewrite as INSERT … SELECT with json_ingest, matching Courses inserts.

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.0.0 - 2026-10-27 - Fix Create Account Settings VALUES syntax

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "queries"
cfg.MIGRATION = "1405"
cfg.QUERY_REF = "075"
cfg.QUERY_NAME = "Create Account Settings"

-- Fixed SQL: INSERT … SELECT (portable) + jsonb ingest of the text payload
local sql_new = [[
                    INSERT INTO ${SCHEMA}account_settings (
                        account_id,
                        client_id,
                        collection,
                        valid_after,
                        valid_until,
                        created_id,
                        created_at,
                        updated_id,
                        updated_at
                    )
                    SELECT
                        :ACCOUNTID,
                        :CLIENTID,
                        ${SCHEMA}json_ingest(CAST(:COLLECTION AS ${TEXT})),
                        CAST('2025-01-01 00:00:00' AS ${TIMESTAMP_TZ}),
                        CAST('2035-01-01 00:00:00' AS ${TIMESTAMP_TZ}),
                        0,
                        ${NOW},
                        0,
                        ${NOW}]]

-- Previous SQL from acuranzo_1188.lua (intentionally broken VALUES list)
local sql_old = [[
                    INSERT INTO ${SCHEMA}account_settings (
                        account_id,
                        client_id,
                        collection,
                        ${COMMON_FIELDS}
                    )
                    VALUES
                        :ACCOUNTID,
                        :CLIENTID,
                        :COLLECTION,
                        '2025-01-01 00:00:00',
                        '2035-01-01 00:00:00',
                        0,
                        ${NOW},
                        0,
                        ${NOW}
                    );]]

-- ----------------------------------------------------------------------------
-- Forward: replace QueryRef #075 SQL
-- ----------------------------------------------------------------------------
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
            UPDATE ${SCHEMA}${QUERIES}
            SET code = [==[
]] .. sql_new .. [[
            ]==],
                name = '${QUERY_NAME}',
                summary = [==[
                    # Migration #${MIGRATION} - QueryRef #${QUERY_REF} - ${QUERY_NAME}

                    Fixes Create Account Settings. The 1188 SQL used
                    `VALUES` without a parenthesized row, which PostgreSQL
                    rejects. This rewrite uses `INSERT … SELECT` and
                    `${SCHEMA}json_ingest` for the collection payload.

                    ## Parameters

                    - ACCOUNTID (INTEGER)
                    - CLIENTID (TEXT)
                    - COLLECTION (TEXT JSON object)

                    ## Tables

                    - `${SCHEMA}account_settings`
                ]==]
            WHERE query_ref = ${QUERY_REF}
            AND query_type_a28 = ${TYPE_SQL};

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Fix VALUES syntax on QueryRef #${QUERY_REF} - ${QUERY_NAME}'       AS name,
        [=[
            # Forward Migration ${MIGRATION}: Fix QueryRef #${QUERY_REF}

            Replaces the SQL for QueryRef #${QUERY_REF} (Create Account
            Settings) so INSERT is valid on PostgreSQL. Profile settings
            and photo sync depend on this create path when no row exists
            yet for (account_id, client_id).
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})

-- ----------------------------------------------------------------------------
-- Reverse: restore acuranzo_1188.lua SQL
-- ----------------------------------------------------------------------------
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
            UPDATE ${SCHEMA}${QUERIES}
            SET code = [==[
]] .. sql_old .. [[
            ]==]
            WHERE query_ref = ${QUERY_REF}
            AND query_type_a28 = ${TYPE_SQL};

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Restore QueryRef #${QUERY_REF} - ${QUERY_NAME}'                    AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Restore QueryRef #${QUERY_REF}

            Puts the acuranzo_1188.lua SQL back for QueryRef #${QUERY_REF}.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})

return queries
end
