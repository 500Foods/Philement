-- schematool_expect.lua
-- Extract expected migration payloads (code/name/summary) from Lua sources.
-- Same generation path as tests/lib/get_migration.lua / test_31; decode wrappers
-- like tests/lib/get_diagram.sh (base64 + optional brotli).
--
-- Usage:
--   lua schematool_expect.lua <migrations_dir> <engine> <design> <schema> <ref>
--   lua schematool_expect.lua <migrations_dir> <engine> <design> <schema> --all [from] [to]
--
-- Prints one JSON object (single ref) or a JSON array (--all).
--
-- CHANGELOG
-- 1.0.2 - 2026-08-23 - Stderr progress: expect N/M ref R
-- 1.0.1 - 2026-07-29 - extract_as_field: ignore commas inside SQL string literals
-- 1.0.0 - 2026-07-29 - Phase 2 expected payload extraction

-- luacheck: globals arg package

local migrations_dir = arg[1]
local engine = arg[2]
local design = arg[3]
local schema_name = arg[4]
local mode_or_ref = arg[5]

if not migrations_dir or not engine or not design or mode_or_ref == nil then
    io.stderr:write(
        "Usage: lua schematool_expect.lua <migrations_dir> <engine> <design> <schema> <ref>\n"
        .. "       lua schematool_expect.lua <migrations_dir> <engine> <design> <schema> --all [from] [to]\n"
    )
    os.exit(1)
end

if schema_name == nil or schema_name == "." then
    schema_name = ""
end

-- Load design database module (cwd / package.path must include migrations_dir)
package.path = migrations_dir .. "/?.lua;" .. package.path

local ok_db, database = pcall(require, "database")
if not ok_db then
    io.stderr:write("Error: require('database') failed: " .. tostring(database) .. "\n")
    os.exit(1)
end

if not database.defaults or not database.defaults[engine] then
    io.stderr:write("Error: unsupported or missing engine defaults: " .. tostring(engine) .. "\n")
    os.exit(1)
end

local brotli_mod
do
    local ok_b, mod = pcall(require, "brotli")
    if ok_b then
        brotli_mod = mod
    end
end

local function base64_decode(data)
    local b = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"
    data = data:gsub("%s", ""):gsub("[^" .. b .. "=]", "")
    return (data:gsub(".", function(x)
        if x == "=" then
            return ""
        end
        local r, f = "", (b:find(x, 1, true) - 1)
        for i = 6, 1, -1 do
            r = r .. (f % 2 ^ i - f % 2 ^ (i - 1) > 0 and "1" or "0")
        end
        return r
    end):gsub("%d%d%d?%d?%d?%d?%d?%d?", function(x)
        if #x ~= 8 then
            return ""
        end
        local c = 0
        for i = 1, 8 do
            c = c + (x:sub(i, i) == "1" and 2 ^ (8 - i) or 0)
        end
        return string.char(c)
    end))
end

