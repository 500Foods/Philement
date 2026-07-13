-- Migration: acuranzo_1264.lua
-- QueryRef #130 - Update Script for the Hydrogen Scripting Subsystem (Scripting Manager CRUD)

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.0.0 - 2026-07-13 - Initial creation for Scripting Manager CRUD

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "queries"
cfg.MIGRATION = "1264"
cfg.QUERY_REF = "130"
cfg.QUERY_NAME = "Update Script"
-- ----------------------------------------------------------------------------
-- Forward: Populate QueryRef #130 - Update Script
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
        ${QTC_MEDIUM}                                                       AS query_queue_a58,
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
                ${QTC_MEDIUM}                                                       AS query_queue_a58,
                ${TIMEOUT}                                                          AS query_timeout,
                [==[
                    UPDATE ${SCHEMA}scripts
                       SET script_type     = :SCRIPT_TYPE,
                           schedule        = :SCHEDULE,
                           next_run        = :NEXT_RUN,
                           last_run_start  = :LAST_RUN_START,
                           last_run_end    = :LAST_RUN_END,
                           status          = :STATUS,
                           code            = :CODE,
                           summary         = :SUMMARY,
                           updated_at      = ${NOW},
                           updated_id      = :USERID
                     WHERE group_name      = :GROUP_NAME
                       AND script_name     = :SCRIPT_NAME
                ]==]                                                                AS code,
                '${QUERY_NAME}'                                                     AS name,
                [==[
                    # QueryRef #${QUERY_REF} - ${QUERY_NAME}

                    Updates an existing row in the `scripts` table.

                    ## Parameters

                    - `GROUP_NAME` (string, required): The script's group.
                    - `SCRIPT_NAME` (string, required): The script's name.
                    - `SCRIPT_TYPE` (integer, required): Lookup 061 value.
                    - `SCHEDULE` (text, optional): Cron-like schedule string.
                    - `NEXT_RUN` (timestamp, optional): Next scheduled run.
                    - `LAST_RUN_START` (timestamp, optional): Last run start time.
                    - `LAST_RUN_END` (timestamp, optional): Last run end time.
                    - `STATUS` (integer, required): Lookup 062 value.
                    - `CODE` (text, optional): Lua source code.
                    - `SUMMARY` (text, optional): Markdown description.
                    - `USERID` (integer, required): ID of the user performing the update.

                    ## Returns

                    - Returns affected rows count.

                    ## Tables

                    - `${SCHEMA}scripts`: Script registry (migration 1201).

                    ## Notes

                    - The composite primary key is `(group_name, script_name)`.
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
            # Forward Migration ${MIGRATION}: Populate QueryRef #${QUERY_REF} - ${QUERY_NAME}

            This migration creates the query for QueryRef #${QUERY_REF} - ${QUERY_NAME}
            for the Hydrogen Scripting Subsystem (Phase 11b).
        ]=]
                                                                             AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})

-- ----------------------------------------------------------------------------
-- Reverse: Remove QueryRef #130 - Update Script
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
            DELETE FROM ${SCHEMA}${TABLE}
            WHERE query_ref = ${QUERY_REF};

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

            This migration removes QueryRef #${QUERY_REF} - ${QUERY_NAME}.
        ]=]
                                                                             AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})

return queries
end
