-- schematool_catalog_fold.lua
-- Hybrid C: fold applied migration forward code (type 1003) into expected catalog.
--
-- Usage:
--   lua schematool_catalog_fold.lua --db PATH [--schema NAME] [--only-tables a,b]
--        [--checklist-out PATH]  (stdout JSON if no out)
--
-- CHANGELOG
-- 1.1.0 - 2026-08-23 - Record last fold ref on each expected column
-- 1.0.1 - 2026-08-23 - Lua 5.5: do not assign to generic-for loop variable
-- 1.0.0 - 2026-08-02 - Phase 7b hybrid C fold from applied codes

-- luacheck: globals arg

local db_path
local schema_name = ""
local only_tables_csv = ""
local out_path

local i = 1
while i <= #arg do
    local a = arg[i]
    if a == "--db" then
        db_path = arg[i + 1]
        i = i + 2
    elseif a == "--schema" then
        schema_name = arg[i + 1] or ""
        i = i + 2
    elseif a == "--only-tables" then
        only_tables_csv = arg[i + 1] or ""
        i = i + 2
    elseif a == "--out" then
        out_path = arg[i + 1]
        i = i + 2
    else
        io.stderr:write("Error: unknown argument: " .. tostring(a) .. "\n")
        os.exit(1)
    end
end

if not db_path then
    io.stderr:write(
        "Usage: lua schematool_catalog_fold.lua --db PATH [--schema S] "
            .. "[--only-tables a,b] [--out PATH]\n"
    )
    os.exit(1)
end

local function write_all(path, data)
    local f, err = io.open(path, "wb")
    if not f then
        io.stderr:write("Error: cannot write " .. path .. ": " .. tostring(err) .. "\n")
        os.exit(1)
    end
    f:write(data)
    f:close()
end

local TMP_DIR = (os.getenv("TMPDIR") or "/tmp")
    .. "/schematool_fold_"
    .. tostring(os.time())
    .. "_"
    .. tostring(math.random(100000))
os.execute('mkdir -p "' .. TMP_DIR .. '"')

local function cleanup_tmp()
    os.execute('rm -rf "' .. TMP_DIR .. '"')
end

local function json_escape(s)
    s = tostring(s or "")
    s = s:gsub("\\", "\\\\")
    s = s:gsub('"', '\\"')
    s = s:gsub("\n", "\\n")
    s = s:gsub("\r", "\\r")
    s = s:gsub("\t", "\\t")
    return s
end

local function strip_schema(name)
    if not name then
        return ""
    end
    name = name:gsub('^"', ""):gsub('"$', "")
    name = name:gsub("^%[", ""):gsub("%]$", "")
    local bare = name:match("%.([%w_]+)$")
    if bare then
        return bare:lower()
    end
    return name:lower()
end

local function is_keyword_col(name)
    local u = name:upper()
    return u == "PRIMARY"
        or u == "CONSTRAINT"
        or u == "UNIQUE"
        or u == "FOREIGN"
        or u == "CHECK"
        or u == "KEY"
        or u == "INDEX"
        or u == "REFERENCES"
end

-- tables[name] = { columns = { [name] = {name, data_type, nullable, ref} }, col_order = {}, pk = {} }
local tables = {}
local current_ref = 0

local function ensure_table(tname)
    tname = tname:lower()
    if not tables[tname] then
        tables[tname] = { columns = {}, col_order = {}, pk = {} }
    end
    return tables[tname]
end

local function drop_table(tname)
    tables[tname:lower()] = nil
end

local function rename_table(from_n, to_n)
    from_n = from_n:lower()
    to_n = to_n:lower()
    if from_n == to_n then
        return
    end
    local t = tables[from_n]
    if not t then
        return
    end
    tables[from_n] = nil
    tables[to_n] = t
end

