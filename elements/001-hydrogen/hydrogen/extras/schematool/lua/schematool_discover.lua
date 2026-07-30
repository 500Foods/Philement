-- schematool_discover.lua
-- Discover design_NNNN.lua migrations and emit checklist data JSON (disk-only).
-- Usage:
--   lua schematool_discover.lua <migrations_dir> <design> [from_ref] [to_ref]
--
-- CHANGELOG
-- 1.0.0 - 2026-07-29 - Phase 1 discovery for SchemaTool

-- luacheck: globals arg

local migrations_dir = arg[1]
local design = arg[2]
local from_ref = tonumber(arg[3] or "")
local to_ref = tonumber(arg[4] or "")

if not migrations_dir or migrations_dir == "" or not design or design == "" then
    io.stderr:write("Usage: lua schematool_discover.lua <migrations_dir> <design> [from] [to]\n")
    os.exit(1)
end

local function is_dir(path)
    local ok, _, code = os.rename(path, path)
    if ok then
        return true
    end
    -- permission denied still means path exists on some systems
    return code == 13
end

if not is_dir(migrations_dir) then
    -- fallback: try opening a known file pattern via io.popen ls
    local test = io.open(migrations_dir, "r")
    if not test then
        io.stderr:write("Error: migrations directory not readable: " .. migrations_dir .. "\n")
        os.exit(1)
    end
    test:close()
end

-- Escape magic chars in design for Lua patterns (do not use string.format — % confuses it)
local design_pat = design:gsub("(%W)", "%%%1")
local pattern = "^" .. design_pat .. "_(%d+)%.lua$"
local entries = {}

local handle = io.popen('ls -1 "' .. migrations_dir:gsub('"', '\\"') .. '" 2>/dev/null')
if not handle then
    io.stderr:write("Error: failed to list migrations directory\n")
    os.exit(1)
end

for name in handle:lines() do
    local num = name:match(pattern)
    if num then
        local ref = tonumber(num)
        if ref then
            local include = true
            if from_ref and ref < from_ref then
                include = false
            end
            if to_ref and ref > to_ref then
                include = false
            end
            if include then
                table.insert(entries, { ref = ref, file = name })
            end
        end
    end
end
handle:close()

table.sort(entries, function(a, b)
    return a.ref < b.ref
end)

-- Emit JSON array manually (no cjson dependency)
local parts = { "[" }
for i, e in ipairs(entries) do
    local comma = (i < #entries) and "," or ""
    table.insert(parts, string.format(
        '{"ref":%d,"file":%q,"load":"-","load_match":"-","apply":"-","apply_match":"-","notes":"disk only"}%s',
        e.ref, e.file, comma
    ))
end
table.insert(parts, "]")
io.write(table.concat(parts, ""))
io.write("\n")
