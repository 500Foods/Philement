-- schematool_compare.lua
-- Join disk ⟷ expected ⟷ DB rows; emit checklist data JSON + findings JSON.
--
-- Usage:
--   lua schematool_compare.lua \
--     --disk PATH --expected PATH --db PATH \
--     --normalize loose|strict \
--     --checklist-out PATH --findings-out PATH \
--     [--only-failures] [--include-reverse] [--include-diagram]
--
-- CHANGELOG
-- 1.0.0 - 2026-07-29 - Phase 4 compare

-- luacheck: globals arg package

local function script_dir()
    local src = arg[0] or ""
    local dir = src:match("^(.*)/[^/]+$")
    return dir or "."
end

package.path = script_dir() .. "/?.lua;" .. package.path

local normalize = require("schematool_normalize")

local disk_path, disk_all_path, expected_path, db_path
local checklist_out, findings_out
local mode = "loose"
local only_failures = false
local include_reverse = false
local include_diagram = false

local i = 1
while i <= #arg do
    local a = arg[i]
    if a == "--disk" then
        disk_path = arg[i + 1]
        i = i + 2
    elseif a == "--disk-all" then
        disk_all_path = arg[i + 1]
        i = i + 2
    elseif a == "--expected" then
        expected_path = arg[i + 1]
        i = i + 2
    elseif a == "--db" then
        db_path = arg[i + 1]
        i = i + 2
    elseif a == "--normalize" then
        mode = arg[i + 1] or "loose"
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
    elseif a == "--include-reverse" then
        include_reverse = true
        i = i + 1
    elseif a == "--include-diagram" then
        include_diagram = true
        i = i + 1
    else
        io.stderr:write("Error: unknown argument: " .. tostring(a) .. "\n")
        os.exit(1)
    end
end

if not disk_path or not expected_path or not db_path or not checklist_out or not findings_out then
    io.stderr:write(
        "Usage: lua schematool_compare.lua --disk P --expected P --db P "
        .. "--checklist-out P --findings-out P [--disk-all P] [--normalize loose|strict] "
        .. "[--only-failures] [--include-reverse] [--include-diagram]\n"
    )
    os.exit(1)
end
disk_all_path = disk_all_path or disk_path

if mode ~= "loose" and mode ~= "strict" then
    io.stderr:write("Error: --normalize must be loose|strict\n")
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
    .. "/schematool_cmp_"
    .. tostring(os.time())
    .. "_"
    .. tostring(math.random(100000))
os.execute('mkdir -p "' .. TMP_DIR .. '"')

