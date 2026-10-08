-- schematool_rows.lua
-- Net of keyed INSERT, UPDATE, and DELETE from disk forward SQL.
-- A live key the migrations never name is not a finding and is not deleted.
--
-- The queries table is the metadata track. DML against it is not a default row.
-- A statement that does not bind one key is unkeyed DML. Apply stays refused.
-- AutoMigration owns that statement.
--
-- Usage:
--   lua schematool_rows.lua --expected PATH --catalog PATH --engine E
--        --schema S [--only-tables a,b] [--from N] [--to N] --out PATH
--   lua schematool_rows.lua --probe-sqlite DB --rows PATH --live-out PATH
--   lua schematool_rows.lua --compare --rows PATH [--live PATH]
--        --findings-out PATH [--sql-only]
--
-- CHANGELOG
-- 1.0.2 - 2026-10-07 - Encode does not shadow the unkeyed helper
-- 1.0.1 - 2026-10-07 - apply_insert no longer returns an undefined name
-- 1.0.0 - 2026-10-07 - Keyed default rows, targeted probe, row findings

-- luacheck: globals arg

local function script_dir()
    local src = (arg and arg[0]) or ""
    local dir = src:match("^(.*)/[^/]+$")
    return dir or "."
end

package.path = script_dir() .. "/?.lua;" .. package.path

local apply = require("schemahelper_apply")

local M = {}

local function json_escape(s)
    s = tostring(s or "")
    s = s:gsub("\\", "\\\\")
    s = s:gsub('"', '\\"')
    s = s:gsub("\n", "\\n")
    s = s:gsub("\r", "\\r")
    s = s:gsub("\t", "\\t")
    return s
end

local function sh_quote(s)
    return "'" .. tostring(s):gsub("'", "'\\''") .. "'"
end

local function utf8_from_code(code)
    if type(code) ~= "number" or code < 0 or code > 0x10FFFF then
        return "?"
    end
    if code >= 0xD800 and code <= 0xDFFF then
        return "?"
    end
    if code < 0x80 then
        return string.char(code)
    end
    if code < 0x800 then
        local b1 = 0xC0 + math.floor(code / 64)
        local b2 = 0x80 + (code % 64)
        return string.char(b1, b2)
    end
    if code < 0x10000 then
        local b1 = 0xE0 + math.floor(code / 4096)
        local b2 = 0x80 + (math.floor(code / 64) % 64)
        local b3 = 0x80 + (code % 64)
        return string.char(b1, b2, b3)
    end
    local b1 = 0xF0 + math.floor(code / 262144)
    local b2 = 0x80 + (math.floor(code / 4096) % 64)
    local b3 = 0x80 + (math.floor(code / 64) % 64)
    local b4 = 0x80 + (code % 64)
    return string.char(b1, b2, b3, b4)
end

local function skip_ws(s, i)
    local n = #s
    while i <= n do
        local c = s:sub(i, i)
        if c:match("%s") then
            i = i + 1
        elseif c == "-" and s:sub(i + 1, i + 1) == "-" then
            local nl = s:find("\n", i, true)
            if not nl then
                return n + 1
            end
            i = nl + 1
        elseif c == "/" and s:sub(i + 1, i + 1) == "*" then
            local e = s:find("*/", i + 2, true)
            if not e then
                return n + 1
            end
            i = e + 2
        else
            return i
        end
    end
    return i
end

local function keyword_at(s, i, word)
    local n = #word
    if s:sub(i, i + n - 1):upper() ~= word:upper() then
        return nil
    end
    local after = s:sub(i + n, i + n)
    if after:match("[%w_]") then
        return nil
    end
    return i + n
end

