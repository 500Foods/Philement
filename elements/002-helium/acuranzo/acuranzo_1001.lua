-- Migration: acuranzo_1001.lua
-- Creates the lookups table and populating it with the next migration.

-- CHANGELOG
-- 1.0.0 -0 2025-09-13 - Initial creation for lookups table with PostgreSQL support.

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
                            4,                                  -- query_id
                            1001,                               -- query_ref
                            %%TYPE_FORWARD_MIGRATIO%%,          -- query_type_lua_28    
                            %%DIALECT%%,                        -- query_dialect_lua_30    
                            'Create Lookups Table Query',       -- name, summary, query_code
                            [=[
                                # Forward Migration 1001: Create Lookups Table Query

                                This migration creates the lookups table for storing key-value lookup data.
                            ]=],
                            [=[
                                CREATE TABLE IF NOT EXISTS app.lookups
                                (
                                    lookup_id integer NOT NULL,
                                    key_idx integer NOT NULL,
                                    value_txt character varying(100) COLLATE pg_catalog."default",
                                    value_int integer,
                                    sort_seq integer NOT NULL,
                                    status_lua_1 integer NOT NULL,
                                    collection jsonb,
                                    valid_after timestamp with time zone,
                                    valid_until timestamp with time zone,
                                    created_id integer NOT NULL,
                                    created_at timestamp with time zone NOT NULL,
                                    updated_id integer NOT NULL,
                                    updated_at timestamp with time zone NOT NULL,
                                    summary text COLLATE pg_catalog."default",
                                    code text COLLATE pg_catalog."default",
                                    CONSTRAINT lookups_pkey PRIMARY KEY (lookup_id, key_idx)
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
                            5,                                  -- query_id
                            1001,                               -- query_ref
                            %%TYPE_REVERSE_MIGRATIO%%,          -- query_type_lua_28    
                            %%DIALECT%%,                        -- query_dialect_lua_30    
                            'Delete Lookups Table Query',       -- name, summary, query_code
                            [=[
                                # Reverse Migration 1001: Delete Lookups Table Query

                                This is provided for completeness when testing the migration system
                                to ensure that forward and reverse migrations are complete.
                            ]=],
                            [=[
                                DROP TABLE app.lookups; 
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
                            6,                                  -- query_id
                            1001,                               -- query_ref
                            %%TYPE_DIAGRAM_MIGRATIO%%,          -- query_type_lua_28    
                            %%DIALECT%%,                        -- query_dialect_lua_30    
                            'Diagram Lookups Table Query',      -- name, summary
                            [=[
                                # Diagram Migration 1001: Diagram Lookups Table Query

                                This is the PlantUML code for the lookups table. 
                            ]=],
                            'PlantUML JSON in collection',      -- query_code, 
                            %%STATUS_ACTIVE%%,                  -- query_status_lua_27, collection
                            %%JSON_INGEST_START%%
                            [=[
                                {
                                    "object_type": "table",
                                    "object_id": "table.lookups",
                                    "plant_uml": 
                                    [==[
                                        @startuml
                                            entity lookups {
                                            * lookup_id : INTEGER [NOT NULL]
                                            * key_idx : INTEGER [NOT NULL]
                                            value_txt : VARCHAR_100
                                            value_int : INTEGER
                                            * sort_seq : INTEGER [NOT NULL]
                                            * status_lua_1 : INTEGER [NOT NULL]
                                            collection : JSONB
                                            valid_after : TIMESTAMP_TZ
                                            valid_until : TIMESTAMP_TZ
                                            * created_id : INTEGER [NOT NULL]
                                            * created_at : TIMESTAMP_TZ [NOT NULL]
                                            * updated_id : INTEGER [NOT NULL]
                                            * updated_at : TIMESTAMP_TZ [NOT NULL]
                                            summary : TEXT
                                            code : TEXT
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