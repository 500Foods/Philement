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
cfg.SCHEMA_NAME = "coltypes"
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
  "$schema": "coltypes-schema.json",
  "$version": "1.0.0",
  "$description": "Lithium Tabulator column type definitions. Each coltype defines display formatting, alignment, editor behavior, and value-handling rules for columns of that data type.",

  "coltypes": {

    "integer": {
      "description": "Whole numbers (IDs, counts, references)",
      "align": "right",
      "vertAlign": "middle",
      "formatter": "number",
      "formatterParams": {
        "thousand": ",",
        "precision": 0
      },
      "editor": "number",
      "editorParams": {
        "min": null,
        "max": null,
        "step": 1
      },
      "sorter": "number",
      "headerFilterFunc": ">=",
      "headerFilterPlaceholder": "filter...",
      "blank": "",
      "zero": "",
      "cssClass": "li-col-integer",
      "bottomCalc": "sum",
      "bottomCalcFormatter": "number",
      "bottomCalcFormatterParams": {
        "thousand": ",",
        "precision": 0
      },
      "accessorDownload": null,
      "width": null,
      "minWidth": 40
    },

    "index": {
      "description": "Simple whole numbers (lookup values)",
      "align": "right",
      "vertAlign": "middle",
      "formatter": "number",
      "formatterParams": {
        "thousand": "",
        "precision": 0
      },
      "editor": "number",
      "editorParams": {
        "min": null,
        "max": null,
        "step": 1
      },
      "sorter": "number",
      "headerFilterFunc": ">=",
      "headerFilterPlaceholder": "filter...",
      "blank": "",
      "zero": "",
      "cssClass": "li-col-integer",
      "bottomCalc": "count",
      "bottomCalcFormatter": "number",
      "bottomCalcFormatterParams": {
        "thousand": "",
        "precision": 0
      },
      "accessorDownload": null,
      "width": null,
      "minWidth": 40
    },

    "decimal": {
      "description": "Floating-point numbers (prices, measurements, rates)",
      "align": "right",
      "vertAlign": "middle",
      "formatter": "number",
      "formatterParams": {
        "thousand": ",",
        "precision": 2,
        "decimal": "."
      },
      "editor": "number",
      "editorParams": {
        "min": null,
        "max": null,
        "step": 0.01
      },
      "sorter": "number",
      "headerFilterFunc": ">=",
      "headerFilterPlaceholder": "filter...",
      "blank": "",
      "zero": "0.00",
      "cssClass": "li-col-decimal",
      "bottomCalc": "sum",
      "bottomCalcFormatter": "number",
      "bottomCalcFormatterParams": {
        "thousand": ",",
        "precision": 2
      },
      "accessorDownload": null,
      "width": null,
      "minWidth": 80
    },

    "currency": {
      "description": "Monetary values with currency symbol",
      "align": "right",
      "vertAlign": "middle",
      "formatter": "money",
      "formatterParams": {
        "thousand": ",",
        "decimal": ".",
        "symbol": "$",
        "symbolAfter": false,
        "precision": 2
      },
      "editor": "number",
      "editorParams": {
        "min": 0,
        "max": null,
        "step": 0.01
      },
      "sorter": "number",
      "headerFilterFunc": ">=",
      "headerFilterPlaceholder": "filter...",
      "blank": "",
      "zero": "$0.00",
      "cssClass": "li-col-currency",
      "bottomCalc": "sum",
      "bottomCalcFormatter": "money",
      "bottomCalcFormatterParams": {
        "thousand": ",",
        "decimal": ".",
        "symbol": "$",
        "precision": 2
      },
      "accessorDownload": null,
      "width": null,
      "minWidth": 90
    },

    "percent": {
      "description": "Percentage values (0–100 or 0.0–1.0)",
      "align": "right",
      "vertAlign": "middle",
      "formatter": "number",
      "formatterParams": {
        "precision": 1,
        "suffix": "%"
      },
      "editor": "number",
      "editorParams": {
        "min": 0,
        "max": 100,
        "step": 0.1
      },
      "sorter": "number",
      "headerFilterFunc": ">=",
      "headerFilterPlaceholder": "filter...",
      "blank": "",
      "zero": "0.0%",
      "cssClass": "li-col-percent",
      "bottomCalc": "avg",
      "bottomCalcFormatter": "number",
      "bottomCalcFormatterParams": {
        "precision": 1,
        "suffix": "%"
      },
      "accessorDownload": null,
      "width": null,
      "minWidth": 70
    },

    "string": {
      "description": "General text/string values",
      "align": "left",
      "vertAlign": "middle",
      "formatter": "plaintext",
      "formatterParams": {},
      "editor": "input",
      "editorParams": {
        "search": false
      },
      "sorter": "alphanum",
      "headerFilterFunc": "like",
      "headerFilterPlaceholder": "filter...",
      "blank": "",
      "zero": null,
      "cssClass": "li-col-string",
      "bottomCalc": null,
      "accessorDownload": null,
      "width": null,
      "minWidth": 80
    },

    "text": {
      "description": "Long text / multiline content (descriptions, notes)",
      "align": "left",
      "vertAlign": "top",
      "formatter": "plaintext",
      "formatterParams": {},
      "editor": "textarea",
      "editorParams": {},
      "sorter": "alphanum",
      "headerFilterFunc": "like",
      "headerFilterPlaceholder": "filter...",
      "blank": "",
      "zero": null,
      "cssClass": "li-col-text",
      "bottomCalc": null,
      "accessorDownload": null,
      "width": null,
      "minWidth": 150
    },

    "html": {
      "description": "Rich HTML content (rendered in cell)",
      "align": "left",
      "vertAlign": "top",
      "formatter": "html",
      "formatterParams": {},
      "editor": "textarea",
      "editorParams": {},
      "sorter": "alphanum",
      "headerFilterFunc": "like",
      "headerFilterPlaceholder": "filter...",
      "blank": "",
      "zero": null,
      "cssClass": "li-col-html",
      "bottomCalc": null,
      "accessorDownload": null,
      "width": null,
      "minWidth": 150
    },

    "boolean": {
      "description": "True/false toggle values",
      "align": "center",
      "vertAlign": "middle",
      "formatter": "tickCross",
      "formatterParams": {
        "allowEmpty": true,
        "allowTruthy": true
      },
      "editor": "tickCross",
      "editorParams": {
        "tristate": false,
        "indeterminateValue": null
      },
      "sorter": "boolean",
      "headerFilterFunc": "=",
      "headerFilterPlaceholder": "filter...",
      "blank": null,
      "zero": false,
      "cssClass": "li-col-boolean",
      "bottomCalc": null,
      "accessorDownload": null,
      "width": 70,
      "minWidth": 50
    },

    "date": {
      "description": "Date values (no time component)",
      "align": "center",
      "vertAlign": "middle",
      "formatter": "datetime",
      "formatterParams": {
        "inputFormat": "yyyy-MM-dd",
        "outputFormat": "yyyy-MM-dd",
        "invalidPlaceholder": ""
      },
      "editor": "date",
      "editorParams": {},
      "sorter": "date",
      "sorterParams": {
        "format": "yyyy-MM-dd"
      },
      "headerFilterFunc": "like",
      "headerFilterPlaceholder": "filter...",
      "blank": "",
      "zero": null,
      "cssClass": "li-col-date",
      "bottomCalc": null,
      "accessorDownload": null,
      "width": 110,
      "minWidth": 90
    },

    "datetime": {
      "description": "Date + time values (timestamps)",
      "align": "center",
      "vertAlign": "middle",
      "formatter": "datetime",
      "formatterParams": {
        "inputFormat": "yyyy-MM-dd'T'HH:mm:ss",
        "outputFormat": "yyyy-MM-dd HH:mm",
        "invalidPlaceholder": ""
      },
      "editor": "datetime-local",
      "editorParams": {},
      "sorter": "datetime",
      "sorterParams": {
        "format": "yyyy-MM-dd'T'HH:mm:ss"
      },
      "headerFilterFunc": "like",
      "headerFilterPlaceholder": "filter...",
      "blank": "",
      "zero": null,
      "cssClass": "li-col-datetime",
      "bottomCalc": null,
      "accessorDownload": null,
      "width": 150,
      "minWidth": 120
    },

    "time": {
      "description": "Time-only values (HH:mm or HH:mm:ss)",
      "align": "center",
      "vertAlign": "middle",
      "formatter": "datetime",
      "formatterParams": {
        "inputFormat": "HH:mm:ss",
        "outputFormat": "HH:mm",
        "invalidPlaceholder": ""
      },
      "editor": "time",
      "editorParams": {},
      "sorter": "time",
      "sorterParams": {
        "format": "HH:mm:ss"
      },
      "headerFilterFunc": "like",
      "headerFilterPlaceholder": "filter...",
      "blank": "",
      "zero": null,
      "cssClass": "li-col-time",
      "bottomCalc": null,
      "accessorDownload": null,
      "width": 80,
      "minWidth": 60
    },

    "lookup": {
      "description": "Foreign key reference to a lookup table (displays label, stores ID)",
      "align": "left",
      "vertAlign": "middle",
      "formatter": "lookup",
      "formatterParams": {
        "lookupTable": null
      },
      "editor": "list",
      "editorParams": {
        "valuesLookup": true,
        "autocomplete": true,
        "listOnEmpty": true,
        "freetext": false,
        "allowEmpty": false,
        "sort": "asc"
      },
      "sorter": "alphanum",
      "headerFilterFunc": "like",
      "headerFilterPlaceholder": "filter...",
      "blank": "",
      "zero": null,
      "cssClass": "li-col-lookup",
      "bottomCalc": null,
      "accessorDownload": null,
      "width": null,
      "minWidth": 80
    },

    "enum": {
      "description": "Fixed set of values (status codes, categories)",
      "align": "left",
      "vertAlign": "middle",
      "formatter": "plaintext",
      "formatterParams": {},
      "editor": "list",
      "editorParams": {
        "values": [],
        "autocomplete": true,
        "listOnEmpty": true,
        "freetext": false,
        "allowEmpty": false
      },
      "sorter": "alphanum",
      "headerFilterFunc": "like",
      "headerFilterPlaceholder": "filter...",
      "blank": "",
      "zero": null,
      "cssClass": "li-col-enum",
      "bottomCalc": null,
      "accessorDownload": null,
      "width": null,
      "minWidth": 80
    },

    "email": {
      "description": "Email address (clickable mailto link)",
      "align": "left",
      "vertAlign": "middle",
      "formatter": "link",
      "formatterParams": {
        "urlPrefix": "mailto:",
        "target": "_blank"
      },
      "editor": "input",
      "editorParams": {
        "search": false
      },
      "sorter": "alphanum",
      "headerFilterFunc": "like",
      "headerFilterPlaceholder": "filter...",
      "blank": "",
      "zero": null,
      "cssClass": "li-col-email",
      "bottomCalc": null,
      "accessorDownload": null,
      "width": null,
      "minWidth": 120
    },

    "url": {
      "description": "Hyperlink URL (clickable)",
      "align": "left",
      "vertAlign": "middle",
      "formatter": "link",
      "formatterParams": {
        "target": "_blank",
        "urlField": null
      },
      "editor": "input",
      "editorParams": {
        "search": false
      },
      "sorter": "alphanum",
      "headerFilterFunc": "like",
      "headerFilterPlaceholder": "filter...",
      "blank": "",
      "zero": null,
      "cssClass": "li-col-url",
      "bottomCalc": null,
      "accessorDownload": null,
      "width": null,
      "minWidth": 120
    },

    "image": {
      "description": "Image thumbnail (URL stored in field)",
      "align": "center",
      "vertAlign": "middle",
      "formatter": "image",
      "formatterParams": {
        "height": "32px",
        "width": "32px",
        "urlPrefix": "",
        "urlSuffix": ""
      },
      "editor": false,
      "editorParams": {},
      "sorter": "alphanum",
      "headerFilterFunc": "like",
      "headerFilterPlaceholder": "filter...",
      "blank": "",
      "zero": null,
      "cssClass": "li-col-image",
      "bottomCalc": null,
      "accessorDownload": null,
      "width": 50,
      "minWidth": 40
    },

    "color": {
      "description": "Color swatch (hex color value)",
      "align": "center",
      "vertAlign": "middle",
      "formatter": "color",
      "formatterParams": {},
      "editor": "input",
      "editorParams": {
        "search": false
      },
      "sorter": "alphanum",
      "headerFilterFunc": "like",
      "headerFilterPlaceholder": "filter...",
      "blank": "",
      "zero": null,
      "cssClass": "li-col-color",
      "bottomCalc": null,
      "accessorDownload": null,
      "width": 60,
      "minWidth": 40
    },

    "progress": {
      "description": "Progress bar (0–100 numeric value displayed as bar)",
      "align": "left",
      "vertAlign": "middle",
      "formatter": "progress",
      "formatterParams": {
        "min": 0,
        "max": 100,
        "color": ["#ff0000", "#ffaa00", "#00cc00"],
        "legendColor": "#ffffff",
        "legendAlign": "center"
      },
      "editor": "number",
      "editorParams": {
        "min": 0,
        "max": 100,
        "step": 1
      },
      "sorter": "number",
      "headerFilterFunc": ">=",
      "headerFilterPlaceholder": "filter...",
      "blank": "",
      "zero": 0,
      "cssClass": "li-col-progress",
      "bottomCalc": "avg",
      "bottomCalcFormatter": "progress",
      "bottomCalcFormatterParams": {
        "min": 0,
        "max": 100
      },
      "accessorDownload": null,
      "width": 120,
      "minWidth": 80
    },

    "star": {
      "description": "Star rating (0–5)",
      "align": "center",
      "vertAlign": "middle",
      "formatter": "star",
      "formatterParams": {
        "stars": 5
      },
      "editor": "star",
      "editorParams": {},
      "sorter": "number",
      "headerFilterFunc": ">=",
      "headerFilterPlaceholder": "filter...",
      "blank": "",
      "zero": 0,
      "cssClass": "li-col-star",
      "bottomCalc": "avg",
      "bottomCalcFormatter": "star",
      "bottomCalcFormatterParams": {},
      "accessorDownload": null,
      "width": 100,
      "minWidth": 80
    },

    "rownum": {
      "description": "Auto-incrementing row number (display only, not from data)",
      "align": "right",
      "vertAlign": "middle",
      "formatter": "rownum",
      "formatterParams": {},
      "editor": false,
      "editorParams": {},
      "sorter": "number",
      "headerFilterFunc": null,
      "headerFilterPlaceholder": null,
      "blank": "",
      "zero": null,
      "cssClass": "li-col-rownum",
      "bottomCalc": "count",
      "bottomCalcFormatter": "number",
      "bottomCalcFormatterParams": {
        "thousand": ","
      },
      "accessorDownload": null,
      "width": 50,
      "minWidth": 40
    },

    "json": {
      "description": "JSON object or array (displayed as truncated string)",
      "align": "left",
      "vertAlign": "top",
      "formatter": "plaintext",
      "formatterParams": {},
      "editor": "textarea",
      "editorParams": {},
      "sorter": "alphanum",
      "headerFilterFunc": "like",
      "headerFilterPlaceholder": "filter...",
      "blank": "",
      "zero": null,
      "cssClass": "li-col-json",
      "bottomCalc": null,
      "accessorDownload": null,
      "width": null,
      "minWidth": 120
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
