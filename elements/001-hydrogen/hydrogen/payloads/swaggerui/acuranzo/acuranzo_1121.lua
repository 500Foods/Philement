-- Migration: acuranzo_1121.lua
-- QueryRef #030 - Get Lookups List

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.0.0 - 2025-12-30 - Initial creation

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "queries"
cfg.MIGRATION = "1121"
cfg.QUERY_REF = "030"
cfg.QUERY_NAME = "Get Lookups List"
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
                        AL.lookup_id,
                        AL.key_idx,
                        AL.value_txt,
                        AL.value_int,
                        AL.sort_seq,
                        AL.status_a1,
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
                        record_size,
                        AC.lookup_count,
                        AC.lookup_size
                    FROM
                        ${SCHEMA}lookups AL -- Lookup Entry
                    LEFT OUTER JOIN (
                        SELECT
                            lookup_id,
                            COUNT(*) lookup_count,
                            SUM(
                                (${SIZE_INTEGER} * 7)
                                + (${SIZE_TIMESTAMP} * 4)
                                + COALESCE(LENGTH(value_txt), 0)
                                + COALESCE(LENGTH(summary), 0)
                                + COALESCE(LENGTH(code), 0)
                                + ${SIZE_COLLECTION}
                            ) lookup_size
                        FROM
                            ${SCHEMA}lookups
                        GROUP BY
                            lookup_id
                    ) AC -- Lookup Collection
                    ON AL.key_idx = AC.lookup_id
                    WHERE
                        (AL.lookup_id = 0)
                    ORDER BY
                        AL.sort_seq,
                        AL.key_idx
                ]==]                                                                AS code,
                '${QUERY_NAME}'                                                     AS name,
                [==[
                    #  QueryRef #${QUERY_REF} - ${QUERY_NAME}

                    This query retrieves the list of lookup entries along with their
                    associated collection data for a specified lookup ID.

                    ## Parameters

                    - `LOOKUPID` (integer, required): The ID of the lookup to retrieve entries for.

                    ## Returns

                    - `lookup_id` (integer): The ID of the lookup entry.
                    - `key_idx` (integer): The key index of the lookup entry.
                    - `value_txt` (string): The text value of the lookup entry.
                    - `value_int` (integer): The integer value of the lookup entry.
                    - `sort_seq` (integer): The sort sequence of the lookup entry.
                    - `status_a1` (integer): The status of the lookup entry.
                    - `summary` (text): A summary description of the lookup entry.
                    - `code` (string): Code associated with the lookup entry.
                    - `collection` (JSON): Additional collection data for the lookup entry.
                    - `valid_after` (timestamp): The timestamp after which the lookup entry is valid.
                    - `valid_until` (timestamp): The timestamp until which the lookup entry is valid.
                    - `created_id` (integer): The ID of the user who created the lookup entry
                    - `created_at` (timestamp): The timestamp when the lookup entry was created.
                    - `updated_id` (integer): The ID of the user who last updated the lookup
                    - `updated_at` (timestamp): The timestamp when the lookup entry was last updated.
                    - `record_size` (integer): The calculated size of the lookup entry record.
                    - `lookup_count` (integer): The total number of entries for the lookup ID.
                    - `lookup_size` (integer): The total size of all entries for the lookup ID

                    ## Tables

                    - `${SCHEMA}lookups`: Contains lookup entries.

                    ## Notes

                    - Ensure that the provided `LOOKUPID` is valid and exists in the database.

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
