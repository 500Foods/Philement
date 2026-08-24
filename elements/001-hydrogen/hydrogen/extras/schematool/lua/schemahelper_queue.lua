-- schemahelper_queue.lua
-- Merge SchemaTool metadata + catalog JSON into a review queue.
--
--
-- CHANGELOG
-- 0.4.11 - 2026-08-23 - Decode brotli/base64 in compare; explore view
-- 0.4.10 - 2026-08-23 - Migration vs DB sides; ignore 1000→1003 apply
-- 0.4.9 - 2026-08-23 - Explore: field-level expected/actual diff
-- 0.4.0 - 2026-08-23 - Phase 4: packet refs on dashboard / review
-- 0.2.2 - 2026-08-23 - Phase 2: explore lines, decision persistence, cursor tracking
-- 0.2.0 - 2026-08-23 - Phase 1: findings ingest, sidecar decisions, dashboard totals

local M = {}

local brotli_mod
do
    local ok_b, mod = pcall(require, "brotli")
    if ok_b then
        brotli_mod = mod
    end
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

local function json_subobj(obj, subobj_key)
    local pat = '"' .. subobj_key .. '"%s*:%s*%{'
    local s, e = obj:find(pat)
    if not s then
        local arr_pat = '"' .. subobj_key .. '"%s*:%s*%['
        s, e = obj:find(arr_pat)
        if not s then
            return ""
        end
    end
    local i2 = e + 1
    local depth = 1
    local result = {}
    local in_string = false
    local escape = false
    while depth > 0 and i2 <= #obj do
        local c = obj:sub(i2, i2)
        if in_string then
            result[#result + 1] = c
            if escape then
                escape = false
            elseif c == "\\" then
                escape = true
            elseif c == '"' then
                in_string = false
            end
        else
            if c == '"' then
                in_string = true
                result[#result + 1] = c
            elseif c == "{" or c == "[" then
                depth = depth + 1
                result[#result + 1] = c
            elseif c == "}" or c == "]" then
                depth = depth - 1
                result[#result + 1] = c
            elseif c == "," or c == ":" or (c:match("%s")) then
                result[#result + 1] = c
            else
                result[#result + 1] = c
            end
        end
        i2 = i2 + 1
    end
    return table.concat(result)
end

local function payload_text(obj)
    local txt = json_string_field(obj, "detail")
    if txt == "" then
        txt = json_string_field(obj, "notes")
    end
    return txt
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
        local st_kind = json_string_field(obj, "kind")
        if st_kind == "" then
            st_kind = "drift"
        end
        add_finding(findings, {
            id = string.format("meta:drift:%d:%d:%s", ref, db_type, field),
            class = "metadata content drift",
            kind = st_kind,
            ref = ref,
            db_type = db_type,
            file = json_string_field(obj, "file"),
            summary = (db_type == 1003)
                and string.format("APPLY check ref %d — %s mismatch", ref, field)
                or string.format("LOAD check ref %d type %d — %s mismatch",
                    ref, db_type, field),
            expected = json_subobj(obj, "expected"),
            actual = json_subobj(obj, "actual"),
            detail = payload_text(obj),
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
            detail = payload_text(obj),
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
            detail = payload_text(obj),
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
            rec.note = json_string_field(obj, "note")
            rec.hash = json_string_field(obj, "hash")
            rec.packet = json_string_field(obj, "packet")
            rec.ref = json_num_field(obj, "ref")
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

local function find_by_id(findings, id)
    for _, f in ipairs(findings) do
        if f.id == id then
            return f
        end
    end
    return nil
end

function M.find_finding(findings, id)
    return find_by_id(findings, id)
end

local function load_detail_section(out_dir, finding)
    if not finding then
        return {}
    end
    local detail_path
    if finding.kind == "drift" or finding.class == "metadata content drift" then
        detail_path = out_dir .. "/finding_detail.txt"
    elseif finding.kind == "orphan" then
        detail_path = out_dir .. "/finding_detail.txt"
    elseif finding.kind == "missing_load" then
        detail_path = out_dir .. "/finding_detail.txt"
    elseif finding.kind == "missing_apply" then
        detail_path = out_dir .. "/finding_detail.txt"
    elseif finding.kind == "anomaly" then
        detail_path = out_dir .. "/finding_detail.txt"
    else
        detail_path = out_dir .. "/catalog_finding_detail.txt"
    end
    local custom = detail_path
    if finding.kind == "drift" or finding.kind == "orphan" or
       finding.kind == "missing_load" or finding.kind == "missing_apply" or
       finding.kind == "anomaly" then
        local ref_str = tostring(finding.ref or 0)
        local kind_str = finding.kind or "meta"
        custom = out_dir .. "/finding_detail_" .. kind_str .. "_" .. ref_str .. ".txt"
    end
    if not file_exists(custom) then
        custom = detail_path
    end
    if not file_exists(custom) then
        return {}
    end
    local lines = {}
    local f = io.open(custom, "r")
    if not f then
        return {}
    end
    for line in f:lines() do
        lines[#lines + 1] = line
    end
    f:close()
    return lines
end

function M.load_detail_section(out_dir, finding)
    return load_detail_section(out_dir, finding)
end

local function clip_text(s, n)
    s = tostring(s or ""):gsub("\n", "\\n"):gsub("\r", "\\r")
    if #s > n then
        return s:sub(1, n) .. "…"
    end
    return s
end

local function first_diff_at(a, b)
    a = a or ""
    b = b or ""
    local n = math.min(#a, #b)
    for i = 1, n do
        if a:byte(i) ~= b:byte(i) then
            return i
        end
    end
    if #a ~= #b then
        return n + 1
    end
    return nil
end

local function base64_decode(data)
    local b = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"
    data = tostring(data or ""):gsub("%s", ""):gsub("[^" .. b .. "=]", "")
    return (data:gsub(".", function(x)
        if x == "=" then
            return ""
        end
        local r, f = "", (b:find(x, 1, true) - 1)
        for i = 6, 1, -1 do
            r = r .. (f % 2 ^ i - f % 2 ^ (i - 1) > 0 and "1" or "0")
        end
        return r
    end):gsub("%d%d%d?%d?%d?%d?%d?%d?", function(x)
        if #x ~= 8 then
            return ""
        end
        local c = 0
        for i = 1, 8 do
            c = c + (x:sub(i, i) == "1" and 2 ^ (8 - i) or 0)
        end
        return string.char(c)
    end))
end

local function decode_brotli_b64(b64)
    local raw = base64_decode(b64)
    if raw == "" then
        return nil
    end
    if not brotli_mod then
        return nil
    end
    local ok, dec = pcall(brotli_mod.decompress, raw)
    if ok and type(dec) == "string" then
        return dec
    end
    return nil
end

local function decode_embedded(s)
    s = tostring(s or "")
    s = s:gsub(
        "BROTLI_DECOMPRESS%s*%(%s*CRYPTO_DECODE%s*%(%s*'([^']+)'[^)]*%)%s*%)",
        function(b64)
            return decode_brotli_b64(b64) or "«brotli decode failed»"
        end)
    s = s:gsub(
        "BROTLI_DECOMPRESS%s*%(%s*FROM_BASE64%s*%(%s*'([^']+)'%s*%)%s*%)",
        function(b64)
            return decode_brotli_b64(b64) or "«brotli decode failed»"
        end)
    s = s:gsub(
        "CRYPTO_DECODE%s*%(%s*'([^']+)'[^)]*%)",
        function(b64)
            local raw = base64_decode(b64)
            if raw == "" then
                return "«base64 decode failed»"
            end
            return raw
        end)
    return s
end

function M.decode_embedded(s)
    return decode_embedded(s)
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

local function sides_of(finding)
    local left = finding.expected or ""
    local right = finding.actual or ""
    local lname = "Migration"
    local rname = "Database"
    if finding.live and finding.live ~= "" then
        right = finding.live
        lname = "Expected"
        rname = "Live"
    end
    return left, right, lname, rname
end

local function is_apply_promotion(a, b)
    return (a == "1000" and b == "1003") or (a == "1003" and b == "1000")
end

local function pad_clip(s, w)
    s = tostring(s or "")
    if w < 1 then
        return ""
    end
    if #s > w then
        return s:sub(1, w)
    end
    return s .. string.rep(" ", w - #s)
end

local function wrap_hard(s, w)
    local out = {}
    s = tostring(s or "")
    if w < 8 then
        w = 8
    end
    if s == "" then
        out[1] = ""
        return out
    end
    while #s > w do
        out[#out + 1] = s:sub(1, w)
        s = s:sub(w + 1)
    end
    out[#out + 1] = s
    return out
end

local function explain_check(finding)
    local lines = {}
    if finding.live and finding.live ~= "" then
        lines[#lines + 1] = "  check:     catalog expected vs live object"
        return lines
    end
    local file = finding.file
    if not file or file == "" then
        file = "(migration)"
    end
    local ref = tostring(finding.ref or "?")
    if finding.db_type == 1003 then
        lines[#lines + 1] = "  check:     APPLY — migration vs applied queries row"
        lines[#lines + 1] = "  migration: " .. file
        lines[#lines + 1] = "  database:  queries  ref=" .. ref .. "  type=1003 (applied)"
        lines[#lines + 1] = "  compared:  code, name, summary"
        lines[#lines + 1] = "  ignored:   query_type 1000→1003 is APPLY promotion, not a defect"
    elseif finding.db_type == 1000 then
        lines[#lines + 1] = "  check:     LOAD — migration vs loaded queries row"
        lines[#lines + 1] = "  migration: " .. file
        lines[#lines + 1] = "  database:  queries  ref=" .. ref .. "  type=1000 (loaded)"
        lines[#lines + 1] = "  compared:  code, name, summary"
    else
        lines[#lines + 1] = "  left:      migration / expected"
        lines[#lines + 1] = "  right:     database / actual"
    end
    return lines
end

local function focus_pair(left, right, width)
    local at = first_diff_at(left, right) or 1
    local col = math.max(20, math.floor((width - 3) / 2))
    local radius = math.max(8, col - 2)
    local lo = math.max(1, at - math.floor(radius / 3))
    local function win(s)
        local hi = math.min(#s, lo + radius - 1)
        local chunk = s:sub(lo, hi)
        if lo > 1 then
            chunk = "…" .. chunk
        end
        if hi < #s then
            chunk = chunk .. "…"
        end
        return chunk
    end
    local caret = at - lo + 1
    if lo > 1 then
        caret = caret + 1
    end
    if caret < 1 then
        caret = 1
    elseif caret > col then
        caret = col
    end
    return {
        pad_clip(win(left), col) .. " │ " .. pad_clip(win(right), col),
        pad_clip(string.rep(" ", caret - 1) .. "^", col)
            .. " │ "
            .. pad_clip(string.rep(" ", caret - 1) .. "^", col),
    }
end

local function payload_map(blob)
    if not blob or blob == "" then
        return nil
    end
    local qt = json_num_field(blob, "query_type")
    local name = json_string_field(blob, "name")
    local summary = decode_embedded(json_string_field(blob, "summary"))
    local code = decode_embedded(json_string_field(blob, "code"))
    if not qt and name == "" and summary == "" and code == "" then
        return nil
    end
    return {
        { "query_type", qt and tostring(qt) or "" },
        { "name", name },
        { "summary", summary },
        { "code", code },
    }
end

local function line_diff_lines(label, left, right, lname, rname, max_lines, width)
    local a = split_lines(left)
    local e = split_lines(right)
    local maxn = math.max(#a, #e)
    local changed = {}
    for n = 1, maxn do
        if (a[n] or "") ~= (e[n] or "") then
            changed[#changed + 1] = n
        end
    end
    local out = {}
    width = width or 100
    if #changed == 0 then
        if left == right then
            out[#out + 1] = "  " .. label .. ": identical"
        else
            local at = first_diff_at(left, right)
            out[#out + 1] = string.format(
                "  %s: same line-count, differ at byte %s",
                label, tostring(at or "?"))
            local pair = focus_pair(left, right, math.max(40, width - 4))
            out[#out + 1] = "    " .. pad_clip(lname, 10) .. " │ " .. rname
            for _, row in ipairs(pair) do
                out[#out + 1] = "    " .. row
            end
        end
        return out
    end
    local context = 1
    local show = {}
    for _, n in ipairs(changed) do
        for d = -context, context do
            local idx = n + d
            if idx >= 1 and idx <= maxn then
                show[idx] = true
            end
        end
    end
    out[#out + 1] = string.format(
        "  %s: %d differing line(s) of %d",
        label, #changed, maxn)
    out[#out + 1] = "    " .. pad_clip(lname, math.max(16, math.floor((width - 7) / 2)))
        .. " │ " .. rname
    local printed = 0
    local prev = 0
    max_lines = max_lines or 80
    local col = math.max(16, math.floor((width - 7) / 2))
    for n = 1, maxn do
        if show[n] then
            if prev > 0 and n > prev + 1 then
                out[#out + 1] = "    …"
            end
            local al = a[n]
            local el = e[n]
            if al == nil then
                out[#out + 1] = string.format("    %4d only in %s", n, rname)
                for _, w in ipairs(wrap_hard(el or "", width - 8)) do
                    out[#out + 1] = "         " .. w
                end
            elseif el == nil then
                out[#out + 1] = string.format("    %4d only in %s", n, lname)
                for _, w in ipairs(wrap_hard(al, width - 8)) do
                    out[#out + 1] = "         " .. w
                end
            elseif al ~= el then
                local at = first_diff_at(al, el)
                out[#out + 1] = string.format(
                    "    line %d  differ at byte %s", n, tostring(at or "?"))
                for _, row in ipairs(focus_pair(al, el, width - 4)) do
                    out[#out + 1] = "    " .. row
                end
            else
                out[#out + 1] = string.format(
                    "    %4d %s", n, clip_text(al, col))
            end
            printed = printed + 1
            prev = n
            if printed >= max_lines then
                out[#out + 1] = "    … diff truncated"
                break
            end
        end
    end
    return out
end

local function compare_lines(finding, mode, width)
    local lines = {}
    width = width or 100
    local left, right, lname, rname = sides_of(finding)
    if left == "" and right == "" then
        return lines
    end
    if left == right then
        lines[#lines + 1] = "  " .. lname .. " and " .. rname .. " are identical"
        return lines
    end
    local lp = payload_map(left)
    local rp = payload_map(right)
    if lp and rp then
        for i = 1, #lp do
            local key = lp[i][1]
            local lv = lp[i][2] or ""
            local rv = rp[i][2] or ""
            if key == "query_type" and is_apply_promotion(lv, rv) then
                if mode == "full" then
                    lines[#lines + 1] = string.format(
                        "  query_type: %s (migration) → %s (applied)  — ignored",
                        lv, rv)
                end
            elseif lv == rv then
                if mode == "full" and lv ~= "" and key ~= "query_type" then
                    lines[#lines + 1] = "  " .. key .. ": identical"
                end
            elseif mode == "short" then
                if key == "code" or key == "summary" then
                    local nchg = 0
                    local a = split_lines(lv)
                    local b = split_lines(rv)
                    local maxn = math.max(#a, #b)
                    for n = 1, maxn do
                        if (a[n] or "") ~= (b[n] or "") then
                            nchg = nchg + 1
                        end
                    end
                    lines[#lines + 1] = string.format(
                        "  %s: %d of %d lines differ  (Migration vs Database)",
                        key, nchg, maxn)
                else
                    lines[#lines + 1] = string.format(
                        "  %s:  Migration=%s", key, clip_text(lv, 40))
                    lines[#lines + 1] = string.format(
                        "  %s   Database =%s",
                        string.rep(" ", #key), clip_text(rv, 40))
                end
            else
                local part = line_diff_lines(
                    key, lv, rv, lname, rname, 80, width)
                for _, row in ipairs(part) do
                    lines[#lines + 1] = row
                end
            end
        end
        return lines
    end
    if mode == "short" or (#left <= 80 and #right <= 80) then
        lines[#lines + 1] = "  " .. lname .. ": " .. clip_text(left, 72)
        lines[#lines + 1] = "  " .. rname .. ": " .. clip_text(right, 72)
        return lines
    end
    local part = line_diff_lines("value", left, right, lname, rname, 80, width)
    for _, row in ipairs(part) do
        lines[#lines + 1] = row
    end
    return lines
end

function M.build_explore_view(finding)
    local facts = {}
    if finding and finding.id then
        facts[#facts + 1] = finding.id
    end
    if finding and finding.file and finding.file ~= "" then
        facts[#facts + 1] = "file  " .. finding.file
    end
    if finding then
        for _, r in ipairs(explain_check(finding)) do
            facts[#facts + 1] = r:gsub("^%s+", "")
        end
    end
    local rows = {}
    if not finding then
        return { facts = facts, rows = rows }
    end
    local left, right = sides_of(finding)
    local lp = payload_map(left)
    local rp = payload_map(right)
    if not lp or not rp then
        local a = split_lines(decode_embedded(left))
        local b = split_lines(decode_embedded(right))
        local maxn = math.max(#a, #b)
        for n = 1, maxn do
            rows[#rows + 1] = {
                kind = "pair",
                n = n,
                left = a[n] or "",
                right = b[n] or "",
                same = (a[n] or "") == (b[n] or ""),
            }
        end
        return { facts = facts, rows = rows }
    end
    for i = 1, #lp do
        local key = lp[i][1]
        local lv = lp[i][2] or ""
        local rv = rp[i][2] or ""
        if key == "query_type" and is_apply_promotion(lv, rv) then
            facts[#facts + 1] = "query_type 1000→1003 ignored (APPLY)"
        elseif lv == rv then
            if lv ~= "" and key ~= "query_type" then
                facts[#facts + 1] = key .. ": identical"
            end
        else
            local a = split_lines(lv)
            local b = split_lines(rv)
            local maxn = math.max(#a, #b)
            local changed = {}
            for n = 1, maxn do
                if (a[n] or "") ~= (b[n] or "") then
                    changed[#changed + 1] = n
                end
            end
            rows[#rows + 1] = {
                kind = "label",
                text = string.format(
                    "%s — %d of %d lines differ", key, #changed, maxn),
            }
            local context = 2
            local show = {}
            for _, n in ipairs(changed) do
                for d = -context, context do
                    local idx = n + d
                    if idx >= 1 and idx <= maxn then
                        show[idx] = true
                    end
                end
            end
            local prev = 0
            for n = 1, maxn do
                if show[n] then
                    if prev > 0 and n > prev + 1 then
                        rows[#rows + 1] = { kind = "label", text = "…" }
                    end
                    rows[#rows + 1] = {
                        kind = "pair",
                        n = n,
                        left = a[n] or "",
                        right = b[n] or "",
                        same = (a[n] or "") == (b[n] or ""),
                    }
                    prev = n
                end
            end
        end
    end
    return { facts = facts, rows = rows }
end

local function note_for_state(id, state)
    if not id or not state then
        return ""
    end
    local rec = state.by_id and state.by_id[id]
    if rec and rec.note and rec.note ~= "" then
        return rec.note
    end
    return ""
end

function M.note_for(finding_id, out_dir, state)
    if not finding_id then
        return ""
    end
    if state then
        return note_for_state(finding_id, state)
    end
    local loaded = M.load_state(out_dir and M.default_state_path(
        out_dir, "", "") or "")
    return note_for_state(finding_id, loaded)
end

local function build_finding_lines(finding, _, state)
    local lines = {}
    lines[#lines + 1] = "id:       " .. finding.id
    lines[#lines + 1] = "class:    " .. finding.class
    if finding.summary and finding.summary ~= "" then
        lines[#lines + 1] = "summary:  " .. finding.summary
    end
    if finding.file and finding.file ~= "" then
        lines[#lines + 1] = "file:     " .. finding.file
    end
    if finding.ref then
        lines[#lines + 1] = "ref:      " .. finding.ref
    end
    if finding.expected and finding.expected ~= "" then
        if finding.live and finding.live ~= "" then
            lines[#lines + 1] = "expected: " .. finding.expected
            lines[#lines + 1] = "live:     " .. finding.live
        else
            lines[#lines + 1] = "expected: " .. finding.expected
        end
    end
    if finding.actual and finding.actual ~= "" and
        (not finding.live or finding.live == "") then
        lines[#lines + 1] = "actual:   " .. finding.actual
    end
    local note = note_for_state(finding.id, state)
    if note and note ~= "" then
        lines[#lines + 1] = "operator note: " .. note
    end
    return lines
end

local function json_explore_lines(finding, out_dir, state, width)
    local lines = {}
    lines[#lines + 1] = "=== Exploration: " .. finding.id .. " ==="
    lines[#lines + 1] = ""
    local flines = build_finding_lines(finding, out_dir, state)
    for _, l in ipairs(flines) do
        if not l:match("^expected:") and not l:match("^live:")
            and not l:match("^actual:") then
            lines[#lines + 1] = l
        end
    end
    lines[#lines + 1] = ""
    for _, row in ipairs(explain_check(finding)) do
        lines[#lines + 1] = row
    end
    lines[#lines + 1] = ""
    lines[#lines + 1] = "--- Migration vs Database ---"
    local diffs = compare_lines(finding, "full", width)
    if #diffs == 0 then
        lines[#lines + 1] = "(no expected/actual payload on this finding)"
    else
        for _, l in ipairs(diffs) do
            lines[#lines + 1] = l
        end
    end
    if finding.detail and finding.detail ~= "" then
        lines[#lines + 1] = ""
        lines[#lines + 1] = "--- Notes ---"
        for line in (finding.detail .. "\n"):gmatch("(.-)\n") do
            lines[#lines + 1] = line
        end
    end
    lines[#lines + 1] = ""
    lines[#lines + 1] = "Press q or Esc to return"
    return lines
end

function M.json_explore_lines(finding, out_dir, state, width)
    if not finding then
        return { "No finding selected" }
    end
    return json_explore_lines(finding, out_dir, state, width)
end

local function filter_detail_for_finding(all, finding)
    if not finding or #all == 0 then
        return {}
    end
    local ref = finding.ref
    local id = finding.id or ""
    local needle = id
    if (not needle or needle == "") and ref then
        needle = "ref=" .. tostring(ref)
    end
    if not needle or needle == "" then
        return {}
    end
    local out = {}
    local capture = false
    for _, line in ipairs(all) do
        local hit = line:find(needle, 1, true)
            or (ref and line:find("ref=" .. tostring(ref), 1, true))
        if hit and not capture then
            capture = true
        elseif capture and (
            line:match("^DRIFT  ref=")
            or line:match("^ORPHAN")
            or line:match("^MISSING")
            or line:match("^Finding:")
        ) then
            break
        end
        if capture then
            out[#out + 1] = line
        end
    end
    return out
end

function M.explore_lines(out_dir, finding_id, findings, state, width)
    if not finding_id then
        return { "No finding selected" }
    end
    local finding = nil
    if findings then
        finding = find_by_id(findings, finding_id)
    end
    if not finding then
        return { "Finding not found: " .. finding_id }
    end
    local lines = json_explore_lines(finding, out_dir, state, width)
    local detail = filter_detail_for_finding(
        load_detail_section(out_dir, finding), finding)
    if #detail > 0 then
        if lines[#lines] == "Press q or Esc to return" then
            lines[#lines] = nil
        end
        lines[#lines + 1] = "--- SchemaTool detail ---"
        for _, l in ipairs(detail) do
            lines[#lines + 1] = l
        end
        lines[#lines + 1] = ""
        lines[#lines + 1] = "Press q or Esc to return"
    end
    return lines
end

function M.json_subobj(obj, subobj_key)
    return json_subobj(obj, subobj_key)
end

function M.payload_text(obj)
    return payload_text(obj)
end

function M.load_metadata(path, tmp_dir, findings)
    return load_metadata(path, tmp_dir, findings)
end

local function jq_update_state(state_file, update_filter)
    if not file_exists(state_file) then
        return false, "state file not found: " .. state_file
    end
    local tmp_dir = (os.getenv("TMPDIR") or "/tmp")
        .. "/schemahelper_u_"
        .. tostring(os.time())
        .. "_"
        .. tostring(math.random(100000))
    os.execute('mkdir -p "' .. tmp_dir .. '"')
    local fpath = tmp_dir .. "/u.jq"
    local f = io.open(fpath, "w")
    if not f then
        os.execute('rm -rf "' .. tmp_dir .. '"')
        return false, "cannot write jq filter"
    end
    f:write(update_filter .. "\n")
    f:close()
    local cmd = string.format(
        'jq -S -f "%s" "%s" > "%s.tmp" && mv "%s.tmp" "%s"',
        fpath, state_file, state_file, state_file, state_file
    )
    local ok, why, code = os.execute(cmd)
    os.execute('rm -rf "' .. tmp_dir .. '"')
    if ok == true then
        return true
    end
    if why == "exit" then
        return false, "jq update failed with exit code " .. tostring(code)
    end
    return false, "jq update failed"
end

function M.jq_update_state(state_file, update_filter)
    return jq_update_state(state_file, update_filter)
end

function M.save_cursor(state_file, finding_id)
    if not finding_id then
        return false, "no finding_id provided"
    end
    local esc_id = finding_id:gsub('"', '\\"')
    local filter = string.format('.cursor_id = "%s" | .updated_utc = "%s"',
        esc_id, os.date("!%Y-%m-%dT%H:%M:%SZ"))
    return jq_update_state(state_file, filter)
end

function M.save_decision(state_file, finding_id, action, extra)
    if not finding_id or not action then
        return false, "finding_id and action are required"
    end
    local esc_id = finding_id:gsub('"', '\\"')
    local now = os.date("!%Y-%m-%dT%H:%M:%SZ")
    local parts = {}
    parts[#parts + 1] = string.format('.cursor_id = "%s"', esc_id)
    parts[#parts + 1] = string.format('.updated_utc = "%s"', now)
    local dec_fields = {}
    dec_fields[#dec_fields + 1] = string.format('"id": "%s"', esc_id)
    dec_fields[#dec_fields + 1] = string.format('"action": "%s"', action)
    if extra then
        if extra.hash and extra.hash ~= "" then
            local esc_hash = extra.hash:gsub('"', '\\"')
            dec_fields[#dec_fields + 1] = string.format('"hash": "%s"', esc_hash)
        end
        if extra.note and extra.note ~= "" then
            local esc_note = extra.note:gsub('"', '\\"'):gsub("\n", "\\n")
            dec_fields[#dec_fields + 1] = string.format('"note": "%s"', esc_note)
        end
        if extra.ref then
            dec_fields[#dec_fields + 1] = string.format('"ref": %d', extra.ref)
        end
        if extra.packet and extra.packet ~= "" then
            local esc_pkt = extra.packet:gsub('"', '\\"')
            dec_fields[#dec_fields + 1] = string.format('"packet": "%s"', esc_pkt)
        end
    end
    dec_fields[#dec_fields + 1] = string.format('"at": "%s"', now)
    local dec_json = '{' .. table.concat(dec_fields, ", ") .. '}'
    local filter = string.format(
        '.cursor_id = "%s" | .updated_utc = "%s" | '
      .. '.decisions |= (map(select(.id != "%s")) + [%s])',
        esc_id, now, esc_id, dec_json
    )
    table.insert(parts, filter)
    local combined = table.concat(parts, " | ")
    return jq_update_state(state_file, combined)
end

function M.build_dashboard_lines(opts)
    local out_dir = opts.out_dir or "."
    local track = opts.track or "both"
    local state = opts.state or { by_id = {} }
    local built = M.build({
        out_dir = out_dir,
        track = track,
        state = state,
    })
    local lines = {}
    lines[#lines + 1] = string.format("Total migrations found      %d", built.totals.total)
    lines[#lines + 1] = string.format("Perfect migrations          %d", built.totals.perfect)
    lines[#lines + 1] = string.format("Accepted variations         %d", built.totals.accepted)
    lines[#lines + 1] = string.format("Subject for review          %d", built.totals.subject)
    if built.totals.applied > 0 or built.totals.packet > 0 then
        lines[#lines + 1] = string.format("Applied / packets           %d / %d",
            built.totals.applied, built.totals.packet)
    end
    local reserved = opts.reserved or {}
    if #reserved > 0 then
        lines[#lines + 1] = ""
        lines[#lines + 1] = "Reserved packet refs"
        for i = 1, #reserved do
            local item = reserved[i]
            lines[#lines + 1] = string.format("  %-6s %s",
                tostring(item.ref or "?"),
                item.name or item.path or "")
        end
    end
    lines[#lines + 1] = ""
    lines[#lines + 1] = "Variance classes (subject for review)"
    if #built.classes == 0 then
        lines[#lines + 1] = "  (none)"
    else
        for i = 1, #built.classes do
            local c = built.classes[i]
            lines[#lines + 1] = string.format("  %-28s %d", c.name, c.count)
        end
    end
    return lines, built
end

local function g_label(next_ref, g_reason)
    if g_reason and g_reason ~= "" then
        return "  [g] generate a migration       (disabled — " .. g_reason .. ")"
    end
    if next_ref then
        return "  [g] generate a migration       (next ref " .. tostring(next_ref) .. ")"
    end
    return "  [g] generate a migration"
end

function M.build_review_lines(finding, next_ref, g_reason)
    local lines = {}
    lines[#lines + 1] = "This is the variance"
    lines[#lines + 1] = "  id:       " .. finding.id
    lines[#lines + 1] = "  class:    " .. finding.class
    if finding.ref then
        lines[#lines + 1] = "  ref:      " .. finding.ref
    end
    if finding.summary and finding.summary ~= "" then
        lines[#lines + 1] = "  note:     " .. finding.summary
    end
    for _, row in ipairs(explain_check(finding)) do
        lines[#lines + 1] = row
    end
    local diffs = compare_lines(finding, "short")
    for _, row in ipairs(diffs) do
        lines[#lines + 1] = row
    end
    lines[#lines + 1] = ""
    lines[#lines + 1] = "What would you like to do?"
    lines[#lines + 1] = "  [e] explore in more detail"
    lines[#lines + 1] = "  [s] skip for now"
    lines[#lines + 1] = "  [a] accept permanent variance"
    lines[#lines + 1] = "  [u] apply to database          (disabled — Phase 5)"
    lines[#lines + 1] = g_label(next_ref, g_reason)
    lines[#lines + 1] = "  [n]ext  [p]rev  [r]e-audit  [q]uit to dashboard"
    return lines
end

function M.build_review_lines_detailed(finding, out_dir, state, next_ref, g_reason)
    local lines = {}
    lines[#lines + 1] = "This is the variance"
    lines[#lines + 1] = "  id:       " .. finding.id
    lines[#lines + 1] = "  class:    " .. finding.class
    if finding.ref then
        lines[#lines + 1] = "  ref:      " .. finding.ref
    end
    if finding.file and finding.file ~= "" then
        lines[#lines + 1] = "  file:     " .. finding.file
    end
    if finding.summary and finding.summary ~= "" then
        lines[#lines + 1] = "  note:     " .. finding.summary
    end
    for _, row in ipairs(explain_check(finding)) do
        lines[#lines + 1] = row
    end
    local diffs = compare_lines(finding, "short")
    for _, row in ipairs(diffs) do
        lines[#lines + 1] = row
    end
    local note = note_for_state(finding.id, state)
    if note and note ~= "" then
        lines[#lines + 1] = "  operator: " .. note
    end
    lines[#lines + 1] = ""
    lines[#lines + 1] = "What would you like to do?"
    lines[#lines + 1] = "  [e] explore in more detail"
    lines[#lines + 1] = "  [s] skip for now"
    lines[#lines + 1] = "  [a] accept permanent variance"
    lines[#lines + 1] = "  [u] apply to database          (disabled — Phase 5)"
    lines[#lines + 1] = g_label(next_ref, g_reason)
    lines[#lines + 1] = "  [n]ext  [p]rev  [r]e-audit  [q]uit to dashboard"
    return lines
end

return M
