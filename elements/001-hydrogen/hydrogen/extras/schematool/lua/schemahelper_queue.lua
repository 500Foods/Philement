-- schemahelper_queue.lua
-- Merge SchemaTool metadata + catalog JSON into a review queue.
-- Orchestrator: delegates pure helpers / state / load / decode to sibling
-- modules under lua/ (schemahelper_qutil, _qstate, _qload, _qdecode).
--
-- CHANGELOG
-- 0.5.8 - 2026-08-25 - Split into lua/ submodules (qutil/qstate/qload/qdecode); this file is now the orchestrator
-- 0.5.7 - 2026-08-24 - Decode MySQL/MariaDB lowercase brotli_decompress(FROM_BASE64('...'))
-- 0.5.6 - 2026-08-24 - Decode DB2 brotli+base64; PostgreSQL brotli_decompress + CONVERT_FROM(DECODE)
-- 0.5.5 - 2026-08-24 - Phase 7: u_label + explain_check for catalog DDL apply
-- 0.5.4 - 2026-08-24 - Phase 5 slice: orphan [U] label + explain_check branch
-- 0.5.1 - 2026-08-23 - [U] is update; catalog findings show last fold ref
-- 0.5.0 - 2026-08-23 - Phase 5: review [U] reason (one-field apply)
-- 0.4.14 - 2026-08-23 - Explore Enter decodes highlighted brotli line
-- 0.4.13 - 2026-08-23 - One finding per field; decoded brotli is its own item
-- 0.4.11 - 2026-08-23 - Decode brotli/base64 in compare; explore view
-- 0.4.10 - 2026-08-23 - Migration vs DB sides; ignore 1000→1003 apply
-- 0.4.9 - 2026-08-23 - Explore: field-level expected/actual diff
-- 0.4.0 - 2026-08-23 - Phase 4: packet refs on dashboard / review
-- 0.2.2 - 2026-08-23 - Phase 2: explore lines, decision persistence, cursor tracking
-- 0.2.0 - 2026-08-23 - Phase 1: findings ingest, sidecar decisions, dashboard totals

local U = require("schemahelper_qutil")
local S = require("schemahelper_qstate")
local L = require("schemahelper_qload")
local D = require("schemahelper_qdecode")

local M = {}

-- Forward references satisfied by sibling modules.
local has_embed = U.has_embed
local split_lines = U.split_lines

function M.default_state_path(out_dir, design, engine)
    return S.default_state_path(out_dir, design, engine)
end

function M.load_state(path)
    return S.load_state(path)
end

function M.create_state(path, design, engine, schema)
    return S.create_state(path, design, engine, schema)
end

function M.artifacts_present(out_dir, track)
    return S.artifacts_present(out_dir, track)
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
        meta_counts = L.load_metadata(out_dir .. "/findings.json", tmp, all)
    end
    if track == "catalog" or track == "both" then
        cat_counts = L.load_catalog(out_dir .. "/catalog_findings.json", tmp, all)
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
    if not U.file_exists(custom) then
        custom = detail_path
    end
    if not U.file_exists(custom) then
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

function M.decode_embedded(s)
    return D.decode_embedded(s)
end

function M.has_embed(s)
    return U.has_embed(s)
end

function M.build_line_decode_view(left_line, right_line)
    return D.build_line_decode_view(left_line, right_line)
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

