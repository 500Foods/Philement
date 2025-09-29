-- Migration: acuranzo_1006.lua
-- Creates the actions table and populating it with the next migration.

-- CHANGELOG
-- 1.1.0 -0 2025-09-28 - Changed diagram query to use JSON table definition instead of PlantUML for custom ERD tool.
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
                                CREATE TABLE IF NOT EXISTS %%SCHEMA%%actions
                                (
                                    action_id integer NOT NULL DEFAULT nextval('%%SCHEMA%%actions_action_id_seq'::regclass),
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
                            %%NOW%%,                      -- created_at
                            0,                                  -- updated_id
                            %%NOW%%                       -- updated_at
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
                                DROP TABLE %%SCHEMA%%actions; 
                            ]=],
                            %%STATUS_ACTIVE%%,                  -- query_status_lua_27
                            NULL,                               -- collection
                            NULL,                               -- valid_after
                            NULL,                               -- valid_until
                            0,                                  -- created_id
                            %%NOW%%,                      -- created_at
                            0,                                  -- updated_id
                            %%NOW%%                       -- updated_at
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
                            'Diagram Tables: %%SCHEMA%%actions',      -- name, summary
                            [=[
                                # Diagram Migration 1006

                                ## Diagram Tables: %%SCHEMA%%actions

                                This is the first JSON Diagram code for the actions table.
                            ]=],
                            'JSON Table Definition in collection',     -- query_code,
                            %%STATUS_ACTIVE%%,                  -- query_status_lua_27, collection
                            %%JSON_INGEST_START%%               -- DIAGRAM_START
                            [=[
                                [
                                    {
                                    "object_type": "table",
                                    "object_id": "table.actions",
                                    "table": [
                                        {
                                            "name": "action_id",
                                            "datatype": "%%INTEGER%%",
                                            "nullable": false,
                                            "primary_key": false,
                                            "unique": false
                                        },
                                        {
                                            "name": "action_type_lua_24",
                                            "datatype": "%%INTEGER%%",
                                            "nullable": false,
                                            "primary_key": false,
                                            "unique": false
                                        },
                                        {
                                            "name": "system_id",
                                            "datatype": "%%INTEGER%%",
                                            "nullable": true,
                                            "primary_key": false,
                                            "unique": false
                                        },
                                        {
                                            "name": "application_id",
                                            "datatype": "%%INTEGER%%",
                                            "nullable": true,
                                            "primary_key": false,
                                            "unique": false
                                        },
                                        {
                                            "name": "application_version",
                                            "datatype": "%%VARCHAR_20%%",
                                            "nullable": true,
                                            "primary_key": false,
                                            "unique": false
                                        },
                                        {
                                            "name": "account_id",
                                            "datatype": "%%INTEGER%%",
                                            "nullable": true,
                                            "primary_key": false,
                                            "unique": false
                                        },
                                        {
                                            "name": "feature_lua_21",
                                            "datatype": "%%INTEGER%%",
                                            "nullable": false,
                                            "primary_key": false,
                                            "unique": false
                                        },
                                        {
                                            "name": "action",
                                            "datatype": "%%VARCHAR_500%%",
                                            "nullable": true,
                                            "primary_key": false,
                                            "unique": false
                                        },
                                        {
                                            "name": "action_msecs",
                                            "datatype": "%%INTEGER%%",
                                            "nullable": false,
                                            "primary_key": false,
                                            "unique": false
                                        },
                                        {
                                            "name": "ip_address",
                                            "datatype": "%%VARCHAR_50%%",
                                            "nullable": true,
                                            "primary_key": false,
                                            "unique": false
                                        },
                                        {
                                            "name": "created_id",
                                            "datatype": "%%INTEGER%%",
                                            "nullable": false,
                                            "primary_key": false,
                                            "unique": false
                                        },
                                        {
                                            "name": "created_at",
                                            "datatype": "%%TIMESTAMP_TZ%%",
                                            "nullable": false,
                                            "primary_key": false,
                                            "unique": false
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
                            %%NOW%%,                      -- created_at
                            0,                                  -- updated_id
                            %%NOW%%                       -- updated_at
                        );
                    ]]
        }
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --           
    }
}