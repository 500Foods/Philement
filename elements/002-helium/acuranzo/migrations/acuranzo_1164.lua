-- Migration: acuranzo_1164.lua
-- QueryRef #064: Reconstruct Conversation (get hashes + metadata)

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.0.0 - 2026-03-22 - Initial creation for Phase 6 - Chat Service

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "queries"
cfg.MIGRATION = "1164"
cfg.QUERY_REF = "064"
cfg.QUERY_NAME = "Reconstruct Conversation"
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
        ${QTC_MEDIUM}                                                       AS query_queue_a58,
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
                ${QTC_MEDIUM}                                                       AS query_queue_a58,
                ${TIMEOUT}                                                          AS query_timeout,
                [==[
                    SELECT
                        c.convos_id,
                        c.convos_ref,
                        c.engine_name,
                        c.model,
                        c.tokens_prompt,
                        c.tokens_completion,
                        c.cost_total,
                        c.duration_ms,
                        c.segment_refs,
                        c.created_at,
                        c.updated_at,
                        cs.segment_hash,
                        cs.access_count,
                        cs.last_accessed
                    FROM
                        ${SCHEMA}convos c
                    LEFT OUTER JOIN
                        ${SCHEMA}convo_segs cs
                        ON cs.segment_hash = ANY(
                            COALESCE(
                                (
                                    SELECT array元素
                                    FROM jsonb_array_elements(c.segment_refs) AS array
                                    WHERE jsonb_typeof(c.segment_refs) = 'string'
                                ),
                                ARRAY[]::text[]
                            )
                        )
                    WHERE
                        c.convos_id = :CONVOSID
                ]==]                                                                AS code,
                '${QUERY_NAME}'                                                     AS name,
                [==[
                    # QueryRef ${QUERY_REF} - ${QUERY_NAME}

                    This INTERNAL query reconstructs a conversation by retrieving
                    segment hashes and metadata from the convos table.

                    ## Parameters

                    - CONVOSID: The conversation ID to reconstruct

                    ## Returns

                    - convos_id: Conversation ID
                    - convos_ref: Conversation reference
                    - engine_name: Engine used
                    - model: Model used
                    - tokens_prompt: Prompt tokens used
                    - tokens_completion: Completion tokens used
                    - cost_total: Total cost
                    - duration_ms: Duration in milliseconds
                    - segment_refs: JSON array of segment hashes
                    - created_at: Creation timestamp
                    - updated_at: Last update timestamp
                    - segment_hash: Individual segment hashes
                    - access_count: Segment access count
                    - last_accessed: Last access timestamp

                    ## Tables

                    - `${SCHEMA}convos`: Conversations table
                    - `${SCHEMA}convo_segs`: Segment storage

                    ## Security Notes

                    - query_type = 0 (internal_sql) prevents access via REST API
                    - For internal Chat Storage implementation only

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
        'Populate QueryRef ${QUERY_REF} - ${QUERY_NAME}'                   AS name,
        [=[
            # Forward Migration ${MIGRATION}: Populate QueryRef ${QUERY_REF} - ${QUERY_NAME}

            This migration creates the query for QueryRef ${QUERY_REF} - ${QUERY_NAME}
            for Phase 6 conversation storage.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
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
        'Remove QueryRef ${QUERY_REF}'                                      AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Remove QueryRef ${QUERY_REF}

            This is provided for completeness when testing the migration system.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
return queries end