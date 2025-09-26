-- Migration: acuranzo_1019.lua
-- Creates the systems table and populating it with the next migration.

-- CHANGELOG
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
                                CREATE TABLE IF NOT EXISTS app.systems
                                (
                                    system_id integer NOT NULL,
                                    name character varying(100) COLLATE pg_catalog."default" NOT NULL,
                                    summary character varying(500) COLLATE pg_catalog."default",
                                    access_type_lua_15 integer NOT NULL,
                                    status_lua_14 integer NOT NULL,
                                    collection jsonb,
                                    valid_after timestamp with time zone,
                                    valid_until timestamp with time zone,
                                    created_id integer NOT NULL,
                                    created_at timestamp with time zone NOT NULL,
                                    updated_id integer NOT NULL,
                                    updated_at timestamp with time zone NOT NULL,
                                    CONSTRAINT systems_pkey PRIMARY KEY (system_id)
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
                                DROP TABLE app.systems; 
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
                            60,                                 -- query_id
                            1019,                               -- query_ref
                            %%TYPE_DIAGRAM_MIGRATIO%%,          -- query_type_lua_28    
                            %%DIALECT%%,                        -- query_dialect_lua_30    
                            'Diagram Systems Table Query',      -- name, summary
                            [=[
                                # Diagram Migration 1019: Diagram Systems Table Query

                                This is the PlantUML code for the systems table. 
                            ]=],
                            'PlantUML JSON in collection',      -- query_code, 
                            %%STATUS_ACTIVE%%,                  -- query_status_lua_27, collection
                            %%JSON_INGEST_START%%
                            [=[
                                {
                                    "object_type": "table",
                                    "object_id": "table.systems",
                                    "plant_uml": 
                                    [==[
                                        @startuml
                                            entity systems {
                                            * system_id : INTEGER [NOT NULL]
                                            * name : VARCHAR_100 [NOT NULL]
                                            summary : VARCHAR_500
                                            * access_type_lua_15 : INTEGER [NOT NULL]
                                            * status_lua_14 : INTEGER [NOT NULL]
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