-- Migration: acuranzo_1396.lua
-- QueryRef #129 store a blank schedule and blank run times as NULL
--
-- Does not edit the migration that last replaced this QueryRef.
-- Hydrogen rejects JSON null in every parameter group. Lithium sends
-- '' for a blank schedule or run time. Each name is used once.
-- CAST the text before NULLIF so DB2 does not size the marker from ''.

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.0.0 - 2026-10-22 - Blank schedule and run times store SQL NULL

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "queries"
cfg.MIGRATION = "1396"
cfg.QUERY_REF = "129"
cfg.QUERY_NAME = "Insert Script"
if engine == "mysql" then
    cfg.CAST_TEXT = "char(255)"
else
    cfg.CAST_TEXT = cfg.TEXT
end

local sql_new = [[
                    INSERT INTO ${SCHEMA}scripts (
                        group_name,
                        script_name,
                        script_type,
                        schedule,
                        next_run,
                        last_run_start,
                        last_run_end,
                        status,
                        code,
                        summary,
                        invokable,
                        ${COMMON_FIELDS}
                    )
                    VALUES (
                        :GROUP_NAME,
                        :SCRIPT_NAME,
                        :SCRIPT_TYPE,
                        NULLIF(CAST(:SCHEDULE AS ${CAST_TEXT}), ''),
                        CAST(NULLIF(CAST(:NEXT_RUN AS ${CAST_TEXT}), '') AS ${TIMESTAMP_TZ}),
                        CAST(NULLIF(CAST(:LAST_RUN_START AS ${CAST_TEXT}), '') AS ${TIMESTAMP_TZ}),
                        CAST(NULLIF(CAST(:LAST_RUN_END AS ${CAST_TEXT}), '') AS ${TIMESTAMP_TZ}),
                        :STATUS,
                        :CODE,
                        :SUMMARY,
                        :INVOKABLE,
                        ${COMMON_VALUES}
                    )]]

local sql_old = [[
                    INSERT INTO ${SCHEMA}scripts (
                        group_name,
                        script_name,
                        script_type,
                        schedule,
                        next_run,
                        last_run_start,
                        last_run_end,
                        status,
                        code,
                        summary,
                        invokable,
                        ${COMMON_FIELDS}
                    )
                    VALUES (
                        :GROUP_NAME,
                        :SCRIPT_NAME,
                        :SCRIPT_TYPE,
                        :SCHEDULE,
                        :NEXT_RUN,
                        :LAST_RUN_START,
                        :LAST_RUN_END,
                        :STATUS,
                        :CODE,
                        :SUMMARY,
                        :INVOKABLE,
                        ${COMMON_VALUES}
                    )]]

-- ----------------------------------------------------------------------------
-- Forward: replace QueryRef #129 so a blank string stores NULL
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
        'Blank schedule stores NULL on QueryRef #${QUERY_REF} - ${QUERY_NAME}' AS name,
        [=[
            # Forward Migration ${MIGRATION}: QueryRef #${QUERY_REF} blank schedule

            Replaces the SQL for QueryRef #${QUERY_REF} (Insert Script).
            A blank SCHEDULE, NEXT_RUN, LAST_RUN_START, or LAST_RUN_END
            stores SQL NULL. A value is kept. This file does not ALTER
            the table. INTEGER SCRIPT_TYPE, STATUS, USERID, and
            INVOKABLE stay direct assignments.

            Apply waits for a human. Deploy the Scripting save change
            after this migration and 1395 are applied.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})

-- ----------------------------------------------------------------------------
-- Reverse: restore the SQL from acuranzo_1392.lua
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

            Puts the acuranzo_1392.lua SQL back for QueryRef #${QUERY_REF}.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})

return queries end
