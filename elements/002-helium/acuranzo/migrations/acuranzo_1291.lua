-- Migration: acuranzo_1291.lua
-- Phase 35: courses catalog table (Band G)
--
-- Durable Reception/Hydrogen course catalog. Prices live in course_prices
-- (acuranzo_1292). Design: FINISHLINE FL-34.

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.0.0 - 2026-08-08 - Create courses table (Phase 35 / FL-34)

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "courses"
cfg.MIGRATION = "1291"
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
            CREATE TABLE ${SCHEMA}${TABLE}
            (
                course_id               ${INTEGER}          NOT NULL,
                code                    ${VARCHAR_50}       NOT NULL,
                slug                    ${VARCHAR_100}      NOT NULL,
                title                   ${VARCHAR_500}      NOT NULL,
                summary                 ${VARCHAR_500}              ,
                description             ${TEXT}                     ,
                image_path              ${VARCHAR_500}              ,
                canvas_course_id        ${INTEGER}                  ,
                pricing_type            ${VARCHAR_20}       NOT NULL,
                stripe_product_id       ${VARCHAR_100}              ,
                is_featured             ${INTEGER_SMALL}    NOT NULL DEFAULT 0,
                sort_order              ${INTEGER}          NOT NULL DEFAULT 0,
                published               ${INTEGER_SMALL}    NOT NULL DEFAULT 0,
                level                   ${VARCHAR_20}               ,
                delivery_style          ${VARCHAR_20}               ,
                license                 ${VARCHAR_64}               ,
                source                  ${VARCHAR_64}               ,
                has_ai                  ${INTEGER_SMALL}    NOT NULL DEFAULT 0,
                has_quizzes             ${INTEGER_SMALL}    NOT NULL DEFAULT 0,
                has_badge               ${INTEGER_SMALL}    NOT NULL DEFAULT 0,
                has_certificate         ${INTEGER_SMALL}    NOT NULL DEFAULT 0,
                ${COMMON_CREATE}
                ${PRIMARY}(course_id),
                ${UNIQUE}(code),
                ${UNIQUE}(slug)
            );

            ${SUBQUERY_DELIMITER}

            CREATE INDEX ${TABLE}_idx_catalog
                ON ${SCHEMA}${TABLE}(published, is_featured, sort_order);

            ${SUBQUERY_DELIMITER}

            CREATE INDEX ${TABLE}_idx_canvas
                ON ${SCHEMA}${TABLE}(canvas_course_id);

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Create ${TABLE} Table'                                             AS name,
        [=[
            # Forward Migration ${MIGRATION}: Create ${TABLE} Table

            Band G Phase 35 — Reception catalog course rows (FL-34).

            ## Schema (summary)

            - **course_id**: Surrogate PK.
            - **code**: Canonical label without locale, e.g. `5C-001-W5C`
              (UNIQUE). Canvas may show `5C-001-W5C-EN`.
            - **slug**: URL-safe unique key.
            - **title / summary / description**: Marketing copy; description
              is plain text/Markdown (not raw HTML).
            - **image_path**: Site-relative asset path.
            - **canvas_course_id**: LMS numeric id (nullable until published).
            - **pricing_type**: `free` | `paid`. Amounts live in
              `course_prices` (migration 1292) — no `price_cents_*` columns.
            - **stripe_product_id**: NULL until Band J / STRIPE_PLAN.
            - **is_featured / sort_order / published**: Home + catalog flags.
            - **level / delivery_style / license / source / has_***: Card tags.

            No SQL FK to accounts/canvas. `COMMON_CREATE` only for audit
            timestamps (no second domain created_at).
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
            ${DROP_CHECK};

            ${SUBQUERY_DELIMITER}

            DROP TABLE ${SCHEMA}${TABLE};

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Drop ${TABLE} Table'                                               AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Drop ${TABLE} Table

            Drops courses. Reverse course_prices (1292) first if present.
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
        ${TYPE_DIAGRAM_MIGRATION}                                           AS query_type_a28,
        ${DIALECT}                                                          AS query_dialect_a30,
        ${QTC_SLOW}                                                         AS query_queue_a58,
        ${TIMEOUT}                                                          AS query_timeout,
        'JSON Table Definition in collection'                               AS code,
        'Diagram Tables: ${SCHEMA}${TABLE}'                                 AS name,
        [=[
            # Diagram Migration ${MIGRATION}

            ## Diagram Tables: ${SCHEMA}${TABLE}
        ]=]
                                                                            AS summary,
                                                                            -- DIAGRAM_START
        ${JSON_INGEST_START}
        [=[
            {
                "diagram": [
                    {
                        "object_type": "table",
                        "object_id": "table.${TABLE}",
                        "object_ref": "${MIGRATION}",
                        "table": [
                            {"name": "course_id", "datatype": "${INTEGER}", "nullable": false, "primary_key": true, "unique": true},
                            {"name": "code", "datatype": "${VARCHAR_50}", "nullable": false, "primary_key": false, "unique": true},
                            {"name": "slug", "datatype": "${VARCHAR_100}", "nullable": false, "primary_key": false, "unique": true},
                            {"name": "title", "datatype": "${VARCHAR_500}", "nullable": false, "primary_key": false, "unique": false},
                            {"name": "summary", "datatype": "${VARCHAR_500}", "nullable": true, "primary_key": false, "unique": false},
                            {"name": "description", "datatype": "${TEXT}", "nullable": true, "primary_key": false, "unique": false},
                            {"name": "image_path", "datatype": "${VARCHAR_500}", "nullable": true, "primary_key": false, "unique": false},
                            {"name": "canvas_course_id", "datatype": "${INTEGER}", "nullable": true, "primary_key": false, "unique": false},
                            {"name": "pricing_type", "datatype": "${VARCHAR_20}", "nullable": false, "primary_key": false, "unique": false},
                            {"name": "stripe_product_id", "datatype": "${VARCHAR_100}", "nullable": true, "primary_key": false, "unique": false},
                            {"name": "is_featured", "datatype": "${INTEGER_SMALL}", "nullable": false, "primary_key": false, "unique": false},
                            {"name": "sort_order", "datatype": "${INTEGER}", "nullable": false, "primary_key": false, "unique": false},
                            {"name": "published", "datatype": "${INTEGER_SMALL}", "nullable": false, "primary_key": false, "unique": false},
                            {"name": "level", "datatype": "${VARCHAR_20}", "nullable": true, "primary_key": false, "unique": false},
                            {"name": "delivery_style", "datatype": "${VARCHAR_20}", "nullable": true, "primary_key": false, "unique": false},
                            {"name": "license", "datatype": "${VARCHAR_64}", "nullable": true, "primary_key": false, "unique": false},
                            {"name": "source", "datatype": "${VARCHAR_64}", "nullable": true, "primary_key": false, "unique": false},
                            {"name": "has_ai", "datatype": "${INTEGER_SMALL}", "nullable": false, "primary_key": false, "unique": false},
                            {"name": "has_quizzes", "datatype": "${INTEGER_SMALL}", "nullable": false, "primary_key": false, "unique": false},
                            {"name": "has_badge", "datatype": "${INTEGER_SMALL}", "nullable": false, "primary_key": false, "unique": false},
                            {"name": "has_certificate", "datatype": "${INTEGER_SMALL}", "nullable": false, "primary_key": false, "unique": false},
                            ${COMMON_DIAGRAM}
                        ]
                    }
                ]
            }
        ]=]
        ${JSON_INGEST_END}
                                                                            -- DIAGRAM_END
                                                                            AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
return queries end
