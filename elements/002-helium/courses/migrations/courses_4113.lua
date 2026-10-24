-- Migration: courses_4113.lua
-- QueryRef #4518 - cast the copied name as varchar(100)
--
-- Does not edit courses_4105.lua. Same caller ref 4518.
-- course_name is varchar(100). The copy still has no draft,
-- so it stays Suggested and this statement sends no email.
-- Reverse puts the text cast back.
--
-- luacheck: no max line length
-- luacheck: no unused args
--
-- CHANGELOG
-- 1.0.0 - 2026-10-23 - QueryRef 4518 casts the copied name as varchar(100)

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "queries"
cfg.MIGRATION = "4113"
cfg.QUERY_REF = "4518"
cfg.QUERY_NAME = "Staff: Duplicate Suggestion"
if engine == "mysql" then
    cfg.CAST_INTEGER = "signed"
    cfg.CAST_TITLE = "char(100)"
else
    cfg.CAST_INTEGER = cfg.INTEGER
    cfg.CAST_TITLE = cfg.VARCHAR_100
end
cfg.CAST_TEXT = cfg.TEXT
if engine == "db2" then
    cfg.FROM_ONE = "FROM SYSIBM.SYSDUMMY1"
elseif engine == "firebird" then
    cfg.FROM_ONE = "FROM RDB$DATABASE"
else
    cfg.FROM_ONE = ""
end

local sql_old = [[
                    INSERT INTO ${SCHEMA}course_suggestions (
                        suggestion_id,
                        course_name,
                        summary,
                        detail,
                        intended_audience,
                        additional_features,
                        submitter_name,
                        submitter_email,
                        chacha_token,
                        chacha_site,
                        ip_address,
                        review_status,
                        valid_after,
                        valid_until,
                        created_id,
                        created_at,
                        updated_id,
                        updated_at
                    )
                    SELECT
                        COALESCE((SELECT MAX(suggestion_id) FROM ${SCHEMA}course_suggestions), 0) + 1,
                        src.course_name,
                        found.summary,
                        found.detail,
                        found.intended_audience,
                        found.additional_features,
                        found.submitter_name,
                        found.submitter_email,
                        'staff',
                        'staff',
                        NULL,
                        0,
                        NULL,
                        NULL,
                        src.user_id,
                        ${NOW},
                        src.user_id,
                        ${NOW}
                    FROM (
                        SELECT CAST(:SUGGESTION_ID AS ${CAST_INTEGER}) AS suggestion_id,
                               CAST(:COURSE_NAME AS ${CAST_TEXT}) AS course_name,
                               CAST(:USERID AS ${CAST_INTEGER}) AS user_id
                        ${FROM_ONE}
                    ) src
                    JOIN ${SCHEMA}course_suggestions found
                      ON found.suggestion_id = src.suggestion_id;
                ]]
local sql_new = [[
                    INSERT INTO ${SCHEMA}course_suggestions (
                        suggestion_id,
                        course_name,
                        summary,
                        detail,
                        intended_audience,
                        additional_features,
                        submitter_name,
                        submitter_email,
                        chacha_token,
                        chacha_site,
                        ip_address,
                        review_status,
                        valid_after,
                        valid_until,
                        created_id,
                        created_at,
                        updated_id,
                        updated_at
                    )
                    SELECT
                        COALESCE((SELECT MAX(suggestion_id) FROM ${SCHEMA}course_suggestions), 0) + 1,
                        src.course_name,
                        found.summary,
                        found.detail,
                        found.intended_audience,
                        found.additional_features,
                        found.submitter_name,
                        found.submitter_email,
                        'staff',
                        'staff',
                        NULL,
                        0,
                        NULL,
                        NULL,
                        src.user_id,
                        ${NOW},
                        src.user_id,
                        ${NOW}
                    FROM (
                        SELECT CAST(:SUGGESTION_ID AS ${CAST_INTEGER}) AS suggestion_id,
                               CAST(:COURSE_NAME AS ${CAST_TITLE}) AS course_name,
                               CAST(:USERID AS ${CAST_INTEGER}) AS user_id
                        ${FROM_ONE}
                    ) src
                    JOIN ${SCHEMA}course_suggestions found
                      ON found.suggestion_id = src.suggestion_id;
                ]]

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
        'Rewrite QueryRef #${QUERY_REF} - ${QUERY_NAME}'                    AS name,
        [=[
            # Forward Migration ${MIGRATION}: QueryRef #${QUERY_REF}

            Replaces the SQL for QueryRef #4518. The copied course name is cast as varchar(100). The new row still has no draft.
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

            Puts the courses_4105.lua SQL back, including the text cast on the course name.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
return queries end
