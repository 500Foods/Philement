-- Migration: acuranzo_1481.lua
-- Report Units, Background, Overflow lookups (Reporting Phase 75)
--
-- Lookup 075 keys 17 (Units) and 18 (Background).
-- Lookup 054 keys 53-58: Position Units, Font Units, Fore Color,
-- Back Color, Background Color, Overflow. Align (4) moves to
-- Formatting with Justified. Page Units (34) moves to Units.
-- Static Text (053 key 0) drops Fit (5) and gains Overflow +
-- colours. Background Color (57) is added to every Element Kind
-- attribute list; Fill Color (47) is removed from shape lists.
-- Fill Style (46) stays. Report Top gains Position Units and Font
-- Units by name. No Render module. QueryRef 175 stays unused.
-- Reverse deletes new 075/054 keys and restores prior collections.
--
-- luacheck: no max line length
-- luacheck: no unused args
--
-- CHANGELOG
-- 1.0.0 - 2026-11-09 - Phase 75 Units, Background, Overflow lookups

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "lookups"
cfg.MIGRATION = "1481"

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
            INSERT INTO ${SCHEMA}${TABLE}
                (lookup_id, key_idx, status_a1, value_txt, value_int, sort_seq, code, summary, collection, ${COMMON_FIELDS})
            VALUES
                (75, 17, 1, 'Units', 17, 17, '', '', ${JSON_INGEST_START}[===[{"icon": "<fa fa-ruler-combined></fa>"}]===]${JSON_INGEST_END}, ${COMMON_VALUES});

            ${SUBQUERY_DELIMITER}

            INSERT INTO ${SCHEMA}${TABLE}
                (lookup_id, key_idx, status_a1, value_txt, value_int, sort_seq, code, summary, collection, ${COMMON_FIELDS})
            VALUES
                (75, 18, 1, 'Background', 18, 18, '', '', ${JSON_INGEST_START}[===[{"icon": "<fa fa-fill-drip></fa>"}]===]${JSON_INGEST_END}, ${COMMON_VALUES});

            ${SUBQUERY_DELIMITER}

            INSERT INTO ${SCHEMA}${TABLE}
                (lookup_id, key_idx, status_a1, value_txt, value_int, sort_seq, code, summary, collection, ${COMMON_FIELDS})
            VALUES
                (54, 53, 1, 'Position Units', 1, 230, '', '', ${JSON_INGEST_START}[===[{"List": ["in", "mm", "cm", "px", "pt"], "Group": "Units", "section": 17, "Label": "Position Units", "Editor": "List"}]===]${JSON_INGEST_END}, ${COMMON_VALUES});

            ${SUBQUERY_DELIMITER}

            INSERT INTO ${SCHEMA}${TABLE}
                (lookup_id, key_idx, status_a1, value_txt, value_int, sort_seq, code, summary, collection, ${COMMON_FIELDS})
            VALUES
                (54, 54, 1, 'Font Units', 1, 231, '', '', ${JSON_INGEST_START}[===[{"List": ["pt", "in", "mm", "cm", "px"], "Group": "Units", "section": 17, "Label": "Font Units", "Editor": "List"}]===]${JSON_INGEST_END}, ${COMMON_VALUES});

            ${SUBQUERY_DELIMITER}

            INSERT INTO ${SCHEMA}${TABLE}
                (lookup_id, key_idx, status_a1, value_txt, value_int, sort_seq, code, summary, collection, ${COMMON_FIELDS})
            VALUES
                (54, 55, 1, 'Fore Color', 1, 232, '', '', ${JSON_INGEST_START}[===[{"Group": "Formatting", "section": 7, "Label": "Fore Color", "Editor": "Color"}]===]${JSON_INGEST_END}, ${COMMON_VALUES});

            ${SUBQUERY_DELIMITER}

            INSERT INTO ${SCHEMA}${TABLE}
                (lookup_id, key_idx, status_a1, value_txt, value_int, sort_seq, code, summary, collection, ${COMMON_FIELDS})
            VALUES
                (54, 56, 1, 'Back Color', 1, 233, '', '', ${JSON_INGEST_START}[===[{"Group": "Formatting", "section": 7, "Label": "Back Color", "Editor": "Color"}]===]${JSON_INGEST_END}, ${COMMON_VALUES});

            ${SUBQUERY_DELIMITER}

            INSERT INTO ${SCHEMA}${TABLE}
                (lookup_id, key_idx, status_a1, value_txt, value_int, sort_seq, code, summary, collection, ${COMMON_FIELDS})
            VALUES
                (54, 57, 1, 'Background Color', 1, 234, '', '', ${JSON_INGEST_START}[===[{"Group": "Background", "section": 18, "Label": "Background Color", "Editor": "Color"}]===]${JSON_INGEST_END}, ${COMMON_VALUES});

            ${SUBQUERY_DELIMITER}

            INSERT INTO ${SCHEMA}${TABLE}
                (lookup_id, key_idx, status_a1, value_txt, value_int, sort_seq, code, summary, collection, ${COMMON_FIELDS})
            VALUES
                (54, 58, 1, 'Overflow', 1, 235, '', '', ${JSON_INGEST_START}[===[{"List": ["Clip", "Ellipsis", "Shrink", "Grow"], "Group": "Formatting", "section": 7, "Label": "Overflow", "Editor": "List"}]===]${JSON_INGEST_END}, ${COMMON_VALUES});

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"List": ["Left", "Center", "Right", "Justified"], "Group": "Formatting", "section": 7, "Label": "Align", "Editor": "List"}]===]${JSON_INGEST_END}
            WHERE lookup_id = 54
              AND key_idx = 4;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"List": ["in", "mm", "cm", "px", "pt"], "Group": "Units", "section": 17, "Label": "Page Units", "Editor": "List"}]===]${JSON_INGEST_END}
            WHERE lookup_id = 54
              AND key_idx = 34;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-page></fa>", "color": "Color-H", "label": "Report", "attributes": ["Title", "Object Type", "Link", "Page Size", "Page Orientation", "Page Width", "Page Height", "Page Margin Left", "Page Margin Right", "Page Margin Top", "Page Margin Bottom", "Position Units", "Font Units", "Page Units", "Font Family", "Font Size", "Font Style", "Grid Visible", "Grid Style", "Grid Color", "Grid Background Color", "Grid Spacing", "Grid Weight"]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 53
              AND key_idx = 9;

            ${SUBQUERY_DELIMITER}

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
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-image></fa>", "color": "Color-A", "group": "Image", "attributes": [0, 1, 2, 3, 4, 5, 14, 15, 16, 17, 29, 57]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 53
              AND key_idx = 2;

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
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-image></fa>", "color": "Color-B", "group": "Image", "attributes": [0, 1, 2, 3, 4, 5, 8, 7, 14, 15, 16, 17, 29, 57]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 53
              AND key_idx = 22;

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
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-shapes></fa>", "color": "Color-A", "group": "Shape", "attributes": [0, 1, 2, 3, 4, 14, 29, 41, 42, 43, 44, 45, 46, 48, 49, 50, 51, 57]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 53
              AND key_idx = 25;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-shapes></fa>", "color": "Color-B", "group": "Shape", "attributes": [0, 1, 2, 3, 8, 7, 14, 29, 41, 42, 43, 44, 45, 46, 48, 49, 50, 51, 57]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 53
              AND key_idx = 26;

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
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-image></fa>", "color": "Color-C", "group": "Image", "attributes": [0, 1, 2, 3, 4, 14, 24, 29, 57]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 53
              AND key_idx = 30;

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
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-shapes></fa>", "color": "Color-C", "group": "Shape", "attributes": [0, 1, 2, 3, 4, 14, 24, 29, 57]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 53
              AND key_idx = 33;

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
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-image></fa>", "color": "Color-D", "group": "Image", "attributes": [0, 1, 2, 3, 4, 14, 24, 29, 57]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 53
              AND key_idx = 36;

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
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-shapes></fa>", "color": "Color-D", "group": "Shape", "attributes": [0, 1, 2, 3, 4, 14, 24, 29, 57]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 53
              AND key_idx = 39;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-chart-mixed></fa>", "color": "Color-D", "group": "Chart", "attributes": [0, 1, 2, 3, 4, 14, 24, 29, 57]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 53
              AND key_idx = 40;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-barcode></fa>", "color": "Color-D", "group": "Barcode", "attributes": [0, 1, 2, 3, 4, 14, 24, 29, 57]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 53
              AND key_idx = 41;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-chart-mixed></fa>", "color": "Color-A", "group": "Chart", "attributes": [0, 1, 2, 3, 4, 14, 24, 29, 57]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 53
              AND key_idx = 42;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-barcode></fa>", "color": "Color-A", "group": "Barcode", "attributes": [0, 1, 2, 3, 4, 14, 24, 29, 52, 57]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 53
              AND key_idx = 43;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-chart-mixed></fa>", "color": "Color-B", "group": "Chart", "attributes": [0, 1, 2, 3, 4, 14, 24, 29, 57]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 53
              AND key_idx = 44;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-barcode></fa>", "color": "Color-B", "group": "Barcode", "attributes": [0, 1, 2, 3, 4, 8, 7, 14, 24, 29, 52, 57]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 53
              AND key_idx = 45;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-chart-mixed></fa>", "color": "Color-C", "group": "Chart", "attributes": [0, 1, 2, 3, 4, 14, 24, 29, 57]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 53
              AND key_idx = 46;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-barcode></fa>", "color": "Color-C", "group": "Barcode", "attributes": [0, 1, 2, 3, 4, 14, 24, 29, 57]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 53
              AND key_idx = 47;

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
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-image></fa>", "color": "Color-E", "group": "Image", "attributes": [0, 1, 2, 3, 4, 14, 24, 29, 57]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 53
              AND key_idx = 50;

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
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-shapes></fa>", "color": "Color-E", "group": "Shape", "attributes": [0, 1, 2, 3, 4, 14, 24, 29, 57]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 53
              AND key_idx = 53;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-chart-mixed></fa>", "color": "Color-E", "group": "Chart", "attributes": [0, 1, 2, 3, 4, 14, 24, 29, 57]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 53
              AND key_idx = 54;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-barcode></fa>", "color": "Color-E", "group": "Barcode", "attributes": [0, 1, 2, 3, 4, 14, 24, 29, 57]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 53
              AND key_idx = 55;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-page></fa>", "color": "Color-F", "group": "SubReport", "attributes": [0, 1, 2, 3, 4, 14, 24, 29, 57]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 53
              AND key_idx = 56;

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

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Units, Background, Overflow lookups (Phase 75)'                    AS name,
        [=[
            # Forward Migration ${MIGRATION}: Units, Background, Overflow

            Adds lookup 075 keys 17-18 and 054 keys 53-58. Moves Align
            to Formatting with Justified. Moves Page Units into Units.
            Updates Element Kind attribute lists (Background Color;
            Static Text drops Fit; shapes drop Fill Color). Extends
            Report Top attributes with Position Units, Font Units,
            and Page Units.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]====]})