local function explain_check(finding)
    local lines = {}
    if finding.object and finding.object ~= "" then
        lines[#lines + 1] = "  check:     catalog expected vs live object"
        lines[#lines + 1] = "  table:     " .. finding.object
        if finding.column and finding.column ~= "" and finding.column ~= "-" then
            lines[#lines + 1] = "  column:    " .. finding.column
        end
        if finding.ref then
            lines[#lines + 1] = "  migration: last fold ref "
                .. tostring(finding.ref)
                .. " (expected shape)"
        else
            lines[#lines + 1] = "  migration: (fold did not record a ref)"
        end
        if finding.expected and finding.expected ~= "" then
            lines[#lines + 1] = "  expected:  " .. finding.expected
        end
        if finding.live and finding.live ~= "" then
            lines[#lines + 1] = "  live:      " .. finding.live
        end
        if finding.kind == "nullable" then
            local want_null = (finding.expected == "true"
                or finding.expected == "YES" or finding.expected == "1")
            if want_null then
                lines[#lines + 1] = "  apply:     [U]pdate Database — ALTER COLUMN DROP NOT NULL"
            else
                lines[#lines + 1] = "  apply:     [U]pdate Database — ALTER COLUMN SET NOT NULL"
            end
        elseif finding.kind == "column" then
            lines[#lines + 1] = "  apply:     [U]pdate Database — ADD COLUMN (type from expected fold)"
        end
        return lines
    end
    local file = finding.file
    if not file or file == "" then
        file = "(migration)"
    end
    local ref = tostring(finding.ref or "?")
    if finding.kind == "orphan" then
        lines[#lines + 1] = "  check:     orphan — ref in DB, absent from disk"
        lines[#lines + 1] = "  migration: (no design_" .. ref .. ".lua on disk)"
        lines[#lines + 1] = "  database:  queries  ref=" .. ref
            .. "  type 1000/1003 (loaded/applied)"
        lines[#lines + 1] = "  action:    [U]pdate Database — deletes ref rows (BETWEEN 1000 AND 1003)"
        return lines
    end
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
    local at = U.first_diff_at(left, right) or 1
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
        U.pad_clip(win(left), col) .. " │ " .. U.pad_clip(win(right), col),
        U.pad_clip(string.rep(" ", caret - 1) .. "^", col)
            .. " │ "
            .. U.pad_clip(string.rep(" ", caret - 1) .. "^", col),
    }
end

local function payload_map(blob)
    if not blob or blob == "" then
        return nil
    end
    local qt = U.json_num_field(blob, "query_type")
    local name = U.json_string_field(blob, "name")
    local summary = D.decode_embedded(U.json_string_field(blob, "summary"))
    local code = D.decode_embedded(U.json_string_field(blob, "code"))
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
            local at = U.first_diff_at(left, right)
            out[#out + 1] = string.format(
                "  %s: same line-count, differ at byte %s",
                label, tostring(at or "?"))
            local pair = focus_pair(left, right, math.max(40, width - 4))
            out[#out + 1] = "    " .. U.pad_clip(lname, 10) .. " │ " .. rname
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
    out[#out + 1] = "    " .. U.pad_clip(lname, math.max(16, math.floor((width - 7) / 2)))
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
                for _, w in ipairs(U.wrap_hard(el or "", width - 8)) do
                    out[#out + 1] = "         " .. w
                end
            elseif el == nil then
                out[#out + 1] = string.format("    %4d only in %s", n, lname)
                for _, w in ipairs(U.wrap_hard(al, width - 8)) do
                    out[#out + 1] = "         " .. w
                end
            elseif al ~= el then
                local at = U.first_diff_at(al, el)
                out[#out + 1] = string.format(
                    "    line %d  differ at byte %s", n, tostring(at or "?"))
                for _, row in ipairs(focus_pair(al, el, width - 4)) do
                    out[#out + 1] = "    " .. row
                end
            else
                out[#out + 1] = string.format(
                    "    %4d %s", n, U.clip_text(al, col))
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

local function field_pair(finding)
    local left, right, lname, rname = sides_of(finding)
    local field = finding.field
    local view = finding.view or "raw"
    local lp = payload_map(left)
    local rp = payload_map(right)
    if lp and rp and field and field ~= "" then
        local lv, rv = "", ""
        for i = 1, #lp do
            if lp[i][1] == field then
                lv = lp[i][2] or ""
                rv = rp[i][2] or ""
                break
            end
        end
        if view == "raw" then
            lv = U.payload_raw(left, field)
            rv = U.payload_raw(right, field)
        end
        return lv, rv, lname, rname, field, view
    end
    if view == "decoded" then
        return D.decode_embedded(left), D.decode_embedded(right), lname, rname,
            field or "value", view
    end
    return left, right, lname, rname, field or "value", view
end

local function compare_lines(finding, mode, width)
    local lines = {}
    width = width or 100
    local lv, rv, lname, rname, field, view = field_pair(finding)
    if lv == "" and rv == "" then
        local left, right = sides_of(finding)
        if left == "" and right == "" then
            return lines
        end
    end
    local label = field
    if view == "decoded" then
        label = field .. " (decoded)"
    elseif has_embed(lv) or has_embed(rv) then
        label = field .. " (encoded)"
    end
    if lv == rv then
        lines[#lines + 1] = "  " .. label .. ": identical"
        return lines
    end
    if mode == "short" then
        if field == "code" or field == "summary" or view == "decoded"
            or #lv > 80 or #rv > 80 then
            local a = split_lines(lv)
            local b = split_lines(rv)
            local maxn = math.max(#a, #b)
            local nchg = 0
            for n = 1, maxn do
                if (a[n] or "") ~= (b[n] or "") then
                    nchg = nchg + 1
                end
            end
            lines[#lines + 1] = string.format(
                "  %s: %d of %d lines differ  (Migration vs Database)",
                label, nchg, maxn)
        else
            lines[#lines + 1] = string.format(
                "  %s:  Migration=%s", label, U.clip_text(lv, 40))
            lines[#lines + 1] = string.format(
                "  %s   Database =%s",
                string.rep(" ", #label), U.clip_text(rv, 40))
        end
        return lines
    end
    local part = line_diff_lines(label, lv, rv, lname, rname, 80, width)
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
    if finding and finding.field and finding.field ~= "" then
        local view = finding.view or "raw"
        if view == "decoded" then
            facts[#facts + 1] = "field  " .. finding.field .. "  (decoded)"
        else
            facts[#facts + 1] = "field  " .. finding.field
        end
    end
    if finding then
        for _, r in ipairs(explain_check(finding)) do
            facts[#facts + 1] = r:gsub("^%s+", "")
        end
    end
    local rows = {}
    if not finding then
        return { facts = facts, rows = rows, first_diff = 0 }
    end
    local lv, rv, _, _, field, view = field_pair(finding)
    local a = split_lines(lv)
    local b = split_lines(rv)
    local maxn = math.max(#a, #b)
    local nchg = 0
    local first_diff = 0
    for n = 1, maxn do
        if (a[n] or "") ~= (b[n] or "") then
            nchg = nchg + 1
            if first_diff == 0 then
                first_diff = n
            end
        end
    end
    local label = field or "value"
    if view == "decoded" then
        label = label .. " (decoded)"
    elseif has_embed(lv) or has_embed(rv) then
        label = label .. " (encoded)"
    end
    if maxn == 0 then
        rows[#rows + 1] = {
            kind = "label",
            text = label .. " — empty on both sides",
        }
        return { facts = facts, rows = rows, first_diff = 0 }
    end
    rows[#rows + 1] = {
        kind = "label",
        text = string.format("%s — %d of %d lines differ", label, nchg, maxn),
    }
    for n = 1, maxn do
        rows[#rows + 1] = {
            kind = "pair",
            n = n,
            left = a[n] or "",
            right = b[n] or "",
            same = (a[n] or "") == (b[n] or ""),
        }
    end
    return { facts = facts, rows = rows, first_diff = first_diff }
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
    return U.json_subobj(obj, subobj_key)
end

function M.payload_text(obj)
    return U.payload_text(obj)
end

function M.load_metadata(path, tmp_dir, findings)
    return L.load_metadata(path, tmp_dir, findings)
end

function M.jq_update_state(state_file, update_filter)
    return S.jq_update_state(state_file, update_filter)
end

function M.save_cursor(state_file, finding_id)
    return S.save_cursor(state_file, finding_id)
end

function M.save_decision(state_file, finding_id, action, extra)
    return S.save_decision(state_file, finding_id, action, extra)
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
    lines[#lines + 1] = string.format("Findings for review         %d", built.totals.subject)
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
    lines[#lines + 1] = "Variance classes (findings for review)"
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
        return "  [G]enerate Migration       (disabled — " .. g_reason .. ")"
    end
    if next_ref then
        return "  [G]enerate Migration       (next ref " .. tostring(next_ref) .. ")"
    end
    return "  [G]enerate Migration"
end

local function promote_label(finding, state, allow_write)
    if not allow_write then
        return "  [M] Promote packet to Helium   (disabled — need --allow-write)"
    end
    local id = finding and finding.id
    local rec = state and state.by_id and state.by_id[id]
    if rec and rec.action == "packet" and rec.ref then
        return "  [M] Promote packet to Helium   (packet ref "
            .. tostring(rec.ref) .. ")"
    end
    return "  [M] Promote packet to Helium   (no packet — generate with [G] first)"
end

local function u_label(u_reason, finding)
    if u_reason and u_reason ~= "" then
        return "  [U]pdate Database            (disabled — " .. u_reason .. ")"
    end
    if finding and finding.kind == "orphan" then
        return "  [U]pdate Database            (delete orphan, type REF)"
    end
    if finding and finding.class
        and finding.class:find("^catalog") then
        return "  [U]pdate Database            (apply catalog DDL, type object.column)"
    end
    return "  [U]pdate Database            (type REF.field)"
end

function M.build_review_lines(finding, next_ref, g_reason, u_reason)
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
    lines[#lines + 1] = "  [E]xplore in more detail"
    lines[#lines + 1] = "  [S]kip for now"
    lines[#lines + 1] = "  [A]ccept permanent variance"
    lines[#lines + 1] = u_label(u_reason, finding)
    lines[#lines + 1] = g_label(next_ref, g_reason)
    lines[#lines + 1] = "  [N]ext  [P]rev  [R]e-audit  [Q]uit to dashboard"
    return lines
end

function M.build_review_lines_detailed(finding, out_dir, state, next_ref, g_reason, u_reason, allow_write)
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
    lines[#lines + 1] = "  [E]xplore in more detail"
    lines[#lines + 1] = "  [S]kip for now"
    lines[#lines + 1] = "  [A]ccept permanent variance"
    lines[#lines + 1] = u_label(u_reason, finding)
    lines[#lines + 1] = g_label(next_ref, g_reason)
    lines[#lines + 1] = promote_label(finding, state, allow_write)
    lines[#lines + 1] = "  [N]ext  [P]rev  [R]e-audit  [Q]uit to dashboard"
    return lines
end

function M.g_label(next_ref, g_reason)
    return g_label(next_ref, g_reason)
end

function M.promote_label(finding, state, allow_write)
    return promote_label(finding, state, allow_write)
end

function M.u_label(u_reason, finding)
    return u_label(u_reason, finding)
end

return M
