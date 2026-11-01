-- Migration: acuranzo_1425.lua
-- QueryRef #168 - Insert Report Revision

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.0.0 - 2026-10-31 - Initial creation

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "queries"
cfg.MIGRATION = "1425"
cfg.QUERY_REF = "168"
cfg.QUERY_NAME = "Insert Report Revision"
if engine == "mysql" or engine == "mariadb" then
    cfg.CAST_INTEGER = "signed"
    cfg.CAST_TEXT = "char(255)"
else
    cfg.CAST_INTEGER = cfg.INTEGER
    cfg.CAST_TEXT = cfg.TEXT
end

-- report_id allocation is COALESCE(MAX(report_id), 0) + 1, the same
-- shape as courses_4103.lua. The source is a derived table of the
-- integer columns so the INSERT does not read the target table's
-- large columns. REPORT_ID 0 allocates a new report at rev_id 1.
-- Any other REPORT_ID appends MAX(rev_id) + 1 for that report.
-- Each parameter name is used once.
local sql_new = [[
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

                    Appends one revision. It does not update the open rev_id.
                    INTEGER REPORT_ID 0 allocates the next report_id and
                    stores rev_id 1. Any other REPORT_ID stores that id and
                    the next rev_id for it. The allocation is
                    COALESCE(MAX(...), 0) + 1, as in courses_4103.lua.

                    STRING NAME is cast to the name column width.
                    STRING REPORT, THUMBNAIL, DESIGN, SCRIPTS, PARAMETERS,
                    and SUMMARY are required. Send '' for a blank.
                    Each name is used once. Returns report_id and rev_id.
                    MySQL and MariaDB cast the id as signed and the name
                    as char(255).
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

            Inserts the report revision write. Existing report rows stay.
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

            Deletes the TYPE_SQL row only. Report rows stay.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
return queries end
