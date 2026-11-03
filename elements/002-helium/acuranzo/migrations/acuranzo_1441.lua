-- Migration: acuranzo_1441.lua
-- Lookup 054 (Report Object Attributes) sections for Reporting Phase 13b
--
-- Does not edit 1087 or 1437 in place. Adds a numeric "section" key
-- (a lookup 075 key_idx) to the collection of every 054 row, keys 0-40.
-- "Group" stays until Lithium reads "section". sort_seq is not changed.
-- Key 24 Grid Background Color stays in Grid (8). No caller-facing
-- QueryRef. Reverse restores the 1087 JSON as amended by 1437.

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.0.0 - 2026-11-02 - Initial creation (Reporting Phase 13b)

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "lookups"
cfg.MIGRATION = "1441"
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
            SET collection = ${JSON_INGEST_START}[===[{"Group": "Position", "section": 5, "Editor": "Number"}]===]${JSON_INGEST_END}
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx = 0;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"Group": "Position", "section": 5, "Editor": "Number"}]===]${JSON_INGEST_END}
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx = 1;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"Group": "Position", "section": 5, "Editor": "Number"}]===]${JSON_INGEST_END}
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx = 2;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"Group": "Position", "section": 5, "Editor": "Number"}]===]${JSON_INGEST_END}
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx = 3;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"List": ["Left", "Center", "Right"], "Group": "Position", "section": 5, "Editor": "List"}]===]${JSON_INGEST_END}
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx = 4;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"Group": "Position", "section": 5, "Editor": "List"}]===]${JSON_INGEST_END}
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx = 5;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"Group": "Object", "section": 4, "Label": "Name", "Editor": "Text"}]===]${JSON_INGEST_END}
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx = 6;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"Group": "Object", "section": 4, "Editor": "Field"}]===]${JSON_INGEST_END}
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx = 7;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"Group": "Object", "section": 4, "Label": "Datasource", "Editor": "Datasource"}]===]${JSON_INGEST_END}
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx = 8;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"Group": "Formatting", "section": 7, "Editor": "List"}]===]${JSON_INGEST_END}
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx = 9;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"Group": "Formatting", "section": 7, "Editor": "Boolean"}]===]${JSON_INGEST_END}
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx = 10;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"Group": "Formatting", "section": 7, "Editor": "Boolean"}]===]${JSON_INGEST_END}
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx = 11;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"Group": "Formatting", "section": 7, "Editor": "Boolean"}]===]${JSON_INGEST_END}
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx = 12;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"Group": "Object", "section": 4, "Label": "Type", "Editor": "Text"}]===]${JSON_INGEST_END}
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx = 13;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"Group": "Position", "section": 5, "Editor": "Number"}]===]${JSON_INGEST_END}
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx = 14;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"Hint": {"default": "Default: Vanadium Sans"}, "List": ["Vanadium Sans", "Vanadium Mono", "Helvetica", "Open Sans", "Cairo", "Courier New", "Lexend"], "Group": "Fonts", "section": 6, "Label": "Font Family", "Editor": "List"}]===]${JSON_INGEST_END}
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx = 15;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"Hint": {"default": "Typically expressed in points.<br>Default: 10 pt"}, "Group": "Fonts", "section": 6, "Label": "Font Size", "Editor": "Number"}]===]${JSON_INGEST_END}
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx = 16;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"List": ["Normal", "Bold", "Italic", "Bold + Italic"], "Group": "Fonts", "section": 6, "Label": "Font Style", "Editor": "List"}]===]${JSON_INGEST_END}
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx = 17;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"Group": "Report", "section": 1, "Label": "Title", "Editor": "Text"}]===]${JSON_INGEST_END}
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx = 18;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"Group": "Report", "section": 1, "Editor": "Boolean"}]===]${JSON_INGEST_END}
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx = 19;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"List": ["True", "False"], "Group": "Grid", "section": 8, "Label": "Visible", "Editor": "List"}]===]${JSON_INGEST_END}
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx = 20;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"Group": "Grid", "section": 8, "Label": "Color", "Editor": "Text"}]===]${JSON_INGEST_END}
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx = 21;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"List": ["Grid", "Dots", "Dotted", "Dashed", "None"], "Group": "Grid", "section": 8, "Label": "Style", "Editor": "List"}]===]${JSON_INGEST_END}
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx = 22;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"Group": "Grid", "section": 8, "Label": "Spacing", "Editor": "Number"}]===]${JSON_INGEST_END}
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx = 23;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"Group": "Grid", "section": 8, "Label": "Background", "Editor": "Text"}]===]${JSON_INGEST_END}
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx = 24;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"Group": "Ruler", "section": 9, "Label": "Background", "Editor": "Color"}]===]${JSON_INGEST_END}
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx = 25;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"Group": "Ruler", "section": 9, "Label": "Orientation", "Editor": "List"}]===]${JSON_INGEST_END}
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx = 26;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"Group": "Ruler", "section": 9, "Label": "Units", "Editor": "List"}]===]${JSON_INGEST_END}
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx = 27;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"Group": "Ruler", "section": 9, "Label": "Display", "Editor": "List"}]===]${JSON_INGEST_END}
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx = 28;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"Group": "Object", "section": 4, "Label": "Link", "Editor": "None"}]===]${JSON_INGEST_END}
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx = 29;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"List": ["US Letter", "US Legal", "A4", "5x7", "4x6", "1x2", "Custom"], "Group": "Page Layout", "section": 2, "Label": "Page Size", "Editor": "List"}]===]${JSON_INGEST_END}
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx = 30;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"Group": "Page Layout", "section": 2, "Label": "Page Width", "Editor": "Number"}]===]${JSON_INGEST_END}
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx = 31;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"Group": "Page Layout", "section": 2, "Label": "Page Height", "Editor": "Number"}]===]${JSON_INGEST_END}
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx = 32;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"List": ["Portrait", "Landscape"], "Group": "Page Layout", "section": 2, "Label": "Orientation", "Editor": "List"}]===]${JSON_INGEST_END}
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx = 33;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"List": ["in", "mm", "cm", "px", "pt"], "Group": "Page Layout", "section": 2, "Label": "Units", "Editor": "List"}]===]${JSON_INGEST_END}
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx = 34;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"Group": "Page Margins", "section": 3, "Label": "Left", "Editor": "Number"}]===]${JSON_INGEST_END}
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx = 35;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"Group": "Page Margins", "section": 3, "Label": "Right", "Editor": "Number"}]===]${JSON_INGEST_END}
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx = 36;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"Group": "Page Margins", "section": 3, "Label": "Top", "Editor": "Number"}]===]${JSON_INGEST_END}
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx = 37;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"Group": "Page Margins", "section": 3, "Label": "Bottom", "Editor": "Number"}]===]${JSON_INGEST_END}
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx = 38;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"List": ["Enabled", "Disabled"], "Group": "Page Margins", "section": 3, "Label": "Alignment Guides", "Editor": "List"}]===]${JSON_INGEST_END}
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx = 39;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"Group": "Grid", "section": 8, "Label": "Weight", "Editor": "Number"}]===]${JSON_INGEST_END}
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx = 40;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Update Lookup ${LOOKUP_ID} sections in ${TABLE} table'                                                            AS name,
        [=[
            # Forward Migration ${MIGRATION}: Lookup ${LOOKUP_ID} - ${LOOKUP_NAME} sections

            Adds a lookup 075 "section" key to keys 0-40 and keeps "Group".
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
            SET collection = ${JSON_INGEST_START}[===[{"Group": "Position", "Editor": "Number"}]===]${JSON_INGEST_END}
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx = 0;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"Group": "Position", "Editor": "Number"}]===]${JSON_INGEST_END}
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx = 1;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"Group": "Position", "Editor": "Number"}]===]${JSON_INGEST_END}
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx = 2;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"Group": "Position", "Editor": "Number"}]===]${JSON_INGEST_END}
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx = 3;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"List": ["Left", "Center", "Right"], "Group": "Position", "Editor": "List"}]===]${JSON_INGEST_END}
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx = 4;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"Group": "Position", "Editor": "List"}]===]${JSON_INGEST_END}
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx = 5;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"Group": "Object", "Label": "Name", "Editor": "Text"}]===]${JSON_INGEST_END}
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx = 6;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"Group": "Object", "Editor": "Field"}]===]${JSON_INGEST_END}
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx = 7;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"Group": "Object", "Label": "Datasource", "Editor": "Datasource"}]===]${JSON_INGEST_END}
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx = 8;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"Group": "Formatting", "Editor": "List"}]===]${JSON_INGEST_END}
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx = 9;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"Group": "Formatting", "Editor": "Boolean"}]===]${JSON_INGEST_END}
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx = 10;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"Group": "Formatting", "Editor": "Boolean"}]===]${JSON_INGEST_END}
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx = 11;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"Group": "Formatting", "Editor": "Boolean"}]===]${JSON_INGEST_END}
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx = 12;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"Group": "Object", "Label": "Type", "Editor": "Text"}]===]${JSON_INGEST_END}
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx = 13;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"Group": "Position", "Editor": "Number"}]===]${JSON_INGEST_END}
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx = 14;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"Hint": {"default": "Default: Vanadium Sans"}, "List": ["Vanadium Sans", "Vanadium Mono", "Helvetica", "Open Sans", "Cairo", "Courier New", "Lexend"], "Group": "Fonts", "Label": "Font Family", "Editor": "List"}]===]${JSON_INGEST_END}
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx = 15;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"Hint": {"default": "Typically expressed in points.<br>Default: 10 pt"}, "Group": "Fonts", "Label": "Font Size", "Editor": "Number"}]===]${JSON_INGEST_END}
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx = 16;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"List": ["Normal", "Bold", "Italic", "Bold + Italic"], "Group": "Fonts", "Label": "Font Style", "Editor": "List"}]===]${JSON_INGEST_END}
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx = 17;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"Group": "Report", "Label": "Title", "Editor": "Text"}]===]${JSON_INGEST_END}
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx = 18;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"Group": "Report", "Editor": "Boolean"}]===]${JSON_INGEST_END}
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx = 19;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"List": ["True", "False"], "Group": "Grid", "Label": "Visible", "Editor": "List"}]===]${JSON_INGEST_END}
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx = 20;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"Group": "Grid", "Label": "Color", "Editor": "Text"}]===]${JSON_INGEST_END}
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx = 21;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"List": ["Grid", "Dots", "Dotted", "Dashed", "None"], "Group": "Grid", "Label": "Style", "Editor": "List"}]===]${JSON_INGEST_END}
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx = 22;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"Group": "Grid", "Label": "Spacing", "Editor": "Number"}]===]${JSON_INGEST_END}
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx = 23;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"Group": "Grid", "Label": "Background", "Editor": "Text"}]===]${JSON_INGEST_END}
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx = 24;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"Group": "Ruler", "Label": "Background", "Editor": "Color"}]===]${JSON_INGEST_END}
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx = 25;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"Group": "Ruler", "Label": "Orientation", "Editor": "List"}]===]${JSON_INGEST_END}
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx = 26;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"Group": "Ruler", "Label": "Units", "Editor": "List"}]===]${JSON_INGEST_END}
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx = 27;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"Group": "Ruler", "Label": "Display", "Editor": "List"}]===]${JSON_INGEST_END}
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx = 28;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"Group": "Object", "Label": "Link", "Editor": "None"}]===]${JSON_INGEST_END}
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx = 29;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"List": ["US Letter", "US Legal", "A4", "5x7", "4x6", "1x2", "Custom"], "Group": "Page Layout", "Label": "Page Size", "Editor": "List"}]===]${JSON_INGEST_END}
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx = 30;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"Group": "Page Layout", "Label": "Page Width", "Editor": "Number"}]===]${JSON_INGEST_END}
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx = 31;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"Group": "Page Layout", "Label": "Page Height", "Editor": "Number"}]===]${JSON_INGEST_END}
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx = 32;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"List": ["Portrait", "Landscape"], "Group": "Page Layout", "Label": "Orientation", "Editor": "List"}]===]${JSON_INGEST_END}
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx = 33;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"List": ["in", "mm", "cm", "px", "pt"], "Group": "Page Layout", "Label": "Units", "Editor": "List"}]===]${JSON_INGEST_END}
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx = 34;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"Group": "Page Margins", "Label": "Left", "Editor": "Number"}]===]${JSON_INGEST_END}
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx = 35;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"Group": "Page Margins", "Label": "Right", "Editor": "Number"}]===]${JSON_INGEST_END}
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx = 36;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"Group": "Page Margins", "Label": "Top", "Editor": "Number"}]===]${JSON_INGEST_END}
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx = 37;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"Group": "Page Margins", "Label": "Bottom", "Editor": "Number"}]===]${JSON_INGEST_END}
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx = 38;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"List": ["Enabled", "Disabled"], "Group": "Page Margins", "Label": "Alignment Guides", "Editor": "List"}]===]${JSON_INGEST_END}
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx = 39;

            ${SUBQUERY_DELIMITER}
            UPDATE ${SCHEMA}${TABLE}
            SET collection = ${JSON_INGEST_START}[===[{"Group": "Grid", "Label": "Weight", "Editor": "Number"}]===]${JSON_INGEST_END}
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx = 40;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Undo Lookup ${LOOKUP_ID} sections in ${TABLE} table'                                                            AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Undo Lookup ${LOOKUP_ID} sections

            Restores the 1087 JSON, as amended by 1437, on keys 0-40.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
return queries end
