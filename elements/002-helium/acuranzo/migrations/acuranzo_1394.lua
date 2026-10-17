-- Migration: acuranzo_1394.lua
-- Lookup 059 key 13 scripts/script-manager tableDef
--
-- CATCHUP Phase 7. Lookup 059 keys 0-12 are other tables. Scripting asks
-- for table path scripts/script-manager and previously fell through to
-- auto-discovery. value_txt is that path so the loader matches it.
-- deleteQueryRef 131 is recorded here. The navigator delete still cannot
-- call it: the primary key is the pair (group_name, script_name).

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.0.0 - 2026-10-16 - Lookup 059 key 13 scripts/script-manager tableDef

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "lookups"
cfg.MIGRATION = "1394"
cfg.LOOKUP_ID = "059"
cfg.KEY_IDX = "13"
cfg.TABLEDEF_NAME = "scripts/script-manager"
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
                (${LOOKUP_ID}, ${KEY_IDX}, 1, '${TABLEDEF_NAME}', 0, 13, '',
                [==[
                    # Script Manager Table Definition

                    Column definitions for the Scripting Manager list.
                    List QueryRef 089 omits `code`. Detail QueryRef 087
                    returns `code`. Delete QueryRef 131 takes
                    GROUP_NAME and SCRIPT_NAME, not an integer id.

                    ## Table Configuration

                    - **Table path**: scripts/script-manager
                    - **QueryRef**: 89 (List All Scripts)
                    - **SearchQueryRef**: 90 (Search Scripts)
                    - **DetailQueryRef**: 87 (Get Script by Group/Name)
                    - **InsertQueryRef**: 129 (Insert Script)
                    - **UpdateQueryRef**: 130 (Update Script)
                    - **DeleteQueryRef**: 131 (Delete Script)
                    - **Readonly**: false
                    - **SelectableRows**: 1
                ]==],
                ${JSON_INGEST_START}[===[
{
  "$schema": "../tabledef-schema.json",
  "$version": "1.0.0",
  "$description": "Scripting Manager list. Matches Lithium table path scripts/script-manager.",

  "table": "script-manager",
  "title": "Script Manager",
  "queryRef": 89,
  "searchQueryRef": 90,
  "detailQueryRef": 87,
  "insertQueryRef": 129,
  "updateQueryRef": 130,
  "deleteQueryRef": 131,
  "readonly": false,
  "selectableRows": 1,
  "layout": "fitColumns",
  "responsiveLayout": "collapse",
  "resizableColumns": true,
  "persistSort": true,
  "persistFilter": true,
  "initialSort": [
    { "column": "group_name", "dir": "asc" },
    { "column": "script_name", "dir": "asc" }
  ],

  "columns": {

    "group_name": {
      "title": "Group",
      "field": "group_name",
      "coltype": "string",
      "visible": true,
      "headerFilter": true,
      "editable": false,
      "primaryKey": true,
      "description": "Script group. Rename is forbidden. With script_name this is the primary key."
    },

    "script_name": {
      "title": "Name",
      "field": "script_name",
      "coltype": "string",
      "visible": true,
      "headerFilter": true,
      "editable": false,
      "primaryKey": true,
      "description": "Script name within the group. Rename is forbidden."
    },

    "script_type": {
      "title": "Type",
      "field": "script_type",
      "coltype": "lookup",
      "visible": true,
      "headerFilter": true,
      "groupable": true,
      "editable": true,
      "description": "Lookup 061 script type.",
      "lookupRef": "061",
      "editorParams": {
        "valuesLookup": "061"
      }
    },

    "status": {
      "title": "Status",
      "field": "status",
      "coltype": "lookup",
      "visible": true,
      "headerFilter": true,
      "groupable": true,
      "editable": true,
      "description": "Lookup 062 script status.",
      "lookupRef": "062",
      "editorParams": {
        "valuesLookup": "062"
      }
    },

    "invokable": {
      "title": "Invokable",
      "field": "invokable",
      "coltype": "integer",
      "visible": true,
      "headerFilter": true,
      "editable": true,
      "description": "1 allows POST /api/conduit/script. 0 is returned as script_not_found. INTEGER_SMALL, default 0.",
      "width": 110
    },

    "schedule": {
      "title": "Schedule",
      "field": "schedule",
      "coltype": "string",
      "visible": true,
      "headerFilter": true,
      "editable": true,
      "description": "Optional cron-like schedule. Empty means on demand."
    },

    "summary": {
      "title": "Summary",
      "field": "summary",
      "coltype": "string",
      "visible": true,
      "headerFilter": true,
      "editable": true,
      "description": "Markdown description."
    },

    "code": {
      "title": "Code",
      "field": "code",
      "coltype": "string",
      "visible": false,
      "headerFilter": false,
      "editable": false,
      "description": "Lua source. Present on detail QueryRef 087. The list query omits it."
    },

    "next_run": {
      "title": "Next Run",
      "field": "next_run",
      "coltype": "datetime",
      "visible": false,
      "headerFilter": true,
      "editable": false,
      "description": "Next scheduled run."
    },

    "last_run_start": {
      "title": "Last Start",
      "field": "last_run_start",
      "coltype": "datetime",
      "visible": false,
      "headerFilter": true,
      "editable": false,
      "description": "When the script last started."
    },

    "last_run_end": {
      "title": "Last End",
      "field": "last_run_end",
      "coltype": "datetime",
      "visible": false,
      "headerFilter": true,
      "editable": false,
      "description": "When the script last completed."
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

            Inserts Lookup ${LOOKUP_ID} key ${KEY_IDX} for the Scripting
            Manager list. `value_txt` is `scripts/script-manager`, the
            Lithium table path. `invokable` is an integer column.
            `deleteQueryRef` is 131 (GROUP_NAME + SCRIPT_NAME).
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

            Deletes Lookup ${LOOKUP_ID} key ${KEY_IDX}.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
return queries end
