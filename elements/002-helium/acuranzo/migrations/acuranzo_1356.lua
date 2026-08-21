-- Migration: acuranzo_1356.lua
-- PRIORITIZE 2.18: QueryRef #150 revoked / unenrolled lines
--
-- Do not restamp 1326. archived stays the student Canvas hide.
-- revoked = entitlement ended (refund / deactivate).
-- unenrolled = system Canvas heal (expired leftover seat).
--
-- luacheck: no max line length
-- luacheck: no unused args
--
-- CHANGELOG
-- 1.0.0 - 2026-08-21 - Enrolment History revoked/unenrolled (PRIORITIZE 2.18)

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "queries"
cfg.MIGRATION = "1356"
cfg.QUERY_REF = "150"
-- ----------------------------------------------------------------------------
-- Forward: replace #150 CASE
-- ----------------------------------------------------------------------------
table.insert(queries,{sql=[====[

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
            UPDATE ${SCHEMA}${QUERIES}
               SET code = [==[
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
               ]==]
             WHERE query_ref = ${QUERY_REF};

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'QueryRef #150 revoked/unenrolled copy'                             AS name,
        [=[
            # Forward Migration ${MIGRATION}: Enrolment History 2.18

            Distinguishes student `archived` / `unarchived` from
            operator `revoked` and system `unenrolled`.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]====]})
-- ----------------------------------------------------------------------------
-- Reverse: restore 1326 CASE
-- ----------------------------------------------------------------------------
table.insert(queries,{sql=[====[

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
        ${QTC_FAST}                                                         AS query_queue_a58,
        ${TIMEOUT}                                                          AS query_timeout,
        [=[
            UPDATE ${SCHEMA}${QUERIES}
               SET code = [==[
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
                                WHEN 'archived' THEN 'archived'
                                WHEN 'unarchived' THEN 'unarchived'
                                WHEN 'synced' THEN 'is added to course (sync)'
                                ELSE 'is added to course'
                            END
                            || ' on '
                            || TO_CHAR(created_at, 'YYYY-Mon-DD HH24:MI:SS')
                        ) AS line
                    FROM ${SCHEMA}enrollment_events
                    WHERE course_id = :COURSEID
                    ORDER BY created_at DESC, event_id DESC
               ]==]
             WHERE query_ref = ${QUERY_REF};

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Restore QueryRef #150 prior CASE'                                  AS name,
        [=[
            # Reverse Migration ${MIGRATION}: restore 1326 Enrolment History
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]====]})

return queries end
