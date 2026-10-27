-- Migration: acuranzo_1403.lua
-- Lookup 059 key 4 profile-manager/user-options tableDef
--
-- acuranzo_1181 inserted an empty collection {}. Lithium then loaded
-- 0 columns for profile-manager/user-options and could not group or
-- keep the index field. This writes the checked-in tabledef from
-- elements/003-lithium/config/tabulator/profile-manager/user-options.json.
--
-- luacheck: no max line length
-- luacheck: no unused args
--
-- CHANGELOG
-- 1.0.0 - 2026-10-27 - Fill Lookup 059 key 4 user-options tableDef

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "lookups"
cfg.MIGRATION = "1403"
cfg.LOOKUP_ID = "059"
cfg.KEY_IDX = "4"
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
            SET collection = ${JSON_INGEST_START}[===[
{
  "$schema": "../tabledef-schema.json",
  "$version": "1.0.0",
  "$description": "Column definitions for User Options (Lookup 060 Profile Sections). Matches Lithium table path profile-manager/user-options (Lookup 059 key 4; also known as user-profile-sections).",
  "table": "user-options",
  "title": "User Options",
  "queryRef": 26,
  "readonly": true,
  "selectableRows": 1,
  "layout": "fitColumns",
  "responsiveLayout": "collapse",
  "resizableColumns": true,
  "persistSort": false,
  "persistFilter": false,
  "columns": {
    "key_idx": {
      "title": "ID#",
      "field": "key_idx",
      "coltype": "index",
      "visible": false,
      "sort": true,
      "filter": true,
      "editable": false,
      "calculated": false,
      "primaryKey": true,
      "description": "Database primary key — hidden from display",
      "bottomCalc": null
    },
    "index": {
      "title": "Index",
      "field": "index",
      "coltype": "integer",
      "visible": false,
      "sort": true,
      "filter": true,
      "editable": false,
      "calculated": false,
      "primaryKey": false,
      "description": "Section index for page selection — hidden from display"
    },
    "section": {
      "title": "Section",
      "field": "section",
      "coltype": "string",
      "visible": false,
      "sort": true,
      "filter": true,
      "editable": false,
      "calculated": true,
      "primaryKey": false,
      "description": "Group/section the option belongs to",
      "group": 1,
      "width": 120
    },
    "icon": {
      "title": "<fa fa-wrench></fa>",
      "field": "icon",
      "coltype": "string",
      "visible": true,
      "sort": false,
      "filter": false,
      "editable": false,
      "calculated": true,
      "primaryKey": false,
      "description": "Icon for the option",
      "width": 28,
      "formatter": "html",
      "hozAlign": "center",
      "bottomCalc": "count"
    },
    "label": {
      "title": "Section",
      "field": "label",
      "coltype": "string",
      "visible": true,
      "sort": true,
      "filter": true,
      "editable": false,
      "calculated": true,
      "primaryKey": false,
      "description": "Section name"
    },
    "status": {
      "title": "Status",
      "field": "status_a1",
      "coltype": "lookup",
      "visible": false,
      "sort": false,
      "filter": false,
      "editable": false,
      "calculated": false,
      "primaryKey": false,
      "description": "Active/Inactive status",
      "lookupRef": "a1"
    }
  }
}
]===]${JSON_INGEST_END},
                value_txt = 'profile-manager/user-options'
            WHERE lookup_id = ${LOOKUP_ID}
            AND key_idx = ${KEY_IDX};

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Fill Lookup ${LOOKUP_ID} key ${KEY_IDX} user-options tableDef'                                                    AS name,
        [=[
            # Forward Migration ${MIGRATION}: user-options tableDef

            Updates Lookup ${LOOKUP_ID} key ${KEY_IDX} collection with the
            Profile Manager user-options column layout (icon, label,
            hidden index, grouped section). Sets value_txt to
            profile-manager/user-options.

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
            SET collection = ${JSON_INGEST_START}[===[
{}
]===]${JSON_INGEST_END},
                value_txt = 'user-profile-sections'
            WHERE lookup_id = ${LOOKUP_ID}
            AND key_idx = ${KEY_IDX};

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Clear Lookup ${LOOKUP_ID} key ${KEY_IDX} tableDef to empty object'                                                    AS name,
        [=[
            # Reverse Migration ${MIGRATION}: empty tableDef

            Restores the acuranzo_1181.lua empty collection {} and
            value_txt user-profile-sections.

        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})

return queries end
