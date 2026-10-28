-- Migration: acuranzo_1408.lua
-- Lookup 059 key 17 profile-manager/login-history tableDef
--
-- Lookup 059 keys 0-15 are in use. Key 16 is left free (the
-- Date Formats Luxon tokens table asks for it). Key 17 is the
-- Profile > Security > Login History "Recent Activity" grid.
-- Rows come from QueryRef 159 (acuranzo_1406); the page loads
-- them itself and pre-formats When/Result. The grid is readonly.
--
-- luacheck: no max line length
-- luacheck: no unused args
--
-- CHANGELOG
-- 1.0.0 - 2026-10-27 - Lookup 059 key 17 profile-manager/login-history

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "lookups"
cfg.MIGRATION = "1408"
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
            INSERT INTO ${SCHEMA}${TABLE}
                (lookup_id, key_idx, status_a1, value_txt, value_int, sort_seq, code, summary, collection, ${COMMON_FIELDS})
            VALUES
                (${LOOKUP_ID}, ${KEY_IDX}, 1, '${TABLEDEF_NAME}', 0, 17, '',
                [==[
                    # Login History Table Definition

                    Recent Activity grid on Profile > Security > Login History.
                    Columns: When (24h PCT), Result, Login ID, IP,
                    Duration (ms), App Version. Rows from QueryRef 159,
                    pre-formatted by the page. The grid is readonly.

                    ## Table Configuration

                    - **Table path**: profile-manager/login-history
                    - **QueryRef**: 159 (Get Login History, loaded by the page)
                    - **Readonly**: true
                    - **SelectableRows**: 1
                ]==],
                ${JSON_INGEST_START}[===[
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
                ${JSON_INGEST_END}, ${COMMON_VALUES});

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Populate ${TABLEDEF_NAME} tableDef in Lookup ${LOOKUP_ID}'        AS name,
        [=[
            # Forward Migration ${MIGRATION}: Populate ${TABLEDEF_NAME} tableDef in Lookup ${LOOKUP_ID}

            Inserts Lookup ${LOOKUP_ID} key ${KEY_IDX}. value_txt is
            profile-manager/login-history. Rows come from QueryRef 159.
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
        'Remove ${TABLEDEF_NAME} tableDef from Lookup ${LOOKUP_ID}'        AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Remove ${TABLEDEF_NAME} tableDef from Lookup ${LOOKUP_ID}

            Deletes Lookup ${LOOKUP_ID} key ${KEY_IDX}. Suggestions stay.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
return queries end
