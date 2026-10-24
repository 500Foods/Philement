-- Migration: courses_4109.lua
-- Mail template course.suggestion_received
--
-- One receipt note to the person who suggested the course.
-- The send site attempts it once and records sent or failed
-- on course_build_runs.receipt_mail. No diagram.
--
-- luacheck: no max line length
-- luacheck: no unused args
--
-- CHANGELOG
-- 1.0.0 - 2026-10-23 - Seed course.suggestion_received

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "mail_templates"
cfg.MIGRATION = "4109"
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
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
            SELECT
                n.new_template_id,
                'course.suggestion_received',
                'Suggestion received',
                1,
                'We received your course suggestion',
                'Hi %FIRST_NAME|there%,\n\nWe received your suggestion, %COURSE_NAME|a course%.\n\n%APP_NAME% · %TIMESTAMP%',
                '<p>Hi %FIRST_NAME|there%,</p><p>We received your suggestion, <strong>%COURSE_NAME|a course%</strong>.</p><p>%APP_NAME% · %TIMESTAMP%</p>',
                '{}',
                ${COMMON_VALUES}
            FROM (
                SELECT COALESCE(MAX(template_id), 0) + 1 AS new_template_id
                FROM ${SCHEMA}${TABLE}
            ) n
            WHERE NOT EXISTS (
                SELECT 1
                  FROM ${SCHEMA}${TABLE} existing
                 WHERE existing.template_key = 'course.suggestion_received'
            );

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Seed course.suggestion_received mail template'                     AS name,
        [=[
            # Forward Migration ${MIGRATION}: Seed course.suggestion_received

            Active template for the one receipt attempt. Macros are
            FIRST_NAME and COURSE_NAME. Built-ins APP_NAME and
            TIMESTAMP come from the mail relay. template_id is MAX+1.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
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
            WHERE template_key = 'course.suggestion_received';

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Remove course.suggestion_received mail template'                   AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Remove course.suggestion_received

            Deletes that template key only.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
return queries end
