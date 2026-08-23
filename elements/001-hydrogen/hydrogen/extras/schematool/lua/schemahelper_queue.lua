-- schemahelper_queue.lua
-- Merge SchemaTool metadata + catalog JSON into a review queue.
--
-- CHANGELOG
-- 0.2.0 - 2026-08-23 - Phase 1: findings ingest, sidecar decisions, dashboard totals

local M = {}

local function json_escape(s)
    s = tostring(s or "")
    s = s:gsub("\\", "\\\\")
    s = s:gsub('"', '\\"')
    s = s:gsub("\n", "\\n")
    s = s:gsub("\r", "\\r")
    s = s:gsub("\t", "\\t")
    return s
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

local function jq_lines(filter, path, tmp_dir)
    if not file_exists(path) then
        return {}
    end
    local fpath = tmp_dir .. "/q.jq"
    write_all(fpath, filter .. "\n")
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

local function jq_num(filter, path, tmp_dir)
    local lines = jq_lines(filter, path, tmp_dir)
    if #lines == 0 then
        return 0
    end
    return tonumber(lines[1]) or 0
end

local function first_field(obj)
    local arr = obj:match('"fields"%s*:%s*%[(.-)%]')
    if not arr then
        return "code"
    end
    local field = arr:match('"([^"]+)"')
    return field or "code"
end

