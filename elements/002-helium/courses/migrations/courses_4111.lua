-- Migration: courses_4111.lua
-- QueryRef #4514 - cast the working title as varchar(100)
--
-- Does not edit courses_4100.lua. Same caller ref 4514.
-- Statement 1 assigns the title. CAST AS text into varchar(100)
-- fails on this database. The title cast now matches the column.
-- course_name is still not written. Reverse puts the text cast back.
--
-- luacheck: no max line length
-- luacheck: no unused args
--
-- CHANGELOG
-- 1.0.0 - 2026-10-23 - QueryRef 4514 casts the title as varchar(100)

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "queries"
cfg.MIGRATION = "4111"
cfg.QUERY_REF = "4514"
cfg.QUERY_NAME = "Staff: Save Working Title"
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
                    -- TRANSACTION_START
                    UPDATE ${SCHEMA}course_build_runs
                       SET working_title = CAST(:WORKING_TITLE AS ${CAST_TEXT}),
                           updated_id = CAST(:USERID AS ${CAST_INTEGER}),
                           updated_at = ${NOW}
                     WHERE suggestion_id = CAST(:SUGGESTION_ID AS ${CAST_INTEGER});
                    -- STATEMENT
                    INSERT INTO ${SCHEMA}course_build_runs (
                        run_id,
                        suggestion_id,
                        working_title,
                        state,
                        valid_after,
                        valid_until,
                        created_id,
                        created_at,
                        updated_id,
                        updated_at
                    )
                    SELECT
                        COALESCE((SELECT MAX(run_id) FROM ${SCHEMA}course_build_runs), 0) + 1,
                        src.suggestion_id,
                        src.working_title,
                        'suggested',
                        NULL,
                        NULL,
                        src.user_id,
                        ${NOW},
                        src.user_id,
                        ${NOW}
                    FROM (
                        SELECT CAST(:SUGGESTION_ID AS ${CAST_INTEGER}) AS suggestion_id,
                               CAST(:WORKING_TITLE AS ${CAST_TEXT}) AS working_title,
                               CAST(:USERID AS ${CAST_INTEGER}) AS user_id
                        ${FROM_ONE}
                    ) src
                    WHERE NOT EXISTS (
                        SELECT 1
                          FROM ${SCHEMA}course_build_runs existing
                         WHERE existing.suggestion_id = src.suggestion_id
                    )
                      AND EXISTS (
                        SELECT 1
                          FROM ${SCHEMA}course_suggestions suggestion
                         WHERE suggestion.suggestion_id = src.suggestion_id
                    );
                    -- STATEMENT
                    INSERT INTO ${SCHEMA}course_build_actions (
                        action_id,
                        run_id,
                        actor_account_id,
                        action_name,
                        human_cost_cents,
                        token_cost_cents,
                        elapsed_ms,
                        prompt_count,
                        prompt,
                        result_ref,
                        valid_after,
                        valid_until,
                        created_id,
                        created_at,
                        updated_id,
                        updated_at
                    )
                    SELECT
                        COALESCE((SELECT MAX(action_id) FROM ${SCHEMA}course_build_actions), 0) + 1,
                        run.run_id,
                        src.user_id,
                        'working_title',
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        src.user_id,
                        ${NOW},
                        src.user_id,
                        ${NOW}
                    FROM (
                        SELECT CAST(:SUGGESTION_ID AS ${CAST_INTEGER}) AS suggestion_id,
                               CAST(:USERID AS ${CAST_INTEGER}) AS user_id
                        ${FROM_ONE}
                    ) src
                    JOIN ${SCHEMA}course_build_runs run
                      ON run.suggestion_id = src.suggestion_id;

                    -- TRANSACTION_END
]]
local sql_new = [[
                    -- TRANSACTION_START
                    UPDATE ${SCHEMA}course_build_runs
                       SET working_title = CAST(:WORKING_TITLE AS ${CAST_TITLE}),
                           updated_id = CAST(:USERID AS ${CAST_INTEGER}),
                           updated_at = ${NOW}
                     WHERE suggestion_id = CAST(:SUGGESTION_ID AS ${CAST_INTEGER});
                    -- STATEMENT
                    INSERT INTO ${SCHEMA}course_build_runs (
                        run_id,
                        suggestion_id,
                        working_title,
                        state,
                        valid_after,
                        valid_until,
                        created_id,
                        created_at,
                        updated_id,
                        updated_at
                    )
                    SELECT
                        COALESCE((SELECT MAX(run_id) FROM ${SCHEMA}course_build_runs), 0) + 1,
                        src.suggestion_id,
                        src.working_title,
                        'suggested',
                        NULL,
                        NULL,
                        src.user_id,
                        ${NOW},
                        src.user_id,
                        ${NOW}
                    FROM (
                        SELECT CAST(:SUGGESTION_ID AS ${CAST_INTEGER}) AS suggestion_id,
                               CAST(:WORKING_TITLE AS ${CAST_TITLE}) AS working_title,
                               CAST(:USERID AS ${CAST_INTEGER}) AS user_id
                        ${FROM_ONE}
                    ) src
                    WHERE NOT EXISTS (
                        SELECT 1
                          FROM ${SCHEMA}course_build_runs existing
                         WHERE existing.suggestion_id = src.suggestion_id
                    )
                      AND EXISTS (
                        SELECT 1
                          FROM ${SCHEMA}course_suggestions suggestion
                         WHERE suggestion.suggestion_id = src.suggestion_id
                    );
                    -- STATEMENT
                    INSERT INTO ${SCHEMA}course_build_actions (
                        action_id,
                        run_id,
                        actor_account_id,
                        action_name,
                        human_cost_cents,
                        token_cost_cents,
                        elapsed_ms,
                        prompt_count,
                        prompt,
                        result_ref,
                        valid_after,
                        valid_until,
                        created_id,
                        created_at,
                        updated_id,
                        updated_at
                    )
                    SELECT
                        COALESCE((SELECT MAX(action_id) FROM ${SCHEMA}course_build_actions), 0) + 1,
                        run.run_id,
                        src.user_id,
                        'working_title',
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        src.user_id,
                        ${NOW},
                        src.user_id,
                        ${NOW}
                    FROM (
                        SELECT CAST(:SUGGESTION_ID AS ${CAST_INTEGER}) AS suggestion_id,
                               CAST(:USERID AS ${CAST_INTEGER}) AS user_id
                        ${FROM_ONE}
                    ) src
                    JOIN ${SCHEMA}course_build_runs run
                      ON run.suggestion_id = src.suggestion_id;

                    -- TRANSACTION_END
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

            Replaces the SQL for QueryRef #4514. The working title is cast as varchar(100). course_name is not written. The row must already exist.
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

            Puts the courses_4100.lua SQL back, including the text cast on the working title.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
return queries end
