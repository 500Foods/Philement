-- Migration: acuranzo_1363.lua
-- PRIORITIZE 2.19: QueryRef 147 Q / LEVEL / PAID_FREE / TAG / SORT / HIDE_RETIRED
--
-- Do not restamp 1347 (already APPLIED). #148 is unchanged.
-- #147 still returns retired published rows unless HIDE_RETIRED=1.
-- Always bind every new name ('' when unused). Never pass nil.
--
-- luacheck: no max line length
-- luacheck: no unused args
--
-- CHANGELOG
-- 1.0.0 - 2026-08-21 - Catalog list filter/sort params on QueryRef 147

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "queries"
cfg.MIGRATION = "1363"
-- ----------------------------------------------------------------------------
-- Forward: replace #147 SELECT with optional filter/sort binds
-- ----------------------------------------------------------------------------
table.insert(queries,{sql=[====[

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
            UPDATE ${SCHEMA}${QUERIES}
               SET code = [==[
                    SELECT
                        c.course_id,
                        c.code,
                        c.slug,
                        c.title,
                        c.summary,
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
                        c.tags,
                        c.retired,
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
                        END AS price_fallback,
                        CASE
                            WHEN c.pricing_type = 'free' THEN NULL
                            WHEN p.price_id IS NOT NULL THEN p.stripe_price_id
                            ELSE p_cad.stripe_price_id
                        END AS stripe_price_id
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
                      AND (NULLIF(:Q, '') IS NULL
                           OR STRPOS(LOWER(c.title), LOWER(:Q)) > 0
                           OR STRPOS(LOWER(c.summary), LOWER(:Q)) > 0
                           OR STRPOS(LOWER(c.code), LOWER(:Q)) > 0)
                      AND (NULLIF(:LEVEL, '') IS NULL
                           OR c.level = ANY(string_to_array(:LEVEL, ',')))
                      AND (NULLIF(:PAID_FREE, '') IS NULL
                           OR :PAID_FREE = 'all'
                           OR (:PAID_FREE = 'free' AND c.pricing_type = 'free')
                           OR (:PAID_FREE = 'paid' AND c.pricing_type <> 'free'))
                      AND (NULLIF(:TAG, '') IS NULL
                           OR (
                                (NOT ('ai' = ANY(string_to_array(:TAG, ','))) OR c.has_ai = 1)
                            AND (NOT ('quizzes' = ANY(string_to_array(:TAG, ','))) OR c.has_quizzes = 1)
                            AND (NOT ('badge' = ANY(string_to_array(:TAG, ','))) OR c.has_badge = 1)
                            AND (NOT ('certificate' = ANY(string_to_array(:TAG, ','))) OR c.has_certificate = 1)
                           ))
                      AND (NULLIF(:HIDE_RETIRED, '') IS NULL
                           OR :HIDE_RETIRED <> '1'
                           OR c.retired = 0)
                    ORDER BY
                      CASE LOWER(CAST(:SORT AS VARCHAR(20)))
                        WHEN 'title' THEN 1
                        WHEN 'price-asc' THEN 2
                        WHEN 'price-desc' THEN 3
                        WHEN 'newest' THEN 4
                        ELSE 0
                      END,
                      CASE WHEN LOWER(CAST(:SORT AS VARCHAR(20))) = 'title'
                           THEN c.title ELSE '' END,
                      CASE WHEN LOWER(CAST(:SORT AS VARCHAR(20))) = 'price-asc'
                           THEN CASE
                                  WHEN c.pricing_type = 'free' THEN 0
                                  ELSE COALESCE(p.unit_amount_cents, p_cad.unit_amount_cents)
                                END
                           ELSE 0 END,
                      CASE WHEN LOWER(CAST(:SORT AS VARCHAR(20))) = 'price-desc'
                           THEN CASE
                                  WHEN c.pricing_type = 'free' THEN 0
                                  ELSE COALESCE(p.unit_amount_cents, p_cad.unit_amount_cents)
                                END
                           ELSE 0 END DESC,
                      CASE WHEN LOWER(CAST(:SORT AS VARCHAR(20))) = 'newest'
                           THEN c.course_id ELSE 0 END DESC,
                      CASE WHEN LOWER(CAST(:SORT AS VARCHAR(20))) IN ('title', 'price-asc', 'price-desc', 'newest')
                           THEN 0 ELSE c.sort_order END,
                      CASE WHEN LOWER(CAST(:SORT AS VARCHAR(20))) = 'newest'
                           THEN 0 ELSE c.course_id END
               ]==]
             WHERE query_ref = 147
               AND query_type_a28 = ${TYPE_PUBLIC};

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Add catalog filter params to QueryRef 147'                            AS name,
        [=[
            # Forward Migration ${MIGRATION}: Catalog list filters

            PRIORITIZE 2.19. Replaces the SELECT body of public
            QueryRef **147** so it accepts `Q`, `LEVEL` (comma OR),
            `PAID_FREE`, `TAG` (comma AND of ai/quizzes/badge/certificate),
            `SORT`, and `HIDE_RETIRED`. Empty string = unused. Still
            `published=1` only; retired stay listed unless
            `HIDE_RETIRED=1`. Do not restamp 1347. #148 unchanged.
                ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]====]})

-- ----------------------------------------------------------------------------
-- Reverse: restore 1347 #147 SELECT (CURRENCY only)
-- ----------------------------------------------------------------------------
table.insert(queries,{sql=[====[

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
            UPDATE ${SCHEMA}${QUERIES}
               SET code = [==[
                    SELECT
                        c.course_id,
                        c.code,
                        c.slug,
                        c.title,
                        c.summary,
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
                        c.tags,
                        c.retired,
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
                        END AS price_fallback,
                        CASE
                            WHEN c.pricing_type = 'free' THEN NULL
                            WHEN p.price_id IS NOT NULL THEN p.stripe_price_id
                            ELSE p_cad.stripe_price_id
                        END AS stripe_price_id
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
                    ORDER BY c.is_featured DESC, c.sort_order ASC, c.course_id ASC
               ]==]
             WHERE query_ref = 147
               AND query_type_a28 = ${TYPE_PUBLIC};

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Restore QueryRef 147 without filter params'                           AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Restore 1347 catalog list SELECT
                ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]====]})

return queries end
