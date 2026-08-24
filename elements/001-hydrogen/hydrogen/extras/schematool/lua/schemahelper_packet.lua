-- schemahelper_packet.lua
-- Reserve the next migration ref and write a review-only packet directory.
--
-- CHANGELOG
-- 0.5.5 - 2026-08-24 - Phase 7: promote to Helium migration stub
-- 0.4.0 - 2026-08-23 - Phase 4: next-ref, collision, packet files

local M = {}

local function sh_quote(s)
    return "'" .. tostring(s):gsub("'", "'\\''") .. "'"
end

local function json_escape(s)
    s = tostring(s or "")
    s = s:gsub("\\", "\\\\")
    s = s:gsub('"', '\\"')
    s = s:gsub("\n", "\\n")
    s = s:gsub("\r", "\\r")
    s = s:gsub("\t", "\\t")
    return s
end

local function is_dir(path)
    if not path or path == "" then
        return false
    end
    local h = io.popen("test -d " .. sh_quote(path) .. " && printf yes")
    if not h then
        return false
    end
    local out = h:read("*a") or ""
    h:close()
    return out == "yes"
end

local function file_exists(path)
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

local function write_all(path, data)
    local f, err = io.open(path, "wb")
    if not f then
        return nil, err
    end
    f:write(data)
    f:close()
    return true
end

local function read_all(path)
    local f = io.open(path, "r")
    if not f then
        return nil
    end
    local data = f:read("*a")
    f:close()
    return data
end

