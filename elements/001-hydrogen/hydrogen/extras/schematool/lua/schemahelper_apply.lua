-- schemahelper_apply.lua
-- One-field metadata UPDATE from official Lua. No catalog DDL, no DELETE.
--
-- CHANGELOG
-- 0.5.0 - 2026-08-23 - Phase 5: per-field UPDATE, confirm REF.field

local M = {}

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

function M.confirm_token(finding)
    if not finding or not finding.ref or not finding.field then
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
    local field = finding.field
    if not field or not APPLY_FIELDS[field] then
        return "not a metadata field"
    end
    if (finding.class or "") ~= "metadata content drift" then
        local kind = finding.kind or ""
        if kind == "missing_load" or kind == "missing_apply" then
            return "run Hydrogen AutoMigration"
        end
        if kind == "orphan" then
            return "orphan DELETE not in this slice"
        end
        if kind == "anomaly" or (finding.class or ""):find("anomaly", 1, true) then
            return "anomaly — do not auto-delete"
        end
        if (finding.class or ""):find("^catalog") or kind:find("^cat") then
            return "catalog / no live DDL"
        end
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

function M.build_sql(finding, conn)
    local why = M.refuse_reason(finding, true)
    if why then
        return nil, why
    end
    conn = conn or {}
    local engine = conn.engine or ""
    if engine == "" then
        return nil, "unresolved engine"
    end
    local value = json_string_field(finding.expected, finding.field)
    local qtable = M.qualify_queries(engine, conn.schema)
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
    local name = string.format(
        "schemahelper_apply_%d_%s_%s.sql",
        finding.ref, finding.field, stamp)
    local path = out_dir .. "/" .. name
    local lines = {
        "-- SchemaHelper apply (metadata only; not executed by SchemaTool)",
        "-- finding " .. tostring(finding.id or ""),
        "-- confirm " .. tostring(M.confirm_token(finding) or ""),
        "-- Updating queries metadata does NOT replay DDL.",
        sql,
        "",
    }
    local f, err = io.open(path, "wb")
    if not f then
        return nil, err
    end
    f:write(table.concat(lines, "\n"))
    f:close()
    return path
end

return M
