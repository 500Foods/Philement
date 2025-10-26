-- Migration: acuranzo_1001.lua
-- Creates the lookups table and populating it with the next migration.

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 2.0.0 - 2025-10-18 - Moved to latest migration format
-- 1.1.0 - 2025-09-28 - Changed diagram query to use JSON table definition instead of PlantUML for custom ERD tool.
-- 1.0.0 - 2025-09-13 - Initial creation for lookups table with PostgreSQL support.

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "lookups"
cfg.MIGRATION = "1001"
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
table.insert(queries,{sql=[[

    INSERT INTO ${SCHEMA}${QUERIES} (
        ${QUERIES_INSERT}
    )
    VALUES (
        (SELECT COALESCE(MAX(query_id), 0) + 1 FROM ${SCHEMA}${QUERIES}),   -- query_id
        ${MIGRATION},                                                       -- query_ref
        ${STATUS_ACTIVE},                                                   -- query_status_a27
        ${TYPE_FORWARD_MIGRATION},                                          -- query_type_a28
        ${DIALECT},                                                         -- query_dialect_a30
        ${QTC_SLOW},                                                        -- query_queue_a58
        ${TIMEOUT}                                                          -- query_timeout
        [=[
            CREATE TABLE ${SCHEMA}{$TABLE}
            (
                lookup_id               ${INTEGER}          NOT NULL,
                key_idx                 ${INTEGER}          NOT NULL,
                status_a1               ${INTEGER}          NOT NULL,
                value_txt               ${TEXT}                     ,
                value_int               ${INTEGER}                  ,
                sort_seq                ${INTEGER}          NOT NULL,
                code                    ${TEXTBIG}                  ,
                summary                 ${TEXTBIG}                  ,
                collection              ${JSON}                     ,
                ${COMMON_CREATE}
                ${PRIMARY}(lookup_id, key_idx)
            );
        ]=],
                                                                            -- code
        'Create ${TABLE} Table',                                            -- name
        [=[
            # Forward Migration 1001: Create ${TABLE} Table

            This migration creates the ${TABLE} table for storing key-value lookup data.
        ]=],
                                                                            -- summary
        NULL,                                                               -- collection
        ${COMMON_INSERT}
    );

]]})
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
table.insert(queries,{sql=[[

    INSERT INTO ${SCHEMA}${QUERIES} (
        ${QUERIES_INSERT}
    )
    VALUES (
        (SELECT COALESCE(MAX(query_id), 0) + 1 FROM ${SCHEMA}${QUERIES}),   -- query_id
        ${MIGRATION},                                                       -- query_ref
        ${STATUS_ACTIVE},                                                   -- query_status_a27
        ${TYPE_REVERSE_MIGRATION},                                          -- query_type_a28
        ${DIALECT},                                                         -- query_dialect_a30
        ${QTC_SLOW},                                                        -- query_queue_a58
        ${TIMEOUT},                                                         -- query_timeout
        [=[
            DROP TABLE ${SCHEMA}${TABLE};
        ]=],
                                                                            -- code
        'Drop ${TABLE} Table',                                              -- name
        [=[
            # Reverse Migration ${MIGRATION}: Drop ${TABLE} Table

            This is provided for completeness when testing the migration system
            to ensure that forward and reverse migrations are complete.
        ]=],
                                                                            -- summary
        NULL,                                                               -- collection
        ${COMMON_INSERT}
    );

]]})
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
table.insert(queries,{sql=[[

    INSERT INTO ${SCHEMA}${QUERIES} (
        ${QUERIES_INSERT}
    )
    VALUES (
        (SELECT COALESCE(MAX(query_id), 0) + 1 FROM ${SCHEMA}${QUERIES}),   -- query_id
        ${MIGRATION},                                                       -- query_ref
        ${STATUS_ACTIVE},                                                   -- query_status_a27
        ${TYPE_DIAGRAM_MIGRATION},                                          -- query_type_a28
        ${DIALECT},                                                         -- query_dialect_a30
        ${QTC_SLOW},                                                        -- query_queue_a58
        ${TIMEOUT},                                                         -- query_timeout
        'JSON Table Definition in collection',                              -- code
        'Diagram Tables: ${SCHEMA}${TABLE}',                                -- name
        [=[
            # Diagram Migration ${MIGRATION}

            ## Diagram Tables: ${SCHEMA}${TABLE}

            This is the first JSON Diagram code for the ${TABLE} table.
        ]=],
                                                                            -- summary
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
                                "name": "lookup_id",
                                "datatype": "${INTEGER}",
                                "nullable": false,
                                "primary_key": true,
                                "unique": false
                            },
                            {
                                "name": "key_idx",
                                "datatype": "${INTEGER}",
                                "nullable": false,
                                "primary_key": true,
                                "unique": false
                            },
                            {
                                "name": "status_a1",
                                "datatype": "${INTEGER}",
                                "nullable": false,
                                "primary_key": false,
                                "unique": false,
                                "lookup": true
                            },
                            {
                                "name": "value_txt",
                                "datatype": "${VARCHAR_100}",
                                "nullable": true,
                                "primary_key": false,
                                "unique": false
                            },
                            {
                                "name": "value_int",
                                "datatype": "${INTEGER}",
                                "nullable": true,
                                "primary_key": false,
                                "unique": false
                            },
                            {
                                "name": "sort_seq",
                                "datatype": "${INTEGER}",
                                "nullable": false,
                                "primary_key": false,
                                "unique": false
                            },
                            {
                                "name": "code",
                                "datatype": "${TEXTBIG}",
                                "nullable": true,
                                "primary_key": false,
                                "unique": false
                            },
                            {
                                "name": "summary",
                                "datatype": "${TEXTBIG}",
                                "nullable": true,
                                "primary_key": false,
                                "unique": false,
                                "standard": false
                            },
                            {
                                "name": "collection",
                                "datatype": "${JSON}",
                                "nullable": true,
                                "primary_key": false,
                                "unique": false,
                                "standard": false
                            },
                            ${COMMON_DIAGRAM}
                        ]
                    }
                ]
            }
        ]=]
        ${JSON_INGEST_END}
                                                                            -- DIAGRAM_END
        ,                                                                   -- collection
        ${COMMON_INSERT}
    );

]]})
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
return queries end
