-- Migration: acuranzo_1013.lua
-- Creates the lists table and populating it with the next migration.

-- CHANGELOG
-- 1.0.0 -0 2025-09-13 - Initial creation for lists table with PostgreSQL support.

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
                            40,                                 -- query_id
                            1013,                               -- query_ref
                            %%TYPE_FORWARD_MIGRATIO%%,          -- query_type_lua_28    
                            %%DIALECT%%,                        -- query_dialect_lua_30    
                            'Create Lists Table Query',         -- name, summary, query_code
                            [=[
                                # Forward Migration 1013: Create Lists Table Query

                                This migration creates the lists table for storing list data.
                            ]=],
                            [=[
                                CREATE TABLE IF NOT EXISTS app.lists
                                (
                                    list_type_lua_31 integer NOT NULL,
                                    list_key integer,
                                    list_value character varying(100) COLLATE pg_catalog."default",
                                    list_note character varying(100) COLLATE pg_catalog."default",
                                    status_lua_32 integer NOT NULL,
                                    collection jsonb,
                                    valid_after timestamp with time zone,
                                    valid_until timestamp with time zone,
                                    created_id integer NOT NULL,
                                    created_at timestamp with time zone NOT NULL,
                                    updated_id integer NOT NULL,
                                    updated_at timestamp with time zone NOT NULL
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
                            41,                                 -- query_id
                            1013,                               -- query_ref
                            %%TYPE_REVERSE_MIGRATIO%%,          -- query_type_lua_28    
                            %%DIALECT%%,                        -- query_dialect_lua_30    
                            'Delete Lists Table Query',         -- name, summary, query_code
                            [=[
                                # Reverse Migration 1013: Delete Lists Table Query

                                This is provided for completeness when testing the migration system
                                to ensure that forward and reverse migrations are complete.
                            ]=],
                            [=[
                                DROP TABLE app.lists; 
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
                            42,                                 -- query_id
                            1013,                               -- query_ref
                            %%TYPE_DIAGRAM_MIGRATIO%%,          -- query_type_lua_28    
                            %%DIALECT%%,                        -- query_dialect_lua_30    
                            'Diagram Lists Table Query',        -- name, summary
                            [=[
                                # Diagram Migration 1013: Diagram Lists Table Query

                                This is the PlantUML code for the lists table. 
                            ]=],
                            'PlantUML JSON in collection',      -- query_code, 
                            %%STATUS_ACTIVE%%,                  -- query_status_lua_27, collection
                            %%JSON_INGEST_START%%
                            [=[
                                {
                                    "object_type": "table",
                                    "object_id": "table.lists",
                                    "plant_uml": 
                                    [==[
                                        @startuml
                                            entity lists {
                                            * list_type_lua_31 : INTEGER [NOT NULL]
                                            list_key : INTEGER
                                            list_value : VARCHAR_100
                                            list_note : VARCHAR_100
                                            * status_lua_32 : INTEGER [NOT NULL]
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