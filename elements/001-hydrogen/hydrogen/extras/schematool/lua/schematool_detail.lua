-- schematool_detail.lua
-- Human-readable finding details after the checklist table: field diffs +
-- commented remediation SQL (DB actual → Lua expected).
--
-- Usage:
--   lua schematool_detail.lua --findings PATH --engine E [--schema S]
--     [--qualified T] [--max-lines N] [--context N]
--
-- Catalog mode:
--   lua schematool_detail.lua --catalog-findings PATH [--max-lines N]
--
-- CHANGELOG
-- 1.0.0 - 2026-08-06 - Post-table drift detail (diff + commented UPDATE)

-- luacheck: globals arg

local findings_path, catalog_path
local engine, schema, qualified = "postgresql", "", "queries"
local max_lines = 80
local context = 3

local i = 1
while i <= #arg do
    local a = arg[i]
    if a == "--findings" then
        findings_path = arg[i + 1]
        i = i + 2
    elseif a == "--catalog-findings" then
        catalog_path = arg[i + 1]
        i = i + 2
    elseif a == "--engine" then
        engine = arg[i + 1] or engine
        i = i + 2
    elseif a == "--schema" then
        schema = arg[i + 1] or ""
        i = i + 2
    elseif a == "--qualified" then
        qualified = arg[i + 1] or qualified
        i = i + 2
    elseif a == "--max-lines" then
        max_lines = tonumber(arg[i + 1]) or max_lines
        i = i + 2
    elseif a == "--context" then
        context = tonumber(arg[i + 1]) or context
        i = i + 2
    else
        io.stderr:write("Error: unknown argument: " .. tostring(a) .. "\n")
        os.exit(1)
    end
end

