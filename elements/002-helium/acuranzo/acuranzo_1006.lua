-- Migration: acuranzo_1006.lua
-- Creates the actions table and populating it with the next migration.

-- CHANGELOG
-- 1.0.0 -0 2025-09-13 - Initial creation for actions table with PostgreSQL support.

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
                            19,                                 -- query_id
                            1006,                               -- query_ref
                            %%TYPE_FORWARD_MIGRATIO%%,          -- query_type_lua_28    
                            %%DIALECT%%,                        -- query_dialect_lua_30    
                            'Create Actions Table Query',       -- name, summary, query_code
                            [=[
                                # Forward Migration 1006: Create Actions Table Query

                                This migration creates the actions table for storing action data.
                            ]=],
                            [=[
                                CREATE TABLE IF NOT EXISTS app.actions
                                (
                                    action_id integer NOT NULL DEFAULT nextval('app.actions_action_id_seq'::regclass),
                                    action_type_lua_24 integer NOT NULL,
                                    system_id integer,
                                    application_id integer,
                                    application_version character varying(20) COLLATE pg_catalog."default",
                                    account_id integer,
                                    feature_lua_21 integer NOT NULL,
                                    action character varying(500) COLLATE pg_catalog."default",
                                    action_msecs integer NOT NULL,
                                    ip_address character varying(50) COLLATE pg_catalog."default",
                                    created_id integer NOT NULL,
                                    created_at timestamp with time zone NOT NULL
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
                            20,                                 -- query_id
                            1006,                               -- query_ref
                            %%TYPE_REVERSE_MIGRATIO%%,          -- query_type_lua_28    
                            %%DIALECT%%,                        -- query_dialect_lua_30    
                            'Delete Actions Table Query',       -- name, summary, query_code
                            [=[
                                # Reverse Migration 1006: Delete Actions Table Query

                                This is provided for completeness when testing the migration system
                                to ensure that forward and reverse migrations are complete.
                            ]=],
                            [=[
                                DROP TABLE app.actions; 
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
                            21,                                 -- query_id
                            1006,                               -- query_ref
                            %%TYPE_DIAGRAM_MIGRATIO%%,          -- query_type_lua_28    
                            %%DIALECT%%,                        -- query_dialect_lua_30    
                            'Diagram Actions Table Query',      -- name, summary
                            [=[
                                # Diagram Migration 1006: Diagram Actions Table Query

                                This is the PlantUML code for the actions table. 
                            ]=],
                            'PlantUML JSON in collection',      -- query_code, 
                            %%STATUS_ACTIVE%%,                  -- query_status_lua_27, collection
                            %%JSON_INGEST_START%%
                            [=[
                                {
                                    "object_type": "table",
                                    "object_id": "table.actions",
                                    "plant_uml": 
                                    [==[
                                        @startuml
                                            entity actions {
                                            action_id : INTEGER [NOT NULL]
                                            * action_type_lua_24 : INTEGER [NOT NULL]
                                            system_id : INTEGER
                                            application_id : INTEGER
                                            application_version : VARCHAR_20
                                            account_id : INTEGER
                                            * feature_lua_21 : INTEGER [NOT NULL]
                                            action : VARCHAR_500
                                            * action_msecs : INTEGER [NOT NULL]
                                            ip_address : VARCHAR_50
                                            * created_id : INTEGER [NOT NULL]
                                            * created_at : TIMESTAMP_TZ [NOT NULL]
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