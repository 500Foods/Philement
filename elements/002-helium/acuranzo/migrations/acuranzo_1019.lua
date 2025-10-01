-- Migration: acuranzo_1019.lua
-- Creates the systems table and populating it with the next migration.

-- CHANGELOG
-- 1.1.0 -0 2025-09-28 - Changed diagram query to use JSON table definition instead of PlantUML for custom ERD tool.
-- 1.0.0 -0 2025-09-13 - Initial creation for systems table with PostgreSQL support.

local config = require 'database'

return {
    queries = {
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --        
        {
            sql =   [[
                        INSERT INTO %%SCHEMA%%queries (
                            %%QUERY_INSERT_COLUMNS%%
                        )           
                        VALUES (
                            58,                                 -- query_id
                            1019,                               -- query_ref
                            %%TYPE_FORWARD_MIGRATIO%%,          -- query_type_lua_28    
                            %%DIALECT%%,                        -- query_dialect_lua_30    
                            'Create Systems Table Query',       -- name, summary, query_code
                            [=[
                                # Forward Migration 1019: Create Systems Table Query

                                This migration creates the systems table for storing system data.
                            ]=],
                            [=[
                                CREATE TABLE IF NOT EXISTS %%SCHEMA%%systems
                                (
                                    system_id %%INTEGER%% NOT NULL,
                                    name %%VARCHAR_100%% NOT NULL,
                                    summary %%VARCHAR_500%%,
                                    access_type_lua_15 %%INTEGER%% NOT NULL,
                                    status_lua_14 %%INTEGER%% NOT NULL,
                                    collection %%JSONB%%,
                                    valid_after %%TIMESTAMP_TZ%%,
                                    valid_until %%TIMESTAMP_TZ%%,
                                    created_id %%INTEGER%% NOT NULL,
                                    created_at %%TIMESTAMP_TZ%% NOT NULL,
                                    updated_id %%INTEGER%% NOT NULL,
                                    updated_at %%TIMESTAMP_TZ%% NOT NULL
                                );
                            ]=],
                            %%STATUS_ACTIVE%%,                  -- query_status_lua_27
                            NULL,                               -- collection
                            NULL,                               -- valid_after
                            NULL,                               -- valid_until
                            0,                                  -- created_id
                            %%NOW%%,                            -- created_at
                            0,                                  -- updated_id
                            %%NOW%%                             -- updated_at
                        );
                    ]]
        },        
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --        
        {
            sql =   [[
                        INSERT INTO %%SCHEMA%%queries (
                            %%QUERY_INSERT_COLUMNS%%
                        )           
                        VALUES (
                            59,                                 -- query_id
                            1019,                               -- query_ref
                            %%TYPE_REVERSE_MIGRATIO%%,          -- query_type_lua_28    
                            %%DIALECT%%,                        -- query_dialect_lua_30    
                            'Delete Systems Table Query',       -- name, summary, query_code
                            [=[
                                # Reverse Migration 1019: Delete Systems Table Query

                                This is provided for completeness when testing the migration system
                                to ensure that forward and reverse migrations are complete.
                            ]=],
                            [=[
                                DROP TABLE %%SCHEMA%%systems; 
                            ]=],
                            %%STATUS_ACTIVE%%,                  -- query_status_lua_27
                            NULL,                               -- collection
                            NULL,                               -- valid_after
                            NULL,                               -- valid_until
                            0,                                  -- created_id
                            %%NOW%%,                            -- created_at
                            0,                                  -- updated_id
                            %%NOW%%                             -- updated_at
                        );
                    ]]
        },        
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --       
        {
            sql =   [[
                        INSERT INTO %%SCHEMA%%queries (
                            %%QUERY_INSERT_COLUMNS%%
                        )           
                        VALUES (
                            60,                                         -- query_id
                            1019,                                       -- query_ref
                            %%TYPE_DIAGRAM_MIGRATIO%%,                  -- query_type_lua_28    
                            %%DIALECT%%,                                -- query_dialect_lua_30    
                            'Diagram Tables: %%SCHEMA%%systems',        -- name, summary
                            [=[
                                # Diagram Migration 1019

                                ## Diagram Tables: %%SCHEMA%%systems

                                This is the first JSON Diagram code for the systems table.
                            ]=],
                            'JSON Table Definition in collection',      -- query_code,
                            %%STATUS_ACTIVE%%,                          -- query_status_lua_27, collection
                            %%JSON_INGEST_START%%                       -- DIAGRAM_START
                            [=[
                                [
                                    {
                                        "object_type": "table",
                                        "object_id": "table.systems",
                                        "object_ref": "1019",
                                        "table": [
                                            {
                                                "name": "system_id",
                                                "datatype": "%%INTEGER%%",
                                                "nullable": false,
                                                "primary_key": true,
                                                "unique": true
                                            },
                                            {
                                                "name": "name",
                                                "datatype": "%%VARCHAR_100%%",
                                                "nullable": false,
                                                "primary_key": false,
                                                "unique": false
                                            },
                                            {
                                                "name": "summary",
                                                "datatype": "%%VARCHAR_500%%",
                                                "nullable": true,
                                                "primary_key": false,
                                                "unique": false
                                            },
                                            {
                                                "name": "status_lua_14",
                                                "datatype": "%%INTEGER%%",
                                                "nullable": false,
                                                "primary_key": false,
                                                "unique": false,
                                                "lookup": true
                                            },
                                            {
                                                "name": "access_type_lua_15",
                                                "datatype": "%%INTEGER%%",
                                                "nullable": false,
                                                "primary_key": false,
                                                "unique": false,
                                                "lookup": true
                                            },
                                            {
                                                "name": "collection",
                                                "datatype": "%%JSONB%%",
                                                "nullable": true,
                                                "primary_key": false,
                                                "unique": false,
                                                "standard": true
                                            },
                                            {
                                                "name": "valid_after",
                                                "datatype": "%%TIMESTAMP_TZ%%",
                                                "nullable": true,
                                                "primary_key": false,
                                                "unique": false,
                                                "standard": true
                                            },
                                            {
                                                "name": "valid_until",
                                                "datatype": "%%TIMESTAMP_TZ%%",
                                                "nullable": true,
                                                "primary_key": false,
                                                "unique": false,
                                                "standard": true
                                            },
                                            {
                                                "name": "created_id",
                                                "datatype": "%%INTEGER%%",
                                                "nullable": false,
                                                "primary_key": false,
                                                "unique": false,
                                                "standard": true
                                            },
                                            {
                                                "name": "created_at",
                                                "datatype": "%%TIMESTAMP_TZ%%",
                                                "nullable": false,
                                                "primary_key": false,
                                                "unique": false,
                                                "standard": true
                                            },
                                            {
                                                "name": "updated_id",
                                                "datatype": "%%INTEGER%%",
                                                "nullable": false,
                                                "primary_key": false,
                                                "unique": false,
                                                "standard": true
                                            },
                                            {
                                                "name": "updated_at",
                                                "datatype": "%%TIMESTAMP_TZ%%",
                                                "nullable": false,
                                                "primary_key": false,
                                                "unique": false,
                                                "standard": true
                                            }
                                        ]
                                    }
                                ]
                            ]=]
                            %%JSON_INGEST_END%%                 -- DIAGRAM_END
                            ,
                            NULL,                               -- valid_after
                            NULL,                               -- valid_until
                            0,                                  -- created_id
                            %%NOW%%,                            -- created_at
                            0,                                  -- updated_id
                            %%NOW%%                             -- updated_at
                        );
                    ]]
        }
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --           
    }
}