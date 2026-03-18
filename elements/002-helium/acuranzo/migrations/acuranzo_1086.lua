-- Migration: acuranzo_1086.lua
-- Defaults for Lookup 053 - Report Object Types

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.0.1 - 2026-03-18 - Updated to use lower-case JSON keys, <fa>-style icon definitions
-- 1.0.0 - 2025-12-25 - Initial creation

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "lookups"
cfg.MIGRATION = "1086"
cfg.LOOKUP_ID = "053"
cfg.LOOKUP_NAME = "Report Object Types"
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
            INSERT INTO ${SCHEMA}${TABLE}
            (
                lookup_id,
                key_idx,
                status_a1,
                value_txt,
                value_int,
                sort_seq,
                code,
                summary,
                collection,
                ${COMMON_FIELDS}
            )
            VALUES (
                0,                              -- lookup_id
                ${LOOKUP_ID},                   -- key_idx
                1,                              -- status_a1
                '${LOOKUP_NAME}',               -- value_txt
                0,                              -- value_int
                0,                              -- sort_seq
                '',                             -- code
                [==[
                    # ${LOOKUP_ID} - ${LOOKUP_NAME}

                    Used by the reporting system, naturally.
                ]==],                           -- summary
                ${JSON_INGEST_START}
                [==[
                    {
                        "Default": "JSONEditor",
                        "CSSEditor": false,
                        "HTMLEditor": true,
                        "JSONEditor": true,
                        "LookupEditor": false
                    }
                ]==]
                ${JSON_INGEST_END},             -- collection
                ${COMMON_VALUES}
            );

            ${SUBQUERY_DELIMITER}

            INSERT INTO ${SCHEMA}${TABLE}
                (lookup_id, key_idx, status_a1, value_txt, value_int, sort_seq, code, summary, collection, ${COMMON_FIELDS})
            VALUES
                (${LOOKUP_ID},  0, 1, 'Static Text',            1,  0, '', '', ${JIS}[==[{"icon": "<fa fa-text></fa>", "color": "Color-A", "group": "Text", "attributes": [0, 1, 2, 3, 4, 14, 24, 29]}]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID},  1, 1, 'Static Number',          1,  1, '', '', ${JIS}[==[{"icon": "<fa fa-hashtag></fa>", "color": "Color-A", "group": "Number", "attributes": [0, 1, 2, 3, 4, 14, 24, 29]}]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID},  2, 1, 'Static Image',           1,  2, '', '', ${JIS}[==[{"icon": "<fa fa-image></fa>", "color": "Color-A", "group": "Image", "attributes": [0, 1, 2, 3, 4, 14, 24, 29]}]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID},  3, 1, 'Static Memo',            1,  3, '', '', ${JIS}[==[{"icon": "<fa fa-memo></fa>", "color": "Color-A", "group": "Memo", "attributes": [0, 1, 2, 3, 4, 14, 24, 29]}]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID},  4, 1, 'Static DateTime',        1,  4, '', '', ${JIS}[==[{"icon": "<fa fa-calendar-clock></fa>", "color": "Color-A", "group": "DateTime", "attributes": [0, 1, 2, 3, 4, 14, 24, 29]}]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID},  5, 1, 'System DateTime',        7,  0, '', '', ${JIS}[==[{"icon": "<fa fa-calendar-clock fa-swap-opacity></fa>", "attributes": [0, 1, 2, 3, 4, 14, 24, 29]}]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID},  6, 1, 'System User',            7,  1, '', '', ${JIS}[==[{"icon": "<fa fa-user></fa>"}]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID},  7, 1, 'System Server',          7,  2, '', '', ${JIS}[==[{"Iion": "<fa fa-server></fa>"}]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID},  8, 1, 'System Version',         7,  3, '', '', ${JIS}[==[{"icon": "<fa fa-file-chart-column></fa>"}]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID},  9, 1, 'Report Top',             0,  0, '', '', ${JIS}[==[{"icon": "<fa fa-page></fa>", "color": "Color-H", "label": "Report", "attributes": ["Title", "Object Type", "Link", "Page Size", "Page Orientation", "Page Width", "Page Height", "Page Margin Left", "Page Margin Right", "Page Margin Top", "Page Margin Bottom", "Font Family", "Font Size", "Font Style", "Grid Visible", "Grid Style", "Grid Color", "Grid Background Color", "Grid Spacing", "Grid Weight"]}]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID}, 10, 1, 'Package',                0,  0, '', '', ${JIS}[==[{}]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID}, 11, 1, 'Package Header',         6,  0, '', '', ${JIS}[==[{"icon": "<fa fa-folder-closed fa-swap-opacity></fa>"}]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID}, 12, 1, 'Report Header',          6,  1, '', '', ${JIS}[==[{"icon": "<fa fa-file-arrow-up></fa>"}]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID}, 13, 1, 'Page Header',            6,  2, '', '', ${JIS}[==[{"icon": "<fa fa-border-top></fa>", "color": "Color-A", "label": "Page Header", "attributes": [3, 6, 7, 8, 13, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 29]}]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID}, 14, 1, 'Group Header',           6,  3, '', '', ${JIS}[==[{"icon": "<fa fa-page-caret-up></fa>", "attributes": [0, 1, 2, 3, 4, 14, 24, 29]}]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID}, 15, 1, 'Detail',                 6,  4, '', '', ${JIS}[==[{"icon": "<fa fa-border-center-h></fa>", "color": "Color-B", "label": "Detail", "attributes": [3, 6, 7, 8, 13, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24]}]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID}, 16, 1, 'Group Footer',           6,  5, '', '', ${JIS}[==[{"Iion": "<fa fa-page-caret-down></fa>", "attributes": [0, 1, 2, 3, 4, 14, 24, 29]}]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID}, 17, 1, 'Page Footer',            6,  6, '', '', ${JIS}[==[{"icon": "<fa fa-border-bottom></fa>", "color": "Color-A", "attributes": [3, 6, 7, 8, 13, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 29]}]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID}, 18, 1, 'Report Footer',          6,  7, '', '', ${JIS}[==[{"icon": "<fa fa-file-arrow-down></fa>"}]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID}, 19, 1, 'Package Footer',         6,  8, '', '', ${JIS}[==[{"icon": "<fa fa-folder-closed></fa>"}]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID}, 20, 1, 'Field Text',             2,  0, '', '', ${JIS}[==[{"icon": "<fa fa-text></fa>", "color": "Color-B", "group": "Text", "attributes": [0, 1, 2, 3, 4, 14, 24, 29]}]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID}, 21, 1, 'Field Number',           2,  1, '', '', ${JIS}[==[{"icon": "<fa fa-hashtag></fa>", "color": "Color-B", "group": "Number", "attributes": [0, 1, 2, 3, 4, 14, 24, 29]}]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID}, 22, 1, 'Field Image',            2,  2, '', '', ${JIS}[==[{"icon": "<fa fa-image></fa>", "color": "Color-B", "group": "Image", "attributes": [0, 1, 2, 3, 4, 14, 24, 29]}]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID}, 23, 1, 'Field Memo',             2,  3, '', '', ${JIS}[==[{"icon": "<fa fa-memo></fa>", "color": "Color-B", "group": "Memo", "attributes": [0, 1, 2, 3, 4, 14, 24, 29]}]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID}, 24, 1, 'Field DateTime',         2,  4, '', '', ${JIS}[==[{"icon": "<fa fa-calendar-clock></fa>", "color": "Color-B", "group": "DateTime", "attributes": [0, 1, 2, 3, 4, 14, 24, 29]}]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID}, 25, 1, 'Static Shape',           1,  5, '', '', ${JIS}[==[{"icon": "<fa fa-shapes></fa>", "color": "Color-A", "group": "Shape", "attributes": [0, 1, 2, 3, 4, 14, 24, 29]}]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID}, 26, 1, 'Field Shape',            2,  6, '', '', ${JIS}[==[{"icon": "<fa fa-shapes></fa>", "color": "Color-B", "group": "Shape", "attributes": [0, 1, 2, 3, 4, 14, 24, 29]}]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID}, 27, 1, 'Calculated Counter',     3,  0, '', '', ${JIS}[==[{}]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID}, 28, 1, 'Calculated Text',        3,  0, '', '', ${JIS}[==[{"icon": "<fa fa-text></fa>", "color": "Color-C", "group": "Text", "attributes": [0, 1, 2, 3, 4, 14, 24, 29]}]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID}, 29, 1, 'Calculated Number',      3,  1, '', '', ${JIS}[==[{"icon": "<fa fa-hashtag></fa>", "color": "Color-C", "group": "Number", "attributes": [0, 1, 2, 3, 4, 14, 24, 29]}]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID}, 30, 1, 'Calculated Image',       3,  2, '', '', ${JIS}[==[{"icon": "<fa fa-image></fa>", "color": "Color-C", "group": "Image", "attributes": [0, 1, 2, 3, 4, 14, 24, 29]}]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID}, 31, 1, 'Calculated Memo',        3,  3, '', '', ${JIS}[==[{"icon": "<fa fa-memo></fa>", "color": "Color-C", "group": "Memo", "attributes": [0, 1, 2, 3, 4, 14, 24, 29]}]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID}, 32, 1, 'Calculated DateTime',    3,  4, '', '', ${JIS}[==[{"icon": "<fa fa-calendar-clock></fa>", "color": "Color-C", "group": "DateTime", "attributes": [0, 1, 2, 3, 4, 14, 24, 29]}]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID}, 33, 1, 'Calculated Shape',       3,  5, '', '', ${JIS}[==[{"icon": "<fa fa-shapes></fa>", "color": "Color-C", "group": "Shape", "attributes": [0, 1, 2, 3, 4, 14, 24, 29]}]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID}, 34, 1, 'Aggregate Text',         4,  0, '', '', ${JIS}[==[{"icon": "<fa fa-text></fa>", "color": "Color-D", "group": "Text", "attributes": [0, 1, 2, 3, 4, 14, 24, 29]}]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID}, 35, 1, 'Aggregate Number',       4,  1, '', '', ${JIS}[==[{"icon": "<fa fa-hashtag></fa>", "color": "Color-D", "group": "Number", "attributes": [0, 1, 2, 3, 4, 14, 24, 29]}]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID}, 36, 1, 'Aggregate Image',        4,  2, '', '', ${JIS}[==[{"icon": "<fa fa-image></fa>", "color": "Color-D", "group": "Image", "attributes": [0, 1, 2, 3, 4, 14, 24, 29]}]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID}, 37, 1, 'Aggregate Memo',         4,  3, '', '', ${JIS}[==[{"icon": "<fa fa-memo></fa>", "color": "Color-D", "group": "Memo", "attributes": [0, 1, 2, 3, 4, 14, 24, 29]}]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID}, 38, 1, 'Aggregate DateTime',     4,  4, '', '', ${JIS}[==[{"icon": "<fa fa-calendar-clock></fa>", "color": "Color-D", "group": "DateTime", "attributes": [0, 1, 2, 3, 4, 14, 24, 29]}]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID}, 39, 1, 'Aggregate Shape',        4,  5, '', '', ${JIS}[==[{"icon": "<fa fa-shapes></fa>", "color": "Color-D", "group": "Shape", "attributes": [0, 1, 2, 3, 4, 14, 24, 29]}]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID}, 40, 1, 'Aggregate Chart',        4,  6, '', '', ${JIS}[==[{"icon": "<fa fa-chart-mixed></fa>", "color": "Color-D", "group": "Chart", "attributes": [0, 1, 2, 3, 4, 14, 24, 29]}]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID}, 41, 1, 'Aggregate Barcode',      4,  7, '', '', ${JIS}[==[{"icon": "<fa fa-barcode></fa>", "color": "Color-D", "group": "Barcode", "attributes": [0, 1, 2, 3, 4, 14, 24, 29]}]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID}, 42, 1, 'Static Chart',           1,  6, '', '', ${JIS}[==[{"icon": "<fa fa-chart-mixed></fa>", "color": "Color-A", "group": "Chart", "attributes": [0, 1, 2, 3, 4, 14, 24, 29]}]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID}, 43, 1, 'Static Barcode',         1,  7, '', '', ${JIS}[==[{"icon": "<fa fa-barcode></fa>", "color": "Color-A", "group": "Barcode", "attributes": [0, 1, 2, 3, 4, 14, 24, 29]}]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID}, 44, 1, 'Field Chart',            2,  6, '', '', ${JIS}[==[{"icon": "<fa fa-chart-mixed></fa>", "color": "Color-B", "group": "Chart", "attributes": [0, 1, 2, 3, 4, 14, 24, 29]}]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID}, 45, 1, 'Field Barcode',          2,  7, '', '', ${JIS}[==[{"icon": "<fa fa-barcode></fa>", "color": "Color-B", "group": "Barcode", "attributes": [0, 1, 2, 3, 4, 14, 24, 29]}]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID}, 46, 1, 'Calculated Chart',       3,  6, '', '', ${JIS}[==[{"icon": "<fa fa-chart-mixed></fa>", "color": "Color-C", "group": "Chart", "attributes": [0, 1, 2, 3, 4, 14, 24, 29]}]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID}, 47, 1, 'Calculated Barcode',     3,  7, '', '', ${JIS}[==[{"icon": "<fa fa-barcode></fa>", "color": "Color-C", "group": "Barcode", "attributes": [0, 1, 2, 3, 4, 14, 24, 29]}]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID}, 48, 1, 'CrossTab Text',          5,  0, '', '', ${JIS}[==[{"icon": "<fa fa-text></fa>", "color": "Color-E", "group": "Text", "attributes": [0, 1, 2, 3, 4, 14, 24, 29]}]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID}, 49, 1, 'CrossTab Number',        5,  1, '', '', ${JIS}[==[{"icon": "<fa fa-hashtag></fa>", "color": "Color-E", "group": "Number", "attributes": [0, 1, 2, 3, 4, 14, 24, 29]}]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID}, 50, 1, 'CrossTab Image',         5,  2, '', '', ${JIS}[==[{"icon": "<fa fa-image></fa>", "color": "Color-E", "group": "Image", "attributes": [0, 1, 2, 3, 4, 14, 24, 29]}]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID}, 51, 1, 'CrossTab Memo',          5,  3, '', '', ${JIS}[==[{"icon": "<fa fa-memo></fa>", "color": "Color-E", "group": "Memo", "attributes": [0, 1, 2, 3, 4, 14, 24, 29]}]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID}, 52, 1, 'CrossTab DateTime',      5,  4, '', '', ${JIS}[==[{"icon": "<fa fa-calendar-clock</fa>", "color": "Color-E", "group": "DateTime", "attributes": [0, 1, 2, 3, 4, 14, 24, 29]}]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID}, 53, 1, 'CrossTab Shape',         5,  5, '', '', ${JIS}[==[{"icon": "<fa fa-shapes></fa>", "color": "Color-E", "group": "Shape", "attributes": [0, 1, 2, 3, 4, 14, 24, 29]}]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID}, 54, 1, 'CrossTab Chart',         5,  6, '', '', ${JIS}[==[{"icon": "<fa fa-chart-mixed></fa>", "color": "Color-E", "group": "Chart", "attributes": [0, 1, 2, 3, 4, 14, 24, 29]}]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID}, 55, 1, 'CrossTab Barcode',       5,  7, '', '', ${JIS}[==[{"icon": "<fa fa-barcode></fa>", "color": "Color-E", "group": "Barcode", "attributes": [0, 1, 2, 3, 4, 14, 24, 29]}]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID}, 56, 1, 'SubReport',              1,  8, '', '', ${JIS}[==[{"icon": "<fa fa-page></fa>", "color": "Color-F", "group": "SubReport", "attributes": [0, 1, 2, 3, 4, 14, 24, 29]}]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID}, 57, 1, 'Report Bottom',          0,  0, '', '', ${JIS}[==[{"icon": "<fa fa-page></fa>", "color": "Color-H", "label": "End of Report"}]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID}, 58, 1, 'Ruler',                  6,  9, '', '', ${JIS}[==[{"icon": "<fa fa-ruler></fa>", "attributes": [6, 13, 25, 26, 27, 28]}]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID}, 59, 1, 'Report',                 0,  0, '', '', ${JIS}[==[{}]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID}, 60, 1, 'Section',                0,  0, '', '', ${JIS}[==[{}]==]${JIE}, ${COMMON_VALUES});

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Populate Lookup ${LOOKUP_ID} in ${TABLE} table'                    AS name,
        [=[
            # Forward Migration ${MIGRATION}: Poulate Lookup ${LOOKUP_ID} - ${LOOKUP_NAME}

            This migration creates the lookup values for Lookup ${LOOKUP_ID} - ${LOOKUP_NAME}
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
            DELETE FROM ${SCHEMA}${TABLE}
            WHERE lookup_id = 0
            AND key_idx = ${LOOKUP_ID};

            ${SUBQUERY_DELIMITER}

            DELETE FROM ${SCHEMA}${TABLE}
            WHERE lookup_id = ${LOOKUP_ID}
                AND (key_idx >= 0)
                AND (key_idx <= 60)

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Remove Lookup ${LOOKUP_ID} from ${TABLE} Table'                             AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Remove Lookup ${LOOKUP_ID} - ${LOOKUP_NAME} from ${TABLE} Table

            This is provided for completeness when testing the migration system
            to ensure that forward and reverse migrations are complete.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
return queries end