local function set_column(tname, cname, data_type, nullable)
    local t = ensure_table(tname)
    cname = cname:lower()
    local prev = t.columns[cname]
    if not prev then
        t.col_order[#t.col_order + 1] = cname
    end
    local dt = data_type or (prev and prev.data_type) or ""
    dt = dt:lower():gsub("%s+", " "):gsub("^%s+", ""):gsub("%s+$", "")
    local nullv = nullable
    if nullv == nil then
        nullv = prev and prev.nullable
        if nullv == nil then
            nullv = true
        end
    end
    t.columns[cname] = {
        name = cname,
        data_type = dt,
        nullable = nullv and true or false,
        ref = current_ref,
    }
end

local function set_nullable(tname, cname, nullable)
    local t = tables[tname:lower()]
    if not t then
        return
    end
    local c = t.columns[cname:lower()]
    if not c then
        -- column mentioned in ALTER but unknown: record presence
        set_column(tname, cname, "", nullable)
        return
    end
    c.nullable = nullable and true or false
    c.ref = current_ref
end

local function drop_column(tname, cname)
    local t = tables[tname:lower()]
    if not t then
        return
    end
    cname = cname:lower()
    t.columns[cname] = nil
    local new_order = {}
    for _, n in ipairs(t.col_order) do
        if n ~= cname then
            new_order[#new_order + 1] = n
        end
    end
    t.col_order = new_order
end

local function parse_create_body(tname, body)
    -- Split on commas not inside parens
    local depth = 0
    local cur = {}
    local parts = {}
    for j = 1, #body do
        local ch = body:sub(j, j)
        if ch == "(" then
            depth = depth + 1
            cur[#cur + 1] = ch
        elseif ch == ")" then
            depth = depth - 1
            cur[#cur + 1] = ch
        elseif ch == "," and depth == 0 then
            parts[#parts + 1] = table.concat(cur)
            cur = {}
        else
            cur[#cur + 1] = ch
        end
    end
    if #cur > 0 then
        parts[#parts + 1] = table.concat(cur)
    end

    local pk_cols = {}
    for _, part in ipairs(parts) do
        local line = part:gsub("^%s+", ""):gsub("%s+$", "")
        if line == "" then
            goto continue
        end
        local up = line:upper()
        if up:match("^PRIMARY%s+KEY") then
            local inside = line:match("%((.-)%)")
            if inside then
                for col in inside:gmatch("[%w_]+") do
                    pk_cols[#pk_cols + 1] = col:lower()
                end
            end
            goto continue
        end
        if up:match("^CONSTRAINT") or up:match("^UNIQUE") or up:match("^FOREIGN")
            or up:match("^CHECK") or up:match("^KEY%s") or up:match("^INDEX") then
            goto continue
        end
        local cname, rest = line:match("^([%w_]+)%s+(.*)$")
        if not cname or is_keyword_col(cname) then
            goto continue
        end
        rest = rest or ""
        local nullable = true
        if rest:upper():find("NOT%s+NULL", 1, false) then
            nullable = false
        end
        local dtype = rest
        dtype = dtype:gsub("[Nn][Oo][Tt]%s+[Nn][Uu][Ll][Ll]", "")
        dtype = dtype:gsub("[Dd][Ee][Ff][Aa][Uu][Ll][Tt]%s+%S+", "")
        dtype = dtype:gsub("[Pp][Rr][Ii][Mm][Aa][Rr][Yy]%s+[Kk][Ee][Yy]", "")
        dtype = dtype:gsub("[Uu][Nn][Ii][Qq][Uu][Ee]", "")
        dtype = dtype:gsub("%s+", " "):gsub("^%s+", ""):gsub("%s+$", "")
        if rest:upper():find("PRIMARY%s+KEY", 1, false) then
            pk_cols[#pk_cols + 1] = cname:lower()
            nullable = false
        end
        set_column(tname, cname, dtype, nullable)
        ::continue::
    end
    if #pk_cols > 0 then
        ensure_table(tname).pk = pk_cols
    end
end

local function apply_statement(stmt)
    stmt = stmt:gsub("/%*.-%*/", " ")
    -- strip line comments that are not delimiter markers mid-line carefully
    local cleaned_lines = {}
    for line in (stmt .. "\n"):gmatch("(.-)\n") do
        if not line:match("^%s*%-%-") then
            cleaned_lines[#cleaned_lines + 1] = line:gsub("%-%-.*$", "")
        end
    end
    stmt = table.concat(cleaned_lines, "\n")
    stmt = stmt:gsub("%s+", " "):gsub("^%s+", ""):gsub("%s+$", "")
    if stmt == "" then
        return
    end

    local up = stmt:upper()

    -- skip status updates and DML that is not DDL
    if up:match("^UPDATE%s+") and up:find("QUERY_TYPE", 1, true) then
        return
    end
    if up:match("^INSERT%s+") or up:match("^DELETE%s+") or up:match("^SELECT%s+") then
        return
    end

    -- CREATE TABLE name ( body )
    local create_pat = "^[Cc][Rr][Ee][Aa][Tt][Ee]%s+[Tt][Aa][Bb][Ll][Ee]%s+"
        .. "([%w_%.\"%[%]]+)%s*%((.*)%)%s*;?%s*$"
    local ct_name, ct_body = stmt:match(create_pat)
    if ct_name and ct_body then
        local bare = strip_schema(ct_name)
        -- SQLite rebuild: accounts_new → rename later maps onto final table
        parse_create_body(bare, ct_body)
        return
    end

    -- DROP TABLE [IF EXISTS] name (Lua patterns have no non-capturing groups)
    local dt = stmt:match(
        "^[Dd][Rr][Oo][Pp]%s+[Tt][Aa][Bb][Ll][Ee]%s+[Ii][Ff]%s+[Ee][Xx][Ii][Ss][Tt][Ss]%s+([%w_%.\"%[%]]+)"
    )
    if not dt then
        dt = stmt:match("^[Dd][Rr][Oo][Pp]%s+[Tt][Aa][Bb][Ll][Ee]%s+([%w_%.\"%[%]]+)")
    end
    if dt then
        drop_table(strip_schema(dt))
        return
    end

    -- ALTER TABLE name RENAME TO new
    local rename_pat = "^[Aa][Ll][Tt][Ee][Rr]%s+[Tt][Aa][Bb][Ll][Ee]%s+"
        .. "([%w_%.\"%[%]]+)%s+[Rr][Ee][Nn][Aa][Mm][Ee]%s+[Tt][Oo]%s+([%w_%.\"%[%]]+)"
    local rn_from, rn_to = stmt:match(rename_pat)
    if rn_from and rn_to then
        rename_table(strip_schema(rn_from), strip_schema(rn_to))
        return
    end

    local at_table = stmt:match("^[Aa][Ll][Tt][Ee][Rr]%s+[Tt][Aa][Bb][Ll][Ee]%s+([%w_%.\"%[%]]+)%s+")
    if not at_table then
        return
    end
    local tname = strip_schema(at_table)
    local rest = stmt:match("^[Aa][Ll][Tt][Ee][Rr]%s+[Tt][Aa][Bb][Ll][Ee]%s+[%w_%.\"%[%]]+%s+(.*)$") or ""

    -- ADD COLUMN col type ...
    local add_col, add_rest = rest:match("^[Aa][Dd][Dd]%s+[Cc][Oo][Ll][Uu][Mm][Nn]%s+([%w_]+)%s+(.*)$")
    if add_col then
        local nullable = true
        if add_rest:upper():find("NOT%s+NULL", 1, false) then
            nullable = false
        end
        local dtype = add_rest
        dtype = dtype:gsub("[Nn][Oo][Tt]%s+[Nn][Uu][Ll][Ll]", "")
        dtype = dtype:gsub("[Dd][Ee][Ff][Aa][Uu][Ll][Tt]%s+%S+", "")
        dtype = dtype:gsub("%s+", " "):gsub("^%s+", ""):gsub("%s+$", ""):gsub(";$", "")
        set_column(tname, add_col, dtype, nullable)
        return
    end

    -- DROP COLUMN col
    local drop_col = rest:match("^[Dd][Rr][Oo][Pp]%s+[Cc][Oo][Ll][Uu][Mm][Nn]%s+([%w_]+)")
    if drop_col then
        drop_column(tname, drop_col)
        return
    end

    -- ALTER COLUMN col DROP NOT NULL
    local drop_nn_pat = "^[Aa][Ll][Tt][Ee][Rr]%s+[Cc][Oo][Ll][Uu][Mm][Nn]%s+"
        .. "([%w_]+)%s+[Dd][Rr][Oo][Pp]%s+[Nn][Oo][Tt]%s+[Nn][Uu][Ll][Ll]"
    local ac_drop = rest:match(drop_nn_pat)
    if ac_drop then
        set_nullable(tname, ac_drop, true)
        return
    end

    -- ALTER COLUMN col SET NOT NULL
    local set_nn_pat = "^[Aa][Ll][Tt][Ee][Rr]%s+[Cc][Oo][Ll][Uu][Mm][Nn]%s+"
        .. "([%w_]+)%s+[Ss][Ee][Tt]%s+[Nn][Oo][Tt]%s+[Nn][Uu][Ll][Ll]"
    local ac_set = rest:match(set_nn_pat)
    if ac_set then
        set_nullable(tname, ac_set, false)
        return
    end

    -- MODIFY COLUMN col type [NULL|NOT NULL]  (MySQL)
    local mod_pat = "^[Mm][Oo][Dd][Ii][Ff][Yy]%s+[Cc][Oo][Ll][Uu][Mm][Nn]%s+([%w_]+)%s+(.*)$"
    local mc_col, mc_rest = rest:match(mod_pat)
    if mc_col then
        local nullable = true
        if mc_rest:upper():find("NOT%s+NULL", 1, false) then
            nullable = false
        elseif mc_rest:upper():match("%sNULL%s*$") or mc_rest:upper():match("%sNULL%s*;%s*$") then
            nullable = true
        end
        local dtype = mc_rest
        dtype = dtype:gsub("[Nn][Oo][Tt]%s+[Nn][Uu][Ll][Ll]", "")
        dtype = dtype:gsub("%s+[Nn][Uu][Ll][Ll]%s*;?%s*$", "")
        dtype = dtype:gsub("%s+", " "):gsub("^%s+", ""):gsub("%s+$", ""):gsub(";$", "")
        set_column(tname, mc_col, dtype, nullable)
        return
    end
end

local function fold_code(code)
    if not code or code == "" then
        return
    end
    -- Split on subquery delimiter (various casings/spacing)
    local chunks = {}
    local rest = code
    local delim = "%-%-%s*SUBQUERY%s+DELIMITER"
    while true do
        local s, e = rest:find(delim)
        if not s then
            chunks[#chunks + 1] = rest
            break
        end
        chunks[#chunks + 1] = rest:sub(1, s - 1)
        rest = rest:sub(e + 1)
    end
    for _, chunk in ipairs(chunks) do
        -- further split on semicolons at depth 0 for multi-statement chunks
        local depth = 0
        local cur = {}
        local stmts = {}
        for j = 1, #chunk do
            local ch = chunk:sub(j, j)
            if ch == "(" then
                depth = depth + 1
                cur[#cur + 1] = ch
            elseif ch == ")" then
                depth = depth - 1
                cur[#cur + 1] = ch
            elseif ch == ";" and depth == 0 then
                stmts[#stmts + 1] = table.concat(cur)
                cur = {}
            else
                cur[#cur + 1] = ch
            end
        end
        if #cur > 0 then
            stmts[#stmts + 1] = table.concat(cur)
        end
        for _, st in ipairs(stmts) do
            apply_statement(st)
        end
    end
end

-- Extract applied forward bodies ordered by ref via jq (-c objects)
local filter = [[
  [.[] | select(.query_type == 1003) | {r:.query_ref, c:.code}]
  | sort_by(.r) | .[]
]]

local lines = {}
do
    local fpath = TMP_DIR .. "/fold.jq"
    write_all(fpath, filter .. "\n")
    local cmd = 'jq -c -f "' .. fpath .. '" "' .. db_path:gsub('"', '\\"') .. '"'
    local h = io.popen(cmd)
    if not h then
        cleanup_tmp()
        io.stderr:write("Error: jq failed extracting applied codes\n")
        os.exit(1)
    end
    for line in h:lines() do
        lines[#lines + 1] = line
    end
    h:close()
end

local function json_string_field(obj, key)
    local pat = '"' .. key .. '"%s*:%s*"'
    local s, e = obj:find(pat)
    if not s then
        return ""
    end
    local i2 = e + 1
    local parts = {}
    while i2 <= #obj do
        local c = obj:sub(i2, i2)
        if c == "\\" then
            local n = obj:sub(i2 + 1, i2 + 1)
            if n == "n" then
                parts[#parts + 1] = "\n"
            elseif n == "r" then
                parts[#parts + 1] = "\r"
            elseif n == "t" then
                parts[#parts + 1] = "\t"
            elseif n == '"' then
                parts[#parts + 1] = '"'
            elseif n == "\\" then
                parts[#parts + 1] = "\\"
            elseif n == "u" then
                local hex = obj:sub(i2 + 2, i2 + 5)
                local code = tonumber(hex, 16)
                if code and code < 128 then
                    parts[#parts + 1] = string.char(code)
                end
                i2 = i2 + 5
            else
                parts[#parts + 1] = n
            end
            i2 = i2 + 2
        elseif c == '"' then
            break
        else
            parts[#parts + 1] = c
            i2 = i2 + 1
        end
    end
    return table.concat(parts)
end

local function json_num_field(obj, key)
    local n = obj:match('"' .. key .. '"%s*:%s*(%-?%d+)')
    return n and tonumber(n) or nil
end

for _, line in ipairs(lines) do
    current_ref = json_num_field(line, "r") or 0
    local code = json_string_field(line, "c")
    fold_code(code)
end

-- only-tables filter
local only = {}
local only_set = false
if only_tables_csv and only_tables_csv ~= "" then
    only_set = true
    for raw in only_tables_csv:gmatch("[^,]+") do
        local trimmed = raw:gsub("^%s+", ""):gsub("%s+$", ""):lower()
        if trimmed ~= "" then
            only[trimmed] = true
        end
    end
end

local names = {}
for n, _ in pairs(tables) do
    if (not only_set) or only[n] then
        names[#names + 1] = n
    end
end
table.sort(names)

local parts = { '{"schema":"' .. json_escape(schema_name) .. '","tables":[' }
for idx, n in ipairs(names) do
    local t = tables[n]
    parts[#parts + 1] = '{"table":"' .. json_escape(n) .. '","columns":['
    local cols = {}
    for _, cn in ipairs(t.col_order) do
        local c = t.columns[cn]
        if c then
            local ref_json = ""
            if c.ref and c.ref > 0 then
                ref_json = string.format(',"ref":%d', c.ref)
            end
            cols[#cols + 1] = string.format(
                '{"name":"%s","data_type":"%s","nullable":%s,"default":null%s}',
                json_escape(c.name),
                json_escape(c.data_type),
                c.nullable and "true" or "false",
                ref_json
            )
        end
    end
    parts[#parts + 1] = table.concat(cols, ",")
    parts[#parts + 1] = '],"primary_key":['
    local pks = {}
    for _, pk in ipairs(t.pk or {}) do
        pks[#pks + 1] = '"' .. json_escape(pk) .. '"'
    end
    parts[#parts + 1] = table.concat(pks, ",")
    parts[#parts + 1] = '],"indexes":[]}'
    if idx < #names then
        parts[#parts + 1] = ","
    end
end
parts[#parts + 1] = "]}"

local out = table.concat(parts)
cleanup_tmp()

if out_path then
    write_all(out_path, out .. "\n")
else
    io.write(out .. "\n")
end
