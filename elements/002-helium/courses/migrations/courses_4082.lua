-- Migration: courses_4082.lua
-- QueryRef #4501 also writes retired
--
-- Does not edit courses_4077.lua. published stays 0. Reload the
-- Course Manager before this file is applied so the insert sends
-- INTEGER RETIRED. Each name is used once.
--
-- luacheck: no max line length
-- luacheck: no unused args
--
-- CHANGELOG
-- 1.0.0 - 2026-10-22 - Staff insert writes courses.retired

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "queries"
cfg.MIGRATION = "4082"
cfg.QUERY_REF = "4501"
cfg.QUERY_NAME = "Staff: Insert Course"
if engine == "mysql" then
    cfg.CAST_INTEGER = "signed"
    cfg.CAST_TEXT = "char(255)"
else
    cfg.CAST_INTEGER = cfg.INTEGER
    cfg.CAST_TEXT = cfg.TEXT
end
if engine == "db2" then
    cfg.FROM_ONE = "FROM SYSIBM.SYSDUMMY1"
elseif engine == "firebird" then
    cfg.FROM_ONE = "FROM RDB$DATABASE"
else
    cfg.FROM_ONE = ""
end

local sql_new = [[
                    INSERT INTO ${SCHEMA}courses (
                        course_id,
                        code,
                        slug,
                        title,
                        image_path,
                        canvas_course_id,
                        pricing_type,
                        is_featured,
                        sort_order,
                        published,
                        retired,
                        tags,
                        created_id,
                        created_at,
                        updated_id,
                        updated_at
                    )
                    SELECT
                        COALESCE((SELECT MAX(course_id) FROM ${SCHEMA}courses), 0) + 1,
                        CAST(:CODE AS ${CAST_TEXT}),
                        CAST(:SLUG AS ${CAST_TEXT}),
                        CAST(:TITLE AS ${CAST_TEXT}),
                        NULLIF(CAST(:IMAGE_PATH AS ${CAST_TEXT}), ''),
                        CAST(NULLIF(CAST(:CANVAS_COURSE_ID AS ${CAST_TEXT}), '') AS ${CAST_INTEGER}),
                        CAST(:PRICING_TYPE AS ${CAST_TEXT}),
                        CAST(:IS_FEATURED AS ${CAST_INTEGER}),
                        CAST(:SORT_ORDER AS ${CAST_INTEGER}),
                        0,
                        CAST(:RETIRED AS ${CAST_INTEGER}),
                        ${JIS}CAST(:TAGS AS ${CAST_TEXT})${JIE},
                        CAST(:CREATEDID AS ${CAST_INTEGER}),
                        ${NOW},
                        CAST(:UPDATEDID AS ${CAST_INTEGER}),
                        ${NOW}
                    ${FROM_ONE}]]

local sql_old = [[
                    INSERT INTO ${SCHEMA}courses (
                        course_id,
                        code,
                        slug,
                        title,
                        image_path,
                        canvas_course_id,
                        pricing_type,
                        is_featured,
                        sort_order,
                        published,
                        tags,
                        created_id,
                        created_at,
                        updated_id,
                        updated_at
                    )
                    SELECT
                        COALESCE((SELECT MAX(course_id) FROM ${SCHEMA}courses), 0) + 1,
                        CAST(:CODE AS ${CAST_TEXT}),
                        CAST(:SLUG AS ${CAST_TEXT}),
                        CAST(:TITLE AS ${CAST_TEXT}),
                        NULLIF(CAST(:IMAGE_PATH AS ${CAST_TEXT}), ''),
                        CAST(NULLIF(CAST(:CANVAS_COURSE_ID AS ${CAST_TEXT}), '') AS ${CAST_INTEGER}),
                        CAST(:PRICING_TYPE AS ${CAST_TEXT}),
                        CAST(:IS_FEATURED AS ${CAST_INTEGER}),
                        CAST(:SORT_ORDER AS ${CAST_INTEGER}),
                        0,
                        ${JIS}CAST(:TAGS AS ${CAST_TEXT})${JIE},
                        CAST(:CREATEDID AS ${CAST_INTEGER}),
                        ${NOW},
                        CAST(:UPDATEDID AS ${CAST_INTEGER}),
                        ${NOW}
                    ${FROM_ONE}]]

-- ----------------------------------------------------------------------------
-- Forward: replace QueryRef #4501 so a new course can start retired
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

            Replaces the SQL for QueryRef #${QUERY_REF}. published
            stays 0. INTEGER RETIRED is the Options checkbox.
            MySQL casts use signed and char(255). DB2 and Firebird
            still select from the dummy row.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})

-- ----------------------------------------------------------------------------
-- Reverse: restore the SQL from courses_4077.lua
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

            Puts the courses_4077.lua SQL back. A new row then uses
            the column default for retired.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})

return queries end
