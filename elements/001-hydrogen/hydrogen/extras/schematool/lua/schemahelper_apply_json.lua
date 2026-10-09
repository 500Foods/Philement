-- schemahelper_apply_json.lua
-- Flat JSON row decoder and default-row SQL builder (INSERT/UPDATE/DELETE)
-- for SchemaHelper apply. Depends on schemahelper_apply_sqlutil and
-- schemahelper_apply_engine.
--
-- CHANGELOG
-- 0.6.3 - 2026-10-09 - Extracted from schemahelper_apply.lua (row SQL helpers)

local U = require("schemahelper_apply_sqlutil")
local E = require("schemahelper_apply_engine")

local M = {}

function M.csv_names(text)
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

function M.decode_flat(text)
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
                    parts[#parts + 1] = U.utf8_from_code(code)
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

function M.row_literal(engine, col, spec, nums, nulls)
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

function M.field_literal(engine, value)
    if engine == "postgresql" or engine == "yugabytedb" then
        return U.dollar_quote(value)
    end
    if engine == "mssql" then
        return "N" .. U.sql_string_literal(value)
    end
    return U.sql_string_literal(value)
end

function M.build_row_sql(finding, engine, schema)
    local kind = finding.kind or ""
    local qualified, name_err = E.guard_names(engine, schema, finding.object or "")
    if not qualified then
        return nil, name_err
    end
    local map, err = M.decode_flat(finding.expected or "")
    if not map then
        return nil, err
    end
    local key_list = M.csv_names(map.__keys and map.__keys.text or "")
    local order = M.csv_names(map.__order and map.__order.text or "")
    local _, nums = M.csv_names(map.__nums and map.__nums.text or "")
    local _, nulls = M.csv_names(map.__nulls and map.__nulls.text or "")
    if #key_list == 0 then
        return nil, "default row has no key"
    end
    local function lit(col)
        if not U.safe_ident(col) then
            return nil, "unsafe column"
        end
        return M.row_literal(engine, col, map[col], nums, nulls)
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

return M
