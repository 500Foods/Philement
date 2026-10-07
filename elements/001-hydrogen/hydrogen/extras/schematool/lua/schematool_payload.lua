-- schematool_payload.lua
-- Plus-list design resolution for SchemaTool discovery and expect.
--
-- --design acuranzo+argent is one payload. The anchor directory is the
-- first design's migrations folder. Later designs are siblings:
--   dirname(dirname(anchor))/<design>/migrations
-- A single design name stays the anchor only.
--
-- CHANGELOG
-- 1.0.0 - 2026-10-07 - Payload plus-list: anchor plus sibling migrations

local M = {}

function M.is_dir(path)
    if not path or path == "" then
        return false
    end
    local ok, _, code = os.rename(path, path)
    if ok or code == 13 then
        return true
    end
    return false
end

-- Split "acuranzo+argent" into design names. Names are [%a_][%w_]*.
function M.split(spec)
    if type(spec) ~= "string" or spec == "" then
        return nil, "design name is empty"
    end
    local names = {}
    local seen = {}
    local start_at = 1
    while true do
        local plus = spec:find("+", start_at, true)
        local piece
        if plus then
            piece = spec:sub(start_at, plus - 1)
        else
            piece = spec:sub(start_at)
        end
        if not piece:match("^[%a_][%w_]*$") then
            return nil, "design name must be letters, digits, and underscores: '"
                .. piece .. "'"
        end
        if seen[piece] then
            return nil, "design '" .. piece .. "' is repeated in " .. spec
        end
        seen[piece] = true
        names[#names + 1] = piece
        if not plus then
            break
        end
        start_at = plus + 1
    end
    return names
end

local function parent_dir(path)
    local trimmed = (path or ""):gsub("/+$", "")
    return trimmed:match("^(.*)/[^/]+$")
end

-- First design uses anchor. Later designs are sibling migrations folders.
function M.locations(anchor, spec)
    local names, err = M.split(spec)
    if not names then
        return nil, err
    end
    local out = {}
    for i, name in ipairs(names) do
        local dir = anchor
        if i > 1 then
            local design_root = parent_dir(anchor)
            local helium = design_root and parent_dir(design_root)
            if not helium then
                return nil, "cannot resolve sibling design '" .. name
                    .. "' from " .. tostring(anchor)
            end
            dir = helium .. "/" .. name .. "/migrations"
        end
        out[#out + 1] = { name = name, dir = dir }
    end
    return out
end

local function quote_path(path)
    return '"' .. tostring(path):gsub('"', '\\"') .. '"'
end

-- List design_NNNN.lua (and, for a single design, literal design_NNNN.lua).
-- from_ref / to_ref are inclusive. Either may be nil.
function M.list_entries(dir, name, from_ref, to_ref, generic_fallback)
    local name_pat = "^" .. name:gsub("(%W)", "%%%1") .. "_(%d+)%.lua$"
    local entries = {}
    local seen = {}
    local handle = io.popen("ls -1 " .. quote_path(dir) .. " 2>/dev/null")
    if not handle then
        return nil, "failed to list " .. tostring(dir)
    end
    for fname in handle:lines() do
        local num = fname:match(name_pat)
        if not num and generic_fallback then
            num = fname:match("^design_(%d+)%.lua$")
        end
        if num then
            local ref = tonumber(num)
            local include = ref ~= nil and not seen[ref]
            if include and from_ref and ref < from_ref then
                include = false
            end
            if include and to_ref and ref > to_ref then
                include = false
            end
            if include then
                seen[ref] = true
                entries[#entries + 1] = {
                    ref = ref,
                    file = fname,
                    design = name,
                    dir = dir,
                }
            end
        end
    end
    handle:close()
    table.sort(entries, function(a, b)
        return a.ref < b.ref
    end)
    return entries
end

return M
