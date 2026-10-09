-- schematool_rows_json.lua
-- JSON helpers shared by the schematool_rows* modules.
--
-- Provides minimal escaping and a hand-rolled JSON parser/encoder pair so
-- the rows modules do not depend on a host JSON library. The parser is a
-- small recursive-descent reader used for model files, live probe output,
-- and jq line folding.
--
-- CHANGELOG
-- 1.0.0 - 2026-10-09 - Split from schematool_rows.lua
--
-- luacheck: globals arg package

local M = {}

function M.utf8_from_code(code)
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

function M.json_escape(s)
    s = tostring(s or "")
    s = s:gsub("\\", "\\\\")
    s = s:gsub('"', '\\"')
    s = s:gsub("\n", "\\n")
    s = s:gsub("\r", "\\r")
    s = s:gsub("\t", "\\t")
    return s
end

function M.sh_quote(s)
    return "'" .. tostring(s):gsub("'", "'\\''") .. "'"
end

function M.encode_string(s)
    return '"' .. M.json_escape(s) .. '"'
end

function M.encode_value(v)
    local tv = type(v)
    if tv == "string" then
        return M.encode_string(v)
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
    return M.encode_string(tostring(v))
end

function M.encode_array(arr)
    local parts = {}
    for i = 1, #arr do
        parts[#parts + 1] = M.encode_value(arr[i])
    end
    return "[" .. table.concat(parts, ",") .. "]"
end

function M.encode_map(map)
    local keys = {}
    for k, _ in pairs(map) do
        keys[#keys + 1] = k
    end
    table.sort(keys)
    local parts = {}
    for _, k in ipairs(keys) do
        parts[#parts + 1] = M.encode_string(k) .. ":" .. M.encode_value(map[k])
    end
    return "{" .. table.concat(parts, ",") .. "}"
end

function M.encode_row(row)
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

function M.write_all(path, data)
    local f, err = io.open(path, "wb")
    if not f then
        return nil, err
    end
    f:write(data)
    f:close()
    return true
end

function M.read_all(path)
    local f, err = io.open(path, "rb")
    if not f then
        return nil, err
    end
    local data = f:read("*a")
    f:close()
    return data
end

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
                    parts[#parts + 1] = M.utf8_from_code(code)
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

M.parse_json = parse_json

return M
