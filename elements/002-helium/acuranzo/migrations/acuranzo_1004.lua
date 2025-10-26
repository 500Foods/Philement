-- Migration: acuranzo_1004.lua
-- Creates the account_roles table and populating it with the next migration.

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 2.0.0 - 2025-10-25 - Moved to latest migration format
-- 1.1.0 - 2025-09-28 - Changed diagram query to use JSON table definition instead of PlantUML for custom ERD tool.
-- 1.0.0 - 2025-09-13 - Initial creation for account_roles table with PostgreSQL support.

return function(engine, design_name, schema_name, cfg)
local queries = {}
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
table.insert(queries,{sql=[[

    INSERT INTO ${SCHEMA}queries (
        ${QUERIES_INSERT}
    )
    VALUES (
        (SELECT COALESCE(MAX(query_id), 0) + 1 FROM ${SCHEMA}queries),      -- query_id
        1004,                                                               -- query_ref
        ${STATUS_ACTIVE},                                                   -- query_status_a27
        ${TYPE_FORWARD_MIGRATION},                                          -- query_type_a28
        ${DIALECT},                                                         -- query_dialect_a30
        ${QTC_SLOW},                                                        -- query_queue_a58
        5000,                                                               -- query_timeout
        [=[
            CREATE TABLE IF NOT EXISTS ${SCHEMA}account_roles
            (
                account_id              ${INTEGER}          NOT NULL,
                role_id                 ${INTEGER}          NOT NULL,
                system_id               ${INTEGER}          NOT NULL,
                status_a37              ${INTEGER}          NOT NULL,
                name                    ${VARCHAR_100}              ,
                summary                 ${TEXTBIG}                  ,
                collection              ${JSON}                     ,
                ${COMMON_CREATE}
                PRIMARY KEY(account_id, role_id, system_id)
            );
        ]=],
                                                                            -- code
        'Create Account Roles Table Query',                                 -- name
        [=[
            # Forward Migration 1004: Create Account Roles Table Query

            This migration creates the account_roles table for storing account role data.
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
        1003,                                                               -- query_ref
        ${STATUS_ACTIVE},                                                   -- query_status_a27
        ${TYPE_REVERSE_MIGRATION},                                          -- query_type_a28
        ${DIALECT},                                                         -- query_dialect_a30
        ${QTC_SLOW},                                                        -- query_queue_a58
        5000,                                                               -- query_timeout
        [=[
            DROP TABLE ${SCHEMA}account_roles;
        ]=],
                                                                            -- code
        'Delete Account Roles Table Query',                                 -- name
        [=[
            # Reverse Migration 1004: Delete Account Roles Table Query

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
        1004,                                                               -- query_ref
        ${STATUS_ACTIVE},                                                   -- query_status_a27
        ${TYPE_DIAGRAM_MIGRATION},                                          -- query_type_a28
        ${DIALECT},                                                         -- query_dialect_a30
        ${QTC_SLOW},                                                        -- query_queue_a58
        5000,                                                               -- query_timeout
        'JSON Table Definition in collection',                              -- code,
        'Diagram Tables: ${SCHEMA}account_roles',                           -- name
        [=[
            # Diagram Migration 1004

            ## Diagram Tables: ${SCHEMA}account_roles

            This is the first JSON Diagram code for the account_roles table.
        ]=],
                                                                            -- summary
                                                                            -- DIAGRAM_START
        ${JSON_INGEST_START}
        [=[
            {
                "diagram": [
                    {
                        "object_type": "table",
                        "object_id": "table.account_roles",
                        "object_ref": "1004",
                        "table": [
                            {
                                "name": "account_id",
                                "datatype": "${INTEGER}",
                                "nullable": false,
                                "primary_key": true,
                                "unique": false
                            },
                            {
                                "name": "role_id",
                                "datatype": "${INTEGER}",
                                "nullable": false,
                                "primary_key": true,
                                "unique": false
                            },
                            {
                                "name": "system_id",
                                "datatype": "${INTEGER}",
                                "nullable": false,
                                "primary_key": true,
                                "unique": false
                            },
                            {
                                "name": "status_a37",
                                "datatype": "${INTEGER}",
                                "nullable": false,
                                "primary_key": false,
                                "unique": false,
                                "lookup": true
                            },
                            {
                                "name": "name",
                                "datatype": "${VARCHAR_100}",
                                "nullable": true,
                                "primary_key": false,
                                "unique": false
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
                                "standard": true
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
