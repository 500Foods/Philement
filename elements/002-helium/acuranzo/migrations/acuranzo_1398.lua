-- Migration: acuranzo_1398.lua
-- Lookup 059 key 14 default columns are Code and Title
--
-- Does not edit acuranzo_1397.lua. A saved LithiumTable template
-- can keep the earlier columns until that template is reset.
-- The grid stays readonly. The form is the editor.
--
-- luacheck: no max line length
-- luacheck: no unused args
--
-- CHANGELOG
-- 1.0.0 - 2026-10-22 - Course Manager default columns are Code and Title

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "lookups"
cfg.MIGRATION = "1398"
cfg.LOOKUP_ID = "059"
cfg.KEY_IDX = "14"
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
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
  "$description": "Course Manager list. Default visible columns are Code and Title.",
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
    {
      "column": "sort_order",
      "dir": "asc"
    },
    {
      "column": "course_id",
      "dir": "asc"
    }
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
      "visible": false,
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
      "visible": false,
      "headerFilter": false,
      "editable": false,
      "description": "free or paid."
    },
    "published": {
      "title": "Published",
      "field": "published",
      "coltype": "integer",
      "visible": false,
      "headerFilter": false,
      "editable": false,
      "description": "1 after a sync publishes the course. The form does not write this."
    },
    "retired": {
      "title": "Retired",
      "field": "retired",
      "coltype": "integer",
      "visible": false,
      "headerFilter": false,
      "editable": false,
      "description": "1 when retired. Options writes this after the retired update is applied."
    },
    "canvas_course_id": {
      "title": "Canvas",
      "field": "canvas_course_id",
      "coltype": "integer",
      "visible": false,
      "headerFilter": false,
      "editable": false,
      "description": "Canvas course id. Blank stays null."
    },
    "sort_order": {
      "title": "Sort",
      "field": "sort_order",
      "coltype": "integer",
      "visible": false,
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
      "description": "Read only. The Commerce tab lists prices."
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
]===]${JSON_INGEST_END}
            WHERE lookup_id = ${LOOKUP_ID}
            AND key_idx = ${KEY_IDX};

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Show Code and Title on Lookup ${LOOKUP_ID} key ${KEY_IDX}'                                                            AS name,
        [=[
            # Forward Migration ${MIGRATION}: Code and Title columns

            Updates Lookup ${LOOKUP_ID} key ${KEY_IDX} collection.
            visible is true for code and title only. Other columns
            stay in the definition. A saved template can still show
            the previous columns until it is reset.

        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
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
    {
      "column": "sort_order",
      "dir": "asc"
    },
    {
      "column": "course_id",
      "dir": "asc"
    }
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
]===]${JSON_INGEST_END}
            WHERE lookup_id = ${LOOKUP_ID}
            AND key_idx = ${KEY_IDX};

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Restore Lookup ${LOOKUP_ID} key ${KEY_IDX} columns'                                                            AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Restore Lookup ${LOOKUP_ID} key ${KEY_IDX}

            Puts the acuranzo_1397.lua column visibility back.
            Course rows stay.

        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})

return queries end
