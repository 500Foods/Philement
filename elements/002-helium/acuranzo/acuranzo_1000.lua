-- Migration: acuranzo_1000.lua
-- Bootstraps the migration system by creating the queries table and populating it with the next migration.

-- CHANGELOG
-- 1.0.0 -0 2025-09-13 - Initial creation for queries table with SQLite, PostgreSQL, MySQL, DB2 support.

local config = require 'database'

return {
    queries = {
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --        
        {
            sql =   [[
                        CREATE TABLE %%SCHEMA%%queries (
                            query_id %%SERIAL%%,
                            query_ref %%INTEGER%% NOT NULL,
                            query_type_lua_28 %%INTEGER%% NOT NULL,
                            query_dialect_lua_30 %%INTEGER%% NOT NULL,
                            name %%VARCHAR_100%% NOT NULL,
                            summary %%TEXT%%,
                            query_code %%TEXT%% NOT NULL,
                            query_status_lua_27 %%INTEGER%% NOT NULL,
                            collection %%JSONB%%,
                            valid_after %%TIMESTAMP_TZ%%,
                            valid_until %%TIMESTAMP_TZ%%,
                            created_id %%INTEGER%% NOT NULL,
                            created_at %%TIMESTAMP_TZ%% NOT NULL,
                            updated_id %%INTEGER%% NOT NULL,
                            updated_at %%TIMESTAMP_TZ%% NOT NULL
                        );
                    ]]
        },
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --        
        {
            sql =   [[
                        %%JSON_INGEST_FUNCTION%%
                    ]]
        },
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --        
        {
            sql =   [[
                        INSERT INTO %%SCHEMA%%queries (
                            %%QUERY_INSERT_COLUMNS%%
                        )           
                        VALUES (
                            1,                                  -- query_id
                            1000,                               -- query_ref
                            %%TYPE_FORWARD_MIGRATIO%%,          -- query_type_lua_28    
                            %%DIALECT%%,                        -- query_dialect_lua_30    
                            'Create Tables Query',              -- name, summary, query_code
                            [=[
                                # Forward Migration 1000: Create Tables Query

                                This is the first migration that, technically, is run automatically 
                                when connecting to an empty database and kicks off the migration,
                                so long as the database has been configured with AutoMigration: true
                                in its config (this is the default if not supplied).
                            ]=],
                            [=[
                                CREATE TABLE %%SCHEMA%%queries (
                                    query_id %%SERIAL%%,
                                    query_ref %%INTEGER%% NOT NULL,
                                    query_type_lua_28 %%INTEGER%% NOT NULL,
                                    query_dialect_lua_30 %%INTEGER%% NOT NULL,
                                    name %%VARCHAR_100%% NOT NULL,
                                    summary %%TEXT%%,
                                    query_code %%TEXT%% NOT NULL,
                                    query_status_lua_27 %%INTEGER%% NOT NULL,
                                    collection %%JSONB%%,
                                    valid_after %%TIMESTAMP_TZ%%,
                                    valid_until %%TIMESTAMP_TZ%%,
                                    created_id %%INTEGER%% NOT NULL,
                                    created_at %%TIMESTAMP_TZ%% NOT NULL,
                                    updated_id %%INTEGER%% NOT NULL,
                                    updated_at %%TIMESTAMP_TZ%% NOT NULL
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
                            2,                                  -- query_id
                            1000,                               -- query_ref
                            %%TYPE_REVERSE_MIGRATIO%%,          -- query_type_lua_28    
                            %%DIALECT%%,                        -- query_dialect_lua_30    
                            'Delete Tables Query',              -- name, summary, query_code
                            [=[
                                # Reverse Migration 1000: Delete Tables Query

                                This is provided for completeness when testing the migration system
                                to ensure that forward and reverse migrations are complete.
                            ]=],
                            [=[
                                DROP TABLE %%SCHEMA%%queries; 
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
                            3,                                  -- query_id
                            1000,                               -- query_ref
                            %%TYPE_DIAGRAM_MIGRATIO%%,          -- query_type_lua_28    
                            %%DIALECT%%,                        -- query_dialect_lua_30    
                            'Diagram Tables Query',             -- name, summary
                            [=[
                                # Diagram Migration 1000: Diagram Tables Query

                                This is the first PlantUML code for the query table. 
                            ]=],
                            'PlantUML JSON in collection',      -- query_code, 
                            %%STATUS_ACTIVE%%,                  -- query_status_lua_27, collection
                            %%JSON_INGEST_START%%
                            [=[
                                {
                                    "object_type": "table",
                                    "object_id": "table.queries",
                                    "plant_uml": 
                                    [==[
                                    @startuml
                                    hide circle
                                    left to right direction

                                    entity queries {
                                        *   query_id                \t SERIAL
                                            query_ref               \t INTEGER      \t [NOT NULL]
                                            query_type_lua_28       \t INTEGER      \t [NOT NULL]
                                            query_dialect_lua_30    \t INTEGER      \t [NOT NULL]
                                            name                    \t VARCHAR_100  \t [NOT NULL]
                                            summary                 \t TEXT
                                            query_code              \t TEXT         \t [NOT NULL]
                                            query_status_lua_27     \t INTEGER      \t [NOT NULL]

                                            collection              \t JSONB

                                            valid_after             \t TIMESTAMP
                                            valid_until             \t TIMESTAMP
                                            created_id              \t INTEGER      \t [NOT NULL]
                                            created_at              \t TIMESTAMP    \t [NOT NULL]
                                            updated_id              \t INTEGER      \t [NOT NULL]
                                            updated_at              \t TIMESTAMP    \t [NOT NULL]
                                    }


                                        entity "Users" {
                                            * user_id     \t int      \t NOT NULL   \t <<PK>> 
                                            username    \t varchar  \t NOT NULL            
                                        }

                                        entity "Orders" {
                                            * order_id     \t int     \t NOT NULL    \t <<PK>> 
                                            * order_date   \t date    \t NOT NULL    \t <<PK>>
                                            other        \t string  
                                            misc         \t float   
                                            user_id      \t int     \t NOT NULL    \t <<FK>>
                                        }

                                        Users::user_id ||.up.o{ Orders::user_id : FK 

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