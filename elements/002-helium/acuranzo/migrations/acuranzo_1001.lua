-- Migration: acuranzo_1001.lua
-- Creates the lookups table and populating it with the next migration.

-- CHANGELOG
-- 2.0.0 - 2025-10-18 - Moved to latest migration format
-- 1.1.0 - 2025-09-28 - Changed diagram query to use JSON table definition instead of PlantUML for custom ERD tool.
-- 1.0.0 - 2025-09-13 - Initial creation for lookups table with PostgreSQL support.

return function(engine, design_name, schema_name, cfg)
local queries = {}
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
table.insert(queries,{sql=[[

    INSERT INTO ${SCHEMA}queries (
        ${QUERIES_INSERT}
    )
    VALUES (
        4,                                                                  -- query_id
        1001,                                                               -- query_ref
        ${TYPE_FORWARD_MIGRATION},                                          -- query_type_lua_28
        ${DIALECT},                                                         -- query_dialect_lua_30
        'Create Lookups Table Query',                                       -- name, summary, query_code
        [=[
            # Forward Migration 1001: Create Lookups Table Query

            This migration creates the lookups table for storing key-value lookup data.
        ]=],
        [=[
            CREATE TABLE IF NOT EXISTS ${SCHEMA}lookups
            (
                lookup_id               ${INTEGER}          NOT NULL,
                key_idx                 ${INTEGER}          NOT NULL,
                value_txt               ${TEXT}                     ,
                value_int               ${INTEGER}                  ,
                sort_seq                ${INTEGER}          NOT NULL,
                status_lua_1            ${INTEGER}          NOT NULL,
                summary                 ${BIGTEXT}                  ,
                code                    ${BIGTEXT}                  ,
                collection              ${JSONB}                    ,
                valid_after             ${TIMESTAMP_TZ}             ,
                valid_until             ${TIMESTAMP_TZ}             ,
                created_id              ${INTEGER}          NOT NULL,
                created_at              ${TIMESTAMP_TZ}     NOT NULL,
                updated_id              ${INTEGER}          NOT NULL,
                updated_at              ${TIMESTAMP_TZ}     NOT NULL,
                ${PRIMARY}(lookup_id, key_idx)
            );
        ]=],
        ${STATUS_ACTIVE},                                                   -- query_status_lua_27
        NULL,                                                               -- collection
        ${QUERIES_COMMON}
    );

]]})
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
table.insert(queries,{sql=[[

    INSERT INTO ${SCHEMA}queries (
        ${QUERIES_INSERT}
    )
    VALUES (
        5,                                                                  -- query_id
        1001,                                                               -- query_ref
        ${TYPE_REVERSE_MIGRATION},                                          -- query_type_lua_28
        ${DIALECT},                                                         -- query_dialect_lua_30
        'Delete Lookups Table Query',                                       -- name, summary, query_code
        [=[
            # Reverse Migration 1001: Delete Lookups Table Query

            This is provided for completeness when testing the migration system
            to ensure that forward and reverse migrations are complete.
        ]=],
        [=[
            DROP TABLE ${SCHEMA}lookups;
        ]=],
        ${STATUS_ACTIVE},                                                   -- query_status_lua_27
        NULL,                                                               -- collection
        ${QUERIES_COMMON}
    );

]]})
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
table.insert(queries,{sql=[[

    INSERT INTO ${SCHEMA}queries (
        ${QUERIES_INSERT}
    )
    VALUES (
        6,                                                                  -- query_id
        1001,                                                               -- query_ref
        ${TYPE_DIAGRAM_MIGRATION},                                          -- query_type_lua_28
        ${DIALECT},                                                         -- query_dialect_lua_30
        'Diagram Tables: ${SCHEMA}lookups',                                 -- name, summary
        [=[
            # Diagram Migration 1001

            ## Diagram Tables: ${SCHEMA}lookups

            This is the first JSON Diagram code for the lookups table.
        ]=],
        'JSON Table Definition in collection',                              -- query_code,
        ${STATUS_ACTIVE},                                                   -- query_status_lua_27
                                                                            -- DIAGRAM_START
        ${JSON_INGEST_START}
        [=[
            {
                "diagram": [
                    {
                        "object_type": "table",
                        "object_id": "table.lookups",
                        "object_ref": "1001",
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
                                "name": "summary",
                                "datatype": "${TEXT}",
                                "nullable": true,
                                "primary_key": false,
                                "unique": false
                            },
                            {
                                "name": "code",
                                "datatype": "${TEXT}",
                                "nullable": true,
                                "primary_key": false,
                                "unique": false
                            },
                            {
                                "name": "status_lua_1",
                                "datatype": "${INTEGER}",
                                "nullable": false,
                                "primary_key": false,
                                "unique": false,
                                "lookup": true
                            },
                            {
                                "name": "collection",
                                "datatype": "${JSONB}",
                                "nullable": true,
                                "primary_key": false,
                                "unique": false,
                                "standard": true
                            },
                            {
                                "name": "valid_after",
                                "datatype": "${TIMESTAMP_TZ}",
                                "nullable": true,
                                "primary_key": false,
                                "unique": false,
                                "standard": true
                            },
                            {
                                "name": "valid_until",
                                "datatype": "${TIMESTAMP_TZ}",
                                "nullable": true,
                                "primary_key": false,
                                "unique": false,
                                "standard": true
                            },
                            {
                                "name": "created_id",
                                "datatype": "${INTEGER}",
                                "nullable": false,
                                "primary_key": false,
                                "unique": false,
                                "standard": true
                            },
                            {
                                "name": "created_at",
                                "datatype": "${TIMESTAMP_TZ}",
                                "nullable": false,
                                "primary_key": false,
                                "unique": false,
                                "standard": true
                            },
                            {
                                "name": "updated_id",
                                "datatype": "${INTEGER}",
                                "nullable": false,
                                "primary_key": false,
                                "unique": false,
                                "standard": true
                            },
                            {
                                "name": "updated_at",
                                "datatype": "${TIMESTAMP_TZ}",
                                "nullable": false,
                                "primary_key": false,
                                "unique": false,
                                "standard": true
                            }
                        ]
                    }
                ]
            }
        ]=]
        ${JSON_INGEST_END}
                                                                            -- DIAGRAM_END
        ,                                                                   -- collection
        ${QUERIES_COMMON}
    );

]]})
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
return queries end