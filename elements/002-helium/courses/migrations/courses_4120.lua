-- Migration: courses_4120.lua
-- QueryRef #4523 - Staff: List Runs by State
--
-- Caller ref 4523 is not a file number. File numbers stay below 4500.
-- STRING STATE. An empty STATE returns every run. Otherwise exact
-- match on course_build_runs.state (P20a names, lower case).
-- Uses course_build_runs_idx_state from 4118. Single statement.
--
-- luacheck: no max line length
-- luacheck: no unused args
--
-- CHANGELOG
-- 1.0.0 - 2026-10-26 - Staff: List Runs by State

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "queries"
cfg.MIGRATION = "4120"
cfg.QUERY_REF = "4523"
cfg.QUERY_NAME = "Staff: List Runs by State"
if engine == "mysql" or engine == "mariadb" then
    cfg.CAST_INTEGER = "signed"
    cfg.CAST_STATE = "char(50)"
else
    cfg.CAST_INTEGER = cfg.INTEGER
    cfg.CAST_STATE = cfg.VARCHAR_50
end
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
                ${QTC_FAST}                                                         AS query_queue_a58,
                ${TIMEOUT}                                                          AS query_timeout,
                [==[
                    SELECT
                        r.run_id,
                        r.suggestion_id,
                        r.working_title,
                        r.state,
                        r.template_key,
                        r.price_cents,
                        r.canvas_course_id,
                        r.error_text,
                        COALESCE(r.flagged, 0) AS flagged,
                        r.receipt_mail,
                        r.annotation,
                        r.created_at,
                        r.updated_at,
                        s.course_name,
                        s.submitter_name,
                        s.submitter_email,
                        (SELECT COUNT(*) FROM ${SCHEMA}course_build_artifacts a
                          WHERE a.run_id = r.run_id) AS artifact_count
                    FROM ${SCHEMA}course_build_runs r
                    LEFT JOIN ${SCHEMA}course_suggestions s
                      ON s.suggestion_id = r.suggestion_id
                    WHERE CAST(:STATE AS ${CAST_STATE}) = ''
                       OR r.state = CAST(:STATE AS ${CAST_STATE})
                    ORDER BY r.updated_at DESC,
                             r.run_id DESC
                ]==]                                                                AS code,
                '${QUERY_NAME}'                                                     AS name,
                [==[
                    # QueryRef #${QUERY_REF} - ${QUERY_NAME}

                    Staff read. STRING STATE; empty returns all runs. One row per run with suggestion name, submitter, and artifact count.
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

            Staff read. STRING STATE; empty returns all runs. One row per run with suggestion name, submitter, and artifact count.
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
