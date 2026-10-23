-- Migration: courses_4077.lua
-- QueryRef #4501 - Staff: Insert Course
--
-- 4501 is not a file number. A type 1 row at 4009 would lose to the
-- type 1001 reverse row of courses_4009.lua in the auth_query cache.
--
-- TYPE_SQL. Creates one Lithium catalog row. published stays 0.
-- Sync from Canvas does not insert. Title is required because the
-- column is NOT NULL. Canvas copy replaces it on a later sync.
-- Each parameter name is used once.
--
-- luacheck: no max line length
-- luacheck: no unused args
--
-- CHANGELOG
-- 1.0.0 - 2026-10-22 - Staff insert of a courses row (CATCHUP Phase 13)

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "queries"
cfg.MIGRATION = "4077"
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
        ${QTC_FAST}                                                         AS query_queue_a58,
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
                ${TYPE_SQL}                                                         AS query_type_a28,
                ${DIALECT}                                                          AS query_dialect_a30,
                ${QTC_FAST}                                                         AS query_queue_a58,
                ${TIMEOUT}                                                          AS query_timeout,
                [==[
]] .. sql_new .. [[
                ]==]                                                                AS code,
                '${QUERY_NAME}'                                                     AS name,
                [==[
                    # QueryRef #${QUERY_REF} - ${QUERY_NAME}

                    CATCHUP Phase 13. Not public (`TYPE_SQL`).

                    Inserts one `courses` row. `published` is 0. `retired`
                    stays the column default. `stripe_product_id` stays
                    NULL. Summary and description stay NULL until Sync
                    from Canvas. This statement does not call that sync.

                    `course_id` is MAX+1. Each name is bound once.
                    Empty image path and empty canvas id store NULL.
                    `TAGS` is a JSON object. Send `{}` when there are
                    no keys. MySQL casts use `signed` and `char(255)`.
                    On that engine a title longer than 255 characters
                    does not fit the cast. Yugabyte casts text.
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
            # Forward Migration ${MIGRATION}: Populate QueryRef #${QUERY_REF}
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
            WHERE query_ref = ${QUERY_REF}
              AND query_type_a28 = ${TYPE_SQL};

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Remove QueryRef #${QUERY_REF}'                                     AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Remove QueryRef #${QUERY_REF}

            Deletes the TYPE_SQL row only. Courses rows stay.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
return queries end
