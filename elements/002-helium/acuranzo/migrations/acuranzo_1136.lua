-- Migration: acuranzo_1136.lua
-- QueryRef #045 - Get Version History

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.0.0 - 2025-12-31 - Initial creation

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "queries"
cfg.MIGRATION = "1136"
cfg.QUERY_REF = "045"
cfg.QUERY_NAME = "Get Version History"
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
                        AL.lookup_id,
                        AL.key_idx,
                        AL.value_txt,
                        AL.value_int,
                        AL.sort_seq,
                        AL.status_lua_1,
                        AL.summary,
                        AL.code,
                        AL.collection,
                        AL.valid_after,
                        AL.valid_until,
                        AL.created_id,
                        AL.created_at,
                        AL.updated_id,
                        AL.updated_at,
                        (${SIZE_INTEGER} * 7)
                        + (${SIZE_TIMESTAMP} * 4)
                        + COALESCE(LENGTH(AL.value_txt), 0)
                        + COALESCE(LENGTH(AL.summary), 0)
                        + COALESCE(LENGTH(AL.code), 0)
                        + ${SIZE_COLLECTION}
                        record_size
                    FROM
                        ${SCHEMA}lookups AL
                    WHERE
                        (AL.lookup_id = :CLIENTSERVER)
                        and (AL.key_idx = :KEYIDX)
                    ORDER BY
                        AL.sort_seq,
                        AL.key_idx,
                        AL.value_txt,
                        AL.value_int
                ]==]                                                                AS code,
                '${QUERY_NAME}'                                                     AS name,
                [==[
                    #  QueryRef #${QUERY_REF} - ${QUERY_NAME}

                    This query retrives version history for client or server applications.

                    ## Parameters

                    - `:CLIENTSERVER` (integer): The choice between client or server history.
                    - `:KEYIDX` (integer): The specific history item to retrieve.

                    ## Returns

                    - lookup_id: The lookup ID for the history item.
                    - key_idx: The unique identifier for the history item.
                    - value_txt: The text value associated with the history item.
                    - value_int: The integer value associated with the history item.
                    - sort_seq: The sort sequence for the history item.
                    - status_lua_1: The status of the history item.
                    - summary: A summary of the history item.
                    - code: The code associated with the history item.
                    - collection: A JSON object containing additional details about the history item.
                    - valid_after: The timestamp after which the history item is valid.
                    - valid_until: The timestamp until which the history item is valid.
                    - created_id: The ID of the user who created the history item.
                    - created_at: The timestamp when the history item was created.
                    - updated_id: The ID of the user who last updated the history item.
                    - updated_at: The timestamp when the history item was last updated.
                    - record_size: The size of the history item record.

                    ## Tables

                    - `${SCHEMA}lookups`: Stores lookup keys

                    ## Notes

                    - CLIENTSERVER = 45 for client history, 44 for server history.
                    - Will likely expand to other numbers if other apps are added

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
