-- Migration: acuranzo_1020.lua
-- Creates the templates table and populating it with the next migration.

-- CHANGELOG
-- 1.1.0 -0 2025-09-28 - Changed diagram query to use JSON table definition instead of PlantUML for custom ERD tool.
-- 1.0.0 -0 2025-09-13 - Initial creation for templates table with PostgreSQL support.

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
                            61,                                 -- query_id
                            1020,                               -- query_ref
                            %%TYPE_FORWARD_MIGRATIO%%,          -- query_type_lua_28    
                            %%DIALECT%%,                        -- query_dialect_lua_30    
                            'Create Templates Table Query',     -- name, summary, query_code
                            [=[
                                # Forward Migration 1020: Create Templates Table Query

                                This migration creates the templates table for storing template data.
                            ]=],
                            [=[
                                CREATE TABLE IF NOT EXISTS app.templates
                                (
                                    template_id integer NOT NULL,
                                    name character varying(100) COLLATE pg_catalog."default" NOT NULL,
                                    summary character varying(500) COLLATE pg_catalog."default",
                                    entity_id integer NOT NULL,
                                    entity_type_lua_9 integer NOT NULL,
                                    status_lua_12 integer NOT NULL,
                                    collection jsonb,
                                    valid_after timestamp with time zone,
                                    valid_until timestamp with time zone,
                                    created_id integer NOT NULL,
                                    created_at timestamp with time zone NOT NULL,
                                    updated_id integer NOT NULL,
                                    updated_at timestamp with time zone NOT NULL,
                                    CONSTRAINT templates_pkey PRIMARY KEY (template_id)
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
                            62,                                 -- query_id
                            1020,                               -- query_ref
                            %%TYPE_REVERSE_MIGRATIO%%,          -- query_type_lua_28    
                            %%DIALECT%%,                        -- query_dialect_lua_30    
                            'Delete Templates Table Query',     -- name, summary, query_code
                            [=[
                                # Reverse Migration 1020: Delete Templates Table Query

                                This is provided for completeness when testing the migration system
                                to ensure that forward and reverse migrations are complete.
                            ]=],
                            [=[
                                DROP TABLE app.templates; 
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
                            63,                                 -- query_id
                            1020,                               -- query_ref
                            %%TYPE_DIAGRAM_MIGRATIO%%,          -- query_type_lua_28    
                            %%DIALECT%%,                        -- query_dialect_lua_30    
                            'Diagram Tables: app.templates',    -- name, summary
                            [=[
                                # Diagram Migration 1020

                                ## Diagram Tables: app.templates

                                This is the first JSON Diagram code for the templates table.
                            ]=],
                            'JSON Table Definition in collection',     -- query_code,
                            %%STATUS_ACTIVE%%,                  -- query_status_lua_27, collection
                            %%JSON_INGEST_START%%
                            [=[
                                {
                                    "object_type": "table",
                                    "object_id": "table.templates",
                                    "table": [
                                        {
                                            "name": "template_id",
                                            "datatype": "%%INTEGER%%",
                                            "nullable": false,
                                            "primary_key": true,
                                            "unique": true
                                        },
                                        {
                                            "name": "name",
                                            "datatype": "%%VARCHAR_100%%",
                                            "nullable": false,
                                            "primary_key": false,
                                            "unique": false
                                        },
                                        {
                                            "name": "summary",
                                            "datatype": "%%VARCHAR_500%%",
                                            "nullable": true,
                                            "primary_key": false,
                                            "unique": false
                                        },
                                        {
                                            "name": "entity_id",
                                            "datatype": "%%INTEGER%%",
                                            "nullable": false,
                                            "primary_key": false,
                                            "unique": false
                                        },
                                        {
                                            "name": "entity_type_lua_9",
                                            "datatype": "%%INTEGER%%",
                                            "nullable": false,
                                            "primary_key": false,
                                            "unique": false
                                        },
                                        {
                                            "name": "status_lua_12",
                                            "datatype": "%%INTEGER%%",
                                            "nullable": false,
                                            "primary_key": false,
                                            "unique": false
                                        },
                                        {
                                            "name": "collection",
                                            "datatype": "%%JSONB%%",
                                            "nullable": true,
                                            "primary_key": false,
                                            "unique": false
                                        },
                                        {
                                            "name": "valid_after",
                                            "datatype": "%%TIMESTAMP_TZ%%",
                                            "nullable": true,
                                            "primary_key": false,
                                            "unique": false
                                        },
                                        {
                                            "name": "valid_until",
                                            "datatype": "%%TIMESTAMP_TZ%%",
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
                                        },
                                        {
                                            "name": "updated_id",
                                            "datatype": "%%INTEGER%%",
                                            "nullable": false,
                                            "primary_key": false,
                                            "unique": false
                                        },
                                        {
                                            "name": "updated_at",
                                            "datatype": "%%TIMESTAMP_TZ%%",
                                            "nullable": false,
                                            "primary_key": false,
                                            "unique": false
                                        }
                                    ]
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