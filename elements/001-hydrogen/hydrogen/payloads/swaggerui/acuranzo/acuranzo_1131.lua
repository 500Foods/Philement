-- Migration: acuranzo_1131.lua
-- QueryRef #040 - Get Chats Lis + Search

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.0.0 - 2025-12-31 - Initial creation

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "queries"
cfg.MIGRATION = "1131"
cfg.QUERY_REF = "040"
cfg.QUERY_NAME = "Get Chats List + Search"
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
                        convos_id,
                        convos_ref,
                        convos_icon,
                        convos_keywords,
                        updated_at,
                        (${SIZE_INTEGER} * 3)
                        + (${SIZE_TIMESTAMP} * 4)
                        + coalesce(LENGTH(convos_ref),0)
                        + coalesce(LENGTH(convos_keywords),0)
                        + coalesce(LENGTH(convos_icon),0)
                        + coalesce(LENGTH(prompt),0)
                        + coalesce(LENGTH(response),0)
                        + coalesce(LENGTH(context),0)
                        + coalesce(LENGTH(history),0)
                        record_size
                    FROM
                        ${SCHEMA}convos
                        JOIN (
                            SELECT
                                MAX(convos_id) max_convos_id
                            FROM
                                ${SCHEMA}convos
                            WHERE
                                NOT (
                                    (convos_icon IS NULL)
                                    OR (convos_keywords IS NULL)
                                )
                                AND (
                                    (UPPER(convos_keywords) LIKE '%' || :SEARCH || '%')
                                    OR (UPPER(prompt) LIKE '%' || :SEARCH || '%')
                                    OR (UPPER(response) LIKE '%' || :SEARCH || '%')
                                    OR (UPPER(context) LIKE '%' || :SEARCH || '%')
                                )
                            GROUP BY
                                convos_ref,
                                convos_icon,
                                convos_keywords
                        ) convos_variations ON app.convos.convos_id = max_convos_id
                    WHERE
                        NOT (
                            (convos_ref LIKE '%.Docs.%')
                            OR (convos_ref LIKE '%.Queries.%')
                        )
                    ORDER BY
                        updated_at DESC
                ]==]                                                                AS code,
                '${QUERY_NAME}'                                                     AS name,
                [==[
                    #  QueryRef #${QUERY_REF} - ${QUERY_NAME}

                    This query returns a list of conversations that match the search criteria.

                    ## Parameters

                    - :SEARCH (string): The search string to match against the conversation keywords, prompt, response, and context.

                    ## Returns

                    - `convos_id` (integer): The unique identifier for the conversation.
                    - `convos_ref` (string): The reference for the conversation.
                    - `convos_icon` (string): The icon for the conversation.
                    - `convos_keywords` (string): The keywords for the conversation.
                    - `updated_at` (timestamp): The timestamp when the conversation was last updated.
                    - `record_size` (integer): The size of the conversation record.

                    ## Tables

                    - `${SCHEMA}convos`: Stores chat conversations

                    ## Notes

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
