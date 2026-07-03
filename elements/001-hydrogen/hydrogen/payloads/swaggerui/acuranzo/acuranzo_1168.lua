-- Migration: acuranzo_1168.lua
-- QueryRef #068: Get Chat (reconstruct from hash-based storage)
-- Replaces: QueryRef #041 (legacy prompt/response retrieval)
-- Phase 7 - Update Existing Chat Queries

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.0.0 - 2026-03-22 - Initial creation for Phase 7 - Chat Service
--                     Reconstructs conversation from hash references in segment_refs

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "queries"
cfg.MIGRATION = "1168"
cfg.QUERY_REF = "068"
cfg.QUERY_NAME = "Get Chat (Reconstruct)"
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
                        c.segment_refs,
                        c.engine_name,
                        c.model,
                        c.tokens_prompt,
                        c.tokens_completion,
                        c.cost_total,
                        c.user_id,
                        c.duration_ms,
                        c.session_id,
                        c.created_at,
                        c.updated_at,
                        COALESCE(jsonb_agg(
                            jsonb_build_object(
                                'segment_hash', s.segment_hash,
                                'segment_content', s.segment_content,
                                'uncompressed_size', s.uncompressed_size,
                                'compressed_size', s.compressed_size
                            ) ORDER BY s.created_at
                        ), '[]'::jsonb) AS segments
                    FROM
                        ${SCHEMA}convos c
                    LEFT JOIN LATERAL (
                        SELECT
                            seg.segment_hash,
                            seg.segment_content,
                            seg.uncompressed_size,
                            seg.compressed_size,
                            seg.created_at
                        FROM
                            ${SCHEMA}convo_segs seg,
                            jsonb_array_elements_text(c.segment_refs::jsonb) AS hash
                        WHERE
                            seg.segment_hash = hash
                    ) s ON true
                    WHERE
                        c.convos_id = :CONVOSID
                    GROUP BY
                        c.convos_id, c.convos_ref, c.segment_refs, c.engine_name,
                        c.model, c.tokens_prompt, c.tokens_completion, c.cost_total,
                        c.user_id, c.duration_ms, c.session_id, c.created_at, c.updated_at
                ]==]                                                                AS code,
                '${QUERY_NAME}'                                                     AS name,
                [==[
                    # QueryRef #${QUERY_REF} - ${QUERY_NAME}

                    This query retrieves a chat conversation and reconstructs it from
                    hash-based content-addressable storage by fetching segment content
                    from the convo_segs table based on segment_refs.

                    ## Parameters

                    - CONVOSID: The conversation ID to retrieve

                    ## Returns

                    - convos_id: Conversation ID
                    - convos_ref: Conversation reference
                    - segment_refs: JSON array of segment hashes
                    - segments: Array of segment objects with content
                    - engine_name, model: AI engine information
                    - tokens_prompt, tokens_completion, cost_total: Usage metrics
                    - user_id, session_id: User/session identifiers
                    - created_at, updated_at: Timestamps

                    ## Tables

                    - `${SCHEMA}convos`: Conversation metadata table
                    - `${SCHEMA}convo_segs`: Segment content storage

                    ## Notes

                    This replaces the legacy QueryRef #041 which retrieved prompt/response
                    directly from the convos table. The new approach reconstructs the
                    conversation from hash references.

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