-- Migration: acuranzo_1282.lua
-- Fix system event templates: COUNT/SUMMARY use macro defaults for debounce

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.0.0 - 2026-07-28 - Defaults so render succeeds before debounce flush

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "mail_templates"
cfg.MIGRATION = "1282"
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
            UPDATE ${SCHEMA}${TABLE}
            SET subject_template = '[%COUNT|1%x] MailRelayEvent server_started %SERVER_NAME%',
                text_template = 'Server %SERVER_NAME% (%APP_NAME%) started at %TIMESTAMP%.\n%SUMMARY|1 event%',
                html_template = '<p>Server %SERVER_NAME% (%APP_NAME%) started at %TIMESTAMP%.</p><p>%SUMMARY|1 event%</p>'
            WHERE template_key = 'system.server_started';

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${TABLE}
            SET subject_template = '[%COUNT|1%x] MailRelayEvent databases_ready %SERVER_NAME%',
                text_template = 'Databases ready on %SERVER_NAME% (%APP_NAME%) at %TIMESTAMP%.\n%SUMMARY|1 event%',
                html_template = '<p>Databases ready on %SERVER_NAME% (%APP_NAME%) at %TIMESTAMP%.</p><p>%SUMMARY|1 event%</p>'
            WHERE template_key = 'system.databases_ready';

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Fix system event template COUNT/SUMMARY defaults'                  AS name,
        [=[
            # Forward Migration ${MIGRATION}: macro defaults for debounce templates
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})

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
            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Reverse 1282 template defaults'                                    AS name,
        [=[
            # Reverse Migration ${MIGRATION}
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})

return queries end
