-- Migration: courses_4112.lua
-- QueryRef #4517 - cast title and annotation as varchar
--
-- Does not edit courses_4104.lua. Same caller ref 4517.
-- working_title is varchar(100). annotation is varchar(500).
-- Detail stays a bare bind. State and receipt_mail stay put.
-- MySQL casts use char(n). Reverse puts the text casts back.
--
-- luacheck: no max line length
-- luacheck: no unused args
--
-- CHANGELOG
-- 1.0.0 - 2026-10-23 - QueryRef 4517 casts title and note as varchar

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "queries"
cfg.MIGRATION = "4112"
cfg.QUERY_REF = "4517"
cfg.QUERY_NAME = "Staff: Update Suggestion"
if engine == "mysql" then
    cfg.CAST_INTEGER = "signed"
    cfg.CAST_TEXT = "char(255)"
    cfg.CAST_TITLE = "char(100)"
    cfg.CAST_NOTE = "char(255)"
elseif engine == "mariadb" then
    cfg.CAST_INTEGER = "signed"
    cfg.CAST_TEXT = "char(255)"
    cfg.CAST_TITLE = cfg.VARCHAR_100
    cfg.CAST_NOTE = cfg.VARCHAR_500
else
    cfg.CAST_INTEGER = cfg.INTEGER
    cfg.CAST_TEXT = cfg.TEXT
    cfg.CAST_TITLE = cfg.VARCHAR_100
    cfg.CAST_NOTE = cfg.VARCHAR_500
end
if engine == "db2" then
    cfg.FROM_ONE = "FROM SYSIBM.SYSDUMMY1"
elseif engine == "firebird" then
    cfg.FROM_ONE = "FROM RDB$DATABASE"
else
    cfg.FROM_ONE = ""
end

local sql_old = [[
                    -- TRANSACTION_START
                    UPDATE ${SCHEMA}course_suggestions
                       SET course_name = :COURSE_NAME,
                           summary = :SUMMARY,
                           detail = :DETAIL,
                           intended_audience = :INTENDED_AUDIENCE,
                           additional_features = :ADDITIONAL_FEATURES,
                           submitter_name = :SUBMITTER_NAME,
                           submitter_email = :SUBMITTER_EMAIL,
                           updated_id = CAST(:USERID AS ${CAST_INTEGER}),
                           updated_at = ${NOW}
                     WHERE suggestion_id = CAST(:SUGGESTION_ID AS ${CAST_INTEGER});
                    -- STATEMENT
                    UPDATE ${SCHEMA}course_build_runs
                       SET working_title = CAST(:WORKING_TITLE AS ${CAST_TEXT}),
                           annotation = CAST(:ANNOTATION AS ${CAST_TEXT}),
                           updated_id = CAST(:USERID AS ${CAST_INTEGER}),
                           updated_at = ${NOW}
                     WHERE suggestion_id = CAST(:SUGGESTION_ID AS ${CAST_INTEGER});
                    -- STATEMENT
                    INSERT INTO ${SCHEMA}course_build_runs (
                        run_id,
                        suggestion_id,
                        working_title,
                        state,
                        receipt_mail,
                        flagged,
                        annotation,
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
                        0,
                        src.annotation,
                        NULL,
                        NULL,
                        src.user_id,
                        ${NOW},
                        src.user_id,
                        ${NOW}
                    FROM (
                        SELECT CAST(:SUGGESTION_ID AS ${CAST_INTEGER}) AS suggestion_id,
                               CAST(:WORKING_TITLE AS ${CAST_TEXT}) AS working_title,
                               CAST(:ANNOTATION AS ${CAST_TEXT}) AS annotation,
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
                        'edit',
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
                    UPDATE ${SCHEMA}course_suggestions
                       SET course_name = :COURSE_NAME,
                           summary = :SUMMARY,
                           detail = :DETAIL,
                           intended_audience = :INTENDED_AUDIENCE,
                           additional_features = :ADDITIONAL_FEATURES,
                           submitter_name = :SUBMITTER_NAME,
                           submitter_email = :SUBMITTER_EMAIL,
                           updated_id = CAST(:USERID AS ${CAST_INTEGER}),
                           updated_at = ${NOW}
                     WHERE suggestion_id = CAST(:SUGGESTION_ID AS ${CAST_INTEGER});
                    -- STATEMENT
                    UPDATE ${SCHEMA}course_build_runs
                       SET working_title = CAST(:WORKING_TITLE AS ${CAST_TITLE}),
                           annotation = CAST(:ANNOTATION AS ${CAST_NOTE}),
                           updated_id = CAST(:USERID AS ${CAST_INTEGER}),
                           updated_at = ${NOW}
                     WHERE suggestion_id = CAST(:SUGGESTION_ID AS ${CAST_INTEGER});
                    -- STATEMENT
                    INSERT INTO ${SCHEMA}course_build_runs (
                        run_id,
                        suggestion_id,
                        working_title,
                        state,
                        receipt_mail,
                        flagged,
                        annotation,
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
                        0,
                        src.annotation,
                        NULL,
                        NULL,
                        src.user_id,
                        ${NOW},
                        src.user_id,
                        ${NOW}
                    FROM (
                        SELECT CAST(:SUGGESTION_ID AS ${CAST_INTEGER}) AS suggestion_id,
                               CAST(:WORKING_TITLE AS ${CAST_TITLE}) AS working_title,
                               CAST(:ANNOTATION AS ${CAST_NOTE}) AS annotation,
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
                        'edit',
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

            Replaces the SQL for QueryRef #4517. The title cast is varchar(100) and the annotation cast is varchar(500). Detail stays unbound text. receipt_mail is not written.
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

            Puts the courses_4104.lua SQL back, including the text casts.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
return queries end
