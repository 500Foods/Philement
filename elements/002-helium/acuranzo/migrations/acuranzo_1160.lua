-- Migration: acuranzo_1160.lua
-- Extend convos table with segment_refs and usage tracking columns for Phase 6

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.0.0 - 2026-03-22 - Initial creation for Phase 6 - Chat Service

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "convos"
cfg.MIGRATION = "1160"
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
            ALTER TABLE ${SCHEMA}${TABLE}
                ADD COLUMN segment_refs ${JSON},
                ADD COLUMN engine_name ${VARCHAR_50},
                ADD COLUMN model ${VARCHAR_100},
                ADD COLUMN tokens_prompt ${INTEGER},
                ADD COLUMN tokens_completion ${INTEGER},
                ADD COLUMN cost_total ${FLOAT},
                ADD COLUMN user_id ${INTEGER},
                ADD COLUMN duration_ms ${INTEGER},
                ADD COLUMN session_id ${CHAR_20};

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
       ]=]
                                                                            AS code,
        'Extend ${TABLE} with segment_refs and usage tracking'             AS name,
        [=[
            # Forward Migration ${MIGRATION}: Extend ${TABLE} with segment_refs and usage tracking

            This migration adds new columns to the ${TABLE} table for Phase 6 -
            Conversation History with Content-Addressable Storage.

            ## New Columns

            - **segment_refs**: JSON array of segment_hash references to conversation_segments
            - **engine_name**: Name of the AI engine/model used
            - **model**: Specific model identifier
            - **tokens_prompt**: Number of tokens in the prompt
            - **tokens_completion**: Number of tokens in the completion
            - **cost_total**: Total cost incurred for this conversation
            - **user_id**: User who initiated the conversation
            - **duration_ms**: Duration of the conversation in milliseconds
            - **session_id**: Session identifier for tracking

            These columns maintain backwards compatibility with legacy storage while
            enabling the new content-addressable storage pattern.
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
            ALTER TABLE ${SCHEMA}${TABLE}
                DROP COLUMN IF EXISTS segment_refs,
                DROP COLUMN IF EXISTS engine_name,
                DROP COLUMN IF EXISTS model,
                DROP COLUMN IF EXISTS tokens_prompt,
                DROP COLUMN IF EXISTS tokens_completion,
                DROP COLUMN IF EXISTS cost_total,
                DROP COLUMN IF EXISTS user_id,
                DROP COLUMN IF EXISTS duration_ms,
                DROP COLUMN IF EXISTS session_id;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Remove segment_refs and usage tracking columns from ${TABLE}'     AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Remove segment_refs and usage tracking columns

            This is provided for completeness when testing the migration system
            to ensure that forward and reverse migrations are complete.
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
        ${TYPE_DIAGRAM_MIGRATION}                                           AS query_type_a28,
        ${DIALECT}                                                          AS query_dialect_a30,
        ${QTC_SLOW}                                                         AS query_queue_a58,
        ${TIMEOUT}                                                          AS query_timeout,
        'JSON Table Definition in collection'                               AS code,
        'Diagram Tables: ${SCHEMA}${TABLE}'                                 AS name,
        [=[
            # Diagram Migration ${MIGRATION}

            ## Diagram Tables: ${SCHEMA}${TABLE}

            Update existing convos table diagram with new Phase 6 columns for
            content-addressable storage and usage tracking.
        ]=]
                                                                            AS summary,
                                                                            -- DIAGRAM_START
        ${JSON_INGEST_START}
        [=[
            {
                "diagram": [
                    {
                        "object_type": "table",
                        "object_id": "table.${TABLE}",
                        "object_ref": "${MIGRATION}",
                        "table": [
                            {
                                "name": "segment_refs",
                                "datatype": "${JSON}",
                                "nullable": true,
                                "primary_key": false,
                                "unique": false
                            },
                            {
                                "name": "engine_name",
                                "datatype": "${VARCHAR_50}",
                                "nullable": true,
                                "primary_key": false,
                                "unique": false
                            },
                            {
                                "name": "model",
                                "datatype": "${VARCHAR_100}",
                                "nullable": true,
                                "primary_key": false,
                                "unique": false
                            },
                            {
                                "name": "tokens_prompt",
                                "datatype": "${INTEGER}",
                                "nullable": true,
                                "primary_key": false,
                                "unique": false
                            },
                            {
                                "name": "tokens_completion",
                                "datatype": "${INTEGER}",
                                "nullable": true,
                                "primary_key": false,
                                "unique": false
                            },
                            {
                                "name": "cost_total",
                                "datatype": "${FLOAT}",
                                "nullable": true,
                                "primary_key": false,
                                "unique": false
                            },
                            {
                                "name": "user_id",
                                "datatype": "${INTEGER}",
                                "nullable": true,
                                "primary_key": false,
                                "unique": false
                            },
                            {
                                "name": "duration_ms",
                                "datatype": "${INTEGER}",
                                "nullable": true,
                                "primary_key": false,
                                "unique": false
                            },
                            {
                                "name": "session_id",
                                "datatype": "${CHAR_20}",
                                "nullable": true,
                                "primary_key": false,
                                "unique": false
                            }
                        ]
                    }
                ]
            }
        ]=]
        ${JSON_INGEST_END}
                                                                            -- DIAGRAM_END
                                                                            AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
return queries end