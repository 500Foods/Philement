-- schemahelper_apply.lua
-- Per-finding apply to database: one-field metadata UPDATE, confirmed
-- orphan DELETE, or single-statement catalog DDL (nullable / add column).
--
-- CHANGELOG
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
}

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
                else
                    parts[#parts + 1] = "?"
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

function M.qualify_queries(engine, schema)
    if not schema or schema == "" or schema == "." or engine == "sqlite" then
        return "queries"
    end
    if engine == "db2" then
        return string.upper(schema) .. ".QUERIES"
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
    return schema .. "." .. table_name
end

local function lookup_column_type(out_dir, table_name, column)
    local exp_path = (out_dir or "") .. "/catalog_expected.json"
    if not file_exists(exp_path) then
        return ""
    end
    local tmp = (os.getenv("TMPDIR") or "/tmp")
        .. "/schemahelper_apply_"
        .. tostring(os.time())
        .. "_"
        .. tostring(math.random(100000))
    os.execute('mkdir -p "' .. tmp .. '"')
    local filter = '.tables[]? | select(.table == $t) | .columns[]? | '
        .. 'select(.name == $c) | .data_type // empty'
    local fpath = tmp .. "/type.jq"
    write_all(fpath, filter .. "\n")
    local cmd = string.format(
        'jq -r --arg t %s --arg c %s -f %s %s 2>/dev/null',
        sh_quote(table_name), sh_quote(column), sh_quote(fpath), sh_quote(exp_path))
    local h = io.popen(cmd)
    if not h then
        os.execute('rm -rf "' .. tmp .. '"')
        return ""
    end
    local result = h:read("*l") or ""
    h:close()
    os.execute('rm -rf "' .. tmp .. '"')
    result = result:gsub("%s+$", "")
    if result == "null" or result == "" then
        return ""
    end
    return result
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
    local class = finding.class or ""
    if class:find("^catalog") then
        if finding.object then
            if finding.column and finding.column ~= ""
                and finding.column ~= "-" then
                return finding.object .. "." .. finding.column
            end
            return finding.object
        end
        return nil
    end
    if not finding.ref or not finding.field then
        return nil
    end
    return string.format("%d.%s", finding.ref, finding.field)
end

function M.refuse_reason(finding, allow_write)
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
    if kind == "missing_load" or kind == "missing_apply" then
        return "run Hydrogen AutoMigration"
    end
    if kind == "anomaly" or class:find("anomaly", 1, true) then
        return "anomaly — do not auto-delete"
    end
    if class:find("^catalog") or kind:find("^cat") then
        if kind == "nullable" or kind == "column" then
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
    if engine == "postgresql" or engine == "cockroachdb"
        or engine == "yugabytedb" then
        return dollar_quote(value)
    end
    return sql_string_literal(value)
end

function M.build_catalog_sql(finding, conn, out_dir)
    local engine = conn and conn.engine or ""
    if engine == "" then
        return nil, "unresolved engine"
    end
    local schema = conn and conn.schema or ""
    local table_name = finding.object or ""
    if table_name == "" then
        return nil, "missing table object"
    end
    local qualified = M.qualify_table(engine, schema, table_name)
    local column = finding.column or ""
    if column == "" or column == "-" then
        return nil, "missing column"
    end
    if finding.kind == "nullable" then
        local want_null = (finding.expected == "true"
            or finding.expected == "YES" or finding.expected == "1")
        local action
        if want_null then
            action = "DROP NOT NULL"
        else
            action = "SET NOT NULL"
        end
        return string.format(
            "ALTER TABLE %s ALTER COLUMN %s %s;",
            qualified, column, action)
    end
    if finding.kind == "column" then
        local col_type = lookup_column_type(out_dir, table_name, column)
        if col_type == "" then
            return nil, "cannot determine column type for "
                .. table_name .. "." .. column
        end
        return string.format(
            "ALTER TABLE %s ADD COLUMN %s %s;",
            qualified, column, col_type)
    end
    return nil, "unsupported catalog check: " .. tostring(finding.kind)
end

function M.build_sql(finding, conn, out_dir)
    local why = M.refuse_reason(finding, true)
    if why then
        return nil, why
    end
    conn = conn or {}
    local engine = conn.engine or ""
    if engine == "" then
        return nil, "unresolved engine"
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
    local value = json_string_field(finding.expected, finding.field)
    local sql = string.format(
        "UPDATE %s\n   SET %s = %s\n WHERE query_ref = %d\n   AND query_type_a28 = %d;",
        qtable,
        finding.field,
        M.field_literal(engine, value),
        finding.ref,
        finding.db_type
    )
    return sql
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
    elseif class:find("^catalog") then
        local obj = finding.object or "unknown"
        local col = finding.column or "-"
        col = col:gsub("[^%w_]", "_")
        obj = obj:gsub("[^%w_]", "_")
        name = string.format("schemahelper_apply_ddl_%s_%s_%s.sql",
            obj, col, stamp)
        header = "-- SchemaHelper apply (catalog DDL; not executed by SchemaTool)"
        caveat = "-- This ALTER statement mutates live DDL shape."
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
