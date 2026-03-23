-- Migration: acuranzo_1167.lua
-- QueryRef #067: Store Chat (with hash-based content-addressable storage)
-- Replaces: QueryRef #036 (legacy prompt/response storage)
-- Phase 7 - Update Existing Chat Queries

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.0.0 - 2026-03-22 - Initial creation for Phase 7 - Chat Service
--                     Replaces legacy #036 with content-addressable storage

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "queries"
cfg.MIGRATION = "1167"
cfg.QUERY_REF = "067"
cfg.QUERY_NAME = "Store Chat (Hash-based)"
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
                    ${INSERT_KEY_START} convos_id ${INSERT_KEY_END}
                        INSERT INTO ${SCHEMA}convos (
                            convos_id,
                            convos_ref,
                            segment_refs,
                            engine_name,
                            model,
                            tokens_prompt,
                            tokens_completion,
                            cost_total,
                            user_id,
                            duration_ms,
                            session_id,
                            created_id,
                            created_at,
                            updated_id,
                            updated_at
                        )
                        WITH next_convos_id AS (
                            SELECT COALESCE(MAX(convos_id), 0) + 1 AS new_convos_id
                            FROM ${SCHEMA}convos
                        )
                        SELECT
                            new_convos_id
                            :CONVOREF,
                            :SEGMENT_REFS,
                            :ENGINE_NAME,
                            :MODEL,
                            :TOKENS_PROMPT,
                            :TOKENS_COMPLETION,
                            :COST_TOTAL,
                            :USER_ID,
                            :DURATION_MS,
                            :SESSION_ID,
                            :ACCOUNTID,
                            ${NOW},
                            :ACCOUNTID,
                            ${NOW}
                        FROM
                            next_convos_id
                    ${INSERT_KEY_RETURN} convos_id
                    ;
                ]==]                                                                AS code,
                '${QUERY_NAME}'                                                     AS name,
                [==[
                    # QueryRef #${QUERY_REF} - ${QUERY_NAME}

                    This query stores a chat conversation using content-addressable storage.
                    Segment content is stored separately in convo_segs table with hash references
                    stored in convos.segment_refs.

                    ## Parameters

                    - CONVOREF: The conversation reference
                    - SEGMENT_REFS: JSON array of segment_hash values (from convo_segs)
                    - ENGINE_NAME: Name of the AI engine used
                    - MODEL: Model identifier
                    - TOKENS_PROMPT: Token count for prompt
                    - TOKENS_COMPLETION: Token count for completion
                    - COST_TOTAL: Total cost in dollars
                    - USER_ID: User who initiated the conversation
                    - DURATION_MS: Duration in milliseconds
                    - SESSION_ID: Session identifier
                    - ACCOUNTID: Account creating the conversation

                    ## Returns

                    - convos_id of the newly inserted conversation

                    ## Tables

                    - `${SCHEMA}convos`: Stores conversation metadata with segment references
                    - `${SCHEMA}convo_segs`: Stores actual segment content (separate)

                    ## Notes

                    This replaces the legacy QueryRef #036 which stored full prompt/response
                    in the convos table. The new approach stores only hash references.

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