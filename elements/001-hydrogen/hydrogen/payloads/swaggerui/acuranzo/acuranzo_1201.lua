-- Migration: acuranzo_1201.lua
-- Creates the scripts table for the Hydrogen Scripting Subsystem (Phase 11a)

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.0.0 - 2026-07-01 - Initial creation for LUA_PLAN Phase 11a (scripts table)

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "scripts"
cfg.MIGRATION = "1201"

-- ----------------------------------------------------------------------------
-- Forward: Create the scripts table
-- ----------------------------------------------------------------------------
table.insert(queries,{sql=[[

    INSERT INTO ${SCHEMA}${QUERIES} (
        ${QUERIES_INSERT}
    )
    WITH next_query_id AS (
        SELECT COALESCE(MAX(query_id), 0) + 1 AS new_query_id
        FROM ${SCHEMA}${QUERIES}
    )
    SELECT
        new_query_id                                                        AS query_id,
        ${MIGRATION}                                                        AS query_ref,
        ${STATUS_ACTIVE}                                                    AS query_status_a27,
        ${TYPE_FORWARD_MIGRATION}                                           AS query_type_a28,
        ${DIALECT}                                                          AS query_dialect_a30,
        ${QTC_SLOW}                                                         AS query_queue_a58,
        ${TIMEOUT}                                                          AS query_timeout,
        [=[
            CREATE TABLE ${SCHEMA}${TABLE}
            (
                group_name           ${TEXT}             NOT NULL,
                script_name          ${TEXT}             NOT NULL,
                script_type          ${INTEGER}          NOT NULL,
                schedule             ${TEXT}                     ,
                next_run             ${TIMESTAMP_TZ}             ,
                last_run_start       ${TIMESTAMP_TZ}             ,
                last_run_end         ${TIMESTAMP_TZ}             ,
                status               ${INTEGER}          NOT NULL,
                code                 ${TEXT_BIG}                 ,
                summary              ${TEXT_BIG}                 ,
                ${COMMON_CREATE}
                ${PRIMARY}(group_name, script_name)
            );

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Create ${TABLE} Table'                                             AS name,
        [=[
            # Forward Migration ${MIGRATION}: Create ${TABLE} Table

            This migration creates the ${TABLE} table for the Hydrogen Scripting
            Subsystem. The table is the source of truth for every Lua source the
            subsystem knows about, with a discriminator (script_type) that
            distinguishes Orchestrators (long-running, one per process), Scripts
            (named runnable units workers execute), and Snippets (inline source
            submitted at runtime, not in the table).

            ## Columns

            - group_name:    Logical grouping (e.g. 'Orchestrators', 'Reports', 'Maintenance')
            - script_name:   Script identifier within the group (e.g. 'Orchestrator.lua')
            - script_type:   0 = Orchestrator, 1 = Script, 2 = Snippet (see Lookup 061)
            - schedule:      Optional cron-like schedule string; NULL = on demand
            - next_run:      Next scheduled run timestamp; NULL = unscheduled
            - last_run_start:When the script last started; NULL = never run
            - last_run_end:  When the script last completed; NULL = never completed
            - status:        0 = Disabled, 1 = Enabled (see Lookup 062)
            - code:          The Lua source
            - summary:       Markdown description / changelog
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})

-- ----------------------------------------------------------------------------
-- Reverse: Drop the scripts table
-- ----------------------------------------------------------------------------
table.insert(queries,{sql=[[

    INSERT INTO ${SCHEMA}${QUERIES} (
        ${QUERIES_INSERT}
    )
    WITH next_query_id AS (
        SELECT COALESCE(MAX(query_id), 0) + 1 AS new_query_id
        FROM ${SCHEMA}${QUERIES}
    )
    SELECT
        new_query_id                                                        AS query_id,
        ${MIGRATION}                                                        AS query_ref,
        ${STATUS_ACTIVE}                                                    AS query_status_a27,
        ${TYPE_REVERSE_MIGRATION}                                           AS query_type_a28,
        ${DIALECT}                                                          AS query_dialect_a30,
        ${QTC_SLOW}                                                         AS query_queue_a58,
        ${TIMEOUT}                                                          AS query_timeout,
        [=[
            ${DROP_CHECK};

            ${SUBQUERY_DELIMITER}

            DROP TABLE ${SCHEMA}${TABLE};

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Drop ${TABLE} Table'                                               AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Drop ${TABLE} Table

            This is provided for completeness when testing the migration system
            to ensure that forward and reverse migrations are complete.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})

-- ----------------------------------------------------------------------------
-- Diagram: JSON Table Definition for the scripts table
-- ----------------------------------------------------------------------------
table.insert(queries,{sql=[[

    INSERT INTO ${SCHEMA}${QUERIES} (
        ${QUERIES_INSERT}
    )
    WITH next_query_id AS (
        SELECT COALESCE(MAX(query_id), 0) + 1 AS new_query_id
        FROM ${SCHEMA}${QUERIES}
    )
    SELECT
        new_query_id                                                        AS query_id,
        ${MIGRATION}                                                        AS query_ref,
        ${STATUS_ACTIVE}                                                    AS query_status_a27,
        ${TYPE_DIAGRAM_MIGRATION}                                           AS query_type_a28,
        ${DIALECT}                                                          AS query_dialect_a30,
        ${QTC_SLOW}                                                         AS query_queue_a58,
        ${TIMEOUT}                                                          AS query_timeout,
        'JSON Table Definition in collection'                               AS code,
        'Diagram Tables: ${SCHEMA}${TABLE}'                                 AS name,
        [=[
            # Diagram Migration ${MIGRATION}

            ## Diagram Tables: ${SCHEMA}${TABLE}
        ]=]
                                                                            AS summary,
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
                                "name": "group_name",
                                "datatype": "${TEXT}",
                                "nullable": false,
                                "primary_key": true,
                                "unique": false
                            },
                            {
                                "name": "script_name",
                                "datatype": "${TEXT}",
                                "nullable": false,
                                "primary_key": true,
                                "unique": false
                            },
                            {
                                "name": "script_type",
                                "datatype": "${INTEGER}",
                                "nullable": false,
                                "primary_key": false,
                                "unique": false,
                                "lookup": true
                            },
                            {
                                "name": "schedule",
                                "datatype": "${TEXT}",
                                "nullable": true,
                                "primary_key": false,
                                "unique": false
                            },
                            {
                                "name": "next_run",
                                "datatype": "${TIMESTAMP_TZ}",
                                "nullable": true,
                                "primary_key": false,
                                "unique": false
                            },
                            {
                                "name": "last_run_start",
                                "datatype": "${TIMESTAMP_TZ}",
                                "nullable": true,
                                "primary_key": false,
                                "unique": false
                            },
                            {
                                "name": "last_run_end",
                                "datatype": "${TIMESTAMP_TZ}",
                                "nullable": true,
                                "primary_key": false,
                                "unique": false
                            },
                            {
                                "name": "status",
                                "datatype": "${INTEGER}",
                                "nullable": false,
                                "primary_key": false,
                                "unique": false,
                                "lookup": true
                            },
                            {
                                "name": "code",
                                "datatype": "${TEXT_BIG}",
                                "nullable": true,
                                "primary_key": false,
                                "unique": false
                            },
                            {
                                "name": "summary",
                                "datatype": "${TEXT_BIG}",
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
                                                                            AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})

return queries
end
