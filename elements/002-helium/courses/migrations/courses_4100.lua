-- Migration: courses_4100.lua
-- QueryRef #4514 - Staff: Save Working Title
--
-- Caller ref 4514 is not a file number. One transaction updates an
-- existing draft, inserts the draft when it is missing, and appends
-- one ledger row. course_name is not written. The suggestion must
-- already exist. Each bind name is used once per statement.
-- TRANSACTION_END is on its own line.
--
-- luacheck: no max line length
-- luacheck: no unused args
--
-- CHANGELOG
-- 1.0.0 - 2026-10-23 - Staff save of the working title

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "queries"
cfg.MIGRATION = "4100"
cfg.QUERY_REF = "4514"
cfg.QUERY_NAME = "Staff: Save Working Title"
if engine == "mysql" or engine == "mariadb" then
    cfg.CAST_INTEGER = "signed"
else
    cfg.CAST_INTEGER = cfg.INTEGER
end
cfg.CAST_TEXT = cfg.TEXT
if engine == "db2" then
    cfg.FROM_ONE = "FROM SYSIBM.SYSDUMMY1"
elseif engine == "firebird" then
    cfg.FROM_ONE = "FROM RDB$DATABASE"
else
    cfg.FROM_ONE = ""
end

local sql_new = [[
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
                ${QTC_SLOW}                                                         AS query_queue_a58,
                ${TIMEOUT}                                                          AS query_timeout,
                [==[
]] .. sql_new .. [[
                ]==]                                                                AS code,
                '${QUERY_NAME}'                                                     AS name,
                [==[
                    # QueryRef #${QUERY_REF} - ${QUERY_NAME}

                    Staff write (`TYPE_SQL`). Three statements, one
                    transaction.

                    1. Update `working_title` when the draft exists.
                    2. Insert the draft with state `suggested` when it
                       does not, and only when the suggestion exists.
                    3. Insert one `course_build_actions` row named
                       `working_title`. The measure columns are NULL.

                    `course_suggestions.course_name` is not updated.

                    ## Parameters

                    - `SUGGESTION_ID` (integer, required)
                    - `WORKING_TITLE` (string, required, max 100)
                    - `USERID` (integer, required)

                    ## Units on the ledger row

                    human_cost_cents, token_cost_cents, elapsed_ms,
                    and prompt_count stay NULL here. Confirm those
                    units before apply. See migration 4098.
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

            Installs the working-title save. The suggestion row is
            only read, to refuse an unknown id.
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

            Deletes the TYPE_SQL row only. Draft rows and ledger rows
            stay.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
return queries end
