-- Migration: courses_4131.lua
-- QueryRef #4526 - Staff: Set Run State
--
-- CATCHUP Phase 22 add-on (P22a). Andrew amended P20a on 2026-10-27:
-- a logged staff override may set any run state, including research
-- and publishing. No silent or unlogged override.
-- Params: INTEGER SUGGESTION_ID, USERID; STRING FROM_STATE, TO_STATE.
-- One transaction. The ledger row is written first, only when the run
-- is still at FROM_STATE and TO_STATE is a P20a state that differs
-- from it. The guarded UPDATE then moves the run. A stale FROM_STATE
-- or a bad TO_STATE changes nothing (0 rows); the client reports
-- wrong_state. Ledger: action_name 'override', prompt = previous
-- state, result_ref = new state. Suggested clears receipt_mail, as
-- 4522 does. This query does not send the email.
--
-- luacheck: no max line length
-- luacheck: no unused args
--
-- CHANGELOG
-- 1.0.0 - 2026-10-27 - Staff: Set Run State (logged override)

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "queries"
cfg.MIGRATION = "4131"
cfg.QUERY_REF = "4526"
cfg.QUERY_NAME = "Staff: Set Run State"

if engine == "mysql" or engine == "mariadb" then
    cfg.CAST_INTEGER = "signed"
    cfg.CAST_TEXT = "char(255)"
    cfg.CAST_STATE = "char(50)"
else
    cfg.CAST_INTEGER = cfg.INTEGER
    cfg.CAST_TEXT = cfg.TEXT
    cfg.CAST_STATE = cfg.VARCHAR_50
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
                        'override',
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        src.from_state,
                        src.to_state,
                        NULL,
                        NULL,
                        src.user_id,
                        ${NOW},
                        src.user_id,
                        ${NOW}
                    FROM (
                        SELECT CAST(:SUGGESTION_ID AS ${CAST_INTEGER}) AS suggestion_id,
                               CAST(:USERID AS ${CAST_INTEGER}) AS user_id,
                               CAST(:FROM_STATE AS ${CAST_STATE}) AS from_state,
                               CAST(:TO_STATE AS ${CAST_STATE}) AS to_state
                        ${FROM_ONE}
                    ) src
                    JOIN ${SCHEMA}course_build_runs run
                      ON run.suggestion_id = src.suggestion_id
                     AND run.state = src.from_state
                    WHERE src.to_state <> src.from_state
                      AND src.to_state IN ('suggested', 'received', 'intake', 'duplicate', 'rejected', 'junk', 'submission', 'accepted', 'declined', 'outline', 'approve', 'research', 'review', 'build', 'inspect', 'checklist', 'draft', 'publishing', 'announcing', 'complete', 'abandoned', 'failed');
                    -- STATEMENT
                    UPDATE ${SCHEMA}course_build_runs
                       SET state = CAST(:TO_STATE AS ${CAST_STATE}),
                           receipt_mail = CASE
                               WHEN CAST(:TO_STATE AS ${CAST_STATE}) = 'suggested' THEN NULL
                               ELSE receipt_mail
                           END,
                           updated_id = CAST(:USERID AS ${CAST_INTEGER}),
                           updated_at = ${NOW}
                     WHERE suggestion_id = CAST(:SUGGESTION_ID AS ${CAST_INTEGER})
                       AND state = CAST(:FROM_STATE AS ${CAST_STATE})
                       AND CAST(:TO_STATE AS ${CAST_STATE}) <> CAST(:FROM_STATE AS ${CAST_STATE})
                       AND CAST(:TO_STATE AS ${CAST_STATE}) IN ('suggested', 'received', 'intake', 'duplicate', 'rejected', 'junk', 'submission', 'accepted', 'declined', 'outline', 'approve', 'research', 'review', 'build', 'inspect', 'checklist', 'draft', 'publishing', 'announcing', 'complete', 'abandoned', 'failed');

                    -- TRANSACTION_END
                ]==]                                                                AS code,
                '${QUERY_NAME}'                                                     AS name,
                [==[
                    # QueryRef #${QUERY_REF} - ${QUERY_NAME}

                    Logged staff override (P22a). Moves the run from FROM_STATE to any P20a TO_STATE in one transaction and writes one override ledger row (prompt = previous state, result_ref = new state). A stale FROM_STATE changes nothing. Suggested clears receipt_mail. This query does not send the email.
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

            Logged staff override (P22a). Moves the run from FROM_STATE to any P20a TO_STATE in one transaction and writes one override ledger row (prompt = previous state, result_ref = new state). A stale FROM_STATE changes nothing. Suggested clears receipt_mail. This query does not send the email.
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
