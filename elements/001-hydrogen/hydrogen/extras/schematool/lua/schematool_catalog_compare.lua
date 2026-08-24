-- schematool_catalog_compare.lua
-- Compare expected catalog JSON vs live catalog JSON (hybrid C checks).
--
-- Usage:
--   lua schematool_catalog_compare.lua \
--     --expected PATH --live PATH \
--     --checklist-out PATH --findings-out PATH \
--     [--only-failures]
--
-- CHANGELOG
-- 1.1.1 - 2026-08-23 - Pass expected column fold ref onto failures[]
-- 1.1.0 - 2026-08-23 - Additive findings: failures[] + live_extras[] (tables unchanged)
-- 1.0.0 - 2026-08-02 - Phase 7c catalog compare

-- luacheck: globals arg

local expected_path, live_path, checklist_out, findings_out
local only_failures = false

local i = 1
while i <= #arg do
    local a = arg[i]
    if a == "--expected" then
        expected_path = arg[i + 1]
        i = i + 2
    elseif a == "--live" then
        live_path = arg[i + 1]
        i = i + 2
    elseif a == "--checklist-out" then
        checklist_out = arg[i + 1]
        i = i + 2
    elseif a == "--findings-out" then
        findings_out = arg[i + 1]
        i = i + 2
    elseif a == "--only-failures" then
        only_failures = true
        i = i + 1
    else
        io.stderr:write("Error: unknown argument: " .. tostring(a) .. "\n")
        os.exit(1)
    end
end

if not expected_path or not live_path or not checklist_out or not findings_out then
    io.stderr:write(
        "Usage: lua schematool_catalog_compare.lua --expected P --live P "
            .. "--checklist-out P --findings-out P [--only-failures]\n"
    )
    os.exit(1)
end

local function write_all(path, data)
    local f, err = io.open(path, "wb")
    if not f then
        io.stderr:write("Error: cannot write " .. path .. ": " .. tostring(err) .. "\n")
        os.exit(1)
    end
    f:write(data)
    f:close()
end

local TMP_DIR = (os.getenv("TMPDIR") or "/tmp")
    .. "/schematool_catcmp_"
    .. tostring(os.time())
    .. "_"
    .. tostring(math.random(100000))
os.execute('mkdir -p "' .. TMP_DIR .. '"')

local function cleanup_tmp()
    os.execute('rm -rf "' .. TMP_DIR .. '"')
end

