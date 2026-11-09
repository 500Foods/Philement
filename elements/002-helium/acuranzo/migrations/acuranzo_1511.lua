-- Migration: acuranzo_1511.lua
-- Reports.Render: enable sandbox.io via scripts.options
--
-- Phase 79: Render.Metrics needs filesystem fonts (fontkit openSync).
-- Modules load into the Reports.Render Lua state, so the flag is on
-- the entry script. Reverse clears options back to '{}'.
--
-- luacheck: no max line length
-- luacheck: no unused args
--
-- CHANGELOG
-- 1.0.0 - 2026-11-09 - SET Reports.Render options sandbox.io

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "scripts"
cfg.MIGRATION = "1511"
cfg.GROUP_NAME = "Reports"
-- No cfg.SCRIPT_NAME: this is options-only, not a Render body packet.

-- ----------------------------------------------------------------------------
-- Forward: set options
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
            UPDATE ${SCHEMA}${TABLE}
               SET options = ${JSON_INGEST_START}[===[{"sandbox":{"io":true}}]===]${JSON_INGEST_END}
             WHERE group_name = '${GROUP_NAME}'
               AND script_name = 'Render';

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Enable sandbox.io on Reports.Render'                               AS name,
        [=[
            # Forward Migration ${MIGRATION}: Reports.Render options

            Sets `scripts.options` to `{"sandbox":{"io":true}}` for
            `Reports.Render` so Render.Metrics can open font files under
            the configured font root (global AllowIo stays false).
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})

-- ----------------------------------------------------------------------------
-- Reverse: clear options
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
            UPDATE ${SCHEMA}${TABLE}
               SET options = ${JSON_INGEST_START}[===[{}]===]${JSON_INGEST_END}
             WHERE group_name = '${GROUP_NAME}'
               AND script_name = 'Render';

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Clear Reports.Render options'                                      AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Clear Reports.Render options

            Sets `options` back to `{}` for Reports.Render.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})

return queries end
