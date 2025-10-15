-- Migration: acuranzo_1018.lua
-- Creates the sessions table and populating it with the next migration.

-- CHANGELOG
-- 1.1.0 -0 2025-09-28 - Changed diagram query to use JSON table definition instead of PlantUML for custom ERD tool.
-- 1.0.0 -0 2025-09-13 - Initial creation for sessions table with PostgreSQL support.

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
                            55,                                 -- query_id
                            1018,                               -- query_ref
                            ${TYPE_FORWARD_MIGRATIO},          -- query_type_lua_28    
                            ${DIALECT},                        -- query_dialect_lua_30    
                            'Create Sessions Table Query',      -- name, summary, query_code
                            [=[
                                # Forward Migration 1018: Create Sessions Table Query

                                This migration creates the sessions table for storing session data.
                            ]=],
                            [=[
                                CREATE TABLE IF NOT EXISTS ${SCHEMA}sessions
                                (
                                    session_id CHAR(20) PRIMARY KEY NOT NULL,   
                                    account_id ${INTEGER} NOT NULL,
                                    session ${BIGTEXT} NOT NULL,
                                    session_length ${INTEGER} NOT NULL,
                                    session_issues ${INTEGER} NOT NULL,
                                    session_changes ${INTEGER} NOT NULL,
                                    session_secs ${INTEGER} NOT NULL,
                                    status_lua_25 ${INTEGER} NOT NULL,
                                    flag_lua_26 ${INTEGER} NOT NULL,
                                    created_at ${TIMESTAMP_TZ} NOT NULL,
                                    updated_at ${TIMESTAMP_TZ} NOT NULL,
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
                            56,                                 -- query_id
                            1018,                               -- query_ref
                            ${TYPE_REVERSE_MIGRATIO},          -- query_type_lua_28    
                            ${DIALECT},                        -- query_dialect_lua_30    
                            'Delete Sessions Table Query',      -- name, summary, query_code
                            [=[
                                # Reverse Migration 1018: Delete Sessions Table Query

                                This is provided for completeness when testing the migration system
                                to ensure that forward and reverse migrations are complete.
                            ]=],
                            [=[
                                DROP TABLE ${SCHEMA}sessions; 
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
                            57,                                         -- query_id
                            1018,                                       -- query_ref
                            ${TYPE_DIAGRAM_MIGRATIO},                  -- query_type_lua_28    
                            ${DIALECT},                                -- query_dialect_lua_30    
                            'Diagram Tables: ${SCHEMA}sessions',       -- name, summary
                            [=[
                                # Diagram Migration 1018

                                ## Diagram Tables: ${SCHEMA}sessions

                                This is the first JSON Diagram code for the sessions table.
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
                                            "object_id": "table.sessions",
                                            "object_ref": "1018",
                                            "table": [
                                                {
                                                    "name": "session_id",
                                                    "datatype": "${TEXT}",
                                                    "nullable": false,
                                                    "primary_key": false,
                                                    "unique": false
                                                },
                                                {
                                                    "name": "account_id",
                                                    "datatype": "${INTEGER}",
                                                    "nullable": false,
                                                    "primary_key": false,
                                                    "unique": false
                                                },
                                                {
                                                    "name": "session",
                                                    "datatype": "${BIGTEXT}",
                                                    "nullable": false,
                                                    "primary_key": false,
                                                    "unique": false
                                                },
                                                {
                                                    "name": "session_length",
                                                    "datatype": "${INTEGER}",
                                                    "nullable": false,
                                                    "primary_key": false,
                                                    "unique": false
                                                },
                                                {
                                                    "name": "session_issues",
                                                    "datatype": "${INTEGER}",
                                                    "nullable": false,
                                                    "primary_key": false,
                                                    "unique": false
                                                },
                                                {
                                                    "name": "session_changes",
                                                    "datatype": "${INTEGER}",
                                                    "nullable": false,
                                                    "primary_key": false,
                                                    "unique": false
                                                },
                                                {
                                                    "name": "session_secs",
                                                    "datatype": "${INTEGER}",
                                                    "nullable": false,
                                                    "primary_key": false,
                                                    "unique": false
                                                },
                                                {
                                                    "name": "status_lua_25",
                                                    "datatype": "${INTEGER}",
                                                    "nullable": false,
                                                    "primary_key": false,
                                                    "unique": false,
                                                    "lookup": true
                                                },
                                                {
                                                    "name": "flag_lua_26",
                                                    "datatype": "${INTEGER}",
                                                    "nullable": false,
                                                    "primary_key": false,
                                                    "unique": false,
                                                    "lookup": true
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