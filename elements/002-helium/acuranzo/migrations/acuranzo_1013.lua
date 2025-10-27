-- Migration: acuranzo_1013.lua
-- Creates the lists table and populating it with the next migration.

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 2.0.0 - 2025-10-27 - Moved to latest migration format
-- 1.1.0 - 2025-09-28 - Changed diagram query to use JSON table definition instead of PlantUML for custom ERD tool.
-- 1.0.0 - 2025-09-13 - Initial creation for lists table with PostgreSQL support.

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "lists"
cfg.MIGRATION = "1013"
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
table.insert(queries,{sql=[[

    INSERT INTO ${SCHEMA}${QUERIES} (
        ${QUERIES_INSERT}
    )
    VALUES (
        (SELECT COALESCE(MAX(query_id), 0) + 1 FROM ${SCHEMA}${QUERIES}),   -- query_id
        ${MIGRATION},                                                       -- query_ref
        ${STATUS_ACTIVE},                                                   -- query_status_a27
        ${TYPE_FORWARD_MIGRATION},                                          -- query_type_a28
        ${DIALECT},                                                         -- query_dialect_a30
        ${QTC_SLOW},                                                        -- query_queue_a58
        ${TIMEOUT},                                                         -- query_timeout
        [=[
            CREATE TABLE IF NOT EXISTS ${SCHEMA}${TABLE}
            (
                list_key                ${INTEGER}          NOT NULL,
                list_type_a31           ${INTEGER}          NOT NULL,
                status_a32              ${INTEGER}          NOT NULL,
                list_value              ${TEXT}                     ,
                list_note               ${TEXT}                     ,
                collection              ${JSON}                     ,
                ${COMMON_CREATE}
                ${PRIMARY}(list_key)
            );
       ]=],
                                                                            -- code
        'Create ${TABLE} Table',                                            -- name
        [=[
            # Forward Migration ${MIGRATION}: Create ${TABLE} Table

            This migration creates the ${TABLE} table for storing list data.
        ]=],
                                                                            -- summary
        NULL,                                                               -- collection
        ${COMMON_INSERT}
    );

]]})
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
table.insert(queries,{sql=[[

    INSERT INTO ${SCHEMA}${QUERIES} (
        ${QUERIES_INSERT}
    )
    VALUES (
        (SELECT COALESCE(MAX(query_id), 0) + 1 FROM ${SCHEMA}${QUERIES}),   -- query_id
        ${MIGRATION},                                                       -- query_ref
        ${STATUS_ACTIVE},                                                   -- query_status_a27
        ${TYPE_REVERSE_MIGRATION},                                          -- query_type_a28
        ${DIALECT},                                                         -- query_dialect_a30
        ${QTC_SLOW},                                                        -- query_queue_a58
        ${TIMEOUT},                                                         -- query_timeout
        [=[
            DROP TABLE ${SCHEMA}${TABLE};
        ]=],
                                                                            -- code
        'Drop ${TABLE} Table',                                              -- name
        [=[
            # Reverse Migration ${MIGRATION}: Drop ${TABLE} Table

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

    INSERT INTO ${SCHEMA}${QUERIES} (
        ${QUERIES_INSERT}
    )
    VALUES (
        (SELECT COALESCE(MAX(query_id), 0) + 1 FROM ${SCHEMA}${QUERIES}),   -- query_id
        ${MIGRATION},                                                       -- query_ref
        ${STATUS_ACTIVE},                                                   -- query_status_a27
        ${TYPE_DIAGRAM_MIGRATION},                                          -- query_type_a28
        ${DIALECT},                                                         -- query_dialect_a30
        ${QTC_SLOW},                                                        -- query_queue_a58
        ${TIMEOUT},                                                         -- query_timeout
        'JSON Table Definition in collection',                              -- code,
        'Diagram Tables: ${SCHEMA}${TABLE}',                                -- name
        [=[
            # Diagram Migration ${MIGRATION}

            ## Diagram Tables: ${SCHEMA}${TABLE}

            This is the first JSON Diagram code for the ${TABLE} table.
        ]=],
                                                                            -- summary
                                                                            -- DIAGRAM_START
        ${JSON_INGEST_START}
        [=[
            {
                "diagram": [
                    {
                        "object_type": "table",
                        "object_id": "table.${TABLE}",
                        "object_ref": "${MIGRATION}",
                        "table": [
                            {
                                "name": "list_key",
                                "datatype": "${INTEGER}",
                                "nullable": false,
                                "primary_key": true,
                                "unique": false
                            },
                            {
                                "name": "list_type_lua_31",
                                "datatype": "${INTEGER}",
                                "nullable": false,
                                "primary_key": false,
                                "unique": false
                            },
                            {
                                "name": "status_lua_32",
                                "datatype": "${INTEGER}",
                                "nullable": false,
                                "primary_key": false,
                                "unique": false,
                                "lookup": true
                            },
                            {
                                "name": "list_value",
                                "datatype": "${TEXT}",
                                "nullable": true,
                                "primary_key": false,
                                "unique": false
                            },
                            {
                                "name": "list_note",
                                "datatype": "${TEXT}",
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
