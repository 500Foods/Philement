-- Migration: courses_4081.lua
-- QueryRef #4502 also writes retired
--
-- Does not edit courses_4078.lua. Hydrogen ignores a bind name the
-- SQL does not mention, and fails when the SQL names a bind the
-- client omits. Reload the Course Manager before this file is
-- applied. Each name is used once.
--
-- luacheck: no max line length
-- luacheck: no unused args
--
-- CHANGELOG
-- 1.0.0 - 2026-10-22 - Staff update writes courses.retired

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "queries"
cfg.MIGRATION = "4081"
cfg.QUERY_REF = "4502"
cfg.QUERY_NAME = "Staff: Update Course"
if engine == "mysql" then
    cfg.CAST_INTEGER = "signed"
    cfg.CAST_TEXT = "char(255)"
else
    cfg.CAST_INTEGER = cfg.INTEGER
    cfg.CAST_TEXT = cfg.TEXT
end

local sql_new = [[
                    UPDATE ${SCHEMA}courses
                       SET code = CAST(:CODE AS ${CAST_TEXT}),
                           slug = CAST(:SLUG AS ${CAST_TEXT}),
                           image_path = NULLIF(CAST(:IMAGE_PATH AS ${CAST_TEXT}), ''),
                           canvas_course_id = CAST(NULLIF(CAST(:CANVAS_COURSE_ID AS ${CAST_TEXT}), '') AS ${CAST_INTEGER}),
                           pricing_type = CAST(:PRICING_TYPE AS ${CAST_TEXT}),
                           is_featured = CAST(:IS_FEATURED AS ${CAST_INTEGER}),
                           retired = CAST(:RETIRED AS ${CAST_INTEGER}),
                           sort_order = CAST(:SORT_ORDER AS ${CAST_INTEGER}),
                           tags = ${JIS}CAST(:TAGS AS ${CAST_TEXT})${JIE},
                           updated_at = ${NOW},
                           updated_id = CAST(:USERID AS ${CAST_INTEGER})
                     WHERE course_id = CAST(:COURSEID AS ${CAST_INTEGER})]]

local sql_old = [[
                    UPDATE ${SCHEMA}courses
                       SET code = CAST(:CODE AS ${CAST_TEXT}),
                           slug = CAST(:SLUG AS ${CAST_TEXT}),
                           image_path = NULLIF(CAST(:IMAGE_PATH AS ${CAST_TEXT}), ''),
                           canvas_course_id = CAST(NULLIF(CAST(:CANVAS_COURSE_ID AS ${CAST_TEXT}), '') AS ${CAST_INTEGER}),
                           pricing_type = CAST(:PRICING_TYPE AS ${CAST_TEXT}),
                           is_featured = CAST(:IS_FEATURED AS ${CAST_INTEGER}),
                           sort_order = CAST(:SORT_ORDER AS ${CAST_INTEGER}),
                           tags = ${JIS}CAST(:TAGS AS ${CAST_TEXT})${JIE},
                           updated_at = ${NOW},
                           updated_id = CAST(:USERID AS ${CAST_INTEGER})
                     WHERE course_id = CAST(:COURSEID AS ${CAST_INTEGER})]]

-- ----------------------------------------------------------------------------
-- Forward: replace QueryRef #4502 so Options Retired is stored
-- ----------------------------------------------------------------------------
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
            UPDATE ${SCHEMA}${QUERIES}
            SET code = [==[
]] .. sql_new .. [[
            ]==]
            WHERE query_ref = ${QUERY_REF}
            AND query_type_a28 = ${TYPE_SQL};

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Store retired on QueryRef #${QUERY_REF} - ${QUERY_NAME}'           AS name,
        [=[
            # Forward Migration ${MIGRATION}: QueryRef #${QUERY_REF} retired

            Replaces the SQL for QueryRef #${QUERY_REF}. The Options
            Retired checkbox is INTEGER RETIRED. Title, summary,
            description, published, and stripe_product_id stay
            unchanged. This file does not ALTER the table.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})

-- ----------------------------------------------------------------------------
-- Reverse: restore the SQL from courses_4078.lua
-- ----------------------------------------------------------------------------
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
            UPDATE ${SCHEMA}${QUERIES}
            SET code = [==[
]] .. sql_old .. [[
            ]==]
            WHERE query_ref = ${QUERY_REF}
            AND query_type_a28 = ${TYPE_SQL};

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Restore QueryRef #${QUERY_REF} - ${QUERY_NAME}'                    AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Restore QueryRef #${QUERY_REF}

            Puts the courses_4078.lua SQL back. That statement does
            not write retired.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})

return queries end
