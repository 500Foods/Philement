-- Migration: acuranzo_1507.lua
-- scripts.options JSON (sandbox + limits) for Hydrogen scripting
--
-- Phase 79 follow-on: per-script sandbox opt-in and resource limits.
-- Default '{}'. Reverse drops the column.
--
-- luacheck: no max line length
-- luacheck: no unused args
--
-- CHANGELOG
-- 1.0.0 - 2026-11-09 - ADD scripts.options JSON NOT NULL DEFAULT '{}'

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "scripts"
cfg.MIGRATION = "1507"

-- ----------------------------------------------------------------------------
-- Forward: ADD options
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
            ALTER TABLE ${SCHEMA}${TABLE}
                ADD COLUMN options ${JSON} NOT NULL DEFAULT '{}';

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Add scripts.options JSON'                                          AS name,
        [=[
            # Forward Migration ${MIGRATION}: scripts.options

            Adds `options` (${JSON}, NOT NULL, DEFAULT '{}') to `scripts`.

            Shape (absent keys = deny / use global defaults):

            ```json
            {
              "sandbox": {
                "io": true,
                "osExecute": false,
                "debug": false,
                "loadlib": false
              },
              "limits": {
                "memorySoftKB": 32768,
                "memoryHardKB": 65536,
                "maxRuntimeSeconds": 3600,
                "instructionHookInterval": 5000
              }
            }
            ```

            Sandbox flags are opt-in when the global Scripting.Sandbox.*
            config is false. Limits feed ScoreboardJobLimits at submit.
            No diagram (column add; same as 1297 / 1365).
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})

-- ----------------------------------------------------------------------------
-- Reverse: DROP options
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
            ALTER TABLE ${SCHEMA}${TABLE}
                DROP COLUMN options;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Drop scripts.options'                                              AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Drop scripts.options

            Removes the `options` column from `scripts`.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})

return queries end
