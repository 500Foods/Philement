-- Migration: acuranzo_1002.lua
-- Creates the account_access table and populating it with the next migration.

-- CHANGELOG
-- 1.1.0 -0 2025-09-28 - Changed diagram query to use JSON table definition instead of PlantUML for custom ERD tool.
-- 1.0.0 -0 2025-09-13 - Initial creation for account_access table with PostgreSQL support.

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
                            7,                                      -- query_id
                            1002,                                   -- query_ref
                            %%TYPE_FORWARD_MIGRATIO%%,              -- query_type_lua_28    
                            %%DIALECT%%,                            -- query_dialect_lua_30    
                            'Create Account Access Table Query',    -- name, summary, query_code
                            [=[
                                # Forward Migration 1002: Create Account Access Table Query

                                This migration creates the account_access table for storing account access data.
                            ]=],
                            [=[
                                CREATE TABLE IF NOT EXISTS %%SCHEMA%%account_access
                                (
                                    account_id integer NOT NULL,
                                    access_id integer NOT NULL,
                                    feature_lua_21 integer NOT NULL,
                                    access_type_lua_22 integer NOT NULL,
                                    status_lua_23 integer NOT NULL,
                                    collection jsonb,
                                    valid_after timestamp with time zone,
                                    valid_until timestamp with time zone,
                                    created_id integer NOT NULL,
                                    created_at timestamp with time zone NOT NULL,
                                    updated_id integer NOT NULL,
                                    updated_at timestamp with time zone NOT NULL,
                                    CONSTRAINT account_access_pkey PRIMARY KEY (access_id)
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
                            8,                                      -- query_id
                            1002,                                   -- query_ref
                            %%TYPE_REVERSE_MIGRATIO%%,              -- query_type_lua_28    
                            %%DIALECT%%,                            -- query_dialect_lua_30    
                            'Delete Account Access Table Query',    -- name, summary, query_code
                            [=[
                                # Reverse Migration 1002: Delete Account Access Table Query

                                This is provided for completeness when testing the migration system
                                to ensure that forward and reverse migrations are complete.
                            ]=],
                            [=[
                                DROP TABLE %%SCHEMA%%account_access; 
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
                            9,                                              -- query_id
                            1002,                                           -- query_ref
                            %%TYPE_DIAGRAM_MIGRATIO%%,                      -- query_type_lua_28    
                            %%DIALECT%%,                                    -- query_dialect_lua_30    
                            'Diagram Tables: %%SCHEMA%%account_access',     -- name, summary
                            [=[
                                # Diagram Migration 1002

                                ## Diagram Tables: %%SCHEMA%%account_access

                                This is the first JSON Diagram code for the account_access table.
                            ]=],
                            'JSON Table Definition in collection',      -- query_code,
                            %%STATUS_ACTIVE%%,                          -- query_status_lua_27, collection
                            %%JSON_INGEST_START%%                       -- DIAGRAM_START
                            [=[
                                [ 
                                    {
                                        "object_type": "table",
                                        "object_id": "table.account_access",
                                        "object_ref": "1002",
                                        "table": [
                                            {
                                                "name": "access_id",
                                                "datatype": "%%INTEGER%%",
                                                "nullable": false,
                                                "primary_key": true,
                                                "unique": true
                                            },
                                            {
                                                "name": "account_id",
                                                "datatype": "%%INTEGER%%",
                                                "nullable": false,
                                                "primary_key": false,
                                                "unique": false
                                            },
                                            {
                                                "name": "feature_lua_21",
                                                "datatype": "%%INTEGER%%",
                                                "nullable": false,
                                                "primary_key": false,
                                                "unique": false,
                                                "lookup": true
                                            },
                                            {
                                                "name": "access_type_lua_22",
                                                "datatype": "%%INTEGER%%",
                                                "nullable": false,
                                                "primary_key": false,
                                                "unique": false,
                                                "lookup": true
                                            },
                                            {
                                                "name": "status_lua_23",
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
                            %%MOW%%                             -- updated_at
                        );
                    ]]
        }
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --           
    }
}