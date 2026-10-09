-- schemahelper_apply_sqlutil.lua
-- String, SQL literal, and JSON-parsing helpers used by SchemaHelper
-- apply operations. No internal module dependencies.
--
-- CHANGELOG
-- 0.6.3 - 2026-10-09 - Extracted from schemahelper_apply.lua (string/JSON helpers)

local M = {}

function M.file_exists(path)
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

function M.write_all(path, data)
    local f, err = io.open(path, "wb")
    if not f then
        return nil, err
    end
    f:write(data)
    f:close()
    return true
end

function M.sh_quote(s)
    return "'" .. tostring(s):gsub("'", "'\\''") .. "'"
end

function M.utf8_from_code(code)
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

function M.json_string_field(obj, key)
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
                parts[#parts + 1] = M.utf8_from_code(code)
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

function M.json_has_string(obj, key)
    obj = tostring(obj or "")
    local pat = '"' .. key .. '"%s*:%s*"'
    return obj:find(pat) ~= nil
end

function M.dollar_quote(body)
    body = tostring(body or "")
    local tag = "schematool"
    local n = 0
    while body:find("%$" .. tag .. "%$", 1, true) do
        n = n + 1
        tag = "schematool" .. tostring(n)
    end
    return "$" .. tag .. "$" .. body .. "$" .. tag .. "$"
end

function M.sql_string_literal(body)
    return "'" .. tostring(body or ""):gsub("'", "''") .. "'"
end

function M.backtick_name(schema, table_name)
    if schema:match("^[%w_]+$") and tostring(table_name):match("^[%w_]+$") then
        return "`" .. schema .. "`.`" .. table_name .. "`"
    end
    return schema .. "." .. table_name
end

function M.safe_ident(name)
    name = tostring(name or "")
    if name:match("^[%w_]+$") then
        return name
    end
    return nil
end

function M.safe_type(dtype)
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

M.APPLY_FIELDS = {
    code = true,
    name = true,
    summary = true,
    row = true,
}

M.ROW_FIELDS = { "code", "name", "summary" }

return M
