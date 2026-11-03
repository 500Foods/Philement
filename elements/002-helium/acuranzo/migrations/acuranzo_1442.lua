-- Migration: acuranzo_1442.lua
-- Lookup 054 Color editors and lookup 053 key 24 cleanup (Reporting, after Phase 14b)
--
-- Does not edit 1086, 1087, 1436, or 1441 in place. Sets "Editor" to
-- "Color" on lookup 054 keys 21 (Grid Color) and 24 (Grid Background
-- Color), matching key 25. Removes 054 key 24 from every lookup 053
-- item "attributes" list (key 24 is report/page only, per Andrew
-- 2026-11-02). No caller-facing QueryRef. Reverse restores the 1441
-- JSON on 054 keys 21 and 24 and the 1436 JSON on the 053 rows.

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.0.0 - 2026-11-03 - Initial creation (Reporting, 054 Color / 053 cleanup)

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "lookups"
cfg.MIGRATION = "1442"
cfg.LOOKUP_ID = "054"
cfg.LOOKUP_NAME = "Report Object Attributes"
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
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
            SET collection = ${JSON_INGEST_START}[===[{"Group": "Grid", "section": 8, "Label": "Color", "Editor": "Color"}]===]${JSON_INGEST_END}
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx = 21;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"Group": "Grid", "section": 8, "Label": "Background", "Editor": "Color"}]===]${JSON_INGEST_END}
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx = 24;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-text></fa>", "color": "Color-A", "group": "Text", "attributes": [0, 1, 2, 3, 4, 5, 14, 15, 16, 17, 29]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 053
              AND key_idx = 0;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-page-caret-down></fa>", "attributes": [0, 1, 2, 3, 4, 14, 29]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 053
              AND key_idx = 16;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-text></fa>", "color": "Color-B", "group": "Text", "attributes": [0, 1, 2, 3, 4, 5, 7, 8, 14, 15, 16, 17, 29]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 053
              AND key_idx = 20;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-hashtag></fa>", "color": "Color-B", "group": "Number", "attributes": [0, 1, 2, 3, 4, 5, 7, 8, 14, 15, 16, 17, 29]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 053
              AND key_idx = 21;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-image></fa>", "color": "Color-B", "group": "Image", "attributes": [0, 1, 2, 3, 4, 5, 7, 8, 14, 15, 16, 17, 29]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 053
              AND key_idx = 22;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-calendar-clock></fa>", "color": "Color-B", "group": "DateTime", "attributes": [0, 1, 2, 3, 4, 5, 7, 8, 14, 15, 16, 17, 29]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 053
              AND key_idx = 24;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-text></fa>", "color": "Color-C", "group": "Text", "attributes": [0, 1, 2, 3, 4, 5, 7, 8, 14, 15, 16, 17, 29]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 053
              AND key_idx = 28;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-hashtag></fa>", "color": "Color-C", "group": "Number", "attributes": [0, 1, 2, 3, 4, 5, 7, 8, 14, 15, 16, 17, 29]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 053
              AND key_idx = 29;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-calendar-clock></fa>", "color": "Color-C", "group": "DateTime", "attributes": [0, 1, 2, 3, 4, 5, 7, 8, 14, 15, 16, 17, 29]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 053
              AND key_idx = 32;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-text></fa>", "color": "Color-D", "group": "Text", "attributes": [0, 1, 2, 3, 4, 5, 7, 8, 14, 15, 16, 17, 29]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 053
              AND key_idx = 34;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-hashtag></fa>", "color": "Color-D", "group": "Number", "attributes": [0, 1, 2, 3, 4, 5, 7, 8, 14, 15, 16, 17, 29]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 053
              AND key_idx = 35;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-calendar-clock></fa>", "color": "Color-D", "group": "DateTime", "attributes": [0, 1, 2, 3, 4, 5, 7, 8, 14, 15, 16, 17, 29]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 053
              AND key_idx = 38;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-calendar-clock></fa>", "color": "Color-E", "group": "DateTime", "attributes": [0, 1, 2, 3, 4, 14, 29]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 053
              AND key_idx = 52;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-hashtag></fa>", "color": "Color-A", "group": "Number", "attributes": [0, 1, 2, 3, 4, 5, 14, 15, 16, 17, 29]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 053
              AND key_idx = 61;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-file-lines></fa>", "color": "Color-A", "group": "Number", "attributes": [0, 1, 2, 3, 4, 5, 14, 15, 16, 17, 29]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 053
              AND key_idx = 62;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-sliders></fa>", "color": "Color-A", "group": "Parameter", "attributes": [0, 1, 2, 3, 4, 5, 8, 14, 15, 16, 17, 29]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 053
              AND key_idx = 63;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Update Lookup ${LOOKUP_ID} Color editors and Lookup 053 attributes in ${TABLE} table'                                                            AS name,
        [=[
            # Forward Migration ${MIGRATION}: Lookup ${LOOKUP_ID} - ${LOOKUP_NAME} Color editors

            Sets Editor to Color on 054 keys 21 and 24, and removes 054 key 24
            from the lookup 053 item attribute lists.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
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
            SET collection = ${JSON_INGEST_START}[===[{"Group": "Grid", "section": 8, "Label": "Color", "Editor": "Text"}]===]${JSON_INGEST_END}
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx = 21;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"Group": "Grid", "section": 8, "Label": "Background", "Editor": "Text"}]===]${JSON_INGEST_END}
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx = 24;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-text></fa>", "color": "Color-A", "group": "Text", "attributes": [0, 1, 2, 3, 4, 5, 14, 15, 16, 17, 24, 29]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 053
              AND key_idx = 0;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-page-caret-down></fa>", "attributes": [0, 1, 2, 3, 4, 14, 24, 29]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 053
              AND key_idx = 16;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-text></fa>", "color": "Color-B", "group": "Text", "attributes": [0, 1, 2, 3, 4, 5, 7, 8, 14, 15, 16, 17, 24, 29]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 053
              AND key_idx = 20;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-hashtag></fa>", "color": "Color-B", "group": "Number", "attributes": [0, 1, 2, 3, 4, 5, 7, 8, 14, 15, 16, 17, 24, 29]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 053
              AND key_idx = 21;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-image></fa>", "color": "Color-B", "group": "Image", "attributes": [0, 1, 2, 3, 4, 5, 7, 8, 14, 15, 16, 17, 24, 29]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 053
              AND key_idx = 22;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-calendar-clock></fa>", "color": "Color-B", "group": "DateTime", "attributes": [0, 1, 2, 3, 4, 5, 7, 8, 14, 15, 16, 17, 24, 29]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 053
              AND key_idx = 24;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-text></fa>", "color": "Color-C", "group": "Text", "attributes": [0, 1, 2, 3, 4, 5, 7, 8, 14, 15, 16, 17, 24, 29]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 053
              AND key_idx = 28;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-hashtag></fa>", "color": "Color-C", "group": "Number", "attributes": [0, 1, 2, 3, 4, 5, 7, 8, 14, 15, 16, 17, 24, 29]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 053
              AND key_idx = 29;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-calendar-clock></fa>", "color": "Color-C", "group": "DateTime", "attributes": [0, 1, 2, 3, 4, 5, 7, 8, 14, 15, 16, 17, 24, 29]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 053
              AND key_idx = 32;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-text></fa>", "color": "Color-D", "group": "Text", "attributes": [0, 1, 2, 3, 4, 5, 7, 8, 14, 15, 16, 17, 24, 29]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 053
              AND key_idx = 34;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-hashtag></fa>", "color": "Color-D", "group": "Number", "attributes": [0, 1, 2, 3, 4, 5, 7, 8, 14, 15, 16, 17, 24, 29]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 053
              AND key_idx = 35;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-calendar-clock></fa>", "color": "Color-D", "group": "DateTime", "attributes": [0, 1, 2, 3, 4, 5, 7, 8, 14, 15, 16, 17, 24, 29]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 053
              AND key_idx = 38;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-calendar-clock></fa>", "color": "Color-E", "group": "DateTime", "attributes": [0, 1, 2, 3, 4, 14, 24, 29]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 053
              AND key_idx = 52;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-hashtag></fa>", "color": "Color-A", "group": "Number", "attributes": [0, 1, 2, 3, 4, 5, 14, 15, 16, 17, 24, 29]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 053
              AND key_idx = 61;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-file-lines></fa>", "color": "Color-A", "group": "Number", "attributes": [0, 1, 2, 3, 4, 5, 14, 15, 16, 17, 24, 29]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 053
              AND key_idx = 62;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-sliders></fa>", "color": "Color-A", "group": "Parameter", "attributes": [0, 1, 2, 3, 4, 5, 8, 14, 15, 16, 17, 24, 29]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 053
              AND key_idx = 63;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Undo Lookup ${LOOKUP_ID} Color editors and Lookup 053 attributes in ${TABLE} table'                                                            AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Undo Lookup ${LOOKUP_ID} Color editors

            Restores the 1441 JSON on 054 keys 21 and 24 and the 1436 JSON
            on the lookup 053 rows.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
return queries end
