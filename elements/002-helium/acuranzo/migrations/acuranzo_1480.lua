-- Migration: acuranzo_1480.lua
-- Report property lists for shape fill and static items (Reporting)
--
-- Restores two lookup updates that rode on the Render migrations before the
-- 2026-11-09 split (old acuranzo_1467.lua and acuranzo_1468.lua).
-- Lookup 054 key 46 (Fill Style) lists none, solid, grid, dots, hatched,
-- and noise. Lookup 053 keys 1, 2, 3, and 4 use the static-text attribute
-- list. Collections are JSON objects.
-- Reverse puts back the acuranzo_1460.lua Fill Style list (none, solid)
-- and the acuranzo_1086.lua attribute lists.
-- No new lookup id. No new lookup key. QueryRef 175 stays unused.
--
-- luacheck: no max line length
-- luacheck: no unused args
--
-- CHANGELOG
-- 1.0.0 - 2026-11-09 - Lookup updates from old acuranzo_1467.lua and acuranzo_1468.lua

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "lookups"
cfg.MIGRATION = "1480"

-- ----------------------------------------------------------------------------
-- Forward: Fill Style list and static-text attribute lists
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
            UPDATE ${SCHEMA}lookups
            SET collection = ${JSON_INGEST_START}[===[{"Group":"Fill","section":11,"Label":"Fill Style","Editor":"List","List":["none","solid","grid","dots","hatched","noise"]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 54
              AND key_idx = 46;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}lookups
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-hashtag></fa>", "color": "Color-A", "group": "Number", "attributes": [0, 1, 2, 3, 4, 5, 14, 15, 16, 17, 29]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 053
              AND key_idx = 1;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}lookups
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-image></fa>", "color": "Color-A", "group": "Image", "attributes": [0, 1, 2, 3, 4, 5, 14, 15, 16, 17, 29]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 053
              AND key_idx = 2;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}lookups
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-memo></fa>", "color": "Color-A", "group": "Memo", "attributes": [0, 1, 2, 3, 4, 5, 14, 15, 16, 17, 29]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 053
              AND key_idx = 3;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}lookups
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-calendar-clock></fa>", "color": "Color-A", "group": "DateTime", "attributes": [0, 1, 2, 3, 4, 5, 14, 15, 16, 17, 29]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 053
              AND key_idx = 4;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Fill Style list and static-text attributes'                        AS name,
        [=[
            # Forward Migration ${MIGRATION}: report property lists

            Lookup 054 key 46 (Fill Style) lists none, solid, grid, dots,
            hatched, and noise. Lookup 053 keys 1, 2, 3, and 4 use the
            static-text attribute list.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]====]})

-- ----------------------------------------------------------------------------
-- Reverse: none, solid Fill Style list and the 1086 attribute lists
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
            UPDATE ${SCHEMA}lookups
            SET collection = ${JSON_INGEST_START}[===[{"Group":"Fill","section":11,"Label":"Fill Style","Editor":"List","List":["none","solid"]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 54
              AND key_idx = 46;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}lookups
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-hashtag></fa>", "color": "Color-A", "group": "Number", "attributes": [0, 1, 2, 3, 4, 14, 24, 29]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 053
              AND key_idx = 1;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}lookups
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-image></fa>", "color": "Color-A", "group": "Image", "attributes": [0, 1, 2, 3, 4, 14, 24, 29]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 053
              AND key_idx = 2;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}lookups
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-memo></fa>", "color": "Color-A", "group": "Memo", "attributes": [0, 1, 2, 3, 4, 14, 24, 29]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 053
              AND key_idx = 3;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}lookups
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-calendar-clock></fa>", "color": "Color-A", "group": "DateTime", "attributes": [0, 1, 2, 3, 4, 14, 24, 29]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 053
              AND key_idx = 4;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Restore Fill Style and 1086 attribute lists'                       AS name,
        [=[
            # Reverse Migration ${MIGRATION}: restore report property lists

            Lookup 054 key 46 lists none and solid again. Lookup 053 keys
            1, 2, 3, and 4 list the 1086 attributes again, including key 24.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]====]})

return queries end
