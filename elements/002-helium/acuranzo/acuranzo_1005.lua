-- Migration: acuranzo_1005.lua
-- Creates the accounts table and populating it with the next migration.

-- CHANGELOG
-- 1.0.0 -0 2025-09-13 - Initial creation for accounts table with PostgreSQL support.

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
                            16,                                 -- query_id
                            1005,                               -- query_ref
                            %%TYPE_FORWARD_MIGRATIO%%,          -- query_type_lua_28    
                            %%DIALECT%%,                        -- query_dialect_lua_30    
                            'Create Accounts Table Query',      -- name, summary, query_code
                            [=[
                                # Forward Migration 1005: Create Accounts Table Query

                                This migration creates the accounts table for storing account data.
                            ]=],
                            [=[
                                CREATE TABLE IF NOT EXISTS app.accounts
                                (
                                    account_id integer NOT NULL,
                                    name character varying(100) COLLATE pg_catalog."default" NOT NULL,
                                    summary character varying(500) COLLATE pg_catalog."default",
                                    first_name character varying(50) COLLATE pg_catalog."default" NOT NULL,
                                    middle_name character varying(50) COLLATE pg_catalog."default" NOT NULL,
                                    last_name character varying(50) COLLATE pg_catalog."default" NOT NULL,
                                    password_hash character varying(128) COLLATE pg_catalog."default" NOT NULL,
                                    iana_timezone_lua_17 integer NOT NULL,
                                    status_lua_16 integer NOT NULL,
                                    collection jsonb,
                                    valid_after timestamp with time zone,
                                    valid_until timestamp with time zone,
                                    created_id integer NOT NULL,
                                    created_at timestamp with time zone NOT NULL,
                                    updated_id integer NOT NULL,
                                    updated_at timestamp with time zone NOT NULL,
                                    CONSTRAINT accounts_pkey PRIMARY KEY (account_id)
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
                            17,                                 -- query_id
                            1005,                               -- query_ref
                            %%TYPE_REVERSE_MIGRATIO%%,          -- query_type_lua_28    
                            %%DIALECT%%,                        -- query_dialect_lua_30    
                            'Delete Accounts Table Query',      -- name, summary, query_code
                            [=[
                                # Reverse Migration 1005: Delete Accounts Table Query

                                This is provided for completeness when testing the migration system
                                to ensure that forward and reverse migrations are complete.
                            ]=],
                            [=[
                                DROP TABLE app.accounts; 
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
                            18,                                 -- query_id
                            1005,                               -- query_ref
                            %%TYPE_DIAGRAM_MIGRATIO%%,          -- query_type_lua_28    
                            %%DIALECT%%,                        -- query_dialect_lua_30    
                            'Diagram Accounts Table Query',     -- name, summary
                            [=[
                                # Diagram Migration 1005: Diagram Accounts Table Query

                                This is the PlantUML code for the accounts table. 
                            ]=],
                            'PlantUML JSON in collection',      -- query_code, 
                            %%STATUS_ACTIVE%%,                  -- query_status_lua_27, collection
                            %%JSON_INGEST_START%%
                            [=[
                                {
                                    "object_type": "table",
                                    "object_id": "table.accounts",
                                    "plant_uml": 
                                    [==[
                                        @startuml
                                            entity accounts {
                                            * account_id : INTEGER [NOT NULL]
                                            * name : VARCHAR_100 [NOT NULL]
                                            summary : VARCHAR_500
                                            * first_name : VARCHAR_50 [NOT NULL]
                                            * middle_name : VARCHAR_50 [NOT NULL]
                                            * last_name : VARCHAR_50 [NOT NULL]
                                            * password_hash : VARCHAR_128 [NOT NULL]
                                            * iana_timezone_lua_17 : INTEGER [NOT NULL]
                                            * status_lua_16 : INTEGER [NOT NULL]
                                            collection : JSONB
                                            valid_after : TIMESTAMP_TZ
                                            valid_until : TIMESTAMP_TZ
                                            * created_id : INTEGER [NOT NULL]
                                            * created_at : TIMESTAMP_TZ [NOT NULL]
                                            * updated_id : INTEGER [NOT NULL]
                                            * updated_at : TIMESTAMP_TZ [NOT NULL]
                                            }
                                        @enduml
                                    ]==]
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