-- Migration: acuranzo_1021.lua
-- Creates the tokens table and populating it with the next migration.

-- CHANGELOG
-- 1.1.0 -0 2025-09-28 - Changed diagram query to use JSON table definition instead of PlantUML for custom ERD tool.
-- 1.0.0 -0 2025-09-13 - Initial creation for tokens table with PostgreSQL support.

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
                            64,                                 -- query_id
                            1021,                               -- query_ref
                            %%TYPE_FORWARD_MIGRATIO%%,          -- query_type_lua_28    
                            %%DIALECT%%,                        -- query_dialect_lua_30    
                            'Create Tokens Table Query',        -- name, summary, query_code
                            [=[
                                # Forward Migration 1021: Create Tokens Table Query

                                This migration creates the tokens table for storing token data.
                            ]=],
                            [=[
                                CREATE TABLE IF NOT EXISTS app.tokens
                                (
                                    token_hash character(128) COLLATE pg_catalog."default" NOT NULL,
                                    account_id integer NOT NULL,
                                    system_id integer NOT NULL,
                                    application_id integer NOT NULL,
                                    application_version character varying(20) COLLATE pg_catalog."default" NOT NULL,
                                    ip_address character varying(50) COLLATE pg_catalog."default" NOT NULL,
                                    valid_after timestamp with time zone NOT NULL,
                                    valid_until timestamp with time zone NOT NULL,
                                    CONSTRAINT tokens_pkey PRIMARY KEY (token_hash)
                                );
                            ]=],
                            %%STATUS_ACTIVE%%,                  -- query_status_lua_27
                            NULL,                               -- collection
                            NULL,                               -- valid_after
                            NULL,                               -- valid_until
                            0,                                  -- created_id
                            %%TIMESTAMP%%,                      -- created_at
                            0,                                  -- updated_id
                            %%TIMESTAMP%%                       -- updated_at
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
                            65,                                 -- query_id
                            1021,                               -- query_ref
                            %%TYPE_REVERSE_MIGRATIO%%,          -- query_type_lua_28    
                            %%DIALECT%%,                        -- query_dialect_lua_30    
                            'Delete Tokens Table Query',        -- name, summary, query_code
                            [=[
                                # Reverse Migration 1021: Delete Tokens Table Query

                                This is provided for completeness when testing the migration system
                                to ensure that forward and reverse migrations are complete.
                            ]=],
                            [=[
                                DROP TABLE app.tokens; 
                            ]=],
                            %%STATUS_ACTIVE%%,                  -- query_status_lua_27
                            NULL,                               -- collection
                            NULL,                               -- valid_after
                            NULL,                               -- valid_until
                            0,                                  -- created_id
                            %%TIMESTAMP%%,                      -- created_at
                            0,                                  -- updated_id
                            %%TIMESTAMP%%                       -- updated_at
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
                            66,                                 -- query_id
                            1021,                               -- query_ref
                            %%TYPE_DIAGRAM_MIGRATIO%%,          -- query_type_lua_28    
                            %%DIALECT%%,                        -- query_dialect_lua_30    
                            'Diagram Tables: app.tokens',       -- name, summary
                            [=[
                                # Diagram Migration 1021

                                ## Diagram Tables: app.tokens

                                This is the first JSON Diagram code for the tokens table.
                            ]=],
                            'JSON Table Definition in collection',     -- query_code,
                            %%STATUS_ACTIVE%%,                  -- query_status_lua_27, collection
                            %%JSON_INGEST_START%%
                            [=[
                                {
                                    "object_type": "table",
                                    "object_id": "table.tokens",
                                    "table": [
                                        {
                                            "name": "token_hash",
                                            "datatype": "%%CHAR_128%%",
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
                                            "name": "system_id",
                                            "datatype": "%%INTEGER%%",
                                            "nullable": false,
                                            "primary_key": false,
                                            "unique": false
                                        },
                                        {
                                            "name": "application_id",
                                            "datatype": "%%INTEGER%%",
                                            "nullable": false,
                                            "primary_key": false,
                                            "unique": false
                                        },
                                        {
                                            "name": "application_version",
                                            "datatype": "%%VARCHAR_20%%",
                                            "nullable": false,
                                            "primary_key": false,
                                            "unique": false
                                        },
                                        {
                                            "name": "ip_address",
                                            "datatype": "%%VARCHAR_50%%",
                                            "nullable": false,
                                            "primary_key": false,
                                            "unique": false
                                        },
                                        {
                                            "name": "valid_after",
                                            "datatype": "%%TIMESTAMP_TZ%%",
                                            "nullable": false,
                                            "primary_key": false,
                                            "unique": false
                                        },
                                        {
                                            "name": "valid_until",
                                            "datatype": "%%TIMESTAMP_TZ%%",
                                            "nullable": false,
                                            "primary_key": false,
                                            "unique": false
                                        }
                                    ]
                                }
                            ]=]
                            %%JSON_INGEST_END%%
                            ,
                            NULL,                               -- valid_after
                            NULL,                               -- valid_until
                            0,                                  -- created_id
                            %%TIMESTAMP%%,                      -- created_at
                            0,                                  -- updated_id
                            %%TIMESTAMP%%                       -- updated_at
                        );
                    ]]
        }
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --           
    }
}