-- Migration: acuranzo_1010.lua
-- Creates the documents table and populating it with the next migration.

-- CHANGELOG
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
                            'Diagram Documents Table Query',    -- name, summary
                            [=[
                                # Diagram Migration 1010: Diagram Documents Table Query

                                This is the PlantUML code for the documents table. 
                            ]=],
                            'PlantUML JSON in collection',      -- query_code, 
                            %%STATUS_ACTIVE%%,                  -- query_status_lua_27, collection
                            %%JSON_INGEST_START%%
                            [=[
                                {
                                    "object_type": "table",
                                    "object_id": "table.documents",
                                    "plant_uml": 
                                    [==[
                                        @startuml
                                            entity documents {
                                            * doc_id : INTEGER [NOT NULL]
                                            * rev_id : INTEGER [NOT NULL]
                                            * doc_library_lua_49 : INTEGER [NOT NULL]
                                            * doc_status_lua_50 : INTEGER [NOT NULL]
                                            * name : VARCHAR_100 [NOT NULL]
                                            summary : TEXT
                                            collection : JSONB
                                            valid_after : TIMESTAMP_TZ
                                            valid_until : TIMESTAMP_TZ
                                            * created_id : INTEGER [NOT NULL]
                                            * created_at : TIMESTAMP_TZ [NOT NULL]
                                            * updated_id : INTEGER [NOT NULL]
                                            * updated_at : TIMESTAMP_TZ [NOT NULL]
                                            doc_type_lua_51 : INTEGER
                                            file_preview : TEXT
                                            file_name : TEXT
                                            file_data : TEXT
                                            file_text : TEXT
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