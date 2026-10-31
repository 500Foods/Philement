-- Migration: acuranzo_1418.lua
-- Creates the aem_observations table for Audience and Engagement Metrics

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.0.0 - 2026-10-30 - Initial creation

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "aem_observations"
cfg.MIGRATION = "1418"

-- ----------------------------------------------------------------------------
-- Forward: Create aem_observations
-- ----------------------------------------------------------------------------
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
                property_a69        ${INTEGER_SMALL}    NOT NULL,
                channel_a70         ${INTEGER_SMALL}    NOT NULL,
                metric_a72          ${INTEGER_SMALL}    NOT NULL,
                kind_a71            ${INTEGER_SMALL}    NOT NULL,
                source_a73          ${INTEGER_SMALL}    NOT NULL,
                observed_at         ${TIMESTAMP_TZ}     NOT NULL,
                value_num           ${FLOAT_BIG}                ,
                value_text          ${TEXT}                     ,
                dims                ${JSON}             NOT NULL,
                dims_hash           ${VARCHAR_64}       NOT NULL,
                ${COMMON_CREATE}
                ${PRIMARY}(property_a69, channel_a70, metric_a72, observed_at, dims_hash)
            );

            ${SUBQUERY_DELIMITER}

            CREATE INDEX ${TABLE}_idx_series
                ON ${SCHEMA}${TABLE}(property_a69, channel_a70, metric_a72, dims_hash, observed_at);

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Create ${TABLE} Table'                                                            AS name,
        [=[
            # Forward Migration ${MIGRATION}: Create ${TABLE} Table

            One fact row per 15-minute bin. property, channel, metric,
            observed_at, and dims_hash are the primary key. kind is a
            column and is not part of the key. There is no surrogate id
            and no foreign key. dims has no default.

            ## Index

            - PRIMARY KEY (property_a69, channel_a70, metric_a72, observed_at, dims_hash).
            - INDEX ${TABLE}_idx_series (property_a69, channel_a70, metric_a72, dims_hash, observed_at)
              for the gauge predecessor lookup.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})
-- ----------------------------------------------------------------------------
-- Reverse: Drop aem_observations
-- ----------------------------------------------------------------------------
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
            -- DROP INDEX ${TABLE}_idx_series;

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
        'Drop ${TABLE} Table'                                                            AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Drop ${TABLE} Table

            Drops the table. The series index goes with it.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})

-- ----------------------------------------------------------------------------
-- Diagram: ${TABLE}
-- ----------------------------------------------------------------------------
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
                                "name": "property_a69",
                                "datatype": "${INTEGER_SMALL}",
                                "nullable": false,
                                "primary_key": true,
                                "unique": false,
                                "lookup": true
                            },
                            {
                                "name": "channel_a70",
                                "datatype": "${INTEGER_SMALL}",
                                "nullable": false,
                                "primary_key": true,
                                "unique": false,
                                "lookup": true
                            },
                            {
                                "name": "metric_a72",
                                "datatype": "${INTEGER_SMALL}",
                                "nullable": false,
                                "primary_key": true,
                                "unique": false,
                                "lookup": true
                            },
                            {
                                "name": "kind_a71",
                                "datatype": "${INTEGER_SMALL}",
                                "nullable": false,
                                "primary_key": false,
                                "unique": false,
                                "lookup": true
                            },
                            {
                                "name": "source_a73",
                                "datatype": "${INTEGER_SMALL}",
                                "nullable": false,
                                "primary_key": false,
                                "unique": false,
                                "lookup": true
                            },
                            {
                                "name": "observed_at",
                                "datatype": "${TIMESTAMP_TZ}",
                                "nullable": false,
                                "primary_key": true,
                                "unique": false
                            },
                            {
                                "name": "value_num",
                                "datatype": "${FLOAT_BIG}",
                                "nullable": true,
                                "primary_key": false,
                                "unique": false
                            },
                            {
                                "name": "value_text",
                                "datatype": "${TEXT}",
                                "nullable": true,
                                "primary_key": false,
                                "unique": false
                            },
                            {
                                "name": "dims",
                                "datatype": "${JSON}",
                                "nullable": false,
                                "primary_key": false,
                                "unique": false
                            },
                            {
                                "name": "dims_hash",
                                "datatype": "${VARCHAR_64}",
                                "nullable": false,
                                "primary_key": true,
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
return queries
end
