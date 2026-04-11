-- Migration: acuranzo_1153.lua
-- Tabulator Schema: coltypes.json (Lookup 059, Key 0)

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.0.0 - 2026-03-15 - Initial creation

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "lookups"
cfg.MIGRATION = "1153"
cfg.LOOKUP_ID = "059"
cfg.KEY_IDX = "0"
cfg.SCHEMA_NAME = "column-types"
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
                (lookup_id, key_idx, status_a1, value_txt, value_int, sort_seq, code, summary, collection, ${COMMON_FIELDS})
            VALUES
                (${LOOKUP_ID}, ${KEY_IDX}, 1, '${SCHEMA_NAME}', 0, 0, '',
                [==[
                    # Column Types Schema

                    Defines all available column types for Tabulator tables including formatters,
                    editors, sorters, and display properties. Used as the base configuration
                    for all table columns in the Lithium application.

                    ## Available Coltypes

                    - **integer**: Whole numbers (IDs, counts, references)
                    - **index**: Simple whole numbers (lookup values)
                    - **decimal**: Floating-point numbers (prices, measurements)
                    - **currency**: Monetary values with currency symbol
                    - **percent**: Percentage values (0–100)
                    - **string**: General text/string values
                    - **text**: Long text / multiline content
                    - **html**: Rich HTML content (rendered in cell)
                    - **boolean**: True/false toggle values
                    - **date**: Date values (no time component)
                    - **datetime**: Date + time values (timestamps)
                    - **time**: Time-only values (HH:mm or HH:mm:ss)
                    - **lookup**: Foreign key reference to a lookup table
                    - **enum**: Fixed set of values (status codes, categories)
                    - **email**: Email address (clickable mailto link)
                    - **url**: Hyperlink URL (clickable)
                    - **image**: Image thumbnail (URL stored in field)
                    - **color**: Color swatch (hex color value)
                    - **progress**: Progress bar (0–100 numeric value)
                    - **star**: Star rating (0–5)
                    - **rownum**: Auto-incrementing row number
                    - **json**: JSON object or array (displayed as truncated string)

                    ## Schema Location

                    Source: `elements/003-lithium/config/tabulator/coltypes.json`
                ]==],
                ${JSON_INGEST_START}
[===[
{
  "$description": "Lithium Tabulator column type definitions. Each coltype defines display formatting, alignment, editor behavior, and value-handling rules for columns of that data type.",
  "$schema": "coltypes-schema.json",
  "$version": "1.0.0",
  "coltypes": {
    "default": {
      "description": "Default column configuration",
      "title": "Column",
      "field": null,
      "visible": false,
      "hozAlign": "left",
      "vertAlign": "middle",
      "headerHozAlign": null,
      "headerVertical": false,
      "width": null,
      "minWidth": 20,
      "maxWidth": null,
      "frozen": false,
      "resizable": true,
      "responsive": null,
      "formatter": "plaintext",
      "formatterParams": {},
      "cssClass": "",
      "cellStyle": null,
      "headerStyle": null,
      "rowStyle": null,
      "rowHandle": false,
      "tooltip": null,
      "headerTooltip": null,
      "download": true,
      "headerWordWrap": false,
      "editor": "input",
      "editorParams": { "search": false },
      "editable": false,
      "validator": null,
      "sorter": "alphanum",
      "sorterParams": {},
      "headerSort": true,
      "sortPri": null,
      "headerFilter": false,
      "headerFilterFunc": "like",
      "headerFilterPlaceholder": "filter...",
      "groupable": false,
      "groupPri": null,
      "groupOrd": "asc",
      "columnPri": null,
      "blank": "",
      "zero": null,
      "accessor": null,
      "accessorParams": {},
      "accessorDownload": null,
      "mutator": null,
      "mutatorParams": {},
      "bottomCalc": null,
      "bottomCalcFormatter": null,
      "bottomCalcFormatterParams": {},
      "bottomCalcHozAlign": null,
      "bottomCalcStyle": null,
      "lookupRef": null,
      "lookupStyle": null,
      "lookupEdit": null,
      "contextMenu": null,
      "headerClickMenu": null,
      "cellClick": null,
      "headerClick": null
    },
    "boolean": {
      "description": "True/false toggle values",
      "hozAlign": "center",
      "minWidth": 50,
      "width": 70,
      "formatter": "tickCross",
      "formatterParams": { "allowEmpty": true, "allowTruthy": true },
      "cssClass": "li-col-boolean",
      "editor": "tickCross",
      "editorParams": { "tristate": false, "indeterminateValue": null },
      "sorter": "boolean",
      "headerFilterFunc": "=",
      "blank": null,
      "zero": false,
      "bottomCalc": null
    },
    "color": {
      "description": "Color swatch (hex color value)",
      "hozAlign": "center",
      "minWidth": 40,
      "width": 60,
      "formatter": "color",
      "cssClass": "li-col-color",
      "editor": "input",
      "editorParams": { "search": false },
      "sorter": "alphanum",
      "blank": ""
    },
    "currency": {
      "description": "Monetary values with currency symbol",
      "hozAlign": "right",
      "minWidth": 90,
      "formatter": "money",
      "formatterParams": { "thousand": ",", "decimal": ".", "symbol": "$", "symbolAfter": false, "precision": 2 },
      "cssClass": "li-col-currency",
      "editor": "number",
      "editorParams": { "min": 0, "max": null, "step": 0.01 },
      "sorter": "number",
      "headerFilterFunc": ">=",
      "blank": "",
      "zero": "$0.00",
      "bottomCalc": "sum",
      "bottomCalcFormatter": "money",
      "bottomCalcFormatterParams": { "thousand": ",", "decimal": ".", "symbol": "$", "precision": 2 }
    },
    "date": {
      "description": "Date values (no time component)",
      "hozAlign": "center",
      "minWidth": 90,
      "width": 110,
      "formatter": "datetime",
      "formatterParams": { "inputFormat": "yyyy-MM-dd", "outputFormat": "yyyy-MM-dd", "invalidPlaceholder": "" },
      "cssClass": "li-col-date",
      "editor": "date",
      "sorter": "date",
      "sorterParams": { "format": "yyyy-MM-dd" }
    },
    "datetime": {
      "description": "Date + time values (timestamps)",
      "hozAlign": "center",
      "minWidth": 120,
      "width": 150,
      "formatter": "datetime",
      "formatterParams": { "inputFormat": "yyyy-MM-dd'T'HH:mm:ss", "outputFormat": "yyyy-MM-dd HH:mm", "invalidPlaceholder": "" },
      "cssClass": "li-col-datetime",
      "editor": "datetime-local",
      "sorter": "datetime",
      "sorterParams": { "format": "yyyy-MM-dd'T'HH:mm:ss" }
    },
    "decimal": {
      "description": "Floating-point numbers (prices, measurements, rates)",
      "hozAlign": "right",
      "minWidth": 80,
      "formatter": "number",
      "formatterParams": { "thousand": ",", "precision": 2, "decimal": "." },
      "cssClass": "li-col-decimal",
      "editor": "number",
      "editorParams": { "min": null, "max": null, "step": 0.01 },
      "sorter": "number",
      "headerFilterFunc": ">=",
      "blank": "",
      "zero": "0.00",
      "bottomCalc": "sum",
      "bottomCalcFormatter": "number",
      "bottomCalcFormatterParams": { "thousand": ",", "precision": 2 }
    },
    "email": {
      "description": "Email address (clickable mailto link)",
      "minWidth": 120,
      "formatter": "link",
      "formatterParams": { "target": "_blank", "urlPrefix": "mailto:" },
      "cssClass": "li-col-email",
      "editor": "input",
      "editorParams": { "search": false },
      "sorter": "alphanum",
      "headerFilterFunc": "like",
      "blank": ""
    },
    "enum": {
      "description": "Fixed set of values (status codes, categories)",
      "minWidth": 80,
      "formatter": "plaintext",
      "cssClass": "li-col-enum",
      "editor": "list",
      "editorParams": { "allowEmpty": false, "autocomplete": true, "freetext": false, "listOnEmpty": true, "values": [] },
      "sorter": "alphanum",
      "headerFilterFunc": "like",
      "blank": ""
    },
    "html": {
      "description": "Rich HTML content (rendered in cell)",
      "vertAlign": "top",
      "minWidth": 150,
      "formatter": "html",
      "cssClass": "li-col-html",
      "editor": "textarea",
      "sorter": "alphanum",
      "headerFilterFunc": "like",
      "blank": ""
    },
    "image": {
      "description": "Image thumbnail (URL stored in field)",
      "hozAlign": "center",
      "minWidth": 50,
      "width": 50,
      "formatter": "image",
      "formatterParams": { "height": "32px", "urlPrefix": "", "urlSuffix": "", "width": "32px" },
      "cssClass": "li-col-image",
      "editor": false,
      "sorter": "alphanum",
      "headerFilterFunc": "like",
      "blank": ""
    },
    "index": {
      "description": "Simple whole numbers (lookup values)",
      "hozAlign": "right",
      "minWidth": 40,
      "formatter": "number",
      "formatterParams": { "thousand": "", "precision": 0 },
      "cssClass": "li-col-integer",
      "editor": "number",
      "editorParams": { "min": null, "max": null, "step": 1 },
      "sorter": "number",
      "headerFilterFunc": ">=",
      "blank": "0",
      "zero": "0",
      "bottomCalc": "count",
      "bottomCalcFormatter": "number",
      "bottomCalcFormatterParams": { "thousand": "", "precision": 0 }
    },
    "integer": {
      "description": "Whole numbers (IDs, counts, references)",
      "hozAlign": "right",
      "minWidth": 40,
      "formatter": "number",
      "formatterParams": { "thousand": ",", "precision": 0 },
      "cssClass": "li-col-integer",
      "editor": "number",
      "editorParams": { "min": null, "max": null, "step": 1 },
      "sorter": "number",
      "headerFilterFunc": ">=",
      "blank": "",
      "zero": "",
      "bottomCalc": "sum",
      "bottomCalcFormatter": "number",
      "bottomCalcFormatterParams": { "thousand": ",", "precision": 0 }
    },
    "json": {
      "description": "JSON object or array (displayed as truncated string)",
      "vertAlign": "top",
      "minWidth": 120,
      "formatter": "plaintext",
      "cssClass": "li-col-json",
      "editor": "textarea",
      "sorter": "alphanum",
      "headerFilterFunc": "like",
      "blank": ""
    },
    "lookup": {
      "description": "Foreign key reference to a lookup table (displays label, stores ID)",
      "minWidth": 80,
      "formatter": "lookup",
      "formatterParams": { "lookupTable": null },
      "cssClass": "li-col-lookup",
      "editor": "list",
      "editorParams": { "allowEmpty": false, "autocomplete": true, "freetext": false, "listOnEmpty": true, "sort": "asc", "valuesLookup": true },
      "sorter": "alphanum",
      "headerFilterFunc": "like",
      "blank": ""
    },
    "lookupIcon": {
      "description": "Like lookup but displays an icon in the cell, shows icons only in dropdown",
      "hozAlign": "center",
      "minWidth": 50,
      "formatter": "icon",
      "cssClass": "li-col-lookup",
      "editor": "list",
      "editorParams": { "allowEmpty": false, "autocomplete": true, "freetext": false, "listOnEmpty": true, "sort": "asc", "valuesLookup": true, "itemFormatter": "iconOnly" },
      "sorter": "alphanum",
      "headerFilterFunc": "like",
      "lookupStyle": "icon",
      "lookupEdit": "icon",
      "blank": ""
    },
    "lookupIconText": {
      "description": "Like lookup but displays icon + name in cell and dropdown",
      "minWidth": 100,
      "formatter": "iconText",
      "cssClass": "li-col-lookup",
      "editor": "list",
      "editorParams": { "allowEmpty": false, "autocomplete": true, "freetext": false, "listOnEmpty": true, "sort": "asc", "valuesLookup": true, "itemFormatter": "iconWithText" },
      "sorter": "alphanum",
      "headerFilterFunc": "like",
      "lookupStyle": "iconText",
      "lookupEdit": "iconText",
      "blank": ""
    },
    "lookupIconList": {
      "description": "Like lookup but displays only icon in cell, icon + name in dropdown",
      "hozAlign": "center",
      "minWidth": 50,
      "formatter": "icon",
      "cssClass": "li-col-lookup",
      "editor": "list",
      "editorParams": { "allowEmpty": false, "autocomplete": true, "freetext": false, "listOnEmpty": true, "sort": "asc", "valuesLookup": true, "itemFormatter": "iconWithText" },
      "sorter": "alphanum",
      "headerFilterFunc": "like",
      "lookupStyle": "icon",
      "lookupEdit": "iconText",
      "blank": ""
    },
    "percent": {
      "description": "Percentage values (0-100 or 0.0-1.0)",
      "hozAlign": "right",
      "minWidth": 70,
      "formatter": "number",
      "formatterParams": { "precision": 1, "suffix": "%" },
      "cssClass": "li-col-percent",
      "editor": "number",
      "editorParams": { "min": 0, "max": 100, "step": 0.1 },
      "sorter": "number",
      "headerFilterFunc": ">=",
      "blank": "",
      "zero": "0.0%",
      "bottomCalc": "avg",
      "bottomCalcFormatter": "number",
      "bottomCalcFormatterParams": { "precision": 1, "suffix": "%" }
    },
    "progress": {
      "description": "Progress bar (0-100 numeric value displayed as bar)",
      "minWidth": 80,
      "width": 120,
      "formatter": "progress",
      "formatterParams": { "color": ["#ff0000", "#ffaa00", "#00cc00"], "legendAlign": "center", "legendColor": "#ffffff", "max": 100, "min": 0 },
      "cssClass": "li-col-progress",
      "editor": "number",
      "editorParams": { "min": 0, "max": 100, "step": 1 },
      "sorter": "number",
      "headerFilterFunc": ">=",
      "blank": "",
      "zero": 0,
      "bottomCalc": "avg",
      "bottomCalcFormatter": "progress",
      "bottomCalcFormatterParams": { "max": 100, "min": 0 }
    },
    "rownum": {
      "description": "Auto-incrementing row number (display only, not from data)",
      "hozAlign": "right",
      "minWidth": 40,
      "width": 50,
      "formatter": "rownum",
      "cssClass": "li-col-rownum",
      "editor": false,
      "sorter": "number",
      "headerSort": false,
      "headerFilterFunc": null,
      "headerFilterPlaceholder": null,
      "blank": "",
      "bottomCalc": "count",
      "bottomCalcFormatter": "number",
      "bottomCalcFormatterParams": { "thousand": "," }
    },
    "star": {
      "description": "Star rating (0-5)",
      "hozAlign": "center",
      "minWidth": 80,
      "width": 100,
      "formatter": "star",
      "formatterParams": { "stars": 5 },
      "cssClass": "li-col-star",
      "editor": "star",
      "sorter": "number",
      "headerFilterFunc": ">=",
      "blank": "",
      "zero": 0,
      "bottomCalc": "avg",
      "bottomCalcFormatter": "star"
    },
    "string": {
      "description": "General text/string values",
      "minWidth": 80,
      "formatter": "plaintext",
      "cssClass": "li-col-string",
      "editor": "input",
      "editorParams": { "search": false },
      "sorter": "alphanum",
      "headerFilter": true,
      "headerFilterFunc": "like",
      "groupable": true
    },
    "text": {
      "description": "Long text/multiline content (descriptions, notes)",
      "vertAlign": "top",
      "minWidth": 150,
      "formatter": "plaintext",
      "cssClass": "li-col-text",
      "editor": "textarea",
      "sorter": "alphanum",
      "headerFilterFunc": "like",
      "blank": ""
    },
    "time": {
      "description": "Time-only values (HH:mm or HH:mm:ss)",
      "hozAlign": "center",
      "minWidth": 60,
      "width": 80,
      "formatter": "datetime",
      "formatterParams": { "inputFormat": "HH:mm:ss", "outputFormat": "HH:mm", "invalidPlaceholder": "" },
      "cssClass": "li-col-time",
      "editor": "time",
      "sorter": "time",
      "sorterParams": { "format": "HH:mm:ss" },
      "headerFilterFunc": "like"
    },
    "url": {
      "description": "Hyperlink URL (clickable)",
      "minWidth": 120,
      "formatter": "link",
      "formatterParams": { "target": "_blank", "urlField": null },
      "cssClass": "li-col-url",
      "editor": "input",
      "editorParams": { "search": false },
      "sorter": "alphanum",
      "headerFilterFunc": "like",
      "blank": ""
    }
  }
}
]===]
                ${JSON_INGEST_END}, ${COMMON_VALUES});

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Populate ${SCHEMA_NAME} schema in Lookup ${LOOKUP_ID}'             AS name,
        [=[
            # Forward Migration ${MIGRATION}: Populate ${SCHEMA_NAME} schema in Lookup ${LOOKUP_ID}

            This migration inserts the coltypes.json schema into the lookups table
            as Lookup ${LOOKUP_ID}, Key ${KEY_IDX}. This schema defines all available
            column types for Tabulator tables.
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
            WHERE lookup_id = ${LOOKUP_ID}
            AND key_idx = ${KEY_IDX};

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Remove ${SCHEMA_NAME} schema from Lookup ${LOOKUP_ID}'             AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Remove ${SCHEMA_NAME} schema from Lookup ${LOOKUP_ID}

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
