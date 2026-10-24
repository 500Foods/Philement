-- Migration: acuranzo_1400.lua
-- Lookup 059 key 15 uses QueryRef 4515 and shows the manage block
--
-- Does not edit acuranzo_1399.lua. Key 15 stays
-- courses/course-builder. Key 16 stays free. Receipt, flag, and
-- annotation are hidden. No insert, update, or delete query ref
-- is stored here. The screen supplies those calls.
--
-- luacheck: no max line length
-- luacheck: no unused args
--
-- CHANGELOG
-- 1.0.0 - 2026-10-23 - Lookup 059 key 15 QueryRef 4515

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "lookups"
cfg.MIGRATION = "1400"
cfg.LOOKUP_ID = "059"
cfg.KEY_IDX = "15"
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
            SET collection = ${JSON_INGEST_START}[===[
{
  "$schema": "../tabledef-schema.json",
  "$version": "1.0.0",
  "$description": "Course Builder suggestion list. QueryRef 4515. Staff can edit rows. Receipt, flag, and annotation stay hidden.",
  "table": "course-builder",
  "title": "Course Builder",
  "queryRef": 4515,
  "readonly": false,
  "selectableRows": 1,
  "layout": "fitColumns",
  "responsiveLayout": "collapse",
  "resizableColumns": true,
  "persistSort": true,
  "persistFilter": false,
  "initialSort": [
    {
      "column": "last_activity",
      "dir": "desc"
    },
    {
      "column": "suggestion_id",
      "dir": "desc"
    }
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
      "description": "Suggested, Received, or Intake."
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
      "description": "Submitted name. A working-title save does not change it. A staff edit can."
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
    },
    "receipt_mail": {
      "title": "Receipt",
      "field": "receipt_mail",
      "coltype": "string",
      "visible": false,
      "headerFilter": false,
      "editable": false,
      "description": "sent, failed, or empty when no receipt attempt has been made."
    },
    "flagged": {
      "title": "Flag",
      "field": "flagged",
      "coltype": "integer",
      "visible": false,
      "headerFilter": false,
      "editable": false,
      "description": "1 when the suggestion is flagged."
    },
    "annotation": {
      "title": "Annotation",
      "field": "annotation",
      "coltype": "string",
      "visible": false,
      "headerFilter": false,
      "editable": false,
      "description": "Staff note. The editor sends at most 250 characters."
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
        'Point Lookup ${LOOKUP_ID} key ${KEY_IDX} at QueryRef 4515'        AS name,
        [=[
            # Forward Migration ${MIGRATION}: QueryRef 4515 on Lookup ${LOOKUP_ID} key ${KEY_IDX}

            Updates Lookup ${LOOKUP_ID} key ${KEY_IDX}. The list query
            is 4515. The grid shows the manage block. Receipt, flag,
            and annotation stay hidden.
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
            SET collection = ${JSON_INGEST_START}[===[
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
        'Restore Lookup ${LOOKUP_ID} key ${KEY_IDX} QueryRef 4513'         AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Restore Lookup ${LOOKUP_ID} key ${KEY_IDX}

            Restores the collection written by acuranzo_1399. QueryRef
            4513 and a readonly grid. Suggestions stay.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
return queries end
