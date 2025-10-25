-- Migration: acuranzo_1003.lua
-- Creates the account_contacts table and populating it with the next migration.

-- CHANGELOG
-- 2.0.0 - 2025-10-25 - Moved to latest migration format
-- 1.1.0 - 2025-09-28 - Changed diagram query to use JSON table definition instead of PlantUML for custom ERD tool.
-- 1.0.0 - 2025-09-13 - Initial creation for account_contacts table with PostgreSQL support.

-- luacheck: no max line length
-- luacheck: no unused args

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
            CREATE TABLE ${SCHEMA}account_contacts
            (
                contact_id ${INTEGER} NOT NULL,
                account_id ${INTEGER} NOT NULL,
                contact_type_lua_18 ${INTEGER} NOT NULL,
                contact_seq ${INTEGER} NOT NULL,
                contact ${VARCHAR_100} NOT NULL,
                summary ${VARCHAR_500},
                authenticate_lua_19 ${INTEGER} NOT NULL,
                status_lua_20 ${INTEGER} NOT NULL,
                collection ${JSONB},
                valid_after ${TIMESTAMP_TZ},
                valid_until ${TIMESTAMP_TZ},
                created_id ${INTEGER} NOT NULL,
                created_at ${TIMEZONE_TZ} NOT NULL,
                updated_id ${INTEGER} NOT NULL,
                updated_at ${TIMEZONE_TZ} NOT NULL
            );
        ]=],
                                                                            -- code
        'Create Account Contacts Table Query',                               -- name
        [=[
            # Forward Migration 1003: Create Account Contacts Table Query

            This migration creates the account_contacts table for storing account contact data.
        ]=],
                                                                            -- summary
        NULL,                                                               -- collection
        ${QUERIES_COMMON}
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
            DROP TABLE ${SCHEMA}account_contacts;
        ]=],
                                                                            -- code
        'Delete Account Contacts Table Query',                              -- name
        [=[
            # Reverse Migration 1003: Delete Account Contacts Table Query

            This is provided for completeness when testing the migration system
            to ensure that forward and reverse migrations are complete.
        ]=],
                                                                            -- summary
        NULL,                                                               -- collection
        ${QUERIES_COMMON}
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
        ${TYPE_DIAGRAM_MIGRATION},                                          -- query_type_a28
        ${DIALECT},                                                         -- query_dialect_a30
        ${QTC_SLOW},                                                        -- query_queue_a58
        5000,                                                               -- query_timeout
        'JSON Table Definition in collection',                              -- code,
        'Diagram Tables: ${SCHEMA}account_contacts',                        -- name
        [=[
            # Diagram Migration 1003

            ## Diagram Tables: ${SCHEMA}account_contacts

            This is the first JSON Diagram code for the account_contacts table.
        ]=],
                                                                            -- summary
                                                                            -- DIAGRAM_START
        ${JSON_INGEST_START}
        [=[
            {
                "diagram": [
                    {
                        "object_type": "table",
                        "object_id": "table.account_contacts",
                        "object_ref": "1003",
                        "table": [
                            {
                                "name": "account_id",
                                "datatype": "${INTEGER}",
                                "nullable": false,
                                "primary_key": true,
                                "unique": false
                            },
                            {
                                "name": "contact_id",
                                "datatype": "${INTEGER}",
                                "nullable": false,
                                "primary_key": true,
                                "unique": true
                            },
                            {
                                "name": "contact_seq",
                                "datatype": "${INTEGER}",
                                "nullable": false,
                                "primary_key": true,
                                "unique": false
                            },
                            {
                                "name": "contact",
                                "datatype": "${VARCHAR_100}",
                                "nullable": false,
                                "primary_key": false,
                                "unique": false
                            },
                            {
                                "name": "summary",
                                "datatype": "${VARCHAR_500}",
                                "nullable": true,
                                "primary_key": false,
                                "unique": false
                            },
                            {
                                "name": "contact_type_lua_18",
                                "datatype": "${INTEGER}",
                                "nullable": false,
                                "primary_key": false,
                                "unique": false,
                                "lookup": true
                            },
                            {
                                "name": "authenticate_lua_19",
                                "datatype": "${INTEGER}",
                                "nullable": false,
                                "primary_key": false,
                                "unique": false,
                                "lookup": true
                            },
                            {
                                "name": "status_lua_20",
                                "datatype": "${INTEGER}",
                                "nullable": false,
                                "primary_key": false,
                                "unique": false,
                                "lookup": true
                            },
                            {
                                "name": "collection",
                                "datatype": "${JSONB}",
                                "nullable": true,
                                "primary_key": false,
                                "unique": false,
                                "standard": true
                            },
                            {
                                "name": "valid_after",
                                "datatype": "${TIMESTAMP_TZ}",
                                "nullable": true,
                                "primary_key": false,
                                "unique": false,
                                "standard": true
                            },
                            {
                                "name": "valid_until",
                                "datatype": "${TIMESTAMP_TZ}",
                                "nullable": true,
                                "primary_key": false,
                                "unique": false,
                                "standard": true
                            },
                            {
                                "name": "created_id",
                                "datatype": "${INTEGER}",
                                "nullable": false,
                                "primary_key": false,
                                "unique": false,
                                "standard": true
                            },
                            {
                                "name": "created_at",
                                "datatype": "${TIMESTAMP_TZ}",
                                "nullable": false,
                                "primary_key": false,
                                "unique": false,
                                "standard": true
                            },
                            {
                                "name": "updated_id",
                                "datatype": "${INTEGER}",
                                "nullable": false,
                                "primary_key": false,
                                "unique": false,
                                "standard": true
                            },
                            {
                                "name": "updated_at",
                                "datatype": "${TIMESTAMP_TZ}",
                                "nullable": false,
                                "primary_key": false,
                                "unique": false,
                                "standard": true
                            }
                        ]
                    }
                ]
            }
        ]=]
        ${JSON_INGEST_END}
                                                                            -- DIAGRAM_END
        ,                                                                   -- collection
        ${QUERIES_COMMON}
    );

]]})
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
return queries end
