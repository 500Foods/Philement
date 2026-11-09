-- Migration: acuranzo_1470.lua
-- Reports.Render.Data (Reporting)
-- Datasources, binds, query rows, the driving datasource, and parameter
-- items. Always loaded.
--
-- One module of the split Reports.Render. Reports.Render (acuranzo_1479.lua)
-- reads this row with H.altquery_sync and runs it with load() in its
-- per-request environment. The row is not invokable and has no MCP access.
-- Reverse deletes the row. QueryRef 175 stays unused. No lookup.
--
-- luacheck: no max line length
-- luacheck: no unused args
--
-- CHANGELOG
-- 1.0.0 - 2026-11-09 - Split from the acuranzo_1479.lua Reports.Render body

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "scripts"
cfg.MIGRATION = "1470"
cfg.GROUP_NAME = "Reports"
cfg.SCRIPT_NAME = "Render.Data"

local sql_new = [==[
-- Reports.Render.Data (Reporting). Always loaded at the rows step: datasources, binds, query rows, the driver, parameter items.
-- Loaded by Reports.Render into its per-request environment.
-- Names defined here are shared with Reports.Render and its other modules.

-- Rows: the first detail band with a datasource drives the report.
rows, driver = nil, nil
function reporting_types(collection)
    local doc = collection
    if type(doc) == "string" then
        if doc == "" then return {} end
        local parsed, jerr = json_decode(doc)
        if jerr or type(parsed) ~= "table" then return {} end
        doc = parsed
    end
    if type(doc) ~= "table" then return {} end
    if doc.reporting == nil and type(doc[1]) == "table" then
        doc = doc[1]
    end
    local rep = type(doc) == "table" and doc.reporting or nil
    if type(rep) ~= "table" or type(rep.parameters) ~= "table" then
        return {}
    end
    local types = {}
    for _, p in ipairs(rep.parameters) do
        if type(p) == "table" and type(p.name) == "string" and p.kind ~= "calculated" then
            types[p.name] = tostring(p.type or "string"):lower()
        end
    end
    return types
end

function definition_defaults()
    local map = {}
    local list = def.parameters
    if type(list) ~= "table" then return map end
    for _, p in ipairs(list) do
        if type(p) == "table" and type(p.name) == "string" then
            map[p.name] = p.default
        end
    end
    return map
end

function coerce_bind(name, value, types)
    if value == nil then return nil end
    local typ = types[name]
    if typ == nil then return value end
    if typ == "integer" or typ == "int" then
        local n = tonumber(value)
        if n == nil or n ~= n or n == math.huge or n == -math.huge then return value end
        return math.floor(n)
    end
    if typ == "float" or typ == "number" then
        local n = tonumber(value)
        if n == nil or n ~= n or n == math.huge or n == -math.huge then return value end
        return n
    end
    if typ == "boolean" or typ == "bool" then
        return value == true or value == 1 or value == "1" or value == "true"
    end
    if typ == "string" or typ == "text" or typ == "memo" or typ == "date" or typ == "datetime" then
        return plain(value)
    end
    return value
end

function resolve_raw(raw, overrides, defaults)
    if type(raw) ~= "table" then return raw end
    if raw.parameter ~= nil then
        local pname = tostring(raw.parameter)
        if overrides[pname] ~= nil then return overrides[pname] end
        if defaults[pname] ~= nil then return defaults[pname] end
        return nil
    end
    if raw.literal ~= nil then return raw.literal end
    return nil
end

function build_binds(ds, types)
    local overrides = type(P.parameters) == "table" and P.parameters or {}
    local defaults = definition_defaults()
    local stored = type(ds.params) == "table" and ds.params or {}
    local binds = {}
    local missing = false
    for name, raw in pairs(stored) do
        if type(name) == "string" then
            local value
            if overrides[name] ~= nil then
                value = overrides[name]
            else
                value = resolve_raw(raw, overrides, defaults)
            end
            value = coerce_bind(name, value, types)
            if value == nil then
                missing = true
                add_log("warn", "datasource " .. tostring(ds.name) .. " parameter " .. name
                    .. " is not set; no rows")
            else
                binds[name] = value
            end
        end
    end
    return binds, missing
end

function session_database()
    local hy = type(P._hydrogen) == "table" and P._hydrogen or {}
    local database = hy.database
    if type(database) ~= "string" or database == "" then return nil end
    return database
end

function unconfigured(message)
    local s = tostring(message or "")
    if s:find("database_name is empty", 1, true) then return true end
    if s:find("no database specified", 1, true) then return true end
    if s:find("not found", 1, true) and s:find("database", 1, true) then return true end
    return false
end

function query_rows(ds, name)
    local ref = tonumber(ds.queryRef)
    if ref == nil or ref ~= math.floor(ref) or ref < 1 then
        add_log("warn", "datasource " .. tostring(name) .. " has no queryRef; no rows")
        return {}
    end
    ref = math.floor(ref)
    local database = session_database()
    if not database then
        add_log("warn", "datasource " .. tostring(name) .. " has no session database; no rows")
        return {}
    end
    if type(H) ~= "table" or type(H.altquery_sync) ~= "function" then
        add_log("warn", "datasource " .. tostring(name) .. " query " .. tostring(ref)
            .. " failed: H.altquery_sync is missing; no rows")
        return {}
    end
    -- Type 3 is SQL - Reporting. Type 1 does not find QueryRef 174.
    local ok, res, err = pcall(H.altquery_sync, database, [[
        SELECT code, collection FROM ${SCHEMA}queries
        WHERE query_ref = :QREF AND query_type_a28 = 3
    ]], { QREF = ref })
    if not ok then
        err = res
        res = nil
    end
    if err then
        if unconfigured(err) then
            add_log("warn", "datasource " .. tostring(name) .. " database " .. database
                .. " is not configured; no rows")
        else
            add_log("warn", "datasource " .. tostring(name) .. " query " .. tostring(ref)
                .. " failed: " .. tostring(err) .. "; no rows")
        end
        return {}
    end
    local row = type(res) == "table" and type(res.rows) == "table" and res.rows[1] or nil
    local code = row_field(row, "code")
    if type(code) ~= "string" or code == "" then
        add_log("warn", "datasource " .. tostring(name) .. " query " .. tostring(ref)
            .. " is not installed; no rows")
        return {}
    end
    local binds, missing = build_binds(ds, reporting_types(row_field(row, "collection")))
    if missing then return {} end
    local rok, rres, rerr = pcall(H.altquery_sync, database, code, binds)
    if not rok then
        rerr = rres
        rres = nil
    end
    if rerr then
        if unconfigured(rerr) then
            add_log("warn", "datasource " .. tostring(name) .. " database " .. database
                .. " is not configured; no rows")
        else
            add_log("warn", "datasource " .. tostring(name) .. " query " .. tostring(ref)
                .. " failed: " .. tostring(rerr) .. "; no rows")
        end
        return {}
    end
    local got = type(rres) == "table" and type(rres.rows) == "table" and rres.rows or {}
    add_log("info", "datasource " .. tostring(name) .. " ran query " .. tostring(ref)
        .. "; " .. tostring(#got) .. " rows")
    return got
end

function rows_for(name, band_id)
    if not declared[name] then
        add_log("warn", "detail " .. tostring(band_id) .. " names datasource " .. tostring(name)
            .. " which is not declared; no rows")
        return {}
    end
    local r = sources[name]
    if type(r) == "table" then
        return r
    end
    local ds = declared[name]
    if type(ds) == "table" and ds.kind == "query" then
        return query_rows(ds, name)
    end
    add_log("warn", "datasource " .. tostring(name) .. " has no array in sources; no rows")
    return {}
end
for _, d in ipairs(details) do
    if d.datasource ~= nil and d.datasource ~= "" then
        if not driver then
            driver = d.datasource
            rows = rows_for(driver, d.id)
        elseif d.datasource ~= driver then
            add_log("warn", "detail " .. tostring(d.id) .. " uses datasource " .. tostring(d.datasource)
                .. "; rows come from " .. driver .. " in this version")
        end
    end
end
if not driver then
    rows = { {} } -- static report: each band once
end

-- Parameter items (63): "NAME" (first datasource with it) or "ds.NAME".
warned_param = {}
function param_value(ref)
    if ref == nil or ref == "" then return nil end
    ref = tostring(ref)
    local ds_name, pname = ref:match("^([^%.]+)%.(.+)$")
    for _, ds in ipairs(type(def.datasources) == "table" and def.datasources or {}) do
        if type(ds) == "table" and type(ds.params) == "table"
            and (ds_name == nil or ds.name == ds_name) then
            local v = ds.params[pname or ref]
            if v ~= nil then return v end
        end
    end
    if not warned_param[ref] then
        warned_param[ref] = true
        add_log("warn", "parameter " .. ref .. " is not set on any datasource; printed blank")
    end
    return nil
end
]==]

-- ----------------------------------------------------------------------------
-- Forward: insert Reports.Render.Data
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
]====] .. sql_new .. [====[
                ]==],
                'Reports.Render.Data module (Reporting)',
                0,
                0,
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
        'Insert Reports.Render.Data'                                        AS name,
        [=[
            # Forward Migration ${MIGRATION}: Reports.Render.Data

            Inserts group Reports, script Render.Data. Datasources, binds, query rows, the driving datasource, and parameter items. Always loaded.
            Reports.Render loads it. It is not invokable.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]====]})

-- ----------------------------------------------------------------------------
-- Reverse: delete Reports.Render.Data
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
            WHERE group_name = '${GROUP_NAME}'
              AND script_name = '${SCRIPT_NAME}';

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Delete Reports.Render.Data'                                        AS name,
        [=[
            # Reverse Migration ${MIGRATION}: delete Reports.Render.Data

            Deletes the group Reports script Render.Data row.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]====]})

return queries end
