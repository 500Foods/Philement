-- Migration: acuranzo_1436.lua
-- Lookup 053 (Report Object Types) repair for Reporting Phase 11
--
-- Does not edit 1086 in place. Fixes the "Iion" key on keys 7 and 16,
-- closes the icon tag on key 52, inserts keys 61 Page Number,
-- 62 Page Count, and 63 Parameter, and rewrites the v1 attribute lists
-- (lookup 054 ids) so Phase 14 can edit position (0-5, 14), font
-- (15-17), field (7) and datasource (8). Static text keeps its text on
-- the item: no field or datasource ids. Lookup 054 keys 9-12 stay off.
-- Chart and barcode rows are untouched. No caller-facing QueryRef.
-- Reverse restores the 1086 JSON and deletes only keys 61-63.

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.0.0 - 2026-11-02 - Initial creation (Reporting Phase 11)

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "lookups"
cfg.MIGRATION = "1436"
cfg.LOOKUP_ID = "053"
cfg.LOOKUP_NAME = "Report Object Types"
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
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-text></fa>", "color": "Color-A", "group": "Text", "attributes": [0, 1, 2, 3, 4, 5, 14, 15, 16, 17, 24, 29]}]===]${JSON_INGEST_END}
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx = 0;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-server></fa>"}]===]${JSON_INGEST_END}
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx = 7;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-page-caret-down></fa>", "attributes": [0, 1, 2, 3, 4, 14, 24, 29]}]===]${JSON_INGEST_END}
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx = 16;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-text></fa>", "color": "Color-B", "group": "Text", "attributes": [0, 1, 2, 3, 4, 5, 7, 8, 14, 15, 16, 17, 24, 29]}]===]${JSON_INGEST_END}
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx = 20;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-hashtag></fa>", "color": "Color-B", "group": "Number", "attributes": [0, 1, 2, 3, 4, 5, 7, 8, 14, 15, 16, 17, 24, 29]}]===]${JSON_INGEST_END}
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx = 21;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-image></fa>", "color": "Color-B", "group": "Image", "attributes": [0, 1, 2, 3, 4, 5, 7, 8, 14, 15, 16, 17, 24, 29]}]===]${JSON_INGEST_END}
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx = 22;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-calendar-clock></fa>", "color": "Color-B", "group": "DateTime", "attributes": [0, 1, 2, 3, 4, 5, 7, 8, 14, 15, 16, 17, 24, 29]}]===]${JSON_INGEST_END}
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx = 24;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-text></fa>", "color": "Color-C", "group": "Text", "attributes": [0, 1, 2, 3, 4, 5, 7, 8, 14, 15, 16, 17, 24, 29]}]===]${JSON_INGEST_END}
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx = 28;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-hashtag></fa>", "color": "Color-C", "group": "Number", "attributes": [0, 1, 2, 3, 4, 5, 7, 8, 14, 15, 16, 17, 24, 29]}]===]${JSON_INGEST_END}
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx = 29;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-calendar-clock></fa>", "color": "Color-C", "group": "DateTime", "attributes": [0, 1, 2, 3, 4, 5, 7, 8, 14, 15, 16, 17, 24, 29]}]===]${JSON_INGEST_END}
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx = 32;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-text></fa>", "color": "Color-D", "group": "Text", "attributes": [0, 1, 2, 3, 4, 5, 7, 8, 14, 15, 16, 17, 24, 29]}]===]${JSON_INGEST_END}
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx = 34;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-hashtag></fa>", "color": "Color-D", "group": "Number", "attributes": [0, 1, 2, 3, 4, 5, 7, 8, 14, 15, 16, 17, 24, 29]}]===]${JSON_INGEST_END}
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx = 35;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-calendar-clock></fa>", "color": "Color-D", "group": "DateTime", "attributes": [0, 1, 2, 3, 4, 5, 7, 8, 14, 15, 16, 17, 24, 29]}]===]${JSON_INGEST_END}
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx = 38;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-calendar-clock></fa>", "color": "Color-E", "group": "DateTime", "attributes": [0, 1, 2, 3, 4, 14, 24, 29]}]===]${JSON_INGEST_END}
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx = 52;

            ${SUBQUERY_DELIMITER}
            INSERT INTO ${SCHEMA}${TABLE}
                (lookup_id, key_idx, status_a1, value_txt, value_int, sort_seq, code, summary, collection, ${COMMON_FIELDS})
            VALUES
                (${LOOKUP_ID}, 61, 1, 'Page Number', 7, 4, '', '', ${JSON_INGEST_START}[==[{"icon": "<fa fa-hashtag></fa>", "color": "Color-A", "group": "Number", "attributes": [0, 1, 2, 3, 4, 5, 14, 15, 16, 17, 24, 29]}]==]${JSON_INGEST_END}, ${COMMON_VALUES}),
                (${LOOKUP_ID}, 62, 1, 'Page Count', 7, 5, '', '', ${JSON_INGEST_START}[==[{"icon": "<fa fa-file-lines></fa>", "color": "Color-A", "group": "Number", "attributes": [0, 1, 2, 3, 4, 5, 14, 15, 16, 17, 24, 29]}]==]${JSON_INGEST_END}, ${COMMON_VALUES}),
                (${LOOKUP_ID}, 63, 1, 'Parameter', 1, 9, '', '', ${JSON_INGEST_START}[==[{"icon": "<fa fa-sliders></fa>", "color": "Color-A", "group": "Parameter", "attributes": [0, 1, 2, 3, 4, 5, 8, 14, 15, 16, 17, 24, 29]}]==]${JSON_INGEST_END}, ${COMMON_VALUES});

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Repair Lookup ${LOOKUP_ID} in ${TABLE} table'                                                            AS name,
        [=[
            # Forward Migration ${MIGRATION}: Repair Lookup ${LOOKUP_ID} - ${LOOKUP_NAME}

            Fixes icon keys 7 and 16 and the key 52 icon tag, inserts keys 61-63,
            and updates the v1 attribute lists for the Phase 14 property panel.
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
            DELETE FROM ${SCHEMA}${TABLE}
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx IN (61, 62, 63);

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-text></fa>", "color": "Color-A", "group": "Text", "attributes": [0, 1, 2, 3, 4, 14, 24, 29]}]===]${JSON_INGEST_END}
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx = 0;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"Iion": "<fa fa-server></fa>"}]===]${JSON_INGEST_END}
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx = 7;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"Iion": "<fa fa-page-caret-down></fa>", "attributes": [0, 1, 2, 3, 4, 14, 24, 29]}]===]${JSON_INGEST_END}
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx = 16;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-text></fa>", "color": "Color-B", "group": "Text", "attributes": [0, 1, 2, 3, 4, 14, 24, 29]}]===]${JSON_INGEST_END}
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx = 20;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-hashtag></fa>", "color": "Color-B", "group": "Number", "attributes": [0, 1, 2, 3, 4, 14, 24, 29]}]===]${JSON_INGEST_END}
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx = 21;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-image></fa>", "color": "Color-B", "group": "Image", "attributes": [0, 1, 2, 3, 4, 14, 24, 29]}]===]${JSON_INGEST_END}
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx = 22;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-calendar-clock></fa>", "color": "Color-B", "group": "DateTime", "attributes": [0, 1, 2, 3, 4, 14, 24, 29]}]===]${JSON_INGEST_END}
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx = 24;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-text></fa>", "color": "Color-C", "group": "Text", "attributes": [0, 1, 2, 3, 4, 14, 24, 29]}]===]${JSON_INGEST_END}
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx = 28;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-hashtag></fa>", "color": "Color-C", "group": "Number", "attributes": [0, 1, 2, 3, 4, 14, 24, 29]}]===]${JSON_INGEST_END}
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx = 29;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-calendar-clock></fa>", "color": "Color-C", "group": "DateTime", "attributes": [0, 1, 2, 3, 4, 14, 24, 29]}]===]${JSON_INGEST_END}
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx = 32;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-text></fa>", "color": "Color-D", "group": "Text", "attributes": [0, 1, 2, 3, 4, 14, 24, 29]}]===]${JSON_INGEST_END}
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx = 34;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-hashtag></fa>", "color": "Color-D", "group": "Number", "attributes": [0, 1, 2, 3, 4, 14, 24, 29]}]===]${JSON_INGEST_END}
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx = 35;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-calendar-clock></fa>", "color": "Color-D", "group": "DateTime", "attributes": [0, 1, 2, 3, 4, 14, 24, 29]}]===]${JSON_INGEST_END}
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx = 38;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-calendar-clock</fa>", "color": "Color-E", "group": "DateTime", "attributes": [0, 1, 2, 3, 4, 14, 24, 29]}]===]${JSON_INGEST_END}
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx = 52;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Undo Lookup ${LOOKUP_ID} repair in ${TABLE} table'                                                            AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Undo Lookup ${LOOKUP_ID} repair

            Deletes keys 61-63 and restores the acuranzo_1086 JSON on the updated keys.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
return queries end
