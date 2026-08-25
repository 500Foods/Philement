-- schemahelper_wrappers.lua
-- SchemaTool wrapper discovery + metadata, plus path/sh quoting helpers.
--
-- CHANGELOG
-- 0.5.8 - 2026-08-25 - Extracted from schemahelper.lua (wrapper cluster)

local connect = require("schemahelper_connect")
local C = require("schemahelper_const")

local WRAPPER_ORDER = C.WRAPPER_ORDER
local WRAPPER_BLURB = C.WRAPPER_BLURB

local function read_tool_version(path)
    local f = io.open(path, "r")
    if not f then
        return nil, nil
    end
    local ver
    local date
    for line in f:lines() do
        local v = line:match('^VERSION="([^"]+)"')
        if v then
            ver = v
        end
        local dv, dd = line:match("^#%s+(%d+%.%d+%.%d+)%s+%-%s+(%d%d%d%d%-%d%d%-%d%d)%s+")
        if dv and (not ver or dv == ver) and not date then
            date = dd
        end
        if ver and date then
            break
        end
    end
    f:close()
    return ver, date
end

local function wrapper_engine(path)
    local base = path:match("([^/]+)$") or path
    return base:match("^schematool_(.+)%.sh$") or ""
end

local function wrapper_meta(path)
    local engine = wrapper_engine(path)
    local flags = connect.parse_wrapper(path)
    local design = flags.design or "acuranzo"
    local schema = flags.schema or ""
    if schema == "" then
        local resolved = connect.resolve(path)
        schema = resolved.schema or ""
    end
    return design, engine, schema
end

local function wrapper_dir(path)
    return path:match("^(.*)/[^/]+$") or "."
end

local function discover_wrappers(dir)
    local found = {}
    local cmd = 'ls -1 "' .. dir:gsub('"', '\\"') .. '"/schematool_*.sh 2>/dev/null'
    local h = io.popen(cmd)
    if h then
        for line in h:lines() do
            if line ~= "" then
                local eng = wrapper_engine(line)
                if eng ~= "" then
                    found[eng] = line
                end
            end
        end
        h:close()
    end
    local list = {}
    local seen = {}
    for _, eng in ipairs(WRAPPER_ORDER) do
        if found[eng] then
            list[#list + 1] = { engine = eng, path = found[eng] }
            seen[eng] = true
        end
    end
    local extras = {}
    for eng, path in pairs(found) do
        if not seen[eng] then
            extras[#extras + 1] = { engine = eng, path = path }
        end
    end
    table.sort(extras, function(a, b)
        return a.engine < b.engine
    end)
    for _, item in ipairs(extras) do
        list[#list + 1] = item
    end
    return list
end

local function wrapper_label(item)
    local base = item.path:match("([^/]+)$") or item.path
    local blurb = WRAPPER_BLURB[item.engine] or item.engine
    return string.format("%-28s %s", base, blurb)
end

local function sh_quote(s)
    return "'" .. tostring(s):gsub("'", "'\\''") .. "'"
end

local function ensure_dir(path)
    if path and path ~= "" then
        os.execute("mkdir -p " .. sh_quote(path))
    end
end

local function count_disk_refs(migrations, design)
    if not migrations or migrations == "" then
        return 0
    end
    local design_pat = (design or "acuranzo"):gsub("(%W)", "%%%1")
    local patterns = {
        "^" .. design_pat .. "_(%d+)%.lua$",
        "^design_(%d+)%.lua$",
    }
    local n = 0
    local h = io.popen("ls -1 " .. sh_quote(migrations) .. " 2>/dev/null")
    if not h then
        return 0
    end
    for name in h:lines() do
        for i = 1, #patterns do
            if name:match(patterns[i]) then
                n = n + 1
                break
            end
        end
    end
    h:close()
    return n
end

return {
    read_tool_version = read_tool_version,
    wrapper_engine = wrapper_engine,
    wrapper_meta = wrapper_meta,
    wrapper_dir = wrapper_dir,
    discover_wrappers = discover_wrappers,
    wrapper_label = wrapper_label,
    sh_quote = sh_quote,
    ensure_dir = ensure_dir,
    count_disk_refs = count_disk_refs,
}