if not findings_path and not catalog_path then
    io.stderr:write(
        "Usage: lua schematool_detail.lua --findings P --engine E | --catalog-findings P\n"
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
    .. "/schematool_detail_"
    .. tostring(os.time())
    .. "_"
    .. tostring(math.random(100000))
os.execute('mkdir -p "' .. TMP_DIR .. '"')

local function cleanup_tmp()
    os.execute('rm -rf "' .. TMP_DIR .. '"')
end

local jq_seq = 0
local function jq_raw(filter, path)
    jq_seq = jq_seq + 1
    local fpath = TMP_DIR .. "/f" .. tostring(jq_seq) .. ".jq"
    write_all(fpath, filter .. "\n")
    local cmd = 'jq -c -f "' .. fpath .. '" "' .. path:gsub('"', '\\"') .. '"'
    local h = io.popen(cmd)
    if not h then
        return nil
    end
    local data = h:read("*a")
    h:close()
    if data then
        data = data:gsub("%s+$", "")
    end
    return data
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

local function strip_json_string(s)
    if not s or s == "null" then
        return ""
    end
    if s:sub(1, 1) == '"' then
        return json_string_field('{"x":' .. s .. "}", "x")
    end
    return s
end

local function parse_payload(obj)
    if not obj or obj == "null" or obj == "" then
        return nil
    end
    return {
        query_type = json_num_field(obj, "query_type"),
        name = json_string_field(obj, "name"),
        summary = json_string_field(obj, "summary"),
        code = json_string_field(obj, "code"),
    }
end

local function dollar_quote(body, tag)
    tag = tag or "schematool"
    local t = tag
    local n = 0
    while body:find("%$" .. t .. "%$", 1, true) do
        n = n + 1
        t = tag .. tostring(n)
    end
    return "$" .. t .. "$" .. body .. "$" .. t .. "$"
end

local function sql_string_literal(body)
    return "'" .. tostring(body or ""):gsub("'", "''") .. "'"
end

local function format_code_literal(body)
    if engine == "postgresql" then
        return dollar_quote(body or "", "schematool")
    end
    return sql_string_literal(body)
end

local function comment_sql_lines(sql_text)
    local out = {}
    for line in (sql_text .. "\n"):gmatch("(.-)\n") do
        out[#out + 1] = "-- " .. line
    end
    return table.concat(out, "\n")
end

local function split_lines(text)
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

-- Unified-ish diff: mark changed lines with context (no external diff binary).
local function format_diff(label, actual, expected)
    local a = split_lines(actual)
    local e = split_lines(expected)
    local maxn = math.max(#a, #e)
    local changed = {}
    for n = 1, maxn do
        if (a[n] or "") ~= (e[n] or "") then
            changed[#changed + 1] = n
        end
    end
    if #changed == 0 then
        return string.format("  (%s: no line-level difference after normalize — check whitespace/type)", label)
    end

    local show = {}
    local function mark(n)
        show[n] = true
        for d = 1, context do
            if n - d >= 1 then
                show[n - d] = true
            end
            if n + d <= maxn then
                show[n + d] = true
            end
        end
    end
    for _, n in ipairs(changed) do
        mark(n)
    end

    local out = {}
    out[#out + 1] = string.format("  --- DB actual (%s)  +++ Lua expected (%s)", label, label)
    local printed = 0
    local prev = 0
    for n = 1, maxn do
        if show[n] then
            if prev > 0 and n > prev + 1 then
                out[#out + 1] = "  @@@ … @@@"
            end
            local al = a[n]
            local el = e[n]
            if al == nil then
                out[#out + 1] = string.format("  +%4d|%s", n, el or "")
            elseif el == nil then
                out[#out + 1] = string.format("  -%4d|%s", n, al or "")
            elseif al ~= el then
                out[#out + 1] = string.format("  -%4d|%s", n, al)
                out[#out + 1] = string.format("  +%4d|%s", n, el)
            else
                out[#out + 1] = string.format("   %4d|%s", n, al)
            end
            printed = printed + 1
            prev = n
            if printed >= max_lines then
                out[#out + 1] = string.format(
                    "  … diff truncated at %d lines (use --max-lines or full .sql artifact)",
                    max_lines
                )
                break
            end
        end
    end
    out[#out + 1] = string.format(
        "  (%d differing line(s) of %d max(actual,expected))",
        #changed,
        maxn
    )
    return table.concat(out, "\n")
end

local function rule(ch, n)
    return string.rep(ch, n or 78)
end

local function emit(s)
    io.write(s)
    if s:sub(-1) ~= "\n" then
        io.write("\n")
    end
end

---------------------------------------------------------------------------
-- Catalog findings detail
---------------------------------------------------------------------------
if catalog_path then
    local exit_code = tonumber(jq_raw(".exit_code", catalog_path)) or 0
    local nfail = tonumber(jq_raw(
        '[.rows[]? | select(.status != "Y")] | length',
        catalog_path
    )) or 0
    -- fallback: checklist array at root
    if nfail == 0 then
        nfail = tonumber(jq_raw('[.[]? | select(.status != "Y")] | length', catalog_path)) or 0
    end

    if exit_code == 0 and nfail == 0 then
        cleanup_tmp()
        os.exit(0)
    end

    emit("")
    emit(rule("="))
    emit("Catalog finding details (live vs expected net shape)")
    emit("  - = live DB    + = expected (folded applied DDL)")
    emit("  Prefer a NEW forward migration for live DDL fixes — do not hand-ALTER blindly.")
    emit(rule("="))

    local rows_filter = '[.rows[]? | select(.status != "Y")]'
    local n = tonumber(jq_raw(rows_filter .. " | length", catalog_path)) or 0
    local base_path = rows_filter
    if n == 0 then
        base_path = '[.[]? | select(.status != "Y")]'
        n = tonumber(jq_raw(base_path .. " | length", catalog_path)) or 0
    end

    for idx = 0, n - 1 do
        local base = string.format("%s[%d]", base_path, idx)
        local obj = strip_json_string(jq_raw(base .. ".object", catalog_path))
        local col = strip_json_string(jq_raw(base .. ".column", catalog_path))
        local check = strip_json_string(jq_raw(base .. ".check", catalog_path))
        local status = strip_json_string(jq_raw(base .. ".status", catalog_path))
        local expected = strip_json_string(jq_raw(base .. ".expected", catalog_path))
        local live = strip_json_string(jq_raw(base .. ".live", catalog_path))
        local notes = strip_json_string(jq_raw(base .. ".notes", catalog_path))

        emit("")
        emit(rule("-"))
        emit(string.format(
            "FAIL  %s.%s  check=%s  status=%s",
            obj ~= "" and obj or "?",
            col ~= "" and col or "(table)",
            check ~= "" and check or "?",
            status
        ))
        if notes ~= "" then
            emit("  notes: " .. notes)
        end
        emit(string.format("  live:     %s", live))
        emit(string.format("  expected: %s", expected))

        if check == "nullable" then
            local want_null = (expected == "true" or expected == "YES" or expected == "1")
            if want_null then
                emit("  guidance: live column should allow NULL (e.g. DROP NOT NULL / ALTER … NULL).")
            else
                emit("  guidance: live column should be NOT NULL (e.g. ALTER … SET NOT NULL) — data may block.")
            end
            emit("  remediation: author a new numbered migration; do not edit historical Lua in place.")
        elseif check == "presence" or check == "table" or check == "column" then
            emit("  guidance: create missing object via new forward migration (Hydrogen LOAD/APPLY).")
        end
    end

    emit("")
    emit(rule("="))
    cleanup_tmp()
    os.exit(0)
end

---------------------------------------------------------------------------
-- Metadata findings detail
---------------------------------------------------------------------------
local exit_code = tonumber(jq_raw(".exit_code", findings_path)) or 0
local cnt_drift = tonumber(jq_raw(".counts.drift", findings_path)) or 0
local cnt_mload = tonumber(jq_raw(".counts.missing_load", findings_path)) or 0
local cnt_mapply = tonumber(jq_raw(".counts.missing_apply", findings_path)) or 0
local cnt_anom = tonumber(jq_raw(".counts.anomalies", findings_path)) or 0
local cnt_orph = tonumber(jq_raw(".counts.orphans", findings_path)) or 0

if exit_code == 0 and cnt_drift == 0 and cnt_mload == 0 and cnt_mapply == 0
    and cnt_anom == 0 and cnt_orph == 0 then
    cleanup_tmp()
    os.exit(0)
end

local qtable = qualified
if schema and schema ~= "" and schema ~= "." and engine ~= "sqlite" then
    qtable = schema .. "." .. qualified
end

emit("")
emit(rule("="))
emit("Finding details (metadata track)")
emit("  Legend:  - DB actual (stored queries row)   + Lua expected (disk migration)")
emit("  Goal: bring DB metadata into compliance with current Lua (does NOT replay DDL).")
emit("  Prefer NEW forward migration when live schema must change.")
emit(rule("="))

-- Drifts
local nd = tonumber(jq_raw(".drifts | length", findings_path)) or 0
for idx = 0, nd - 1 do
    local base = string.format(".drifts[%d]", idx)
    local kind = strip_json_string(jq_raw(base .. ".kind", findings_path))
    local ref = tonumber(jq_raw(base .. ".ref", findings_path)) or 0
    local file = strip_json_string(jq_raw(base .. ".file", findings_path))
    local db_type = tonumber(jq_raw(base .. ".db_type", findings_path)) or 1003
    local fields_json = jq_raw(base .. ".fields", findings_path) or "[]"
    local fields = {}
    for name in fields_json:gmatch('"(.-)"') do
        fields[#fields + 1] = name
    end
    local exp = parse_payload(jq_raw(base .. ".expected", findings_path))
    local act = parse_payload(jq_raw(base .. ".actual", findings_path))

    emit("")
    emit(rule("-"))
    emit(string.format(
        "DRIFT  ref=%d  file=%s  kind=%s  db_type=%d  fields=%s",
        ref,
        file ~= "" and file or "?",
        kind ~= "" and kind or "?",
        db_type,
        #fields > 0 and table.concat(fields, "+") or "code"
    ))
    emit(string.format(
        "  meaning: stored type-%d payload does not match Lua (%s)",
        db_type,
        file
    ))

    local fieldset = {}
    for _, f in ipairs(fields) do
        fieldset[f] = true
    end
    if not next(fieldset) then
        fieldset.code = true
    end

    for _, fname in ipairs({ "name", "summary", "code" }) do
        if fieldset[fname] and exp and act then
            emit("")
            emit(format_diff(fname, act[fname] or "", exp[fname] or ""))
        end
    end

    if exp then
        emit("")
        emit("  Commented remediation (NOT EXECUTED) — UPDATE DB → Lua expected:")
        local sets = {}
        if fieldset.code or not next(fieldset) then
            sets[#sets + 1] = "code = " .. format_code_literal(exp.code)
        end
        if fieldset.name then
            sets[#sets + 1] = "name = " .. format_code_literal(exp.name)
        end
        if fieldset.summary then
            sets[#sets + 1] = "summary = " .. format_code_literal(exp.summary)
        end
        if #sets == 0 then
            sets[#sets + 1] = "code = " .. format_code_literal(exp.code)
        end
        local upd = string.format(
            "UPDATE %s\n   SET %s\n WHERE query_ref = %d\n   AND query_type_a28 = %d;",
            qtable,
            table.concat(sets, ",\n       "),
            ref,
            db_type
        )
        emit(comment_sql_lines(upd))
        emit("  NOTE: this aligns queries metadata only; live product tables are unchanged.")
    end
end

-- Missing LOAD
local nml = tonumber(jq_raw(".missing_load | length", findings_path)) or 0
for idx = 0, nml - 1 do
    local base = string.format(".missing_load[%d]", idx)
    local ref = tonumber(jq_raw(base .. ".ref", findings_path)) or 0
    local file = strip_json_string(jq_raw(base .. ".file", findings_path))
    emit("")
    emit(rule("-"))
    emit(string.format("MISSING LOAD  ref=%d  file=%s", ref, file))
    emit("  meaning: on disk, no type 1000/1003 row in queries.")
    emit("  remediation: run Hydrogen AutoMigration (LOAD then APPLY) — do not hand-paste DDL.")
end

-- Missing APPLY
local nma = tonumber(jq_raw(".missing_apply | length", findings_path)) or 0
for idx = 0, nma - 1 do
    local base = string.format(".missing_apply[%d]", idx)
    local ref = tonumber(jq_raw(base .. ".ref", findings_path)) or 0
    local file = strip_json_string(jq_raw(base .. ".file", findings_path))
    emit("")
    emit(rule("-"))
    emit(string.format("MISSING APPLY  ref=%d  file=%s", ref, file))
    emit("  meaning: type 1000 present, no type 1003.")
    emit("  remediation: run Hydrogen APPLY through this ref.")
end

-- Anomalies
local na = tonumber(jq_raw(".anomalies | length", findings_path)) or 0
for idx = 0, na - 1 do
    local base = string.format(".anomalies[%d]", idx)
    local kind = strip_json_string(jq_raw(base .. ".kind", findings_path))
    local ref = tonumber(jq_raw(base .. ".ref", findings_path)) or 0
    local file = strip_json_string(jq_raw(base .. ".file", findings_path))
    emit("")
    emit(rule("-"))
    emit(string.format("ANOMALY  ref=%d  kind=%s  file=%s", ref, kind, file))
    if kind == "both_1000_1003" then
        emit("  meaning: both loaded (1000) and applied (1003) rows exist for same ref.")
        emit("  guidance: keep 1003; remove stray 1000 if APPLY already succeeded.")
        local del = string.format(
            "DELETE FROM %s\n WHERE query_ref = %d\n   AND query_type_a28 = 1000;",
            qtable,
            ref
        )
        emit(comment_sql_lines(del))
    end
end

-- Orphans
local no = tonumber(jq_raw(".orphans | length", findings_path)) or 0
for idx = 0, no - 1 do
    local base = string.format(".orphans[%d]", idx)
    local ref = tonumber(jq_raw(base .. ".ref", findings_path)) or 0
    emit("")
    emit(rule("-"))
    emit(string.format("ORPHAN  ref=%d  (in DB, not on disk)", ref))
    emit("  meaning: DB has migration metadata with no matching design_NNNN.lua.")
    emit("  remediation: review .mig capture; author new Lua if keeping; else commented DELETE in .sql.")
    local del = string.format(
        "DELETE FROM %s\n WHERE query_ref = %d\n   AND query_type_a28 BETWEEN 1000 AND 1003;",
        qtable,
        ref
    )
    emit(comment_sql_lines(del))
end

emit("")
emit(rule("="))
emit(string.format(
    "End detail · drifts=%d missing_load=%d missing_apply=%d anomalies=%d orphans=%d · exit=%d",
    cnt_drift,
    cnt_mload,
    cnt_mapply,
    cnt_anom,
    cnt_orph,
    exit_code
))
emit(rule("="))

cleanup_tmp()
os.exit(0)
