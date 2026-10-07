-- schematool_discover.lua
-- Discover design_NNNN.lua migrations and emit checklist data JSON (disk-only).
-- Usage:
--   lua schematool_discover.lua <migrations_dir> <design> [from_ref] [to_ref]
--
-- <design> is one name or a plus-list (acuranzo+argent). The migrations
-- directory is the first design. Later designs are sibling folders.
--
-- CHANGELOG
-- 1.1.0 - 2026-10-07 - Plus-list payload; one combined ref-ordered list
-- 1.0.0 - 2026-07-29 - Phase 1 discovery for SchemaTool

-- luacheck: globals arg

local function script_dir()
    local src = arg[0] or ""
    local dir = src:match("^(.*)/[^/]+$")
    return dir or "."
end

package.path = script_dir() .. "/?.lua;" .. package.path

local payload = require("schematool_payload")

local migrations_dir = arg[1]
local design = arg[2]
local from_ref = tonumber(arg[3] or "")
local to_ref = tonumber(arg[4] or "")

if not migrations_dir or migrations_dir == "" or not design or design == "" then
    io.stderr:write(
        "Usage: lua schematool_discover.lua <migrations_dir> <design> [from] [to]\n"
    )
    os.exit(1)
end

local function fail(msg)
    io.stderr:write("Error: " .. msg .. "\n")
    os.exit(1)
end

local function has_database(dir)
    local f = io.open(dir .. "/database.lua", "r")
    if not f then
        return false
    end
    f:close()
    return true
end

local locs, loc_err = payload.locations(migrations_dir, design)
if not locs then
    fail(loc_err)
end

local generic = #locs == 1
local entries = {}
local seen_ref = {}

for _, loc in ipairs(locs) do
    if not payload.is_dir(loc.dir) then
        fail("migrations directory for design '" .. loc.name .. "' not found: " .. loc.dir)
    end
    if not generic and not has_database(loc.dir) then
        fail("database.lua not found for design '" .. loc.name .. "' in " .. loc.dir)
    end
    local present, present_err = payload.list_entries(loc.dir, loc.name, nil, nil, generic)
    if not present then
        fail(present_err)
    end
    if #present == 0 then
        fail("no migrations matched " .. loc.name .. "_NNNN.lua in " .. loc.dir)
    end
    local ranged, range_err = payload.list_entries(
        loc.dir, loc.name, from_ref, to_ref, generic
    )
    if not ranged then
        fail(range_err)
    end
    for _, row in ipairs(ranged) do
        if seen_ref[row.ref] then
            fail("migration ref " .. tostring(row.ref)
                .. " is in both " .. seen_ref[row.ref] .. " and " .. row.design)
        end
        seen_ref[row.ref] = row.design
        entries[#entries + 1] = row
    end
end

table.sort(entries, function(a, b)
    return a.ref < b.ref
end)

local parts = { "[" }
for i, e in ipairs(entries) do
    local comma = (i < #entries) and "," or ""
    local row = string.format(
        '{"ref":%d,"file":%q,"design":%q,',
        e.ref, e.file, e.design
    )
    row = row .. '"load":"-","load_match":"-","apply":"-","apply_match":"-","notes":"disk only"}'
    table.insert(parts, row .. comma)
end
table.insert(parts, "]")
io.write(table.concat(parts, ""))
io.write("\n")
