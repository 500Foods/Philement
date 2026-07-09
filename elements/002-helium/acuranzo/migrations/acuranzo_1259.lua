-- Migration: acuranzo_1259.lua
-- Seed the mail.test template

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.0.0 - 2026-07-08 - Initial creation for MAILRELAY_PLAN Phase 7.5b

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "mail_templates"
cfg.MIGRATION = "1259"
-- ----------------------------------------------------------------------------
-- Forward: Insert the mail.test template
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
            VALUES (
                1,
                'mail.test',
                'Mail Relay Test Template',
                1,                              -- Active (Template Status lookup 064)
                'Hello %NAME% from %APP_NAME%',
                'Hello %NAME%,\n\nThis is a test message from %APP_NAME% (%SERVER_NAME%) at %TIMESTAMP%.',
                '<p>Hello %NAME%,</p><p>This is a test message from %APP_NAME% (%SERVER_NAME%) at %TIMESTAMP%.</p>',
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
        'Seed mail.test Template'                                           AS name,
        [=[
            # Forward Migration ${MIGRATION}: Seed mail.test Template

            Inserts the `mail.test` template used by Mail Relay blackbox and
            integration tests. The template exercises built-in macros
            (%APP_NAME%, %SERVER_NAME%, %TIMESTAMP%) and a user-supplied
            %NAME% macro.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})

-- ----------------------------------------------------------------------------
-- Reverse: Remove the mail.test template
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
            WHERE template_key = 'mail.test';

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Remove mail.test Template'                                         AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Remove mail.test Template

            Deletes the `mail.test` template seeded by the forward migration.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})

return queries end
