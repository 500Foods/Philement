-- Migration: acuranzo_1280.lua
-- Seed system mail templates (server_started, databases_ready, server_stopped)

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.0.0 - 2026-07-28 - System event templates for Mail Relay events blackbox

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "mail_templates"
cfg.MIGRATION = "1280"
-- ----------------------------------------------------------------------------
-- Forward: Insert system event templates
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
            INSERT INTO ${SCHEMA}${TABLE} (
                template_id,
                template_key,
                name,
                status_a64,
                subject_template,
                text_template,
                html_template,
                collection,
                ${COMMON_FIELDS}
            )
            VALUES
            (
                3,
                'system.server_started',
                'System Server Started',
                1,
                '[%COUNT|1%x] MailRelayEvent server_started %SERVER_NAME%',
                'Server %SERVER_NAME% (%APP_NAME%) started at %TIMESTAMP%.\n%SUMMARY|1 event%',
                '<p>Server %SERVER_NAME% (%APP_NAME%) started at %TIMESTAMP%.</p><p>%SUMMARY|1 event%</p>',
                '{}',
                ${COMMON_VALUES}
            ),
            (
                4,
                'system.databases_ready',
                'System Databases Ready',
                1,
                '[%COUNT|1%x] MailRelayEvent databases_ready %SERVER_NAME%',
                'Databases ready on %SERVER_NAME% (%APP_NAME%) at %TIMESTAMP%.\n%SUMMARY|1 event%',
                '<p>Databases ready on %SERVER_NAME% (%APP_NAME%) at %TIMESTAMP%.</p><p>%SUMMARY|1 event%</p>',
                '{}',
                ${COMMON_VALUES}
            ),
            (
                5,
                'system.server_stopped',
                'System Server Stopped',
                1,
                'MailRelayEvent server_stopped %SERVER_NAME%',
                'Server %SERVER_NAME% (%APP_NAME%) stopping at %TIMESTAMP%.',
                '<p>Server %SERVER_NAME% (%APP_NAME%) stopping at %TIMESTAMP%.</p>',
                '{}',
                ${COMMON_VALUES}
            );

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Seed system mail event templates'                                  AS name,
        [=[
            # Forward Migration ${MIGRATION}: Seed system mail event templates

            Inserts `system.server_started`, `system.databases_ready`, and
            `system.server_stopped` templates used by Mail Relay system events
            and blackbox tests. Started/ready subjects include %COUNT% for
            debounce coalescing.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})

-- ----------------------------------------------------------------------------
-- Reverse: Remove system event templates
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
            DELETE FROM ${SCHEMA}${TABLE}
            WHERE template_key IN (
                'system.server_started',
                'system.databases_ready',
                'system.server_stopped'
            );

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Remove system mail event templates'                                AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Remove system mail event templates
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})

return queries end
