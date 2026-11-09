-- Migration: acuranzo_1512.lua
-- Lookup Overflow Fit + Min/Max Font Size (Reporting Phase 80)
--
-- Extends 054 Overflow List with Fit; adds 054 keys 59/60.
-- Wires Min/Max onto every 053 kind that already has Overflow (58).
-- Reversible. No Render module.
--
-- luacheck: no max line length
--
-- CHANGELOG
-- 1.0.0 - 2026-11-09 - Phase 80 Overflow Fit, Min/Max Font Size

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "lookups"
cfg.MIGRATION = "1512"

-- ----------------------------------------------------------------------------
-- Forward
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
            SET collection = ${JSON_INGEST_START}[===[{"List": ["Clip", "Ellipsis", "Shrink", "Grow", "Fit"], "Group": "Formatting", "section": 7, "Label": "Overflow", "Editor": "List"}]===]${JSON_INGEST_END}
            WHERE lookup_id = 54
              AND key_idx = 58;

            ${SUBQUERY_DELIMITER}

            INSERT INTO ${SCHEMA}${TABLE}
                (lookup_id, key_idx, status_a1, value_txt, value_int, sort_seq, code, summary, collection, ${COMMON_FIELDS})
            VALUES
                (54, 59, 1, 'Min Font Size', 1, 236, '', '', ${JSON_INGEST_START}[===[{"Group": "Formatting", "section": 7, "Label": "Min Font Size", "Editor": "Number"}]===]${JSON_INGEST_END}, ${COMMON_VALUES});

            ${SUBQUERY_DELIMITER}

            INSERT INTO ${SCHEMA}${TABLE}
                (lookup_id, key_idx, status_a1, value_txt, value_int, sort_seq, code, summary, collection, ${COMMON_FIELDS})
            VALUES
                (54, 60, 1, 'Max Font Size', 1, 237, '', '', ${JSON_INGEST_START}[===[{"Group": "Formatting", "section": 7, "Label": "Max Font Size", "Editor": "Number"}]===]${JSON_INGEST_END}, ${COMMON_VALUES});

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-text></fa>", "color": "Color-A", "group": "Text", "attributes": [0, 1, 2, 3, 4, 14, 15, 16, 17, 29, 55, 56, 57, 58, 59, 60]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 53
              AND key_idx = 0;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-hashtag></fa>", "color": "Color-A", "group": "Number", "attributes": [0, 1, 2, 3, 4, 5, 14, 15, 16, 17, 29, 57, 55, 56, 58, 59, 60]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 53
              AND key_idx = 1;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-memo></fa>", "color": "Color-A", "group": "Memo", "attributes": [0, 1, 2, 3, 4, 5, 14, 15, 16, 17, 29, 57, 55, 56, 58, 59, 60]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 53
              AND key_idx = 3;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-calendar-clock></fa>", "color": "Color-A", "group": "DateTime", "attributes": [0, 1, 2, 3, 4, 5, 14, 15, 16, 17, 29, 57, 55, 56, 58, 59, 60]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 53
              AND key_idx = 4;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-text></fa>", "color": "Color-B", "group": "Text", "attributes": [0, 1, 2, 3, 4, 5, 8, 7, 14, 15, 16, 17, 29, 57, 55, 56, 58, 59, 60]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 53
              AND key_idx = 20;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-hashtag></fa>", "color": "Color-B", "group": "Number", "attributes": [0, 1, 2, 3, 4, 5, 8, 7, 14, 15, 16, 17, 29, 57, 55, 56, 58, 59, 60]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 53
              AND key_idx = 21;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-memo></fa>", "color": "Color-B", "group": "Memo", "attributes": [0, 1, 2, 3, 4, 5, 8, 7, 14, 15, 16, 17, 29, 57, 55, 56, 58, 59, 60]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 53
              AND key_idx = 23;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-calendar-clock></fa>", "color": "Color-B", "group": "DateTime", "attributes": [0, 1, 2, 3, 4, 5, 8, 7, 14, 15, 16, 17, 29, 57, 55, 56, 58, 59, 60]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 53
              AND key_idx = 24;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-text></fa>", "color": "Color-C", "group": "Text", "attributes": [0, 1, 2, 3, 4, 5, 7, 8, 14, 15, 16, 17, 29, 57, 55, 56, 58, 59, 60]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 53
              AND key_idx = 28;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-hashtag></fa>", "color": "Color-C", "group": "Number", "attributes": [0, 1, 2, 3, 4, 5, 7, 8, 14, 15, 16, 17, 29, 57, 55, 56, 58, 59, 60]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 53
              AND key_idx = 29;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-memo></fa>", "color": "Color-C", "group": "Memo", "attributes": [0, 1, 2, 3, 4, 14, 24, 29, 57, 55, 56, 58, 59, 60]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 53
              AND key_idx = 31;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-calendar-clock></fa>", "color": "Color-C", "group": "DateTime", "attributes": [0, 1, 2, 3, 4, 5, 7, 8, 14, 15, 16, 17, 29, 57, 55, 56, 58, 59, 60]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 53
              AND key_idx = 32;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-text></fa>", "color": "Color-D", "group": "Text", "attributes": [0, 1, 2, 3, 4, 5, 7, 8, 14, 15, 16, 17, 29, 57, 55, 56, 58, 59, 60]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 53
              AND key_idx = 34;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-hashtag></fa>", "color": "Color-D", "group": "Number", "attributes": [0, 1, 2, 3, 4, 5, 7, 8, 14, 15, 16, 17, 29, 57, 55, 56, 58, 59, 60]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 53
              AND key_idx = 35;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-memo></fa>", "color": "Color-D", "group": "Memo", "attributes": [0, 1, 2, 3, 4, 14, 24, 29, 57, 55, 56, 58, 59, 60]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 53
              AND key_idx = 37;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-calendar-clock></fa>", "color": "Color-D", "group": "DateTime", "attributes": [0, 1, 2, 3, 4, 5, 7, 8, 14, 15, 16, 17, 29, 57, 55, 56, 58, 59, 60]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 53
              AND key_idx = 38;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-text></fa>", "color": "Color-E", "group": "Text", "attributes": [0, 1, 2, 3, 4, 14, 24, 29, 57, 55, 56, 58, 59, 60]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 53
              AND key_idx = 48;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-hashtag></fa>", "color": "Color-E", "group": "Number", "attributes": [0, 1, 2, 3, 4, 14, 24, 29, 57, 55, 56, 58, 59, 60]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 53
              AND key_idx = 49;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-memo></fa>", "color": "Color-E", "group": "Memo", "attributes": [0, 1, 2, 3, 4, 14, 24, 29, 57, 55, 56, 58, 59, 60]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 53
              AND key_idx = 51;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-calendar-clock></fa>", "color": "Color-E", "group": "DateTime", "attributes": [0, 1, 2, 3, 4, 14, 29, 57, 55, 56, 58, 59, 60]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 53
              AND key_idx = 52;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-hashtag></fa>", "color": "Color-A", "group": "Number", "attributes": [0, 1, 2, 3, 4, 5, 14, 15, 16, 17, 29, 57, 55, 56, 58, 59, 60]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 53
              AND key_idx = 61;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-file-lines></fa>", "color": "Color-A", "group": "Number", "attributes": [0, 1, 2, 3, 4, 5, 14, 15, 16, 17, 29, 57, 55, 56, 58, 59, 60]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 53
              AND key_idx = 62;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-sliders></fa>", "color": "Color-A", "group": "Parameter", "attributes": [0, 1, 2, 3, 4, 5, 8, 14, 15, 16, 17, 29, 57, 55, 56, 58, 59, 60]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 53
              AND key_idx = 63;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Overflow Fit + Min/Max Font Size'                                  AS name,
        [=[
            # Forward Migration ${MIGRATION}: Overflow Fit, Min/Max Font Size

            Lookup 054 Overflow List becomes Clip, Ellipsis, Shrink,
            Grow, Fit. Adds Min Font Size (59) and Max Font Size (60).
            Appends 59/60 to every 053 kind that lists Overflow (58).
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]====]})

