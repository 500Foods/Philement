-- Migration: acuranzo_1169.lua
-- QueryRef #069: Get Chats List (with segment count and storage metrics)
-- Replaces: QueryRef #039 (legacy chat list without metrics)
-- Phase 7 - Update Existing Chat Queries

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.0.0 - 2026-03-22 - Initial creation for Phase 7 - Chat Service
--                     Adds segment count and storage metrics to chat list

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "queries"
cfg.MIGRATION = "1169"
cfg.QUERY_REF = "069"
cfg.QUERY_NAME = "Get Chats List (with Metrics)"
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
                        c.convos_id,
                        c.convos_ref,
                        c.convos_icon,
                        c.convos_keywords,
                        c.updated_at,
                        c.engine_name,
                        c.model,
                        c.tokens_prompt,
                        c.tokens_completion,
                        c.cost_total,
                        c.session_id,
                        c.user_id,
                        c.duration_ms,
                        c.created_at,
                        COALESCE(jsonb_array_length(c.segment_refs::jsonb), 0) AS segment_count,
                        (
                            COALESCE(
                                (
                                    SELECT SUM(s.uncompressed_size)
                                    FROM ${SCHEMA}convo_segs s,
                                        jsonb_array_elements_text(c.segment_refs::jsonb) AS hash
                                    WHERE s.segment_hash = hash
                                ), 0
                            )
                        ) AS total_uncompressed_bytes,
                        (
                            COALESCE(
                                (
                                    SELECT SUM(s.compressed_size)
                                    FROM ${SCHEMA}convo_segs s,
                                        jsonb_array_elements_text(c.segment_refs::jsonb) AS hash
                                    WHERE s.segment_hash = hash
                                ), 0
                            )
                        ) AS total_compressed_bytes,
                        (
                            COALESCE(
                                (
                                    SELECT AVG(s.compression_ratio)
                                    FROM ${SCHEMA}convo_segs s,
                                        jsonb_array_elements_text(c.segment_refs::jsonb) AS hash
                                    WHERE s.segment_hash = hash
                                ), 0
                            )
                        ) AS avg_compression_ratio,
                        (${SIZE_INTEGER} * 3)
                        + (${SIZE_TIMESTAMP} * 4)
                        + coalesce(LENGTH(c.convos_ref),0)
                        + coalesce(LENGTH(c.convos_keywords),0)
                        + coalesce(LENGTH(c.convos_icon),0)
                        + coalesce(LENGTH(c.segment_refs),0)
                        + coalesce(LENGTH(c.engine_name),0)
                        + coalesce(LENGTH(c.model),0)
                        record_size
                    FROM
                        ${SCHEMA}convos c
                    WHERE
                        NOT (
                            (c.convos_ref like '%.Docs.%')
                            OR (c.convos_ref like '%.Queries.%')
                        )
                    ORDER BY
                        c.updated_at DESC
                ]==]                                                                AS code,
                '${QUERY_NAME}'                                                     AS name,
                [==[
                    # QueryRef #${QUERY_REF} - ${QUERY_NAME}

                    This query returns a list of chat conversations with segment count
                    and storage metrics. Includes both legacy columns (for backwards
                    compatibility) and new Phase 6/7 columns.

                    ## Parameters

                    - No parameters

                    ## Returns

                    - convos_id, convos_ref: Basic conversation info
                    - convos_icon, convos_keywords, updated_at: Legacy columns
                    - segment_count: Number of segments in segment_refs array
                    - total_uncompressed_bytes: Sum of uncompressed segment sizes
                    - total_compressed_bytes: Sum of compressed segment sizes
                    - avg_compression_ratio: Average compression ratio across segments
                    - engine_name, model: AI engine info from Phase 6
                    - tokens_prompt, tokens_completion, cost_total: Usage metrics
                    - user_id, session_id, duration_ms: Session tracking
                    - record_size: Total record size for listing

                    ## Tables

                    - `${SCHEMA}convos`: Conversation metadata with segment_refs
                    - `${SCHEMA}convo_segs`: Segment content storage (for metrics)

                    ## Notes

                    This replaces QueryRef #039 with enhanced metrics. For backwards
                    compatibility, legacy columns (prompt/response/context/history) are
                    still available in the convos table.

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

            This migration creates the query for QueryRef #${QUERY_REF} - ${QUERY_NAME}
            for Phase 7 - Update Existing Chat Queries.
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
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
return queries end