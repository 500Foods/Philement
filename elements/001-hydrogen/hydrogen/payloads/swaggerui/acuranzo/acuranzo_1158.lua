-- Migration: acuranzo_1158.lua
-- QueryRef #061 - Internal: Get AI Chat Engines with API Keys

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.0.1 - 2026-03-20 - Minor fixes - set lookup specifically as 38, other syntax adjustments
-- 1.0.0 - 2026-03-20 - Initial creation for Chat Service Phase 1

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "queries"
cfg.MIGRATION = "1158"
cfg.QUERY_REF = "061"
cfg.QUERY_NAME = "Get AI Chat Engines with API Keys"
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
                ${TYPE_INTERNAL_SQL}                                                AS query_type_a28,
                ${DIALECT}                                                          AS query_dialect_a30,
                ${QTC_FAST}                                                         AS query_queue_a58,
                ${TIMEOUT}                                                          AS query_timeout,
                [==[
                    SELECT
                        key_idx,
                        value_txt,
                        value_int,
                        collection,
                        valid_after,
                        valid_until,
                        created_id,
                        created_at,
                        updated_id,
                        updated_at
                    FROM
                        ${SCHEMA}lookups
                    WHERE
                        (lookup_id = 38)
                        AND (status_a1 = 1)
                        AND (
                            (valid_after IS NULL)
                            OR (valid_after <= ${NOW})
                        )
                        AND (
                            (valid_until IS NULL)
                            OR (valid_until >= ${NOW})
                        )
                    ORDER BY
                        sort_seq,
                        key_idx
                ]==]                                                                AS code,
                '${QUERY_NAME}'                                                     AS name,
                [==[
                    # QueryRef #${QUERY_REF} - ${QUERY_NAME}

                    This INTERNAL query retrieves AI chat engines from the lookups table,
                    returning the FULL collection data including API keys. This query is
                    intended for use ONLY by the Hydrogen server's internal Chat Proxy
                    service and should NEVER be exposed via the REST API.

                    ## Parameters

                    - None: The Lookup in this case is fixed at 38.

                    ## Returns

                    - key_idx: The unique identifier for the lookup entry.
                    - value_txt: The text value associated with the lookup entry.
                    - value_int: The integer value associated with the lookup entry.
                    - collection: The COMPLETE JSON collection including API keys and all metadata.
                    - valid_after: The timestamp after which the lookup entry is valid.
                    - valid_until: The timestamp until which the lookup entry is valid.
                    - created_id: The ID of the user who created the lookup entry.
                    - created_at: The timestamp when the lookup entry was created.
                    - updated_id: The ID of the user who last updated the lookup entry.
                    - updated_at: The timestamp when the lookup entry was last updated.

                    ## Tables

                    - `${SCHEMA}lookups`: Stores lookup keys and engine configurations

                    ## Security Notes

                    - query_type = 0 (internal_sql) prevents access via REST API
                    - fixed lookup = 38
                    - Returns raw collection JSON with API keys visible
                    - For internal Chat Engine Cache (CEC) bootstrap only
                    - QueryRef #044 provides the public filtered version

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
            # Forward Migration ${MIGRATION}: Populate QueryRef #${QUERY_REF} - ${QUERY_NAME}

            This migration creates the internal query for QueryRef #${QUERY_REF} - ${QUERY_NAME}
            for use by the Chat Proxy service.
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
