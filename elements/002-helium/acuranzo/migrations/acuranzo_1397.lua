-- Migration: acuranzo_1397.lua
-- Lookup 059 key 14 courses/course-manager tableDef
--
-- CATCHUP Phase 13. Lookup 059 keys 0-13 already exist. Key 13 is
-- scripts/script-manager. value_txt is courses/course-manager so the
-- Course Manager grid resolves this definition. List QueryRef 4500.
-- Insert 4501. Update 4502. No delete. The grid is readonly. The
-- form writes through those QueryRefs.
--
-- luacheck: no max line length
-- luacheck: no unused args
--
-- CHANGELOG
-- 1.0.0 - 2026-10-22 - Lookup 059 key 14 courses/course-manager tableDef

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "lookups"
cfg.MIGRATION = "1397"
cfg.LOOKUP_ID = "059"
cfg.KEY_IDX = "14"
cfg.TABLEDEF_NAME = "courses/course-manager"
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
                (${LOOKUP_ID}, ${KEY_IDX}, 1, '${TABLEDEF_NAME}', 0, 14, '',
                [==[
                    # Course Manager Table Definition

                    Column definitions for the Course Manager list.
                    QueryRef 4500 returns every courses row. The form
                    inserts with 4501 and updates with 4502. There is
                    no delete. Search and the retired chip run in the
                    browser.

                    ## Table Configuration

                    - **Table path**: courses/course-manager
                    - **QueryRef**: 4500 (Staff: List Courses)
                    - **InsertQueryRef**: 4501 (Staff: Insert Course)
                    - **UpdateQueryRef**: 4502 (Staff: Update Course)
                    - **Readonly**: true
                    - **SelectableRows**: 1
                ]==],
                ${JSON_INGEST_START}[===[
{
  "$schema": "../tabledef-schema.json",
  "$version": "1.0.0",
  "$description": "Course Manager list. Matches Lithium table path courses/course-manager.",

  "table": "course-manager",
  "title": "Course Manager",
  "queryRef": 4500,
  "insertQueryRef": 4501,
  "updateQueryRef": 4502,
  "readonly": true,
  "selectableRows": 1,
  "layout": "fitColumns",
  "responsiveLayout": "collapse",
  "resizableColumns": true,
  "persistSort": true,
  "persistFilter": false,
  "initialSort": [
    { "column": "sort_order", "dir": "asc" },
    { "column": "course_id", "dir": "asc" }
  ],

  "columns": {

    "course_id": {
      "title": "Id",
      "field": "course_id",
      "coltype": "integer",
      "visible": false,
      "headerFilter": false,
      "editable": false,
      "primaryKey": true,
      "description": "courses.course_id. Assigned by the insert query."
    },

    "code": {
      "title": "Code",
      "field": "code",
      "coltype": "string",
      "visible": true,
      "headerFilter": false,
      "editable": false,
      "description": "Operator course code."
    },

    "slug": {
      "title": "Slug",
      "field": "slug",
      "coltype": "string",
      "visible": true,
      "headerFilter": false,
      "editable": false,
      "description": "Catalog slug."
    },

    "title": {
      "title": "Title",
      "field": "title",
      "coltype": "string",
      "visible": true,
      "headerFilter": false,
      "editable": false,
      "description": "Required on create. Sync replaces it."
    },

    "pricing_type": {
      "title": "Pricing",
      "field": "pricing_type",
      "coltype": "string",
      "visible": true,
      "headerFilter": false,
      "editable": false,
      "description": "free or paid."
    },

    "published": {
      "title": "Published",
      "field": "published",
      "coltype": "integer",
      "visible": true,
      "headerFilter": false,
      "editable": false,
      "description": "1 after a sync publishes the course. The form does not write this."
    },

    "retired": {
      "title": "Retired",
      "field": "retired",
      "coltype": "integer",
      "visible": true,
      "headerFilter": false,
      "editable": false,
      "description": "1 when retired. The retire action writes this."
    },

    "canvas_course_id": {
      "title": "Canvas",
      "field": "canvas_course_id",
      "coltype": "integer",
      "visible": true,
      "headerFilter": false,
      "editable": false,
      "description": "Canvas course id. Blank stays null."
    },

    "sort_order": {
      "title": "Sort",
      "field": "sort_order",
      "coltype": "integer",
      "visible": true,
      "headerFilter": false,
      "editable": false,
      "description": "List order. The query also sorts by course_id."
    },

    "image_path": {
      "title": "Image",
      "field": "image_path",
      "coltype": "string",
      "visible": false,
      "headerFilter": false,
      "editable": false,
      "description": "Image URL or site path."
    },

    "is_featured": {
      "title": "Featured",
      "field": "is_featured",
      "coltype": "integer",
      "visible": false,
      "headerFilter": false,
      "editable": false,
      "description": "1 when the course is featured."
    },

    "summary": {
      "title": "Summary",
      "field": "summary",
      "coltype": "string",
      "visible": false,
      "headerFilter": false,
      "editable": false,
      "description": "View only. Sync writes it."
    },

    "description": {
      "title": "Description",
      "field": "description",
      "coltype": "string",
      "visible": false,
      "headerFilter": false,
      "editable": false,
      "description": "View only. Sync writes it."
    },

    "stripe_product_id": {
      "title": "Stripe product",
      "field": "stripe_product_id",
      "coltype": "string",
      "visible": false,
      "headerFilter": false,
      "editable": false,
      "description": "Read only. Price rows are a later screen."
    },

    "tags": {
      "title": "Tags",
      "field": "tags",
      "coltype": "string",
      "visible": false,
      "headerFilter": false,
      "editable": false,
      "description": "JSON object."
    },

    "canvas_updated_at": {
      "title": "Last sync",
      "field": "canvas_updated_at",
      "coltype": "datetime",
      "visible": false,
      "headerFilter": false,
      "editable": false,
      "description": "Last Canvas sync time."
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
            courses/course-manager. QueryRef 4500 lists every course.
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

            Deletes Lookup ${LOOKUP_ID} key ${KEY_IDX}. Course rows stay.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
return queries end
