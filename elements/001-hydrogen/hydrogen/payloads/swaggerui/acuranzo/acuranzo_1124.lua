-- Migration: acuranzo_1124.lua
-- QueryRef #033 - Get Session Logs List + Search

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.0.0 - 2025-12-31 - Initial creation

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "queries"
cfg.MIGRATION = "1124"
cfg.QUERY_REF = "033"
cfg.QUERY_NAME = "Get Session Logs List + Search"
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
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
                ${QTC_MEDIUM}                                                       AS query_queue_a58,
                ${TIMEOUT}                                                          AS query_timeout,
                [==[
                    SELECT
                        session_id,
                        account_id,
                        session_length,
                        session_issues,
                        session_changes,
                        session_secs,
                        session_mins,
                        status_a25,
                        flag_a26,
                        sessions.created_at,
                        sessions.updated_at,
                        ${JRS}lua25.collection${JRM}'$.icon'${JRE}) status_icon
                    FROM (
                        SELECT
                            session_id,
                            account_id,
                            SUM(session_length) session_length,
                            SUM(session_issues) session_issues,
                            SUM(session_changes) session_changes,
                            MAX(session_secs) session_secs,
                            MAX(status_a25) status_a25,
                            MIN(flag_a26) flag_a26,
                            MIN(created_at) created_at,
                            MAX(updated_at) updated_at,
                            1 + (SUM(session_secs) / 60) session_mins
                        FROM
                            ${SCHEMA}sessions
                        WHERE
                            (account_id = :ACCOUNTID)
                        GROUP BY
                            session_id,
                            account_id
                    ) sessions
                    LEFT OUTER JOIN
                        ${SCHEMA}lookups lua25
                        ON lua25.lookup_id = 25
                        AND lua25.key_idx = sessions.status_a25
                    WHERE
                        session_id IN (
                            SELECT
                                session_id
                            FROM
                                ${SCHEMA}sessions
                            WHERE
                                (account_id = :ACCOUNTID)
                                AND (
                                    (UPPER(session_id) LIKE '%' || :SEARCH || '%')
                                    OR (UPPER(session) LIKE '%' || :SEARCH || '%')
                                )
                        )
                    ORDER BY
                        sessions.created_at DESC
                ]==]                                                                AS code,
                '${QUERY_NAME}'                                                     AS name,
                [==[
                    #  QueryRef #${QUERY_REF} - ${QUERY_NAME}

                    This query returns the sessions for a given account, filtered by a search term.

                    ## Parameters

                    - ACCOUNTID - The account to search for sessions
                    - SEARCH - The search term to filter the results

                    ## Returns

                    - session_id - The session ID
                    - account_id - The account ID
                    - session_length - The length of the session
                    - session_issues - The number of issues in the session
                    - session_changes - The number of changes in the session
                    - session_secs - The number of seconds in the session
                    - session_mins - The number of minutes in the session
                    - status_a25 - The status of the session
                    - flag_a26 - The flag of the session
                    - created_at - The creation date of the session
                    - updated_at - The last update date of the session
                    - status_icon - The icon for the status of the session

                    ## Tables

                    - 1${SCHEMA}sessions`: The sessions table

                    ## Notes

                    - The search term is applied to the session_id and session fields

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
            # Forward Migration ${MIGRATION}: Poulate QueryRef #${QUERY_REF} - ${QUERY_NAME}

            This migration creates the query for QueryRef #${QUERY_REF} - ${QUERY_NAME}
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
            WHERE query_ref = ${QUERY_REF};

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Remove QueryRef #${QUERY_REF} - ${QUERY_NAME}'                     AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Remove QueryRef #${QUERY_REF} - ${QUERY_NAME}

            This is provided for completeness when testing the migration system
            to ensure that forward and reverse migrations are complete.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
return queries end
