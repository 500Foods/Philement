-- Migration: acuranzo_1409.lua
-- Lookup 059 key 16 profile-manager/luxon-tokens tableDef
--
-- Lookup 059 keys 0-15 and 17 are in use. Key 16 has been requested
-- by the Date Formats Luxon tokens table (LuxonTokenTable,
-- lookupKeyIdx 16) without a row, so it fell back to auto-discovery.
-- Rows are static and built client-side. The grid is readonly.
--
-- luacheck: no max line length
-- luacheck: no unused args
--
-- CHANGELOG
-- 1.0.0 - 2026-10-27 - Lookup 059 key 16 profile-manager/luxon-tokens

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "lookups"
cfg.MIGRATION = "1409"
cfg.LOOKUP_ID = "059"
cfg.KEY_IDX = "16"
cfg.TABLEDEF_NAME = "profile-manager/luxon-tokens"
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
                (${LOOKUP_ID}, ${KEY_IDX}, 1, '${TABLEDEF_NAME}', 0, 16, '',
                [==[
                    # Luxon Tokens Table Definition

                    Luxon format token reference on Profile > Date Formats.
                    Columns: Group, Token, Description, Sample, Current
                    (hidden id key). Rows are static, built by
                    LuxonTokenTable. The grid is readonly.

                    ## Table Configuration

                    - **Table path**: profile-manager/luxon-tokens
                    - **QueryRef**: none (static data)
                    - **Readonly**: true
                    - **SelectableRows**: 1
                ]==],
                ${JSON_INGEST_START}[===[
{
  "$schema": "../tabledef-schema.json",
  "$version": "1.0.0",
  "$description": "Luxon format token reference on Profile > Date Formats. Lookup 059 key 16 (acuranzo_1409). Static rows built client-side by LuxonTokenTable.",
  "table": "luxon-tokens",
  "title": "Luxon Tokens",
  "queryRef": null,
  "readonly": true,
  "selectableRows": 1,
  "layout": "fitColumns",
  "responsiveLayout": "collapse",
  "resizableColumns": true,
  "persistSort": true,
  "persistFilter": false,
  "columns": {
    "id": {
      "title": "ID",
      "field": "id",
      "coltype": "string",
      "visible": false,
      "sort": true,
      "filter": false,
      "editable": false,
      "calculated": false,
      "primaryKey": true,
      "description": "Primary key (same as token)"
    },
    "group": {
      "title": "Group",
      "field": "group",
      "coltype": "string",
      "visible": true,
      "sort": true,
      "filter": true,
      "editable": false,
      "calculated": false,
      "primaryKey": false,
      "description": "Token category (Timezones, Years, Months, ...)",
      "width": 160
    },
    "token": {
      "title": "Token",
      "field": "token",
      "coltype": "string",
      "visible": true,
      "sort": true,
      "filter": true,
      "editable": false,
      "calculated": false,
      "primaryKey": false,
      "description": "Luxon format token",
      "width": 90,
      "bottomCalc": "count",
      "bottomCalcFormatter": "number",
      "bottomCalcFormatterParams": {
        "thousand": ""
      }
    },
    "description": {
      "title": "Description",
      "field": "description",
      "coltype": "string",
      "visible": true,
      "sort": true,
      "filter": true,
      "editable": false,
      "calculated": false,
      "primaryKey": false,
      "description": "What the token renders"
    },
    "example": {
      "title": "Sample",
      "field": "example",
      "coltype": "string",
      "visible": true,
      "sort": true,
      "filter": true,
      "editable": false,
      "calculated": false,
      "primaryKey": false,
      "description": "Token applied to the page's sample datetime",
      "width": 200
    },
    "current": {
      "title": "Current",
      "field": "current",
      "coltype": "string",
      "visible": true,
      "sort": true,
      "filter": true,
      "editable": false,
      "calculated": false,
      "primaryKey": false,
      "description": "Token applied to the current datetime",
      "width": 200
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
            profile-manager/luxon-tokens. Rows are static, built client-side.
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
