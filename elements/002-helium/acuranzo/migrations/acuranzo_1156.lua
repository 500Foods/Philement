-- Migration: acuranzo_1156.lua
-- Tabulator Schema: lookups-list.json (Lookup 059, Key 2)

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.0.0 - 2026-03-17 - Initial creation
-- 1.1.0 - 2026-04-11 - Renamed properties: display→title, sort→headerSort, filter→headerFilter, group→groupable
-- 1.2.0 - 2026-04-11 - Removed redundant properties matching coltype defaults; moved overrides to top-level

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "lookups"
cfg.MIGRATION = "1156"
cfg.LOOKUP_ID = "059"
cfg.KEY_IDX = "2"
cfg.SCHEMA_NAME = "lookups-list"
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
                (${LOOKUP_ID}, ${KEY_IDX}, 1, '${SCHEMA_NAME}', 0, 2, '',
                [==[
                    # Lookups List Table Definition

                    Column definitions for the Lookups Manager parent table (left panel). Maps query
                    result fields from QueryRef 030 to display properties and coltypes.

                    ## Table Configuration

                    - **Table**: lookups-list
                    - **Title**: Lookups List
                    - **QueryRef**: 30 (Get Lookups List)
                    - **SearchQueryRef**: 31 (Get Lookups List + Search)
                    - **Readonly**: false
                    - **SelectableRows**: 1

                    ## Columns

                    - **key_idx**: ID# (integer, visible - the lookup ID)
                    - **value_txt**: Name (string, visible - the lookup name)
                    - **value_int**: Entries (integer, visible - count of entries)
                    - **lookup_count**: Total Entries (integer, visible)
                    - **lookup_size**: Total Size (integer, visible)
                    - **sort_seq**: Sort (integer, visible)
                    - **status_a1**: Status (lookup, references lookup 1)
                    - **summary**: Description (text, hidden)
                    - **code**: Code (string, hidden)
                    - **collection**: Collection (JSON, hidden)
                    - **valid_after**: Valid After (datetime, hidden)
                    - **valid_until**: Valid Until (datetime, hidden)
                    - **created_at**: Created (datetime, hidden)
                    - **updated_at**: Updated (datetime, hidden)

                    ## Schema Location

                    Source: `elements/003-lithium/config/tabulator/lookups/lookups-list.json`
                ]==],
                ${JSON_INGEST_START}
[===[
{
  "$schema": "../../tabledef-schema.json",
  "$version": "1.0.0",
  "$description": "Column definitions for the Lookups Manager parent table (left panel). Maps QueryRef 030 result fields to display properties.",

  "table": "lookups-list",
  "title": "Lookups List",
  "queryRef": 30,
  "searchQueryRef": 31,
  "readonly": false,
  "selectableRows": 1,
  "layout": "fitColumns",
  "responsiveLayout": "collapse",
  "resizableColumns": true,
  "persistSort": true,
  "persistFilter": true,
  "initialSort": [
    { "column": "sort_seq", "dir": "asc" }
  ],

  "columns": {

    "key_idx": {
      "title": "ID#",
      "field": "key_idx",
      "coltype": "index",
      "visible": true,
      "headerFilter": true,
      "editable": false,
      "primaryKey": true,
      "description": "Lookup ID (primary key) - identifies the lookup table",
      "width": 60
    },

    "value_txt": {
      "title": "Name",
      "field": "value_txt",
      "coltype": "string",
      "visible": true,
      "headerFilter": true,
      "editable": true,
      "description": "Lookup display name"
    },

    "value_int": {
      "title": "Entries",
      "field": "value_int",
      "coltype": "integer",
      "visible": true,
      "headerFilter": true,
      "editable": false,
      "calculated": true,
      "description": "Number of entries in this lookup (from lookup entry)",
      "width": 70,
      "formatterParams": {
        "thousand": ",",
        "precision": 0
      }
    },

    "lookup_count": {
      "title": "Total",
      "field": "lookup_count",
      "coltype": "integer",
      "visible": true,
      "headerFilter": true,
      "editable": false,
      "calculated": true,
      "description": "Total count of entries for this lookup (calculated)",
      "width": 70,
      "formatterParams": {
        "thousand": ",",
        "precision": 0
      }
    },

    "lookup_size": {
      "title": "Size",
      "field": "lookup_size",
      "coltype": "integer",
      "visible": false,
      "headerFilter": true,
      "editable": false,
      "calculated": true,
      "description": "Total size in bytes of all entries for this lookup",
      "width": 80,
      "formatterParams": {
        "thousand": ",",
        "precision": 0,
        "suffix": " B"
      }
    },

    "sort_seq": {
      "title": "Sort",
      "field": "sort_seq",
      "coltype": "integer",
      "visible": false,
      "headerFilter": true,
      "editable": true,
      "description": "Sort sequence for ordering lookups",
      "width": 60
    },

    "status_a1": {
      "title": "Status",
      "field": "status_a1",
      "coltype": "lookup",
      "visible": false,
      "headerFilter": true,
      "groupable": true,
      "editable": true,
      "description": "Lookup status — references lookup table 1 (Active, Inactive, etc.)",
      "lookupRef": "1"
    },

    "summary": {
      "title": "Description",
      "field": "summary",
      "coltype": "string",
      "visible": false,
      "headerFilter": true,
      "editable": true,
      "description": "Detailed description of the lookup's purpose"
    },

    "code": {
      "title": "Code",
      "field": "code",
      "coltype": "string",
      "visible": false,
      "headerFilter": true,
      "editable": true,
      "description": "Optional code associated with the lookup"
    },

    "collection": {
      "title": "Collection",
      "field": "collection",
      "coltype": "json",
      "visible": false,
      "headerFilter": false,
      "editable": true,
      "description": "JSON metadata for the lookup"
    },

    "valid_after": {
      "title": "Valid After",
      "field": "valid_after",
      "coltype": "datetime",
      "visible": false,
      "headerFilter": true,
      "editable": true,
      "description": "Timestamp when the lookup becomes valid"
    },

    "valid_until": {
      "title": "Valid Until",
      "field": "valid_until",
      "coltype": "datetime",
      "visible": false,
      "headerFilter": true,
      "editable": true,
      "description": "Timestamp when the lookup expires"
    },

    "created_at": {
      "title": "Created",
      "field": "created_at",
      "coltype": "datetime",
      "visible": false,
      "headerFilter": true,
      "editable": false,
      "calculated": true,
      "description": "Record creation timestamp"
    },

    "updated_at": {
      "title": "Updated",
      "field": "updated_at",
      "coltype": "datetime",
      "visible": false,
      "headerFilter": true,
      "editable": false,
      "calculated": true,
      "description": "Last modification timestamp"
    },

    "created_id": {
      "title": "Created By",
      "field": "created_id",
      "coltype": "index",
      "visible": false,
      "headerFilter": true,
      "editable": false,
      "calculated": true,
      "description": "User ID who created the record"
    },

    "updated_id": {
      "title": "Updated By",
      "field": "updated_id",
      "coltype": "index",
      "visible": false,
      "headerFilter": true,
      "editable": false,
      "calculated": true,
      "description": "User ID who last modified the record"
    },

    "record_size": {
      "title": "Size",
      "field": "record_size",
      "coltype": "integer",
      "visible": false,
      "headerFilter": true,
      "editable": false,
      "calculated": true,
      "description": "Calculated size of this record in bytes",
      "formatterParams": {
        "thousand": ",",
        "precision": 0,
        "suffix": " B"
      }
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

            This migration inserts the lookups-list.json table definition into the lookups table
            as Lookup ${LOOKUP_ID}, Key ${KEY_IDX}. This schema defines the column layout
            and configuration for the Lookups Manager parent table.
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
