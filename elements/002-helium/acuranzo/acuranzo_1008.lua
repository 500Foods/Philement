-- Migration: acuranzo_1008.lua
-- Creates the convos table and populating it with the next migration.

-- CHANGELOG
-- 1.1.0 -0 2025-09-28 - Changed diagram query to use JSON table definition instead of PlantUML for custom ERD tool.
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
                                CREATE TABLE IF NOT EXISTS %%SCHEMA%%convos
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
                                DROP TABLE %%SCHEMA%%convos; 
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
                            27,                                 -- query_id
                            1008,                               -- query_ref
                            %%TYPE_DIAGRAM_MIGRATIO%%,          -- query_type_lua_28    
                            %%DIALECT%%,                        -- query_dialect_lua_30    
                            'Diagram Tables: %%SCHEMA%%convos', -- name, summary
                            [=[
                                # Diagram Migration 1008

                                ## Diagram Tables: %%SCHEMA%%convos

                                This is the first JSON Diagram code for the convos table.
                            ]=],
                            'JSON Table Definition in collection',     -- query_code,
                            %%STATUS_ACTIVE%%,                         -- query_status_lua_27, collection
                            %%JSON_INGEST_START%%                      -- DIAGRAM_START
                            [=[
                                [
                                    {
                                    "object_type": "table",
                                    "object_id": "table.convos",
                                    "object_id": "1008",
                                    "table": [
                                        {
                                            "name": "convos_id",
                                            "datatype": "%%INTEGER%%",
                                            "nullable": false,
                                            "primary_key": true,
                                            "unique": true
                                        },
                                        {
                                            "name": "convos_ref",
                                            "datatype": "%%VARCHAR_50%%",
                                            "nullable": false,
                                            "primary_key": false,
                                            "unique": true
                                        },
                                        {
                                            "name": "convos_keywords",
                                            "datatype": "%%TEXT%%",
                                            "nullable": true,
                                            "primary_key": false,
                                            "unique": false
                                        },
                                        {
                                            "name": "convos_icon",
                                            "datatype": "%%TEXT%%",
                                            "nullable": true,
                                            "primary_key": false,
                                            "unique": false
                                        },
                                        {
                                            "name": "prompt",
                                            "datatype": "%%TEXT%%",
                                            "nullable": true,
                                            "primary_key": false,
                                            "unique": false
                                        },
                                        {
                                            "name": "response",
                                            "datatype": "%%TEXT%%",
                                            "nullable": true,
                                            "primary_key": false,
                                            "unique": false
                                        },
                                        {
                                            "name": "context",
                                            "datatype": "%%TEXT%%",
                                            "nullable": true,
                                            "primary_key": false,
                                            "unique": false
                                        },
                                        {
                                            "name": "history",
                                            "datatype": "%%TEXT%%",
                                            "nullable": true,
                                            "primary_key": false,
                                            "unique": false
                                        },
                                        {
                                            "name": "collection",
                                            "datatype": "%%JSONB%%",
                                            "nullable": true,
                                            "primary_key": false,
                                            "unique": false,
                                            "standard": true
                                        },
                                        {
                                            "name": "valid_after",
                                            "datatype": "%%TIMESTAMP_TZ%%",
                                            "nullable": true,
                                            "primary_key": false,
                                            "unique": false,
                                            "standard": true
                                        },
                                        {
                                            "name": "valid_until",
                                            "datatype": "%%TIMESTAMP_TZ%%",
                                            "nullable": true,
                                            "primary_key": false,
                                            "unique": false,
                                            "standard": true
                                        },
                                        {
                                            "name": "created_id",
                                            "datatype": "%%INTEGER%%",
                                            "nullable": false,
                                            "primary_key": false,
                                            "unique": false,
                                            "standard": true
                                        },
                                        {
                                            "name": "created_at",
                                            "datatype": "%%TIMESTAMP_TZ%%",
                                            "nullable": false,
                                            "primary_key": false,
                                            "unique": false,
                                            "standard": true
                                        },
                                        {
                                            "name": "updated_id",
                                            "datatype": "%%INTEGER%%",
                                            "nullable": false,
                                            "primary_key": false,
                                            "unique": false,
                                            "standard": true
                                        },
                                        {
                                            "name": "updated_at",
                                            "datatype": "%%TIMESTAMP_TZ%%",
                                            "nullable": false,
                                            "primary_key": false,
                                            "unique": false,
                                            "standard": true
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
                            %%NOW%%,                            -- created_at
                            0,                                  -- updated_id
                            %%NOW%%                             -- updated_at
                        );
                    ]]
        }
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --           
    }
}