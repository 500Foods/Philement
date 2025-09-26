-- Migration: acuranzo_1008.lua
-- Creates the convos table and populating it with the next migration.

-- CHANGELOG
-- 1.0.0 -0 2025-09-13 - Initial creation for convos table with PostgreSQL support.

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
                            25,                                 -- query_id
                            1008,                               -- query_ref
                            %%TYPE_FORWARD_MIGRATIO%%,          -- query_type_lua_28    
                            %%DIALECT%%,                        -- query_dialect_lua_30    
                            'Create Convos Table Query',        -- name, summary, query_code
                            [=[
                                # Forward Migration 1008: Create Convos Table Query

                                This migration creates the convos table for storing conversation data.
                            ]=],
                            [=[
                                CREATE TABLE IF NOT EXISTS app.convos
                                (
                                    convos_id integer NOT NULL,
                                    convos_ref character(50) COLLATE pg_catalog."default" NOT NULL,
                                    convos_keywords text COLLATE pg_catalog."default",
                                    convos_icon text COLLATE pg_catalog."default",
                                    prompt text COLLATE pg_catalog."default",
                                    response text COLLATE pg_catalog."default",
                                    context text COLLATE pg_catalog."default",
                                    history text COLLATE pg_catalog."default",
                                    collection jsonb,
                                    valid_after timestamp with time zone,
                                    valid_until timestamp with time zone,
                                    created_id integer NOT NULL,
                                    created_at timestamp with time zone NOT NULL,
                                    updated_id integer NOT NULL,
                                    updated_at timestamp with time zone NOT NULL,
                                    CONSTRAINT convos_pkey PRIMARY KEY (convos_id)
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
                            26,                                 -- query_id
                            1008,                               -- query_ref
                            %%TYPE_REVERSE_MIGRATIO%%,          -- query_type_lua_28    
                            %%DIALECT%%,                        -- query_dialect_lua_30    
                            'Delete Convos Table Query',        -- name, summary, query_code
                            [=[
                                # Reverse Migration 1008: Delete Convos Table Query

                                This is provided for completeness when testing the migration system
                                to ensure that forward and reverse migrations are complete.
                            ]=],
                            [=[
                                DROP TABLE app.convos; 
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
                            27,                                 -- query_id
                            1008,                               -- query_ref
                            %%TYPE_DIAGRAM_MIGRATIO%%,          -- query_type_lua_28    
                            %%DIALECT%%,                        -- query_dialect_lua_30    
                            'Diagram Convos Table Query',       -- name, summary
                            [=[
                                # Diagram Migration 1008: Diagram Convos Table Query

                                This is the PlantUML code for the convos table. 
                            ]=],
                            'PlantUML JSON in collection',      -- query_code, 
                            %%STATUS_ACTIVE%%,                  -- query_status_lua_27, collection
                            %%JSON_INGEST_START%%
                            [=[
                                {
                                    "object_type": "table",
                                    "object_id": "table.convos",
                                    "plant_uml": 
                                    [==[
                                        @startuml
                                            entity convos {
                                            * convos_id : INTEGER [NOT NULL]
                                            * convos_ref : CHAR_50 [NOT NULL]
                                            convos_keywords : TEXT
                                            convos_icon : TEXT
                                            prompt : TEXT
                                            response : TEXT
                                            context : TEXT
                                            history : TEXT
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