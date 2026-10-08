-- Migration: acuranzo_1304.lua
-- Seed Reception user-notification mail templates (Band L Phase 55)
--
-- Prefs live on user_preferences (acuranzo_1301). Send sites (Phase 56,
-- CB-29, future weekly digest) must call H.mail / mailrelay_send_template
-- only when the matching notify_* flag is true. Design: FINISHLINE FL-51/55.
-- Data-only seed (no diagram).

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.0.0 - 2026-08-12 - Seed user.course_expiring / expired / new_course_announcement / weekly_summary

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "mail_templates"
cfg.MIGRATION = "1304"
-- ----------------------------------------------------------------------------
-- Forward: Insert Reception user-notification templates
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
                6,
                'user.course_expiring',
                'Course Access Expiring',
                1,
                'Your access to %COURSE_TITLE|your course% expires in %DAYS_LEFT|7% days',
                'Hi %FIRST_NAME|there%,\n\nYour access to %COURSE_TITLE|your course% ends on %EXPIRES_AT|soon% (%DAYS_LEFT|7% days left).\n\nOpen the course: %COURSE_URL|https://www.500courses.com/#my-courses%\nManage email preferences: %SETTINGS_URL|https://www.500courses.com/#account%\n\n%APP_NAME% · %TIMESTAMP%',
                '<p>Hi %FIRST_NAME|there%,</p><p>Your access to <strong>%COURSE_TITLE|your course%</strong> ends on %EXPIRES_AT|soon% (%DAYS_LEFT|7% days left).</p><p><a href="%COURSE_URL|https://www.500courses.com/#my-courses%">Open the course</a></p><p>Manage email preferences in <a href="%SETTINGS_URL|https://www.500courses.com/#account%">Account Settings</a>.</p><p>%APP_NAME% · %TIMESTAMP%</p>',
                '{}',
                ${COMMON_VALUES}
            ),
            (
                7,
                'user.course_expired',
                'Course Access Expired',
                1,
                'Your access to %COURSE_TITLE|your course% has ended',
                'Hi %FIRST_NAME|there%,\n\nYour access to %COURSE_TITLE|your course% ended on %EXPIRES_AT|recently%.\n\nBrowse courses: %CATALOG_URL|https://www.500courses.com/#courses%\nManage email preferences: %SETTINGS_URL|https://www.500courses.com/#account%\n\n%APP_NAME% · %TIMESTAMP%',
                '<p>Hi %FIRST_NAME|there%,</p><p>Your access to <strong>%COURSE_TITLE|your course%</strong> ended on %EXPIRES_AT|recently%.</p><p><a href="%CATALOG_URL|https://www.500courses.com/#courses%">Browse courses</a></p><p>Manage email preferences in <a href="%SETTINGS_URL|https://www.500courses.com/#account%">Account Settings</a>.</p><p>%APP_NAME% · %TIMESTAMP%</p>',
                '{}',
                ${COMMON_VALUES}
            ),
            (
                8,
                'user.new_course_announcement',
                'New Course Announcement',
                1,
                'New at 500 Courses: %COURSE_TITLE|a new course%',
                'Hi %FIRST_NAME|there%,\n\nA new course is available: %COURSE_TITLE|a new course%.\n\n%COURSE_BLURB|Take a look in the catalog.%\n\nView the course: %COURSE_URL|https://www.500courses.com/#courses%\nManage email preferences: %SETTINGS_URL|https://www.500courses.com/#account%\n\n%APP_NAME% · %TIMESTAMP%',
                '<p>Hi %FIRST_NAME|there%,</p><p>A new course is available: <strong>%COURSE_TITLE|a new course%</strong>.</p><p>%COURSE_BLURB|Take a look in the catalog.%</p><p><a href="%COURSE_URL|https://www.500courses.com/#courses%">View the course</a></p><p>Manage email preferences in <a href="%SETTINGS_URL|https://www.500courses.com/#account%">Account Settings</a>.</p><p>%APP_NAME% · %TIMESTAMP%</p>',
                '{}',
                ${COMMON_VALUES}
            ),
            (
                9,
                'user.weekly_summary',
                'Weekly Course Summary',
                1,
                'Your week at 500 Courses',
                'Hi %FIRST_NAME|there%,\n\nHere is your weekly summary.\n\n%SUMMARY|See your account for current enrollments and expiration dates.%\n\nMy Courses: %MY_COURSES_URL|https://www.500courses.com/#my-courses%\nManage email preferences: %SETTINGS_URL|https://www.500courses.com/#account%\n\n%APP_NAME% · %TIMESTAMP%',
                '<p>Hi %FIRST_NAME|there%,</p><p>Here is your weekly summary.</p><p>%SUMMARY|See your account for current enrollments and expiration dates.%</p><p><a href="%MY_COURSES_URL|https://www.500courses.com/#my-courses%">My Courses</a></p><p>Manage email preferences in <a href="%SETTINGS_URL|https://www.500courses.com/#account%">Account Settings</a>.</p><p>%APP_NAME% · %TIMESTAMP%</p>',
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
        'Seed Reception user-notification mail templates'                   AS name,
        [=[
            # Forward Migration ${MIGRATION}: Seed Reception user mail templates

            Inserts four active `mail_templates` rows used by Reception
            notification prefs (`user_preferences`, acuranzo_1301 / FL-51):

            | template_id | template_key | Pref |
            |-------------|--------------|------|
            | 6 | `user.course_expiring` | `notify_expiring` |
            | 7 | `user.course_expired` | `notify_expired` |
            | 8 | `user.new_course_announcement` | `notify_new_course` |
            | 9 | `user.weekly_summary` | `notify_weekly_summary` |

            Producer macros use `|default` so preview/render succeeds before
            a send site fills every key. Built-ins `%APP_NAME%` and
            `%TIMESTAMP%` come from Mail Relay context.

            Send sites (not this migration) must skip the send when the
            matching pref is false. Missing prefs row = COALESCE to column
            defaults (marketing off, transactional on). Admin/ops mail is
            not gated by these prefs.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})

-- ----------------------------------------------------------------------------
-- Reverse: Remove Reception user-notification templates
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
                'user.course_expiring',
                'user.course_expired',
                'user.new_course_announcement',
                'user.weekly_summary'
            );

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Remove Reception user-notification mail templates'                 AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Remove Reception user mail templates

            Deletes the four `user.*` templates seeded by the forward
            migration. Does not touch system/auth/test templates.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})

return queries end
