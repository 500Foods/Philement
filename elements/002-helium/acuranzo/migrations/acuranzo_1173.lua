-- Migration: acuranzo_1173.lua
-- QueryRef #071: Store Media Asset (with deduplication)

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.0.0 - 2026-03-23 - Initial creation for Phase 12 - Chat Service

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "queries"
cfg.MIGRATION = "1173"
cfg.QUERY_REF = "071"
cfg.QUERY_NAME = "Store Media Asset"
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
            INSERT INTO ${SCHEMA}media_assets (
                media_hash,
                media_data,
                media_size,
                mime_type,
                created_at,
                last_accessed,
                access_count
            )
            SELECT
                :MEDIAHASH,
                :MEDIADATA,
                :MEDIASIZE,
                :MIMETYPE,
                ${NOW},
                ${NOW},
                1
            WHERE NOT EXISTS (
                SELECT 1 FROM ${SCHEMA}media_assets
                WHERE media_hash = :MEDIAHASH
            )
        ]==]                                                                AS code,
        '${QUERY_NAME}'                                                     AS name,
        [==[
            # QueryRef ${QUERY_REF} - ${QUERY_NAME}

            This INTERNAL query stores a media asset with deduplication.
            Only inserts if the media_hash doesn't already exist.

            ## Parameters

            - MEDIA_HASH: SHA-256 hash of the media content
            - MEDIA_DATA: Binary media content (as hex string)
            - MEDIA_SIZE: Size of the media in bytes
            - MIME_TYPE: MIME type of the media (e.g., image/jpeg)

            ## Returns

            - No data returned (INSERT with WHERE NOT EXISTS)

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