-- ----------------------------------------------------------------------------
-- Reverse
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
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-text></fa>", "color": "Color-A", "group": "Text", "attributes": [0, 1, 2, 3, 4, 14, 15, 16, 17, 29, 55, 56, 57, 58]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 53
              AND key_idx = 0;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-hashtag></fa>", "color": "Color-A", "group": "Number", "attributes": [0, 1, 2, 3, 4, 5, 14, 15, 16, 17, 29, 57, 55, 56, 58]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 53
              AND key_idx = 1;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-memo></fa>", "color": "Color-A", "group": "Memo", "attributes": [0, 1, 2, 3, 4, 5, 14, 15, 16, 17, 29, 57, 55, 56, 58]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 53
              AND key_idx = 3;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-calendar-clock></fa>", "color": "Color-A", "group": "DateTime", "attributes": [0, 1, 2, 3, 4, 5, 14, 15, 16, 17, 29, 57, 55, 56, 58]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 53
              AND key_idx = 4;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-text></fa>", "color": "Color-B", "group": "Text", "attributes": [0, 1, 2, 3, 4, 5, 8, 7, 14, 15, 16, 17, 29, 57, 55, 56, 58]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 53
              AND key_idx = 20;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-hashtag></fa>", "color": "Color-B", "group": "Number", "attributes": [0, 1, 2, 3, 4, 5, 8, 7, 14, 15, 16, 17, 29, 57, 55, 56, 58]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 53
              AND key_idx = 21;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-memo></fa>", "color": "Color-B", "group": "Memo", "attributes": [0, 1, 2, 3, 4, 5, 8, 7, 14, 15, 16, 17, 29, 57, 55, 56, 58]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 53
              AND key_idx = 23;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-calendar-clock></fa>", "color": "Color-B", "group": "DateTime", "attributes": [0, 1, 2, 3, 4, 5, 8, 7, 14, 15, 16, 17, 29, 57, 55, 56, 58]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 53
              AND key_idx = 24;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-text></fa>", "color": "Color-C", "group": "Text", "attributes": [0, 1, 2, 3, 4, 5, 7, 8, 14, 15, 16, 17, 29, 57, 55, 56, 58]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 53
              AND key_idx = 28;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-hashtag></fa>", "color": "Color-C", "group": "Number", "attributes": [0, 1, 2, 3, 4, 5, 7, 8, 14, 15, 16, 17, 29, 57, 55, 56, 58]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 53
              AND key_idx = 29;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-memo></fa>", "color": "Color-C", "group": "Memo", "attributes": [0, 1, 2, 3, 4, 14, 24, 29, 57, 55, 56, 58]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 53
              AND key_idx = 31;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-calendar-clock></fa>", "color": "Color-C", "group": "DateTime", "attributes": [0, 1, 2, 3, 4, 5, 7, 8, 14, 15, 16, 17, 29, 57, 55, 56, 58]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 53
              AND key_idx = 32;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-text></fa>", "color": "Color-D", "group": "Text", "attributes": [0, 1, 2, 3, 4, 5, 7, 8, 14, 15, 16, 17, 29, 57, 55, 56, 58]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 53
              AND key_idx = 34;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-hashtag></fa>", "color": "Color-D", "group": "Number", "attributes": [0, 1, 2, 3, 4, 5, 7, 8, 14, 15, 16, 17, 29, 57, 55, 56, 58]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 53
              AND key_idx = 35;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-memo></fa>", "color": "Color-D", "group": "Memo", "attributes": [0, 1, 2, 3, 4, 14, 24, 29, 57, 55, 56, 58]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 53
              AND key_idx = 37;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-calendar-clock></fa>", "color": "Color-D", "group": "DateTime", "attributes": [0, 1, 2, 3, 4, 5, 7, 8, 14, 15, 16, 17, 29, 57, 55, 56, 58]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 53
              AND key_idx = 38;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-text></fa>", "color": "Color-E", "group": "Text", "attributes": [0, 1, 2, 3, 4, 14, 24, 29, 57, 55, 56, 58]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 53
              AND key_idx = 48;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-hashtag></fa>", "color": "Color-E", "group": "Number", "attributes": [0, 1, 2, 3, 4, 14, 24, 29, 57, 55, 56, 58]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 53
              AND key_idx = 49;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-memo></fa>", "color": "Color-E", "group": "Memo", "attributes": [0, 1, 2, 3, 4, 14, 24, 29, 57, 55, 56, 58]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 53
              AND key_idx = 51;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-calendar-clock></fa>", "color": "Color-E", "group": "DateTime", "attributes": [0, 1, 2, 3, 4, 14, 29, 57, 55, 56, 58]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 53
              AND key_idx = 52;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-hashtag></fa>", "color": "Color-A", "group": "Number", "attributes": [0, 1, 2, 3, 4, 5, 14, 15, 16, 17, 29, 57, 55, 56, 58]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 53
              AND key_idx = 61;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-file-lines></fa>", "color": "Color-A", "group": "Number", "attributes": [0, 1, 2, 3, 4, 5, 14, 15, 16, 17, 29, 57, 55, 56, 58]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 53
              AND key_idx = 62;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-sliders></fa>", "color": "Color-A", "group": "Parameter", "attributes": [0, 1, 2, 3, 4, 5, 8, 14, 15, 16, 17, 29, 57, 55, 56, 58]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 53
              AND key_idx = 63;

            ${SUBQUERY_DELIMITER}

            DELETE FROM ${SCHEMA}${TABLE}
            WHERE lookup_id = 54
              AND key_idx IN (59, 60);

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"List": ["Clip", "Ellipsis", "Shrink", "Grow"], "Group": "Formatting", "section": 7, "Label": "Overflow", "Editor": "List"}]===]${JSON_INGEST_END}
            WHERE lookup_id = 54
              AND key_idx = 58;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Remove Fit / Min/Max Font Size'                                    AS name,
        [=[
            # Reverse Migration ${MIGRATION}: restore Overflow list

            Drops 054 keys 59/60, removes them from 053 attributes,
            and restores Overflow List without Fit.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]====]})

return queries end
