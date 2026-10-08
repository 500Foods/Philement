-- Migration: acuranzo_1313.lua
-- STRIPE_PLAN Phase 3: QueryRefs 147/148 SELECT stripe_price_id
--
-- Do not restamp 1294/1295 (already APPLIED). Reception already maps
-- stripePriceId (FL-68). Values stay NULL until Stripe Prices are seeded.

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.0.0 - 2026-08-13 - Add stripe_price_id to catalog QueryRefs 147/148

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "queries"
cfg.MIGRATION = "1313"
-- ----------------------------------------------------------------------------
-- Forward: replace #147 and #148 SELECT lists
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
               SET code = [==[
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
                      AND c.slug = :SLUG
               ]==]
             WHERE query_ref = 148
               AND query_type_a28 = ${TYPE_PUBLIC};

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Add stripe_price_id to QueryRefs 147/148'                          AS name,
        [=[
            # Forward Migration ${MIGRATION}: Catalog stripe_price_id

            STRIPE_PLAN Phase 3. Replaces the SELECT body of public
            QueryRefs **147** (list) and **148** (by slug) so they
            return `stripe_price_id` from the matched `course_prices`
            row (requested currency, else CAD fallback). Free courses
            return NULL. No new C route.

            Price IDs stay NULL until an operator seeds Stripe Prices
            into `course_prices` (Dashboard or later Course Builder).
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]====]})

-- ----------------------------------------------------------------------------
-- Reverse: restore 1294/1295 SELECT lists
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
                    ORDER BY c.is_featured DESC, c.sort_order ASC, c.course_id ASC
               ]==]
             WHERE query_ref = 147
               AND query_type_a28 = ${TYPE_PUBLIC};

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
               SET code = [==[
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
               ]==]
             WHERE query_ref = 148
               AND query_type_a28 = ${TYPE_PUBLIC};

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Restore QueryRefs 147/148 without stripe_price_id'                 AS name,
        [=[
            # Reverse Migration ${MIGRATION}

            Restores the 1294/1295 SELECT lists (no stripe_price_id).
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]====]})

return queries end
