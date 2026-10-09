-- schematool_rows_parse.lua
-- SQL statement classifier for the schematool_rows model builder.
--
-- Splits a migration code blob into statements, then classifies each
-- INSERT/UPDATE/DELETE/CREATE statement into a keyed row mutation or an
-- unkeyed/default marker. CREATE TABLE notes are folded into a key table
-- so INSERT/UPDATE/DELETE against the same table know its primary key and
-- unique keys. Statements that do not bind a single key row are reported
-- as unkeyed (the caller stays refused and AutoMigration owns them).
--
-- Usage: parsed by schematool_rows.lua / schematool_rows_model.lua only.
--
-- CHANGELOG
-- 1.0.0 - 2026-10-09 - Split from schematool_rows.lua
--
-- luacheck: globals arg package

local M = {}

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

M.skip_ws = skip_ws
M.keyword_at = keyword_at
M.read_ident = read_ident
M.read_table = read_table
M.read_sq = read_sq
M.read_dollar = read_dollar
M.skip_expr = skip_expr
M.read_value = read_value
M.read_tuple = read_tuple
M.read_columns = read_columns
M.parse_where = parse_where

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

M.parse_insert = parse_insert
M.parse_update = parse_update
M.parse_delete = parse_delete

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

M.split_statements = split_statements

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

M.split_code = split_code

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

M.paren_list = paren_list

function M.ensure_key(keys, tname)
    tname = tname:lower()
    if not keys[tname] then
        keys[tname] = { pk = {}, unique = {} }
    end
    return keys[tname]
end

function M.add_unique(info, cols)
    if not cols or #cols == 0 then
        return
    end
    info.unique[#info.unique + 1] = cols
end

function M.note_create(stmt, keys)
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
    local info = M.ensure_key(keys, tname)
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
                M.add_unique(info, paren_list(trimmed))
            end
        elseif up:match("^UNIQUE%s*%(") or up:match("^UNIQUE%s+") then
            M.add_unique(info, paren_list(trimmed))
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
                        M.add_unique(info, { cname:lower() })
                    end
                end
            end
        end
    end
    if #info.pk == 0 and #pk > 0 then
        info.pk = pk
    end
end

function M.classify(stmt, keys)
    local i = skip_ws(stmt, 1)
    if keyword_at(stmt, i, "CREATE") then
        M.note_create(stmt, keys)
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

return M
