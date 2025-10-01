-- Migration: acuranzo_1003.lua
-- Creates the account_contacts table and populating it with the next migration.

-- CHANGELOG
-- 1.1.0 -0 2025-09-28 - Changed diagram query to use JSON table definition instead of PlantUML for custom ERD tool.
-- 1.0.0 -0 2025-09-13 - Initial creation for account_contacts table with PostgreSQL support.

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
                            10,                                         -- query_id
                            1003,                                       -- query_ref
                            %%TYPE_FORWARD_MIGRATIO%%,                  -- query_type_lua_28    
                            %%DIALECT%%,                                -- query_dialect_lua_30    
                            'Create Account Contacts Table Query',      -- name, summary, query_code
                            [=[
                                # Forward Migration 1003: Create Account Contacts Table Query

                                This migration creates the account_contacts table for storing account contact data.
                            ]=],
                            [=[
                                CREATE TABLE %%SCHEMA%%account_contacts
                                (
                                    contact_id %%INTEGER%% NOT NULL,
                                    account_id %%INTEGER%% NOT NULL,
                                    contact_type_lua_18 %%INTEGER%% NOT NULL,
                                    contact_seq %%INTEGER%% NOT NULL,
                                    contact %%VARCHAR_100%% NOT NULL,
                                    summary %%VARCHAR_500%%,
                                    authenticate_lua_19 %%INTEGER%% NOT NULL,
                                    status_lua_20 %%INTEGER%% NOT NULL,
                                    collection %%JSONB%%,
                                    valid_after %%TIMESTAMP_TZ%%,
                                    valid_until %%TIMESTAMP_TZ%%,
                                    created_id %%INTEGER%% NOT NULL,
                                    created_at %%TIMEZONE_TZ%% NOT NULL,
                                    updated_id %%INTEGER%% NOT NULL,
                                    updated_at %%TIMEZONE_TZ%% NOT NULL
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
                            11,                                         -- query_id
                            1003,                                       -- query_ref
                            %%TYPE_REVERSE_MIGRATIO%%,                  -- query_type_lua_28    
                            %%DIALECT%%,                                -- query_dialect_lua_30    
                            'Delete Account Contacts Table Query',      -- name, summary, query_code
                            [=[
                                # Reverse Migration 1003: Delete Account Contacts Table Query

                                This is provided for completeness when testing the migration system
                                to ensure that forward and reverse migrations are complete.
                            ]=],
                            [=[
                                DROP TABLE %%SCHEMA%%account_contacts; 
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
                            12,                                             -- query_id
                            1003,                                           -- query_ref
                            %%TYPE_DIAGRAM_MIGRATIO%%,                      -- query_type_lua_28    
                            %%DIALECT%%,                                    -- query_dialect_lua_30    
                            'Diagram Tables: %%SCHEMA%%account_contacts',   -- name, summary
                            [=[
                                # Diagram Migration 1003

                                ## Diagram Tables: %%SCHEMA%%account_contacts

                                This is the first JSON Diagram code for the account_contacts table.
                            ]=],
                            'JSON Table Definition in collection',          -- query_code,
                            %%STATUS_ACTIVE%%,                              -- query_status_lua_27, collection
                            %%JSON_INGEST_START%%                           -- DIAGRAM_START
                            [=[
                                [
                                    {
                                        "object_type": "table",
                                        "object_id": "table.account_contacts",
                                        "object_ref": "1003",
                                        "table": [
                                            {
                                                "name": "account_id",
                                                "datatype": "%%INTEGER%%",
                                                "nullable": false,
                                                "primary_key": true,
                                                "unique": false
                                            },
                                            {
                                                "name": "contact_id",
                                                "datatype": "%%INTEGER%%",
                                                "nullable": false,
                                                "primary_key": true,
                                                "unique": true
                                            },
                                            {
                                                "name": "contact_seq",
                                                "datatype": "%%INTEGER%%",
                                                "nullable": false,
                                                "primary_key": true,
                                                "unique": false
                                            },
                                            {
                                                "name": "contact",
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
                                                "name": "contact_type_lua_18",
                                                "datatype": "%%INTEGER%%",
                                                "nullable": false,
                                                "primary_key": false,
                                                "unique": false,
                                                "lookup": true
                                            },
                                            {
                                                "name": "authenticate_lua_19",
                                                "datatype": "%%INTEGER%%",
                                                "nullable": false,
                                                "primary_key": false,
                                                "unique": false,
                                                "lookup": true
                                            },
                                            {
                                                "name": "status_lua_20",
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