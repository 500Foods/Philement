-- schematool_remediate.lua
-- Emit fully commented remediation .sql and plain-text orphan .mig from findings.
--
-- Usage:
--   lua schematool_remediate.lua \
--     --findings PATH \
--     --sql-out PATH \
--     [--mig-out PATH] \
--     --design NAME --engine ENGINE --schema NAME --database NAME \
--     --migrations PATH --normalize MODE --version VER --stamp UTC \
--     [--include-ok-comments]
--
-- CHANGELOG
-- 1.0.0 - 2026-07-29 - Phase 4 remediation artifacts

-- luacheck: globals arg

local findings_path, sql_out, mig_out
local design, engine, schema, database, migrations
local normalize_mode, version, stamp
local include_ok = false
local qualified = "queries"

local i = 1
while i <= #arg do
    local a = arg[i]
    if a == "--findings" then
        findings_path = arg[i + 1]
        i = i + 2
    elseif a == "--sql-out" then
        sql_out = arg[i + 1]
        i = i + 2
    elseif a == "--mig-out" then
        mig_out = arg[i + 1]
        i = i + 2
    elseif a == "--design" then
        design = arg[i + 1]
        i = i + 2
    elseif a == "--engine" then
        engine = arg[i + 1]
        i = i + 2
    elseif a == "--schema" then
        schema = arg[i + 1]
        i = i + 2
    elseif a == "--database" then
        database = arg[i + 1]
        i = i + 2
    elseif a == "--migrations" then
        migrations = arg[i + 1]
        i = i + 2
    elseif a == "--normalize" then
        normalize_mode = arg[i + 1]
        i = i + 2
    elseif a == "--version" then
        version = arg[i + 1]
        i = i + 2
    elseif a == "--stamp" then
        stamp = arg[i + 1]
        i = i + 2
    elseif a == "--qualified" then
        qualified = arg[i + 1]
        i = i + 2
    elseif a == "--include-ok-comments" then
        include_ok = true
        i = i + 1
    else
        io.stderr:write("Error: unknown argument: " .. tostring(a) .. "\n")
        os.exit(1)
    end
end

if not findings_path or not sql_out or not design or not engine then
    io.stderr:write(
        "Usage: lua schematool_remediate.lua --findings P --sql-out P "
        .. "--design N --engine E [options]\n"
    )
    os.exit(1)
end

schema = schema or ""
database = database or ""
migrations = migrations or ""
normalize_mode = normalize_mode or "loose"
version = version or "0"
stamp = stamp or ""

local function write_all(path, data)
    local f, err = io.open(path, "wb")
    if not f then
        io.stderr:write("Error: cannot write " .. path .. ": " .. tostring(err) .. "\n")
        os.exit(1)
    end
    f:write(data)
    f:close()
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

local TMP_DIR = (os.getenv("TMPDIR") or "/tmp")
    .. "/schematool_rem_"
    .. tostring(os.time())
    .. "_"
    .. tostring(math.random(100000))
os.execute('mkdir -p "' .. TMP_DIR .. '"')
local jq_seq = 0

local function cleanup_tmp()
    os.execute('rm -rf "' .. TMP_DIR .. '"')
end

local function jq_raw(filter, path)
    -- jq -f avoids shell expanding $ in filters; do not wrap filter in quotes with $
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

local function strip_json_string(s)
    if not s or s == "null" then
        return ""
    end
    if s:sub(1, 1) == '"' then
        return json_string_field('{"x":' .. s .. "}", "x")
    end
    return s
end

local function parse_payload_from_obj(obj)
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

local exit_code = tonumber(jq_raw(".exit_code", findings_path)) or 0
local counts_raw = jq_raw(".counts", findings_path) or "{}"

