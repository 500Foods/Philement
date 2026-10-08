-- Migration: acuranzo_1295.lua
-- QueryRef #148 - Courses: Get by Slug (Phase 37)

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.0.0 - 2026-08-08 - Public get course by slug (FL-37)

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "queries"
cfg.MIGRATION = "1295"
cfg.QUERY_REF = "148"
cfg.QUERY_NAME = "Courses: Get by slug"
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
            INSERT INTO ${SCHEMA}${QUERIES} (
                ${QUERIES_INSERT}
            )
            WITH next_query_id AS (
                SELECT COALESCE(MAX(query_id), 0) + 1 AS new_query_id
                FROM ${SCHEMA}${QUERIES}
            )
            SELECT
                new_query_id                                                        AS query_id,
                ${QUERY_REF}                                                        AS query_ref,
                ${STATUS_ACTIVE}                                                    AS query_status_a27,
                ${TYPE_PUBLIC}                                                      AS query_type_a28,
                ${DIALECT}                                                          AS query_dialect_a30,
                ${QTC_MEDIUM}                                                       AS query_queue_a58,
                ${TIMEOUT}                                                          AS query_timeout,
                [==[
                    SELECT
                        c.course_id,
                        c.code,
                        c.slug,
                        c.title,
                        c.summary,
                        c.description,
                        c.image_path,
                        c.canvas_course_id,
                        c.pricing_type,
                        c.is_featured,
                        c.sort_order,
                        c.level,
                        c.delivery_style,
                        c.license,
                        c.source,
                        c.has_ai,
                        c.has_quizzes,
                        c.has_badge,
                        c.has_certificate,
                        CASE
                            WHEN c.pricing_type = 'free' THEN 0
                            ELSE COALESCE(p.unit_amount_cents, p_cad.unit_amount_cents)
                        END AS unit_amount_cents,
                        CASE
                            WHEN c.pricing_type = 'free' THEN LOWER(CAST(:CURRENCY AS VARCHAR(20)))
                            WHEN p.price_id IS NOT NULL THEN p.currency
                            WHEN p_cad.price_id IS NOT NULL THEN p_cad.currency
                            ELSE LOWER(CAST(:CURRENCY AS VARCHAR(20)))
                        END AS currency,
                        CASE
                            WHEN c.pricing_type = 'free' THEN 0
                            WHEN p.price_id IS NULL
                             AND p_cad.price_id IS NOT NULL
                             AND LOWER(CAST(:CURRENCY AS VARCHAR(20))) <> 'cad'
                            THEN 1
                            ELSE 0
                        END AS price_fallback
                    FROM ${SCHEMA}courses c
                    LEFT JOIN ${SCHEMA}course_prices p
                      ON p.course_id = c.course_id
                     AND p.is_active = 1
                     AND p.currency = LOWER(CAST(:CURRENCY AS VARCHAR(20)))
                    LEFT JOIN ${SCHEMA}course_prices p_cad
                      ON p_cad.course_id = c.course_id
                     AND p_cad.is_active = 1
                     AND p_cad.currency = 'cad'
                    WHERE c.published = 1
                      AND c.slug = :SLUG
                ]==]                                                                AS code,
                '${QUERY_NAME}'                                                     AS name,
                [==[
                    # QueryRef #${QUERY_REF} - ${QUERY_NAME}

                    Public course detail by slug (Phase 37). Same currency /
                    free / CAD-fallback rules as #147.

                    ## Parameters

                    - `SLUG` (string): unique course slug.
                    - `CURRENCY` (string): lowercase ISO 4217.
                ]==]
                                                                                    AS summary,
                '{}'                                                                AS collection,
                ${COMMON_INSERT}
            FROM next_query_id;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Populate QueryRef #${QUERY_REF} - ${QUERY_NAME}'                   AS name,
        [=[
            # Forward Migration ${MIGRATION}: QueryRef #${QUERY_REF}
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
            WHERE query_ref = ${QUERY_REF};

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Remove QueryRef #${QUERY_REF} - ${QUERY_NAME}'                     AS name,
        [=[
            # Reverse Migration ${MIGRATION}
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
return queries end
