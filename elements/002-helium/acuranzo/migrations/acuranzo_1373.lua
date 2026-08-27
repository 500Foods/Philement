-- Migration: acuranzo_1373.lua
-- MCP Phase 15: extend Mcp.Server with resources/* and prompts/*
--
-- luacheck: no max line length
-- luacheck: no unused args
--
-- CHANGELOG
-- 1.0.0 - 2026-08-27 - resources/list, resources/read, prompts/list, prompts/get
--                      Kind from mcp_schema.uri / arguments; no mcp_kind column

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "scripts"
cfg.MIGRATION = "1373"
cfg.GROUP_NAME = "Mcp"
cfg.SCRIPT_NAME = "Server"
-- ----------------------------------------------------------------------------
-- Forward: replace Mcp.Server body (1369 + resources/prompts)
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
            UPDATE ${SCHEMA}scripts
            SET code = [==[
-- Mcp.Server (MCP Phase 15)
-- Lua owns MCP methods. C only validates the JSON-RPC envelope.
-- mcp_access=1, invokable=0.
-- Kind from mcp_schema (no mcp_kind column, no extra dots in Group.Name):
-- uri → resource; arguments without inputSchema → prompt; else tool.

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

local function is_resource_row(row)
    return type(row) == "table" and type(row.schema) == "table"
        and type(row.schema.uri) == "string" and row.schema.uri ~= ""
end

local function is_prompt_row(row)
    return type(row) == "table" and type(row.schema) == "table"
        and type(row.schema.arguments) == "table"
        and type(row.schema.inputSchema) ~= "table"
end

local function is_tool_row(row)
    local name = row and row.name
    return type(name) == "string" and name ~= "" and not HIDE[name]
        and not is_resource_row(row) and not is_prompt_row(row)
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

local function list_page(cursor, page_size)
    local rows, next_cursor = H.mcp.list(cursor, page_size)
    if type(rows) ~= "table" then
        return {}, next_cursor
    end
    return rows, next_cursor
end

local function resource_uri(row)
    if type(row.schema) == "table" and type(row.schema.uri) == "string" and row.schema.uri ~= "" then
        return row.schema.uri
    end
    if type(row.script) == "string" and row.script ~= "" then
        return "hydrogen://mcp/" .. string.lower(row.script)
    end
    return "hydrogen://mcp/resource"
end

local function resource_mime(row)
    if type(row.schema) == "table" and type(row.schema.mimeType) == "string" and row.schema.mimeType ~= "" then
        return row.schema.mimeType
    end
    return "text/plain"
end

local function resource_label(row)
    if type(row.schema) == "table" and type(row.schema.name) == "string" and row.schema.name ~= "" then
        return row.schema.name
    end
    return row.script or row.name or "resource"
end

local function prompt_arguments(row)
    if type(row.schema) == "table" and type(row.schema.arguments) == "table" then
        return row.schema.arguments
    end
    return {}
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
        capabilities = {
            tools = { listChanged = false },
            resources = { subscribe = false, listChanged = false },
            prompts = { listChanged = false },
        },
        serverInfo = { name = "hydrogen", version = "1.0.0" },
        instructions = "Hydrogen MCP server. Tools are named Group.Name. Resources declare mcp_schema.uri; prompts declare mcp_schema.arguments. Writes are explicit (see annotations). Results may be truncated at MaxResultBytes. params._hydrogen is server-injected; do not send it. There is no generic SQL tool.",
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
    local rows, next_cursor = list_page(cursor, page_size)
    local tools = {}
    for i = 1, #rows do
        local row = rows[i]
        if is_tool_row(row) then
            local name = row.name
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
    local listed = list_page(1, 50)
    for i = 1, #listed do
        local row = listed[i]
        if row and row.name == name and not is_tool_row(row) then
            rpc_result(id, tool_error({ text("not found") }))
            return 0
        end
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

if method == "resources/list" then
    local cursor = nil
    local page_size = nil
    if type(mcp_params) == "table" then
        cursor = mcp_params.cursor
        page_size = mcp_params.page_size
    end
    local rows, next_cursor = list_page(cursor, page_size)
    local resources = {}
    for i = 1, #rows do
        local row = rows[i]
        if is_resource_row(row) then
            local item = {
                uri = resource_uri(row),
                name = resource_label(row),
                description = row.summary or "",
                mimeType = resource_mime(row),
            }
            if type(row.annotations) == "table" then
                item.annotations = row.annotations
            end
            resources[#resources + 1] = item
        end
    end
    local listed = { resources = resources }
    if next_cursor ~= nil then
        listed.nextCursor = tostring(next_cursor)
    end
    rpc_result(id, listed)
    return 0
end

if method == "resources/read" then
    local uri = nil
    if type(mcp_params) == "table" then
        uri = mcp_params.uri
    end
    if type(uri) ~= "string" or uri == "" then
        rpc_error(id, -32602, "Invalid params")
        return 0
    end
    local rows = list_page(1, 50)
    local target = nil
    for i = 1, #rows do
        local row = rows[i]
        if is_resource_row(row) and resource_uri(row) == uri then
            target = row.name
            break
        end
    end
    if not target then
        rpc_error(id, -32602, "Unknown resource")
        return 0
    end
    local result, err = H.mcp.call(target, { uri = uri })
    if err then
        rpc_error(id, -32603, err)
        return 0
    end
    if type(result) ~= "table" or type(result.contents) ~= "table" then
        rpc_error(id, -32603, "Invalid resource contents")
        return 0
    end
    if estimate(result, 0) > MAX_RESULT_BYTES then
        rpc_error(id, -32603, "truncated: result exceeded MaxResultBytes")
        return 0
    end
    rpc_result(id, result)
    return 0
end

if method == "prompts/list" then
    local cursor = nil
    local page_size = nil
    if type(mcp_params) == "table" then
        cursor = mcp_params.cursor
        page_size = mcp_params.page_size
    end
    local rows, next_cursor = list_page(cursor, page_size)
    local prompts = {}
    for i = 1, #rows do
        local row = rows[i]
        if is_prompt_row(row) then
            prompts[#prompts + 1] = {
                name = row.name,
                description = row.summary or "",
                arguments = prompt_arguments(row),
            }
        end
    end
    local listed = { prompts = prompts }
    if next_cursor ~= nil then
        listed.nextCursor = tostring(next_cursor)
    end
    rpc_result(id, listed)
    return 0
end

if method == "prompts/get" then
    local name = nil
    local args = nil
    if type(mcp_params) == "table" then
        name = mcp_params.name
        args = mcp_params.arguments
    end
    if type(name) ~= "string" or name == "" then
        rpc_error(id, -32602, "Unknown prompt")
        return 0
    end
    local listed = list_page(1, 50)
    local found = false
    for i = 1, #listed do
        if is_prompt_row(listed[i]) and listed[i].name == name then
            found = true
            break
        end
    end
    if not found then
        rpc_error(id, -32602, "Unknown prompt")
        return 0
    end
    local result, err = H.mcp.call(name, args)
    if err then
        rpc_error(id, -32603, err)
        return 0
    end
    if type(result) ~= "table" or type(result.messages) ~= "table" then
        rpc_error(id, -32603, "Invalid prompt messages")
        return 0
    end
    if estimate(result, 0) > MAX_RESULT_BYTES then
        rpc_error(id, -32603, "truncated: result exceeded MaxResultBytes")
        return 0
    end
    rpc_result(id, result)
    return 0
end

rpc_error(id, -32601, "Method not found")
return 0
                ]==],
                summary = 'MCP protocol script (initialize, ping, tools/list, tools/call, resources/*, prompts/*)'
            WHERE group_name = '${GROUP_NAME}'
              AND script_name = '${SCRIPT_NAME}';

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Extend Mcp.Server with resources and prompts'                      AS name,
        [=[
            # Forward Migration ${MIGRATION}: Mcp.Server resources/prompts

            Adds `resources/list`, `resources/read`, `prompts/list`,
            `prompts/get`. Kind is `mcp_schema.uri` vs `mcp_schema.arguments`
            (Group.Name stays two segments for `H.mcp.call`). No `mcp_kind`.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]====]})

-- ----------------------------------------------------------------------------
-- Reverse: restore Phase 10 Mcp.Server body (acuranzo_1369)
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
            UPDATE ${SCHEMA}scripts
            SET code = [==[
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
                summary = 'MCP protocol script (initialize, ping, tools/list, tools/call)'
            WHERE group_name = '${GROUP_NAME}'
              AND script_name = '${SCRIPT_NAME}';

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Restore Phase 10 Mcp.Server body'                                  AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Restore Mcp.Server 1369
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]====]})

return queries end
