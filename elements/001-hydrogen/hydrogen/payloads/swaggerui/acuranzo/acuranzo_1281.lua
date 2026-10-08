-- Migration: acuranzo_1281.lua
-- Seed Mail.Events.* handler scripts for system mail events

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.0.0 - 2026-07-28 - Default event handlers loadable via Events.Rules

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "scripts"
cfg.MIGRATION = "1281"
-- ----------------------------------------------------------------------------
-- Forward: Insert Mail.Events scripts
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
            INSERT INTO ${SCHEMA}scripts (
                group_name,
                script_name,
                script_type,
                schedule,
                next_run,
                last_run_start,
                last_run_end,
                status,
                code,
                summary,
                ${COMMON_FIELDS}
            )
            VALUES
            (
                'Mail.Events',
                'ServerStarted',
                1,
                NULL, NULL, NULL, NULL,
                1,
                [==[
                    function handle_event(event)
                        local recipients = event.admin_recipients or {}
                        if #recipients == 0 then
                            return nil
                        end
                        local params = {
                            SERVER_NAME = event.server_name or "",
                            APP_NAME = event.app_name or "",
                            TIMESTAMP = event.timestamp or "",
                            COUNT = "1",
                            SUMMARY = "1 event"
                        }
                        if event.params then
                            for k, v in pairs(event.params) do
                                params[k] = v
                            end
                        end
                        return {
                            template_key = "system.server_started",
                            to = recipients,
                            params = params,
                            debounce_key = "system.lifecycle"
                        }
                    end
                ]==],
                'Mail event handler: system.server_started',
                ${COMMON_VALUES}
            ),
            (
                'Mail.Events',
                'DatabasesReady',
                1,
                NULL, NULL, NULL, NULL,
                1,
                [==[
                    function handle_event(event)
                        local recipients = event.admin_recipients or {}
                        if #recipients == 0 then
                            return nil
                        end
                        local params = {
                            SERVER_NAME = event.server_name or "",
                            APP_NAME = event.app_name or "",
                            TIMESTAMP = event.timestamp or "",
                            COUNT = "1",
                            SUMMARY = "1 event"
                        }
                        if event.params then
                            for k, v in pairs(event.params) do
                                params[k] = v
                            end
                        end
                        return {
                            template_key = "system.databases_ready",
                            to = recipients,
                            params = params,
                            debounce_key = "system.lifecycle"
                        }
                    end
                ]==],
                'Mail event handler: system.databases_ready',
                ${COMMON_VALUES}
            ),
            (
                'Mail.Events',
                'ServerStopped',
                1,
                NULL, NULL, NULL, NULL,
                1,
                [==[
                    function handle_event(event)
                        local recipients = event.admin_recipients or {}
                        if #recipients == 0 then
                            return nil
                        end
                        local params = {
                            SERVER_NAME = event.server_name or "",
                            APP_NAME = event.app_name or "",
                            TIMESTAMP = event.timestamp or ""
                        }
                        if event.params then
                            for k, v in pairs(event.params) do
                                params[k] = v
                            end
                        end
                        return {
                            template_key = "system.server_stopped",
                            to = recipients,
                            params = params
                        }
                    end
                ]==],
                'Mail event handler: system.server_stopped',
                ${COMMON_VALUES}
            );

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Seed Mail.Events handler scripts'                                  AS name,
        [=[
            # Forward Migration ${MIGRATION}: Seed Mail.Events handler scripts

            Inserts scripts referenced by MailRelay.Events.Rules:
            Mail.Events.ServerStarted, Mail.Events.DatabasesReady,
            Mail.Events.ServerStopped. Loaded via QueryRef #087.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})

-- ----------------------------------------------------------------------------
-- Reverse
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
            DELETE FROM ${SCHEMA}scripts
            WHERE group_name = 'Mail.Events'
              AND script_name IN ('ServerStarted', 'DatabasesReady', 'ServerStopped');

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Remove Mail.Events handler scripts'                                AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Remove Mail.Events handler scripts
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})

return queries end
