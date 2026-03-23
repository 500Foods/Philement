-- Migration: acuranzo_1174.lua
-- QueryRef #072: Retrieve Media Asset by Hash

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.0.0 - 2026-03-23 - Initial creation for Phase 12 - Chat Service

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "queries"
cfg.MIGRATION = "1174"
cfg.QUERY_REF = "072"
cfg.QUERY_NAME = "Retrieve Media Asset"
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
        [==[
            SELECT
                media_hash,
                media_data,
                media_size,
                mime_type,
                created_at,
                last_accessed,
                access_count
            FROM ${SCHEMA}media_assets
            WHERE media_hash = :MEDIA_HASH
        ]==]                                                                AS code,
        '${QUERY_NAME}'                                                     AS name,
        [==[
            # QueryRef ${QUERY_REF} - ${QUERY_NAME}

            This INTERNAL query retrieves a media asset by its hash.

            ## Parameters

            - MEDIA_HASH: SHA-256 hash of the media content

            ## Returns

            - media_hash: SHA-256 hash
            - media_data: Binary media content (as hex string)
            - media_size: Size in bytes
            - mime_type: MIME type
            - created_at: Timestamp when created
            - last_accessed: Timestamp of last access
            - access_count: Number of times accessed

            ## Tables

            - `${SCHEMA}media_assets`: Media asset storage

            ## Security Notes

            - query_type = 0 (internal_sql) prevents access via REST API
            - For internal Chat Storage implementation only

        ]==]
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
            ${DROP_CHECK};

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Drop QueryRef ${QUERY_REF} - ${QUERY_NAME}'                       AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Drop QueryRef ${QUERY_REF} - ${QUERY_NAME}

            This reverses the addition of QueryRef ${QUERY_REF} - ${QUERY_NAME}.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
return queries end