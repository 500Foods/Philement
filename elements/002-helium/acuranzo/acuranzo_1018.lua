-- Migration: acuranzo_1018.lua
-- Creates the sessions table and populating it with the next migration.

-- CHANGELOG
-- 1.0.0 -0 2025-09-13 - Initial creation for sessions table with PostgreSQL support.

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
                            55,                                 -- query_id
                            1018,                               -- query_ref
                            %%TYPE_FORWARD_MIGRATIO%%,          -- query_type_lua_28    
                            %%DIALECT%%,                        -- query_dialect_lua_30    
                            'Create Sessions Table Query',      -- name, summary, query_code
                            [=[
                                # Forward Migration 1018: Create Sessions Table Query

                                This migration creates the sessions table for storing session data.
                            ]=],
                            [=[
                                CREATE TABLE IF NOT EXISTS app.sessions
                                (
                                    session_id text COLLATE pg_catalog."default" NOT NULL,
                                    account_id integer NOT NULL,
                                    session text COLLATE pg_catalog."default" NOT NULL,
                                    session_length integer NOT NULL,
                                    session_issues integer NOT NULL,
                                    session_secs integer NOT NULL,
                                    status_lua_25 integer NOT NULL,
                                    flag_lua_26 integer NOT NULL,
                                    created_at timestamp with time zone NOT NULL,
                                    updated_at timestamp with time zone NOT NULL,
                                    session_changes integer
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
                            56,                                 -- query_id
                            1018,                               -- query_ref
                            %%TYPE_REVERSE_MIGRATIO%%,          -- query_type_lua_28    
                            %%DIALECT%%,                        -- query_dialect_lua_30    
                            'Delete Sessions Table Query',      -- name, summary, query_code
                            [=[
                                # Reverse Migration 1018: Delete Sessions Table Query

                                This is provided for completeness when testing the migration system
                                to ensure that forward and reverse migrations are complete.
                            ]=],
                            [=[
                                DROP TABLE app.sessions; 
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
                            57,                                 -- query_id
                            1018,                               -- query_ref
                            %%TYPE_DIAGRAM_MIGRATIO%%,          -- query_type_lua_28    
                            %%DIALECT%%,                        -- query_dialect_lua_30    
                            'Diagram Sessions Table Query',     -- name, summary
                            [=[
                                # Diagram Migration 1018: Diagram Sessions Table Query

                                This is the PlantUML code for the sessions table. 
                            ]=],
                            'PlantUML JSON in collection',      -- query_code, 
                            %%STATUS_ACTIVE%%,                  -- query_status_lua_27, collection
                            %%JSON_INGEST_START%%
                            [=[
                                {
                                    "object_type": "table",
                                    "object_id": "table.sessions",
                                    "plant_uml": 
                                    [==[
                                        @startuml
                                            entity sessions {
                                            * session_id : TEXT [NOT NULL]
                                            * account_id : INTEGER [NOT NULL]
                                            * session : TEXT [NOT NULL]
                                            * session_length : INTEGER [NOT NULL]
                                            * session_issues : INTEGER [NOT NULL]
                                            * session_secs : INTEGER [NOT NULL]
                                            * status_lua_25 : INTEGER [NOT NULL]
                                            * flag_lua_26 : INTEGER [NOT NULL]
                                            * created_at : TIMESTAMP_TZ [NOT NULL]
                                            * updated_at : TIMESTAMP_TZ [NOT NULL]
                                            session_changes : INTEGER
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