local function cleanup_tmp()
    os.execute('rm -rf "' .. TMP_DIR .. '"')
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
            elseif n == "u" then
                local hex = obj:sub(i2 + 2, i2 + 5)
                local code = tonumber(hex, 16)
                if code and code < 128 then
                    parts[#parts + 1] = string.char(code)
                else
                    parts[#parts + 1] = "?"
                end
                i2 = i2 + 5
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

-- Use jq -f so $vars in filters are not expanded by the shell
local jq_seq = 0
local function jq_lines(filter, path)
    jq_seq = jq_seq + 1
    local fpath = TMP_DIR .. "/f" .. tostring(jq_seq) .. ".jq"
    write_all(fpath, filter .. "\n")
    local cmd = 'jq -c -f "' .. fpath .. '" "' .. path:gsub('"', '\\"') .. '"'
    local h = io.popen(cmd)
    if not h then
        cleanup_tmp()
        io.stderr:write("Error: jq failed: " .. filter .. "\n")
        os.exit(1)
    end
    local lines = {}
    for line in h:lines() do
        lines[#lines + 1] = line
    end
    h:close()
    return lines
end

local function parse_payload_line(line)
    return {
        query_type = json_num_field(line, "query_type"),
        name = json_string_field(line, "name"),
        summary = json_string_field(line, "summary"),
        code = json_string_field(line, "code"),
        query_ref = json_num_field(line, "query_ref"),
    }
end

local function load_disk(path)
    local rows = {}
    for _, line in ipairs(jq_lines(".[] | {ref, file}", path)) do
        local ref = json_num_field(line, "ref")
        if ref then
            rows[#rows + 1] = {
                ref = ref,
                file = json_string_field(line, "file"),
            }
        end
    end
    table.sort(rows, function(a, b)
        return a.ref < b.ref
    end)
    return rows
end

local function load_db(path)
    local by_ref = {}
    for _, line in ipairs(jq_lines(".[]", path)) do
        local ref = json_num_field(line, "query_ref")
        local qtype = json_num_field(line, "query_type")
        if ref and qtype then
            if not by_ref[ref] then
                by_ref[ref] = {}
            end
            by_ref[ref][qtype] = {
                query_ref = ref,
                query_type = qtype,
                name = json_string_field(line, "name"),
                summary = json_string_field(line, "summary"),
                code = json_string_field(line, "code"),
            }
        end
    end
    return by_ref
end

-- Flatten expected: one line per payload with ref
local function load_expected(path)
    local by_ref = {}
    -- jq -f filter (written to temp file; $vars safe)
    local filter = table.concat({
        'if type=="array" then .[]',
        'elif type=="object" and has("payloads") then .',
        "else empty end",
        "| . as $m",
        "| ($m.ref // $m.query_ref) as $r",
        '| ($m.file // "") as $f',
        "| ($m.payloads // [])[]",
        "| {ref:$r, file:$f, query_type, name, summary, code}",
    }, " ")
    for _, line in ipairs(jq_lines(filter, path)) do
        local ref = json_num_field(line, "ref")
        local qt = json_num_field(line, "query_type")
        if ref and qt then
            if not by_ref[ref] then
                by_ref[ref] = {
                    ref = ref,
                    file = json_string_field(line, "file"),
                    by_type = {},
                }
            end
            by_ref[ref].by_type[qt] = {
                query_type = qt,
                name = json_string_field(line, "name"),
                summary = json_string_field(line, "summary"),
                code = json_string_field(line, "code"),
            }
        end
    end
    return by_ref
end

local function pick_expected_forward(exp)
    if not exp then
        return nil
    end
    return exp.by_type[1000] or exp.by_type[1003]
end

local function pick_db_forward(types)
    if not types then
        return nil, nil
    end
    if types[1000] then
        return types[1000], 1000
    end
    if types[1003] then
        return types[1003], 1003
    end
    return nil, nil
end

local function yn(v)
    return v and "Y" or "N"
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

local function note_join(parts)
    local out = {}
    for _, p in ipairs(parts) do
        if p and p ~= "" then
            out[#out + 1] = p
        end
    end
    return table.concat(out, "; ")
end

local function payload_json(p)
    if not p then
        return "null"
    end
    return string.format(
        '{"query_type":%s,"name":"%s","summary":"%s","code":"%s"}',
        p.query_type and tostring(p.query_type) or "null",
        json_escape(p.name),
        json_escape(p.summary),
        json_escape(p.code)
    )
end

local disk = load_disk(disk_path)
local disk_all = load_disk(disk_all_path)
local expected = load_expected(expected_path)
local db = load_db(db_path)

-- Orphan membership uses full on-disk set (not --from/--to window)
local disk_set = {}
for _, d in ipairs(disk_all) do
    disk_set[d.ref] = true
end

local checklist = {}
local findings = {
    drifts = {},
    missing_load = {},
    missing_apply = {},
    anomalies = {},
    orphans = {},
    ok_refs = {},
}

local counts = {
    total = 0,
    drift = 0,
    missing_load = 0,
    missing_apply = 0,
    anomalies = 0,
    orphans = 0,
    ok = 0,
}

local function add_drift(kind, ref, file, db_type, fields, expected_p, actual_p)
    findings.drifts[#findings.drifts + 1] = {
        kind = kind,
        ref = ref,
        file = file,
        db_type = db_type,
        fields = fields,
        expected = expected_p,
        actual = actual_p,
    }
end

for _, d in ipairs(disk) do
    local ref = d.ref
    local file = d.file
    local types = db[ref]
    local exp = expected[ref]
    local exp_fwd = pick_expected_forward(exp)

    local has_1000 = types and types[1000] ~= nil
    local has_1003 = types and types[1003] ~= nil
    local has_load = has_1000 or has_1003
    local has_apply = has_1003

    local load_s = yn(has_load)
    local apply_s = yn(has_apply)
    local load_match = "-"
    local apply_match = "-"
    local notes = {}
    local is_fail = false
    local is_anomaly = false
    local has_content_drift = false

    if has_1000 and has_1003 then
        notes[#notes + 1] = "both 1000+1003"
        is_anomaly = true
        findings.anomalies[#findings.anomalies + 1] = {
            kind = "both_1000_1003",
            ref = ref,
            file = file,
        }
    end

    if not has_load then
        notes[#notes + 1] = "on disk only"
        is_fail = true
        findings.missing_load[#findings.missing_load + 1] = {
            ref = ref,
            file = file,
            expected = exp_fwd,
        }
    end

    if has_load and not has_apply then
        notes[#notes + 1] = "loaded, not applied"
        is_fail = true
        findings.missing_apply[#findings.missing_apply + 1] = {
            ref = ref,
            file = file,
        }
    end

    if has_load then
        if not exp_fwd then
            load_match = "N"
            notes[#notes + 1] = "no expected forward"
            is_fail = true
            has_content_drift = true
        else
            local db_row, db_type = pick_db_forward(types)
            local ok, bad = normalize.match_payload(exp_fwd, db_row, mode)
            if ok then
                load_match = "Y"
            else
                load_match = "N"
                notes[#notes + 1] = "forward " .. table.concat(bad, "+") .. " drift"
                is_fail = true
                has_content_drift = true
                add_drift("forward_load", ref, file, db_type, bad, exp_fwd, db_row)
            end
        end
    end

    if has_apply then
        if not exp_fwd then
            apply_match = "N"
            is_fail = true
            has_content_drift = true
        else
            local db_row = types[1003]
            local ok, bad = normalize.match_payload(exp_fwd, db_row, mode)
            if ok then
                apply_match = "Y"
            else
                apply_match = "N"
                if not (load_match == "N" and not has_1000) then
                    notes[#notes + 1] = "applied " .. table.concat(bad, "+") .. " drift"
                end
                is_fail = true
                has_content_drift = true
                local already = false
                for _, dr in ipairs(findings.drifts) do
                    if dr.ref == ref and dr.db_type == 1003 then
                        already = true
                        break
                    end
                end
                if not already then
                    add_drift("forward_apply", ref, file, 1003, bad, exp_fwd, db_row)
                end
            end
        end
    end

    if include_reverse and has_load then
        local exp_r = exp and exp.by_type[1001] or nil
        local db_r = types and types[1001] or nil
        if exp_r and db_r then
            local ok, bad = normalize.match_payload(exp_r, db_r, mode)
            if not ok then
                notes[#notes + 1] = "reverse " .. table.concat(bad, "+") .. " drift"
                is_fail = true
                has_content_drift = true
                add_drift("reverse", ref, file, 1001, bad, exp_r, db_r)
            end
        elseif exp_r and not db_r then
            notes[#notes + 1] = "reverse missing in DB"
            is_fail = true
        end
    end

    if include_diagram and has_load then
        local exp_d = exp and exp.by_type[1002] or nil
        local db_d = types and types[1002] or nil
        if exp_d and db_d then
            local ok, bad = normalize.match_payload(exp_d, db_d, mode)
            if not ok then
                notes[#notes + 1] = "diagram " .. table.concat(bad, "+") .. " drift"
                is_fail = true
                has_content_drift = true
                add_drift("diagram", ref, file, 1002, bad, exp_d, db_d)
            end
        elseif exp_d and not db_d then
            notes[#notes + 1] = "diagram missing in DB"
            is_fail = true
        end
    end

    counts.total = counts.total + 1
    if is_anomaly then
        counts.anomalies = counts.anomalies + 1
    end
    if not has_load then
        counts.missing_load = counts.missing_load + 1
    elseif not has_apply then
        counts.missing_apply = counts.missing_apply + 1
    end
    if has_content_drift then
        counts.drift = counts.drift + 1
    end

    local row_ok = (not is_fail) and (not is_anomaly) and has_load and has_apply
        and load_match == "Y" and apply_match == "Y"
    if row_ok then
        counts.ok = counts.ok + 1
        findings.ok_refs[#findings.ok_refs + 1] = ref
    end

    if not (only_failures and row_ok) then
        checklist[#checklist + 1] = {
            ref = ref,
            file = file,
            load = load_s,
            load_match = load_match,
            apply = apply_s,
            apply_match = apply_match,
            notes = note_join(notes),
        }
    end
end

local orphan_refs = {}
for ref, _ in pairs(db) do
    if not disk_set[ref] then
        orphan_refs[#orphan_refs + 1] = ref
    end
end
table.sort(orphan_refs)

for _, ref in ipairs(orphan_refs) do
    local types = db[ref]
    counts.orphans = counts.orphans + 1
    counts.anomalies = counts.anomalies + 1
    local rows = {}
    for _, row in pairs(types) do
        rows[#rows + 1] = row
    end
    table.sort(rows, function(a, b)
        return a.query_type < b.query_type
    end)
    findings.orphans[#findings.orphans + 1] = {
        ref = ref,
        rows = rows,
    }
    checklist[#checklist + 1] = {
        ref = ref,
        file = "(orphan)",
        load = (types[1000] or types[1003]) and "Y" or "N",
        load_match = "-",
        apply = types[1003] and "Y" or "N",
        apply_match = "-",
        notes = "orphan DB ref (not on disk)",
    }
end

table.sort(checklist, function(a, b)
    return a.ref < b.ref
end)

local exit_code = 0
if counts.orphans > 0 then
    exit_code = 3
else
    for _, a in ipairs(findings.anomalies) do
        if a.kind == "both_1000_1003" then
            exit_code = 3
            break
        end
    end
end
if exit_code ~= 3
    and (counts.drift > 0 or counts.missing_load > 0 or counts.missing_apply > 0) then
    exit_code = 2
end

local function emit_checklist()
    local parts = { "[" }
    for idx, r in ipairs(checklist) do
        parts[#parts + 1] = string.format(
            '{"ref":%d,"file":%q,"load":%q,"load_match":%q,"apply":%q,"apply_match":%q,"notes":%q}',
            r.ref,
            r.file,
            r.load,
            r.load_match,
            r.apply,
            r.apply_match,
            r.notes
        )
        if idx < #checklist then
            parts[#parts + 1] = ","
        end
    end
    parts[#parts + 1] = "]"
    return table.concat(parts)
end

local function emit_findings()
    local buf = {}
    buf[#buf + 1] = string.format(
        '{"exit_code":%d,"normalize":"%s","counts":{"total":%d,"ok":%d,"drift":%d,'
            .. '"missing_load":%d,"missing_apply":%d,"anomalies":%d,"orphans":%d},',
        exit_code,
        mode,
        counts.total,
        counts.ok,
        counts.drift,
        counts.missing_load,
        counts.missing_apply,
        counts.anomalies,
        counts.orphans
    )

    buf[#buf + 1] = '"drifts":['
    for idx, d in ipairs(findings.drifts) do
        local fields = {}
        for _, f in ipairs(d.fields or {}) do
            fields[#fields + 1] = string.format("%q", f)
        end
        buf[#buf + 1] = string.format(
            '{"kind":%q,"ref":%d,"file":%q,"db_type":%d,"fields":[%s],"expected":%s,"actual":%s}',
            d.kind,
            d.ref,
            d.file,
            d.db_type or 0,
            table.concat(fields, ","),
            payload_json(d.expected),
            payload_json(d.actual)
        )
        if idx < #findings.drifts then
            buf[#buf + 1] = ","
        end
    end
    buf[#buf + 1] = "],"

    buf[#buf + 1] = '"missing_load":['
    for idx, m in ipairs(findings.missing_load) do
        buf[#buf + 1] = string.format(
            '{"ref":%d,"file":%q,"expected":%s}',
            m.ref,
            m.file,
            payload_json(m.expected)
        )
        if idx < #findings.missing_load then
            buf[#buf + 1] = ","
        end
    end
    buf[#buf + 1] = "],"

    buf[#buf + 1] = '"missing_apply":['
    for idx, m in ipairs(findings.missing_apply) do
        buf[#buf + 1] = string.format('{"ref":%d,"file":%q}', m.ref, m.file)
        if idx < #findings.missing_apply then
            buf[#buf + 1] = ","
        end
    end
    buf[#buf + 1] = "],"

    buf[#buf + 1] = '"anomalies":['
    for idx, a in ipairs(findings.anomalies) do
        buf[#buf + 1] = string.format(
            '{"kind":%q,"ref":%d,"file":%q}',
            a.kind,
            a.ref,
            a.file or ""
        )
        if idx < #findings.anomalies then
            buf[#buf + 1] = ","
        end
    end
    buf[#buf + 1] = "],"

    buf[#buf + 1] = '"orphans":['
    for idx, o in ipairs(findings.orphans) do
        buf[#buf + 1] = string.format('{"ref":%d,"rows":[', o.ref)
        for j, row in ipairs(o.rows) do
            buf[#buf + 1] = string.format(
                '{"query_ref":%d,"query_type":%d,"name":"%s","summary":"%s","code":"%s"}',
                row.query_ref,
                row.query_type,
                json_escape(row.name),
                json_escape(row.summary),
                json_escape(row.code)
            )
            if j < #o.rows then
                buf[#buf + 1] = ","
            end
        end
        buf[#buf + 1] = "]}"
        if idx < #findings.orphans then
            buf[#buf + 1] = ","
        end
    end
    buf[#buf + 1] = "],"

    buf[#buf + 1] = '"ok_refs":['
    for idx, r in ipairs(findings.ok_refs) do
        buf[#buf + 1] = tostring(r)
        if idx < #findings.ok_refs then
            buf[#buf + 1] = ","
        end
    end
    buf[#buf + 1] = "]}"

    return table.concat(buf)
end

-- silence unused helper warning path
local _ = parse_payload_line

write_all(checklist_out, emit_checklist() .. "\n")
write_all(findings_out, emit_findings() .. "\n")
cleanup_tmp()

io.stderr:write(string.format(
    "Compare: total=%d ok=%d drift=%d missing_load=%d missing_apply=%d orphans=%d exit=%d\n",
    counts.total,
    counts.ok,
    counts.drift,
    counts.missing_load,
    counts.missing_apply,
    counts.orphans,
    exit_code
))

os.exit(0)
