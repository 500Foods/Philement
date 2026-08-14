-- Migration: acuranzo_1316.lua
-- STRIPE_PLAN Phase 3: seed sandbox paid course + course_prices
--
-- New published paid course (id 16). Does not flip existing free rows.
-- Stripe test Product/Prices created 2026-08-13 on acct_1ThPJP7QZae4nkUa.

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.0.0 - 2026-08-13 - Seed PH-003-I2P sandbox paid course + CAD/USD/EUR/GBP prices

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "courses"
cfg.MIGRATION = "1316"
-- ----------------------------------------------------------------------------
-- Forward
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
            INSERT INTO ${SCHEMA}courses (
                course_id, code, slug, title, summary, description, image_path,
                canvas_course_id, pricing_type, stripe_product_id,
                is_featured, sort_order, published, level, delivery_style,
                license, source, has_ai, has_quizzes, has_badge, has_certificate,
                ${COMMON_FIELDS}
            )
            VALUES
            (
                16, 'PH-003-I2P', 'introduction-to-lithium-paid',
                'Introduction to Lithium (sandbox paid)',
                'Sandbox paid SKU for Stripe checkout tests. Same Canvas as I2L.',
                'STRIPE_PLAN Phase 3 test course. Not sold in production.',
                '/assets/courses/PH/PH-003-I2L.png', 801, 'paid', 'prod_V4BvyZn5UNwmWG',
                0, 18, 1, 'expert', 'self_guided',
                NULL, 'original', 0, 0, 0, 0,
                ${COMMON_VALUES}
            );

            ${SUBQUERY_DELIMITER}

            INSERT INTO ${SCHEMA}course_prices (
                price_id, course_id, currency, unit_amount_cents,
                stripe_price_id, is_active,
                ${COMMON_FIELDS}
            )
            VALUES
            (
                1, 16, 'cad', 4900, 'price_1U43Xo7QZae4nkUazX8xxktN', 1,
                ${COMMON_VALUES}
            ),
            (
                2, 16, 'usd', 3900, 'price_1U43Xp7QZae4nkUaP8ePITEy', 1,
                ${COMMON_VALUES}
            ),
            (
                3, 16, 'eur', 3500, 'price_1U43Xp7QZae4nkUay260aGPP', 1,
                ${COMMON_VALUES}
            ),
            (
                4, 16, 'gbp', 3000, 'price_1U43Xq7QZae4nkUarqIf7gI3', 1,
                ${COMMON_VALUES}
            );

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Seed sandbox paid course + Stripe prices'                          AS name,
        [=[
            # Forward Migration ${MIGRATION}: Sandbox paid course

            STRIPE_PLAN Phase 3. Inserts published `courses` row 16
            (`PH-003-I2P`, `pricing_type=paid`) and four
            `course_prices` rows (cad/usd/eur/gbp) with Stripe test
            Price ids from account `acct_1ThPJP7QZae4nkUa`.

            Product `prod_V4BvyZn5UNwmWG`. Does not change existing
            free catalog rows. Canvas id 801 matches I2L (sandbox).
            No diagram (data seed).
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]====]})

-- ----------------------------------------------------------------------------
-- Reverse
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
            DELETE FROM ${SCHEMA}course_prices
            WHERE course_id = 16
              AND price_id IN (1, 2, 3, 4);

            ${SUBQUERY_DELIMITER}

            DELETE FROM ${SCHEMA}courses
            WHERE course_id = 16
              AND code = 'PH-003-I2P';

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Remove sandbox paid course + Stripe prices'                         AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Remove sandbox paid course
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]====]})

return queries end