-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
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
            SET collection = ${JSON_INGEST_START}[===[{"List": ["Left", "Center", "Right"], "Group": "Position", "section": 5, "Editor": "List"}]===]${JSON_INGEST_END}
            WHERE lookup_id = 54
              AND key_idx = 4;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"List": ["in", "mm", "cm", "px", "pt"], "Group": "Page Layout", "section": 2, "Label": "Units", "Editor": "List"}]===]${JSON_INGEST_END}
            WHERE lookup_id = 54
              AND key_idx = 34;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-page></fa>", "color": "Color-H", "label": "Report", "attributes": ["Title", "Object Type", "Link", "Page Size", "Page Orientation", "Page Width", "Page Height", "Page Margin Left", "Page Margin Right", "Page Margin Top", "Page Margin Bottom", "Font Family", "Font Size", "Font Style", "Grid Visible", "Grid Style", "Grid Color", "Grid Background Color", "Grid Spacing", "Grid Weight"]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 53
              AND key_idx = 9;

            ${SUBQUERY_DELIMITER}

UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-text></fa>", "color": "Color-A", "group": "Text", "attributes": [0, 1, 2, 3, 4, 5, 14, 15, 16, 17, 29]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 53
              AND key_idx = 0;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-hashtag></fa>", "color": "Color-A", "group": "Number", "attributes": [0, 1, 2, 3, 4, 5, 14, 15, 16, 17, 29]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 53
              AND key_idx = 1;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-image></fa>", "color": "Color-A", "group": "Image", "attributes": [0, 1, 2, 3, 4, 5, 14, 15, 16, 17, 29]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 53
              AND key_idx = 2;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-memo></fa>", "color": "Color-A", "group": "Memo", "attributes": [0, 1, 2, 3, 4, 5, 14, 15, 16, 17, 29]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 53
              AND key_idx = 3;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-calendar-clock></fa>", "color": "Color-A", "group": "DateTime", "attributes": [0, 1, 2, 3, 4, 5, 14, 15, 16, 17, 29]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 53
              AND key_idx = 4;

            ${SUBQUERY_DELIMITER}

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
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-shapes></fa>", "color": "Color-A", "group": "Shape", "attributes": [0, 1, 2, 3, 4, 14, 29, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 53
              AND key_idx = 25;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-shapes></fa>", "color": "Color-B", "group": "Shape", "attributes": [0, 1, 2, 3, 8, 7, 14, 29, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 53
              AND key_idx = 26;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-text></fa>", "color": "Color-C", "group": "Text", "attributes": [0, 1, 2, 3, 4, 5, 7, 8, 14, 15, 16, 17, 29]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 53
              AND key_idx = 28;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-hashtag></fa>", "color": "Color-C", "group": "Number", "attributes": [0, 1, 2, 3, 4, 5, 7, 8, 14, 15, 16, 17, 29]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 53
              AND key_idx = 29;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-image></fa>", "color": "Color-C", "group": "Image", "attributes": [0, 1, 2, 3, 4, 14, 24, 29]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 53
              AND key_idx = 30;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-memo></fa>", "color": "Color-C", "group": "Memo", "attributes": [0, 1, 2, 3, 4, 14, 24, 29]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 53
              AND key_idx = 31;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-calendar-clock></fa>", "color": "Color-C", "group": "DateTime", "attributes": [0, 1, 2, 3, 4, 5, 7, 8, 14, 15, 16, 17, 29]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 53
              AND key_idx = 32;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-shapes></fa>", "color": "Color-C", "group": "Shape", "attributes": [0, 1, 2, 3, 4, 14, 24, 29]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 53
              AND key_idx = 33;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-text></fa>", "color": "Color-D", "group": "Text", "attributes": [0, 1, 2, 3, 4, 5, 7, 8, 14, 15, 16, 17, 29]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 53
              AND key_idx = 34;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-hashtag></fa>", "color": "Color-D", "group": "Number", "attributes": [0, 1, 2, 3, 4, 5, 7, 8, 14, 15, 16, 17, 29]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 53
              AND key_idx = 35;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-image></fa>", "color": "Color-D", "group": "Image", "attributes": [0, 1, 2, 3, 4, 14, 24, 29]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 53
              AND key_idx = 36;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-memo></fa>", "color": "Color-D", "group": "Memo", "attributes": [0, 1, 2, 3, 4, 14, 24, 29]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 53
              AND key_idx = 37;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-calendar-clock></fa>", "color": "Color-D", "group": "DateTime", "attributes": [0, 1, 2, 3, 4, 5, 7, 8, 14, 15, 16, 17, 29]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 53
              AND key_idx = 38;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-shapes></fa>", "color": "Color-D", "group": "Shape", "attributes": [0, 1, 2, 3, 4, 14, 24, 29]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 53
              AND key_idx = 39;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-chart-mixed></fa>", "color": "Color-D", "group": "Chart", "attributes": [0, 1, 2, 3, 4, 14, 24, 29]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 53
              AND key_idx = 40;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-barcode></fa>", "color": "Color-D", "group": "Barcode", "attributes": [0, 1, 2, 3, 4, 14, 24, 29]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 53
              AND key_idx = 41;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-chart-mixed></fa>", "color": "Color-A", "group": "Chart", "attributes": [0, 1, 2, 3, 4, 14, 24, 29]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 53
              AND key_idx = 42;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-barcode></fa>", "color": "Color-A", "group": "Barcode", "attributes": [0, 1, 2, 3, 4, 14, 24, 29, 52]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 53
              AND key_idx = 43;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-chart-mixed></fa>", "color": "Color-B", "group": "Chart", "attributes": [0, 1, 2, 3, 4, 14, 24, 29]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 53
              AND key_idx = 44;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-barcode></fa>", "color": "Color-B", "group": "Barcode", "attributes": [0, 1, 2, 3, 4, 8, 7, 14, 24, 29, 52]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 53
              AND key_idx = 45;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-chart-mixed></fa>", "color": "Color-C", "group": "Chart", "attributes": [0, 1, 2, 3, 4, 14, 24, 29]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 53
              AND key_idx = 46;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-barcode></fa>", "color": "Color-C", "group": "Barcode", "attributes": [0, 1, 2, 3, 4, 14, 24, 29]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 53
              AND key_idx = 47;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-text></fa>", "color": "Color-E", "group": "Text", "attributes": [0, 1, 2, 3, 4, 14, 24, 29]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 53
              AND key_idx = 48;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-hashtag></fa>", "color": "Color-E", "group": "Number", "attributes": [0, 1, 2, 3, 4, 14, 24, 29]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 53
              AND key_idx = 49;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-image></fa>", "color": "Color-E", "group": "Image", "attributes": [0, 1, 2, 3, 4, 14, 24, 29]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 53
              AND key_idx = 50;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-memo></fa>", "color": "Color-E", "group": "Memo", "attributes": [0, 1, 2, 3, 4, 14, 24, 29]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 53
              AND key_idx = 51;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-calendar-clock></fa>", "color": "Color-E", "group": "DateTime", "attributes": [0, 1, 2, 3, 4, 14, 29]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 53
              AND key_idx = 52;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-shapes></fa>", "color": "Color-E", "group": "Shape", "attributes": [0, 1, 2, 3, 4, 14, 24, 29]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 53
              AND key_idx = 53;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-chart-mixed></fa>", "color": "Color-E", "group": "Chart", "attributes": [0, 1, 2, 3, 4, 14, 24, 29]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 53
              AND key_idx = 54;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-barcode></fa>", "color": "Color-E", "group": "Barcode", "attributes": [0, 1, 2, 3, 4, 14, 24, 29]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 53
              AND key_idx = 55;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-page></fa>", "color": "Color-F", "group": "SubReport", "attributes": [0, 1, 2, 3, 4, 14, 24, 29]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 53
              AND key_idx = 56;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-hashtag></fa>", "color": "Color-A", "group": "Number", "attributes": [0, 1, 2, 3, 4, 5, 14, 15, 16, 17, 29]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 53
              AND key_idx = 61;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-file-lines></fa>", "color": "Color-A", "group": "Number", "attributes": [0, 1, 2, 3, 4, 5, 14, 15, 16, 17, 29]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 53
              AND key_idx = 62;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"icon": "<fa fa-sliders></fa>", "color": "Color-A", "group": "Parameter", "attributes": [0, 1, 2, 3, 4, 5, 8, 14, 15, 16, 17, 29]}]===]${JSON_INGEST_END}
            WHERE lookup_id = 53
              AND key_idx = 63;

            ${SUBQUERY_DELIMITER}

            DELETE FROM ${SCHEMA}${TABLE}
            WHERE lookup_id = 54
              AND key_idx IN (53, 54, 55, 56, 57, 58);

            ${SUBQUERY_DELIMITER}

            DELETE FROM ${SCHEMA}${TABLE}
            WHERE lookup_id = 75
              AND key_idx IN (17, 18);

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Remove Units, Background, Overflow lookups (Phase 75)'             AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Units, Background, Overflow

            Restores Align, Page Units, Report Top, and Element Kind
            attribute lists. Deletes 054 keys 53-58 and 075 keys 17-18.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]====]})

    return queries
end
