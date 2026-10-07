-- schemahelper_wrappers.lua
-- SchemaTool wrapper discovery + metadata, plus path/sh quoting helpers.
--
-- CHANGELOG
-- 0.6.5 - 2026-10-07 - Disk ref count follows a design plus-list
-- 0.6.4 - 2026-10-07 - Discover follows the sixteen-stem WRAPPER_ORDER
-- 0.6.3 - 2026-10-07 - Picker rows key by path; test role is its own row
-- 0.5.8 - 2026-08-25 - Extracted from schemahelper.lua (wrapper cluster)

local connect = require("schemahelper_connect")
local C = require("schemahelper_const")
local payload = require("schematool_payload")

local WRAPPER_ORDER = C.WRAPPER_ORDER

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

local function wrapper_engine_role(stem)
    local base = stem:match("^(.*)_test$")
    if base and base ~= "" then
        return base, "test"
    end
    base = stem:match("^(.*)_demo$")
    if base and base ~= "" then
        return base, "demo"
    end
    return stem, "demo"
end

local function wrapper_meta(path)
    local stem = wrapper_engine(path)
    local engine, role = wrapper_engine_role(stem)
    local flags = connect.parse_wrapper(path)
    local design = flags.design or "acuranzo"
    local schema = flags.schema or ""
    if schema == "" then
        local resolved = connect.resolve(path)
        schema = resolved.schema or ""
    end
    return design, engine, schema, role
end

local function wrapper_dir(path)
    return path:match("^(.*)/[^/]+$") or "."
end

local function discover_wrappers(dir)
    local by_stem = {}
    local cmd = 'ls -1 "' .. dir:gsub('"', '\\"') .. '"/schematool_*.sh 2>/dev/null'
    local h = io.popen(cmd)
    if h then
        for line in h:lines() do
            if line ~= "" then
                local stem = wrapper_engine(line)
                if stem ~= "" and not by_stem[stem] then
                    by_stem[stem] = line
                end
            end
        end
        h:close()
    end
    local list = {}
    local seen_path = {}
    local function add(stem)
        local path = by_stem[stem]
        if path and not seen_path[path] then
            seen_path[path] = true
            local engine, role = wrapper_engine_role(stem)
            list[#list + 1] = {
                engine = engine,
                role = role,
                path = path,
                stem = stem,
            }
        end
    end
    for _, stem in ipairs(WRAPPER_ORDER) do
        add(stem)
    end
    local extras = {}
    for stem, path in pairs(by_stem) do
        if not seen_path[path] then
            extras[#extras + 1] = stem
        end
    end
    table.sort(extras)
    for _, stem in ipairs(extras) do
        add(stem)
    end
    return list
end

local function wrapper_label(item)
    local base = item.path:match("([^/]+)$") or item.path
    local engine = item.engine
    local role = item.role
    if not role or role == "" then
        engine, role = wrapper_engine_role(engine or "")
    end
    local blurb = connect.picker_blurb(engine, role)
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
    local locs = payload.locations(migrations, design or "acuranzo")
    if not locs then
        return 0
    end
    local generic = #locs == 1
    local n = 0
    for _, loc in ipairs(locs) do
        if payload.is_dir(loc.dir) then
            local entries = payload.list_entries(loc.dir, loc.name, nil, nil, generic)
            if entries then
                n = n + #entries
            end
        end
    end
    return n
end

return {
    read_tool_version = read_tool_version,
    wrapper_engine = wrapper_engine,
    wrapper_engine_role = wrapper_engine_role,
    wrapper_meta = wrapper_meta,
    wrapper_dir = wrapper_dir,
    discover_wrappers = discover_wrappers,
    wrapper_label = wrapper_label,
    sh_quote = sh_quote,
    ensure_dir = ensure_dir,
    count_disk_refs = count_disk_refs,
}
