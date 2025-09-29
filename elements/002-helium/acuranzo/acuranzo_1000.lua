-- Migration: acuranzo_1000.lua
-- Bootstraps the migration system by creating the queries table and populating it with the next migration.

-- CHANGELOG
-- 1.1.0 -0 2025-09-28 - Changed diagram query to use JSON table definition instead of PlantUML for custom ERD tool.
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
                            3,                                   -- query_id
                            1000,                                -- query_ref
                            %%TYPE_DIAGRAM_MIGRATIO%%,           -- query_type_lua_28    
                            %%DIALECT%%,                         -- query_dialect_lua_30    
                            'Diagram Tables: %%SCHEMA%%queries', -- name, summary
                            [=[
                                # Diagram Migration 1000
                                
                                ## Diagram Tables: %%SCHEMA%%queries

                                This is the first JSON Diagram code for the queries table.
                            ]=],
                            'JSON Table Definition in collection',     -- query_code,
                            %%STATUS_ACTIVE%%,                         -- query_status_lua_27, collection
                            %%JSON_INGEST_START%%                      -- DIAGRAM_START
                            [=[
                                [
                                    {
                                        "object_type": "template",
                                        "object_id": "base template",
                                        "object_value":     "<svg xmlns=\"http://www.w3.org/2000/svg\"
                                                                    xmlns:xlink=\"http://www.w3.org/1999/xlink\"
                                                                    width=\"840\"
                                                                    height=\"660\"
                                                                    viewBox=\"0 0 840 660\">

                                                                <!-- Define clipPath for rounded border with 2mm margin -->
                                                                <defs>
                                                                    <clipPath id=\"border-clip\">
                                                                    <rect x=\"6\" y=\"6\" width=\"828\" height=\"648\" rx=\"14.17\" ry=\"14.17\"/>
                                                                    </clipPath>
                                                                </defs>

                                                                <!-- Thin black border with 2mm margin -->
                                                                <rect x=\"6\" y=\"6\" width=\"828\" height=\"648\" fill=\"none\" stroke=\"black\" stroke-width=\"1\" rx=\"14.17\" ry=\"14.17\"/>

                                                                <!-- White background within border, clipped to rounded shape -->
                                                                <rect x=\"6\" y=\"6\" width=\"828\" height=\"648\" fill=\"white\" clip-path=\"url(#border-clip)\"/>

                                                                <!-- 1cm (28.3pt) dashed silver grid, offset by 2mm margin -->
                                                                <defs>
                                                                    <pattern id=\"grid\" width=\"28.3\" height=\"28.3\" patternUnits=\"userSpaceOnUse\" x=\"6\" y=\"6\">
                                                                        <path d=\"M 28.3 0 L 0 0 0 28.3\" fill=\"none\" stroke=\"silver\" stroke-width=\"0.5\" stroke-dasharray=\"2,2\"/>
                                                                    </pattern>
                                                                </defs>
                                                                <rect x=\"6\" y=\"6\" width=\"828\" height=\"648\" fill=\"url(#grid)\" clip-path=\"url(#border-clip)\"/>
                                                                
                                                                <!-- ERD content placeholder -->
                                                                <g id=\"erd-content\" transform=\"translate(6, 6)\">
                                                                    <!-- ERD SVG content goes here -->\n
                                                                </g>
                                                            </svg>"
                                    },
                                    {
                                        "object_type": "table",
                                        "object_id": "table.queries",
                                        "table": [
                                            {
                                                "name": "query_id",
                                                "datatype": "%%SERIAL%%",
                                                "nullable": false,
                                                "primary_key": true,
                                                "unique": true
                                            },
                                            {
                                                "name": "query_ref",
                                                "datatype": "%%INTEGER%%",
                                                "nullable": false,
                                                "primary_key": false,
                                                "unique": false
                                            },
                                            {
                                                "name": "query_type_lua_28",
                                                "datatype": "%%INTEGER%%",
                                                "nullable": false,
                                                "primary_key": false,
                                                "unique": false
                                            },
                                            {
                                                "name": "query_dialect_lua_30",
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
                                                "name": "query_code",
                                                "datatype": "%%TEXT%%",
                                                "nullable": false,
                                                "primary_key": false,
                                                "unique": false
                                            },
                                            {
                                                "name": "query_status_lua_27",
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
                                ]
                            ]=]
                            %%JSON_INGEST_END%%                 -- DIAGRAM_END
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