-- Migration: acuranzo_1087.lua
-- Defaults for Lookup 054 - Report Object Attributes

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.0.0 - 2025-12-25 - Initial creation

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "lookups"
cfg.MIGRATION = "1087"
cfg.LOOKUP_ID = "054"
cfg.LOOKUP_NAME = "Report Object Attributes"
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

                    Used by the reporting system, naturally, to define the attributes of report objects.
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
                (${LOOKUP_ID},  0, 1, 'Top',                    1, 200, '', '', ${JIS}[==[ {"Group": "Position", "Editor": "Number"} ]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID},  1, 1, 'Left',                   1, 201, '', '', ${JIS}[==[ {"Group": "Position", "Editor": "Number"} ]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID},  2, 1, 'Width',                  1, 202, '', '', ${JIS}[==[ {"Group": "Position", "Editor": "Number"} ]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID},  3, 1, 'Height',                 1, 203, '', '', ${JIS}[==[ {"Group": "Position", "Editor": "Number"} ]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID},  4, 1, 'Align',                  1, 205, '', '', ${JIS}[==[ {"Group": "Position", "Editor": "List"} ]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID},  5, 1, 'Fit',                    1, 206, '', '', ${JIS}[==[ {"Group": "Position", "Editor": "List"} ]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID},  6, 1, 'Object Name',            1, 100, '', '', ${JIS}[==[ {"Group": "Object", "Label": "Name", "Editor": "Text"} ]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID},  7, 1, 'Field',                  1, 103, '', '', ${JIS}[==[ {"Group": "Object", "Editor": "Field"} ]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID},  8, 1, 'Object Datasource',      1, 102, '', '', ${JIS}[==[ {"Group": "Object", "Label": "Datasource", "Editor": "Datasource"} ]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID},  9, 1, 'Number Format',          1, 300, '', '', ${JIS}[==[ {"Group": "Formatting", "Editor": "List"} ]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID}, 10, 1, 'Blank if Zero',          1, 301, '', '', ${JIS}[==[ {"Group": "Formatting", "Editor": "Boolean"} ]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID}, 11, 1, 'Zero if Blank',          1, 302, '', '', ${JIS}[==[ {"Group": "Formatting", "Editor": "Boolean"} ]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID}, 12, 1, 'Negative Parentheses',   1, 303, '', '', ${JIS}[==[ {"Group": "Formatting", "Editor": "Boolean"} ]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID}, 13, 1, 'Object Type',            1, 101, '', '', ${JIS}[==[ {"Group": "Object", "Label": "Type", "Editor": "Text"} ]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID}, 14, 1, 'Rotation',               1, 204, '', '', ${JIS}[==[ {"Group": "Position", "Editor": "Number"} ]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID}, 15, 1, 'Font Family',            1, 150, '', '', ${JIS}[==[ {"Hint": {"default": "Default: Helvetica"}, "List": ["Helvetica", "Open Sans", "Cairo", "Courier New", "Lexend"], "Group": "Fonts", "Label": "Font Family", "Editor": "List"} ]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID}, 16, 1, 'Font Size',              1, 151, '', '', ${JIS}[==[ {"Hint": {"default": "Typically expressed in points.<br>Default: 10 pt"}, "Group": "Fonts", "Label": "Font Size", "Editor": "Number"} ]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID}, 17, 1, 'Font Style',             1, 152, '', '', ${JIS}[==[ {"List": ["Normal", "Bold", "Underline", "Italic", "Bold + Underlilne", "Bold + Italic", "Underline + Italic", "Bold + Underline + Italic", "Superscript", "Subscript", "Strikethrough"], "Group": "Fonts", "Label": "Font Style", "Editor": "List"} ]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID}, 18, 1, 'Title',                  1,   0, '', '', ${JIS}[==[ {"Group": "Report", "Label": "Title", "Editor": "Text"} ]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID}, 19, 1, 'Template',               1,   0, '', '', ${JIS}[==[ {"Group": "Report", "Editor": "Boolean"} ]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID}, 20, 1, 'Grid Visible',           1, 120, '', '', ${JIS}[==[ {"List": ["True", "False"], "Group": "Grid", "Label": "Visible", "Editor": "List"} ]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID}, 21, 1, 'Grid Color',             1, 120, '', '', ${JIS}[==[ {"Group": "Grid", "Label": "Color", "Editor": "Text"} ]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID}, 22, 1, 'Grid Style',             1, 120, '', '', ${JIS}[==[ {"List": ["Grid", "Dots", "Dotted", "Dashed", "None"], "Group": "Grid", "Label": "Style", "Editor": "List"} ]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID}, 23, 1, 'Grid Spacing',           1, 120, '', '', ${JIS}[==[ {"Group": "Grid", "Label": "Spacing", "Editor": "Number"} ]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID}, 24, 1, 'Grid Background Color',  1, 120, '', '', ${JIS}[==[ {"Group": "Grid", "Label": "Background", "Editor": "Text"} ]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID}, 25, 1, 'Ruler Background Color', 1, 120, '', '', ${JIS}[==[ {"Group": "Ruler", "Label": "Background", "Editor": "Color"} ]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID}, 26, 1, 'Ruler Orientation',      1, 120, '', '', ${JIS}[==[ {"Group": "Ruler", "Label": "Orientation", "Editor": "List"} ]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID}, 27, 1, 'Ruler Units',            1, 120, '', '', ${JIS}[==[ {"Group": "Ruler", "Label": "Units", "Editor": "List"} ]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID}, 28, 1, 'Ruler Display',          1, 120, '', '', ${JIS}[==[ {"Group": "Ruler", "Label": "Display", "Editor": "List"} ]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID}, 29, 1, 'Link',                   1, 120, '', '', ${JIS}[==[ {"Group": "Object", "Label": "Link", "Editor": "None"} ]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID}, 30, 1, 'Page Size',              1, 120, '', '', ${JIS}[==[ {"List": ["US Letter", "US Legal", "A4", "5x7", "4x6", "1x2", "Custom"], "Group": "Page Layout", "Label": "Page Size", "Editor": "List"} ]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID}, 31, 1, 'Page Width',             1, 120, '', '', ${JIS}[==[ {"Group": "Page Layout", "Label": "Page Width", "Editor": "Number"} ]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID}, 32, 1, 'Page Height',            1, 120, '', '', ${JIS}[==[ {"Group": "Page Layout", "Label": "Page Height", "Editor": "Number"} ]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID}, 33, 1, 'Page Orientation',       1, 120, '', '', ${JIS}[==[ {"List": ["Portrait", "Landscape"], "Group": "Page Layout", "Label": "Orientation", "Editor": "List"} ]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID}, 34, 1, 'Page Units',             1, 120, '', '', ${JIS}[==[ {"List": ["in", "mm", "cm", "px", "pt"], "Group": "Page Layout", "Label": "Units", "Editor": "List"} ]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID}, 35, 1, 'Page Margin Left',       1, 120, '', '', ${JIS}[==[ {"Group": "Page Margins", "Label": "Left", "Editor": "Number"} ]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID}, 36, 1, 'Page Margin Right',      1, 120, '', '', ${JIS}[==[ {"Group": "Page Margins", "Label": "Right", "Editor": "Number"} ]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID}, 37, 1, 'Page Margin Top',        1, 120, '', '', ${JIS}[==[ {"Group": "Page Margins", "Label": "Top", "Editor": "Number"} ]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID}, 38, 1, 'Page Margin Bottom',     1, 120, '', '', ${JIS}[==[ {"Group": "Page Margins", "Label": "Bottom", "Editor": "Number"} ]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID}, 39, 1, 'Page Margin Guides',     1, 120, '', '', ${JIS}[==[ {"List": ["Enabled", "Disabled"], "Group": "Page Margins", "Label": "Alignment Guides", "Editor": "List"} ]==]${JIE}, ${COMMON_VALUES}),
                (${LOOKUP_ID}, 40, 1, 'Grid Weight',            1, 120, '', '', ${JIS}[==[ {"Group": "Grid", "Label": "Weight", "Editor": "Number"} ]==]${JIE}, ${COMMON_VALUES});

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
                AND (key_idx <= 40)

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
