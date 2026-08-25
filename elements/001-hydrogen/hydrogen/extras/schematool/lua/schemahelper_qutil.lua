-- schemahelper_qutil.lua
-- Pure text / JSON helpers shared across SchemaHelper queue modules.
-- No internal module dependencies; safe to require first.
--
-- CHANGELOG
-- 0.5.8 - 2026-08-25 - Extracted from schemahelper_queue.lua (text/json helpers)

local M = {}

function M.json_escape(s)
    s = tostring(s or "")
    s = s:gsub("\\", "\\\\")
    s = s:gsub('"', '\\"')
    s = s:gsub("\n", "\\n")
    s = s:gsub("\r", "\\r")
    s = s:gsub("\t", "\\t")
    return s
end

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

function M.json_string_field(obj, key)
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

function M.json_subobj(obj, subobj_key)
    local pat = '"' .. subobj_key .. '"%s*:%s*%{'
    local s, e = obj:find(pat)
    if not s then
        local arr_pat = '"' .. subobj_key .. '"%s*:%s*%['
        s, e = obj:find(arr_pat)
        if not s then
            return ""
        end
    end
    local i2 = e + 1
    local depth = 1
    local result = {}
    local in_string = false
    local escape = false
    while depth > 0 and i2 <= #obj do
        local c = obj:sub(i2, i2)
        if in_string then
            result[#result + 1] = c
            if escape then
                escape = false
            elseif c == "\\" then
                escape = true
            elseif c == '"' then
                in_string = false
            end
        else
            if c == '"' then
                in_string = true
                result[#result + 1] = c
            elseif c == "{" or c == "[" then
                depth = depth + 1
                result[#result + 1] = c
            elseif c == "}" or c == "]" then
                depth = depth - 1
                result[#result + 1] = c
            elseif c == "," or c == ":" or (c:match("%s")) then
                result[#result + 1] = c
            else
                result[#result + 1] = c
            end
        end
        i2 = i2 + 1
    end
    return table.concat(result)
end

function M.payload_text(obj)
    local txt = M.json_string_field(obj, "detail")
    if txt == "" then
        txt = M.json_string_field(obj, "notes")
    end
    return txt
end

function M.json_num_field(obj, key)
    local n = obj:match('"' .. key .. '"%s*:%s*(%-?%d+)')
    return n and tonumber(n) or nil
end

function M.jq_lines(filter, path, tmp_dir)
    if not M.file_exists(path) then
        return {}
    end
    local fpath = tmp_dir .. "/q.jq"
    M.write_all(fpath, filter .. "\n")
    local cmd = string.format(
        'jq -c -f "%s" "%s" 2>/dev/null',
        fpath:gsub('"', '\\"'),
        path:gsub('"', '\\"')
    )
    local h = io.popen(cmd)
    if not h then
        return {}
    end
    local lines = {}
    for line in h:lines() do
        if line and line ~= "" and line ~= "null" then
            lines[#lines + 1] = line
        end
    end
    h:close()
    return lines
end

function M.jq_num(filter, path, tmp_dir)
    local lines = M.jq_lines(filter, path, tmp_dir)
    if #lines == 0 then
        return 0
    end
    return tonumber(lines[1]) or 0
end

function M.listed_fields(obj)
    local arr = obj:match('"fields"%s*:%s*%[(.-)%]')
    local fields = {}
    local seen = {}
    if arr then
        for name in arr:gmatch('"([^"]+)"') do
            if name ~= "" and not seen[name] then
                seen[name] = true
                fields[#fields + 1] = name
            end
        end
    end
    return fields
end

function M.payload_raw(obj, name)
    local s = M.json_string_field(obj, name)
    if s ~= "" then
        return s
    end
    local n = M.json_num_field(obj, name)
    if n ~= nil then
        return tostring(n)
    end
    return ""
end

function M.payload_field_differs(expected, actual, name)
    return M.payload_raw(expected, name) ~= M.payload_raw(actual, name)
end

function M.has_embed(s)
    s = tostring(s or "")
    return s:find("BROTLI_DECOMPRESS", 1, true)
        or s:find("CRYPTO_DECODE", 1, true)
        or s:find("FROM_BASE64", 1, true)
        or s:find("BASE64DECODE", 1, true)
        or s:find("brotli_decompress", 1, true)
        or s:find("CONVERT_FROM", 1, true)
end

function M.drift_field_specs(obj, expected, actual)
    local listed = M.listed_fields(obj)
    local seen = {}
    local names = {}
    local function consider(name)
        if not name or name == "" or seen[name] then
            return
        end
        seen[name] = true
        names[#names + 1] = name
    end
    for i = 1, #listed do
        consider(listed[i])
    end
    local extras = { "name", "summary", "code" }
    for i = 1, #extras do
        if M.payload_field_differs(expected, actual, extras[i]) then
            consider(extras[i])
        end
    end
    if #names == 0 then
        names[1] = "code"
    end
    local specs = {}
    for i = 1, #names do
        local name = names[i]
        local ev = M.payload_raw(expected, name)
        local av = M.payload_raw(actual, name)
        if ev ~= av then
            specs[#specs + 1] = { field = name, view = "raw" }
        end
    end
    if #specs == 0 then
        specs[1] = { field = names[1], view = "raw" }
    end
    return specs
end

function M.clip_text(s, n)
    s = tostring(s or ""):gsub("\n", "\\n"):gsub("\r", "\\r")
    if #s > n then
        return s:sub(1, n) .. "…"
    end
    return s
end

function M.first_diff_at(a, b)
    a = a or ""
    b = b or ""
    local n = math.min(#a, #b)
    for i = 1, n do
        if a:byte(i) ~= b:byte(i) then
            return i
        end
    end
    if #a ~= #b then
        return n + 1
    end
    return nil
end

function M.pad_clip(s, w)
    s = tostring(s or "")
    if w < 1 then
        return ""
    end
    if #s > w then
        return s:sub(1, w)
    end
    return s .. string.rep(" ", w - #s)
end

function M.wrap_hard(s, w)
    local out = {}
    s = tostring(s or "")
    if w < 8 then
        w = 8
    end
    if s == "" then
        out[1] = ""
        return out
    end
    while #s > w do
        out[#out + 1] = s:sub(1, w)
        s = s:sub(w + 1)
    end
    out[#out + 1] = s
    return out
end

function M.split_lines(text)
    local lines = {}
    text = text or ""
    if text == "" then
        return lines
    end
    for line in (text .. "\n"):gmatch("(.-)\n") do
        lines[#lines + 1] = line
    end
    return lines
end

return M
