-- Migration: acuranzo_1341.lua
-- PRIORITIZE 2.1: ops.canvas_user_unlinked mail template
--
-- Admin-only. Not gated by user_preferences. To-address is
-- CANVAS_PROVISION_ALERT_EMAIL (EnsureCanvasUser 1342), not a
-- learner. Data-only seed (no diagram).
--
-- luacheck: no max line length
-- luacheck: no unused args
--
-- CHANGELOG
-- 1.0.0 - 2026-08-21 - Seed ops.canvas_user_unlinked (PRIORITIZE 2.1)

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "mail_templates"
cfg.MIGRATION = "1341"
-- ----------------------------------------------------------------------------
-- Forward: Insert ops unlinked-Canvas template
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
                10,
                'ops.canvas_user_unlinked',
                'Ops Canvas User Unlinked',
                1,
                'Canvas user still unlinked after 2m: account %ACCOUNT_ID|?%',
                'Provision.EnsureCanvasUser could not attach a Canvas user after 2 minutes.\n\naccount_id: %ACCOUNT_ID|?%\nemail: %EMAIL|?%\nreason: %REASON|not_found%\n\nJIT stays on. Lua will keep polling (15s) and will not create a second Canvas user.\n\n%APP_NAME% · %TIMESTAMP%',
                '<p><code>Provision.EnsureCanvasUser</code> could not attach a Canvas user after 2 minutes.</p><p>account_id: %ACCOUNT_ID|?%<br>email: %EMAIL|?%<br>reason: %REASON|not_found%</p><p>JIT stays on. Lua will keep polling (15s) and will not create a second Canvas user.</p><p>%APP_NAME% · %TIMESTAMP%</p>',
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
        'Seed ops.canvas_user_unlinked mail template'                       AS name,
        [=[
            # Forward Migration ${MIGRATION}: Seed ops Canvas unlinked template

            PRIORITIZE 2.1. `template_id` 10, key `ops.canvas_user_unlinked`.
            Macros: `%ACCOUNT_ID%`, `%EMAIL%`, `%REASON%` plus Mail Relay
            `%APP_NAME%` / `%TIMESTAMP%`. Admin/ops mail — not a user pref.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})
-- ----------------------------------------------------------------------------
-- Reverse: Remove ops unlinked-Canvas template
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
            WHERE template_key = 'ops.canvas_user_unlinked';

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Remove ops.canvas_user_unlinked mail template'                     AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Remove ops Canvas unlinked template

            Deletes template_key `ops.canvas_user_unlinked` only.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})

return queries end
