-- Migration: acuranzo_1009.lua
-- Creates the dictionaries table and populating it with the next migration.

-- CHANGELOG
-- 1.1.0 -0 2025-09-28 - Changed diagram query to use JSON table definition instead of PlantUML for custom ERD tool.
-- 1.0.0 -0 2025-09-13 - Initial creation for dictionaries table with PostgreSQL support.

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
                            28,                                 -- query_id
                            1009,                               -- query_ref
                            %%TYPE_FORWARD_MIGRATIO%%,          -- query_type_lua_28    
                            %%DIALECT%%,                        -- query_dialect_lua_30    
                            'Create Dictionaries Table Query',  -- name, summary, query_code
                            [=[
                                # Forward Migration 1009: Create Dictionaries Table Query

                                This migration creates the dictionaries table for storing dictionary data.
                            ]=],
                            [=[
                                CREATE TABLE IF NOT EXISTS %%SCHEMA%%dictionaries
                                (
                                    dictionary_id %%INTEGER%% NOT NULL,
                                    language_id %%INTEGER%% NOT NULL,
                                    name %%VARCHAR_100%% NOT NULL,
                                    summary %%VARCHAR_500%%,
                                    status_lua_3 %%INTEGER%% NOT NULL,
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
                            29,                                         -- query_id
                            1009,                                       -- query_ref
                            %%TYPE_REVERSE_MIGRATIO%%,                  -- query_type_lua_28    
                            %%DIALECT%%,                                -- query_dialect_lua_30    
                            'Delete Dictionaries Table Query',          -- name, summary, query_code
                            [=[
                                # Reverse Migration 1009: Delete Dictionaries Table Query

                                This is provided for completeness when testing the migration system
                                to ensure that forward and reverse migrations are complete.
                            ]=],
                            [=[
                                DROP TABLE %%SCHEMA%%dictionaries; 
                            ]=],
                            %%STATUS_ACTIVE%%,                          -- query_status_lua_27
                            NULL,                                       -- collection
                            NULL,                                       -- valid_after
                            NULL,                                       -- valid_until
                            0,                                          -- created_id
                            %%NOW%%,                                    -- created_at
                            0,                                          -- updated_id
                            %%NOW%%                                     -- updated_at
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
                            30,                                         -- query_id
                            1009,                                       -- query_ref
                            %%TYPE_DIAGRAM_MIGRATIO%%,                  -- query_type_lua_28    
                            %%DIALECT%%,                                -- query_dialect_lua_30    
                            'Diagram Tables: %%SCHEMA%%dictionaries',   -- name, summary
                            [=[
                                # Diagram Migration 1009

                                ## Diagram Tables: %%SCHEMA%%dictionaries

                                This is the first JSON Diagram code for the dictionaries table.
                            ]=],
                            'JSON Table Definition in collection',      -- query_code,
                            %%STATUS_ACTIVE%%,                          -- query_status_lua_27, collection
                                                                        -- DIAGRAM_START  
                            %%JSON_INGEST_START%%                      
                            [=[
                                {
                                    "diagram": [
                                        {
                                            "object_type": "table",
                                            "object_id": "table.dictionaries",
                                            "object_ref": "1009",
                                            "table": [
                                                {
                                                    "name": "dictionary_id",
                                                    "datatype": "%%INTEGER%%",
                                                    "nullable": false,
                                                    "primary_key": true,
                                                    "unique": false
                                                },
                                                {
                                                    "name": "language_id",
                                                    "datatype": "%%INTEGER%%",
                                                    "nullable": false,
                                                    "primary_key": true,
                                                    "unique": false
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
                                                    "name": "status_lua_3",
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
                                }
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