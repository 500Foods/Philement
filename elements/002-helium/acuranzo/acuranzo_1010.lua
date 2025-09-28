-- Migration: acuranzo_1010.lua
-- Creates the documents table and populating it with the next migration.

-- CHANGELOG
-- 1.1.0 -0 2025-09-28 - Changed diagram query to use JSON table definition instead of PlantUML for custom ERD tool.
-- 1.0.0 -0 2025-09-13 - Initial creation for documents table with PostgreSQL support.

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
                            31,                                 -- query_id
                            1010,                               -- query_ref
                            %%TYPE_FORWARD_MIGRATIO%%,          -- query_type_lua_28    
                            %%DIALECT%%,                        -- query_dialect_lua_30    
                            'Create Documents Table Query',     -- name, summary, query_code
                            [=[
                                # Forward Migration 1010: Create Documents Table Query

                                This migration creates the documents table for storing document data.
                            ]=],
                            [=[
                                CREATE TABLE IF NOT EXISTS app.documents
                                (
                                    doc_id integer NOT NULL,
                                    rev_id integer NOT NULL,
                                    doc_library_lua_49 integer NOT NULL,
                                    doc_status_lua_50 integer NOT NULL,
                                    name character varying(100) COLLATE pg_catalog."default" NOT NULL,
                                    summary text COLLATE pg_catalog."default",
                                    collection jsonb,
                                    valid_after timestamp with time zone,
                                    valid_until timestamp with time zone,
                                    created_id integer NOT NULL,
                                    created_at timestamp with time zone NOT NULL,
                                    updated_id integer NOT NULL,
                                    updated_at timestamp with time zone NOT NULL,
                                    doc_type_lua_51 integer,
                                    file_preview text COLLATE pg_catalog."default",
                                    file_name text COLLATE pg_catalog."default",
                                    file_data text COLLATE pg_catalog."default",
                                    file_text text COLLATE pg_catalog."default",
                                    CONSTRAINT documents_pkey PRIMARY KEY (doc_id, rev_id),
                                    CONSTRAINT documents_doc_id_rev_id_name_key UNIQUE (doc_id, rev_id, name)
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
                            32,                                 -- query_id
                            1010,                               -- query_ref
                            %%TYPE_REVERSE_MIGRATIO%%,          -- query_type_lua_28    
                            %%DIALECT%%,                        -- query_dialect_lua_30    
                            'Delete Documents Table Query',     -- name, summary, query_code
                            [=[
                                # Reverse Migration 1010: Delete Documents Table Query

                                This is provided for completeness when testing the migration system
                                to ensure that forward and reverse migrations are complete.
                            ]=],
                            [=[
                                DROP TABLE app.documents; 
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
                            33,                                 -- query_id
                            1010,                               -- query_ref
                            %%TYPE_DIAGRAM_MIGRATIO%%,          -- query_type_lua_28    
                            %%DIALECT%%,                        -- query_dialect_lua_30    
                            'Diagram Tables: app.documents',    -- name, summary
                            [=[
                                # Diagram Migration 1010

                                ## Diagram Tables: app.documents

                                This is the first JSON Diagram code for the documents table.
                            ]=],
                            'JSON Table Definition in collection',     -- query_code,
                            %%STATUS_ACTIVE%%,                  -- query_status_lua_27, collection
                            %%JSON_INGEST_START%%
                            [=[
                                {
                                    "object_type": "table",
                                    "object_id": "table.documents",
                                    "table": [
                                        {
                                            "name": "doc_id",
                                            "datatype": "%%INTEGER%%",
                                            "nullable": false,
                                            "primary_key": true,
                                            "unique": false
                                        },
                                        {
                                            "name": "rev_id",
                                            "datatype": "%%INTEGER%%",
                                            "nullable": false,
                                            "primary_key": true,
                                            "unique": false
                                        },
                                        {
                                            "name": "doc_library_lua_49",
                                            "datatype": "%%INTEGER%%",
                                            "nullable": false,
                                            "primary_key": false,
                                            "unique": false
                                        },
                                        {
                                            "name": "doc_status_lua_50",
                                            "datatype": "%%INTEGER%%",
                                            "nullable": false,
                                            "primary_key": false,
                                            "unique": false
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
                                        },
                                        {
                                            "name": "doc_type_lua_51",
                                            "datatype": "%%INTEGER%%",
                                            "nullable": true,
                                            "primary_key": false,
                                            "unique": false
                                        },
                                        {
                                            "name": "file_preview",
                                            "datatype": "%%TEXT%%",
                                            "nullable": true,
                                            "primary_key": false,
                                            "unique": false
                                        },
                                        {
                                            "name": "file_name",
                                            "datatype": "%%TEXT%%",
                                            "nullable": true,
                                            "primary_key": false,
                                            "unique": false
                                        },
                                        {
                                            "name": "file_data",
                                            "datatype": "%%TEXT%%",
                                            "nullable": true,
                                            "primary_key": false,
                                            "unique": false
                                        },
                                        {
                                            "name": "file_text",
                                            "datatype": "%%TEXT%%",
                                            "nullable": true,
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