-- Migration: acuranzo_1293.lua
-- Phase 36: seed courses + course_prices from live Canvas catalog + assets
--
-- Replaces index.html placeholder SVG cards with real codes/titles/images.
-- Free courses: no course_prices rows (API synthesizes 0 — FL-34).
-- Paid demo: none in v1 seed (placeholder $9 cards retired intentionally).

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.0.0 - 2026-08-08 - Seed catalog from Canvas + public/assets/courses (FL-36)

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "courses"
cfg.MIGRATION = "1293"
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
            -- Phase 36 seed: idempotent by code (skip if already present).
            -- course_id assigned from dense 1..N for empty table; MAX+offset if partial.

            INSERT INTO ${SCHEMA}courses (
                course_id, code, slug, title, summary, description, image_path,
                canvas_course_id, pricing_type, stripe_product_id,
                is_featured, sort_order, published, level, delivery_style,
                license, source, has_ai, has_quizzes, has_badge, has_certificate,
                valid_after, valid_until, created_id, created_at, updated_id, updated_at
            )
            SELECT
                (SELECT COALESCE(MAX(course_id), 0) FROM ${SCHEMA}courses) + v.ord,
                v.code, v.slug, v.title, v.summary, v.description, v.image_path,
                v.canvas_course_id, v.pricing_type, NULL,
                v.is_featured, v.sort_order, v.published, v.level, v.delivery_style,
                v.license, v.source, v.has_ai, v.has_quizzes, v.has_badge, v.has_certificate,
                ${NOW}, ${NOW}, 0, ${NOW}, 0, ${NOW}
            FROM (
                VALUES
                -- featured home grid (sort 0..5) — 6 cards matching prior home count
                (1,  '5C-001-W5C', 'welcome-to-500-courses', 'Welcome to 500 Courses',
                     'Your starting point on the 500 Courses platform.',
                     'Intro course granted automatically on first login.',
                     '/assets/courses/5C/5C-001-W5C.png', 1001, 'free',
                     1, 0, 1, 'explorer', 'self_guided', 'CC-BY-4.0', 'original', 0, 0, 0, 0),
                (2,  '5C-002-VAI', 'vermillion', 'Vermillion',
                     'Explore the Vermillion learning experience.',
                     NULL,
                     '/assets/courses/5C/5C-002-VAI.jpg', 1101, 'free',
                     1, 1, 1, 'explorer', 'self_guided', NULL, 'original', 0, 0, 0, 0),
                (3,  '5C-003-COD', 'canvas-on-doks', 'Canvas on DOKS',
                     'Run Canvas LMS on a DigitalOcean Kubernetes cluster.',
                     NULL,
                     '/assets/courses/5C/5C-003-COD.png', 1, 'free',
                     1, 2, 1, 'expert', 'self_guided', 'CC0-1.0', 'original', 0, 0, 0, 1),
                (4,  'LB-001-SAF', 'safe-ai-foundation', 'Safe AI Foundation',
                     'Foundations for using AI safely and responsibly.',
                     NULL,
                     '/assets/courses/LB/LB-001-SAF.png', 501, 'free',
                     1, 3, 1, 'explorer', 'instructor', NULL, 'original', 1, 1, 0, 0),
                (5,  'AZ-001-AZA', 'acuranzo-academy', 'Acuranzo Academy',
                     'Get started with the Acuranzo platform.',
                     NULL,
                     '/assets/courses/AZ/AZ-001-AZA.png', 301, 'free',
                     1, 4, 1, 'explorer', 'instructor', NULL, 'original', 0, 0, 0, 0),
                (6,  'XP-001-ITX', 'introduction-to-x3dp', 'Introduction to X3DP',
                     'An introduction to X3DP concepts and practice.',
                     NULL,
                     '/assets/courses/XP/XP-001-ITX.png', 601, 'free',
                     1, 5, 1, 'explorer', 'self_guided', NULL, 'original', 0, 0, 0, 0),
                -- catalog (published, not featured)
                (7,  'AZ-002-ADT', 'acuranzo-developer-training', 'Acuranzo Developer Training',
                     'Developer-focused training for Acuranzo.',
                     NULL,
                     '/assets/courses/AZ/AZ-002-ADT.png', 401, 'free',
                     0, 10, 1, 'expert', 'instructor', NULL, 'original', 0, 0, 0, 0),
                (8,  '5N-001-FVL', 'festival', 'Festival',
                     'Festival platform overview and operations.',
                     NULL,
                     '/assets/courses/5N/5N-001-FVL.png', 701, 'free',
                     0, 11, 1, 'expert', 'self_guided', NULL, 'original', 0, 0, 0, 0),
                (9,  '5F-001-GAV', 'glm-at-village', 'GLM at Village',
                     'GLM deployment and use at Village.',
                     NULL,
                     '/assets/courses/5F/5F-001-GAV.png', 201, 'free',
                     0, 12, 1, 'expert', 'groups', NULL, 'original', 1, 0, 0, 0),
                (10, 'GM-001-ITG', 'introduction-to-gaius', 'Introduction to GAIUS',
                     'Getting started with GAIUS.',
                     NULL,
                     '/assets/courses/GM/GM-001-ITG.png', 101, 'free',
                     0, 13, 1, 'explorer', 'self_guided', NULL, 'original', 0, 0, 0, 0),
                (11, 'PH-003-I2L', 'introduction-to-lithium', 'Introduction to Lithium',
                     'Learn the Lithium SPA and Hydrogen APIs.',
                     NULL,
                     '/assets/courses/PH/PH-003-I2L.png', 801, 'free',
                     0, 14, 1, 'expert', 'self_guided', NULL, 'original', 0, 0, 0, 0),
                (12, 'LB-002-MCL', 'managing-canvas-for-lanboss', 'Managing Canvas for Lanboss',
                     'Canvas administration for Lanboss.',
                     NULL,
                     '/assets/courses/LB/LB-002-MCL.png', 202, 'free',
                     0, 15, 1, 'expert', 'instructor', NULL, 'original', 0, 0, 0, 0),
                (13, 'LT-001-HCP', 'history-science-cultivated-plants', 'History and Science of Cultivated Plants',
                     'History and science of cultivated plants.',
                     NULL,
                     '/assets/courses/LT/LT-001-HCP.jpg', 901, 'free',
                     0, 16, 1, 'explorer', 'self_guided', 'CC-BY-SA-4.0', 'libretexts', 0, 0, 0, 0),
                (14, 'SC-001-ADH', 'adult-health', 'Adult Health',
                     'Adult health topics.',
                     NULL,
                     '/assets/courses/LT/LT-002-ADH.jpeg', 1002, 'free',
                     0, 17, 1, 'explorer', 'self_guided', NULL, 'canvas_commons', 0, 0, 0, 0),
                -- internal template: published=0 (not in public catalog)
                (15, '5C-000-TMP', 'course-template-english', 'Course Template (English)',
                     'Internal English course template.',
                     NULL,
                     '/assets/courses/5C/5C-000-TMP.png', 1003, 'free',
                     0, 99, 0, NULL, 'self_guided', NULL, 'original', 0, 0, 0, 0)
            ) AS v(
                ord, code, slug, title, summary, description, image_path, canvas_course_id, pricing_type,
                is_featured, sort_order, published, level, delivery_style,
                license, source, has_ai, has_quizzes, has_badge, has_certificate
            )
            WHERE NOT EXISTS (
                SELECT 1 FROM ${SCHEMA}courses c WHERE c.code = v.code
            );

            ${SUBQUERY_DELIMITER}

            -- Free courses intentionally have no course_prices rows (FL-34).

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Seed courses catalog (Phase 36)'                                   AS name,
        [=[
            # Forward Migration ${MIGRATION}: Seed courses

            Inserts live Canvas catalog rows into `${SCHEMA}courses` with
            Reception asset paths. Six featured rows (sort_order 0–5) replace
            the six placeholder home cards. All v1 seeds are `pricing_type=free`
            with **no** `course_prices` rows (FL-34). Course template is
            `published=0`. Idempotent on `code`.
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
            DELETE FROM ${SCHEMA}course_prices
            WHERE course_id IN (
                SELECT course_id FROM ${SCHEMA}courses
                WHERE code IN (
                    '5C-001-W5C','5C-002-VAI','5C-003-COD','LB-001-SAF','AZ-001-AZA',
                    'XP-001-ITX','AZ-002-ADT','5N-001-FVL','5F-001-GAV','GM-001-ITG',
                    'PH-003-I2L','LB-002-MCL','LT-001-HCP','SC-001-ADH','5C-000-TMP'
                )
            );

            ${SUBQUERY_DELIMITER}

            DELETE FROM ${SCHEMA}courses
            WHERE code IN (
                '5C-001-W5C','5C-002-VAI','5C-003-COD','LB-001-SAF','AZ-001-AZA',
                'XP-001-ITX','AZ-002-ADT','5N-001-FVL','5F-001-GAV','GM-001-ITG',
                'PH-003-I2L','LB-002-MCL','LT-001-HCP','SC-001-ADH','5C-000-TMP'
            );

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Remove Phase 36 course seed'                                       AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Remove seeded courses

            Deletes the Phase 36 seed set by `code` (and any prices for those
            courses). Does not drop tables (1291/1292).
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
return queries end
