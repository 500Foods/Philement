-- Migration: acuranzo_1114.lua
-- QueryRef #023 - Get Session Logs List

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.0.0 - 2025-12-29 - Initial creation

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "queries"
cfg.MIGRATION = "1114"
cfg.QUERY_REF = "023"
cfg.QUERY_NAME = "Get Sessions Log List"
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
                    FROM
                        (
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
                  ORDER BY
                    sessions.created_at DESC
                ]==]                                                                AS code,
                '${QUERY_NAME}'                                                     AS name,
                [==[
                    #  QueryRef #${QUERY_REF} - ${QUERY_NAME}

                    This query retrieves a list of session logs for a specified account.

                    ## Parameters

                    - `ACCOUNTID` (integer): The ID of the account whose session logs are to be retrieved.

                    ## Returns

                    A list of session logs for the specified account. Each session log includes the following fields:
                    - `session_id` (integer): The unique identifier for the session.
                    - `account_id` (integer): The ID of the account associated with the session.
                    - `session_length` (integer): The total length of the session.
                    - `session_issues` (integer): The number of issues encountered during the session.
                    - `session_changes` (integer): The number of changes made during the session.
                    - `session_secs` (integer): The duration of the session in seconds.
                    - `session_mins` (integer): The duration of the session in minutes.
                    - `status_a25` (integer): The status code of the session.
                    - `flag_a26` (integer): The flag code associated with the session.
                    - `created_at` (timestamp): The timestamp when the session was created.
                    - `updated_at` (timestamp): The timestamp when the session was last updated.
                    - `status_icon` (string): The icon representing the session status.

                    ## Tables

                    - `${SCHEMA}sessions`: Contains session log data.
                    - `${SCHEMA}lookups`: Contains lookup data for status icons.

                    ## Notes

                    - Ensure that the `ACCOUNTID` parameter is provided when executing this query.
                    - This is the first occurence of the ${JRS}${JRM}${JRE} macro set in a query.
                    - This is used to extract JSON values from JSON columns in the database.
                    - JSON Retrieval Start, JSON Retrieval Middle, JSON Retrieval End.
                    - It is very sensitive to spacing - no spaces beteween the macro, field, and JSON path.

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
