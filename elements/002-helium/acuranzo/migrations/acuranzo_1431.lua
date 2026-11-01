-- Migration: acuranzo_1431.lua
-- QueryRef #167 include reports.category
--
-- Does not edit acuranzo_1424.lua. One revision still matches
-- report_id and rev_id. The row now includes category.

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.0.0 - 2026-10-31 - Get Report returns category

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "queries"
cfg.MIGRATION = "1431"
cfg.QUERY_REF = "167"
cfg.QUERY_NAME = "Get Report"
if engine == "mysql" or engine == "mariadb" then
    cfg.CAST_INTEGER = "signed"
else
    cfg.CAST_INTEGER = cfg.INTEGER
end

local sql_new = [[
                    SELECT report_id,
                           rev_id,
                           name,
                           category,
                           summary,
                           report,
                           thumbnail,
                           design,
                           scripts,
                           parameters
                      FROM ${SCHEMA}reports
                     WHERE report_id = CAST(:REPORT_ID AS ${CAST_INTEGER})
                       AND rev_id = CAST(:REV_ID AS ${CAST_INTEGER})]]

local sql_old = [[
                    SELECT report_id,
                           rev_id,
                           name,
                           summary,
                           report,
                           thumbnail,
                           design,
                           scripts,
                           parameters
                      FROM ${SCHEMA}reports
                     WHERE report_id = CAST(:REPORT_ID AS ${CAST_INTEGER})
                       AND rev_id = CAST(:REV_ID AS ${CAST_INTEGER})]]

-- ----------------------------------------------------------------------------
-- Forward: replace QueryRef #167 so one revision includes category
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
        'Get category on QueryRef #${QUERY_REF} - ${QUERY_NAME}'            AS name,
        [=[
            # Forward Migration ${MIGRATION}: QueryRef #${QUERY_REF} category

            Replaces the SQL for QueryRef #${QUERY_REF} (Get Report).
            One revision still matches REPORT_ID and REV_ID. Each name
            is used once. The row now includes `category`. This file
            does not ALTER the table. Apply waits for a human.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})

-- ----------------------------------------------------------------------------
-- Reverse: restore the SQL from acuranzo_1424.lua
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

            Puts the acuranzo_1424.lua SQL back for QueryRef #${QUERY_REF}.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})

return queries end
