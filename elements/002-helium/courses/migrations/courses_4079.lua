-- Migration: courses_4079.lua
-- QueryRef #4503 - Staff: Enrolment History
--
-- TYPE_SQL twin of QueryRef 4006 for auth_query. Lua still loads
-- 4006 with SELECT code ORDER BY query_id DESC (Enroll.LogEvent).
-- 4006 is also the bookkeeping key of courses_4006.lua, so the
-- type 1001 reverse row is cached first and auth_query never
-- reaches the history SELECT. 4503 is not a file number.
--
-- The line text matches courses_4068.lua. TO_CHAR and || are
-- PostgreSQL. The 500 Courses database is Yugabyte.
--
-- luacheck: no max line length
-- luacheck: no unused args
--
-- CHANGELOG
-- 1.0.0 - 2026-10-22 - Staff enrolment history lines (CATCHUP Phase 13)

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "queries"
cfg.MIGRATION = "4079"
cfg.QUERY_REF = "4503"
cfg.QUERY_NAME = "Staff: Enrolment History"
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
        ${QTC_FAST}                                                         AS query_queue_a58,
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
                        (
                            'User #'
                            || CAST(COALESCE(canvas_user_id, account_id) AS VARCHAR)
                            || ' '
                            || CASE event_type
                                WHEN 'purchased' THEN
                                    'purchased access for '
                                    || UPPER(COALESCE(currency, ''))
                                    || ' '
                                    || TRIM(TO_CHAR(COALESCE(amount_cents, 0) / 100.0, 'FM999999990.00'))
                                WHEN 'renewed' THEN 'renewed'
                                WHEN 'archived' THEN 'archived (Canvas seat removed)'
                                WHEN 'unarchived' THEN 'unarchived (Canvas seat restored)'
                                WHEN 'revoked' THEN 'access revoked'
                                WHEN 'unenrolled' THEN 'unenrolled (entitlement ended)'
                                WHEN 'synced' THEN 'is added to course (sync)'
                                ELSE 'is added to course'
                            END
                            || ' on '
                            || TO_CHAR(created_at, 'YYYY-Mon-DD HH24:MI:SS')
                        ) AS line
                    FROM ${SCHEMA}enrollment_events
                    WHERE course_id = :COURSEID
                    ORDER BY created_at DESC, event_id DESC
                ]==]                                                                AS code,
                '${QUERY_NAME}'                                                     AS name,
                [==[
                    # QueryRef #${QUERY_REF} - ${QUERY_NAME}

                    CATCHUP Phase 13. Same line text as QueryRef 4006.
                    Call this row with a Hydrogen JWT. 4006 stays the
                    copy Lua loads by query_id.

                    Parameter COURSEID is courses.course_id. One `line`
                    per enrolment event, newest first. TO_CHAR is
                    PostgreSQL.
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

            Deletes the TYPE_SQL row only. enrollment_events stay.
            QueryRef 4006 stays.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
return queries end
