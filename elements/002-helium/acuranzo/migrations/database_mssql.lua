-- database_mssql.lua
-- SQL Server (MSSQL) dialect macros for Helium migrations

-- luacheck: no max line length

-- CHANGELOG
-- 1.3.2 - 2026-09-30 - RETURNING no longer skips the INSERT...WITH move; OUTPUT before WITH is skipped
-- 1.3.1 - 2026-09-30 - Keep the newline after SUBQUERY DELIMITER when moving INSERT...WITH
-- 1.3.0 - 2026-09-30 - Statement-shape repairs (RETURNING, VALUES CTE, INSERT...WITH, ADD/DROP COLUMN)
-- 1.2.0 - 2026-09-30 - base64_decode and base64_encode use UTF-8 bytes, same as the other engines
-- 1.1.0 - 2026-09-30 - Added Phase 4 T-SQL helper function bodies: base64_decode, base64_encode, sha256_b64, brotli_decompress (CLR)
-- 1.0.0 - 2026-09-29 - Initial MSSQL dialect (Phase 2 of MSSQL.md)

-- NOTES
-- Requires: msodbcsql18 ODBC driver 18+ and unixODBC on Linux.
-- ${SCHEMA} is a dot-prefixed schema name (e.g. testms.).
-- All text types use NVARCHAR/NCCHAR for Unicode support.
-- JSON functions use native SQL Server 2016+ JSON_VALUE / OPENJSON.
-- Brotli decompression via CLR (extras/brotli_udf_mssql/ C# assembly).
-- sha256_b64 via HASHBYTES('SHA2_256') + XML base64 encoding (Phase 4).
-- base64_decode via XML bytes interpreted as UTF-8 (SQL Server 2019+).
-- ISJSON() available in SQL Server 2016+ for JSON validation.

-- Statement-shape repairs. One pass per statement, so one RETURNING does
-- not suppress a VALUES body elsewhere. An OUTPUT clause left between the
-- INSERT column list and WITH is skipped, so a second pass repairs SQL
-- already stored as INSERT ... OUTPUT ... WITH. replace_query runs this
-- before [=[ ]=] blocks are sealed into queries.code.

local function skip_ws(s, i)
    local n = #s
    while i <= n do
        local c = s:sub(i, i)
        if c ~= " " and c ~= "\t" and c ~= "\n" and c ~= "\r" then break end
        i = i + 1
    end
    return i
end

local function is_ident(c)
    return c:match("[%w_$]") ~= nil
end

-- Word match. Returns the index after the word, or nil.
-- The following character must be whitespace, semicolon, or end.
local function match_word(s, i, word)
    local n = #word
    if i + n - 1 > #s then return nil end
    if s:sub(i, i + n - 1):lower() ~= word:lower() then return nil end
    local nxt = s:sub(i + n, i + n)
    if nxt ~= "" and nxt ~= ";" and not nxt:match("%s") then return nil end
    return i + n
end

local function boundary_ok(s, i)
    if i <= 1 then return true end
    return not is_ident(s:sub(i - 1, i - 1))
end

-- Index of the ')' matching the '(' at open_i. Skips strings and line comments.
local function find_close(s, open_i)
    local depth, in_string, in_comment = 0, false, false
    local i, n = open_i, #s
    while i <= n do
        local c = s:sub(i, i)
        if in_comment then
            if c == "\n" then in_comment = false end
            i = i + 1
        elseif in_string then
            if c == "'" and s:sub(i + 1, i + 1) == "'" then
                i = i + 2
            elseif c == "'" then
                in_string = false
                i = i + 1
            else
                i = i + 1
            end
        elseif c == "-" and s:sub(i + 1, i + 1) == "-" then
            in_comment = true
            i = i + 2
        elseif c == "'" then
            in_string = true
            i = i + 1
        elseif c == "(" then
            depth = depth + 1
            i = i + 1
        elseif c == ")" then
            depth = depth - 1
            if depth == 0 then return i end
            i = i + 1
        else
            i = i + 1
        end
    end
    return nil
end

local function find_returning(s)
    local i, n = 1, #s
    while i <= n do
        local after = match_word(s, i, "RETURNING")
        if after then
            if boundary_ok(s, i) then return i end
            i = i + 1
        else
            i = i + 1
        end
    end
    return nil
end

local function find_insert_col_open(s)
    local i, n = 1, #s
    while i <= n do
        local after = match_word(s, i, "INSERT")
        if after then
            local j = skip_ws(s, after)
            local into = match_word(s, j, "INTO")
            if into then
                j = skip_ws(s, into)
                while j <= n do
                    local c = s:sub(j, j)
                    if c:match("%s") or c == "(" or c == ";" then break end
                    j = j + 1
                end
                j = skip_ws(s, j)
                if s:sub(j, j) == "(" then return j end
            end
        end
        i = i + 1
    end
    return nil
end

local function rewrite_returning(s)
    local ret_at = find_returning(s)
    if not ret_at then return nil end
    local after = match_word(s, ret_at, "RETURNING")
    local col_at = skip_ws(s, after)
    local col_end = col_at
    while col_end <= #s do
        local c = s:sub(col_end, col_end)
        if c:match("%s") or c == ";" or c == "," then break end
        col_end = col_end + 1
    end
    if col_end == col_at then return nil end
    local col = s:sub(col_at, col_end - 1)
    local open_at = find_insert_col_open(s)
    if not open_at then return nil end
    local close_at = find_close(s, open_at)
    if not close_at then return nil end
    local result = s:sub(1, close_at) .. "\nOUTPUT INSERTED." .. col .. "\n" .. s:sub(close_at + 1, ret_at - 1)
    return (result:gsub("%s+$", ""))
end

-- Index of the ')' that closes the last CTE in the WITH list starting at with_at.
-- A following comma continues the list, so INSERT ... WITH a AS (...), b AS (...)
-- moves both CTEs, not only the first.
local function cte_list_end(s, with_at)
    local after = match_word(s, with_at, "WITH")
    if not after then return nil end
    local q, n = skip_ws(s, after), #s
    local recursive = match_word(s, q, "RECURSIVE")
    if recursive then q = skip_ws(s, recursive) end
    local last_close
    while true do
        local name = q
        while q <= n and s:sub(q, q):match("[%w_%.]") do q = q + 1 end
        if q == name then return nil end
        q = skip_ws(s, q)
        if s:sub(q, q) == "(" then
            local cols_close = find_close(s, q)
            if not cols_close then return nil end
            q = skip_ws(s, cols_close + 1)
        end
        local as_at = match_word(s, q, "AS")
        if not as_at then return nil end
        q = skip_ws(s, as_at)
        if s:sub(q, q) ~= "(" then return nil end
        local body_close = find_close(s, q)
        if not body_close then return nil end
        last_close = body_close
        q = skip_ws(s, body_close + 1)
        if s:sub(q, q) ~= "," then return last_close end
        q = skip_ws(s, q + 1)
    end
end

local function rewrite_insert_with(s)
    local i, n = 1, #s
    local insert_at
    while i <= n do
        local after = match_word(s, i, "INSERT")
        if after then
            local j = skip_ws(s, after)
            if match_word(s, j, "INTO") then
                insert_at = i
                break
            end
        end
        i = i + 1
    end
    if not insert_at then return nil end
    local past = skip_ws(s, insert_at + 6)
    local into = match_word(s, past, "INTO")
    if not into then return nil end
    past = skip_ws(s, into)
    while past <= n do
        local c = s:sub(past, past)
        if c:match("%s") or c == "(" or c == ";" then break end
        past = past + 1
    end
    past = skip_ws(s, past)
    if s:sub(past, past) == "(" then
        local close_at = find_close(s, past)
        if not close_at then return nil end
        past = skip_ws(s, close_at + 1)
    end
    -- An earlier RETURNING pass leaves OUTPUT INSERTED.col between the
    -- column list and WITH. Skip that clause so the CTE can still move.
    -- SQL Server rejects INSERT ... OUTPUT ... WITH (FreeTDS native 8180).
    local output_at = match_word(s, past, "OUTPUT")
    if output_at then
        past = skip_ws(s, output_at)
        -- match_word rejects INSERTED.col because the dot is not whitespace.
        if s:sub(past, past + 7):lower() ~= "inserted" then return nil end
        local after_inserted = past + 8
        local boundary = s:sub(after_inserted, after_inserted)
        if boundary ~= "." and boundary ~= "" and not boundary:match("%s") then return nil end
        past = skip_ws(s, after_inserted)
        if s:sub(past, past) ~= "." then return nil end
        past = past + 1
        if not s:sub(past, past):match("[%a_]") then return nil end
        while past <= n and s:sub(past, past):match("[%w_]") do past = past + 1 end
        past = skip_ws(s, past)
    end
    if not match_word(s, past, "WITH") then return nil end
    local cte_close = cte_list_end(s, past)
    if not cte_close then return nil end
    -- Text before INSERT stays put. That prefix holds the newline after
    -- "-- SUBQUERY DELIMITER"; dropping it glues WITH onto the comment,
    -- so APPLY never splits the next statement (migration 1147).
    return s:sub(1, insert_at - 1) .. s:sub(past, cte_close) .. "\n" .. s:sub(insert_at, past - 1) .. s:sub(cte_close + 1)
end

local function rewrite_cte_values(s)
    if not s or s == "" then return nil end
    local i, n = 1, #s
    local cte_open, cte_close, values_at, cols
    while i <= n and not cte_open do
        local c = s:sub(i, i)
        if c == "'" then
            i = i + 1
            while i <= n do
                if s:sub(i, i) == "'" and s:sub(i + 1, i + 1) == "'" then
                    i = i + 2
                elseif s:sub(i, i) == "'" then
                    i = i + 1
                    break
                else
                    i = i + 1
                end
            end
        elseif c == "-" and s:sub(i + 1, i + 1) == "-" then
            while i <= n and s:sub(i, i) ~= "\n" do i = i + 1 end
        else
            local after = match_word(s, i, "WITH")
            local advanced = false
            if after and boundary_ok(s, i) then
                local q = skip_ws(s, after)
                local name = q
                while q <= n and s:sub(q, q):match("[%w_%.]") do q = q + 1 end
                if q ~= name then
                    q = skip_ws(s, q)
                    local col_s, col_e
                    if s:sub(q, q) == "(" then
                        local col_close = find_close(s, q)
                        if col_close then
                            col_s, col_e = q + 1, col_close
                            q = skip_ws(s, col_close + 1)
                        end
                    end
                    local as_at = col_s and match_word(s, q, "AS")
                    if as_at then
                        q = skip_ws(s, as_at)
                        if s:sub(q, q) == "(" then
                            local close_at = find_close(s, q)
                            if close_at then
                                local body = skip_ws(s, q + 1)
                                if match_word(s, body, "VALUES") then
                                    while col_s < col_e and s:sub(col_s, col_s):match("%s") do col_s = col_s + 1 end
                                    while col_e > col_s and s:sub(col_e - 1, col_e - 1):match("%s") do col_e = col_e - 1 end
                                    if col_s < col_e then
                                        cte_open, cte_close = q, close_at
                                        values_at = body
                                        cols = s:sub(col_s, col_e - 1)
                                        advanced = true
                                    end
                                end
                            end
                        end
                    end
                end
            end
            if not advanced then i = i + 1 end
        end
    end
    if not cte_open or not cols or cols == "" then return nil end
    local values_end = cte_close
    while values_end > values_at and s:sub(values_end - 1, values_end - 1):match("%s") do
        values_end = values_end - 1
    end
    return s:sub(1, cte_open) .. "SELECT * FROM (" .. s:sub(values_at, values_end - 1) .. ") AS v(" .. cols .. ")" .. s:sub(cte_close)
end

local function copy_quoted(s, i, out)
    out[#out + 1] = "'"
    i = i + 1
    local n = #s
    while i <= n do
        if s:sub(i, i) == "'" and s:sub(i + 1, i + 1) == "'" then
            out[#out + 1] = "''"
            i = i + 2
        else
            out[#out + 1] = s:sub(i, i)
            if s:sub(i, i) == "'" then return i + 1 end
            i = i + 1
        end
    end
    return i
end

local function rewrite_add_column(s)
    if not s or s == "" then return nil end
    local out, changed = {}, false
    local i, n = 1, #s
    while i <= n do
        local c = s:sub(i, i)
        if c == "'" then
            i = copy_quoted(s, i, out)
        elseif c == "-" and s:sub(i + 1, i + 1) == "-" then
            local start = i
            while i <= n and s:sub(i, i) ~= "\n" do i = i + 1 end
            out[#out + 1] = s:sub(start, i - 1)
        else
            local after = match_word(s, i, "ADD")
            local column_at = after and boundary_ok(s, i) and skip_ws(s, after)
            local col_after = column_at and match_word(s, column_at, "COLUMN")
            if col_after then
                out[#out + 1] = s:sub(i, column_at - 1)
                i = skip_ws(s, col_after)
                changed = true
            else
                out[#out + 1] = c
                i = i + 1
            end
        end
    end
    if not changed then return nil end
    return table.concat(out)
end

local function take_sql_name(s, i, allow_dot)
    local start, parts, n = i, 0, #s
    while true do
        local c = s:sub(i, i)
        if c == "[" then
            local bracket = i
            i = i + 1
            while i <= n do
                if s:sub(i, i + 1) == "]]" then
                    i = i + 2
                elseif s:sub(i, i) == "]" then
                    i = i + 1
                    break
                else
                    i = i + 1
                end
            end
            if i == bracket + 1 or s:sub(i - 1, i - 1) ~= "]" then return nil end
        elseif c:match("%a") or c == "_" then
            i = i + 1
            while i <= n and s:sub(i, i):match("[%w_@#$]") do i = i + 1 end
        else
            return nil
        end
        parts = parts + 1
        if allow_dot and parts == 1 and s:sub(i, i) == "." then
            i = i + 1
        else
            break
        end
    end
    local raw = s:sub(start, i - 1)
    if raw == "" or raw:find("'", 1, true) or raw:find(";", 1, true) or raw:find('"', 1, true) then
        return nil
    end
    return i, raw
end

local function unbracket(raw)
    if raw:sub(1, 1) ~= "[" then return raw end
    if #raw < 3 or raw:sub(-1) ~= "]" then return nil end
    local lit, i = {}, 2
    while i < #raw do
        if raw:sub(i, i + 1) == "]]" then
            lit[#lit + 1] = "]"
            i = i + 2
        else
            lit[#lit + 1] = raw:sub(i, i)
            i = i + 1
        end
    end
    if #lit == 0 then return nil end
    return table.concat(lit)
end

local function drop_batch(table_raw, column_raw)
    local column_lit = unbracket(column_raw)
    if not column_lit then return nil end
    local batch = string.format(
        "DECLARE @df sysname; DECLARE @drop nvarchar(512); " ..
        "SELECT @df = dc.name FROM sys.default_constraints AS dc " ..
        "INNER JOIN sys.columns AS c ON c.object_id = dc.parent_object_id " ..
        "AND c.column_id = dc.parent_column_id " ..
        "WHERE dc.parent_object_id = OBJECT_ID(N'%s') AND c.name = N'%s'; " ..
        "IF @df IS NOT NULL BEGIN " ..
        "SET @drop = N'ALTER TABLE %s DROP CONSTRAINT ' + QUOTENAME(@df); " ..
        "EXEC sp_executesql @drop; END " ..
        "ALTER TABLE %s DROP COLUMN %s;",
        table_raw, column_lit, table_raw, table_raw, column_raw)
    return "EXEC sp_executesql N'" .. batch:gsub("'", "''") .. "'"
end

local function rewrite_drop_column(s)
    if not s or s == "" then return nil end
    local out, changed = {}, false
    local i, n = 1, #s
    while i <= n do
        local c = s:sub(i, i)
        if c == "'" then
            i = copy_quoted(s, i, out)
        elseif c == "-" and s:sub(i + 1, i + 1) == "-" then
            local start = i
            while i <= n and s:sub(i, i) ~= "\n" do i = i + 1 end
            out[#out + 1] = s:sub(start, i - 1)
        else
            local after = match_word(s, i, "ALTER")
            local replaced = false
            if after and boundary_ok(s, i) then
                local q = skip_ws(s, after)
                local table_at = match_word(s, q, "TABLE")
                local name_end, table_raw
                if table_at then
                    name_end, table_raw = take_sql_name(s, skip_ws(s, table_at), true)
                end
                local drop_at = name_end and match_word(s, skip_ws(s, name_end), "DROP")
                local column_word = drop_at and match_word(s, skip_ws(s, drop_at), "COLUMN")
                local col_end, column_raw
                if column_word then
                    col_end, column_raw = take_sql_name(s, skip_ws(s, column_word), false)
                end
                if col_end and s:sub(skip_ws(s, col_end), skip_ws(s, col_end)) ~= "," then
                    local repl = drop_batch(table_raw, column_raw)
                    if repl then
                        out[#out + 1] = repl
                        i = col_end
                        changed = true
                        replaced = true
                    end
                end
            end
            if not replaced then
                out[#out + 1] = c
                i = i + 1
            end
        end
    end
    if not changed then return nil end
    return table.concat(out)
end

local function rewrite_statement(s)
    if not s or s == "" then return s end
    local current = s
    for _ = 1, 4 do
        local wrapped = rewrite_cte_values(current)
        if not wrapped then break end
        current = wrapped
    end
    local returning = rewrite_returning(current)
    if returning then current = returning end
    local moved = rewrite_insert_with(current)
    if moved then current = moved end
    local added = rewrite_add_column(current)
    if added then current = added end
    local dropped = rewrite_drop_column(current)
    if dropped then current = dropped end
    return current
end

local DELIM = "-- SUBQUERY DELIMITER"

local function rewrite_statement_list(s)
    local out, pos, n = {}, 1, #s
    while pos <= n + 1 do
        local a, b = s:find(DELIM, pos, true)
        local piece = a and s:sub(pos, a - 1) or s:sub(pos)
        out[#out + 1] = rewrite_statement(piece)
        if not a then break end
        -- APPLY splits on this marker plus a newline. Put the newline on
        -- the marker and consume one that already follows it, so a second
        -- pass does not insert a blank line.
        out[#out + 1] = DELIM .. "\n"
        pos = b + 1
        if s:sub(pos, pos) == "\n" then pos = pos + 1 end
    end
    return table.concat(out)
end

local function map_blocks(s, level, fn)
    local open = "[" .. string.rep("=", level) .. "["
    local close = "]" .. string.rep("=", level) .. "]"
    local out, pos, n = {}, 1, #s
    while pos <= n do
        local i = s:find(open, pos, true)
        if not i then
            out[#out + 1] = s:sub(pos)
            break
        end
        local j = s:find(close, i + #open, true)
        if not j then
            out[#out + 1] = s:sub(pos)
            break
        end
        out[#out + 1] = s:sub(pos, i + #open - 1)
        out[#out + 1] = fn(s:sub(i + #open, j - 1))
        out[#out + 1] = close
        pos = j + #close
    end
    return table.concat(out)
end

-- Placeholders hide an already-rewritten long-string body from the parent
-- statement. Nested [=[ [==[ ]==] ]=] blocks save inner tokens inside outer
-- bodies, so restore has to walk into those bodies too.
local function restore_tokens(s, saved)
    local function subst(text)
        local n = 1
        while n <= #saved do
            local token = "MSSQLLONG" .. n .. "ENDLONG"
            local at = text:find(token, 1, true)
            if not at then
                n = n + 1
            else
                text = text:sub(1, at - 1) .. subst(saved[n]) .. text:sub(at + #token)
            end
        end
        return text
    end
    return subst(s)
end

local function mask_levels(s, lo, hi, saved)
    for level = hi, lo, -1 do
        s = map_blocks(s, level, function(content)
            saved[#saved + 1] = content
            return "MSSQLLONG" .. #saved .. "ENDLONG"
        end)
    end
    return s
end

local function rewrite_migration_sql(s)
    if not s or s == "" then return s end
    local max_level = 0
    for equals in s:gmatch("%[(=+)%[") do
        if #equals > max_level and #equals <= 5 then max_level = #equals end
    end
    for level = max_level, 1, -1 do
        s = map_blocks(s, level, function(content)
            local saved = {}
            if level < max_level then
                content = mask_levels(content, level + 1, max_level, saved)
            end
            content = rewrite_statement_list(content)
            if #saved > 0 then content = restore_tokens(content, saved) end
            return content
        end)
    end
    local saved = {}
    if max_level > 0 then
        s = mask_levels(s, 1, max_level, saved)
    end
    s = rewrite_statement_list(s)
    if #saved > 0 then s = restore_tokens(s, saved) end
    return s
end

return {
    -- Types
    CHAR_2 = "NCHAR(2)",
    CHAR_20 = "NCHAR(20)",
    CHAR_50 = "NCHAR(50)",
    CHAR_128 = "NCHAR(128)",
    DATE = "DATE",
    DATETIME = "DATETIME2",
    FLOAT = "REAL",
    FLOAT_BIG = "FLOAT",
    INSERT_KEY_START = "-- ",
    INSERT_KEY_END = "",
    INSERT_KEY_RETURN = "RETURNING ",
    INTEGER = "INT",
    INTEGER_BIG = "BIGINT",
    INTEGER_SMALL = "SMALLINT",
    JRS = "JSON_VALUE(",
    JRM = ", ",
    JRE = ")",
    NOW = "SYSUTCDATETIME()",
    PRIMARY = "PRIMARY KEY",
    REORG = "-- REORG TABLE",
    SERIAL = "INT IDENTITY(1,1)",
    SESSION_SECS = "DATEDIFF(SECOND, :SESSION_START, SYSUTCDATETIME())",
    SIZE_COLLECTION = "LEN(collection)",
    SIZE_FLOAT = "4",
    SIZE_FLOAT_BIG = "8",
    SIZE_INTEGER = "4",
    SIZE_INTEGER_BIG = "8",
    SIZE_INTEGER_SMALL = "2",
    SIZE_TIMESTAMP = "8",
    TEXT = "NVARCHAR(255)",
    TEXT_BIG = "NVARCHAR(MAX)",
    TIME = "TIME",
    TIMESTAMP = "DATETIME2",
    TIMESTAMP_TZ = "DATETIMEOFFSET",
    TRFS = "DATEADD(SECOND, 0 + ",
    TRFE = ", ${NOW})",
    TRFMS = "DATEADD(MINUTE, 0 + ",
    TRFME = ", ${NOW})",
    TRMS = "DATEADD(MINUTE, 0 - ",
    TRME = ", ${NOW})",
    UNIQUE = "UNIQUE",
    VARCHAR_20 = "NVARCHAR(20)",
    VARCHAR_50 = "NVARCHAR(50)",
    VARCHAR_64 = "NVARCHAR(64)",
    VARCHAR_100 = "NVARCHAR(100)",
    VARCHAR_128 = "NVARCHAR(128)",
    VARCHAR_500 = "NVARCHAR(500)",

    -- DUMMY_TABLE: SELECT 1 needs no FROM in SQL Server
    DUMMY_TABLE = "",

    -- Password hash: testms.sha256_b64('0', 'password')
    -- Returns base64-encoded SHA256 hash
    -- Phase 4: requires CLR assembly for sha256_b64 function
    -- Usage: ${SHA256_HASH_START}'0'${SHA256_HASH_MID}'${HYDROGEN_DEMO_ADMIN_PASS}'${SHA256_HASH_END}
    SHA256_HASH_START = "${SCHEMA}sha256_b64(",
    SHA256_HASH_MID = ", ",
    SHA256_HASH_END = ")",

    -- Base64 decode via scalar function (Phase 4: CLR or built-in)
    -- Usage: ${BASE64_START}'base64data'${BASE64_END}
    BASE64_START = "${SCHEMA}base64_decode(",
    BASE64_END = ")",

    -- DB2-style extras that MSSQL does not need but must define to avoid ${UNSUBSTITUTED}
    BASE64ENCODE_START = "${SCHEMA}base64_encode(",
    BASE64ENCODE_END = ")",
    BASE64ENCODEBINARY_START = "${SCHEMA}base64_encode_binary(",
    BASE64ENCODEBINARY_END = ")",

    -- Brotli decompression: Phase 4 (CREATE ASSEMBLY + CREATE FUNCTION from extras)
    -- Requires: extras/brotli_udf_mssql/ with compiled C# assembly
    -- Phase 5: Compression disabled until CLR assembly is deployed
    BROTLI_DECOMPRESS_FUNCTION = "-- Phase 4: CREATE ASSEMBLY brotli_assembly + CREATE FUNCTION brotli_decompress (CLR)",
    COMPRESS_START = nil,
    COMPRESS_END = nil,

    -- DROP_CHECK: raise if rows exist
    DROP_CHECK = "IF EXISTS(SELECT 1 FROM ${SCHEMA}${TABLE}) THROW 51000, 'Refusing to drop table ${SCHEMA}${TABLE} – it contains data', 1;",

    -- Date/Time formatting (not used in migrations but defined for completeness)
    DATETIME_FORMAT = "CONVERT(VARCHAR(30), ${NOW}, 120)",
    TIMESTAMP_FORMAT = "CONVERT(VARCHAR(30), ${NOW}, 120)",

    JSON = "NVARCHAR(MAX)",
    JIS = "${SCHEMA}json_ingest(",
    JIE = ")",
    JSON_INGEST_START = "${SCHEMA}json_ingest(",
    JSON_INGEST_END = ")",

    -- Schema ingest: SQL Server JSON_VALUE accepts $ref/$id/$schema natively.
    -- These macros alias json_ingest; no separate function object is required.
    JSON_INGEST_SCHEMA_START = "${SCHEMA}json_ingest(",
    JSON_INGEST_SCHEMA_END = ")",
    JSON_INGEST_SCHEMA_FUNCTION = "",

    -- json_ingest T-SQL function: validates and normalizes JSON, escaping
    -- control characters inside strings. Uses ISJSON() for fast-path validation.
    -- SQL Server 2016+ supports ISJSON(); SQL Server 2022 adds ISJSON with path.
    JSON_INGEST_FUNCTION = [[
        CREATE OR ALTER FUNCTION ${SCHEMA}json_ingest(@s NVARCHAR(MAX))
        RETURNS NVARCHAR(MAX)
        AS
        BEGIN
            DECLARE @out NVARCHAR(MAX) = '';
            DECLARE @i INT = 1;
            DECLARE @L INT = LEN(@s);
            DECLARE @ch NCHAR(1);
            DECLARE @in_str BIT = 0;
            DECLARE @esc BIT = 0;

            -- fast path: already valid JSON
            IF ISJSON(@s) = 1
                RETURN @s;

            WHILE @i <= @L
            BEGIN
                SET @ch = SUBSTRING(@s, @i, 1);

                IF @esc = 1
                BEGIN
                    SET @out = @out + @ch;
                    SET @esc = 0;
                END
                ELSE IF @ch = '\'
                BEGIN
                    SET @out = @out + @ch;
                    SET @esc = 1;
                END
                ELSE IF @ch = '"'
                BEGIN
                    SET @out = @out + @ch;
                    SET @in_str = 1 - @in_str;
                END
                ELSE IF @in_str = 1 AND @ch = CHAR(10)
                BEGIN
                    SET @out = @out + '\n';
                END
                ELSE IF @in_str = 1 AND @ch = CHAR(13)
                BEGIN
                    SET @out = @out + '\r';
                END
                ELSE IF @in_str = 1 AND @ch = CHAR(9)
                BEGIN
                    SET @out = @out + '\t';
                END
                ELSE
                BEGIN
                    SET @out = @out + @ch;
                END

                SET @i = @i + 1;
            END

            -- ensure result is JSON; NULL if still invalid
            IF ISJSON(@out) = 0
                RETURN NULL;

            RETURN @out;
        END
    ]],

    -- base64_decode: XML base64 to bytes, read as UTF-8, stored as NVARCHAR.
    -- SQL Server has no native base64 decode. The XML value() method yields
    -- the raw bytes. A Windows code-page database collation would read those
    -- bytes as that code page. Inserting them into a
    -- UTF-8 column interprets the bytes as UTF-8, matching PostgreSQL
    -- CONVERT_FROM(..., 'UTF8') and MySQL utf8mb4. Invalid base64 returns
    -- the original string. A user-defined function cannot CATCH a bad UTF-8
    -- sequence; migration text is valid UTF-8.
    BASE64_DECODE_FUNCTION = [[
        CREATE OR ALTER FUNCTION ${SCHEMA}base64_decode(@s NVARCHAR(MAX))
        RETURNS NVARCHAR(MAX)
        AS
        BEGIN
            DECLARE @result NVARCHAR(MAX) = '';
            IF @s IS NULL OR LEN(@s) = 0
                RETURN @result;
            DECLARE @clean NVARCHAR(MAX) = REPLACE(REPLACE(@s, CHAR(13), ''), CHAR(10), '');
            DECLARE @bin VARBINARY(MAX) = CAST(N'<x>' + @clean + N'</x>' AS XML).value('(/x)[1]', 'VARBINARY(MAX)');
            IF @bin IS NULL
                RETURN @s;
            DECLARE @utf8 TABLE (v VARCHAR(MAX) COLLATE Latin1_General_100_CI_AS_SC_UTF8);
            INSERT INTO @utf8(v) VALUES (@bin);
            SELECT @result = v FROM @utf8;
            RETURN @result;
        END
    ]],

    -- base64_encode: UTF-8 bytes of the string, then XML base64.
    -- CAST(nvarchar AS varbinary) is UTF-16LE. The other engines encode
    -- UTF-8. Same collation sha256_b64 uses (SQL Server 2019+).
    BASE64_ENCODE_FUNCTION = [[
        CREATE OR ALTER FUNCTION ${SCHEMA}base64_encode(@s NVARCHAR(MAX))
        RETURNS NVARCHAR(MAX)
        AS
        BEGIN
            DECLARE @result NVARCHAR(MAX) = '';
            IF @s IS NULL OR LEN(@s) = 0
                RETURN @result;
            DECLARE @bin VARBINARY(MAX) = CAST(CAST(@s AS VARCHAR(MAX)) COLLATE Latin1_General_100_CI_AS_SC_UTF8 AS VARBINARY(MAX));
            SET @result = CAST(N'' AS XML).value('xs:base64Binary(sql:variable("@bin"))', 'NVARCHAR(MAX)');
            RETURN @result;
        END
    ]],

    -- base64_encode_binary: encode VARBINARY as base64 string (for brotli output).
    BASE64_ENCODE_BINARY_FUNCTION = [[
        CREATE OR ALTER FUNCTION ${SCHEMA}base64_encode_binary(@b VARBINARY(MAX))
        RETURNS NVARCHAR(MAX)
        AS
        BEGIN
            IF @b IS NULL OR DATALENGTH(@b) = 0
                RETURN '';
            RETURN CAST(N'' AS XML).value('xs:base64Binary(sql:variable("@b"))', 'NVARCHAR(MAX)');
        END
    ]],

    -- sha256_b64: SHA-256 hash of UTF-8 concatenated inputs, then base64-encoded.
    -- SQL Server's HASHBYTES('SHA2_256', ...) hashes bytes as sent.
    -- To match SQLite's base64(HASHBYTES('SHA2_256', CONCAT(N'0', N'password')))
    -- we must hash UTF-8 bytes. We cast to VARCHAR with a UTF-8 collation
    -- (Latin1_General_100_CI_AS_SC_UTF8 on SQL Server 2019+) to get UTF-8,
    -- then HASHBYTES, then base64-encode via XML.
    SHA256_B64_FUNCTION = [[
        CREATE OR ALTER FUNCTION ${SCHEMA}sha256_b64(@prefix NVARCHAR(10), @password NVARCHAR(MAX))
        RETURNS NVARCHAR(MAX)
        AS
        BEGIN
            -- Concatenate with UTF-8 encoding to match SQLite/other engines
            -- Latin1_General_100_CI_AS_SC_UTF8 is available in SQL Server 2019+
            DECLARE @combined NVARCHAR(MAX) = @prefix + ISNULL(@password, '');
            -- Cast to VARCHAR with UTF-8 collation to get UTF-8 bytes
            DECLARE @utf8 VARBINARY(MAX) = CAST(CAST(@combined AS VARCHAR(MAX)) COLLATE Latin1_General_100_CI_AS_SC_UTF8 AS VARBINARY(MAX));
            -- SHA-256 hash
            DECLARE @hash VARBINARY(32) = HASHBYTES('SHA2_256', @utf8);
            -- Base64 encode the hash
            RETURN CAST(N'' AS XML).value('xs:base64Binary(sql:variable("@hash"))', 'NVARCHAR(MAX)');
        END
    ]],

    JSON_VALUE_FUNCTION = "-- SQL Server has native JSON_VALUE; no UDR required",

    -- Timezone conversion: SQL Server uses AT TIME ZONE natively (no UDF needed)
    CONVERT_TZ_FUNCTION = "-- SQL Server uses AT TIME ZONE for timezone conversion",

    -- Called from database.lua replace_query. Not a ${macro}.
    rewrite_migration_sql = rewrite_migration_sql,
}
