-- Migration: acuranzo_1464.lua
-- Field item datasource lists (Reporting)
--
-- Lookup 053 keys 20, 21, 22, 23, 24, and 26 list Datasource (054 key 8)
-- before Field (key 7). Key 23 gains the static-memo list plus those two.
-- Key 26 gains the static-shape list, without Align, plus those two.
-- Reverse restores the applied lists. No new lookup. No caller-facing QueryRef.
-- QueryRef 175 stays unused.
--
-- luacheck: no max line length
-- luacheck: no unused args
--
-- CHANGELOG
-- 1.0.0 - 2026-11-05 - Field items list Datasource before Field
-- 1.0.1 - 2026-11-09 - Renumbered from acuranzo_1475.lua (Render split); content unchanged

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "lookups"
cfg.MIGRATION = "1464"

-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
table.insert(queries,{sql=[==[

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
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-text></fa>", "color": "Color-B", "group": "Text", "attributes": [0, 1, 2, 3, 4, 5, 8, 7, 14, 15, 16, 17, 29]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 53
              AND key_idx = 20;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-hashtag></fa>", "color": "Color-B", "group": "Number", "attributes": [0, 1, 2, 3, 4, 5, 8, 7, 14, 15, 16, 17, 29]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 53
              AND key_idx = 21;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-image></fa>", "color": "Color-B", "group": "Image", "attributes": [0, 1, 2, 3, 4, 5, 8, 7, 14, 15, 16, 17, 29]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 53
              AND key_idx = 22;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-memo></fa>", "color": "Color-B", "group": "Memo", "attributes": [0, 1, 2, 3, 4, 5, 8, 7, 14, 15, 16, 17, 29]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 53
              AND key_idx = 23;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-calendar-clock></fa>", "color": "Color-B", "group": "DateTime", "attributes": [0, 1, 2, 3, 4, 5, 8, 7, 14, 15, 16, 17, 29]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 53
              AND key_idx = 24;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-shapes></fa>", "color": "Color-B", "group": "Shape", "attributes": [0, 1, 2, 3, 8, 7, 14, 29, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 53
              AND key_idx = 26;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Field items list Datasource before Field'                          AS name,
        [=[
            # Forward Migration ${MIGRATION}: Field item datasource lists

            Lookup 053 keys 20, 21, 22, 23, 24, and 26 list attribute 8
            (Datasource) before 7 (Field).
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]==]})

-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
table.insert(queries,{sql=[==[

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
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-text></fa>", "color": "Color-B", "group": "Text", "attributes": [0, 1, 2, 3, 4, 5, 7, 8, 14, 15, 16, 17, 29]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 53
              AND key_idx = 20;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-hashtag></fa>", "color": "Color-B", "group": "Number", "attributes": [0, 1, 2, 3, 4, 5, 7, 8, 14, 15, 16, 17, 29]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 53
              AND key_idx = 21;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-image></fa>", "color": "Color-B", "group": "Image", "attributes": [0, 1, 2, 3, 4, 5, 7, 8, 14, 15, 16, 17, 29]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 53
              AND key_idx = 22;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-memo></fa>", "color": "Color-B", "group": "Memo", "attributes": [0, 1, 2, 3, 4, 14, 24, 29]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 53
              AND key_idx = 23;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-calendar-clock></fa>", "color": "Color-B", "group": "DateTime", "attributes": [0, 1, 2, 3, 4, 5, 7, 8, 14, 15, 16, 17, 29]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 53
              AND key_idx = 24;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-shapes></fa>", "color": "Color-B", "group": "Shape", "attributes": [0, 1, 2, 3, 4, 14, 24, 29]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 53
              AND key_idx = 26;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Restore field item attribute lists'                                AS name,
        [=[
            # Reverse Migration ${MIGRATION}: restore the field item lists

            Keys 20, 21, 22, and 24 return to the acuranzo_1442.lua lists.
            Keys 23 and 26 return to the acuranzo_1086.lua lists.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]==]})

return queries end
