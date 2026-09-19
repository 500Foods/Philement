-- Migration: acuranzo_1199.lua
-- Creates the contact_submissions table (Cap Phase 2b: Contact Us form)

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.0.0 - 2026-06-24 - Initial creation for Cap Phase 2b

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "contact_submissions"
cfg.MIGRATION = "1199"
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
            CREATE TABLE ${SCHEMA}${TABLE}
            (
                submission_id       ${INTEGER}          NOT NULL,
                name                ${VARCHAR_50}       NOT NULL,
                email               ${VARCHAR_50}       NOT NULL,
                subject             ${VARCHAR_100}      NOT NULL,
                message             ${TEXT_BIG}         NOT NULL,
                chacha_token        ${VARCHAR_500}      NOT NULL,
                chacha_site         ${VARCHAR_20}       NOT NULL,
                ip_address          ${VARCHAR_50}               ,
                review_status       ${INTEGER_SMALL}    NOT NULL DEFAULT 0,
                ${COMMON_CREATE}
                ${PRIMARY}(submission_id)
            );

            ${SUBQUERY_DELIMITER}

            CREATE INDEX ${TABLE}_idx_created_at
                ON ${SCHEMA}${TABLE}(created_at);

            ${SUBQUERY_DELIMITER}

            CREATE INDEX ${TABLE}_idx_review
                ON ${SCHEMA}${TABLE}(review_status);

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Create ${TABLE} Table'                                             AS name,
        [=[
            # Forward Migration ${MIGRATION}: Create ${TABLE} Table

            This migration creates the ${TABLE} table for the Cap-protected
            "Contact Us" public form. One row per submitted message,
            populated only via the Cap-protected `/api/conduit/cap_query`
            endpoint (QueryRef #086, registered in migration 1200).

            ## Schema

            - **submission_id**: Surrogate primary key.
            - **name**: Required submitter name (VARCHAR_50).
            - **email**: Required submitter e-mail (VARCHAR_50).
            - **subject**: Required short subject line (VARCHAR_100).
            - **message**: Required HTML body (SunEditor output) (TEXT_BIG).
            - **chacha_token**: Cap siteverify token at submission
              (VARCHAR_500).
            - **chacha_site**: Cap public site id at submission (VARCHAR_20).
            - **ip_address**: Server-side client IP captured at submission
              (VARCHAR_50).
            - **review_status**: 0=pending, 1=approved, 2=rejected (defaults
              to 0). Updated out-of-band during manual review.

            ## Indexes

            - PRIMARY KEY on `submission_id`.
            - INDEX on `created_at` - supports listing by recency.
            - INDEX on `review_status` - supports the review queue.

            ## Notes

            This migration is additive. No code reads from this table yet;
            QueryRef #086 inserts and a future review UI will read.

            VARCHAR_50 is used for ip_address because no VARCHAR_45 macro
            currently exists in the engine configs. The extra capacity is
            harmless and still sufficient for IPv6 with an IPv4-mapped
            prefix.
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
            -- DROP INDEX ${TABLE}_idx_created_at;
            -- DROP INDEX ${TABLE}_idx_review;

            ${DROP_CHECK};

            ${SUBQUERY_DELIMITER}

            DROP TABLE ${SCHEMA}${TABLE};

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Drop ${TABLE} Table'                                               AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Drop ${TABLE} Table

            This is provided for completeness when testing the migration
            system to ensure that forward and reverse migrations are complete.
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
        ${TYPE_DIAGRAM_MIGRATION}                                           AS query_type_a28,
        ${DIALECT}                                                          AS query_dialect_a30,
        ${QTC_SLOW}                                                         AS query_queue_a58,
        ${TIMEOUT}                                                          AS query_timeout,
        'JSON Table Definition in collection'                               AS code,
        'Diagram Tables: ${SCHEMA}${TABLE}'                                 AS name,
        [=[
            # Diagram Migration ${MIGRATION}

            ## Diagram Tables: ${SCHEMA}${TABLE}

            This is the JSON Diagram code for the ${TABLE} table.
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
                                "name": "submission_id",
                                "datatype": "${INTEGER}",
                                "nullable": false,
                                "primary_key": true,
                                "unique": true
                            },
                            {
                                "name": "name",
                                "datatype": "${VARCHAR_50}",
                                "nullable": false,
                                "primary_key": false,
                                "unique": false
                            },
                            {
                                "name": "email",
                                "datatype": "${VARCHAR_50}",
                                "nullable": false,
                                "primary_key": false,
                                "unique": false
                            },
                            {
                                "name": "subject",
                                "datatype": "${VARCHAR_100}",
                                "nullable": false,
                                "primary_key": false,
                                "unique": false
                            },
                            {
                                "name": "message",
                                "datatype": "${TEXT_BIG}",
                                "nullable": false,
                                "primary_key": false,
                                "unique": false
                            },
                            {
                                "name": "chacha_token",
                                "datatype": "${VARCHAR_500}",
                                "nullable": false,
                                "primary_key": false,
                                "unique": false
                            },
                            {
                                "name": "chacha_site",
                                "datatype": "${VARCHAR_20}",
                                "nullable": false,
                                "primary_key": false,
                                "unique": false
                            },
                            {
                                "name": "ip_address",
                                "datatype": "${VARCHAR_50}",
                                "nullable": true,
                                "primary_key": false,
                                "unique": false
                            },
                            {
                                "name": "review_status",
                                "datatype": "${INTEGER_SMALL}",
                                "nullable": false,
                                "primary_key": false,
                                "unique": false
                            },
                            ${COMMON_DIAGRAM}
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
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
return queries end
