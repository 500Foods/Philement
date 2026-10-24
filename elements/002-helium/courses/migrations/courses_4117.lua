-- Migration: courses_4117.lua
-- Receipt body for course.suggestion_received
--
-- Does not edit courses_4109.lua. Same template row.
-- Subject and display name stay. Reverse restores the short body.
-- FIRST_NAME is the suggestion submitter_name. COURSE_NAME is
-- the suggestion course_name. Neither value comes from a JWT.
-- The next send after apply uses these words. Queued mail stays.
--
-- luacheck: no max line length
-- luacheck: no unused args
--
-- CHANGELOG
-- 1.0.0 - 2026-10-23 - Receipt body names the course and the outcome

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "mail_templates"
cfg.MIGRATION = "4117"
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
            UPDATE ${SCHEMA}${TABLE}
            SET text_template = 'Hi %FIRST_NAME|there%,

We received your suggestion, %COURSE_NAME|a course%.

You will receive updates as the course progresses through its evaluation and building phases.

Not all course suggestions make it through to a final course, for various reasons. You will be notified of its final disposition either way.

%APP_NAME% · %TIMESTAMP%',
                html_template = '<p>Hi %FIRST_NAME|there%,</p><p>We received your suggestion, <strong>%COURSE_NAME|a course%</strong>.</p><p>You will receive updates as the course progresses through its evaluation and building phases.</p><p>Not all course suggestions make it through to a final course, for various reasons. You will be notified of its final disposition either way.</p><p>%APP_NAME% · %TIMESTAMP%</p>'
            WHERE template_key = 'course.suggestion_received';

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Receipt body names the course and the outcome'                     AS name,
        [=[
            # Forward Migration ${MIGRATION}: Receipt body

            Replaces the text and HTML of course.suggestion_received.
            Subject stays "We received your course suggestion".
            FIRST_NAME is the suggestion submitter_name. COURSE_NAME
            is the suggestion course_name. The send does not read a JWT.
            A message already queued keeps the body it was given.
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
            UPDATE ${SCHEMA}${TABLE}
            SET text_template = 'Hi %FIRST_NAME|there%,\n\nWe received your suggestion, %COURSE_NAME|a course%.\n\n%APP_NAME% · %TIMESTAMP%',
                html_template = '<p>Hi %FIRST_NAME|there%,</p><p>We received your suggestion, <strong>%COURSE_NAME|a course%</strong>.</p><p>%APP_NAME% · %TIMESTAMP%</p>'
            WHERE template_key = 'course.suggestion_received';

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Restore the short course.suggestion_received body'                 AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Restore the short receipt body

            Puts the courses_4109 text and HTML back. Does not delete
            the template row.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
return queries end
