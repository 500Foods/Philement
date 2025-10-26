-- Migration: acuranzo_1008.lua
-- Creates the convos table and populating it with the next migration.

-- CHANGELOG
-- 1.1.0 -0 2025-09-28 - Changed diagram query to use JSON table definition instead of PlantUML for custom ERD tool.
-- 1.0.0 -0 2025-09-13 - Initial creation for convos table with PostgreSQL support.

local config = require 'database'

return {
    queries = {
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --        
        {
            sql =   [[
                        INSERT INTO ${SCHEMA}queries (
                            ${QUERY_INSERT_COLUMNS}
                        )           
                        VALUES (
                            25,                                 -- query_id
                            1008,                               -- query_ref
                            ${TYPE_FORWARD_MIGRATIO},          -- query_type_lua_28    
                            ${DIALECT},                        -- query_dialect_lua_30    
                            'Create Convos Table Query',        -- name, summary, query_code
                            [=[
                                # Forward Migration 1008: Create Convos Table Query

                                This migration creates the convos table for storing conversation data.
                            ]=],
                            [=[
                                CREATE TABLE IF NOT EXISTS ${SCHEMA}convos
                                (
                                    convos_id ${INTEGER} NOT NULL,
                                    convos_ref ${VARCHAR_50} NOT NULL,
                                    convos_keywords ${VARCHAR_500},
                                    convos_icon ${VARCHAR_500},
                                    prompt ${BIGTEXT},
                                    response ${BIGTEXT},
                                    context ${BIGTEXT},
                                    history ${BIGTEXT},
                                    collection ${JSONB},
                                    valid_after ${TIMEZONE_TZ},
                                    valid_until ${TIMEZONE_TZ},
                                    created_id ${INTEGER} NOT NULL,
                                    created_at ${TIMEZONE_TZ} NOT NULL,
                                    updated_id ${INTEGER} NOT NULL,
                                    updated_at ${TIMEZONE_TZ} NOT NULL,
                                );
                            ]=],
                            ${STATUS_ACTIVE},                  -- query_status_lua_27
                            NULL,                               -- collection
                            NULL,                               -- valid_after
                            NULL,                               -- valid_until
                            0,                                  -- created_id
                            ${NOW},                            -- created_at
                            0,                                  -- updated_id
                            ${NOW}                             -- updated_at
                        );
                    ]]
        },        
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --        
        {
            sql =   [[
                        INSERT INTO ${SCHEMA}queries (
                            ${QUERY_INSERT_COLUMNS}
                        )           
                        VALUES (
                            26,                                 -- query_id
                            1008,                               -- query_ref
                            ${TYPE_REVERSE_MIGRATIO},          -- query_type_lua_28    
                            ${DIALECT},                        -- query_dialect_lua_30    
                            'Delete Convos Table Query',        -- name, summary, query_code
                            [=[
                                # Reverse Migration 1008: Delete Convos Table Query

                                This is provided for completeness when testing the migration system
                                to ensure that forward and reverse migrations are complete.
                            ]=],
                            [=[
                                DROP TABLE ${SCHEMA}convos; 
                            ]=],
                            ${STATUS_ACTIVE},                  -- query_status_lua_27
                            NULL,                               -- collection
                            NULL,                               -- valid_after
                            NULL,                               -- valid_until
                            0,                                  -- created_id
                            ${NOW},                            -- created_at
                            0,                                  -- updated_id
                            ${NOW}                             -- updated_at
                        );
                    ]]
        },        
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --       
        {
            sql =   [[
                        INSERT INTO ${SCHEMA}queries (
                            ${QUERY_INSERT_COLUMNS}
                        )           
                        VALUES (
                            27,                                         -- query_id
                            1008,                                       -- query_ref
                            ${TYPE_DIAGRAM_MIGRATIO},                  -- query_type_lua_28    
                            ${DIALECT},                                -- query_dialect_lua_30    
                            'Diagram Tables: ${SCHEMA}convos',         -- name, summary
                            [=[
                                # Diagram Migration 1008

                                ## Diagram Tables: ${SCHEMA}convos

                                This is the first JSON Diagram code for the convos table.
                            ]=],
                            'JSON Table Definition in collection',      -- query_code,
                            ${STATUS_ACTIVE},                          -- query_status_lua_27, collection
                                                                        -- DIAGRAM_START  
                            ${JSON_INGEST_START}                      
                            [=[
                                {
                                    "diagram": [
                                        {
                                            "object_type": "table",
                                            "object_id": "table.convos",
                                            "object_ref": "1008",
                                            "table": [
                                                {
                                                    "name": "convos_id",
                                                    "datatype": "${INTEGER}",
                                                    "nullable": false,
                                                    "primary_key": true,
                                                    "unique": true
                                                },
                                                {
                                                    "name": "convos_ref",
                                                    "datatype": "${VARCHAR_50}",
                                                    "nullable": false,
                                                    "primary_key": false,
                                                    "unique": true
                                                },
                                                {
                                                    "name": "convos_keywords",
                                                    "datatype": "${VARCHAR_100}",
                                                    "nullable": true,
                                                    "primary_key": false,
                                                    "unique": false
                                                },
                                                {
                                                    "name": "convos_icon",
                                                    "datatype": "${VARCHAR_500}",
                                                    "nullable": true,
                                                    "primary_key": false,
                                                    "unique": false
                                                },
                                                {
                                                    "name": "prompt",
                                                    "datatype": "${BIGTEXT}",
                                                    "nullable": true,
                                                    "primary_key": false,
                                                    "unique": false
                                                },
                                                {
                                                    "name": "response",
                                                    "datatype": "${BIGTEXT}",
                                                    "nullable": true,
                                                    "primary_key": false,
                                                    "unique": false
                                                },
                                                {
                                                    "name": "context",
                                                    "datatype": "${BIGTEXT}",
                                                    "nullable": true,
                                                    "primary_key": false,
                                                    "unique": false
                                                },
                                                {
                                                    "name": "history",
                                                    "datatype": "${BIGTEXT}",
                                                    "nullable": true,
                                                    "primary_key": false,
                                                    "unique": false
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
                            ${JSON_INGEST_END}                 -- DIAGRAM_END
                            ,
                            NULL,                               -- valid_after
                            NULL,                               -- valid_until
                            0,                                  -- created_id
                            ${NOW},                            -- created_at
                            0,                                  -- updated_id
                            ${NOW}                             -- updated_at
                        );
                    ]]
        }
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --           
    }
}