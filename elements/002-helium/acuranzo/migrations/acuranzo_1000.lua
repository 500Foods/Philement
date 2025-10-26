-- Migration: acuranzo_1000.lua
-- Bootstraps the migration system by creating the queries table and populating it with the next migration.

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 2.0.0 - 2025-10-18 - Moved to latest migration format
-- 1.1.0 - 2025-09-28 - Changed diagram query to use JSON table definition instead of PlantUML for custom ERD tool.
-- 1.0.0 - 2025-09-13 - Initial creation for queries table with SQLite, PostgreSQL, MySQL, DB2 support.

return function(engine, design_name, schema_name, cfg)
local queries = {}
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
table.insert(queries,{sql=[[

    CREATE TABLE ${SCHEMA}queries (
        query_id                ${INTEGER}          NOT NULL,
        query_ref               ${INTEGER}          NOT NULL,
        query_status_a27        ${INTEGER}          NOT NULL,
        query_type_a28          ${INTEGER}          NOT NULL,
        query_dialect_a30       ${INTEGER}          NOT NULL,
        query_queue_a58         ${INTEGER}          NOT NULL,
        query_timeout           ${INTEGER}          NOT NULL,
        code                    ${TEXTBIG}          NOT NULL,
        name                    ${VARCHAR_100}      NOT NULL,
        summary                 ${TEXTBIG}                  ,
        collection              ${JSON}                     ,
        ${COMMON_CREATE}
        ${PRIMARY}(query_id),                                               -- Primary Key
        ${UNIQUE}(query_ref, query_type_a28)                             -- Unique Column
    );

]]})
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
-- NOTE: SQLite has no JSON handling peculiarities, so no custom function is defined
if engine ~= 'sqlite' then table.insert(queries,{sql=[[

    -- Defined in database_<engine>.lua as a macro
    ${JSON_INGEST_FUNCTION}

]]}) end
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
table.insert(queries,{sql=[[

    INSERT INTO ${SCHEMA}queries (
        ${QUERIES_INSERT}
    )
    VALUES (
        (SELECT COALESCE(MAX(query_id), 0) + 1 FROM ${SCHEMA}queries),      -- query_id
        1000,                                                               -- query_ref
        ${STATUS_ACTIVE},                                                   -- query_status_a27
        ${TYPE_APPLIED_MIGRATION},                                          -- query_type_a28
        ${DIALECT},                                                         -- query_dialect_a30
        ${QTC_SLOW},                                                        -- query_queue_a58
        5000,                                                               -- query_timeout
        [=[
            CREATE TABLE ${SCHEMA}queries (
                query_id                ${INTEGER},
                query_ref               ${INTEGER}          NOT NULL,
                query_status_a27        ${INTEGER}          NOT NULL,
                query_type_a28          ${INTEGER}          NOT NULL,
                query_dialect_a30       ${INTEGER}          NOT NULL,
                query_queue_a58         ${INTEGER}          NOT NULL,
                query_timeout           ${INTEGER}          NOT NULL,
                query_code              ${TEXTBIG}          NOT NULL,
                name                    ${VARCHAR_100}      NOT NULL,
                summary                 ${TEXTBIG}                  ,
                collection              ${JSON}                     ,
                ${COMMON_CREATE}
                ${PRIMARY}(query_id),                                       -- Primary Key
                ${UNIQUE}(query_ref)                                        -- Unique Column
            );
        ]=],
                                                                            -- code
        'Create Tables Query',                                              -- name
        [=[
            # Forward Migration 1000: Create Tables Query

            This is the first migration that, technically, is run automatically
            when connecting to an empty database and kicks off the migration,
            so long as the database has been configured with AutoMigration: true
            in its config (this is the default if not supplied).
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
        1000,                                                               -- query_ref
        ${STATUS_ACTIVE},                                                   -- query_status_a27
        ${TYPE_REVERSE_MIGRATION},                                          -- query_type_a28
        ${DIALECT},                                                         -- query_dialect_a30
        ${QTC_SLOW},                                                        -- query_queue_a58
        5000,                                                               -- query_timeout
        [=[
            DROP TABLE ${SCHEMA}queries;
        ]=],
                                                                            -- code
        'Delete Tables Query',                                              -- name
        [=[
            # Reverse Migration 1000: Delete Tables Query

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
        1000,                                                               -- query_ref
        ${STATUS_ACTIVE},                                                   -- query_status_a27
        ${TYPE_DIAGRAM_MIGRATION},                                          -- query_type_a28
        ${DIALECT},                                                         -- query_dialect_a30
        ${QTC_SLOW},                                                        -- query_queue_a58
        5000,                                                               -- query_timeout
        'JSON Table Definition in collection',                              -- code
        'Diagram Tables: ${SCHEMA}queries',                                 -- name
        [=[
            # Diagram Migration 1000

            ## Diagram Tables: ${SCHEMA}queries

            This is the first JSON Diagram code for the queries table.
        ]=],
                                                                            -- summary
                                                                            -- DIAGRAM_START
        ${JSON_INGEST_START}
        [=[
            {
                "diagram": [
                    {
                        "object_type": "template",
                        "object_id": "base template",
                        "object_value": "<svg   xmlns=\"http://www.w3.org/2000/svg\"
                                                xmlns:xlink=\"http://www.w3.org/1999/xlink\"
                                                width=\"2520\"
                                                height=\"1980\"
                                                viewBox=\"0 0 2520 1980\">
                                            <!-- Define clipPath for rounded border with 2mm margin -->
                                            <defs>
                                                <clipPath id=\"border-clip\">
                                                <rect x=\"18\" y=\"18\" width=\"2484\" height=\"1944\" rx=\"42.51\" ry=\"42.51\"/>
                                                </clipPath>
                                            </defs>
                                            <!-- Thin black border with 2mm margin -->
                                            <rect x=\"18\" y=\"18\" width=\"2484\" height=\"1944\" fill=\"none\" stroke=\"black\" stroke-width=\"3\" rx=\"42.51\" ry=\"42.51\"/>
                                            <!-- White background within border, clipped to rounded shape -->
                                            <rect x=\"18\" y=\"18\" width=\"2484\" height=\"1944\" fill=\"white\" clip-path=\"url(#border-clip)\"/>
                                            <!-- 1cm (28.3pt) dashed silver grid, offset by 2mm margin -->
                                            <defs>
                                                <pattern id=\"grid\" width=\"84.9\" height=\"84.9\" patternUnits=\"userSpaceOnUse\" x=\"18\" y=\"18\">
                                                    <path d=\"M 84.9 0 L 0 0 0 84.9\" fill=\"none\" stroke=\"black\" stroke-width=\"1.5\" stroke-dasharray=\"6,6\"/>
                                                </pattern>
                                            </defs>
                                            <rect x=\"18\" y=\"18\" width=\"2484\" height=\"1944\" fill=\"url(#grid)\" clip-path=\"url(#border-clip)\"/>
                                            <!-- ERD content placeholder -->
                                            <g id=\"erd-content\" transform=\"translate(0, 0)\">
                                                <!-- ERD SVG content goes here -->
                                            </g>
                                        </svg>"
                    },
                    {
                        "object_type": "table",
                        "object_id": "table.queries",
                        "object_ref": "1000",
                        "table": [
                            {
                                "name": "query_id",
                                "datatype": "${INTEGER}",
                                "nullable": false,
                                "primary_key": true,
                                "unique": true
                            },
                            {
                                "name": "query_ref",
                                "datatype": "${INTEGER}",
                                "nullable": false,
                                "primary_key": false,
                                "unique": true
                            },
                            {
                                "name": "query_status_a27",
                                "datatype": "${INTEGER}",
                                "nullable": false,
                                "primary_key": false,
                                "unique": false,
                                "lookup": true
                            },
                            {
                                "name": "query_type_a28",
                                "datatype": "${INTEGER}",
                                "nullable": false,
                                "primary_key": false,
                                "unique": false,
                                "lookup": true
                            },
                            {
                                "name": "query_dialect_a30",
                                "datatype": "${INTEGER}",
                                "nullable": false,
                                "primary_key": false,
                                "unique": false,
                                "lookup": true
                            },
                            {
                                "name": "query_queue_a58",
                                "datatype": "${INTEGER}",
                                "nullable": false,
                                "primary_key": false,
                                "unique": false,
                                "lookup": true
                            },
                            {
                                "name": "query_timeout",
                                "datatype": "${INTEGER}",
                                "nullable": false,
                                "primary_key": false,
                                "unique": false,
                                "lookup": false
                            },
                            {
                                "name": "query_code",
                                "datatype": "${TEXTBIG}",
                                "nullable": false,
                                "primary_key": false,
                                "unique": false
                            },
                            {
                                "name": "name",
                                "datatype": "${VARCHAR_100}",
                                "nullable": false,
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
