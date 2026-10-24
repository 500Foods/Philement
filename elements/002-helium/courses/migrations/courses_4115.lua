-- Migration: courses_4115.lua
-- QueryRef #4522 - Staff: Move to Suggested
--
-- Caller ref 4522 is not a file number. auth_query keeps the first
-- cache row for a query_ref. File numbers stay below 4500.
-- Moves Received or Intake back to suggested and clears receipt_mail.
-- The next receipt pass sends again. A missing draft is left alone.
-- This query does not send the email.
--
-- luacheck: no max line length
-- luacheck: no unused args
--
-- CHANGELOG
-- 1.0.0 - 2026-10-23 - Staff: Move to Suggested

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "queries"
cfg.MIGRATION = "4115"
cfg.QUERY_REF = "4522"
cfg.QUERY_NAME = "Staff: Move to Suggested"

if engine == "mysql" or engine == "mariadb" then
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
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
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
        ${QTC_SLOW}                                                      AS query_queue_a58,
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
                ${QTC_SLOW}                                                      AS query_queue_a58,
                ${TIMEOUT}                                                          AS query_timeout,
                [==[
                    -- TRANSACTION_START
                    UPDATE ${SCHEMA}course_build_runs
                       SET state = 'suggested',
                           receipt_mail = NULL,
                           updated_id = CAST(:USERID AS ${CAST_INTEGER}),
                           updated_at = ${NOW}
                     WHERE suggestion_id = CAST(:SUGGESTION_ID AS ${CAST_INTEGER})
                       AND state IN ('received', 'intake');
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
                        'suggested',
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
                      ON run.suggestion_id = src.suggestion_id
                     AND run.state = 'suggested';

                    -- TRANSACTION_END
                ]==]                                                                AS code,
                '${QUERY_NAME}'                                                     AS name,
                [==[
                    # QueryRef #${QUERY_REF} - ${QUERY_NAME}

                    Moves Received or Intake back to suggested and clears receipt_mail. The next receipt pass sends again. A missing draft is left alone. This query does not send the email.
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

            Moves Received or Intake back to suggested and clears receipt_mail. The next receipt pass sends again. A missing draft is left alone. This query does not send the email.
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

            Deletes the TYPE_SQL row only.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
return queries end
