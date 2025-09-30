-- Migration: acuranzo_1001.lua
-- Creates the lookups table and populating it with the next migration.

-- CHANGELOG
-- 1.1.0 -0 2025-09-28 - Changed diagram query to use JSON table definition instead of PlantUML for custom ERD tool.
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
                                CREATE TABLE IF NOT EXISTS %%SCHEMA%%lookups
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
                            %%NOW%%,                            -- created_at
                            0,                                  -- updated_id
                            %%NOW%%                             -- updated_at
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
                                DROP TABLE %%SCHEMA%%lookups; 
                            ]=],
                            %%STATUS_ACTIVE%%,                  -- query_status_lua_27
                            NULL,                               -- collection
                            NULL,                               -- valid_after
                            NULL,                               -- valid_until
                            0,                                  -- created_id
                            %%NOW%%,                            -- created_at
                            0,                                  -- updated_id
                            %%NOW%%                             -- updated_at
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
                            6,                                          -- query_id
                            1001,                                       -- query_ref
                            %%TYPE_DIAGRAM_MIGRATIO%%,                  -- query_type_lua_28    
                            %%DIALECT%%,                                -- query_dialect_lua_30    
                            'Diagram Tables: %%SCHEMA%%lookups',        -- name, summary
                            [=[
                                # Diagram Migration 1001

                                ## Diagram Tables: %%SCHEMA%%lookups

                                This is the first JSON Diagram code for the lookups table.
                            ]=],
                            'JSON Table Definition in collection',      -- query_code,
                            %%STATUS_ACTIVE%%,                          -- query_status_lua_27, collection
                            %%JSON_INGEST_START%%                       -- DIAGRAM_START
                            [=[
                                [
                                    {
                                        "object_type": "table",
                                        "object_id": "table.lookups",
                                        "object_ref": "1001",
                                        "table": [
                                            {
                                                "name": "lookup_id",
                                                "datatype": "%%INTEGER%%",
                                                "nullable": false,
                                                "primary_key": true,
                                                "unique": false
                                            },
                                            {
                                                "name": "key_idx",
                                                "datatype": "%%INTEGER%%",
                                                "nullable": false,
                                                "primary_key": true,
                                                "unique": false
                                            },
                                            {
                                                "name": "value_txt",
                                                "datatype": "%%VARCHAR_100%%",
                                                "nullable": true,
                                                "primary_key": false,
                                                "unique": false
                                            },
                                            {
                                                "name": "value_int",
                                                "datatype": "%%INTEGER%%",
                                                "nullable": true,
                                                "primary_key": false,
                                                "unique": false
                                            },
                                            {
                                                "name": "sort_seq",
                                                "datatype": "%%INTEGER%%",
                                                "nullable": false,
                                                "primary_key": false,
                                                "unique": false
                                            },
                                            {
                                                "name": "status_lua_1",
                                                "datatype": "%%INTEGER%%",
                                                "nullable": false,
                                                "primary_key": false,
                                                "unique": false,
                                                "lookup": true
                                            },
                                            {
                                                "name": "summary",
                                                "datatype": "%%TEXT%%",
                                                "nullable": true,
                                                "primary_key": false,
                                                "unique": false
                                            },
                                            {
                                                "name": "code",
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