local function read_ident(s, i)
    i = skip_ws(s, i)
    local c = s:sub(i, i)
    if c == '"' or c == "`" or c == "[" then
        local close = (c == "[") and "]" or c
        local j = i + 1
        local parts = {}
        while j <= #s do
            local d = s:sub(j, j)
            if d == close then
                if s:sub(j + 1, j + 1) == close then
                    parts[#parts + 1] = close
                    j = j + 2
                else
                    return table.concat(parts), j + 1
                end
            else
                parts[#parts + 1] = d
                j = j + 1
            end
        end
        return nil, j
    end
    local ident = s:match("^([%w_]+)", i)
    if not ident then
        return nil, i
    end
    return ident, i + #ident
end

local function read_table(s, i)
    local part, j = read_ident(s, i)
    if not part then
        return nil, i
    end
    local last = part
    while true do
        local k = skip_ws(s, j)
        if s:sub(k, k) ~= "." then
            return last, j
        end
        local nxt
        nxt, j = read_ident(s, k + 1)
        if not nxt then
            return last, k
        end
        last = nxt
    end
end

local function read_sq(s, i)
    local q = s:sub(i, i)
    local j = i + 1
    local parts = {}
    while j <= #s do
        local c = s:sub(j, j)
        if c == q then
            if s:sub(j + 1, j + 1) == q then
                parts[#parts + 1] = q
                j = j + 2
            else
                return { kind = "str", text = table.concat(parts) }, j + 1
            end
        else
            parts[#parts + 1] = c
            j = j + 1
        end
    end
    return nil, j
end

local function read_dollar(s, i)
    local tag = s:match("^%$([%w_]*)%$", i)
    if not tag then
        return nil, i
    end
    local open = "$" .. tag .. "$"
    local start_at = i + #open
    local close_at = s:find(open, start_at, true)
    if not close_at then
        return nil, i
    end
    local body = s:sub(start_at, close_at - 1)
    return { kind = "str", text = body }, close_at + #open
end

local function skip_expr(s, i)
    local depth = 0
    local n = #s
    while i <= n do
        i = skip_ws(s, i)
        if i > n then
            return i
        end
        local c = s:sub(i, i)
        if c == "'" or c == '"' or c == "`" then
            local _, j = read_sq(s, i)
            if not j or j <= i then
                return i + 1
            end
            i = j
        elseif c == "$" then
            local _, j = read_dollar(s, i)
            if j and j > i then
                i = j
            else
                i = i + 1
            end
        elseif c == "(" then
            depth = depth + 1
            i = i + 1
        elseif c == ")" then
            if depth == 0 then
                return i
            end
            depth = depth - 1
            i = i + 1
        elseif c == "," and depth == 0 then
            return i
        else
            i = i + 1
        end
    end
    return i
end

local function read_value(s, i)
    i = skip_ws(s, i)
    local c = s:sub(i, i)
    if c == "'" then
        return read_sq(s, i)
    end
    if c == "$" then
        local v, j = read_dollar(s, i)
        if v then
            return v, j
        end
    end
    local npos = keyword_at(s, i, "N")
    if npos then
        local q = skip_ws(s, npos)
        if s:sub(q, q) == "'" then
            return read_sq(s, q)
        end
    end
    local j = keyword_at(s, i, "NULL")
    if j then
        return { kind = "null" }, j
    end
    j = keyword_at(s, i, "TRUE")
    if j then
        return { kind = "bool", text = "TRUE" }, j
    end
    j = keyword_at(s, i, "FALSE")
    if j then
        return { kind = "bool", text = "FALSE" }, j
    end
    local num = s:match("^(%-?%d+%.?%d*)", i)
    if num then
        local after = s:sub(i + #num, i + #num)
        if not after:match("[%w_]") then
            return { kind = "num", text = num }, i + #num
        end
    end
    local e = skip_expr(s, i)
    if e == i then
        return nil, i
    end
    return { kind = "expr" }, e
end

local function read_tuple(s, i)
    i = skip_ws(s, i)
    if s:sub(i, i) ~= "(" then
        return nil, i
    end
    i = i + 1
    local vals = {}
    while true do
        i = skip_ws(s, i)
        if s:sub(i, i) == ")" then
            return vals, i + 1
        end
        local v, j = read_value(s, i)
        if not v then
            return nil, i
        end
        vals[#vals + 1] = v
        i = skip_ws(s, j)
        if s:sub(i, i) == "," then
            i = i + 1
        elseif s:sub(i, i) == ")" then
            return vals, i + 1
        else
            return nil, i
        end
    end
end

local function read_columns(s, i)
    i = skip_ws(s, i)
    if s:sub(i, i) ~= "(" then
        return nil, i
    end
    i = i + 1
    local cols = {}
    while true do
        i = skip_ws(s, i)
        if s:sub(i, i) == ")" then
            return cols, i + 1
        end
        local id, j = read_ident(s, i)
        if not id or not id:match("^[%w_]+$") then
            return nil, i
        end
        cols[#cols + 1] = id:lower()
        i = skip_ws(s, j)
        if s:sub(i, i) == "," then
            i = i + 1
        elseif s:sub(i, i) == ")" then
            return cols, i + 1
        else
            return nil, i
        end
    end
end

local function parse_where(s, i)
    i = skip_ws(s, i)
    local j = keyword_at(s, i, "WHERE")
    if not j then
        return nil, "WHERE does not bind one row"
    end
    i = j
    local preds = {}
    while true do
        local col
        col, i = read_ident(s, i)
        if not col or not col:match("^[%w_]+$") then
            return nil, "WHERE does not bind one row"
        end
        i = skip_ws(s, i)
        if s:sub(i, i) ~= "=" then
            return nil, "WHERE does not bind one row"
        end
        i = i + 1
        local val
        val, i = read_value(s, i)
        if not val or val.kind == "expr" or val.kind == "null" then
            return nil, "WHERE does not bind one row"
        end
        preds[#preds + 1] = { col = col:lower(), val = val }
        i = skip_ws(s, i)
        if keyword_at(s, i, "AND") then
            i = keyword_at(s, i, "AND")
        else
            if s:sub(i):match("%S") then
                return nil, "WHERE does not bind one row"
            end
            return preds
        end
    end
end

local function unkeyed(op, tname, note)
    return {
        op = op,
        table = (tname or ""):lower(),
        unkeyed = true,
        note = note or "WHERE does not bind one row",
    }
end

local function parse_insert(s, i)
    i = keyword_at(s, i, "INSERT")
    i = skip_ws(s, i)
    i = keyword_at(s, i, "INTO")
    if not i then
        return unkeyed("insert", "", "INSERT has no target table")
    end
    local tname
    tname, i = read_table(s, i)
    if not tname then
        return unkeyed("insert", "", "INSERT has no target table")
    end
    if tname:lower() == "queries" then
        return { skip = true }
    end
    i = skip_ws(s, i)
    local cols
    if s:sub(i, i) == "(" then
        local id = read_ident(s, i + 1)
        if id and id:upper() == "SELECT" then
            return unkeyed("insert", tname, "INSERT…SELECT has no literal key")
        end
        cols, i = read_columns(s, i)
        if not cols then
            return unkeyed("insert", tname, "INSERT column list is not names")
        end
    end
    i = skip_ws(s, i)
    if keyword_at(s, i, "SELECT") or keyword_at(s, i, "WITH") then
        return unkeyed("insert", tname, "INSERT…SELECT has no literal key")
    end
    if not keyword_at(s, i, "VALUES") then
        return unkeyed("insert", tname, "INSERT has no VALUES list")
    end
    i = keyword_at(s, i, "VALUES")
    if not cols then
        return unkeyed("insert", tname, "INSERT does not name columns")
    end
    local rows = {}
    while true do
        local vals
        vals, i = read_tuple(s, i)
        if not vals then
            return unkeyed("insert", tname, "INSERT VALUES are not literals")
        end
        rows[#rows + 1] = vals
        i = skip_ws(s, i)
        if s:sub(i, i) == "," then
            i = i + 1
        else
            break
        end
    end
    return {
        op = "insert",
        table = tname:lower(),
        columns = cols,
        rows = rows,
    }
end

local function parse_update(s, i)
    i = keyword_at(s, i, "UPDATE")
    local tname
    tname, i = read_table(s, i)
    if not tname then
        return unkeyed("update", "", "UPDATE has no target table")
    end
    if tname:lower() == "queries" then
        return { skip = true }
    end
    i = skip_ws(s, i)
    if not keyword_at(s, i, "SET") then
        return unkeyed("update", tname, "UPDATE has no SET list")
    end
    i = keyword_at(s, i, "SET")
    local sets = {}
    while true do
        local col
        col, i = read_ident(s, i)
        if not col or not col:match("^[%w_]+$") then
            return unkeyed("update", tname, "SET list is not keyed")
        end
        i = skip_ws(s, i)
        if s:sub(i, i) ~= "=" then
            return unkeyed("update", tname, "SET list is not keyed")
        end
        i = i + 1
        local val
        val, i = read_value(s, i)
        if not val then
            return unkeyed("update", tname, "SET value is not readable")
        end
        sets[#sets + 1] = { col = col:lower(), val = val }
        i = skip_ws(s, i)
        if s:sub(i, i) == "," then
            i = i + 1
        else
            break
        end
    end
    local preds, why = parse_where(s, i)
    if not preds then
        return unkeyed("update", tname, why)
    end
    return {
        op = "update",
        table = tname:lower(),
        sets = sets,
        preds = preds,
    }
end

local function parse_delete(s, i)
    i = keyword_at(s, i, "DELETE")
    i = skip_ws(s, i)
    if not keyword_at(s, i, "FROM") then
        return unkeyed("delete", "", "DELETE has no target table")
    end
    i = keyword_at(s, i, "FROM")
    local tname
    tname, i = read_table(s, i)
    if not tname then
        return unkeyed("delete", "", "DELETE has no target table")
    end
    if tname:lower() == "queries" then
        return { skip = true }
    end
    local preds, why = parse_where(s, i)
    if not preds then
        return unkeyed("delete", tname, why)
    end
    return { op = "delete", table = tname:lower(), preds = preds }
end

local function paren_list(text)
    local inside = text:match("%((.*)%)")
    if not inside then
        return nil
    end
    local cols = {}
    for col in inside:gmatch("[%w_]+") do
        cols[#cols + 1] = col:lower()
    end
    if #cols == 0 then
        return nil
    end
    return cols
end

local function ensure_key(keys, tname)
    tname = tname:lower()
    if not keys[tname] then
        keys[tname] = { pk = {}, unique = {} }
    end
    return keys[tname]
end

local function add_unique(info, cols)
    if not cols or #cols == 0 then
        return
    end
    info.unique[#info.unique + 1] = cols
end

local function note_create(stmt, keys)
    local i = skip_ws(stmt, 1)
    i = keyword_at(stmt, i, "CREATE")
    if not i then
        return
    end
    i = skip_ws(stmt, i)
    if not keyword_at(stmt, i, "TABLE") then
        return
    end
    i = keyword_at(stmt, i, "TABLE")
    local tname
    tname, i = read_table(stmt, i)
    if not tname then
        return
    end
    i = skip_ws(stmt, i)
    if keyword_at(stmt, i, "IF") then
        i = keyword_at(stmt, i, "IF")
        i = skip_ws(stmt, i)
        i = keyword_at(stmt, i, "NOT") or i
        i = skip_ws(stmt, i)
        i = keyword_at(stmt, i, "EXISTS") or i
        i = skip_ws(stmt, i)
    end
    if stmt:sub(i, i) ~= "(" then
        return
    end
    local depth = 0
    local j = i
    local body_at
    local body
    while j <= #stmt do
        local c = stmt:sub(j, j)
        if c == "'" or c == '"' or c == "`" then
            local _, nj = read_sq(stmt, j)
            j = nj or (j + 1)
        elseif c == "(" then
            depth = depth + 1
            if depth == 1 then
                body_at = j + 1
            end
            j = j + 1
        elseif c == ")" then
            depth = depth - 1
            if depth == 0 and body_at then
                body = stmt:sub(body_at, j - 1)
                break
            end
            j = j + 1
        else
            j = j + 1
        end
    end
    if type(body) ~= "string" then
        return
    end
    local info = ensure_key(keys, tname)
    local pk = {}
    local depth2 = 0
    local cur = {}
    local parts = {}
    local function push_part()
        local line = table.concat(cur)
        cur = {}
        if line:match("%S") then
            parts[#parts + 1] = line
        end
    end
    local p = 1
    while p <= #body do
        local c = body:sub(p, p)
        if c == "'" or c == '"' or c == "`" then
            local _, np = read_sq(body, p)
            if np then
                cur[#cur + 1] = body:sub(p, np - 1)
                p = np
            else
                cur[#cur + 1] = c
                p = p + 1
            end
        elseif c == "(" then
            depth2 = depth2 + 1
            cur[#cur + 1] = c
            p = p + 1
        elseif c == ")" then
            if depth2 > 0 then
                depth2 = depth2 - 1
            end
            cur[#cur + 1] = c
            p = p + 1
        elseif c == "," and depth2 == 0 then
            push_part()
            p = p + 1
        else
            cur[#cur + 1] = c
            p = p + 1
        end
    end
    push_part()
    for _, line in ipairs(parts) do
        local trimmed = line:gsub("^%s+", ""):gsub("%s+$", "")
        local up = trimmed:upper()
        if up:match("^PRIMARY%s+KEY") or up:match("^CONSTRAINT%s") then
            if up:find("PRIMARY", 1, true) and up:find("KEY", 1, true) then
                local cols = paren_list(trimmed)
                if cols and #info.pk == 0 then
                    pk = cols
                end
            elseif up:find("UNIQUE", 1, true) then
                add_unique(info, paren_list(trimmed))
            end
        elseif up:match("^UNIQUE%s*%(") or up:match("^UNIQUE%s+") then
            add_unique(info, paren_list(trimmed))
        else
            local cname = trimmed:match("^([%w_]+)")
            if cname then
                local cu = cname:upper()
                if cu ~= "PRIMARY" and cu ~= "CONSTRAINT" and cu ~= "UNIQUE"
                    and cu ~= "FOREIGN" and cu ~= "CHECK" and cu ~= "KEY" then
                    if up:find("PRIMARY%s+KEY") then
                        pk[#pk + 1] = cname:lower()
                    elseif up:find("UNIQUE", 1, true)
                        and not up:find("PRIMARY", 1, true) then
                        add_unique(info, { cname:lower() })
                    end
                end
            end
        end
    end
    if #info.pk == 0 and #pk > 0 then
        info.pk = pk
    end
end

local function split_statements(chunk)
    local stmts = {}
    local parts = {}
    local function flush()
        local text = table.concat(parts)
        parts = {}
        if text:match("%S") then
            stmts[#stmts + 1] = text
        end
    end
    local i = 1
    local n = #chunk
    local depth = 0
    while i <= n do
        local c = chunk:sub(i, i)
        if c == "-" and chunk:sub(i + 1, i + 1) == "-" then
            parts[#parts + 1] = " "
            local nl = chunk:find("\n", i, true)
            if not nl then
                break
            end
            i = nl + 1
        elseif c == "/" and chunk:sub(i + 1, i + 1) == "*" then
            parts[#parts + 1] = " "
            local e = chunk:find("*/", i + 2, true)
            if not e then
                break
            end
            i = e + 2
        elseif c == "'" or c == '"' or c == "`" then
            local j = i + 1
            while j <= n do
                if chunk:sub(j, j) == c then
                    if chunk:sub(j + 1, j + 1) == c then
                        j = j + 2
                    else
                        j = j + 1
                        break
                    end
                else
                    j = j + 1
                end
            end
            parts[#parts + 1] = chunk:sub(i, j - 1)
            i = j
        elseif c == "$" then
            local tag = chunk:match("^%$([%w_]*)%$", i)
            if tag then
                local open = "$" .. tag .. "$"
                local close_at = chunk:find(open, i + #open, true)
                if close_at then
                    local j = close_at + #open
                    parts[#parts + 1] = chunk:sub(i, j - 1)
                    i = j
                else
                    parts[#parts + 1] = c
                    i = i + 1
                end
            else
                parts[#parts + 1] = c
                i = i + 1
            end
        elseif c == "(" then
            depth = depth + 1
            parts[#parts + 1] = c
            i = i + 1
        elseif c == ")" then
            if depth > 0 then
                depth = depth - 1
            end
            parts[#parts + 1] = c
            i = i + 1
        elseif c == ";" and depth == 0 then
            flush()
            i = i + 1
        else
            parts[#parts + 1] = c
            i = i + 1
        end
    end
    flush()
    return stmts
end

local function split_code(code)
    local chunks = {}
    local rest = code or ""
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
    local stmts = {}
    for _, chunk in ipairs(chunks) do
        local part = split_statements(chunk)
        for _, st in ipairs(part) do
            stmts[#stmts + 1] = st
        end
    end
    return stmts
end

local function classify(stmt, keys)
    local i = skip_ws(stmt, 1)
    if keyword_at(stmt, i, "CREATE") then
        note_create(stmt, keys)
        return nil
    end
    if keyword_at(stmt, i, "INSERT") then
        return parse_insert(stmt, i)
    end
    if keyword_at(stmt, i, "UPDATE") then
        return parse_update(stmt, i)
    end
    if keyword_at(stmt, i, "DELETE") then
        return parse_delete(stmt, i)
    end
    return nil
end

local function display_key(parts)
    if #parts == 1 then
        return parts[1]
    end
    return table.concat(parts, "|")
end

local function same_list(a, b)
    if #a ~= #b then
        return false
    end
    for i = 1, #a do
        if a[i] ~= b[i] then
            return false
        end
    end
    return true
end

local function take_literal(v)
    if not v or v.kind == "expr" then
        return nil
    end
    if v.kind == "null" then
        return { null = true }
    end
    if v.kind == "num" then
        return { text = v.text, num = true }
    end
    if v.kind == "bool" or v.kind == "str" then
        return { text = v.text }
    end
    return nil
end

local function blank_row(key_cols, parts, ref)
    return {
        key = display_key(parts),
        parts = parts,
        key_cols = key_cols,
        ref = ref,
        values = {},
        nums = {},
        nulls = {},
        order = {},
    }
end

local function put_col(row, col, lit)
    local seen = row.values[col] ~= nil or row.nulls[col]
    if not seen then
        row.order[#row.order + 1] = col
    end
    if lit.null then
        row.values[col] = nil
        row.nums[col] = nil
        row.nulls[col] = true
        return
    end
    row.nulls[col] = nil
    row.values[col] = lit.text
    if lit.num then
        row.nums[col] = true
    else
        row.nums[col] = nil
    end
end

local function bind_columns(info, present)
    local function try(cols)
        if not cols or #cols == 0 then
            return nil
        end
        local parts = {}
        for _, col in ipairs(cols) do
            local lit = present[col]
            if not lit or not lit.text then
                return nil
            end
            parts[#parts + 1] = lit.text
        end
        return cols, parts
    end
    local cols, parts = try(info.pk)
    if cols then
        return cols, parts
    end
    for _, uniq in ipairs(info.unique or {}) do
        cols, parts = try(uniq)
        if cols then
            return cols, parts
        end
    end
    return nil
end

local function ensure_state(model, tname)
    local st = model.tables[tname]
    if st then
        return st
    end
    st = { key_cols = {}, rows = {}, tombs = {} }
    model.tables[tname] = st
    model.table_order[#model.table_order + 1] = tname
    return st
end

local function remember_unkeyed(model, ref, dml)
    model.unkeyed[#model.unkeyed + 1] = {
        ref = ref,
        table = dml.table or "",
        op = dml.op or "",
        note = dml.note or "WHERE does not bind one row",
    }
end

local function preds_present(preds)
    local present = {}
    for _, pred in ipairs(preds) do
        if present[pred.col] then
            return nil
        end
        present[pred.col] = take_literal(pred.val)
    end
    return present
end

local function accept_key(st, key_cols)
    if #st.key_cols == 0 then
        st.key_cols = key_cols
        return true
    end
    return same_list(st.key_cols, key_cols)
end

local function apply_insert(model, keys, dml, ref)
    local info = ensure_key(keys, dml.table)
    local st = ensure_state(model, dml.table)
    for _, vals in ipairs(dml.rows or {}) do
        if #vals ~= #dml.columns then
            remember_unkeyed(model, ref, unkeyed(
                "insert", dml.table, "INSERT column and value counts differ"))
        else
            local present = {}
            local lits = {}
            for n, col in ipairs(dml.columns) do
                local lit = take_literal(vals[n])
                if lit then
                    lits[col] = lit
                    if lit.text then
                        present[col] = lit
                    end
                end
            end
            local key_cols, parts = bind_columns(info, present)
            if not key_cols or not accept_key(st, key_cols) then
                remember_unkeyed(model, ref, unkeyed(
                    "insert", dml.table, "INSERT does not supply a key"))
            else
                local row = blank_row(key_cols, parts, ref)
                for _, col in ipairs(key_cols) do
                    put_col(row, col, lits[col])
                end
                for _, col in ipairs(dml.columns) do
                    if lits[col] and not row.values[col] and not row.nulls[col] then
                        put_col(row, col, lits[col])
                    end
                end
                st.rows[row.key] = row
                st.tombs[row.key] = nil
            end
        end
    end
end

local function apply_update(model, keys, dml, ref)
    local info = ensure_key(keys, dml.table)
    local present = preds_present(dml.preds or {})
    if not present then
        remember_unkeyed(model, ref, dml)
        return
    end
    local key_cols, parts = bind_columns(info, present)
    if not key_cols then
        remember_unkeyed(model, ref, unkeyed(
            "update", dml.table, "WHERE does not match a key"))
        return
    end
    local st = ensure_state(model, dml.table)
    if not accept_key(st, key_cols) then
        remember_unkeyed(model, ref, unkeyed(
            "update", dml.table, "WHERE does not match the table key"))
        return
    end
    local row = st.rows[display_key(parts)]
    if not row then
        row = blank_row(key_cols, parts, ref)
        for i, col in ipairs(key_cols) do
            local lit = present[col]
            put_col(row, col, lit or { text = parts[i] })
        end
        st.rows[row.key] = row
        st.tombs[row.key] = nil
    end
    row.ref = ref
    for _, set in ipairs(dml.sets or {}) do
        local lit = take_literal(set.val)
        if lit then
            put_col(row, set.col, lit)
        end
    end
end

local function apply_delete(model, keys, dml, ref)
    local info = ensure_key(keys, dml.table)
    local present = preds_present(dml.preds or {})
    if not present then
        remember_unkeyed(model, ref, dml)
        return
    end
    local key_cols, parts = bind_columns(info, present)
    if not key_cols then
        remember_unkeyed(model, ref, unkeyed(
            "delete", dml.table, "WHERE does not match a key"))
        return
    end
    local st = ensure_state(model, dml.table)
    if not accept_key(st, key_cols) then
        remember_unkeyed(model, ref, unkeyed(
            "delete", dml.table, "WHERE does not match the table key"))
        return
    end
    local key = display_key(parts)
    local tomb = blank_row(key_cols, parts, ref)
    for _, col in ipairs(key_cols) do
        put_col(tomb, col, present[col])
    end
    st.rows[key] = nil
    st.tombs[key] = tomb
end

local function apply_dml(model, keys, dml, ref, only)
    if not dml or dml.skip then
        return
    end
    local tname = dml.table or ""
    if only and tname ~= "" and not only[tname] then
        return
    end
    if only and tname == "" then
        return
    end
    if dml.unkeyed then
        remember_unkeyed(model, ref, dml)
        return
    end
    if dml.op == "insert" then
        apply_insert(model, keys, dml, ref)
    elseif dml.op == "update" then
        apply_update(model, keys, dml, ref)
    elseif dml.op == "delete" then
        apply_delete(model, keys, dml, ref)
    end
end

local function sql_lit(engine, text, is_num, is_null)
    if is_null then
        return "NULL"
    end
    if is_num then
        if not tostring(text):match("^%-?%d+%.?%d*$") then
            return nil
        end
        return text
    end
    return apply.field_literal(engine, text)
end

local function key_sort(a, b)
    local na = tonumber(a)
    local nb = tonumber(b)
    if na and nb and na ~= nb then
        return na < nb
    end
    return tostring(a) < tostring(b)
end

function M.probe_sql(engine, schema, table_name, key_cols, data_cols, probes)
    if not probes or #probes == 0 then
        return nil
    end
    if not apply.qualify_table then
        return nil
    end
    local qualified = apply.qualify_table(engine, schema, table_name)
    local cols = {}
    for _, col in ipairs(key_cols) do
        if not col:match("^[%w_]+$") then
            return nil
        end
        cols[#cols + 1] = col
    end
    for _, col in ipairs(data_cols) do
        if not col:match("^[%w_]+$") then
            return nil
        end
        cols[#cols + 1] = col
    end
    local wheres = {}
    for _, probe in ipairs(probes) do
        local preds = {}
        for i, col in ipairs(key_cols) do
            local lit = sql_lit(engine, probe.parts[i], probe.nums[col], false)
            if not lit then
                return nil
            end
            preds[#preds + 1] = col .. " = " .. lit
        end
        wheres[#wheres + 1] = "(" .. table.concat(preds, " AND ") .. ")"
    end
    return "SELECT " .. table.concat(cols, ", ")
        .. " FROM " .. qualified
        .. " WHERE " .. table.concat(wheres, " OR ") .. ";"
end

local function data_columns(st)
    local seen = {}
    for _, col in ipairs(st.key_cols) do
        seen[col] = true
    end
    local cols = {}
    local function add(col)
        if not seen[col] then
            seen[col] = true
            cols[#cols + 1] = col
        end
    end
    local keys = {}
    for key, _ in pairs(st.rows) do
        keys[#keys + 1] = key
    end
    table.sort(keys, key_sort)
    for _, key in ipairs(keys) do
        local row = st.rows[key]
        for _, col in ipairs(row.order) do
            add(col)
        end
    end
    return cols
end

local function probe_list(st)
    local list = {}
    local function add(row)
        local nums = {}
        for _, col in ipairs(st.key_cols) do
            if row.nums[col] then
                nums[col] = true
            end
        end
        list[#list + 1] = { parts = row.parts, nums = nums, key = row.key }
    end
    local keys = {}
    for key, _ in pairs(st.rows) do
        keys[#keys + 1] = key
    end
    table.sort(keys, key_sort)
    for _, key in ipairs(keys) do
        add(st.rows[key])
    end
    local tombs = {}
    for key, _ in pairs(st.tombs) do
        if not st.rows[key] then
            tombs[#tombs + 1] = key
        end
    end
    table.sort(tombs, key_sort)
    for _, key in ipairs(tombs) do
        add(st.tombs[key])
    end
    return list
end

local function finish_probes(model, opts)
    opts = opts or {}
    local engine = opts.engine or "sqlite"
    local schema = opts.schema or ""
    for _, tname in ipairs(model.table_order) do
        local st = model.tables[tname]
        local probes = probe_list(st)
        st.probes = probes
        st.data_cols = data_columns(st)
        st.probe_sql = M.probe_sql(
            engine, schema, tname, st.key_cols, st.data_cols, probes)
    end
end

function M.extract(items, catalog_keys, opts)
    opts = opts or {}
    local keys = {}
    for tname, info in pairs(catalog_keys or {}) do
        local copy = { pk = {}, unique = {} }
        for _, col in ipairs(info.pk or {}) do
            copy.pk[#copy.pk + 1] = tostring(col):lower()
        end
        for _, uniq in ipairs(info.unique or {}) do
            local one = {}
            for _, col in ipairs(uniq) do
                one[#one + 1] = tostring(col):lower()
            end
            copy.unique[#copy.unique + 1] = one
        end
        keys[tostring(tname):lower()] = copy
    end
    local only
    if opts.only then
        only = {}
        for name, _ in pairs(opts.only) do
            only[tostring(name):lower()] = true
        end
    end
    local model = {
        engine = opts.engine or "",
        schema = opts.schema or "",
        tables = {},
        table_order = {},
        unkeyed = {},
    }
    for _, item in ipairs(items or {}) do
        local ref = item.ref or 0
        local in_range = true
        if opts.from_ref and ref < opts.from_ref then
            in_range = false
        end
        if opts.to_ref and ref > opts.to_ref then
            in_range = false
        end
        if in_range then
            for _, stmt in ipairs(split_code(item.code or "")) do
                local dml = classify(stmt, keys)
                apply_dml(model, keys, dml, ref, only)
            end
        end
    end
    finish_probes(model, opts)
    return model
end

local function encode_row(row)
    local nums = {}
    local nulls = {}
    local values = {}
    for col, text in pairs(row.values) do
        values[col] = text
        if row.nums[col] then
            nums[#nums + 1] = col
        end
    end
    for col, _ in pairs(row.nulls) do
        nulls[#nulls + 1] = col
    end
    table.sort(nums)
    table.sort(nulls)
    return {
        key = row.key,
        ref = row.ref,
        order = row.order,
        parts = row.parts,
        values = values,
        nums = nums,
        nulls = nulls,
    }
end

local function encode_string(s)
    return '"' .. json_escape(s) .. '"'
end

local function encode_value(v)
    local tv = type(v)
    if tv == "string" then
        return encode_string(v)
    end
    if tv == "number" then
        return tostring(v)
    end
    if tv == "boolean" then
        if v then
            return "true"
        end
        return "false"
    end
    if v == nil then
        return "null"
    end
    return encode_string(tostring(v))
end

local function encode_array(arr)
    local parts = {}
    for i = 1, #arr do
        parts[#parts + 1] = encode_value(arr[i])
    end
    return "[" .. table.concat(parts, ",") .. "]"
end

local function encode_map(map)
    local keys = {}
    for k, _ in pairs(map) do
        keys[#keys + 1] = k
    end
    table.sort(keys)
    local parts = {}
    for _, k in ipairs(keys) do
        parts[#parts + 1] = encode_string(k) .. ":" .. encode_value(map[k])
    end
    return "{" .. table.concat(parts, ",") .. "}"
end

function M.encode_model(model)
    local tables = {}
    for _, tname in ipairs(model.table_order) do
        local st = model.tables[tname]
        local rows = {}
        local keys = {}
        for key, _ in pairs(st.rows) do
            keys[#keys + 1] = key
        end
        table.sort(keys, key_sort)
        for _, key in ipairs(keys) do
            rows[#rows + 1] = encode_row(st.rows[key])
        end
        local tombs = {}
        local tkeys = {}
        for key, _ in pairs(st.tombs) do
            tkeys[#tkeys + 1] = key
        end
        table.sort(tkeys, key_sort)
        for _, key in ipairs(tkeys) do
            tombs[#tombs + 1] = encode_row(st.tombs[key])
        end
        tables[#tables + 1] = {
            table = tname,
            key = st.key_cols,
            columns = st.data_cols or {},
            probe_sql = st.probe_sql or "",
            rows = rows,
            tombstones = tombs,
        }
    end
    local loose = model.unkeyed or {}
    local parts = {
        '{"engine":', encode_string(model.engine or ""),
        ',"schema":', encode_string(model.schema or ""),
        ',"tables":[',
    }
    for i, t in ipairs(tables) do
        if i > 1 then
            parts[#parts + 1] = ","
        end
        parts[#parts + 1] = '{"table":' .. encode_string(t.table)
        parts[#parts + 1] = ',"key":' .. encode_array(t.key)
        parts[#parts + 1] = ',"columns":' .. encode_array(t.columns)
        parts[#parts + 1] = ',"probe_sql":' .. encode_string(t.probe_sql)
        parts[#parts + 1] = ',"rows":['
        for r, row in ipairs(t.rows) do
            if r > 1 then
                parts[#parts + 1] = ","
            end
            parts[#parts + 1] = '{"key":' .. encode_string(row.key)
            parts[#parts + 1] = ',"ref":' .. tostring(row.ref or 0)
            parts[#parts + 1] = ',"order":' .. encode_array(row.order)
            parts[#parts + 1] = ',"parts":' .. encode_array(row.parts)
            parts[#parts + 1] = ',"values":' .. encode_map(row.values)
            parts[#parts + 1] = ',"nums":' .. encode_array(row.nums)
            parts[#parts + 1] = ',"nulls":' .. encode_array(row.nulls)
            parts[#parts + 1] = "}"
        end
        parts[#parts + 1] = '],"tombstones":['
        for r, row in ipairs(t.tombstones) do
            if r > 1 then
                parts[#parts + 1] = ","
            end
            parts[#parts + 1] = '{"key":' .. encode_string(row.key)
            parts[#parts + 1] = ',"ref":' .. tostring(row.ref or 0)
            parts[#parts + 1] = ',"order":' .. encode_array(row.order)
            parts[#parts + 1] = ',"parts":' .. encode_array(row.parts)
            parts[#parts + 1] = ',"values":' .. encode_map(row.values)
            parts[#parts + 1] = ',"nums":' .. encode_array(row.nums)
            parts[#parts + 1] = ',"nulls":' .. encode_array(row.nulls)
            parts[#parts + 1] = "}"
        end
        parts[#parts + 1] = "]}"
    end
    parts[#parts + 1] = '],"unkeyed":['
    for i, u in ipairs(loose) do
        if i > 1 then
            parts[#parts + 1] = ","
        end
        parts[#parts + 1] = '{"ref":' .. tostring(u.ref or 0)
        parts[#parts + 1] = ',"table":' .. encode_string(u.table or "")
        parts[#parts + 1] = ',"op":' .. encode_string(u.op or "")
        parts[#parts + 1] = ',"note":' .. encode_string(u.note or "")
        parts[#parts + 1] = "}"
    end
    parts[#parts + 1] = "]}"
    return table.concat(parts)
end

-- JSON reader for the model file and for sqlite3 -json probe output.
local function parse_json(text)
    local i = 1
    local n = #text
    local function skip()
        while i <= n and text:sub(i, i):match("%s") do
            i = i + 1
        end
    end
    local parse_value
    local function parse_string()
        i = i + 1
        local parts = {}
        while i <= n do
            local c = text:sub(i, i)
            if c == '"' then
                i = i + 1
                return table.concat(parts)
            end
            if c == "\\" then
                local e = text:sub(i + 1, i + 1)
                if e == "u" then
                    local hex = text:sub(i + 2, i + 5)
                    local code = tonumber(hex, 16)
                    i = i + 6
                    if code and code >= 0xD800 and code <= 0xDBFF
                        and text:sub(i, i + 1) == "\\u" then
                        local low = tonumber(text:sub(i + 2, i + 5), 16)
                        if low and low >= 0xDC00 and low <= 0xDFFF then
                            code = 0x10000
                                + ((code - 0xD800) * 0x400)
                                + (low - 0xDC00)
                            i = i + 6
                        end
                    end
                    parts[#parts + 1] = utf8_from_code(code)
                elseif e == "n" then
                    parts[#parts + 1] = "\n"
                    i = i + 2
                elseif e == "r" then
                    parts[#parts + 1] = "\r"
                    i = i + 2
                elseif e == "t" then
                    parts[#parts + 1] = "\t"
                    i = i + 2
                else
                    parts[#parts + 1] = e
                    i = i + 2
                end
            else
                parts[#parts + 1] = c
                i = i + 1
            end
        end
        return nil
    end
    local function parse_array()
        i = i + 1
        local arr = {}
        skip()
        if text:sub(i, i) == "]" then
            i = i + 1
            return arr
        end
        while true do
            arr[#arr + 1] = parse_value()
            skip()
            local c = text:sub(i, i)
            if c == "," then
                i = i + 1
                skip()
            elseif c == "]" then
                i = i + 1
                return arr
            else
                return nil
            end
        end
    end
    local function parse_object()
        i = i + 1
        local obj = {}
        skip()
        if text:sub(i, i) == "}" then
            i = i + 1
            return obj
        end
        while true do
            skip()
            if text:sub(i, i) ~= '"' then
                return nil
            end
            local key = parse_string()
            skip()
            if text:sub(i, i) ~= ":" then
                return nil
            end
            i = i + 1
            obj[key] = parse_value()
            skip()
            local c = text:sub(i, i)
            if c == "," then
                i = i + 1
            elseif c == "}" then
                i = i + 1
                return obj
            else
                return nil
            end
        end
    end
    parse_value = function()
        skip()
        local c = text:sub(i, i)
        if c == '"' then
            return parse_string()
        end
        if c == "{" then
            return parse_object()
        end
        if c == "[" then
            return parse_array()
        end
        if text:sub(i, i + 3) == "null" then
            i = i + 4
            return nil
        end
        if text:sub(i, i + 3) == "true" then
            i = i + 4
            return true
        end
        if text:sub(i, i + 4) == "false" then
            i = i + 5
            return false
        end
        local num = text:match("^(%-?%d+%.?%d*)", i)
        if num then
            i = i + #num
            return tonumber(num)
        end
        return nil
    end
    local value = parse_value()
    return value
end

local function list_flags(arr)
    local set = {}
    for _, name in ipairs(arr or {}) do
        set[name] = true
    end
    return set
end

local function row_from_json(obj, key_cols)
    local nums = list_flags(obj.nums)
    local nulls = list_flags(obj.nulls)
    local values = {}
    for col, text in pairs(obj.values or {}) do
        if text ~= nil then
            values[col] = tostring(text)
        end
    end
    return {
        key = tostring(obj.key),
        ref = tonumber(obj.ref) or 0,
        parts = obj.parts or {},
        key_cols = key_cols,
        order = obj.order or {},
        values = values,
        nums = nums,
        nulls = nulls,
    }
end

function M.decode_model(text)
    local obj = parse_json(text)
    if type(obj) ~= "table" then
        return nil, "rows JSON is not an object"
    end
    local model = {
        engine = obj.engine or "",
        schema = obj.schema or "",
        tables = {},
        table_order = {},
        unkeyed = obj.unkeyed or {},
    }
    for _, t in ipairs(obj.tables or {}) do
        local key_cols = t.key or {}
        local st = {
            key_cols = key_cols,
            data_cols = t.columns or {},
            probe_sql = t.probe_sql or "",
            rows = {},
            tombs = {},
        }
        for _, row in ipairs(t.rows or {}) do
            local got = row_from_json(row, key_cols)
            st.rows[got.key] = got
        end
        for _, row in ipairs(t.tombstones or {}) do
            local got = row_from_json(row, key_cols)
            st.tombs[got.key] = got
        end
        model.tables[t.table] = st
        model.table_order[#model.table_order + 1] = t.table
    end
    return model
end

local function csv_join(list)
    return table.concat(list or {}, ",")
end

function M.expected_payload(row, cols)
    local nums = {}
    local nulls = {}
    local parts = {}
    parts[#parts + 1] = '{"__keys":"'
        .. json_escape(csv_join(row.key_cols)) .. '"'
    parts[#parts + 1] = ',"__order":"' .. json_escape(csv_join(cols)) .. '"'
    for _, col in ipairs(cols) do
        if row.nulls[col] then
            nulls[#nulls + 1] = col
        else
            local text = row.values[col]
            if text ~= nil then
                if row.nums[col] then
                    nums[#nums + 1] = col
                end
                parts[#parts + 1] = ',"' .. json_escape(col) .. '":"'
                    .. json_escape(text) .. '"'
            end
        end
    end
    if #nums > 0 then
        parts[#parts + 1] = ',"__nums":"' .. json_escape(csv_join(nums)) .. '"'
    end
    if #nulls > 0 then
        parts[#parts + 1] = ',"__nulls":"' .. json_escape(csv_join(nulls)) .. '"'
    end
    parts[#parts + 1] = "}"
    return table.concat(parts)
end

local function live_equal(row, col, live_val)
    if row.nulls[col] then
        return live_val == nil
    end
    if live_val == nil then
        return false
    end
    return tostring(live_val) == tostring(row.values[col])
end

local function live_bits(cols, live_row)
    local bits = {}
    for _, col in ipairs(cols) do
        local v = live_row and live_row[col]
        if v == nil then
            bits[#bits + 1] = col .. "=NULL"
        else
            bits[#bits + 1] = col .. "=" .. tostring(v)
        end
    end
    return table.concat(bits, ", ")
end

local function push_finding(list, item)
    list[#list + 1] = item
end

function M.compare(model, live, sql_only)
    live = live or {}
    local findings = {}
    local counts = {
        row_missing = 0,
        row_diff = 0,
        row_present = 0,
        unkeyed = 0,
    }
    for i, u in ipairs(model.unkeyed or {}) do
        counts.unkeyed = counts.unkeyed + 1
        local tname = u.table ~= "" and u.table or "-"
        push_finding(findings, {
            id = string.format("row:%s:unkeyed:%s:%d", tname, tostring(u.ref or 0), i),
            class = "unkeyed DML",
            check = "unkeyed",
            object = u.table or "",
            column = "-",
            ref = u.ref or 0,
            expected = "",
            live = "",
            notes = "unkeyed DML — run Hydrogen AutoMigration",
        })
    end
    if sql_only then
        return findings, counts
    end
    local probed = live.probed or {}
    for _, tname in ipairs(model.table_order) do
        if probed[tname] then
            local st = model.tables[tname]
            local got = (live.tables and live.tables[tname]) or {}
            local keys = {}
            for key, _ in pairs(st.rows) do
                keys[#keys + 1] = key
            end
            table.sort(keys, key_sort)
            for _, key in ipairs(keys) do
                local row = st.rows[key]
                local have = got[key]
                if not have then
                    counts.row_missing = counts.row_missing + 1
                    push_finding(findings, {
                        id = string.format("row:%s:%s:row_missing", tname, key),
                        class = "default row",
                        check = "row_missing",
                        object = tname,
                        column = key,
                        ref = row.ref,
                        expected = M.expected_payload(row, row.order),
                        live = "absent",
                        notes = "missing default row " .. tname .. "." .. key,
                    })
                else
                    local diff_cols = {}
                    for _, col in ipairs(row.order) do
                        local is_key = false
                        for _, kc in ipairs(row.key_cols) do
                            if kc == col then
                                is_key = true
                            end
                        end
                        if not is_key and not live_equal(row, col, have[col]) then
                            diff_cols[#diff_cols + 1] = col
                        end
                    end
                    if #diff_cols > 0 then
                        local payload_cols = {}
                        for _, col in ipairs(row.key_cols) do
                            payload_cols[#payload_cols + 1] = col
                        end
                        for _, col in ipairs(diff_cols) do
                            payload_cols[#payload_cols + 1] = col
                        end
                        counts.row_diff = counts.row_diff + 1
                        push_finding(findings, {
                            id = string.format("row:%s:%s:row_diff", tname, key),
                            class = "default row",
                            check = "row_diff",
                            object = tname,
                            column = key,
                            ref = row.ref,
                            expected = M.expected_payload(row, payload_cols),
                            live = live_bits(diff_cols, have),
                            notes = "migration-owned columns differ on "
                                .. tname .. "." .. key,
                        })
                    end
                end
            end
            local tombs = {}
            for key, _ in pairs(st.tombs) do
                tombs[#tombs + 1] = key
            end
            table.sort(tombs, key_sort)
            for _, key in ipairs(tombs) do
                if got[key] then
                    local tomb = st.tombs[key]
                    counts.row_present = counts.row_present + 1
                    push_finding(findings, {
                        id = string.format("row:%s:%s:row_present", tname, key),
                        class = "default row",
                        check = "row_present",
                        object = tname,
                        column = key,
                        ref = tomb.ref,
                        expected = M.expected_payload(tomb, tomb.key_cols),
                        live = "present",
                        notes = "migration deleted " .. tname .. "." .. key
                            .. "; the row is still present",
                    })
                end
            end
        end
    end
    return findings, counts
end

function M.encode_findings(findings, counts)
    counts = counts or {}
    local exit_code = 0
    if (counts.row_missing or 0) > 0 or (counts.row_diff or 0) > 0
        or (counts.row_present or 0) > 0 then
        exit_code = 2
    end
    local parts = {
        '{"exit_code":', tostring(exit_code),
        ',"counts":{"row_missing":', tostring(counts.row_missing or 0),
        ',"row_diff":', tostring(counts.row_diff or 0),
        ',"row_present":', tostring(counts.row_present or 0),
        ',"unkeyed":', tostring(counts.unkeyed or 0),
        '},"failures":[',
    }
    for i, f in ipairs(findings or {}) do
        if i > 1 then
            parts[#parts + 1] = ","
        end
        parts[#parts + 1] = '{"id":' .. encode_string(f.id or "")
        parts[#parts + 1] = ',"class":' .. encode_string(f.class or "")
        parts[#parts + 1] = ',"object":' .. encode_string(f.object or "")
        parts[#parts + 1] = ',"column":' .. encode_string(f.column or "")
        parts[#parts + 1] = ',"check":' .. encode_string(f.check or "")
        parts[#parts + 1] = ',"status":"N"'
        parts[#parts + 1] = ',"expected":' .. encode_string(f.expected or "")
        parts[#parts + 1] = ',"live":' .. encode_string(f.live or "")
        parts[#parts + 1] = ',"notes":' .. encode_string(f.notes or "")
        if f.ref and f.ref > 0 then
            parts[#parts + 1] = ',"ref":' .. tostring(f.ref)
        end
        parts[#parts + 1] = "}"
    end
    parts[#parts + 1] = "]}"
    return table.concat(parts)
end

local function write_all(path, data)
    local f, err = io.open(path, "wb")
    if not f then
        return nil, err
    end
    f:write(data)
    f:close()
    return true
end

local function read_all(path)
    local f, err = io.open(path, "rb")
    if not f then
        return nil, err
    end
    local data = f:read("*a")
    f:close()
    return data
end

function M.live_from_probe(model, raw_by_table)
    local live = { probed = {}, tables = {} }
    for tname, body in pairs(raw_by_table or {}) do
        local st = model.tables[tname]
        if st then
            local arr = parse_json(body or "[]")
            if type(arr) ~= "table" then
                return nil, "probe JSON for " .. tname
            end
            local rows = {}
            for _, obj in ipairs(arr) do
                local parts = {}
                local values = {}
                for _, col in ipairs(st.key_cols) do
                    local v = obj[col]
                    if v == nil then
                        parts = nil
                        break
                    end
                    parts[#parts + 1] = tostring(v)
                    values[col] = tostring(v)
                end
                if parts then
                    for col, v in pairs(obj) do
                        if v == nil then
                            values[col] = nil
                        else
                            values[col] = tostring(v)
                        end
                    end
                    rows[display_key(parts)] = values
                end
            end
            live.tables[tname] = rows
            live.probed[tname] = true
        end
    end
    return live
end

local function sqlite_uri(path)
    local esc = tostring(path):gsub(" ", "%%20")
    return "file:" .. esc .. "?mode=ro"
end

function M.probe_sqlite(db_path, model)
    local raw = {}
    for _, tname in ipairs(model.table_order) do
        local st = model.tables[tname]
        local sql = st.probe_sql
        if sql and sql ~= "" then
            local cmd = "sqlite3 -json " .. sh_quote(sqlite_uri(db_path))
                .. " " .. sh_quote(sql)
            local h = io.popen(cmd)
            if not h then
                return nil, "sqlite3 popen failed"
            end
            local body = h:read("*a") or ""
            local ok = h:close()
            if ok ~= true then
                return nil, "sqlite3 probe failed for " .. tname
            end
            raw[tname] = body
        elseif #(st.probes or {}) == 0 then
            raw[tname] = "[]"
        else
            return nil, "no probe sql for " .. tname
        end
    end
    return M.live_from_probe(model, raw)
end

function M.encode_live(live)
    local parts = {'{"probed":['}
    local names = {}
    for name, _ in pairs(live.probed or {}) do
        names[#names + 1] = name
    end
    table.sort(names)
    for i, name in ipairs(names) do
        if i > 1 then
            parts[#parts + 1] = ","
        end
        parts[#parts + 1] = encode_string(name)
    end
    parts[#parts + 1] = '],"tables":{'
    for i, name in ipairs(names) do
        if i > 1 then
            parts[#parts + 1] = ","
        end
        parts[#parts + 1] = encode_string(name) .. ":["
        local rows = live.tables[name] or {}
        local keys = {}
        for key, _ in pairs(rows) do
            keys[#keys + 1] = key
        end
        table.sort(keys, key_sort)
        for r, key in ipairs(keys) do
            if r > 1 then
                parts[#parts + 1] = ","
            end
            parts[#parts + 1] = '{"key":' .. encode_string(key)
            parts[#parts + 1] = ',"values":' .. encode_map(rows[key])
            parts[#parts + 1] = "}"
        end
        parts[#parts + 1] = "]"
    end
    parts[#parts + 1] = "}}"
    return table.concat(parts)
end

function M.decode_live(text)
    local obj = parse_json(text)
    if type(obj) ~= "table" then
        return nil, "live JSON is not an object"
    end
    local live = { probed = {}, tables = {} }
    for _, name in ipairs(obj.probed or {}) do
        live.probed[name] = true
    end
    for tname, rows in pairs(obj.tables or {}) do
        live.tables[tname] = {}
        for _, row in ipairs(rows or {}) do
            local values = {}
            for col, v in pairs(row.values or {}) do
                if v ~= nil then
                    values[col] = tostring(v)
                end
            end
            live.tables[tname][tostring(row.key)] = values
        end
        live.probed[tname] = true
    end
    return live
end

local function jq_codes(path, from_ref, to_ref)
    local filter = [[
      def rows:
        if type == "array" then .[]
        elif type == "object" and has("payloads") then .
        else empty end;
      [ rows
        | . as $m
        | ($m.ref // 0) as $r
        | ($m.payloads // []) as $ps
        | ([$ps[] | select(.query_type == 1000)]) as $fwd
        | (if ($fwd | length) > 0 then $fwd
           else [$ps[] | select(.query_type == 1003)] end)[]
        | {r: $r, c: .code}
      ] | sort_by(.r) | .[]
    ]]
    local tmp = (os.getenv("TMPDIR") or "/tmp")
        .. "/schematool_rows_"
        .. tostring(os.time())
        .. "_"
        .. tostring(math.random(100000))
    os.execute('mkdir -p ' .. sh_quote(tmp))
    local fpath = tmp .. "/fold.jq"
    local ok, err = write_all(fpath, filter .. "\n")
    if not ok then
        os.execute("rm -rf " .. sh_quote(tmp))
        return nil, err
    end
    local cmd = "jq -c -f " .. sh_quote(fpath) .. " " .. sh_quote(path)
    local h = io.popen(cmd)
    if not h then
        os.execute("rm -rf " .. sh_quote(tmp))
        return nil, "jq failed"
    end
    local lines = {}
    for line in h:lines() do
        lines[#lines + 1] = line
    end
    h:close()
    os.execute("rm -rf " .. sh_quote(tmp))
    local items = {}
    for _, line in ipairs(lines) do
        local obj = parse_json(line)
        if type(obj) == "table" then
            local ref = tonumber(obj.r) or 0
            local keep = true
            if from_ref and ref < from_ref then
                keep = false
            end
            if to_ref and ref > to_ref then
                keep = false
            end
            if keep then
                items[#items + 1] = { ref = ref, code = obj.c or "" }
            end
        end
    end
    return items
end

local function load_catalog_keys(path)
    local cmd = "jq -c "
        .. sh_quote(".tables[]? | {t:.table, pk:(.primary_key // [])}")
        .. " " .. sh_quote(path)
    local h = io.popen(cmd)
    if not h then
        return nil, "jq catalog failed"
    end
    local keys = {}
    for line in h:lines() do
        local obj = parse_json(line)
        if type(obj) == "table" and obj.t then
            keys[tostring(obj.t):lower()] = { pk = obj.pk or {} }
        end
    end
    h:close()
    return keys
end

local function csv_only(text)
    if not text or text == "" then
        return nil
    end
    local only = {}
    for raw in tostring(text):gmatch("[^,]+") do
        local name = raw:gsub("^%s+", ""):gsub("%s+$", ""):lower()
        if name ~= "" then
            only[name] = true
        end
    end
    return only
end

local function arg_map(argv)
    local out = {}
    local i = 1
    while i <= #argv do
        local a = argv[i]
        if a:sub(1, 2) == "--" then
            local nxt = argv[i + 1]
            if nxt and nxt:sub(1, 2) ~= "--" then
                out[a] = nxt
                i = i + 2
            else
                out[a] = true
                i = i + 1
            end
        else
            i = i + 1
        end
    end
    return out
end

local function die(msg)
    io.stderr:write("Error: " .. tostring(msg) .. "\n")
    os.exit(1)
end

function M.main(argv)
    local opt = arg_map(argv)
    if opt["--probe-sqlite"] then
        local text, err = read_all(opt["--rows"] or "")
        if not text then
            die(err)
        end
        local model, merr = M.decode_model(text)
        if not model then
            die(merr)
        end
        local live, perr = M.probe_sqlite(opt["--probe-sqlite"], model)
        if not live then
            die(perr)
        end
        local ok, werr = write_all(opt["--live-out"], M.encode_live(live) .. "\n")
        if not ok then
            die(werr)
        end
        return 0
    end
    if opt["--compare"] then
        local text, err = read_all(opt["--rows"] or "")
        if not text then
            die(err)
        end
        local model, merr = M.decode_model(text)
        if not model then
            die(merr)
        end
        local live
        if opt["--sql-only"] or not opt["--live"] then
            live = nil
        else
            local ltext, lerr = read_all(opt["--live"])
            if not ltext then
                die(lerr)
            end
            live, lerr = M.decode_live(ltext)
            if not live then
                die(lerr)
            end
        end
        local findings, counts = M.compare(model, live, opt["--sql-only"] and true or false)
        local body = M.encode_findings(findings, counts)
        local ok, werr = write_all(opt["--findings-out"], body .. "\n")
        if not ok then
            die(werr)
        end
        io.stderr:write(string.format(
            "phase: rows missing=%d diff=%d present=%d unkeyed=%d\n",
            counts.row_missing, counts.row_diff, counts.row_present, counts.unkeyed))
        return 0
    end
    if not opt["--expected"] or not opt["--catalog"] or not opt["--out"] then
        die("need --expected, --catalog, and --out")
    end
    local from_ref = tonumber(opt["--from"] or "")
    local to_ref = tonumber(opt["--to"] or "")
    local items, ierr = jq_codes(opt["--expected"], from_ref, to_ref)
    if not items then
        die(ierr)
    end
    local ckeys, cerr = load_catalog_keys(opt["--catalog"])
    if not ckeys then
        die(cerr)
    end
    local model = M.extract(items, ckeys, {
        engine = opt["--engine"] or "sqlite",
        schema = opt["--schema"] or "",
        only = csv_only(opt["--only-tables"]),
        from_ref = from_ref,
        to_ref = to_ref,
    })
    local ok, werr = write_all(opt["--out"], M.encode_model(model) .. "\n")
    if not ok then
        die(werr)
    end
    return 0
end

if arg and arg[0] and arg[0]:match("schematool_rows%.lua$") then
    os.exit(M.main(arg) or 0)
end

return M
