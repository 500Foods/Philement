-- Migration: acuranzo_1369.lua
-- MCP Phase 10: seed Mcp.Server protocol script
--
-- luacheck: no max line length
-- luacheck: no unused args
--
-- CHANGELOG
-- 1.0.0 - 2026-08-26 - initialize / ping / tools/list / tools/call

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "scripts"
cfg.MIGRATION = "1369"
cfg.GROUP_NAME = "Mcp"
cfg.SCRIPT_NAME = "Server"
-- ----------------------------------------------------------------------------
-- Forward: seed Mcp.Server
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
-- Mcp.Server (MCP Phase 10)
-- Lua owns MCP methods. C only validates the JSON-RPC envelope.
-- mcp_access=1, invokable=0.

local MAX_RESULT_BYTES = 262144
local PROTOCOL_VERSION = "2025-06-18"
local HIDE = { ["Mcp.Server"] = true, ["Mcp.Helpers"] = true }

local function text(str)
    return { type = "text", text = tostring(str or "") }
end

local function tool_error(content_blocks)
    local blocks = content_blocks
    if type(blocks) ~= "table" then
        blocks = { text(tostring(blocks or "error")) }
    end
    return { isError = true, content = blocks }
end

local function estimate(v, n)
    if n > MAX_RESULT_BYTES then
        return n
    end
    local t = type(v)
    if t == "string" then
        return n + #v
    elseif t == "number" or t == "boolean" then
        return n + 16
    elseif t == "table" then
        for k, val in pairs(v) do
            n = estimate(k, n)
            n = estimate(val, n)
            if n > MAX_RESULT_BYTES then
                return n
            end
        end
        return n
    end
    return n
end

local function rpc_result(id, result)
    H.set_result_json({ jsonrpc = "2.0", id = id, result = result })
end

local function rpc_error(id, code, message)
    H.set_result_json({
        jsonrpc = "2.0",
        id = id,
        error = { code = code, message = message },
    })
end

local method = params and params.method or ""
local id = params and params.id
local mcp_params = params and params.params
local hydrogen = params and params._hydrogen

if method == "notifications/initialized" or method == "notifications/cancelled" then
    return 0
end

if method == "initialize" then
    local requested = PROTOCOL_VERSION
    if type(mcp_params) == "table" and type(mcp_params.protocolVersion) == "string" then
        requested = mcp_params.protocolVersion
    elseif type(hydrogen) == "table" and type(hydrogen.protocol_version) == "string" then
        requested = hydrogen.protocol_version
    end
    rpc_result(id, {
        protocolVersion = requested,
        capabilities = { tools = { listChanged = false } },
        serverInfo = { name = "hydrogen", version = "1.0.0" },
        instructions = "Hydrogen MCP server. Tools are named Group.Name. Writes are explicit (see annotations). Results may be truncated at MaxResultBytes. params._hydrogen is server-injected; do not send it. There is no generic SQL tool.",
    })
    return 0
end

if method == "ping" then
    rpc_result(id, {})
    return 0
end

if method == "tools/list" then
    local cursor = nil
    local page_size = nil
    if type(mcp_params) == "table" then
        cursor = mcp_params.cursor
        page_size = mcp_params.page_size
    end
    local rows, next_cursor = H.mcp.list(cursor, page_size)
    local tools = {}
    if type(rows) == "table" then
        for i = 1, #rows do
            local row = rows[i]
            local name = row and row.name
            if name and not HIDE[name] then
                local input_schema = { type = "object" }
                if type(row.schema) == "table" and type(row.schema.inputSchema) == "table" then
                    input_schema = row.schema.inputSchema
                end
                local tool = {
                    name = name,
                    description = row.summary or "",
                    inputSchema = input_schema,
                }
                if type(row.annotations) == "table" then
                    tool.annotations = row.annotations
                end
                tools[#tools + 1] = tool
            end
        end
    end
    local listed = { tools = tools }
    if next_cursor ~= nil then
        listed.nextCursor = tostring(next_cursor)
    end
    rpc_result(id, listed)
    return 0
end

if method == "tools/call" then
    local name = nil
    local args = nil
    if type(mcp_params) == "table" then
        name = mcp_params.name
        args = mcp_params.arguments
    end
    if type(name) ~= "string" or name == "" then
        rpc_result(id, tool_error({ text("missing tool name") }))
        return 0
    end
    local result, err = H.mcp.call(name, args)
    if err then
        rpc_result(id, tool_error({ text(err) }))
        return 0
    end
    local shaped
    if type(result) ~= "table" then
        shaped = { content = { text(tostring(result)) } }
    elseif result.content ~= nil or result.isError == true then
        shaped = result
    else
        shaped = {
            content = { text("ok") },
            structuredContent = result,
        }
    end
    if estimate(shaped, 0) > MAX_RESULT_BYTES then
        shaped = tool_error({ text("truncated: result exceeded MaxResultBytes") })
    end
    rpc_result(id, shaped)
    return 0
end

rpc_error(id, -32601, "Method not found")
return 0
                ]==],
                'MCP protocol script (initialize, ping, tools/list, tools/call)',
                0,
                1,
                NULL,
                NULL,
                ${COMMON_VALUES}
            );

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Seed Mcp.Server protocol script'                                   AS name,
        [=[
            # Forward Migration ${MIGRATION}: Seed Mcp.Server

            Lua MCP protocol: initialize (instructions + tools.listChanged=false),
            ping, tools/list, tools/call, ignore cancelled/initialized.
            Unknown tools return result.isError, not JSON-RPC error.
            Truncates oversized tool results at 256 KiB (config default).
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
              AND script_name = 'Server';

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Remove Mcp.Server protocol script'                                 AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Remove Mcp.Server
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]====]})

return queries end
