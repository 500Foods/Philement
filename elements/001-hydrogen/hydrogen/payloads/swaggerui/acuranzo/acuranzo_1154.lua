-- Migration: acuranzo_1154.lua
-- LithiumTable JSON Schema (lookup_id 059, key_idx 1)

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.5.0 - 2026-07-02 - Use JSON_INGEST_SCHEMA so DB2 accepts JSON Schema $ref keys (JSON2BSON rejects nested $ref)
-- 1.4.0 - 2026-04-19 - Renamed groupOrd to groupDir for consistency with sortDir
-- 1.3.0 - 2026-04-12 - Rearranged migrations to better match evolving documentation
-- 1.2.0 - 2026-04-11 - Removed redundant properties matching coltype defaults; moved overrides to top-level
-- 1.1.0 - 2026-04-11 - Renamed properties: display→title, sort→headerSort, filter→headerFilter, group→groupable
-- 1.0.0 - 2026-03-15 - Initial creation

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "lookups"
cfg.MIGRATION = "1154"
cfg.LOOKUP_ID = "059"
cfg.KEY_IDX = "1"
cfg.TABLEDEF_NAME = "tabledef-json-schema"
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
                (${LOOKUP_ID}, ${KEY_IDX}, 1, '${TABLEDEF_NAME}', 0, 1, '',
                [==[
                    # tableDef JSON Schema

                    This is the JSON schema used to validate tableDefs after they are generated either
                    from the base data, after applying a tableDef from Lookup 59, or generating one from
                    one of the Column Managers.

                ]==],
                ${JSON_INGEST_SCHEMA_START}
[===[
{
  "$schema": "http://json-schema.org/draft-07/schema#",
  "$id": "https://philement.com/schemas/lithium-column-types.json",
  "title": "Lithium Column Types Schema",
  "description": "JSON Schema for LithiumTable column type definitions",
  "type": "object",
  "additionalProperties": true,
  "properties": {
    "default": { "$ref": "#/definitions/columnType" },
    "string": { "$ref": "#/definitions/columnType" },
    "text": { "$ref": "#/definitions/columnType" },
    "integer": { "$ref": "#/definitions/columnType" },
    "index": { "$ref": "#/definitions/columnType" },
    "decimal": { "$ref": "#/definitions/columnType" },
    "boolean": { "$ref": "#/definitions/columnType" },
    "date": { "$ref": "#/definitions/columnType" },
    "datetime": { "$ref": "#/definitions/columnType" },
    "time": { "$ref": "#/definitions/columnType" },
    "currency": { "$ref": "#/definitions/columnType" },
    "percent": { "$ref": "#/definitions/columnType" },
    "progress": { "$ref": "#/definitions/columnType" },
    "email": { "$ref": "#/definitions/columnType" },
    "url": { "$ref": "#/definitions/columnType" },
    "lookup": { "$ref": "#/definitions/columnType" },
    "lookupIcon": { "$ref": "#/definitions/columnType" },
    "lookupIconText": { "$ref": "#/definitions/columnType" },
    "lookupIconList": { "$ref": "#/definitions/columnType" },
    "enum": { "$ref": "#/definitions/columnType" },
    "html": { "$ref": "#/definitions/columnType" },
    "image": { "$ref": "#/definitions/columnType" },
    "color": { "$ref": "#/definitions/columnType" },
    "star": { "$ref": "#/definitions/columnType" },
    "rownum": { "$ref": "#/definitions/columnType" },
    "json": { "$ref": "#/definitions/columnType" }
  },
  "definitions": {
    "columnType": {
      "type": "object",
      "description": "A LithiumTable column type definition",
      "properties": {
        "description": {
          "type": "string",
          "description": "Human-readable description of the column type"
        },
        "title": {
          "type": "string",
          "default": "Column",
          "description": "Default column header title"
        },
        "field": {
          "type": ["string", "null"],
          "default": null,
          "description": "Data field name (JSON key)"
        },
        "visible": {
          "type": "boolean",
          "default": true,
          "description": "Show/hide column in table"
        },
        "hozAlign": {
          "type": "string",
          "enum": ["left", "center", "right"],
          "default": "left",
          "description": "Horizontal text alignment"
        },
        "vertAlign": {
          "type": "string",
          "enum": ["top", "middle", "bottom"],
          "default": "middle",
          "description": "Vertical text alignment"
        },
        "headerHozAlign": {
          "type": ["string", "null"],
          "enum": ["left", "center", "right", null],
          "default": null,
          "description": "Header title horizontal alignment"
        },
        "headerVertical": {
          "type": "boolean",
          "default": false,
          "description": "Rotate header text vertically"
        },
        "width": {
          "type": ["number", "null"],
          "default": null,
          "description": "Column width in pixels",
          "minimum": 0
        },
        "minWidth": {
          "type": "number",
          "default": 40,
          "description": "Minimum column width in pixels",
          "minimum": 0
        },
        "maxWidth": {
          "type": ["number", "null"],
          "default": null,
          "description": "Maximum column width in pixels",
          "minimum": 0
        },
        "frozen": {
          "type": ["boolean", "string"],
          "enum": [false, true, "right"],
          "default": false,
          "description": "Freeze position"
        },
        "resizable": {
          "type": "boolean",
          "default": true,
          "description": "Allow column resizing"
        },
        "responsive": {
          "type": ["number", "null"],
          "default": null,
          "description": "Responsive hide priority"
        },
        "formatter": {
          "type": "string",
          "enum": [
            "plaintext", "number", "money", "datetime", "tickCross",
            "star", "progress", "image", "link", "lookup",
            "html", "color", "rownum", "icon", "iconText"
          ],
          "default": "plaintext",
          "description": "Cell content formatter name"
        },
        "formatterParams": {
          "type": "object",
          "default": {},
          "description": "Parameters passed to the formatter"
        },
        "cssClass": {
          "type": "string",
          "default": "",
          "description": "CSS class applied to cells"
        },
        "cellStyle": {
          "type": ["string", "object", "null"],
          "default": null,
          "description": "Inline CSS for cells"
        },
        "headerStyle": {
          "type": ["string", "object", "null"],
          "default": null,
          "description": "Inline CSS for header"
        },
        "rowStyle": {
          "type": ["string", "object", "null"],
          "default": null,
          "description": "Inline CSS for entire row"
        },
        "rowHandle": {
          "type": "boolean",
          "default": false,
          "description": "Use column as row handle"
        },
        "tooltip": {
          "type": ["string", "null"],
          "default": null,
          "description": "Cell tooltip content"
        },
        "headerTooltip": {
          "type": ["string", "null"],
          "default": null,
          "description": "Header tooltip content"
        },
        "download": {
          "type": "boolean",
          "default": true,
          "description": "Include in download/export"
        },
        "headerWordWrap": {
          "type": "boolean",
          "default": false,
          "description": "Allow word wrap in header"
        },
        "editor": {
          "type": ["string", "boolean"],
          "enum": [
            "input", "number", "textarea", "list", "select",
            "date", "datetime-local", "time", "tickCross", "star", false
          ],
          "default": "input",
          "description": "Cell editor name"
        },
        "editorParams": {
          "type": "object",
          "default": {},
          "description": "Parameters passed to the editor"
        },
        "editable": {
          "type": ["boolean", "null"],
          "default": null,
          "description": "Conditional cell editing"
        },
        "validator": {
          "type": ["string", "array", "object", "null"],
          "default": null,
          "description": "Validation rules"
        },
        "sorter": {
          "type": "string",
          "enum": ["alphanum", "number", "date", "datetime", "time", "boolean"],
          "default": "alphanum",
          "description": "Sort function name"
        },
        "sorterParams": {
          "type": "object",
          "default": {},
          "description": "Parameters passed to the sorter"
        },
        "headerSort": {
          "type": "boolean",
          "default": true,
          "description": "Enable sorting on this column"
        },
        "sortPri": {
          "type": ["number", "null"],
          "default": null,
          "description": "Sort priority for multi-column sorting"
        },
        "headerFilter": {
          "type": "boolean",
          "default": false,
          "description": "Show header filter input"
        },
        "headerFilterFunc": {
          "type": ["string", "null"],
          "enum": [null, "=", "!=", "like", ">=", ">", "<=", "<"],
          "default": "like",
          "description": "Header filter function"
        },
        "headerFilterPlaceholder": {
          "type": "string",
          "default": "filter...",
          "description": "Filter input placeholder"
        },
        "groupable": {
          "type": "boolean",
          "default": false,
          "description": "Enable grouping by this column"
        },
        "groupPri": {
          "type": ["number", "null"],
          "default": null,
          "description": "Group priority"
        },
        "groupDir": {
          "type": "string",
          "enum": ["asc", "desc"],
          "default": "asc",
          "description": "Group direction"
        },
        "columnPri": {
          "type": ["number", "null"],
          "default": null,
          "description": "Display order priority"
        },
        "blank": {
          "type": ["string", "null"],
          "default": "",
          "description": "Display value when data is blank/empty"
        },
        "zero": {
          "type": ["string", "number", "boolean", "null"],
          "default": null,
          "description": "Display value when data is zero"
        },
        "accessor": {
          "type": ["string", "null"],
          "default": null,
          "description": "Transform data on read"
        },
        "accessorParams": {
          "type": "object",
          "default": {},
          "description": "Parameters for accessor"
        },
        "accessorDownload": {
          "type": ["string", "null"],
          "default": null,
          "description": "Transform data on download"
        },
        "mutator": {
          "type": ["string", "null"],
          "default": null,
          "description": "Transform data on write"
        },
        "mutatorParams": {
          "type": "object",
          "default": {},
          "description": "Parameters for mutator"
        },
        "bottomCalc": {
          "type": ["string", "null"],
          "enum": [null, "count", "sum", "avg", "min", "max"],
          "default": null,
          "description": "Footer calculation type"
        },
        "bottomCalcFormatter": {
          "type": ["string", "null"],
          "enum": [null, "plaintext", "number", "money", "datetime", "tickCross", "star", "progress"],
          "default": null,
          "description": "Formatter for calculation result"
        },
        "bottomCalcFormatterParams": {
          "type": "object",
          "default": {},
          "description": "Parameters for bottomCalcFormatter"
        },
        "bottomCalcHozAlign": {
          "type": ["string", "null"],
          "default": null,
          "description": "Footer cell horizontal alignment"
        },
        "bottomCalcStyle": {
          "type": ["string", "object", "null"],
          "default": null,
          "description": "Inline CSS for footer cell"
        },
        "lookupRef": {
          "type": ["string", "null"],
          "default": null,
          "description": "Lookup table reference identifier"
        },
        "lookupStyle": {
          "type": ["string", "null"],
          "enum": [null, "iconText", "icon", "text"],
          "default": null,
          "description": "How lookup values are displayed in cells"
        },
        "lookupEdit": {
          "type": ["string", "null"],
          "enum": [null, "iconText", "icon", "text"],
          "default": null,
          "description": "How lookup values are displayed in edit dropdown"
        },
        "contextMenu": {
          "type": ["array", "object", "null"],
          "default": null,
          "description": "Right-click context menu for cells"
        },
        "headerClickMenu": {
          "type": ["array", "object", "null"],
          "default": null,
          "description": "Menu shown when clicking column header"
        },
        "cellClick": {
          "type": ["object", "null"],
          "default": null,
          "description": "Cell click callback"
        },
        "headerClick": {
          "type": ["object", "null"],
          "default": null,
          "description": "Header click callback"
        }
      }
    }
  }
}
]===]
                ${JSON_INGEST_SCHEMA_END}, ${COMMON_VALUES});

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Populate ${TABLEDEF_NAME} tableDef in Lookup ${LOOKUP_ID}'             AS name,
        [=[
            # Forward Migration ${MIGRATION}: Populate ${TABLEDEF_NAME} in Lookup ${LOOKUP_ID}

            This migration inserts ${TABLEDEF_NAME} into the lookups table
            as Lookup ${LOOKUP_ID}, Key ${KEY_IDX}. This defines schema used to validate
            tableDefs for LithiumTable-based tables.
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
         'Remove ${TABLEDEF_NAME} tableDef from Lookup ${LOOKUP_ID}'             AS name,
         [=[
             # Reverse Migration ${MIGRATION}: Remove ${TABLEDEF_NAME} tableDef from Lookup ${LOOKUP_ID}

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
