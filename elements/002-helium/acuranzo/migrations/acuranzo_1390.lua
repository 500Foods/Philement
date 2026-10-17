-- Migration: acuranzo_1390.lua
-- QueryRef #089 add invokable to List All Scripts
--
-- CATCHUP Phase 7. Does not edit migration 1206.
-- Adds invokable to the SELECT list. No new parameter.

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.0.0 - 2026-10-16 - Add invokable to QueryRef #089 List All Scripts

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "queries"
cfg.MIGRATION = "1390"
cfg.QUERY_REF = "089"
cfg.QUERY_NAME = "List All Scripts (no code)"

local sql_new = [[
                    SELECT
                        group_name,
                        script_name,
                        script_type,
                        schedule,
                        next_run,
                        last_run_start,
                        last_run_end,
                        status,
                        summary,
                        invokable
                    FROM
                        ${SCHEMA}scripts
                    ORDER BY
                        group_name ASC,
                        script_name ASC]]

local sql_old = [[
                    SELECT
                        group_name,
                        script_name,
                        script_type,
                        schedule,
                        next_run,
                        last_run_start,
                        last_run_end,
                        status,
                        summary
                    FROM
                        ${SCHEMA}scripts
                    ORDER BY
                        group_name ASC,
                        script_name ASC]]

-- ----------------------------------------------------------------------------
-- Forward: replace QueryRef #089 so scripts.invokable is in the SQL
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
        'Add invokable to QueryRef #${QUERY_REF} - ${QUERY_NAME}'           AS name,
        [=[
            # Forward Migration ${MIGRATION}: QueryRef #${QUERY_REF} invokable

            Replaces the SQL installed for QueryRef #${QUERY_REF}
            (List All Scripts (no code)) so `invokable` is selected or written.
            The column already exists (migration 1297, INTEGER_SMALL,
            NOT NULL, DEFAULT 0). This file does not ALTER the table.

            Apply waits for a human. QueryRef #129 and #130 require
            INTEGER parameter INVOKABLE. A missing :NAME is rejected
            before SQL runs.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})

-- ----------------------------------------------------------------------------
-- Reverse: restore the SQL from the installing migration
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

            Puts the pre-invokable SQL back for QueryRef #${QUERY_REF}.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})

return queries end

