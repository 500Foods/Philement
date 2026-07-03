-- Migration: acuranzo_1138.lua
-- QueryRef #047 - Get Documents

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.0.0 - 2025-12-31 - Initial creation

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "queries"
cfg.MIGRATION = "1138"
cfg.QUERY_REF = "047"
cfg.QUERY_NAME = "Get Documents"
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
                    SELECT
                        docs.doc_id,
                        docs.rev_id,
                        docs.name,
                        docs.collection,
                        docs.doc_library_a49,
                        docs.doc_status_a50,
                        docs.doc_type_a51,
                        docs.valid_after,
                        docs.valid_until,
                        docs.created_id,
                        docs.created_at,
                        docs.updated_id,
                        docs.updated_at,
                        (${SIZE_INTEGER} * 7)
                        + (${SIZE_TIMESTAMP} * 4)
                        + COALESCE(LENGTH(docs.name), 0)
                        + COALESCE(LENGTH(docs.summary), 0)
                        + COALESCE(LENGTH(docs.file_name), 0)
                        + COALESCE(LENGTH(docs.file_data), 0)
                        + COALESCE(LENGTH(docs.file_text), 0)
                        + COALESCE(LENGTH(docs.file_preview), 0)
                        + ${SIZE_COLLECTION}
                        record_size
                    FROM
                        ${SCHEMA}documents docs
                    LEFT OUTER JOIN
                        ${SCHEMA}lookups library
                        ON (docs.doc_library_a49 = library.key_idx)
                        AND (library.lookup_id = 49)
                  --    LEFT OUTER JOIN
                  --        ${SCHEMA}lookups status
                  --        ON (docs.doc_status_a50 = status.key_idx)
                  --        AND (status.lookup_id = 50)
                  --    WHERE
                  --        (library.status_a1 = 1)
                  --        AND (docs.doc_status_a50 = 1)
                    ORDER BY
                        library.sort_seq,
                        library.value_txt,
                        docs.name
                ]==]                                                                AS code,
                '${QUERY_NAME}'                                                     AS name,
                [==[
                    #  QueryRef #${QUERY_REF} - ${QUERY_NAME}

                    This query returns the list of documents, currently all documents.

                    ## Parameters

                    - No parameters at this stage

                    ## Returns

                    - `doc_id`: The document ID
                    - `rev_id`: The revision ID
                    - `name`: The document name
                    - `collection`: The document collection
                    - `doc_library_a49`: The document library
                    - `doc_status_a50`: The document status
                    - `doc_type_a51`: The document type
                    - `valid_after`: The document valid after date
                    - `valid_until`: The document valid until date
                    - `created_id`: The document created by
                    - `created_at`: The document created at
                    - `updated_id`: The document updated by
                    - `updated_at`: The document updated at
                    - `record_size`: The document record size

                    ## Tables

                    - `documents`: The documents table
                    - `lookups`: The lookups table

                    ## Notes

                    - This will need to be filtered at some point, but for now, it's all documents.

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