local drifts = {}
do
    local n = tonumber(jq_raw(".drifts | length", findings_path)) or 0
    for idx = 0, n - 1 do
        local base = string.format(".drifts[%d]", idx)
        local kind = strip_json_string(jq_raw(base .. ".kind", findings_path))
        local ref = tonumber(jq_raw(base .. ".ref", findings_path))
        local file = strip_json_string(jq_raw(base .. ".file", findings_path))
        local db_type = tonumber(jq_raw(base .. ".db_type", findings_path))
        local fields_json = jq_raw(base .. ".fields", findings_path) or "[]"
        local fields = {}
        for name in fields_json:gmatch('"(.-)"') do
            fields[#fields + 1] = name
        end
        local exp_line = jq_raw(base .. ".expected", findings_path)
        local act_line = jq_raw(base .. ".actual", findings_path)
        drifts[#drifts + 1] = {
            kind = kind,
            ref = ref,
            file = file,
            db_type = db_type,
            fields = fields,
            expected = parse_payload_from_obj(exp_line),
            actual = parse_payload_from_obj(act_line),
        }
    end
end

local missing_load = {}
do
    local n = tonumber(jq_raw(".missing_load | length", findings_path)) or 0
    for idx = 0, n - 1 do
        local base = string.format(".missing_load[%d]", idx)
        missing_load[#missing_load + 1] = {
            ref = tonumber(jq_raw(base .. ".ref", findings_path)),
            file = strip_json_string(jq_raw(base .. ".file", findings_path)),
        }
    end
end

local missing_apply = {}
do
    local n = tonumber(jq_raw(".missing_apply | length", findings_path)) or 0
    for idx = 0, n - 1 do
        local base = string.format(".missing_apply[%d]", idx)
        missing_apply[#missing_apply + 1] = {
            ref = tonumber(jq_raw(base .. ".ref", findings_path)),
            file = strip_json_string(jq_raw(base .. ".file", findings_path)),
        }
    end
end

local anomalies = {}
do
    local n = tonumber(jq_raw(".anomalies | length", findings_path)) or 0
    for idx = 0, n - 1 do
        local base = string.format(".anomalies[%d]", idx)
        anomalies[#anomalies + 1] = {
            kind = strip_json_string(jq_raw(base .. ".kind", findings_path)),
            ref = tonumber(jq_raw(base .. ".ref", findings_path)),
            file = strip_json_string(jq_raw(base .. ".file", findings_path)),
        }
    end
end

local orphans = {}
do
    local n = tonumber(jq_raw(".orphans | length", findings_path)) or 0
    for idx = 0, n - 1 do
        local base = string.format(".orphans[%d]", idx)
        local ref = tonumber(jq_raw(base .. ".ref", findings_path))
        local rows = {}
        local rn = tonumber(jq_raw(base .. ".rows | length", findings_path)) or 0
        for j = 0, rn - 1 do
            local rb = string.format("%s.rows[%d]", base, j)
            local row_json = jq_raw(rb, findings_path)
            if row_json then
                rows[#rows + 1] = {
                    query_ref = json_num_field(row_json, "query_ref"),
                    query_type = json_num_field(row_json, "query_type"),
                    name = json_string_field(row_json, "name"),
                    summary = json_string_field(row_json, "summary"),
                    code = json_string_field(row_json, "code"),
                }
            end
        end
        orphans[#orphans + 1] = { ref = ref, rows = rows }
    end
end

local ok_refs = {}
do
    local raw = jq_raw(".ok_refs", findings_path) or "[]"
    for n in raw:gmatch("(%d+)") do
        ok_refs[#ok_refs + 1] = tonumber(n)
    end
end

local qtable = qualified
if schema and schema ~= "" and schema ~= "." and engine ~= "sqlite" then
    qtable = schema .. "." .. qualified
end

local sql = {}
local function add(line)
    sql[#sql + 1] = line
end

add("-- =============================================================================")
add("-- SchemaTool remediation (NOT EXECUTED)")
add("-- schematool " .. version .. " · phase 4 audit")
add("-- design=" .. design .. " engine=" .. engine
    .. " schema=" .. (schema ~= "" and schema or ".")
    .. " database=" .. (database ~= "" and database or "none"))
add("-- migrations=" .. migrations)
add("-- normalize=" .. normalize_mode)
add("-- Generated: " .. stamp)
add("-- counts: " .. (counts_raw:gsub("\n", " ")))
add("-- exit_code=" .. tostring(exit_code))
add("-- Rule: Uncomment deliberately. Prefer Hydrogen LOAD/APPLY when possible.")
add("-- Updating queries.code does NOT replay DDL against live tables.")
add("-- Prefer a NEW forward migration when live schema must change.")
add("-- Content match scope (v1): code + name + summary.")
add("-- Orphan DB refs: captured to .mig for optional new-migration rebuild.")
add("-- =============================================================================")
add("--")

for _, d in ipairs(drifts) do
    local exp = d.expected
    if exp then
        add("-- ---------------------------------------------------------------------------")
        add(string.format(
            "-- Ref %d: %s drift (type %s) fields=%s",
            d.ref or 0,
            d.kind or "?",
            tostring(d.db_type or "?"),
            table.concat(d.fields or {}, "+")
        ))
        add(string.format("-- Disk: %s", d.file or "?"))
        add("-- NOTE: metadata-only fix. Does not replay DDL. Prefer new migration if schema wrong.")
        add("-- ---------------------------------------------------------------------------")
        local sets = {}
        local fieldset = {}
        for _, f in ipairs(d.fields or {}) do
            fieldset[f] = true
        end
        if fieldset.code or #(d.fields or {}) == 0 then
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
            sets[#sets + 1] = "name = " .. format_code_literal(exp.name)
            sets[#sets + 1] = "summary = " .. format_code_literal(exp.summary)
        end
        local upd = string.format(
            "UPDATE %s\n   SET %s\n WHERE query_ref = %d\n   AND query_type_a28 = %d;",
            qtable,
            table.concat(sets, ",\n       "),
            d.ref,
            d.db_type or 1003
        )
        add(comment_sql_lines(upd))
        add("--")
    end
end

for _, m in ipairs(missing_load) do
    add("-- ---------------------------------------------------------------------------")
    add(string.format("-- Ref %d: missing LOAD and APPLY", m.ref or 0))
    add(string.format("-- Disk: %s", m.file or "?"))
    add("-- Prefer: start Hydrogen with AutoMigration against this DB (LOAD then APPLY).")
    add("-- Do not hand-expand full DDL here (risk of double-apply on partial schemas).")
    add("-- ---------------------------------------------------------------------------")
    add("--")
end

for _, m in ipairs(missing_apply) do
    add("-- ---------------------------------------------------------------------------")
    add(string.format("-- Ref %d: loaded (type 1000), not applied (no type 1003)", m.ref or 0))
    add(string.format("-- Disk: %s", m.file or "?"))
    add("-- Prefer: run Hydrogen APPLY through this ref (AutoMigration / lead APPLY).")
    add("-- ---------------------------------------------------------------------------")
    add("--")
end

for _, a in ipairs(anomalies) do
    if a.kind == "both_1000_1003" then
        add("-- ---------------------------------------------------------------------------")
        add(string.format("-- Ref %d: ANOMALY both type 1000 and 1003 present", a.ref or 0))
        add(string.format("-- Disk: %s", a.file or "?"))
        add("-- Guidance: keep applied (1003); remove stray loaded (1000) if APPLY already done.")
        add("-- ---------------------------------------------------------------------------")
        local del = string.format(
            "DELETE FROM %s\n WHERE query_ref = %d\n   AND query_type_a28 = 1000;",
            qtable,
            a.ref
        )
        add(comment_sql_lines(del))
        add("--")
    end
end

for _, o in ipairs(orphans) do
    add("-- ---------------------------------------------------------------------------")
    add(string.format("-- Ref %d: ORPHAN (in DB, not on disk) — see .mig capture", o.ref or 0))
    add("-- DANGEROUS if live objects depend on this migration. Review .mig before DELETE.")
    add("-- ---------------------------------------------------------------------------")
    local del = string.format(
        "DELETE FROM %s\n WHERE query_ref = %d\n   AND query_type_a28 BETWEEN 1000 AND 1003;",
        qtable,
        o.ref
    )
    add(comment_sql_lines(del))
    add("--")
end

if include_ok then
    for _, r in ipairs(ok_refs) do
        add(string.format("-- OK: %d", r))
    end
end

if #drifts == 0 and #missing_load == 0 and #missing_apply == 0
    and #anomalies == 0 and #orphans == 0 then
    add("-- No remediation blocks (audit clean for selected refs).")
    add("--")
end

write_all(sql_out, table.concat(sql, "\n") .. "\n")

local function lint_sql(path)
    local f = io.open(path, "r")
    if not f then
        return false, "cannot read"
    end
    local bad = {}
    local n = 0
    for line in f:lines() do
        n = n + 1
        local trimmed = line:match("^%s*(.-)%s*$") or ""
        if trimmed ~= "" and not trimmed:match("^%-%-") then
            bad[#bad + 1] = n
        end
    end
    f:close()
    if #bad > 0 then
        return false, "uncommented lines: " .. table.concat(bad, ",")
    end
    return true
end

local ok_lint, lint_err = lint_sql(sql_out)
if not ok_lint then
    io.stderr:write("Error: remediation SQL lint failed: " .. tostring(lint_err) .. "\n")
    os.exit(1)
end

if mig_out and #orphans > 0 then
    local mig = {}
    local function madd(line)
        mig[#mig + 1] = line
    end
    madd("================================================================================")
    madd("SchemaTool orphan capture (.mig) — NOT a runnable migration")
    madd("schematool " .. version)
    madd("design=" .. design .. " engine=" .. engine
        .. " schema=" .. (schema ~= "" and schema or ".")
        .. " database=" .. (database ~= "" and database or "none"))
    madd("Generated: " .. stamp)
    madd("Purpose: review DB-only migration rows; author a new design_NNNN.lua if keeping.")
    madd("================================================================================")
    madd("")
    for _, o in ipairs(orphans) do
        madd("--------------------------------------------------------------------------------")
        madd(string.format("ORPHAN REF %d", o.ref or 0))
        madd("--------------------------------------------------------------------------------")
        for _, row in ipairs(o.rows) do
            madd(string.format("type: %s", tostring(row.query_type)))
            madd("name:")
            madd(row.name or "")
            madd("summary:")
            madd(row.summary or "")
            madd("code:")
            madd(row.code or "")
            madd("")
        end
    end
    write_all(mig_out, table.concat(mig, "\n") .. "\n")
    io.stderr:write("MIG: " .. mig_out .. " (" .. tostring(#orphans) .. " orphan refs)\n")
elseif mig_out then
    write_all(
        mig_out,
        "-- SchemaTool orphan capture: no orphans for this run (" .. stamp .. ")\n"
    )
end

cleanup_tmp()
io.stderr:write("SQL: " .. sql_out .. " (all statements commented)\n")
os.exit(0)
