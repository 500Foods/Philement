-- Migration: acuranzo_1123.lua
-- QueryRef #032 - Get Queries List + Search

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.0.0 - 2025-12-30 - Initial creation

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "queries"
cfg.MIGRATION = "1123"
cfg.QUERY_REF = "032"
cfg.QUERY_NAME = "Get Queries List + Search"
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
                        query_id,
                        query_ref,
                        query_type_a28,
                        query_dialect_a30,
                        query_status_a27,
                        name,
                        valid_after,
                        valid_until,
                        created_id,
                        created_at,
                        updated_id,
                        updated_at,
                        (${SIZE_INTEGER} * 7)
                        + (${SIZE_TIMESTAMP} * 4)
                        + COALESCE(LENGTH(name), 0)
                        + COALESCE(LENGTH(summary), 0)
                        + COALESCE(LENGTH(query_code), 0)
                        + ${SIZE_COLLECTION}
                        record_size
                    FROM
                        ${SCHEMA}queries
                    WHERE
                        (UPPER(name) LIKE '%' || :SEARCH || '%')
                        OR (UPPER(query_code) LIKE '%' || :SEARCH || '%')
                        OR (UPPER(summary) LIKE '%' || :SEARCH || '%')
                        OR (UPPER(collection) LIKE '%' || :SEARCH || '%')
                    ORDER BY
                        query_type_a28,
                        query_ref,
                        name
                ]==]                                                                AS code,
                '${QUERY_NAME}'                                                     AS name,
                [==[
                    #  QueryRef #${QUERY_REF} - ${QUERY_NAME}

                    This query retrieves the list of system queries along with their
                    associated metadata, filtered by a search term.

                    ## Parameters

                    - `SEARCH` (string, required): The search term to filter queries by name,
                      code, summary, or collection data.

                    ## Returns

                    - `query_id` (integer): The unique identifier of the system query.
                    - `query_ref` (string): The reference identifier of the system query.
                    - `query_type_a28` (integer): The type of the system query.
                    - `query_dialect_a30` (integer): The SQL dialect of the system query
                    - `query_status_a27` (integer): The status of the system query.
                    - `name` (string): The name of the system query.
                    - `valid_after` (timestamp): The timestamp after which the query is valid.
                    - `valid_until` (timestamp): The timestamp until which the query is valid.
                    - `created_id` (integer): The ID of the user who created the query.
                    - `created_at` (timestamp): The timestamp when the query was created.
                    - `updated_id` (integer): The ID of the user who last updated the query
                    - `updated_at` (timestamp): The timestamp when the query was last updated.
                    - `record_size` (integer): The calculated size of the query record.

                    ## Tables

                    - `${SCHEMA}queries`: Contains system query definitions.

                    ## Notes

                    - Ensure that the provided `SEARCH` term is in uppercase to match the
                      filtering criteria.
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