local function list_names(path)
    local names = {}
    if not path or path == "" then
        return names
    end
    local h = io.popen("ls -1 " .. sh_quote(path) .. " 2>/dev/null")
    if not h then
        return names
    end
    for line in h:lines() do
        if line and line ~= "" then
            names[#names + 1] = line
        end
    end
    h:close()
    return names
end

local function json_string_field(obj, key)
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

local function json_num_field(obj, key)
    local n = obj:match('"' .. key .. '"%s*:%s*(%-?%d+)')
    return n and tonumber(n) or nil
end

function M.packet_name(design, engine, ref)
    return string.format("schemahelper_%s_%s_%d", design, engine, ref)
end

function M.packet_path(packet_dir, design, engine, ref)
    return (packet_dir or ".") .. "/" .. M.packet_name(design, engine, ref)
end

function M.in_git_tree(path)
    if not path or path == "" then
        return false
    end
    local h = io.popen("git -C " .. sh_quote(path)
        .. " rev-parse --is-inside-work-tree 2>/dev/null")
    if not h then
        return false
    end
    local out = (h:read("*l") or ""):gsub("%s+$", "")
    h:close()
    return out == "true"
end

local function scan_disk_refs(migrations, design)
    local max_ref = 0
    local by_ref = {}
    if not migrations or migrations == "" or not is_dir(migrations) then
        return max_ref, by_ref
    end
    local design_pat = (design or "design"):gsub("(%W)", "%%%1")
    local patterns = {
        "^" .. design_pat .. "_(%d+)%.lua$",
        "^design_(%d+)%.lua$",
    }
    for _, name in ipairs(list_names(migrations)) do
        for i = 1, #patterns do
            local n = name:match(patterns[i])
            if n then
                local ref = tonumber(n)
                if ref then
                    by_ref[ref] = migrations .. "/" .. name
                    if ref > max_ref then
                        max_ref = ref
                    end
                end
                break
            end
        end
    end
    return max_ref, by_ref
end

local function read_manifest(dir)
    local body = read_all(dir .. "/MANIFEST.json")
    if not body then
        return nil
    end
    return {
        ref = json_num_field(body, "ref"),
        status = json_string_field(body, "status"),
        design = json_string_field(body, "design"),
        engine = json_string_field(body, "engine"),
        path = dir,
    }
end

function M.list_reserved(packet_dir, design, engine)
    local list = {}
    if not packet_dir or packet_dir == "" or not is_dir(packet_dir) then
        return list
    end
    local prefix = "schemahelper_" .. (design or "") .. "_" .. (engine or "") .. "_"
    for _, name in ipairs(list_names(packet_dir)) do
        local ref = name:match("_(%d+)$")
        if ref and name:sub(1, #prefix) == prefix then
            local dir = packet_dir .. "/" .. name
            if is_dir(dir) then
                local man = read_manifest(dir)
                local item = man or {
                    ref = tonumber(ref),
                    status = "reserved",
                    path = dir,
                }
                item.ref = item.ref or tonumber(ref)
                item.path = item.path or dir
                item.name = name
                if not item.status or item.status == "" then
                    item.status = "reserved"
                end
                if item.status == "reserved" then
                    list[#list + 1] = item
                end
            end
        end
    end
    table.sort(list, function(a, b)
        return (a.ref or 0) < (b.ref or 0)
    end)
    return list
end

function M.scan_refs(opts)
    opts = opts or {}
    local max_ref, disk = scan_disk_refs(opts.migrations, opts.design)
    local reserved = M.list_reserved(opts.packet_dir, opts.design, opts.engine)
    local reserved_by_ref = {}
    for i = 1, #reserved do
        local ref = reserved[i].ref
        if ref then
            reserved_by_ref[ref] = reserved[i].path
            if ref > max_ref then
                max_ref = ref
            end
        end
    end
    return {
        max_ref = max_ref,
        disk = disk,
        reserved = reserved_by_ref,
    }
end

function M.next_ref(opts)
    local scan = M.scan_refs(opts)
    local forced = tonumber(opts and opts.ref)
    if forced and forced > 0 then
        return forced, scan
    end
    return scan.max_ref + 1, scan
end

function M.collision(opts, ref)
    ref = tonumber(ref)
    if not ref or ref < 1 then
        return "invalid ref"
    end
    local scan = M.scan_refs(opts)
    if scan.disk[ref] then
        return "collision: " .. scan.disk[ref] .. " already exists"
    end
    if scan.reserved[ref] then
        return "collision: " .. scan.reserved[ref] .. " already reserved"
    end
    local dest = M.packet_path(opts.packet_dir, opts.design, opts.engine, ref)
    if is_dir(dest) then
        return "collision: " .. dest .. " already exists"
    end
    return nil, dest
end

local function latest_artifact(out_dir, design, engine, ext)
    local prefix = string.format("schematool_%s_%s_", design or "", engine or "")
    local newest
    for _, name in ipairs(list_names(out_dir)) do
        if name:sub(1, #prefix) == prefix and name:sub(-#ext) == ext then
            if not newest or name > newest then
                newest = name
            end
        end
    end
    if not newest then
        return nil
    end
    return out_dir .. "/" .. newest
end

local function orphan_mig_excerpt(mig_body, ref)
    if not mig_body or mig_body == "" then
        return nil
    end
    local marker = string.format("ORPHAN REF %d", ref)
    local start = mig_body:find(marker, 1, true)
    if not start then
        return mig_body
    end
    local line_start = mig_body:sub(1, start):match(".*\n()[^\n]*$") or 1
    local rest = mig_body:sub(line_start)
    local nxt = rest:find("\nORPHAN REF ", 2, true)
    if nxt then
        return rest:sub(1, nxt)
    end
    return rest
end

function M.suggested_sql(opts, finding)
    local lines = {
        "-- SchemaHelper suggested SQL — REVIEW ONLY. Not applied.",
        "-- Direction: live database → official (inverse of SchemaTool remediation).",
        "-- Confirm the reserved ref is free before promoting into Helium.",
        "--",
        "-- finding: " .. (finding.id or ""),
        "-- class:   " .. (finding.class or ""),
    }
    if finding.ref then
        lines[#lines + 1] = "-- live/orphan ref: " .. tostring(finding.ref)
    end
    if finding.object and finding.object ~= "" then
        lines[#lines + 1] = "-- object: " .. finding.object
            .. "  column: " .. (finding.column or "-")
    end
    if finding.expected and finding.expected ~= "" then
        lines[#lines + 1] = "-- expected: " .. finding.expected
    end
    if finding.live and finding.live ~= "" then
        lines[#lines + 1] = "-- live:     " .. finding.live
    end
    if finding.summary and finding.summary ~= "" then
        lines[#lines + 1] = "-- note:     " .. finding.summary
    end
    lines[#lines + 1] = ""
    if finding.kind == "orphan" then
        local mig = latest_artifact(opts.out_dir, opts.design, opts.engine, ".mig")
        if mig then
            local body = read_all(mig)
            local excerpt = orphan_mig_excerpt(body, finding.ref or 0)
            lines[#lines + 1] = "-- Copied from SchemaTool orphan .mig (not a runnable migration):"
            lines[#lines + 1] = "-- " .. mig
            lines[#lines + 1] = ""
            if excerpt then
                lines[#lines + 1] = excerpt
            end
        else
            lines[#lines + 1] = "-- No SchemaTool .mig artifact in the workspace."
            lines[#lines + 1] = "-- Author a new official migration that captures this live DB ref."
        end
    elseif finding.kind == "extra_table" or finding.kind == "extra_column"
        or finding.class == "catalog live extra" then
        lines[#lines + 1] = "-- Live-ahead catalog object. Best-effort notes only:"
        lines[#lines + 1] = "--   DESCRIBE / capture the live table or column, then author"
        lines[#lines + 1] = "--   a multi-engine design_NNNN.lua. This is not complete DDL."
        if finding.object and finding.object ~= "" then
            lines[#lines + 1] = string.format(
                "--   live object: %s.%s",
                finding.object,
                finding.column or "-"
            )
        end
    else
        lines[#lines + 1] = "-- Packet of context for a human author. Do not apply this file."
        lines[#lines + 1] = "-- SchemaTool remediation SQL (if any) is the opposite direction."
    end
    if lines[#lines] ~= "" then
        lines[#lines + 1] = ""
    end
    return table.concat(lines, "\n")
end

local function finding_json(finding)
    local fields = {}
    local function add_str(key, val)
        if val ~= nil and val ~= "" then
            fields[#fields + 1] = string.format('  "%s": "%s"', key, json_escape(val))
        end
    end
    local function add_num(key, val)
        if val ~= nil then
            fields[#fields + 1] = string.format('  "%s": %s', key, tostring(val))
        end
    end
    add_str("id", finding.id)
    add_str("class", finding.class)
    add_str("kind", finding.kind)
    add_num("ref", finding.ref)
    add_str("file", finding.file)
    add_str("object", finding.object)
    add_str("column", finding.column)
    add_str("expected", finding.expected)
    add_str("live", finding.live)
    add_str("actual", finding.actual)
    add_str("summary", finding.summary)
    return "{\n" .. table.concat(fields, ",\n") .. "\n}\n"
end

local function packet_md(opts, finding, dest_name, ref, note)
    local lines = {
        "# SchemaHelper packet " .. tostring(ref),
        "",
        "- design: `" .. (opts.design or "") .. "`",
        "- engine: `" .. (opts.engine or "") .. "`",
        "- schema: `" .. (opts.schema or "") .. "`",
        "- status: `reserved`",
        "- packet: `" .. dest_name .. "`",
        "- finding: `" .. (finding.id or "") .. "`",
        "- class: `" .. (finding.class or "") .. "`",
        "- created_utc: `" .. (opts.created_utc or "") .. "`",
    }
    if note and note ~= "" then
        lines[#lines + 1] = "- operator note: " .. note
    end
    lines[#lines + 1] = ""
    lines[#lines + 1] = "This is **not** a migration. Confirm the number is free"
    lines[#lines + 1] = "before promoting into Helium as `"
        .. (opts.design or "design") .. "_" .. tostring(ref) .. ".lua`."
    lines[#lines + 1] = ""
    lines[#lines + 1] = "Sibling files: `MANIFEST.json`, `FINDING.json`,"
    lines[#lines + 1] = "`DETAIL.txt`, `SUGGESTED.sql`."
    lines[#lines + 1] = ""
    return table.concat(lines, "\n")
end

local function manifest_json(opts, finding, ref, note)
    local ids = '["' .. json_escape(finding.id or "") .. '"]'
    local note_line = ""
    if note and note ~= "" then
        note_line = ',\n  "note": "' .. json_escape(note) .. '"'
    end
    return string.format(
        '{\n  "ref": %d,\n  "design": "%s",\n  "engine": "%s",\n  "schema": "%s",'
            .. '\n  "created_utc": "%s",\n  "status": "reserved",'
            .. '\n  "finding_ids": %s,\n  "source": "schemahelper",'
            .. '\n  "schemahelper_version": "%s",\n  "schematool_version": "%s"%s\n}\n',
        ref,
        json_escape(opts.design or ""),
        json_escape(opts.engine or ""),
        json_escape(opts.schema or ""),
        json_escape(opts.created_utc or ""),
        ids,
        json_escape(opts.schemahelper_version or "0.4.0"),
        json_escape(opts.schematool_version or ""),
        note_line
    )
end

function M.write(opts, finding, extra)
    extra = extra or {}
    if not finding or not finding.id then
        return nil, "no finding"
    end
    opts = opts or {}
    if not opts.packet_dir or opts.packet_dir == "" then
        return nil, "packet-dir is required"
    end
    local ref = M.next_ref(opts)
    local collide, dest = M.collision(opts, ref)
    if collide then
        return nil, collide
    end
    os.execute("mkdir -p " .. sh_quote(dest))
    if not is_dir(dest) then
        return nil, "failed to create " .. dest
    end
    opts.created_utc = opts.created_utc or os.date("!%Y-%m-%dT%H:%M:%SZ")
    local note = extra.note or ""
    local detail = extra.detail_text
    if not detail or detail == "" then
        detail = (finding.summary or finding.id or "") .. "\n"
    end
    local sql = extra.suggested_sql
    if not sql or sql == "" then
        sql = M.suggested_sql(opts, finding)
    end
    local dest_name = M.packet_name(opts.design, opts.engine, ref)
    local ok, err = write_all(dest .. "/MANIFEST.json", manifest_json(opts, finding, ref, note))
    if not ok then
        return nil, err
    end
    ok, err = write_all(dest .. "/PACKET.md", packet_md(opts, finding, dest_name, ref, note))
    if not ok then
        return nil, err
    end
    ok, err = write_all(dest .. "/FINDING.json", finding_json(finding))
    if not ok then
        return nil, err
    end
    ok, err = write_all(dest .. "/DETAIL.txt", detail)
    if not ok then
        return nil, err
    end
    ok, err = write_all(dest .. "/SUGGESTED.sql", sql)
    if not ok then
        return nil, err
    end
    return {
        ref = ref,
        path = dest,
        name = dest_name,
        note = note,
    }
end

local function lua_brackets(level)
    local eqs = ("="):rep(level)
    return "[" .. eqs .. "[", "]" .. eqs .. "]"
end

local function pick_lua_level(s)
    local level = 0
    local marker = "]"
    while s:find(marker, 1, true) do
        level = level + 1
        marker = "]" .. ("="):rep(level) .. "]"
    end
    return level
end

local function generate_migration_stub(design, ref, engine, finding, suggested,
    sh_ver, st_ver)
    local level = pick_lua_level(suggested or "")
    local open, close = lua_brackets(level)
    local finding_id = json_escape(finding.id or "")
    local object = json_escape(finding.object or "")
    local column = json_escape(finding.column or "")
    local class = json_escape(finding.class or "")
    local desc = "Catalog DDL: " .. (finding.kind or "")
        .. " " .. object
    if column ~= "" and column ~= "-" then
        desc = desc .. "." .. column
    end
    local ddl_body = (suggested or ""):gsub("^%s+", ""):gsub("%s+$", "")
    return string.format([[-- Migration: %s_%04d.lua
-- Promoted from SchemaHelper packet.
-- STUB — review, complete, and author the proper INSERT before loading.
--
-- SchemaHelper: %s   SchemaTool: %s
-- Engine: %s
-- Finding: %s
-- Class: %s

-- luacheck: no max line length
-- luacheck: no unused args

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "queries"
cfg.MIGRATION = "%04d"
cfg.QUERY_REF = "%04d"
cfg.QUERY_NAME = %s
-- FIXME: assign a real QueryRef if %04d is reserved; author the INSERT with
--        ${COMMON_INSERT} fields and a reverse that flips type 1003→forward.

-- ----------------------------------------------------------------------------
-- Forward: %s
-- ----------------------------------------------------------------------------
table.insert(queries,{sql=%s

%s

%s})})

-- ----------------------------------------------------------------------------
-- Reverse
-- ----------------------------------------------------------------------------
-- TODO: author reverse. For catalog nullable/add-column changes the reverse
--       is usually a no-op marker (flip type 1003→forward) so the migration
--       chain can reverse past this ref. See acuranzo_1290 for the pattern.
table.insert(queries,{sql=%s

%s

%s})})

return queries end
]],
        design, ref,
        sh_ver or "", st_ver or "",
        engine or "",
        finding_id,
        class,
        ref, ref,
        string.format("%q", desc),
        ref,
        "DDL",
        open, ddl_body, close,
        -- reverse stub
        open, "-- TODO: reverse DDL", close
    )
end

function M.promote(opts, finding)
    opts = opts or {}
    local design = opts.design or ""
    local engine = opts.engine or ""
    local migrations = opts.migrations or ""
    local packet_dir, packet_ref

    if finding and finding.id then
        local dec = opts.state and opts.state.by_id
            and opts.state.by_id[finding.id]
        if dec and dec.action == "packet"
            and dec.ref and dec.packet then
            packet_ref = dec.ref
            packet_dir = M.packet_path(
                opts.packet_dir, design, engine, dec.ref)
        end
    end

    if not packet_dir or not packet_ref then
        return nil, "no packet for this finding; generate with [G] first"
    end

    if not file_exists(packet_dir) then
        return nil, "packet directory not found: " .. tostring(packet_dir)
    end

    local suggested_path = packet_dir .. "/SUGGESTED.sql"
    if not file_exists(suggested_path) then
        return nil, "packet SUGGESTED.sql not found"
    end
    local suggested = read_all(suggested_path) or ""

    local manifest_body = read_all(packet_dir .. "/MANIFEST.json") or "{}"
    local ref = json_num_field(manifest_body, "ref")
    if not ref or ref < 1 then
        return nil, "cannot determine packet ref from MANIFEST.json"
    end

    if not migrations or migrations == "" then
        return nil, "migrations path is required"
    end
    if not is_dir(migrations) then
        return nil, "migrations directory not found: " .. migrations
    end

    local dest_name = design .. "_" .. string.format("%04d", ref) .. ".lua"
    local dest_path = migrations .. "/" .. dest_name
    if file_exists(dest_path) then
        return nil, "destination already exists: " .. dest_path
    end

    local stub = generate_migration_stub(
        design, ref, engine, finding, suggested,
        opts.schemahelper_version or "0.5.5",
        opts.schematool_version or "")
    local ok, err = write_all(dest_path, stub)
    if not ok then
        return nil, err
    end

    local manifest = read_all(packet_dir .. "/MANIFEST.json") or ""
    local patched = manifest:gsub(
        '"status"%s*:%s*"reserved"', '"status": "promoted"')
    if patched == manifest then
        patched = manifest:gsub(
            '"status"%s*:%s*"reserved"', '"status": "promoted"')
    end
    write_all(packet_dir .. "/MANIFEST.json", patched)

    return dest_path, dest_name, ref
end

return M
