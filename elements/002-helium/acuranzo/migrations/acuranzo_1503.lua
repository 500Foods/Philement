-- Migration: acuranzo_1503.lua
-- Lookup 054 key 4 Label: Align → H. Align (Phase 78)
--
-- Reversible. QueryRef 175 unused. No Render module.
--
-- luacheck: no max line length
--
-- CHANGELOG
-- 1.0.0 - 2026-11-09 - Phase 78 H. Align label

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "lookups"
cfg.MIGRATION = "1503"

-- ----------------------------------------------------------------------------
-- Forward: rename Align → H. Align
-- ----------------------------------------------------------------------------
table.insert(queries,{sql=[====[

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
            SET value_txt = 'H. Align',
                collection = ${JSON_INGEST_START}[===[{"List": ["Left", "Center", "Right", "Justified"], "Group": "Formatting", "section": 7, "Label": "H. Align", "Editor": "List"}]===]${JSON_INGEST_END}
            WHERE lookup_id = 54
              AND key_idx = 4;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Rename Align to H. Align'                                        AS name,
        [=[
            # Forward Migration ${MIGRATION}: H. Align label

            Lookup 054 key 4 Label and value_txt become "H. Align"
            (pairs with client V. Align).
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]====]})

-- ----------------------------------------------------------------------------
-- Reverse: restore Align
-- ----------------------------------------------------------------------------
table.insert(queries,{sql=[====[

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
            SET value_txt = 'Align',
                collection = ${JSON_INGEST_START}[===[{"List": ["Left", "Center", "Right", "Justified"], "Group": "Formatting", "section": 7, "Label": "Align", "Editor": "List"}]===]${JSON_INGEST_END}
            WHERE lookup_id = 54
              AND key_idx = 4;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Restore Align label'                                             AS name,
        [=[
            # Reverse Migration ${MIGRATION}: restore Align label

            Puts lookup 054 key 4 Label back to "Align".
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]====]})

return queries end