local function add_finding(list, item)
    list[#list + 1] = item
end

local function load_metadata(path, tmp_dir, findings)
    if not file_exists(path) then
        return {
            total = 0,
            ok = 0,
            orphans = 0,
        }
    end
    local counts = {
        total = jq_num(".counts.total // 0", path, tmp_dir),
        ok = jq_num(".counts.ok // 0", path, tmp_dir),
        orphans = jq_num(".counts.orphans // 0", path, tmp_dir),
    }

    for _, obj in ipairs(jq_lines(".drifts[]?", path, tmp_dir)) do
        local ref = json_num_field(obj, "ref") or 0
        local db_type = json_num_field(obj, "db_type") or 1003
        local field = first_field(obj)
        add_finding(findings, {
            id = string.format("meta:drift:%d:%d:%s", ref, db_type, field),
            class = "metadata content drift",
            kind = "drift",
            ref = ref,
            file = json_string_field(obj, "file"),
            summary = string.format("ref %d type %d field %s", ref, db_type, field),
        })
    end

    for _, obj in ipairs(jq_lines(".missing_load[]?", path, tmp_dir)) do
        local ref = json_num_field(obj, "ref") or 0
        add_finding(findings, {
            id = string.format("meta:missing_load:%d", ref),
            class = "missing LOAD",
            kind = "missing_load",
            ref = ref,
            file = json_string_field(obj, "file"),
            summary = string.format("ref %d on disk, not loaded", ref),
        })
    end

    for _, obj in ipairs(jq_lines(".missing_apply[]?", path, tmp_dir)) do
        local ref = json_num_field(obj, "ref") or 0
        add_finding(findings, {
            id = string.format("meta:missing_apply:%d", ref),
            class = "missing APPLY",
            kind = "missing_apply",
            ref = ref,
            file = json_string_field(obj, "file"),
            summary = string.format("ref %d loaded, not applied", ref),
        })
    end

    for _, obj in ipairs(jq_lines(".anomalies[]?", path, tmp_dir)) do
        local ref = json_num_field(obj, "ref") or 0
        local kind = json_string_field(obj, "kind")
        if kind == "" then
            kind = "anomaly"
        end
        add_finding(findings, {
            id = string.format("meta:anomaly:%s:%d", kind, ref),
            class = "anomaly 1000+1003",
            kind = kind,
            ref = ref,
            file = json_string_field(obj, "file"),
            summary = string.format("ref %d %s", ref, kind),
        })
    end

    for _, obj in ipairs(jq_lines(".orphans[]?", path, tmp_dir)) do
        local ref = json_num_field(obj, "ref") or 0
        add_finding(findings, {
            id = string.format("orphan:%d", ref),
            class = "orphan DB ref",
            kind = "orphan",
            ref = ref,
            file = "(orphan)",
            summary = string.format("ref %d in DB, not on disk", ref),
        })
    end

    return counts
end

local function catalog_class(check)
    if check == "table" then
        return "catalog missing table"
    end
    if check == "column" then
        return "catalog missing column"
    end
    if check == "nullable" then
        return "catalog nullability"
    end
    if check == "extra_table" or check == "extra_column" then
        return "catalog live extra"
    end
    return "catalog " .. check
end

local function add_catalog_rows(findings, rows, kind_fallback)
    for _, obj in ipairs(rows) do
        local check = json_string_field(obj, "check")
        if check == "" then
            check = kind_fallback
        end
        local object = json_string_field(obj, "object")
        local column = json_string_field(obj, "column")
        if column == "" then
            column = "-"
        end
        add_finding(findings, {
            id = string.format("cat:%s:%s:%s", object, column, check),
            class = catalog_class(check),
            kind = check,
            object = object,
            column = column,
            expected = json_string_field(obj, "expected"),
            live = json_string_field(obj, "live"),
            summary = json_string_field(obj, "notes"),
        })
    end
end

local function load_catalog(path, tmp_dir, findings)
    if not file_exists(path) then
        return { checked = 0, ok = 0 }
    end
    add_catalog_rows(findings, jq_lines(".failures[]?", path, tmp_dir), "catalog")
    add_catalog_rows(findings, jq_lines(".live_extras[]?", path, tmp_dir), "extra_column")
    return {
        checked = jq_num(".counts.checked // 0", path, tmp_dir),
        ok = jq_num(".counts.ok // 0", path, tmp_dir),
    }
end

function M.default_state_path(out_dir, design, engine)
    return string.format("%s/schemahelper_%s_%s.json", out_dir, design, engine)
end

function M.load_state(path)
    local state = {
        version = 1,
        design = "",
        engine = "",
        schema = "",
        updated_utc = "",
        cursor_id = "",
        decisions = {},
        by_id = {},
    }
    if not file_exists(path) then
        return state
    end
    local tmp = (os.getenv("TMPDIR") or "/tmp")
        .. "/schemahelper_st_"
        .. tostring(os.time())
        .. "_"
        .. tostring(math.random(100000))
    os.execute('mkdir -p "' .. tmp .. '"')
    state.design = (jq_lines(".design", path, tmp)[1] or ""):gsub('^"', ""):gsub('"$', "")
    state.engine = (jq_lines(".engine", path, tmp)[1] or ""):gsub('^"', ""):gsub('"$', "")
    state.schema = (jq_lines(".schema // \"\"", path, tmp)[1] or ""):gsub('^"', ""):gsub('"$', "")
    state.cursor_id = (jq_lines(".cursor_id // \"\"", path, tmp)[1] or ""):gsub('^"', ""):gsub('"$', "")
    for _, obj in ipairs(jq_lines(".decisions[]?", path, tmp)) do
        local id = json_string_field(obj, "id")
        local action = json_string_field(obj, "action")
        if id ~= "" then
            local rec = { id = id, action = action }
            state.decisions[#state.decisions + 1] = rec
            state.by_id[id] = rec
        end
    end
    os.execute('rm -rf "' .. tmp .. '"')
    return state
end

function M.create_state(path, design, engine, schema)
    local now = os.date("!%Y-%m-%dT%H:%M:%SZ")
    local body = string.format(
        '{\n  "version": 1,\n  "design": "%s",\n  "engine": "%s",\n  "schema": "%s",'
            .. '\n  "updated_utc": "%s",\n  "cursor_id": "",\n  "decisions": []\n}\n',
        json_escape(design),
        json_escape(engine),
        json_escape(schema or ""),
        now
    )
    return write_all(path, body)
end

function M.artifacts_present(out_dir, track)
    local meta = out_dir .. "/findings.json"
    local cat = out_dir .. "/catalog_findings.json"
    if track == "catalog" then
        return file_exists(cat)
    end
    if track == "metadata" then
        return file_exists(meta)
    end
    return file_exists(meta) or file_exists(cat)
end

function M.build(opts)
    opts = opts or {}
    local out_dir = opts.out_dir or "."
    local track = opts.track or "both"
    local state = opts.state or { by_id = {} }

    local tmp = (os.getenv("TMPDIR") or "/tmp")
        .. "/schemahelper_q_"
        .. tostring(os.time())
        .. "_"
        .. tostring(math.random(100000))
    os.execute('mkdir -p "' .. tmp .. '"')

    local all = {}
    local meta_counts = { total = 0, ok = 0, orphans = 0 }
    local cat_counts = { checked = 0, ok = 0 }

    if track == "metadata" or track == "both" then
        meta_counts = load_metadata(out_dir .. "/findings.json", tmp, all)
    end
    if track == "catalog" or track == "both" then
        cat_counts = load_catalog(out_dir .. "/catalog_findings.json", tmp, all)
    end

    os.execute('rm -rf "' .. tmp .. '"')

    local subject = {}
    local classes = {}
    local accepted = 0
    local applied = 0
    local packet = 0
    local skipped = 0

    for _, item in ipairs(all) do
        local dec = state.by_id and state.by_id[item.id]
        local action = dec and dec.action or ""
        item.action = action
        if action == "accepted" then
            accepted = accepted + 1
        elseif action == "applied" then
            applied = applied + 1
        elseif action == "packet" then
            packet = packet + 1
        else
            if action == "skipped" then
                skipped = skipped + 1
            end
            subject[#subject + 1] = item
            local cls = item.class
            classes[cls] = (classes[cls] or 0) + 1
        end
    end

    local class_list = {}
    for name, n in pairs(classes) do
        class_list[#class_list + 1] = { name = name, count = n }
    end
    table.sort(class_list, function(a, b)
        if a.count == b.count then
            return a.name < b.name
        end
        return a.count > b.count
    end)

    local total = meta_counts.total + meta_counts.orphans
    if total == 0 and track == "catalog" then
        total = cat_counts.checked
    end

    return {
        findings = all,
        subject = subject,
        classes = class_list,
        totals = {
            total = total,
            perfect = meta_counts.ok,
            accepted = accepted,
            subject = #subject,
            applied = applied,
            packet = packet,
            skipped = skipped,
            catalog_ok = cat_counts.ok,
            catalog_checked = cat_counts.checked,
        },
    }
end

return M
