-- schemahelper_apply.lua
-- Per-finding apply: whole-row or one-field metadata UPDATE, confirmed
-- orphan DELETE, or one catalog DDL statement in the engine's dialect.
--
-- CHANGELOG
-- 0.6.1 - 2026-10-07 - One default row: INSERT, UPDATE, or DELETE; token table.key
-- 0.6.0 - 2026-10-07 - Dialect DDL, whole-row metadata, UTF-8 JSON literals
-- 0.5.9 - 2026-10-07 - Type and dropped catalog findings stay review-only
-- 0.5.8 - 2026-10-07 - MariaDB qualifies with the same backticks as MySQL
-- 0.5.7 - 2026-10-07 - Dollar-quote literals no longer treat cockroachdb as postgresql
-- 0.5.6 - 2026-10-07 - MSSQL bracket qualify and N'' field literals
-- 0.5.5 - 2026-08-24 - Phase 7: catalog DDL apply (nullable / add column), louder confirm (object.column)
-- 0.5.4 - 2026-08-24 - Phase 5 slice: confirmed orphan DELETE (true orphans only)
-- 0.5.0 - 2026-08-23 - Phase 5: per-field UPDATE, confirm REF.field

local M = {}

local function file_exists(path)
    if not path or path == "" then
        return false
    end
    local f = io.open(path, "r")
    if not f then
        return false
    end
    f:close()
    return true
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

local function sh_quote(s)
    return "'" .. tostring(s):gsub("'", "'\\''") .. "'"
end

local APPLY_FIELDS = {
    code = true,
    name = true,
    summary = true,
    row = true,
}

local ROW_FIELDS = { "code", "name", "summary" }

local function utf8_from_code(code)
    if type(code) ~= "number" then
        return "?"
    end
    if code < 0 or code > 0x10FFFF then
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

