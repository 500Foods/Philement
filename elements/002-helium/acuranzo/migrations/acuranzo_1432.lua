-- Migration: acuranzo_1432.lua
-- QueryRef #168 store reports.category
--
-- Does not edit acuranzo_1425.lua. REPORT_ID 0 still allocates a
-- report at rev_id 1. Any other id still appends the next revision.
-- :CATEGORY is used once. A blank is an empty string.
-- Deploy the Reports save change before this file, or with it.
-- An insert that names :CATEGORY rejects a caller that omits it.

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.0.0 - 2026-10-31 - Insert Report Revision stores category

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "queries"
cfg.MIGRATION = "1432"
cfg.QUERY_REF = "168"
cfg.QUERY_NAME = "Insert Report Revision"
if engine == "mysql" or engine == "mariadb" then
    cfg.CAST_INTEGER = "signed"
    cfg.CAST_TEXT = "char(255)"
else
    cfg.CAST_INTEGER = cfg.INTEGER
    cfg.CAST_TEXT = cfg.TEXT
end

local sql_new = [[
                    ${INSERT_KEY_START} report_id, rev_id ${INSERT_KEY_END}
                        INSERT INTO ${SCHEMA}reports (
                            report_id,
                            rev_id,
                            report,
                            thumbnail,
                            name,
                            category,
                            design,
                            scripts,
                            parameters,
                            summary,
                            ${COMMON_FIELDS}
                        )
                        SELECT
                            chosen.report_id,
                            chosen.rev_id,
                            :REPORT,
                            :THUMBNAIL,
                            CAST(:NAME AS ${CAST_TEXT}),
                            CAST(:CATEGORY AS ${CAST_TEXT}),
                            :DESIGN,
                            :SCRIPTS,
                            :PARAMETERS,
                            :SUMMARY,
                            ${COMMON_VALUES}
                          FROM (
                                SELECT
                                    CASE
                                        WHEN incoming.given_id = 0 THEN (
                                            SELECT COALESCE(MAX(next_report.report_id), 0) + 1
                                              FROM (
                                                    SELECT report_id
                                                      FROM ${SCHEMA}reports
                                                   ) next_report
                                        )
                                        ELSE incoming.given_id
                                    END AS report_id,
                                    CASE
                                        WHEN incoming.given_id = 0 THEN CAST(1 AS ${CAST_INTEGER})
                                        ELSE (
                                            SELECT COALESCE(MAX(same_rev.rev_id), 0) + 1
                                              FROM (
                                                    SELECT report_id, rev_id
                                                      FROM ${SCHEMA}reports
                                                   ) same_rev
                                             WHERE same_rev.report_id = incoming.given_id
                                        )
                                    END AS rev_id
                                  FROM (
                                        SELECT CAST(:REPORT_ID AS ${CAST_INTEGER}) AS given_id
                                        ${DUMMY_TABLE}
                                       ) incoming
                               ) chosen
                    ${INSERT_KEY_RETURN} report_id, rev_id
                    ;]]

local sql_old = [[
                    ${INSERT_KEY_START} report_id, rev_id ${INSERT_KEY_END}
                        INSERT INTO ${SCHEMA}reports (
                            report_id,
                            rev_id,
                            report,
                            thumbnail,
                            name,
                            design,
                            scripts,
                            parameters,
                            summary,
                            ${COMMON_FIELDS}
                        )
                        SELECT
                            chosen.report_id,
                            chosen.rev_id,
                            :REPORT,
                            :THUMBNAIL,
                            CAST(:NAME AS ${CAST_TEXT}),
                            :DESIGN,
                            :SCRIPTS,
                            :PARAMETERS,
                            :SUMMARY,
                            ${COMMON_VALUES}
                          FROM (
                                SELECT
                                    CASE
                                        WHEN incoming.given_id = 0 THEN (
                                            SELECT COALESCE(MAX(next_report.report_id), 0) + 1
                                              FROM (
                                                    SELECT report_id
                                                      FROM ${SCHEMA}reports
                                                   ) next_report
                                        )
                                        ELSE incoming.given_id
                                    END AS report_id,
                                    CASE
                                        WHEN incoming.given_id = 0 THEN CAST(1 AS ${CAST_INTEGER})
                                        ELSE (
                                            SELECT COALESCE(MAX(same_rev.rev_id), 0) + 1
                                              FROM (
                                                    SELECT report_id, rev_id
                                                      FROM ${SCHEMA}reports
                                                   ) same_rev
                                             WHERE same_rev.report_id = incoming.given_id
                                        )
                                    END AS rev_id
                                  FROM (
                                        SELECT CAST(:REPORT_ID AS ${CAST_INTEGER}) AS given_id
                                        ${DUMMY_TABLE}
                                       ) incoming
                               ) chosen
                    ${INSERT_KEY_RETURN} report_id, rev_id
                    ;]]

-- ----------------------------------------------------------------------------
-- Forward: replace QueryRef #168 so a revision stores category
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
        'Store category on QueryRef #${QUERY_REF} - ${QUERY_NAME}'          AS name,
        [=[
            # Forward Migration ${MIGRATION}: QueryRef #${QUERY_REF} category

            Replaces the SQL for QueryRef #${QUERY_REF} (Insert Report
            Revision). REPORT_ID 0 still allocates the next report_id
            at rev_id 1. Any other id appends the next rev_id.
            :CATEGORY is used once and is cast like :NAME. A blank is
            an empty string. This file does not ALTER the table.

            Apply waits for a human. Deploy the Reports save change
            before this rewrite, or together with it. The rewritten
            statement requires :CATEGORY.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})

-- ----------------------------------------------------------------------------
-- Reverse: restore the SQL from acuranzo_1425.lua
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

            Puts the acuranzo_1425.lua SQL back for QueryRef #${QUERY_REF}.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})

return queries end
