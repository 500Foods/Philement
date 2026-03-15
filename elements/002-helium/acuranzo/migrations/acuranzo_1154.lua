-- Migration: acuranzo_1154.lua
-- Tabulator Schema: query-manager.json (Lookup 059, Key 1)

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.0.0 - 2026-03-15 - Initial creation

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "lookups"
cfg.MIGRATION = "1154"
cfg.LOOKUP_ID = "059"
cfg.KEY_IDX = "1"
cfg.SCHEMA_NAME = "query-manager"
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
                (${LOOKUP_ID}, ${KEY_IDX}, 1, '${SCHEMA_NAME}', 0, 1, '',
                [==[
                    # Query Manager Table Definition

                    Column definitions for the Query Manager table (left panel). Maps query
                    result fields to display properties and coltypes.

                    ## Table Configuration

                    - **Table**: query-manager
                    - **Title**: Query Manager
                    - **QueryRef**: 25 (Get Queries List)
                    - **SearchQueryRef**: 32 (Get Queries List + Search)
                    - **DetailQueryRef**: 27 (Get System Query)
                    - **Readonly**: false
                    - **SelectableRows**: 1

                    ## Columns

                    - **query_id**: ID# (hidden, primary key)
                    - **query_ref**: Ref (index, visible)
                    - **name**: Name (string, visible, editable)
                    - **query_status_a27**: Status (lookup, references lookup 27)
                    - **query_type_a28**: Type (lookup, references lookup 28)
                    - **query_dialect_a30**: Dialect (lookup, references lookup 30)
                    - **query_queue_a58**: Queue (lookup, references lookup 58)
                    - **query_timeout**: Timeout (integer, hidden)
                    - **created_at**: Created (datetime, hidden)
                    - **updated_at**: Updated (datetime, hidden)
                    - **created_by**: Created By (string, hidden)
                    - **updated_by**: Updated By (string, hidden)

                    ## Schema Location

                    Source: `elements/003-lithium/config/tabulator/queries/query-manager.json`
                ]==],
                ${JSON_INGEST_START}
[===[
{
  "$schema": "../tabledef-schema.json",
  "$version": "1.0.0",
  "$description": "Column definitions for the Query Manager table (left panel). Maps query result fields to display properties and coltypes.",

  "table": "query-manager",
  "title": "Query Manager",
  "queryRef": 25,
  "searchQueryRef": 32,
  "detailQueryRef": 27,
  "readonly": false,
  "selectableRows": 1,
  "layout": "fitColumns",
  "responsiveLayout": "collapse",
  "resizableColumns": true,
  "persistSort": true,
  "persistFilter": true,
  "initialSort": [
    { "column": "query_ref", "dir": "asc" }
  ],

  "columns": {

    "query_id": {
      "display": "ID#",
      "field": "query_id",
      "coltype": "index",
      "visible": false,
      "sort": true,
      "filter": true,
      "group": false,
      "editable": false,
      "calculated": false,
      "primaryKey": true,
      "description": "Database primary key — hidden from display, used for API calls",
      "overrides": {
        "bottomCalc": null
      }
    },

    "query_ref": {
      "display": "Ref",
      "field": "query_ref",
      "coltype": "index",
      "visible": true,
      "sort": true,
      "filter": true,
      "group": false,
      "editable": false,
      "calculated": false,
      "primaryKey": false,
      "description": "Human-readable query reference number",
      "overrides": {
        "width": 80,
        "bottomCalc": "count",
        "bottomCalcFormatter": "number",
        "bottomCalcFormatterParams": {
          "thousand": ""
        }
      }
    },

    "name": {
      "display": "Name",
      "field": "name",
      "coltype": "string",
      "visible": true,
      "sort": true,
      "filter": true,
      "group": false,
      "editable": true,
      "calculated": false,
      "primaryKey": false,
      "description": "Query display name"
    },

    "query_status_a27": {
      "display": "Status",
      "field": "query_status_a27",
      "coltype": "lookup",
      "visible": false,
      "sort": true,
      "filter": true,
      "group": true,
      "editable": true,
      "calculated": false,
      "primaryKey": false,
      "description": "Query status — references lookup table 27 (Active, Inactive, etc.)",
      "lookupRef": "27",
      "overrides": {
        "editorParams": {
          "valuesLookup": "27"
        }
      }
    },

    "query_type_a28": {
      "display": "Type",
      "field": "query_type_a28",
      "coltype": "lookup",
      "visible": false,
      "sort": true,
      "filter": true,
      "group": true,
      "editable": false,
      "calculated": false,
      "primaryKey": false,
      "description": "Query type — references lookup table 28 (SQL, Migration, Diagram, etc.)",
      "lookupRef": "28"
    },

    "query_dialect_a30": {
      "display": "Dialect",
      "field": "query_dialect_a30",
      "coltype": "lookup",
      "visible": false,
      "sort": true,
      "filter": true,
      "group": true,
      "editable": true,
      "calculated": false,
      "primaryKey": false,
      "description": "SQL dialect — references lookup table 30 (SQLite, PostgreSQL, MySQL, DB2)",
      "lookupRef": "30"
    },

    "query_queue_a58": {
      "display": "Queue",
      "field": "query_queue_a58",
      "coltype": "lookup",
      "visible": false,
      "sort": true,
      "filter": true,
      "group": true,
      "editable": true,
      "calculated": false,
      "primaryKey": false,
      "description": "Query execution queue — references lookup table 58 (Fast, Slow, etc.)",
      "lookupRef": "58"
    },

    "query_timeout": {
      "display": "Timeout",
      "field": "query_timeout",
      "coltype": "integer",
      "visible": false,
      "sort": true,
      "filter": true,
      "group": false,
      "editable": true,
      "calculated": false,
      "primaryKey": false,
      "description": "Query execution timeout in seconds",
      "overrides": {
        "formatterParams": {
          "thousand": ",",
          "precision": 0,
          "suffix": "s"
        }
      }
    },

   "created_at": {
      "display": "Created",
      "field": "created_at",
      "coltype": "datetime",
      "visible": false,
      "sort": true,
      "filter": true,
      "group": false,
      "editable": false,
      "calculated": true,
      "primaryKey": false,
      "description": "Record creation timestamp (system-generated)"
    },

    "updated_at": {
      "display": "Updated",
      "field": "updated_at",
      "coltype": "datetime",
      "visible": false,
      "sort": true,
      "filter": true,
      "group": false,
      "editable": false,
      "calculated": true,
      "primaryKey": false,
      "description": "Last modification timestamp (system-generated)"
    },

    "created_by": {
      "display": "Created By",
      "field": "created_by",
      "coltype": "string",
      "visible": false,
      "sort": true,
      "filter": true,
      "group": true,
      "editable": false,
      "calculated": true,
      "primaryKey": false,
      "description": "User who created the record (system-generated)"
    },

    "updated_by": {
      "display": "Updated By",
      "field": "updated_by",
      "coltype": "string",
      "visible": false,
      "sort": true,
      "filter": true,
      "group": true,
      "editable": false,
      "calculated": true,
      "primaryKey": false,
      "description": "User who last modified the record (system-generated)"
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

            This migration inserts the query-manager.json table definition into the lookups table
            as Lookup ${LOOKUP_ID}, Key ${KEY_IDX}. This schema defines the column layout
            and configuration for the Query Manager table.
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
