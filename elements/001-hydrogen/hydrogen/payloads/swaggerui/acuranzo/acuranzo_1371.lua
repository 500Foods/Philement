-- Migration: acuranzo_1371.lua
-- MCP Phase 10: seed Mcp.EchoStrict (tool-level isError on bad args)
--
-- luacheck: no max line length
-- luacheck: no unused args
--
-- CHANGELOG
-- 1.0.0 - 2026-08-26 - Validate message; tool_error on mismatch

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "scripts"
cfg.MIGRATION = "1371"
cfg.GROUP_NAME = "Mcp"
cfg.SCRIPT_NAME = "EchoStrict"
-- ----------------------------------------------------------------------------
-- Forward: seed Mcp.EchoStrict
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
-- Mcp.EchoStrict (MCP Phase 10 fixture)
-- Validates arguments against inputSchema. Failures are result.isError
-- (Design Principle 12), never JSON-RPC error.

local function text(str)
    return { type = "text", text = tostring(str or "") }
end

local function tool_error(content_blocks)
    return { isError = true, content = content_blocks }
end

local message = nil
if type(params) == "table" then
    message = params.message
    for k, _ in pairs(params) do
        if k ~= "message" and k ~= "_hydrogen" then
            H.set_result_json(tool_error({ text("invalid arguments: unexpected field") }))
            return 0
        end
    end
end

if type(message) ~= "string" or message == "" then
    H.set_result_json(tool_error({ text("invalid arguments: message (string) required") }))
    return 0
end

H.set_result_json({
    content = { text(message) },
    structuredContent = { message = message },
})
return 0
                ]==],
                'MCP fixture: echo with inputSchema validation (tool_error on mismatch)',
                0,
                1,
                '{"inputSchema":{"type":"object","properties":{"message":{"type":"string"}},"required":["message"],"additionalProperties":false},"outputSchema":{"type":"object","properties":{"message":{"type":"string"}}}}',
                '{"title":"Echo Strict","readOnlyHint":true,"idempotentHint":true}',
                ${COMMON_VALUES}
            );

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Seed Mcp.EchoStrict fixture tool'                                  AS name,
        [=[
            # Forward Migration ${MIGRATION}: Seed Mcp.EchoStrict

            Validates `message` (string, required). Mismatch returns
            `result.isError`, not JSON-RPC `error`. Test 47 tool-failure shape.
            `mcp_access=1`, `invokable=0`.
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
            WHERE group_name = 'Mcp'
              AND script_name = 'EchoStrict';

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Remove Mcp.EchoStrict fixture tool'                                AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Remove Mcp.EchoStrict
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]====]})

return queries end
