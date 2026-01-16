-- Migration: acuranzo_1133.lua
-- QueryRef #042 - Create Lookup Key

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.0.0 - 2025-12-31 - Initial creation

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "queries"
cfg.MIGRATION = "1133"
cfg.QUERY_REF = "042"
cfg.QUERY_NAME = "Create Lookup Key"
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
                    ${INSERT_KEYSTART} key_id ${INSERT_KEYEND}
                        INSERT INTO ${SCHEMA}lookups (
                            lookup_id,
                            key_idx,
                            value_txt,
                            value_int,
                            sort_seq,
                            status_a1,
                            summary,
                            code,
                            collection,
                            created_at,
                            created_id,
                            updated_at,
                            updated_id
                        )
                        WITH next_key_id AS (
                            SELECT COALESCE(MAX(key_id), 0) + 1 AS new_key_id
                            FROM ${SCHEMA}lookups
                            WHERE lookup_id = :LOOKUPID
                        )
                        SELECT
                            :LOOKUPID,
                            new_key_id,
                            :VALUETXT,
                            :VALUEINT,
                            :SORTSEQ,
                            :STATUSLUA1,
                            :SUMMARY,
                            :CODE,
                            :COLLECTION,
                            ${NOW},
                            :USERID,
                            ${NOW},
                            :USERID
                        FROM
                            next_key_idx
                    ${INSERT_KEY_RETURN} key_idx
                    ;
                ]==]                                                                AS code,
                '${QUERY_NAME}'                                                     AS name,
                [==[
                    #  QueryRef #${QUERY_REF} - ${QUERY_NAME}

                    This query creates a new key for a give lookup in the lookups table.

                    ## Parameters

                    - :LOOKUPID (integer): The unique identifier for the lookup.
                    - :VALUETXT (string): The value for the key.
                    - :VALUEINT (integer): The value for the key.
                    - :SORTSEQ (integer): The sort sequence for the key.
                    - :STATUSLUA1 (integer): The status for the key.
                    - :SUMMARY (string): The summary for the key.
                    - :CODE (string): The code for the key.
                    - :COLLECTION (jsonb): The collection for the key.
                    - :USERID (integer): The user ID for the key.

                    ## Returns

                    - `key_idx` (integer): The key index for the new key.
                    - Affected row count, expected to be 1.

                    ## Tables

                    - `${SCHEMA}lookups`: Stores lookup keys

                    ## Notes
                    - This query uses the `next_key_idx` CTE to get the next key index for the lookup.
                    - Sort of like a manual AUTOINCREMENT field.
                    - A little more complex due to wanting to return the new key index from the query.

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