-- Emit TSV lines: table \t column \t nullable \t data_type \t ref
local function emit_flat(path, out_path)
    local filter = [[
.tables[]? |
  .table as $t |
  (.columns // [])[] |
  [
    ($t // ""),
    (.name // ""),
    (if .nullable == true then "true" elif .nullable == false then "false" else "null" end),
    (.data_type // ""),
    (if .ref then (.ref|tostring) else "" end)
  ] | @tsv
]]
    local fpath = TMP_DIR .. "/flat.jq"
    write_all(fpath, filter .. "\n")
    local cmd = string.format(
        'jq -r -f "%s" "%s" > "%s"',
        fpath,
        path:gsub('"', '\\"'),
        out_path:gsub('"', '\\"')
    )
    local rc = os.execute(cmd)
    if rc ~= true and rc ~= 0 then
        cleanup_tmp()
        io.stderr:write("Error: jq flatten failed for " .. path .. "\n")
        os.exit(1)
    end
end

local function load_flat(path)
    local map = {} -- table -> col -> {nullable=bool|nil, data_type=string}
    local tables_seen = {}
    local f = io.open(path, "r")
    if not f then
        return map, tables_seen
    end
    for line in f:lines() do
        local t, c, n, dt, ref = line:match("^(.-)\t(.-)\t(.-)\t(.-)\t(.*)$")
        if not t then
            t, c, n, dt = line:match("^(.-)\t(.-)\t(.-)\t(.*)$")
            ref = ""
        end
        if t and c then
            t = t:lower()
            c = c:lower()
            tables_seen[t] = true
            local nullable = nil
            if n == "true" then
                nullable = true
            elseif n == "false" then
                nullable = false
            end
            if not map[t] then
                map[t] = {}
            end
            map[t][c] = {
                nullable = nullable,
                data_type = (dt or ""):lower(),
                ref = tonumber(ref),
            }
        end
    end
    f:close()
    return map, tables_seen
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

local exp_flat = TMP_DIR .. "/exp.tsv"
local live_flat = TMP_DIR .. "/live.tsv"
emit_flat(expected_path, exp_flat)
emit_flat(live_path, live_flat)

local exp_map, exp_tables = load_flat(exp_flat)
local live_map, live_tables = load_flat(live_flat)

local rows = {}
local counts = {
    ok = 0,
    missing_table = 0,
    missing_column = 0,
    nullability = 0,
    checked = 0,
    live_extra_table = 0,
    live_extra_column = 0,
}

local exp_names = {}
for t, _ in pairs(exp_tables) do
    exp_names[#exp_names + 1] = t
end
table.sort(exp_names)

for _, tname in ipairs(exp_names) do
    if not live_tables[tname] then
        counts.missing_table = counts.missing_table + 1
        counts.checked = counts.checked + 1
        rows[#rows + 1] = {
            object = tname,
            column = "-",
            check = "table",
            status = "N",
            expected = "present",
            live = "missing",
            notes = "missing table",
        }
        goto continue_table
    end

    local cols = exp_map[tname] or {}
    local col_names = {}
    for c, _ in pairs(cols) do
        col_names[#col_names + 1] = c
    end
    table.sort(col_names)

    for _, cn in ipairs(col_names) do
        local exp = cols[cn]
        local live = (live_map[tname] or {})[cn]
        counts.checked = counts.checked + 1

        if not live then
            counts.missing_column = counts.missing_column + 1
            rows[#rows + 1] = {
                object = tname,
                column = cn,
                check = "column",
                status = "N",
                expected = "present",
                live = "missing",
                notes = "missing column",
                ref = exp.ref,
            }
            goto continue_col
        end

        local status = "Y"
        local notes = {}
        local exp_s = exp.nullable == nil and "-" or tostring(exp.nullable)
        local live_s = live.nullable == nil and "-" or tostring(live.nullable)

        if exp.nullable ~= nil and live.nullable ~= nil and exp.nullable ~= live.nullable then
            status = "N"
            counts.nullability = counts.nullability + 1
            notes[#notes + 1] = "nullable exp=" .. exp_s .. " live=" .. live_s
        end

        if status == "Y" then
            counts.ok = counts.ok + 1
        end

        if status == "N" or not only_failures then
            rows[#rows + 1] = {
                object = tname,
                column = cn,
                check = "nullable",
                status = status,
                expected = exp_s,
                live = live_s,
                notes = table.concat(notes, "; "),
                ref = exp.ref,
            }
        end
        ::continue_col::
    end
    ::continue_table::
end

-- Reverse pass: live tables/columns absent from the expected fold.
-- JSON only (live_extras[]). Do not add these to the tables checklist.
local live_extras = {}
local live_names = {}
for tname, _ in pairs(live_tables) do
    live_names[#live_names + 1] = tname
end
table.sort(live_names)

for _, tname in ipairs(live_names) do
    local live_cols = live_map[tname] or {}
    if not exp_tables[tname] then
        counts.live_extra_table = counts.live_extra_table + 1
        live_extras[#live_extras + 1] = {
            object = tname,
            column = "-",
            check = "extra_table",
            status = "N",
            expected = "missing",
            live = "present",
            notes = "live table not in expected fold",
        }
        local col_names = {}
        for cn, _ in pairs(live_cols) do
            col_names[#col_names + 1] = cn
        end
        table.sort(col_names)
        for _, cn in ipairs(col_names) do
            counts.live_extra_column = counts.live_extra_column + 1
            live_extras[#live_extras + 1] = {
                object = tname,
                column = cn,
                check = "extra_column",
                status = "N",
                expected = "missing",
                live = "present",
                notes = "live column not in expected fold",
            }
        end
    else
        local exp_cols = exp_map[tname] or {}
        local col_names = {}
        for cn, _ in pairs(live_cols) do
            col_names[#col_names + 1] = cn
        end
        table.sort(col_names)
        for _, cn in ipairs(col_names) do
            if not exp_cols[cn] then
                counts.live_extra_column = counts.live_extra_column + 1
                live_extras[#live_extras + 1] = {
                    object = tname,
                    column = cn,
                    check = "extra_column",
                    status = "N",
                    expected = "missing",
                    live = "present",
                    notes = "live column not in expected fold",
                }
            end
        end
    end
end

local exit_code = 0
if counts.missing_table > 0 or counts.missing_column > 0 or counts.nullability > 0 then
    exit_code = 2
end

local function row_json(r)
    local ref_json = ""
    if r.ref then
        ref_json = string.format(',"ref":%d', r.ref)
    end
    return string.format(
        '{"object":"%s","column":"%s","check":"%s","status":"%s",'
            .. '"expected":"%s","live":"%s","notes":"%s"%s}',
        json_escape(r.object),
        json_escape(r.column),
        json_escape(r.check),
        json_escape(r.status),
        json_escape(r.expected),
        json_escape(r.live),
        json_escape(r.notes or ""),
        ref_json
    )
end

local cl_parts = { "[" }
local first = true
for _, r in ipairs(rows) do
    local include = true
    if only_failures and r.status == "Y" then
        include = false
    end
    if include then
        if not first then
            cl_parts[#cl_parts + 1] = ","
        end
        first = false
        cl_parts[#cl_parts + 1] = row_json(r)
    end
end
cl_parts[#cl_parts + 1] = "]"
write_all(checklist_out, table.concat(cl_parts) .. "\n")

local fail_parts = {}
for _, r in ipairs(rows) do
    if r.status == "N" then
        fail_parts[#fail_parts + 1] = row_json(r)
    end
end

local extra_parts = {}
for _, r in ipairs(live_extras) do
    extra_parts[#extra_parts + 1] = row_json(r)
end

local findings = string.format(
    '{"exit_code":%d,"counts":{"ok":%d,"missing_table":%d,"missing_column":%d,'
        .. '"nullability":%d,"checked":%d,"live_extra_table":%d,"live_extra_column":%d},'
        .. '"track":"catalog","failures":[%s],"live_extras":[%s]}',
    exit_code,
    counts.ok,
    counts.missing_table,
    counts.missing_column,
    counts.nullability,
    counts.checked,
    counts.live_extra_table,
    counts.live_extra_column,
    table.concat(fail_parts, ","),
    table.concat(extra_parts, ",")
)
write_all(findings_out, findings .. "\n")

cleanup_tmp()
os.exit(0)
