-- Migration: acuranzo_1002.lua
-- Creates the account_access table and populating it with the next migration.

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 2.0.0 - 2025-10-18 - Moved to latest migration format
-- 1.1.0 - 2025-09-28 - Changed diagram query to use JSON table definition instead of PlantUML for custom ERD tool.
-- 1.0.0 - 2025-09-13 - Initial creation for account_access table with PostgreSQL support.

return function(engine, design_name, schema_name, cfg)
local queries = {}
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
table.insert(queries,{sql=[[

    INSERT INTO ${SCHEMA}queries (
        ${QUERIES_INSERT}
    )
    VALUES (
        (SELECT COALESCE(MAX(query_id), 0) + 1 FROM ${SCHEMA}queries),      -- query_id
        1002,                                                               -- query_ref
        ${STATUS_ACTIVE},                                                   -- query_status_a27
        ${TYPE_FORWARD_MIGRATION},                                          -- query_type_a28
        ${DIALECT},                                                         -- query_dialect_a30
        ${QTC_SLOW},                                                        -- query_queue_a58
        5000,                                                               -- query_timeout
        [=[
            CREATE TABLE IF NOT EXISTS ${SCHEMA}account_access
            (
                account_id              ${INTEGER}          NOT NULL,
                access_id               ${INTEGER}          NOT NULL,
                feature_a21             ${INTEGER}          NOT NULL,
                access_type_a22         ${INTEGER}          NOT NULL,
                status_a23              ${INTEGER}          NOT NULL,
                summary                 ${TEXTBIG}                  ,
                collection              ${JSON}                     ,
                ${COMMON_CREATE}
                PRIMARY KEY (access_id)
            );
        ]=],
                                                                            -- code
        'Create Account Access Table Query',                                -- name
        [=[
            # Forward Migration 1002: Create Account Access Table Query

            This migration creates the account_access table for storing account access data.
        ]=],
                                                                            -- summary
        NULL,                                                               -- collection
        ${COMMON_INSERT}
    );

]]})
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
table.insert(queries,{sql=[[

    INSERT INTO ${SCHEMA}queries (
        ${QUERIES_INSERT}
    )
    VALUES (
        (SELECT COALESCE(MAX(query_id), 0) + 1 FROM ${SCHEMA}queries),      -- query_id
        1002,                                                               -- query_ref
        ${STATUS_ACTIVE},                                                   -- query_status_a27
        ${TYPE_REVERSE_MIGRATION},                                          -- query_type_a28
        ${DIALECT},                                                         -- query_dialect_a30
        ${QTC_SLOW},                                                        -- query_queue_a58
        5000,                                                               -- query_timeout
        [=[
            DROP TABLE ${SCHEMA}account_access;
        ]=],
                                                                            -- code
        'Delete Account Access Table Query',                                -- name
        [=[
            # Reverse Migration 1002: Delete Account Access Table Query

            This is provided for completeness when testing the migration system
            to ensure that forward and reverse migrations are complete.
        ]=],
                                                                            -- summary
        NULL,                                                               -- collection
        ${COMMON_INSERT}
    );

]]})
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
table.insert(queries,{sql=[[

    INSERT INTO ${SCHEMA}queries (
        ${QUERIES_INSERT}
    )
    VALUES (
        (SELECT COALESCE(MAX(query_id), 0) + 1 FROM ${SCHEMA}queries),      -- query_id
        1002,                                                               -- query_ref
        ${STATUS_ACTIVE},                                                   -- query_status_a27
        ${TYPE_DIAGRAM_MIGRATION},                                          -- query_type_a28
        ${DIALECT},                                                         -- query_dialect_a30
        ${QTC_SLOW},                                                        -- query_queue_a58
        5000,                                                               -- query_timeout
        'JSON Table Definition in collection',                              -- code,
        'Diagram Tables: ${SCHEMA}account_access',                          -- name
        [=[
            # Diagram Migration 1002

            ## Diagram Tables: ${SCHEMA}account_access

            This is the first JSON Diagram code for the account_access table.
        ]=],
                                                                            -- summary
                                                                            -- DIAGRAM_START
        ${JSON_INGEST_START}
        [=[
            {
                "diagram": [
                    {
                        "object_type": "table",
                        "object_id": "table.account_access",
                        "object_ref": "1002",
                        "table": [
                            {
                                "name": "access_id",
                                "datatype": "${INTEGER}",
                                "nullable": false,
                                "primary_key": true,
                                "unique": true
                            },
                            {
                                "name": "account_id",
                                "datatype": "${INTEGER}",
                                "nullable": false,
                                "primary_key": true,
                                "unique": false
                            },
                            {
                                "name": "feature_a21",
                                "datatype": "${INTEGER}",
                                "nullable": false,
                                "primary_key": false,
                                "unique": false,
                                "lookup": true
                            },
                            {
                                "name": "access_type_a22",
                                "datatype": "${INTEGER}",
                                "nullable": false,
                                "primary_key": false,
                                "unique": false,
                                "lookup": true
                            },
                            {
                                "name": "status_a23",
                                "datatype": "${INTEGER}",
                                "nullable": false,
                                "primary_key": false,
                                "unique": false,
                                "lookup": true
                            },
                            {
                                "name": "summary",
                                "datatype": "${TEXTBIG}",
                                "nullable": true,
                                "primary_key": false,
                                "unique": false
                            },
                            {
                                "name": "collection",
                                "datatype": "${JSON}",
                                "nullable": true,
                                "primary_key": false,
                                "unique": false,
                                "standard": false
                            },
                            ${COMMON_DIAGRAM}
                        ]
                    }
                ]
            }
        ]=]
        ${JSON_INGEST_END}
                                                                            -- DIAGRAM_END
        ,                                                                   -- collection
        ${COMMON_INSERT}
    );

]]})
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
return queries end
