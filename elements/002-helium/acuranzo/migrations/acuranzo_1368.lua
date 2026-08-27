-- Migration: acuranzo_1368.lua
-- MCP Phase 10: seed Mcp.Helpers (content-block / tool_error shapes)
--
-- luacheck: no max line length
-- luacheck: no unused args
--
-- CHANGELOG
-- 1.0.0 - 2026-08-26 - Pure-Lua MCP content-block helpers (not C)

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "scripts"
cfg.MIGRATION = "1368"
cfg.GROUP_NAME = "Mcp"
cfg.SCRIPT_NAME = "Helpers"
-- ----------------------------------------------------------------------------
-- Forward: seed Mcp.Helpers
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
-- Mcp.Helpers (MCP Phase 10)
-- Pure-Lua content-block builders. Not C host functions.
-- Call via H.mcp.call("Mcp.Helpers", { op = "text"|"image"|"audio"|"resource_link"|"tool_error", ... }).
-- Mcp.Server omits this name from tools/list.

local function text(str)
    return { type = "text", text = tostring(str or "") }
end

local function image(data_b64, mime_type)
    return { type = "image", data = tostring(data_b64 or ""), mimeType = tostring(mime_type or "image/png") }
end

local function audio(data_b64, mime_type)
    return { type = "audio", data = tostring(data_b64 or ""), mimeType = tostring(mime_type or "audio/mpeg") }
end

local function resource_link(uri, name, mime_type)
    local block = { type = "resource_link", uri = tostring(uri or ""), name = tostring(name or "") }
    if mime_type then
        block.mimeType = tostring(mime_type)
    end
    return block
end

local function tool_error(content_blocks)
    local blocks = content_blocks
    if type(blocks) ~= "table" then
        blocks = { text(tostring(blocks or "error")) }
    end
    return { isError = true, content = blocks }
end

local op = nil
if type(params) == "table" then
    op = params.op
end

local out
if op == "text" then
    out = text(params.str)
elseif op == "image" then
    out = image(params.data, params.mime_type)
elseif op == "audio" then
    out = audio(params.data, params.mime_type)
elseif op == "resource_link" then
    out = resource_link(params.uri, params.name, params.mime_type)
elseif op == "tool_error" then
    if type(params.content) == "table" then
        out = tool_error(params.content)
    else
        out = tool_error({ text(params.message or "error") })
    end
else
    out = tool_error({ text("unknown op") })
end

H.set_result_json(out)
return 0
                ]==],
                'MCP content-block helpers (text/image/audio/resource_link/tool_error)',
                0,
                1,
                '{"inputSchema":{"type":"object","properties":{"op":{"type":"string"},"str":{"type":"string"},"data":{"type":"string"},"mime_type":{"type":"string"},"uri":{"type":"string"},"name":{"type":"string"},"message":{"type":"string"},"content":{"type":"array"}},"required":["op"]}}',
                '{"title":"MCP Helpers","readOnlyHint":true,"idempotentHint":true}',
                ${COMMON_VALUES}
            );

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Seed Mcp.Helpers script'                                           AS name,
        [=[
            # Forward Migration ${MIGRATION}: Seed Mcp.Helpers

            Pure-Lua MCP content-block and `tool_error` shapes. Not C.
            `mcp_access=1`, `invokable=0`. Filtered out of `tools/list`.
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
              AND script_name = 'Helpers';

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Remove Mcp.Helpers script'                                         AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Remove Mcp.Helpers
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]====]})

return queries end
