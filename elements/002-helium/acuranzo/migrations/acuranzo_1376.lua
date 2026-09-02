-- Migration: acuranzo_1376.lua
-- MCP Phase 14: seed System.Info (MCP tool calling H.system.info)
--
-- luacheck: no max line length
-- luacheck: no unused args
--
-- CHANGELOG
-- 1.0.0 - 2026-09-02 - System.Info tool; inputSchema empty object; readOnlyHint

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "scripts"
cfg.MIGRATION = "1376"
cfg.GROUP_NAME = "System"
cfg.SCRIPT_NAME = "Info"
-- ----------------------------------------------------------------------------
-- Forward: seed System.Info
-- ----------------------------------------------------------------------------
table.insert(queries,{sql=[====[

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
                invokable,
                mcp_access,
                mcp_schema,
                mcp_annotations,
                ${COMMON_FIELDS}
            )
            VALUES (
                '${GROUP_NAME}',
                '${SCRIPT_NAME}',
                1,
                NULL, NULL, NULL, NULL,
                1,
                [==[
-- System.Info (MCP Phase 14)
-- Returns full system status JSON (same collectors as GET /api/system/info).

local info = H.system.info()
if not info then
    H.set_result_json({
        content = { { type = "text", text = "system info unavailable" } },
        isError = true,
    })
    return 0
end

H.set_result_json({
    content = { { type = "text", text = "ok" } },
    structuredContent = info,
})
return 0
                ]==],
                'MCP tool: System.Info via H.system.info()',
                0,
                1,
                '{"inputSchema":{"type":"object","properties":{},"additionalProperties":false}}',
                '{"title":"System.Info","readOnlyHint":true,"idempotentHint":true}',
                ${COMMON_VALUES}
            );

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Seed System.Info MCP tool'                                         AS name,
        [=[
            # Forward Migration ${MIGRATION}: Seed System.Info

            MCP tool in the `System` group that calls `H.system.info()` and
            returns the decoded system-status object as `structuredContent`.

            - `script_type = 1` (MCP_TOOL)
            - `invokable = 0` (MCP-only, not HTTP-invokable)
            - `mcp_access = 1` (QueryRef #153 allowlist)
            - `inputSchema`: empty object (no arguments accepted)
            - `readOnlyHint + idempotentHint`: true (read-only system state)
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]====]})

-- ----------------------------------------------------------------------------
-- Reverse
-- ----------------------------------------------------------------------------
table.insert(queries,{sql=[====[

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
            WHERE group_name = 'System'
              AND script_name = 'Info';

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Remove System.Info MCP tool'                                       AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Remove System.Info
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]====]})

return queries end
