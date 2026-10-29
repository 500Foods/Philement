-- Migration: acuranzo_1412.lua
-- UPDATE Lookup 059 key 17 (profile-manager/login-history): Duration as HH:MM:SS.zzz
--
-- Replaces action_msecs (raw ms integer) with duration_txt (pre-formatted string).
-- Synced with elements/003-lithium/config/tabulator/profile-manager/login-history.json.
--
-- luacheck: no max line length
-- luacheck: no unused args
--
-- CHANGELOG
-- 1.0.0 - 2026-10-28 - Duration column HH:MM:SS.zzz (duration_txt)

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "lookups"
cfg.MIGRATION = "1412"
cfg.LOOKUP_ID = "059"
cfg.KEY_IDX = "17"
cfg.TABLEDEF_NAME = "profile-manager/login-history"
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
  "$description": "Read-only Recent Activity table on Profile > Security > Login History. Lookup 059 key 17 (acuranzo_1408/1412). Rows from QueryRef 159, pre-formatted client-side.",
  "table": "login-history",
  "title": "Login History",
  "queryRef": 159,
  "readonly": true,
  "selectableRows": 1,
  "layout": "fitColumns",
  "responsiveLayout": "collapse",
  "resizableColumns": true,
  "persistSort": true,
  "persistFilter": false,
  "initialSort": [
    {
      "column": "when_txt",
      "dir": "desc"
    }
  ],
  "columns": {
    "row_id": {
      "title": "#",
      "field": "row_id",
      "coltype": "index",
      "visible": false,
      "sort": true,
      "filter": false,
      "editable": false,
      "calculated": false,
      "primaryKey": true,
      "description": "Client-side row number"
    },
    "when_txt": {
      "title": "When",
      "field": "when_txt",
      "coltype": "string",
      "visible": true,
      "sort": true,
      "filter": true,
      "editable": false,
      "calculated": false,
      "primaryKey": false,
      "description": "Login event time (24h, PCT)",
      "width": 170,
      "bottomCalc": "count",
      "bottomCalcFormatter": "number",
      "bottomCalcFormatterParams": {
        "thousand": ""
      }
    },
    "result_txt": {
      "title": "Result",
      "field": "result_txt",
      "coltype": "string",
      "visible": true,
      "sort": true,
      "filter": true,
      "editable": false,
      "calculated": false,
      "primaryKey": false,
      "description": "Login event type (Attempt / Success / Passed)",
      "width": 100
    },
    "login_id": {
      "title": "Login ID",
      "field": "login_id",
      "coltype": "string",
      "visible": true,
      "sort": true,
      "filter": true,
      "editable": false,
      "calculated": false,
      "primaryKey": false,
      "description": "Login ID",
      "width": 160
    },
    "ip_address": {
      "title": "IP",
      "field": "ip_address",
      "coltype": "string",
      "visible": true,
      "sort": true,
      "filter": true,
      "editable": false,
      "calculated": false,
      "primaryKey": false,
      "description": "IP",
      "width": 130
    },
    "location_txt": {
      "title": "Location",
      "field": "location_txt",
      "coltype": "string",
      "visible": true,
      "sort": true,
      "filter": true,
      "editable": false,
      "calculated": false,
      "primaryKey": false,
      "description": "City, region, country from Traefik GeoIP (X-GeoIP2-*) at login time",
      "width": 200
    },
    "duration_txt": {
      "title": "Duration",
      "field": "duration_txt",
      "coltype": "string",
      "visible": true,
      "sort": true,
      "filter": true,
      "editable": false,
      "calculated": false,
      "primaryKey": false,
      "description": "Session length as HH:MM:SS.zzz (from token valid_after to valid_until)",
      "width": 120
    },
    "app_version": {
      "title": "App Version",
      "field": "app_version",
      "coltype": "string",
      "visible": true,
      "sort": true,
      "filter": true,
      "editable": false,
      "calculated": false,
      "primaryKey": false,
      "description": "App Version"
    }
  }
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
        'Update ${TABLEDEF_NAME}: Duration as HH:MM:SS.zzz'                AS name,
        [=[
            # Forward Migration ${MIGRATION}: Update ${TABLEDEF_NAME} in Lookup ${LOOKUP_ID}

            Replaces action_msecs (integer ms) with duration_txt (HH:MM:SS.zzz
            string pre-formatted client-side by formatLoginDuration).
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
  "$description": "Read-only Recent Activity table on Profile > Security > Login History. Lookup 059 key 17 (acuranzo_1408). Rows from QueryRef 159, pre-formatted client-side.",
  "table": "login-history",
  "title": "Login History",
  "queryRef": 159,
  "readonly": true,
  "selectableRows": 1,
  "layout": "fitColumns",
  "responsiveLayout": "collapse",
  "resizableColumns": true,
  "persistSort": true,
  "persistFilter": false,
  "initialSort": [
    {
      "column": "when_txt",
      "dir": "desc"
    }
  ],
  "columns": {
    "row_id": {
      "title": "#",
      "field": "row_id",
      "coltype": "index",
      "visible": false,
      "sort": true,
      "filter": false,
      "editable": false,
      "calculated": false,
      "primaryKey": true,
      "description": "Client-side row number"
    },
    "when_txt": {
      "title": "When",
      "field": "when_txt",
      "coltype": "string",
      "visible": true,
      "sort": true,
      "filter": true,
      "editable": false,
      "calculated": false,
      "primaryKey": false,
      "description": "Login event time (24h, PCT)",
      "width": 170,
      "bottomCalc": "count",
      "bottomCalcFormatter": "number",
      "bottomCalcFormatterParams": {
        "thousand": ""
      }
    },
    "result_txt": {
      "title": "Result",
      "field": "result_txt",
      "coltype": "string",
      "visible": true,
      "sort": true,
      "filter": true,
      "editable": false,
      "calculated": false,
      "primaryKey": false,
      "description": "Login event type (Attempt / Success / Passed)",
      "width": 100
    },
    "login_id": {
      "title": "Login ID",
      "field": "login_id",
      "coltype": "string",
      "visible": true,
      "sort": true,
      "filter": true,
      "editable": false,
      "calculated": false,
      "primaryKey": false,
      "description": "Login ID",
      "width": 160
    },
    "ip_address": {
      "title": "IP",
      "field": "ip_address",
      "coltype": "string",
      "visible": true,
      "sort": true,
      "filter": true,
      "editable": false,
      "calculated": false,
      "primaryKey": false,
      "description": "IP",
      "width": 130
    },
    "action_msecs": {
      "title": "Duration (ms)",
      "field": "action_msecs",
      "coltype": "integer",
      "visible": true,
      "sort": true,
      "filter": true,
      "editable": false,
      "calculated": false,
      "primaryKey": false,
      "description": "Login duration in milliseconds",
      "width": 110
    },
    "app_version": {
      "title": "App Version",
      "field": "app_version",
      "coltype": "string",
      "visible": true,
      "sort": true,
      "filter": true,
      "editable": false,
      "calculated": false,
      "primaryKey": false,
      "description": "App Version"
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
        'Reverse ${TABLEDEF_NAME} Duration column to action_msecs'        AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Restore pre-1412 ${TABLEDEF_NAME}

            Restores Lookup ${LOOKUP_ID} key ${KEY_IDX} collection to the
            acuranzo_1408 shape (action_msecs integer Duration (ms)).
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
return queries end