-- First single-quoted string in a SQL expression (base64 payload), same idea as get_diagram.sh
local function first_single_quoted(expr)
    local i = 1
    local n = #expr
    while i <= n do
        if expr:sub(i, i) == "'" then
            i = i + 1
            local parts = {}
            while i <= n do
                local c = expr:sub(i, i)
                if c == "'" then
                    if expr:sub(i + 1, i + 1) == "'" then
                        parts[#parts + 1] = "'"
                        i = i + 2
                    else
                        return table.concat(parts)
                    end
                else
                    parts[#parts + 1] = c
                    i = i + 1
                end
            end
            return table.concat(parts)
        end
        i = i + 1
    end
    return nil
end

local function decode_payload_expr(expr)
    if not expr or expr:match("^%s*$") then
        return ""
    end
    local trimmed = expr:match("^%s*(.-)%s*$") or expr
    -- Plain SQL string literal (rare for code; common for empty summary)
    if trimmed:sub(1, 1) == "'" and not trimmed:upper():find("DECODE", 1, true)
        and not trimmed:upper():find("BASE64", 1, true)
        and not trimmed:upper():find("FROM_BASE64", 1, true) then
        local inner = trimmed:match("^'(.*)'$")
        if inner then
            return inner:gsub("''", "'")
        end
    end

    local b64 = first_single_quoted(trimmed)
    if not b64 or b64 == "" then
        -- Unquoted plain token
        if not trimmed:find("[%(%)]") then
            return trimmed
        end
        io.stderr:write("Error: could not extract base64 payload from expression\n")
        os.exit(1)
    end

    local raw = base64_decode(b64)
    local upper = trimmed:upper()
    if upper:find("BROTLI", 1, true) then
        if not brotli_mod then
            io.stderr:write("Error: brotli Lua module required for compressed payloads\n")
            os.exit(1)
        end
        local dec = brotli_mod.decompress(raw)
        if not dec then
            io.stderr:write("Error: brotli decompress failed\n")
            os.exit(1)
        end
        return dec
    end
    return raw
end

-- Split run_migration output on QUERY DELIMITER
local function split_queries(sql)
    local parts = {}
    local delim = "\n-- QUERY DELIMITER\n"
    local start = 1
    while true do
        local s, e = sql:find(delim, start, true)
        if not s then
            local tail = sql:sub(start)
            if tail:match("%S") then
                parts[#parts + 1] = tail
            end
            break
        end
        local chunk = sql:sub(start, s - 1)
        if chunk:match("%S") then
            parts[#parts + 1] = chunk
        end
        start = e + 1
    end
    return parts
end

-- Extract AS-aliased field from INSERT…SELECT body.
-- Handles nested parens and commas inside single-quoted SQL strings
-- (e.g. name = 'Extend t with a, b, metadata' AS name).
local function extract_as_field(part, alias)
    -- Prefer last occurrence of " AS <alias>" before FROM for multi-line inserts
    local best_expr
    local search_from = 1
    local alias_pat = "%s+[Aa][Ss]%s+" .. alias .. "[%s,;\n)]"
    while true do
        local as_s, as_e = part:find(alias_pat, search_from)
        if not as_s then
            break
        end
        local before = part:sub(1, as_s - 1)
        -- Walk backward: comma at paren-depth 0 outside quotes ends previous field
        local expr_start = 1
        local depth = 0
        local in_str = false
        local i = #before
        while i >= 1 do
            local c = before:sub(i, i)
            if in_str then
                if c == "'" then
                    -- SQL escaped quote is ''; if prev char is also ', stay in string
                    if i > 1 and before:sub(i - 1, i - 1) == "'" then
                        i = i - 1
                    else
                        in_str = false
                    end
                end
            else
                if c == "'" then
                    in_str = true
                elseif c == ")" then
                    depth = depth + 1
                elseif c == "(" then
                    depth = depth - 1
                elseif c == "," and depth == 0 then
                    expr_start = i + 1
                    break
                end
            end
            i = i - 1
        end
        local expr = before:sub(expr_start):match("^%s*(.-)%s*$")
        best_expr = expr
        search_from = as_e + 1
    end
    return best_expr
end

local function is_insert_part(part)
    return part:upper():find("INSERT%s+INTO", 1) ~= nil
end

local function parse_insert_payload(part)
    if not is_insert_part(part) then
        return nil
    end
    local type_expr = extract_as_field(part, "query_type_a28")
    local ref_expr = extract_as_field(part, "query_ref")
    local code_expr = extract_as_field(part, "code")
    local name_expr = extract_as_field(part, "name")
    local summary_expr = extract_as_field(part, "summary")

    if not type_expr or not code_expr then
        return nil
    end

    local qtype = tonumber((type_expr:match("(%d+)") or ""):match("%d+"))
    local qref = ref_expr and tonumber((ref_expr:match("(%d+)") or ""):match("%d+")) or nil

    local code = decode_payload_expr(code_expr)
    local name = name_expr and decode_payload_expr(name_expr) or ""
    local summary = summary_expr and decode_payload_expr(summary_expr) or ""

    return {
        query_ref = qref,
        query_type = qtype,
        name = name,
        summary = summary,
        code = code,
    }
end

local function json_escape(s)
    s = tostring(s or "")
    s = s:gsub("\\", "\\\\")
    s = s:gsub('"', '\\"')
    s = s:gsub("\b", "\\b")
    s = s:gsub("\f", "\\f")
    s = s:gsub("\n", "\\n")
    s = s:gsub("\r", "\\r")
    s = s:gsub("\t", "\\t")
    return s
end

local function payload_to_json(p)
    return string.format(
        '{"query_ref":%s,"query_type":%s,"name":"%s","summary":"%s","code":"%s"}',
        p.query_ref and tostring(p.query_ref) or "null",
        p.query_type and tostring(p.query_type) or "null",
        json_escape(p.name),
        json_escape(p.summary),
        json_escape(p.code)
    )
end

local function expect_one(ref)
    local ref_str = string.format("%d", ref)
    local mod_name = design .. "_" .. ref_str
    local ok_m, migration_func = pcall(require, mod_name)
    if not ok_m then
        io.stderr:write("Error: require('" .. mod_name .. "') failed: " .. tostring(migration_func) .. "\n")
        os.exit(1)
    end
    if type(migration_func) ~= "function" then
        io.stderr:write("Error: " .. mod_name .. " did not return a function\n")
        os.exit(1)
    end

    -- Same as get_migration.lua: pass defaults table by reference so migration
    -- can set cfg.TABLE / cfg.MIGRATION etc. that replace_query later reads.
    local cfg = database.defaults[engine]

    local ok_q, queries = pcall(migration_func, engine, design, schema_name, cfg)
    if not ok_q then
        io.stderr:write("Error: migration " .. ref_str .. " failed: " .. tostring(queries) .. "\n")
        os.exit(1)
    end

    local sql = database:run_migration(queries, engine, design, schema_name)
    local parts = split_queries(sql)
    local payloads = {}
    for _, part in ipairs(parts) do
        local p = parse_insert_payload(part)
        if p then
            if not p.query_ref then
                p.query_ref = ref
            end
            payloads[#payloads + 1] = p
        end
    end

    if #payloads == 0 then
        io.stderr:write(
            "Error: no INSERT metadata payloads in migration " .. ref_str
            .. " (bootstrap DDL-only parts are not stored as expected code)\n"
        )
        os.exit(1)
    end

    return {
        ref = ref,
        file = design .. "_" .. ref_str .. ".lua",
        engine = engine,
        design = design,
        schema = schema_name,
        payloads = payloads,
    }
end

local function result_to_json(result)
    local buf = {
        string.format(
            '{"ref":%d,"file":"%s","engine":"%s","design":"%s","schema":"%s","payloads":[',
            result.ref,
            json_escape(result.file),
            json_escape(result.engine),
            json_escape(result.design),
            json_escape(result.schema)
        ),
    }
    for i, p in ipairs(result.payloads) do
        buf[#buf + 1] = payload_to_json(p)
        if i < #result.payloads then
            buf[#buf + 1] = ","
        end
    end
    buf[#buf + 1] = "]}"
    return table.concat(buf)
end

local function list_refs(from_ref, to_ref)
    local design_pat = design:gsub("(%W)", "%%%1")
    local pattern = "^" .. design_pat .. "_(%d+)%.lua$"
    local refs = {}
    local handle = io.popen('ls -1 "' .. migrations_dir:gsub('"', '\\"') .. '" 2>/dev/null')
    if not handle then
        io.stderr:write("Error: failed to list migrations directory\n")
        os.exit(1)
    end
    for name in handle:lines() do
        local num = name:match(pattern)
        if num then
            local r = tonumber(num)
            local include = true
            if from_ref and r < from_ref then
                include = false
            end
            if to_ref and r > to_ref then
                include = false
            end
            if include then
                refs[#refs + 1] = r
            end
        end
    end
    handle:close()
    table.sort(refs)
    return refs
end

if mode_or_ref == "--all" then
    local from_ref = tonumber(arg[6] or "")
    local to_ref = tonumber(arg[7] or "")
    local refs = list_refs(from_ref, to_ref)
    if #refs == 0 then
        io.stderr:write("Error: no migrations matched\n")
        os.exit(1)
    end
    local out = { "[" }
    for i, r in ipairs(refs) do
        io.stderr:write(string.format("expect %d/%d ref %d\n", i, #refs, r))
        io.stderr:flush()
        out[#out + 1] = result_to_json(expect_one(r))
        if i < #refs then
            out[#out + 1] = ","
        end
    end
    out[#out + 1] = "]\n"
    io.write(table.concat(out))
else
    local ref = tonumber(mode_or_ref)
    if not ref then
        io.stderr:write("Error: ref must be an integer or --all\n")
        os.exit(1)
    end
    io.write(result_to_json(expect_one(ref)))
    io.write("\n")
end
