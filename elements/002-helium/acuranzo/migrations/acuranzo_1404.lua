-- Migration: acuranzo_1404.lua
-- UPDATE Lookup 059 key 4 (profile-manager/user-options): groupable + icon minWidth

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.0.0 - 2026-10-27 - Replace dead "group":1 with groupable/groupPri/groupDir; icon
--         minWidth 24 (string coltype defaults to 80); label title Page; groupStartOpen false.
--         Synced with elements/003-lithium/config/tabulator/profile-manager/user-options.json.

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "lookups"
cfg.MIGRATION = "1404"
cfg.LOOKUP_ID = "059"
cfg.KEY_IDX = "4"
cfg.TABLEDEF_NAME = "profile-manager/user-options"
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
            UPDATE ${SCHEMA}${TABLE}
            SET value_txt = '${TABLEDEF_NAME}',
                collection = ${JSON_INGEST_START}[===[
{
  "$schema": "../tabledef-schema.json",
  "$version": "1.0.0",
  "$description": "Column definitions for User Options from Lookup 060 (App UI Lists). Displays profile sections and user preferences.",
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
      "description": "Database primary key \u2014 hidden from display",
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
      "description": "Section index for page selection \u2014 hidden from display"
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
      "width": 120,
      "groupable": true,
      "groupPri": 1,
      "groupDir": "asc"
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
      "width": 30,
      "formatter": "html",
      "hozAlign": "center",
      "bottomCalc": "count",
      "minWidth": 24
    },
    "label": {
      "title": "Page",
      "field": "label",
      "coltype": "string",
      "visible": true,
      "sort": true,
      "filter": true,
      "editable": false,
      "calculated": true,
      "primaryKey": false,
      "description": "Page name"
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
  },
  "groupStartOpen": false
}
]===]
                ${JSON_INGEST_END}
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx = ${KEY_IDX};

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Update ${TABLEDEF_NAME} tableDef: groupable + icon minWidth'       AS name,
        [=[
            # Forward Migration ${MIGRATION}: Update ${TABLEDEF_NAME} in Lookup ${LOOKUP_ID}

            Rewrites Lookup ${LOOKUP_ID} key ${KEY_IDX} collection so Profile left-nav
            groups by section (groupable/groupPri), icon column can be ~30px (minWidth
            overrides string coltype 80), and label title is Page. groupStartOpen false.
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
            UPDATE ${SCHEMA}${TABLE}
            SET value_txt = '${TABLEDEF_NAME}',
                collection = ${JSON_INGEST_START}[===[
{
  "$schema": "../tabledef-schema.json",
  "$version": "1.0.0",
  "$description": "Column definitions for User Options from Lookup 060 (App UI Lists). Displays profile sections and user preferences.",

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
]===]
                ${JSON_INGEST_END}
            WHERE lookup_id = ${LOOKUP_ID}
              AND key_idx = ${KEY_IDX};

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Reverse ${TABLEDEF_NAME} tableDef to pre-1404 shape'               AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Restore pre-1404 ${TABLEDEF_NAME}

            Restores Lookup ${LOOKUP_ID} key ${KEY_IDX} collection to the tabledef
            as filled by 1403 (dead "group":1, icon width 28 without minWidth,
            label title Section, no groupStartOpen).
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
return queries end