local function json_string_field(obj, key)
    obj = tostring(obj or "")
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
                i2 = i2 + 2
            elseif n == "r" then
                parts[#parts + 1] = "\r"
                i2 = i2 + 2
            elseif n == "t" then
                parts[#parts + 1] = "\t"
                i2 = i2 + 2
            elseif n == '"' then
                parts[#parts + 1] = '"'
                i2 = i2 + 2
            elseif n == "\\" then
                parts[#parts + 1] = "\\"
                i2 = i2 + 2
            elseif n == "/" then
                parts[#parts + 1] = "/"
                i2 = i2 + 2
            elseif n == "u" then
                local hex = obj:sub(i2 + 2, i2 + 5)
                local code = tonumber(hex, 16)
                i2 = i2 + 6
                if code and code >= 0xD800 and code <= 0xDBFF
                    and obj:sub(i2, i2 + 1) == "\\u" then
                    local low = tonumber(obj:sub(i2 + 2, i2 + 5), 16)
                    if low and low >= 0xDC00 and low <= 0xDFFF then
                        code = 0x10000
                            + ((code - 0xD800) * 0x400)
                            + (low - 0xDC00)
                        i2 = i2 + 6
                    end
                end
                parts[#parts + 1] = utf8_from_code(code)
            else
                parts[#parts + 1] = n
                i2 = i2 + 2
            end
        elseif c == '"' then
            break
        else
            parts[#parts + 1] = c
            i2 = i2 + 1
        end
    end
    return table.concat(parts)
end

local function json_has_string(obj, key)
    obj = tostring(obj or "")
    local pat = '"' .. key .. '"%s*:%s*"'
    return obj:find(pat) ~= nil
end

local function dollar_quote(body)
    body = tostring(body or "")
    local tag = "schematool"
    local n = 0
    while body:find("%$" .. tag .. "%$", 1, true) do
        n = n + 1
        tag = "schematool" .. tostring(n)
    end
    return "$" .. tag .. "$" .. body .. "$" .. tag .. "$"
end

local function sql_string_literal(body)
    return "'" .. tostring(body or ""):gsub("'", "''") .. "'"
end

local function backtick_name(schema, table_name)
    if schema:match("^[%w_]+$") and tostring(table_name):match("^[%w_]+$") then
        return "`" .. schema .. "`.`" .. table_name .. "`"
    end
    return schema .. "." .. table_name
end

local function safe_ident(name)
    name = tostring(name or "")
    if name:match("^[%w_]+$") then
        return name
    end
    return nil
end

local function safe_type(dtype)
    dtype = tostring(dtype or ""):gsub("^%s+", ""):gsub("%s+$", "")
    if dtype == "" then
        return nil
    end
    if dtype:find("[;'\"]") or dtype:find("%-%-", 1, false) then
        return nil
    end
    if not dtype:match("^[%w_ (),.]+$") then
        return nil
    end
    return dtype
end

local function is_pg(engine)
    return engine == "postgresql" or engine == "yugabytedb"
end

local function is_mysql(engine)
    return engine == "mysql" or engine == "mariadb"
end

local function supported_engine(engine)
    return is_pg(engine) or is_mysql(engine) or engine == "sqlite"
        or engine == "db2" or engine == "firebird" or engine == "mssql"
end

local function want_null(finding)
    local e = tostring(finding and finding.expected or "")
    return e == "true" or e == "YES" or e == "1"
end

local function column_is_table(column)
    return column == nil or column == "" or column == "-"
end

function M.qualify_queries(engine, schema)
    if not schema or schema == "" or schema == "." or engine == "sqlite" then
        return "queries"
    end
    if engine == "db2" then
        return string.upper(schema) .. ".QUERIES"
    end
    if engine == "mssql" and schema:match("^[%w_]+$") then
        return "[" .. schema .. "].[queries]"
    end
    if engine == "mysql" or engine == "mariadb" then
        return backtick_name(schema, "queries")
    end
    return schema .. ".queries"
end

function M.qualify_table(engine, schema, table_name)
    if not schema or schema == "" or schema == "." or engine == "sqlite" then
        return table_name
    end
    if engine == "db2" then
        return string.upper(schema) .. "." .. string.upper(table_name)
    end
    if engine == "mssql"
        and schema:match("^[%w_]+$")
        and tostring(table_name):match("^[%w_]+$") then
        return "[" .. schema .. "].[" .. table_name .. "]"
    end
    if engine == "mysql" or engine == "mariadb" then
        return backtick_name(schema, table_name)
    end
    return schema .. "." .. table_name
end

local COL_FILTER = [[
.tables[]? | select(.table == $t) | .columns[]? | select(.name == $c)
| [.name, (.data_type // ""), (if .nullable == false then "0" else "1" end)]
| @tsv
]]

local TABLE_COLS = [[
.tables[]? | select(.table == $t) | .columns[]?
| [.name, (.data_type // ""), (if .nullable == false then "0" else "1" end)]
| @tsv
]]

local TABLE_PK = [[.tables[]? | select(.table == $t) | .primary_key[]?]]

local TABLE_HIT = [[.tables[]? | select(.table == $t) | .table]]

local function jq_capture(out_dir, filter, args)
    local exp_path = (out_dir or "") .. "/catalog_expected.json"
    if not file_exists(exp_path) then
        return nil, "catalog_expected.json not found"
    end
    local tmp = (os.getenv("TMPDIR") or "/tmp")
        .. "/schemahelper_apply_"
        .. tostring(os.time())
        .. "_"
        .. tostring(math.random(100000))
    os.execute('mkdir -p "' .. tmp .. '"')
    local fpath = tmp .. "/q.jq"
    write_all(fpath, filter .. "\n")
    local parts = { "jq", "-r" }
    for i = 1, #args do
        parts[#parts + 1] = "--arg"
        parts[#parts + 1] = args[i][1]
        parts[#parts + 1] = sh_quote(args[i][2])
    end
    parts[#parts + 1] = "-f"
    parts[#parts + 1] = sh_quote(fpath)
    parts[#parts + 1] = sh_quote(exp_path)
    parts[#parts + 1] = "2>/dev/null"
    local h = io.popen(table.concat(parts, " "))
    if not h then
        os.execute('rm -rf "' .. tmp .. '"')
        return nil, "jq failed"
    end
    local result = h:read("*a") or ""
    h:close()
    os.execute('rm -rf "' .. tmp .. '"')
    return result
end

local function nonempty_lines(text)
    local rows = {}
    for line in tostring(text or ""):gmatch("[^\n]+") do
        if line ~= "" and line ~= "null" then
            rows[#rows + 1] = line
        end
    end
    return rows
end

local function lookup_column(out_dir, table_name, column)
    local text, err = jq_capture(out_dir, COL_FILTER, {
        { "t", table_name },
        { "c", column },
    })
    if not text then
        return nil, err
    end
    local line = nonempty_lines(text)[1]
    if not line then
        return nil, "cannot determine column type for "
            .. table_name .. "." .. column
    end
    local name, dtype, flag = line:match("^([^\t]*)\t([^\t]*)\t([^\t]*)$")
    if not name then
        return nil, "cannot determine column type for "
            .. table_name .. "." .. column
    end
    return {
        name = name,
        data_type = dtype,
        nullable = flag ~= "0",
    }
end

local function lookup_table(out_dir, table_name)
    local hit, hit_err = jq_capture(out_dir, TABLE_HIT, {
        { "t", table_name },
    })
    if not hit then
        return nil, nil, hit_err
    end
    if not nonempty_lines(hit)[1] then
        return nil, nil, "table not in the fold"
    end
    local cols_text, cols_err = jq_capture(out_dir, TABLE_COLS, {
        { "t", table_name },
    })
    if not cols_text then
        return nil, nil, cols_err
    end
    local cols = {}
    for _, line in ipairs(nonempty_lines(cols_text)) do
        local name, dtype, flag = line:match("^([^\t]*)\t([^\t]*)\t([^\t]*)$")
        if name then
            cols[#cols + 1] = {
                name = name,
                data_type = dtype,
                nullable = flag ~= "0",
            }
        end
    end
    if #cols == 0 then
        return nil, nil, "no columns in the fold"
    end
    local pk_text, pk_err = jq_capture(out_dir, TABLE_PK, {
        { "t", table_name },
    })
    if not pk_text then
        return nil, nil, pk_err
    end
    return cols, nonempty_lines(pk_text)
end

local function is_default_kind(kind)
    return kind == "row_missing" or kind == "row_diff"
        or kind == "row_present"
end

local function csv_names(text)
    local list = {}
    local set = {}
    for part in tostring(text or ""):gmatch("[^,]+") do
        local name = part:gsub("^%s+", ""):gsub("%s+$", "")
        if name ~= "" then
            list[#list + 1] = name
            set[name] = true
        end
    end
    return list, set
end

local function decode_flat(text)
    text = tostring(text or "")
    local i = 1
    local n = #text
    local function skip()
        while i <= n and text:sub(i, i):match("%s") do
            i = i + 1
        end
    end
    local function read_string()
        if text:sub(i, i) ~= '"' then
            return nil
        end
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
    skip()
    if text:sub(i, i) ~= "{" then
        return nil, "expected row is not an object"
    end
    i = i + 1
    local map = {}
    skip()
    if text:sub(i, i) == "}" then
        return map
    end
    while i <= n do
        skip()
        local key = read_string()
        if not key then
            return nil, "expected row key is not a string"
        end
        skip()
        if text:sub(i, i) ~= ":" then
            return nil, "expected row is missing a colon"
        end
        i = i + 1
        skip()
        local spec
        local c = text:sub(i, i)
        if c == '"' then
            local value = read_string()
            if value == nil then
                return nil, "expected row value is not a string"
            end
            spec = { kind = "str", text = value }
        elseif text:sub(i, i + 3) == "null" then
            i = i + 4
            spec = { kind = "null" }
        else
            local num = text:match("^(%-?%d+%.?%d*)", i)
            if not num then
                return nil, "expected row value is not literal"
            end
            i = i + #num
            spec = { kind = "num", text = num }
        end
        map[key] = spec
        skip()
        c = text:sub(i, i)
        if c == "," then
            i = i + 1
        elseif c == "}" then
            return map
        else
            return nil, "expected row object did not end"
        end
    end
    return nil, "expected row object did not end"
end

local function row_literal(engine, col, spec, nums, nulls)
    if nulls[col] then
        return "NULL"
    end
    if nums[col] or (spec and spec.kind == "num") then
        local text = spec and spec.text or ""
        if not text:match("^%-?%d+%.?%d*$") then
            return nil, "unsafe number"
        end
        return text
    end
    if not spec then
        return nil, "missing value for " .. col
    end
    if spec.kind == "null" then
        return "NULL"
    end
    return M.field_literal(engine, spec.text or "")
end

local function build_row_sql(finding, engine, schema)
    local kind = finding.kind or ""
    local qualified, name_err = guard_names(engine, schema, finding.object or "")
    if not qualified then
        return nil, name_err
    end
    local map, err = decode_flat(finding.expected or "")
    if not map then
        return nil, err
    end
    local key_list = csv_names(map.__keys and map.__keys.text or "")
    local order = csv_names(map.__order and map.__order.text or "")
    local _, nums = csv_names(map.__nums and map.__nums.text or "")
    local _, nulls = csv_names(map.__nulls and map.__nulls.text or "")
    if #key_list == 0 then
        return nil, "default row has no key"
    end
    local function lit(col)
        if not safe_ident(col) then
            return nil, "unsafe column"
        end
        return row_literal(engine, col, map[col], nums, nulls)
    end
    local function where_sql()
        local preds = {}
        for _, col in ipairs(key_list) do
            local value, verr = lit(col)
            if not value then
                return nil, verr
            end
            preds[#preds + 1] = col .. " = " .. value
        end
        return table.concat(preds, " AND ")
    end
    if kind == "row_present" then
        local where, werr = where_sql()
        if not where then
            return nil, werr
        end
        return "DELETE FROM " .. qualified .. " WHERE " .. where .. ";"
    end
    if kind == "row_diff" then
        local keyset = {}
        for _, col in ipairs(key_list) do
            keyset[col] = true
        end
        local sets = {}
        for _, col in ipairs(order) do
            if not keyset[col] then
                local value, verr = lit(col)
                if not value then
                    return nil, verr
                end
                sets[#sets + 1] = col .. " = " .. value
            end
        end
        if #sets == 0 then
            return nil, "no migration-owned column to update"
        end
        local where, werr = where_sql()
        if not where then
            return nil, werr
        end
        return "UPDATE " .. qualified .. " SET "
            .. table.concat(sets, ", ") .. " WHERE " .. where .. ";"
    end
    local cols = {}
    local values = {}
    local seen = {}
    for _, col in ipairs(order) do
        if not seen[col] then
            seen[col] = true
            local value, verr = lit(col)
            if not value then
                return nil, verr
            end
            cols[#cols + 1] = col
            values[#values + 1] = value
        end
    end
    if #cols == 0 then
        return nil, "default row has no columns"
    end
    return "INSERT INTO " .. qualified .. " ("
        .. table.concat(cols, ", ") .. ") VALUES ("
        .. table.concat(values, ", ") .. ");"
end

function M.confirm_token(finding)
    if not finding then
        return nil
    end
    if finding.kind == "orphan" then
        if not finding.ref then
            return nil
        end
        return tostring(finding.ref)
    end
    if is_default_kind(finding.kind) then
        if not finding.object or finding.object == "" then
            return nil
        end
        if not finding.column or finding.column == ""
            or finding.column == "-" then
            return nil
        end
        return finding.object .. "." .. finding.column
    end
    local class = finding.class or ""
    if class:find("^catalog") then
        if finding.kind == "dropped" and finding.object then
            if not column_is_table(finding.column) then
                return "DROP " .. finding.object .. "." .. finding.column
            end
            return "DROP " .. finding.object
        end
        if finding.object then
            if not column_is_table(finding.column) then
                return finding.object .. "." .. finding.column
            end
            return finding.object
        end
        return nil
    end
    if not finding.ref or not finding.field then
        return nil
    end
    if finding.field == "row" then
        return tostring(finding.ref)
    end
    return string.format("%d.%s", finding.ref, finding.field)
end

local function sqlite_rebuild_reason(kind, column)
    if kind == "nullable" then
        return "SQLite nullability needs a table rebuild"
    end
    if kind == "type" then
        return "SQLite type change needs a table rebuild"
    end
    if kind == "dropped" and not column_is_table(column) then
        return "SQLite DROP COLUMN needs a table rebuild"
    end
    return nil
end

function M.refuse_reason(finding, allow_write, engine)
    if not allow_write then
        return "need --allow-write"
    end
    if not finding then
        return "no finding"
    end
    if finding.view == "decoded" then
        return "decoded view — apply the encoded field"
    end
    local kind = finding.kind or ""
    local class = finding.class or ""
    if kind == "missing_load" or kind == "missing_apply" or kind == "unkeyed" then
        return "run Hydrogen AutoMigration"
    end
    if is_default_kind(kind) then
        return nil
    end
    if kind == "anomaly" or class:find("anomaly", 1, true) then
        return "anomaly — do not auto-delete"
    end
    if class:find("^catalog") or kind:find("^cat") then
        if kind == "extra_table" or kind == "extra_column"
            or class == "catalog live extra" then
            return "live extra — not applicable"
        end
        if engine == "sqlite" then
            local why = sqlite_rebuild_reason(kind, finding.column)
            if why then
                return why
            end
        end
        if kind == "nullable" or kind == "column" or kind == "type"
            or kind == "dropped" or kind == "table" then
            return nil
        end
        return "catalog / no live DDL"
    end
    if kind == "orphan" then
        if type(finding.ref) ~= "number" or finding.ref < 1 then
            return "missing ref"
        end
        return nil
    end
    local field = finding.field
    if not field or not APPLY_FIELDS[field] then
        return "not a metadata field"
    end
    if class ~= "metadata content drift" then
        return "not a metadata field UPDATE"
    end
    if type(finding.ref) ~= "number" or finding.ref < 1 then
        return "missing ref"
    end
    if type(finding.db_type) ~= "number" then
        return "missing query type"
    end
    return nil
end

function M.can_apply(finding, allow_write)
    return M.refuse_reason(finding, allow_write) == nil
end

function M.field_literal(engine, value)
    if engine == "postgresql" or engine == "yugabytedb" then
        return dollar_quote(value)
    end
    if engine == "mssql" then
        return "N" .. sql_string_literal(value)
    end
    return sql_string_literal(value)
end

local function guard_names(engine, schema, table_name)
    if not supported_engine(engine) then
        return nil, "unsupported engine"
    end
    if schema and schema ~= "" and schema ~= "." and not safe_ident(schema) then
        return nil, "unsafe schema"
    end
    if not safe_ident(table_name) then
        return nil, "unsafe table"
    end
    return M.qualify_table(engine, schema, table_name)
end

local function column_type_text(out_dir, table_name, column, prefer)
    local typ = safe_type(prefer)
    if typ then
        return typ
    end
    local col, err = lookup_column(out_dir, table_name, column)
    if not col then
        return nil, err
    end
    typ = safe_type(col.data_type)
    if not typ then
        return nil, "unsafe column type"
    end
    return typ
end

local function fold_null_word(out_dir, table_name, column)
    local col, err = lookup_column(out_dir, table_name, column)
    if not col then
        return nil, err
    end
    if col.nullable then
        return "NULL"
    end
    return "NOT NULL"
end

local function add_column_sql(engine, qualified, column, out_dir, table_name)
    local typ, err = column_type_text(out_dir, table_name, column, nil)
    if not typ then
        return nil, err
    end
    if engine == "firebird" or engine == "mssql" then
        return string.format(
            "ALTER TABLE %s ADD %s %s;", qualified, column, typ)
    end
    return string.format(
        "ALTER TABLE %s ADD COLUMN %s %s;", qualified, column, typ)
end

local function nullability_sql(engine, qualified, column, finding, out_dir, table_name)
    local action
    if want_null(finding) then
        action = "DROP NOT NULL"
    else
        action = "SET NOT NULL"
    end
    if is_pg(engine) or engine == "db2" then
        return string.format(
            "ALTER TABLE %s ALTER COLUMN %s %s;",
            qualified, column, action)
    end
    if engine == "firebird" then
        return string.format(
            "ALTER TABLE %s ALTER %s %s;",
            qualified, column, action)
    end
    local typ, err = column_type_text(out_dir, table_name, column, nil)
    if not typ then
        return nil, err
    end
    local verb
    if want_null(finding) then
        verb = "NULL"
    else
        verb = "NOT NULL"
    end
    if is_mysql(engine) then
        return string.format(
            "ALTER TABLE %s MODIFY COLUMN %s %s %s;",
            qualified, column, typ, verb)
    end
    if engine == "mssql" then
        return string.format(
            "ALTER TABLE %s ALTER COLUMN %s %s %s;",
            qualified, column, typ, verb)
    end
    return nil, "unsupported engine"
end

local function type_sql(engine, qualified, column, finding, out_dir, table_name)
    local typ, err = column_type_text(
        out_dir, table_name, column, finding.expected)
    if not typ then
        return nil, err
    end
    if is_pg(engine) then
        return string.format(
            "ALTER TABLE %s ALTER COLUMN %s TYPE %s;",
            qualified, column, typ)
    end
    if engine == "db2" then
        return string.format(
            "ALTER TABLE %s ALTER COLUMN %s SET DATA TYPE %s;",
            qualified, column, typ)
    end
    if engine == "firebird" then
        return string.format(
            "ALTER TABLE %s ALTER %s TYPE %s;",
            qualified, column, typ)
    end
    local verb, verb_err = fold_null_word(out_dir, table_name, column)
    if not verb then
        return nil, verb_err
    end
    if is_mysql(engine) then
        return string.format(
            "ALTER TABLE %s MODIFY COLUMN %s %s %s;",
            qualified, column, typ, verb)
    end
    if engine == "mssql" then
        return string.format(
            "ALTER TABLE %s ALTER COLUMN %s %s %s;",
            qualified, column, typ, verb)
    end
    return nil, "unsupported engine"
end

local function mssql_drop_column(schema, table_name, column, qualified)
    local lines = {
        "DECLARE @schemahelper_dc sysname;",
        "SELECT @schemahelper_dc = dc.name",
        "  FROM sys.default_constraints AS dc",
        "  INNER JOIN sys.columns AS c",
        "    ON c.object_id = dc.parent_object_id",
        "   AND c.column_id = dc.parent_column_id",
        "  INNER JOIN sys.tables AS t ON t.object_id = c.object_id",
        "  INNER JOIN sys.schemas AS s ON s.schema_id = t.schema_id",
        " WHERE t.name = N'" .. table_name .. "'",
    }
    if schema and schema ~= "" and schema ~= "." then
        lines[#lines + 1] = "   AND s.name = N'" .. schema .. "'"
    end
    lines[#lines + 1] = "   AND c.name = N'" .. column .. "';"
    lines[#lines + 1] = "IF @schemahelper_dc IS NOT NULL"
    lines[#lines + 1] = "    EXEC(N'ALTER TABLE " .. qualified
        .. " DROP CONSTRAINT ['"
        .. " + REPLACE(@schemahelper_dc, N']', N']]') + N']');"
    lines[#lines + 1] = "ALTER TABLE " .. qualified
        .. " DROP COLUMN " .. column .. ";"
    return table.concat(lines, "\n")
end

local function drop_column_sql(engine, schema, table_name, column, qualified)
    if engine == "firebird" then
        return string.format("ALTER TABLE %s DROP %s;", qualified, column)
    end
    if engine == "mssql" then
        return mssql_drop_column(schema, table_name, column, qualified)
    end
    return string.format(
        "ALTER TABLE %s DROP COLUMN %s;", qualified, column)
end

local function create_table_sql(engine, qualified, out_dir, table_name)
    local cols, pk, err = lookup_table(out_dir, table_name)
    if not cols then
        return nil, err
    end
    local lines = {}
    for i = 1, #cols do
        local col = cols[i]
        local ident = safe_ident(col.name)
        local typ = safe_type(col.data_type)
        if not ident or not typ then
            return nil, "unsafe column in " .. table_name
        end
        local nullsql = ""
        if not col.nullable then
            nullsql = " NOT NULL"
        end
        lines[#lines + 1] = "    " .. ident .. " " .. typ .. nullsql
    end
    if pk and #pk > 0 then
        local pk_ok = {}
        for i = 1, #pk do
            local ident = safe_ident(pk[i])
            if not ident then
                return nil, "unsafe primary key"
            end
            pk_ok[#pk_ok + 1] = ident
        end
        lines[#lines + 1] = "    PRIMARY KEY ("
            .. table.concat(pk_ok, ", ") .. ")"
    end
    return "CREATE TABLE " .. qualified .. " (\n"
        .. table.concat(lines, ",\n") .. "\n);"
end

function M.build_catalog_sql(finding, conn, out_dir)
    local engine = conn and conn.engine or ""
    if engine == "" then
        return nil, "unresolved engine"
    end
    local why = M.refuse_reason(finding, true, engine)
    if why then
        return nil, why
    end
    local schema = conn and conn.schema or ""
    local table_name = finding.object or ""
    local qualified, name_err = guard_names(engine, schema, table_name)
    if not qualified then
        return nil, name_err
    end
    local kind = finding.kind or ""
    local column = finding.column or ""
    if kind == "table" then
        return create_table_sql(engine, qualified, out_dir, table_name)
    end
    if kind == "dropped" and column_is_table(column) then
        return string.format("DROP TABLE %s;", qualified)
    end
    if not safe_ident(column) then
        return nil, "unsafe column"
    end
    if kind == "column" then
        return add_column_sql(engine, qualified, column, out_dir, table_name)
    end
    if kind == "nullable" then
        return nullability_sql(
            engine, qualified, column, finding, out_dir, table_name)
    end
    if kind == "type" then
        return type_sql(
            engine, qualified, column, finding, out_dir, table_name)
    end
    if kind == "dropped" then
        return drop_column_sql(engine, schema, table_name, column, qualified)
    end
    return nil, "unsupported catalog check: " .. tostring(kind)
end

local function metadata_assign(engine, finding)
    if finding.field == "row" then
        local sets = {}
        for i = 1, #ROW_FIELDS do
            local name = ROW_FIELDS[i]
            if json_has_string(finding.expected, name) then
                local value = json_string_field(finding.expected, name)
                sets[#sets + 1] = name .. " = "
                    .. M.field_literal(engine, value)
            end
        end
        if #sets == 0 then
            return nil, "expected row has no code, name, or summary"
        end
        return "SET " .. table.concat(sets, ",\n       ")
    end
    local value = json_string_field(finding.expected, finding.field)
    return "SET " .. finding.field .. " = " .. M.field_literal(engine, value)
end

function M.build_sql(finding, conn, out_dir)
    conn = conn or {}
    local why = M.refuse_reason(finding, true, conn.engine)
    if why then
        return nil, why
    end
    local engine = conn.engine or ""
    if engine == "" then
        return nil, "unresolved engine"
    end
    local kind = finding.kind or ""
    if is_default_kind(kind) then
        return build_row_sql(finding, engine, conn.schema)
    end
    local class = finding.class or ""
    if class:find("^catalog") then
        return M.build_catalog_sql(finding, conn, out_dir)
    end
    local qtable = M.qualify_queries(engine, conn.schema)
    if finding.kind == "orphan" then
        if type(finding.ref) ~= "number" or finding.ref < 1 then
            return nil, "missing ref"
        end
        return string.format(
            "DELETE FROM %s\n WHERE query_ref = %d\n   AND query_type_a28 BETWEEN 1000 AND 1003;",
            qtable, finding.ref)
    end
    local assign, assign_err = metadata_assign(engine, finding)
    if not assign then
        return nil, assign_err
    end
    return string.format(
        "UPDATE %s\n   %s\n WHERE query_ref = %d\n   AND query_type_a28 = %d;",
        qtable, assign, finding.ref, finding.db_type)
end

function M.write_log(out_dir, finding, sql)
    if not out_dir or out_dir == "" then
        return nil, "no out-dir"
    end
    local stamp = os.date("!%Y%m%dT%H%M%SZ")
    local name
    local header
    local caveat
    local class = finding.class or ""
    if finding.kind == "orphan" then
        name = string.format("schemahelper_apply_%d_delete_%s.sql",
            finding.ref, stamp)
        header = "-- SchemaHelper apply (orphan DELETE; not executed by SchemaTool)"
        caveat = "-- Deleting orphan rows does NOT author a migration."
    elseif is_default_kind(finding.kind) then
        local obj = (finding.object or "row"):gsub("[^%w_]", "_")
        local col = (finding.column or "key"):gsub("[^%w_]", "_")
        name = string.format("schemahelper_apply_row_%s_%s_%s.sql",
            obj, col, stamp)
        header = "-- SchemaHelper apply (one default row; not executed by SchemaTool)"
        caveat = "-- This statement changes only the named key."
    elseif class:find("^catalog") then
        local obj = finding.object or "unknown"
        local col = finding.column or "-"
        col = col:gsub("[^%w_]", "_")
        obj = obj:gsub("[^%w_]", "_")
        name = string.format("schemahelper_apply_ddl_%s_%s_%s.sql",
            obj, col, stamp)
        header = "-- SchemaHelper apply (catalog DDL; not executed by SchemaTool)"
        if finding.kind == "dropped" then
            caveat = "-- This DROP removes an object the fold dropped."
        elseif finding.kind == "table" then
            caveat = "-- This CREATE adds a table from the fold."
        else
            caveat = "-- This statement mutates live DDL shape."
        end
    else
        name = string.format("schemahelper_apply_%d_%s_%s.sql",
            finding.ref, finding.field, stamp)
        header = "-- SchemaHelper apply (metadata only; not executed by SchemaTool)"
        caveat = "-- Updating queries metadata does NOT replay DDL."
    end
    local lines = {
        header,
        "-- finding " .. tostring(finding.id or ""),
        "-- confirm " .. tostring(M.confirm_token(finding) or ""),
        caveat,
        sql,
        "",
    }
    local path = out_dir .. "/" .. name
    local f, err = io.open(path, "wb")
    if not f then
        return nil, err
    end
    f:write(table.concat(lines, "\n"))
    f:close()
    return path
end

return M
