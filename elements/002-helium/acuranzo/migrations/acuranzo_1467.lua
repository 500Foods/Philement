-- Migration: acuranzo_1467.lua
-- Reports.Render.Core (Reporting)
-- Params, log, fail, item keys, CSS and SVG number helpers, JSON, and the
-- report definition. Always loaded.
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
cfg.MIGRATION = "1467"
cfg.GROUP_NAME = "Reports"
cfg.SCRIPT_NAME = "Render.Core"

local sql_new = [==[
-- Reports.Render.Core (Reporting). Always loaded: params, log, fail, item keys, CSS helpers, JSON, the report definition.
-- Loaded by Reports.Render into its per-request environment.
-- Names defined here are shared with Reports.Render and its other modules.

P = params or {}
log = {}

function add_log(level, message)
    log[#log + 1] = { level = level, message = message }
end

function fail(message)
    add_log("error", message)
    H.set_result_json({ error = message, log = log })
    return 0
end

FIELD_KEYS = { [20] = true, [21] = true, [24] = true }
PAGE_KEYS = { [61] = true, [62] = true, [64] = true }
PAGE_TEMPLATE_DEFAULT = "Page {n} of {c}"
page_src = {}
page_agg_src = {}
page_detail = {}
LATER_KEYS = {
    [42] = true, [44] = true,
    [61] = true, [62] = true, [63] = true, [28] = true, [29] = true, [32] = true,
    [34] = true, [38] = true,
}

function plain(v)
    if v == nil then return "" end
    if math.type(v) == "float" then
        if v == math.floor(v) and math.abs(v) < 1e15 then
            return string.format("%d", math.floor(v))
        end
        local s = string.format("%.15g", v)
        if tonumber(s) ~= v then s = string.format("%.17g", v) end
        return s
    end
    return tostring(v)
end

CSS_UNIT = { ["in"] = true, mm = true, cm = true, pt = true, px = true }
ALIGN_CSS = { Left = "left", Center = "center", Right = "right" }

function css_len(len)
    if type(len) ~= "string" then return nil end
    local n, u = len:match("^%s*([%d%.]+)%s*(%a+)%s*$")
    if not n or not CSS_UNIT[u] then return nil end
    return n .. u
end

function css_or(len, fallback, what)
    local css = css_len(len)
    if css then return css end
    if len ~= nil and len ~= "" then
        add_log("warn", what .. " " .. tostring(len) .. " does not parse; used " .. fallback)
    end
    return fallback
end

function esc(s)
    if s == nil then return "" end
    return (tostring(s):gsub("&", "&amp;"):gsub("<", "&lt;"):gsub(">", "&gt;"))
end

function css_family(name)
    if type(name) ~= "string" or name == "" then name = "Vanadium Sans" end
    name = name:gsub("\\", "\\\\"):gsub("'", "\\'")
    return "'" .. name .. "'"
end

function font_css(style)
    if style == nil or style == "" then style = "Normal" end
    if style ~= "Normal" and style ~= "Bold" and style ~= "Italic" and style ~= "Bold + Italic" then
        add_log("warn", "font style " .. tostring(style) .. " is not a v1 style; printed Normal")
        return "400", "normal"
    end
    local bold = style == "Bold" or style == "Bold + Italic"
    local italic = style == "Italic" or style == "Bold + Italic"
    return bold and "700" or "400", italic and "italic" or "normal"
end

-- A stored report is text. Hydrogen does not parse that column into a table.
-- phase27:json-start
function json_decode(text)
    if type(text) ~= "string" then
        return nil, "not json"
    end
    local s = text
    local n = #s
    local i = 1
    local depth = 0

    local function skip()
        local _, e = s:sub(i):find("^%s*")
        if e and e > 0 then
            i = i + e
        end
    end

    local parse_value

    local function parse_string()
        i = i + 1
        local parts = {}
        while i <= n do
            local c = s:sub(i, i)
            if c == '"' then
                i = i + 1
                return table.concat(parts), nil
            end
            if c == "\\" then
                local esc = s:sub(i + 1, i + 1)
                if esc == '"' or esc == "\\" or esc == "/" then
                    parts[#parts + 1] = esc
                    i = i + 2
                elseif esc == "b" then
                    parts[#parts + 1] = "\b"
                    i = i + 2
                elseif esc == "f" then
                    parts[#parts + 1] = "\f"
                    i = i + 2
                elseif esc == "n" then
                    parts[#parts + 1] = "\n"
                    i = i + 2
                elseif esc == "r" then
                    parts[#parts + 1] = "\r"
                    i = i + 2
                elseif esc == "t" then
                    parts[#parts + 1] = "\t"
                    i = i + 2
                elseif esc == "u" then
                    local hex = s:sub(i + 2, i + 5)
                    if not hex:match("^[0-9a-fA-F][0-9a-fA-F][0-9a-fA-F][0-9a-fA-F]$") then
                        return nil, "bad unicode escape"
                    end
                    local cp = tonumber(hex, 16)
                    i = i + 6
                    if cp >= 0xD800 and cp <= 0xDBFF then
                        if s:sub(i, i + 1) ~= "\\u" then
                            return nil, "bad unicode escape"
                        end
                        local hex2 = s:sub(i + 2, i + 5)
                        if not hex2:match("^[0-9a-fA-F][0-9a-fA-F][0-9a-fA-F][0-9a-fA-F]$") then
                            return nil, "bad unicode escape"
                        end
                        local low = tonumber(hex2, 16)
                        if low < 0xDC00 or low > 0xDFFF then
                            return nil, "bad unicode escape"
                        end
                        cp = 0x10000 + (cp - 0xD800) * 0x400 + (low - 0xDC00)
                        i = i + 6
                    elseif cp >= 0xDC00 and cp <= 0xDFFF then
                        return nil, "bad unicode escape"
                    end
                    local okc, ch = pcall(utf8.char, cp)
                    if not okc then
                        return nil, "bad unicode escape"
                    end
                    parts[#parts + 1] = ch
                else
                    return nil, "bad escape"
                end
            else
                if c:byte() < 32 then
                    return nil, "bad string"
                end
                parts[#parts + 1] = c
                i = i + 1
            end
        end
        return nil, "unterminated string"
    end

    local function parse_number()
        local start = i
        if s:sub(i, i) == "-" then
            i = i + 1
        end
        local d = s:sub(i, i)
        if d < "0" or d > "9" then
            return nil, "bad number"
        end
        if d == "0" then
            i = i + 1
        else
            while true do
                local c = s:sub(i, i)
                if c < "0" or c > "9" then break end
                i = i + 1
            end
        end
        if s:sub(i, i) == "." then
            i = i + 1
            local frac = i
            while true do
                local c = s:sub(i, i)
                if c < "0" or c > "9" then break end
                i = i + 1
            end
            if i == frac then
                return nil, "bad number"
            end
        end
        local exp = s:sub(i, i)
        if exp == "e" or exp == "E" then
            i = i + 1
            local sign = s:sub(i, i)
            if sign == "+" or sign == "-" then
                i = i + 1
            end
            local exp_at = i
            while true do
                local c = s:sub(i, i)
                if c < "0" or c > "9" then break end
                i = i + 1
            end
            if i == exp_at then
                return nil, "bad number"
            end
        end
        local num = tonumber(s:sub(start, i - 1))
        if num == nil then
            return nil, "bad number"
        end
        return num, nil
    end

    local function parse_object()
        depth = depth + 1
        if depth > 64 then
            return nil, "too deep"
        end
        i = i + 1
        local obj = {}
        skip()
        if s:sub(i, i) == "}" then
            i = i + 1
            depth = depth - 1
            return obj, nil
        end
        while true do
            skip()
            if s:sub(i, i) ~= '"' then
                return nil, "bad object"
            end
            local key, kerr = parse_string()
            if kerr then
                return nil, kerr
            end
            skip()
            if s:sub(i, i) ~= ":" then
                return nil, "bad object"
            end
            i = i + 1
            local val, verr = parse_value()
            if verr then
                return nil, verr
            end
            obj[key] = val
            skip()
            local c = s:sub(i, i)
            if c == "}" then
                i = i + 1
                depth = depth - 1
                return obj, nil
            end
            if c ~= "," then
                return nil, "bad object"
            end
            i = i + 1
        end
    end

    local function parse_array()
        depth = depth + 1
        if depth > 64 then
            return nil, "too deep"
        end
        i = i + 1
        local arr = {}
        local idx = 1
        skip()
        if s:sub(i, i) == "]" then
            i = i + 1
            depth = depth - 1
            return arr, nil
        end
        while true do
            local val, verr = parse_value()
            if verr then
                return nil, verr
            end
            arr[idx] = val
            idx = idx + 1
            skip()
            local c = s:sub(i, i)
            if c == "]" then
                i = i + 1
                depth = depth - 1
                return arr, nil
            end
            if c ~= "," then
                return nil, "bad array"
            end
            i = i + 1
        end
    end

    parse_value = function()
        skip()
        local c = s:sub(i, i)
        if c == '"' then
            return parse_string()
        end
        if c == "{" then
            return parse_object()
        end
        if c == "[" then
            return parse_array()
        end
        if c == "t" then
            if s:sub(i, i + 3) ~= "true" then
                return nil, "bad value"
            end
            i = i + 4
            return true, nil
        end
        if c == "f" then
            if s:sub(i, i + 4) ~= "false" then
                return nil, "bad value"
            end
            i = i + 5
            return false, nil
        end
        if c == "n" then
            if s:sub(i, i + 3) ~= "null" then
                return nil, "bad value"
            end
            i = i + 4
            return nil, nil
        end
        if c == "-" or (c >= "0" and c <= "9") then
            return parse_number()
        end
        return nil, "bad value"
    end

    local val, verr = parse_value()
    if verr then
        return nil, verr
    end
    skip()
    if i <= n then
        return nil, "trailing data"
    end
    return val, nil
end
-- phase27:json-end

function integer_id(value)
    local n = tonumber(value)
    if n == nil or n ~= n or n == math.huge or n == -math.huge then
        return nil
    end
    if n ~= math.floor(n) or n < 1 then
        return nil
    end
    return math.floor(n)
end

function row_field(row, name)
    if type(row) ~= "table" then
        return nil
    end
    if row[name] ~= nil then
        return row[name]
    end
    local upper = name:upper()
    if row[upper] ~= nil then
        return row[upper]
    end
    for key, value in pairs(row) do
        if type(key) == "string" and key:upper() == upper then
            return value
        end
    end
    return nil
end

function first_datasource_name(def)
    local list = def.datasources
    if type(list) ~= "table" then
        return nil
    end
    for _, entry in ipairs(list) do
        if type(entry) == "string" and entry ~= "" then
            return entry
        end
        if type(entry) == "table" and type(entry.name) == "string" and entry.name ~= "" then
            return entry.name
        end
    end
    return nil
end

-- Detail, group header, and group footer with no datasource use the first one.
function fill_band_datasources(def)
    if type(def) ~= "table" or type(def.bands) ~= "table" then
        return def
    end
    local name = first_datasource_name(def)
    if not name then
        return def
    end
    for _, band in ipairs(def.bands) do
        if type(band) == "table" then
            local key = band.typeKey
            if (key == 14 or key == 15 or key == 16)
                and (band.datasource == nil or band.datasource == "") then
                band.datasource = name
            end
        end
    end
    return def
end

function report_where(id, rev)
    return "report " .. tostring(id) .. " revision " .. tostring(rev)
end

function resolve_definition()
    if type(P.definition) == "table" then
        return P.definition, nil
    end
    local id = integer_id(P.report_id)
    local rev = integer_id(P.rev_id)
    if not id or not rev then
        return nil, nil
    end
    local hy = type(P._hydrogen) == "table" and P._hydrogen or {}
    local database = hy.database
    if type(database) ~= "string" or database == "" then
        return nil, "no session database; cannot load " .. report_where(id, rev)
    end
    if type(H) ~= "table" or type(H.altquery_sync) ~= "function" then
        return nil, "could not read QueryRef 167: H.altquery_sync is missing"
    end
    local ok, res, err = pcall(H.altquery_sync, database, [[
        SELECT code FROM ${SCHEMA}queries
        WHERE query_ref = 167 AND query_type_a28 = 1
    ]], {})
    if not ok then
        return nil, "could not read QueryRef 167: " .. tostring(res)
    end
    if err then
        return nil, "could not read QueryRef 167: " .. tostring(err)
    end
    local code = row_field(type(res) == "table" and type(res.rows) == "table" and res.rows[1] or nil, "code")
    if type(code) ~= "string" or code == "" then
        return nil, "could not read QueryRef 167: query is not installed"
    end
    local rok, rres, rerr = pcall(H.altquery_sync, database, code, {
        REPORT_ID = id,
        REV_ID = rev,
    })
    if not rok then
        return nil, "could not read " .. report_where(id, rev) .. ": " .. tostring(rres)
    end
    if rerr then
        return nil, "could not read " .. report_where(id, rev) .. ": " .. tostring(rerr)
    end
    local row = type(rres) == "table" and type(rres.rows) == "table" and rres.rows[1] or nil
    if type(row) ~= "table" then
        return nil, report_where(id, rev) .. " was not found"
    end
    local cell = row_field(row, "report")
    if cell == nil or cell == "" then
        return nil, report_where(id, rev) .. " has no definition"
    end
    local decoded
    if type(cell) == "table" then
        decoded = cell
    elseif type(cell) == "string" then
        if cell:match("^%s*$") then
            return nil, report_where(id, rev) .. " has no definition"
        end
        local parsed, jerr = json_decode(cell)
        if jerr then
            return nil, "definition is not valid JSON"
        end
        decoded = parsed
    else
        return nil, report_where(id, rev) .. " has no definition"
    end
    if type(decoded) ~= "table" then
        return nil, report_where(id, rev) .. " has no definition"
    end
    return fill_band_datasources(decoded), nil
end


-- Shared with Shapes and Barcode (SVG numbers and CSS lengths in px).
SHAPE_UNIT_PX = { ["in"] = 96, mm = 96 / 25.4, cm = 96 / 2.54, pt = 96 / 72, px = 1 }
function shape_fmt(n)
    if type(n) ~= "number" or n ~= n or n == math.huge or n == -math.huge then n = 0 end
    local neg = n < 0
    local rounded = math.floor(math.abs(n) * 10000 + 0.5) / 10000
    if rounded == 0 then return "0" end
    local text = string.format("%.4f", rounded):gsub("0+$", ""):gsub("%.$", "")
    if neg then return "-" .. text end
    return text
end

function shape_px(len)
    if type(len) ~= "string" then return nil end
    local n, u = len:match("^%s*([%+%-]?[%d%.]+)%s*(%a+)%s*$")
    n = tonumber(n)
    if not n or not SHAPE_UNIT_PX[u] then return nil end
    local px = n * SHAPE_UNIT_PX[u]
    local nearest = math.floor(px + 0.5)
    if math.abs(px - nearest) < 1e-9 then return nearest end
    return px
end
]==]

-- ----------------------------------------------------------------------------
-- Forward: insert Reports.Render.Core
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
                'Reports.Render.Core module (Reporting)',
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
        'Insert Reports.Render.Core'                                        AS name,
        [=[
            # Forward Migration ${MIGRATION}: Reports.Render.Core

            Inserts group Reports, script Render.Core. Params, log, fail, item keys, CSS and SVG number helpers, JSON, and the report definition. Always loaded.
            Reports.Render loads it. It is not invokable.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]====]})

-- ----------------------------------------------------------------------------
-- Reverse: delete Reports.Render.Core
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
        'Delete Reports.Render.Core'                                        AS name,
        [=[
            # Reverse Migration ${MIGRATION}: delete Reports.Render.Core

            Deletes the group Reports script Render.Core row.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]====]})

return queries end
