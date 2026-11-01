-- Migration: acuranzo_1429.lua
-- QueryRef #165 include reports.category
--
-- Does not edit acuranzo_1422.lua. The list returns the latest
-- revision and now includes category. Search stays name-only.

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.0.0 - 2026-10-31 - List Reports returns category

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "queries"
cfg.MIGRATION = "1429"
cfg.QUERY_REF = "165"
cfg.QUERY_NAME = "List Reports"

local sql_new = [[
                    SELECT r.report_id,
                           r.rev_id,
                           r.name,
                           r.category,
                           r.summary,
                           r.design,
                           r.scripts,
                           r.parameters
                      FROM ${SCHEMA}reports r
                      JOIN (
                            SELECT report_id,
                                   MAX(rev_id) AS rev_id
                              FROM ${SCHEMA}reports
                             GROUP BY report_id
                           ) latest
                        ON latest.report_id = r.report_id
                       AND latest.rev_id = r.rev_id
                     ORDER BY r.name, r.report_id]]

local sql_old = [[
                    SELECT r.report_id,
                           r.rev_id,
                           r.name,
                           r.summary,
                           r.design,
                           r.scripts,
                           r.parameters
                      FROM ${SCHEMA}reports r
                      JOIN (
                            SELECT report_id,
                                   MAX(rev_id) AS rev_id
                              FROM ${SCHEMA}reports
                             GROUP BY report_id
                           ) latest
                        ON latest.report_id = r.report_id
                       AND latest.rev_id = r.rev_id
                     ORDER BY r.name, r.report_id]]

-- ----------------------------------------------------------------------------
-- Forward: replace QueryRef #165 so the list includes category
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
        'List category on QueryRef #${QUERY_REF} - ${QUERY_NAME}'           AS name,
        [=[
            # Forward Migration ${MIGRATION}: QueryRef #${QUERY_REF} category

            Replaces the SQL for QueryRef #${QUERY_REF} (List Reports).
            The latest revision of each report now includes `category`.
            `report` and `thumbnail` stay off this list. This file does
            not ALTER the table. Apply waits for a human.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})

-- ----------------------------------------------------------------------------
-- Reverse: restore the SQL from acuranzo_1422.lua
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

            Puts the acuranzo_1422.lua SQL back for QueryRef #${QUERY_REF}.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})

return queries end
