-- Migration: acuranzo_1399.lua
-- Lookup 059 key 15 courses/course-builder tableDef
--
-- Lookup 059 keys 0-14 already exist. Key 14 is
-- courses/course-manager. value_txt is courses/course-builder so
-- the Course Builder grid resolves this definition. List QueryRef
-- 4513. The grid is readonly. Working title is saved by QueryRef
-- 4514, not by a row update.
--
-- luacheck: no max line length
-- luacheck: no unused args
--
-- CHANGELOG
-- 1.0.0 - 2026-10-23 - Lookup 059 key 15 courses/course-builder tableDef

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "lookups"
cfg.MIGRATION = "1399"
cfg.LOOKUP_ID = "059"
cfg.KEY_IDX = "15"
cfg.TABLEDEF_NAME = "courses/course-builder"
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
                (${LOOKUP_ID}, ${KEY_IDX}, 1, '${TABLEDEF_NAME}', 0, 15, '',
                [==[
                    # Course Builder Table Definition

                    Column definitions for the suggestion list.
                    QueryRef 4513 returns every suggestion. Visible
                    columns are Working Title, Last activity, and
                    State. The grid is readonly.

                    ## Table Configuration

                    - **Table path**: courses/course-builder
                    - **QueryRef**: 4513 (Staff: List Suggestions)
                    - **Readonly**: true
                    - **SelectableRows**: 1
                ]==],
                ${JSON_INGEST_START}[===[
{
  "$schema": "../tabledef-schema.json",
  "$version": "1.0.0",
  "$description": "Course Builder suggestion list. Matches Lithium table path courses/course-builder.",
  "table": "course-builder",
  "title": "Course Builder",
  "queryRef": 4513,
  "readonly": true,
  "selectableRows": 1,
  "layout": "fitColumns",
  "responsiveLayout": "collapse",
  "resizableColumns": true,
  "persistSort": true,
  "persistFilter": false,
  "initialSort": [
    { "column": "last_activity", "dir": "desc" },
    { "column": "suggestion_id", "dir": "desc" }
  ],
  "columns": {
    "working_title": {
      "title": "Working Title",
      "field": "working_title",
      "coltype": "string",
      "visible": true,
      "headerFilter": false,
      "editable": false,
      "description": "Draft working title, or the submitted course name when no draft exists."
    },
    "last_activity": {
      "title": "Last activity",
      "field": "last_activity",
      "coltype": "datetime",
      "visible": true,
      "headerFilter": false,
      "editable": false,
      "description": "Draft updated time, or the suggestion created time when no draft exists."
    },
    "state_label": {
      "title": "State",
      "field": "state_label",
      "coltype": "string",
      "visible": true,
      "headerFilter": false,
      "editable": false,
      "description": "Suggested when the draft state is suggested or when no draft exists."
    },
    "suggestion_id": {
      "title": "Id",
      "field": "suggestion_id",
      "coltype": "integer",
      "visible": false,
      "headerFilter": false,
      "editable": false,
      "primaryKey": true,
      "description": "course_suggestions.suggestion_id."
    },
    "course_name": {
      "title": "Suggested Course Name",
      "field": "course_name",
      "coltype": "string",
      "visible": false,
      "headerFilter": false,
      "editable": false,
      "description": "Submitted name. A working-title save does not change it."
    },
    "summary": {
      "title": "Summary",
      "field": "summary",
      "coltype": "string",
      "visible": false,
      "headerFilter": false,
      "editable": false,
      "description": "Submitted summary."
    },
    "detail": {
      "title": "Detail",
      "field": "detail",
      "coltype": "string",
      "visible": false,
      "headerFilter": false,
      "editable": false,
      "description": "Submitted detail. The tab sanitizes this HTML."
    },
    "intended_audience": {
      "title": "Audience",
      "field": "intended_audience",
      "coltype": "string",
      "visible": false,
      "headerFilter": false,
      "editable": false,
      "description": "Submitted audience."
    },
    "additional_features": {
      "title": "Features",
      "field": "additional_features",
      "coltype": "string",
      "visible": false,
      "headerFilter": false,
      "editable": false,
      "description": "Submitted additional features."
    },
    "submitter_name": {
      "title": "Name",
      "field": "submitter_name",
      "coltype": "string",
      "visible": false,
      "headerFilter": false,
      "editable": false,
      "description": "Submitter name."
    },
    "submitter_email": {
      "title": "Email",
      "field": "submitter_email",
      "coltype": "string",
      "visible": false,
      "headerFilter": false,
      "editable": false,
      "description": "Submitter email."
    },
    "draft_title": {
      "title": "Draft title",
      "field": "draft_title",
      "coltype": "string",
      "visible": false,
      "headerFilter": false,
      "editable": false,
      "description": "Stored working title. Null when no draft exists."
    },
    "draft_state": {
      "title": "Draft state",
      "field": "draft_state",
      "coltype": "string",
      "visible": false,
      "headerFilter": false,
      "editable": false,
      "description": "Stored state. Null when no draft exists."
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
            courses/course-builder. QueryRef 4513 lists suggestions.
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
