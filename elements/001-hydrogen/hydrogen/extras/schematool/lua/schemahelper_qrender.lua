-- schemahelper_qrender.lua
-- Dashboard and review-line rendering for SchemaHelper: label generators
-- (generate/promote/update), build_review_lines, build_review_lines_detailed,
-- build_dashboard_lines, and the queue build orchestrator. Depends on
-- schemahelper_qutil, schemahelper_qstate, schemahelper_qload,
-- schemahelper_qdiff, and schemahelper_qview.
--
-- CHANGELOG
-- 0.6.9 - 2026-10-17 - Drop the Ref line; the id already carries it
-- 0.6.8 - 2026-10-17 - Explore header reuses the variance attributes
-- 0.6.7 - 2026-10-17 - Migration name is read from the payload body
-- 0.6.6 - 2026-10-17 - Variance summary is grouped and aligned
-- 0.6.5 - 2026-10-16 - Dashboard counts share a right-aligned column
-- 0.6.4 - 2026-10-16 - Unkeyed DML stays off the review list
-- 0.6.3 - 2026-10-09 - Extracted from schemahelper_queue.lua (render + build)

local U = require("schemahelper_qutil")
local S = require("schemahelper_qstate")
local L = require("schemahelper_qload")
local DF = require("schemahelper_qdiff")
local V = require("schemahelper_qview")

local M = {}

-- Re-export state and decode functions so callers of queue.* still work.
M.default_state_path = S.default_state_path
M.load_state = S.load_state
M.create_state = S.create_state
M.artifacts_present = S.artifacts_present
M.jq_update_state = S.jq_update_state
M.save_cursor = S.save_cursor
M.save_decision = S.save_decision
M.remove_decision = S.remove_decision

M.load_detail_section = V.load_detail_section
M.decode_embedded = V.decode_embedded
M.has_embed = U.has_embed
M.build_line_decode_view = V.build_line_decode_view

M.json_subobj = U.json_subobj
M.payload_text = U.payload_text
M.find_finding = V.find_finding

M.load_metadata = L.load_metadata
M.load_catalog = L.load_catalog
M.load_rows = L.load_rows

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

function M.u_label(u_reason, finding)
    if u_reason and u_reason ~= "" then
        return "  [U]pdate Database            (disabled — " .. u_reason .. ")"
    end
    if finding and finding.kind == "orphan" then
        return "  [U]pdate Database            (delete orphan, type REF)"
    end
    if finding and (finding.kind == "row_missing"
        or finding.kind == "row_diff" or finding.kind == "row_present") then
        return "  [U]pdate Database            (default row, type table.key)"
    end
    if finding and finding.class
        and finding.class:find("^catalog") then
        if finding.kind == "dropped" then
            return "  [U]pdate Database            (drop, type DROP object)"
        end
        return "  [U]pdate Database            (apply catalog DDL, type object.column)"
    end
    if finding and finding.field == "row" then
        return "  [U]pdate Database            (replace row, type REF)"
    end
    return "  [U]pdate Database            (type REF.field)"
end

function M.promote_label(finding, state, allow_write)
    return promote_label(finding, state, allow_write)
end

function M.g_label(next_ref, g_reason)
    return g_label(next_ref, g_reason)
end

local ATTR_W = 12

local function attr_line(label, value)
    if value == nil or value == "" then
        return nil
    end
    return string.format("  %-" .. ATTR_W .. "s %s", label or "", value)
end

local function push(rows, label, value)
    local line = attr_line(label, value)
    if line then
        rows[#rows + 1] = line
    end
end

local function add_section(lines, title, rows)
    if #rows == 0 then
        return
    end
    lines[#lines + 1] = ""
    lines[#lines + 1] = title
    for i = 1, #rows do
        lines[#lines + 1] = rows[i]
    end
end

local function oneline(text)
    text = tostring(text or "")
    text = text:gsub("[\r\n]+", " ")
    text = text:gsub("^%s+", ""):gsub("%s+$", "")
    return text
end

local function migration_name(finding)
    local blobs = { finding.expected, finding.actual }
    for i = 1, #blobs do
        local blob = blobs[i]
        if type(blob) == "string" and blob ~= "" then
            local name = oneline(U.json_string_field(blob, "name"))
            if name ~= "" then
                return name
            end
        end
    end
    return ""
end

local function skip_summary(summary)
    if not summary or summary == "" then
        return true
    end
    if summary:find("^APPLY check ref ") then
        return true
    end
    if summary:find("^LOAD check ref ") then
        return true
    end
    if summary:find("^ref %d+ on disk, not loaded$") then
        return true
    end
    if summary:find("^ref %d+ loaded, not applied$") then
        return true
    end
    if summary:find("^ref %d+ in DB, not on disk$") then
        return true
    end
    if summary:find("^ref %d+ %S+$") then
        return true
    end
    return false
end

local function is_short(value)
    return #oneline(value) <= 80
end

local function brief_text(value)
    if not is_short(value) then
        return nil
    end
    value = oneline(value)
    if value == "" then
        return nil
    end
    return value
end

local function row_kind(finding)
    local kind = finding.kind or ""
    return kind == "row_missing" or kind == "row_diff"
        or kind == "row_present" or kind == "unkeyed"
end

local function catalog_apply(finding)
    local kind = finding.kind or ""
    if kind == "nullable" then
        local want_null = finding.expected == "true"
            or finding.expected == "YES" or finding.expected == "1"
        if want_null then
            return "ALTER COLUMN DROP NOT NULL"
        end
        return "ALTER COLUMN SET NOT NULL"
    end
    if kind == "column" then
        return "ADD COLUMN (type from expected fold)"
    end
    if kind == "type" then
        return "change column type (existing values may be rejected)"
    end
    if kind == "table" then
        return "CREATE TABLE from the folded column list"
    end
    if kind == "dropped" then
        if finding.column and finding.column ~= "" and finding.column ~= "-" then
            return "DROP COLUMN (confirm token DROP object.column)"
        end
        return "DROP TABLE (confirm token DROP object)"
    end
    return nil
end

local function row_apply(kind)
    if kind == "row_missing" then
        return "INSERT this default row"
    end
    if kind == "row_diff" then
        return "UPDATE migration-owned columns"
    end
    if kind == "row_present" then
        return "DELETE this key only"
    end
    return "refused — run Hydrogen AutoMigration"
end

local function migration_rows(finding)
    local rows = {}
    local file = finding.file or ""
    if file == "(orphan)" or file == "(migration)" then
        push(rows, "File", "none on disk")
    else
        push(rows, "File", file)
    end
    push(rows, "Name", migration_name(finding))
    if file == "" and finding.ref and finding.object and finding.object ~= "" then
        push(rows, "Note", "last fold ref " .. tostring(finding.ref)
            .. " (expected shape)")
    end
    return rows
end

local function database_rows(finding)
    local rows = {}
    local kind = finding.kind or ""
    if finding.db_type == 1003 then
        push(rows, "Check", "APPLY — migration vs applied queries row")
        push(rows, "Object", "queries")
        push(rows, "Type", "1003 (applied)")
        push(rows, "Compared", "code, name, summary")
        push(rows, "Ignored",
            "query_type 1000→1003 is APPLY promotion, not a defect")
    elseif finding.db_type == 1000 then
        push(rows, "Check", "LOAD — migration vs loaded queries row")
        push(rows, "Object", "queries")
        push(rows, "Type", "1000 (loaded)")
        push(rows, "Compared", "code, name, summary")
    elseif kind == "orphan" then
        push(rows, "Check", "orphan — ref in the database, absent from disk")
        push(rows, "Object", "queries")
        push(rows, "Type", "1000/1003 (loaded/applied)")
        push(rows, "Apply", "deletes ref rows (BETWEEN 1000 AND 1003)")
    elseif kind == "missing_load" then
        push(rows, "Check", "LOAD — on disk, not loaded")
    elseif kind == "missing_apply" then
        push(rows, "Check", "APPLY — loaded, not applied")
    end
    if finding.field == "row" then
        push(rows, "Note",
            "replace code, name, and summary; this does not replay DDL")
    end
    return rows
end

local function catalog_rows(finding)
    local rows = {}
    push(rows, "Check", "catalog expected vs live object")
    push(rows, "Table", finding.object)
    if finding.column and finding.column ~= "" and finding.column ~= "-" then
        push(rows, "Column", finding.column)
    end
    push(rows, "Expected", brief_text(finding.expected))
    push(rows, "Live", brief_text(finding.live))
    push(rows, "Apply", catalog_apply(finding))
    if not skip_summary(finding.summary) then
        push(rows, "Note", oneline(finding.summary))
    end
    return rows
end

local function row_rows(finding)
    local rows = {}
    push(rows, "Table", finding.object)
    if finding.column and finding.column ~= "" and finding.column ~= "-" then
        push(rows, "Key", finding.column)
    end
    push(rows, "Expected", brief_text(finding.expected))
    push(rows, "Live", brief_text(finding.live))
    push(rows, "Apply", row_apply(finding.kind))
    push(rows, "Confirm", "table.key")
    if not skip_summary(finding.summary) then
        push(rows, "Note", oneline(finding.summary))
    end
    return rows
end

local function cap_label(field)
    field = field or "value"
    if field == "row" then
        return "Payload"
    end
    return field:sub(1, 1):upper() .. field:sub(2)
end

local function difference_rows(finding)
    local lv, rv, lname, rname, field, view = DF.field_pair(finding)
    if (lv == nil or lv == "") and (rv == nil or rv == "") then
        return {}
    end
    if finding.object and finding.object ~= ""
        and is_short(lv) and is_short(rv) then
        return {}
    end
    local rows = {}
    local label = cap_label(field)
    local qualifier = ""
    if view == "decoded" then
        qualifier = "decoded"
    elseif DF.has_embed(lv) or DF.has_embed(rv) then
        qualifier = "encoded"
    end
    if lv == rv then
        push(rows, label, "identical")
        return rows
    end
    local long = field == "code" or field == "summary" or field == "row"
        or view == "decoded" or #lv > 80 or #rv > 80
    if long then
        local a = DF.split_lines(lv)
        local b = DF.split_lines(rv)
        local maxn = math.max(#a, #b)
        local nchg = 0
        for n = 1, maxn do
            if (a[n] or "") ~= (b[n] or "") then
                nchg = nchg + 1
            end
        end
        local text = string.format("%d of %d lines differ", nchg, maxn)
        if qualifier ~= "" then
            text = text .. " (" .. qualifier .. ", " .. lname .. " vs " .. rname .. ")"
        else
            text = text .. " (" .. lname .. " vs " .. rname .. ")"
        end
        push(rows, label, text)
        return rows
    end
    push(rows, label, lname .. "  " .. U.clip_text(lv, 60))
    push(rows, "", rname .. "  " .. U.clip_text(rv, 60))
    return rows
end

local function take_rows(dst, src)
    for i = 1, #src do
        dst[#dst + 1] = src[i]
    end
end

local function variance_attr_rows(finding)
    local rows = {}
    local identity = {}
    push(identity, "Id", finding.id)
    push(identity, "Class", finding.class)
    take_rows(rows, identity)
    take_rows(rows, migration_rows(finding))
    if row_kind(finding) then
        take_rows(rows, row_rows(finding))
    elseif finding.object and finding.object ~= "" then
        take_rows(rows, catalog_rows(finding))
    else
        take_rows(rows, database_rows(finding))
    end
    return rows
end

function M.explore_facts(finding)
    if not finding then
        return {}
    end
    return variance_attr_rows(finding)
end

local function variance_facts(finding)
    local lines = { "This is the variance" }
    local identity = {}
    push(identity, "Id", finding.id)
    push(identity, "Class", finding.class)
    add_section(lines, "Identity", identity)
    add_section(lines, "Migration", migration_rows(finding))
    if row_kind(finding) then
        add_section(lines, "Row", row_rows(finding))
    elseif finding.object and finding.object ~= "" then
        add_section(lines, "Catalog", catalog_rows(finding))
    else
        add_section(lines, "Database", database_rows(finding))
    end
    add_section(lines, "Difference", difference_rows(finding))
    return lines
end

local function append_actions(lines, finding, next_ref, g_reason, u_reason, state, allow_write, with_promote)
    lines[#lines + 1] = ""
    lines[#lines + 1] = "What would you like to do?"
    lines[#lines + 1] = "  [E]xplore in more detail"
    lines[#lines + 1] = "  [S]kip for now"
    lines[#lines + 1] = "  [A]ccept permanent variance"
    lines[#lines + 1] = M.u_label(u_reason, finding)
    lines[#lines + 1] = M.g_label(next_ref, g_reason)
    if with_promote then
        lines[#lines + 1] = promote_label(finding, state, allow_write)
    end
    lines[#lines + 1] = "  [N]ext  [P]rev  [R]e-audit  [Q]uit to dashboard"
end

function M.build_review_lines(finding, next_ref, g_reason, u_reason)
    local lines = variance_facts(finding)
    append_actions(lines, finding, next_ref, g_reason, u_reason, nil, nil, false)
    return lines
end

function M.build_review_lines_detailed(finding, out_dir, state, next_ref, g_reason, u_reason, allow_write)
    local lines = variance_facts(finding)
    local note = V.note_for(finding.id, out_dir, state)
    if note and note ~= "" then
        local rows = {}
        push(rows, "Note", oneline(note))
        add_section(lines, "Operator", rows)
    end
    append_actions(lines, finding, next_ref, g_reason, u_reason, state, allow_write, true)
    return lines
end

local function is_info_extra(item)
    local kind = item.kind or ""
    if kind == "extra_table" or kind == "extra_column" then
        return true
    end
    return item.class == "catalog live extra"
end

local function is_unkeyed(item)
    return (item.kind or "") == "unkeyed"
end

function M.accept_holds(dec, item)
    if not dec or dec.action ~= "accepted" then
        return false
    end
    local got = U.finding_hash(item)
    if got == "" or not dec.hash or dec.hash == "" then
        return false
    end
    return dec.hash == got
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
        L.load_rows(out_dir .. "/rows_findings.json", tmp, all)
    end

    os.execute('rm -rf "' .. tmp .. '"')

    local subject = {}
    local classes = {}
    local info = {}
    local unkeyed_n = 0
    local accepted_list = {}
    local accepted = 0
    local applied = 0
    local packet = 0
    local skipped = 0

    for _, item in ipairs(all) do
        if is_info_extra(item) then
            info[#info + 1] = item
        elseif is_unkeyed(item) then
            unkeyed_n = unkeyed_n + 1
        else
            local dec = state.by_id and state.by_id[item.id]
            local action = dec and dec.action or ""
            item.action = action
            if action == "accepted" and M.accept_holds(dec, item) then
                accepted = accepted + 1
                accepted_list[#accepted_list + 1] = item
            elseif action == "applied" then
                applied = applied + 1
            elseif action == "packet" then
                packet = packet + 1
            else
                if action == "accepted" then
                    item.action = ""
                elseif action == "skipped" then
                    skipped = skipped + 1
                end
                subject[#subject + 1] = item
                local cls = item.class
                classes[cls] = (classes[cls] or 0) + 1
            end
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
        accepted = accepted_list,
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
            info = #info,
            unkeyed = unkeyed_n,
         },
     }
end

-- One right edge for every count. indent is spaces before the label.
function M.align_count_lines(rows)
    local label_w = 0
    local num_w = 1
    for i = 1, #rows do
        local row = rows[i]
        local indent = row.indent or 0
        local width = indent + #row.label
        if width > label_w then
            label_w = width
        end
        local text = tostring(row.value)
        if #text > num_w then
            num_w = #text
        end
    end
    local out = {}
    for i = 1, #rows do
        local row = rows[i]
        local indent = row.indent or 0
        local gap = label_w - indent - #row.label
        if gap < 0 then
            gap = 0
        end
        out[i] = string.rep(" ", indent)
            .. row.label
            .. string.rep(" ", gap)
            .. "  "
            .. string.format("%" .. num_w .. "s", tostring(row.value))
    end
    return out
end

local function summary_rows(totals)
    local rows = {
        { label = "Total migrations found", value = totals.total or 0 },
        { label = "Perfect migrations", value = totals.perfect or 0 },
        { label = "Accepted variations", value = totals.accepted or 0 },
        { label = "Findings for review", value = totals.subject or 0 },
        { label = "Live extras (not applicable)", value = totals.info or 0 },
        { label = "Unkeyed DML (AutoMigration)", value = totals.unkeyed or 0 },
    }
    if (totals.applied or 0) > 0 or (totals.packet or 0) > 0 then
        rows[#rows + 1] = {
            label = "Applied / packets",
            value = string.format("%d / %d", totals.applied or 0, totals.packet or 0),
        }
    end
    return rows
end

local function class_rows(classes)
    local rows = {}
    for i = 1, #classes do
        local c = classes[i]
        rows[#rows + 1] = {
            label = c.name,
            value = c.count,
            indent = 2,
        }
    end
    return rows
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
    local sums = summary_rows(built.totals)
    local classes = class_rows(built.classes)
    local aligned_in = {}
    for i = 1, #sums do
        aligned_in[#aligned_in + 1] = sums[i]
    end
    for i = 1, #classes do
        aligned_in[#aligned_in + 1] = classes[i]
    end
    local aligned = M.align_count_lines(aligned_in)
    for i = 1, #sums do
        lines[#lines + 1] = aligned[i]
    end
    lines[#lines + 1] = ""
    lines[#lines + 1] = "Accepted"
    lines[#lines + 1] = ""
    lines[#lines + 1] = "Variance classes (findings for review)"
    if #classes == 0 then
        lines[#lines + 1] = "  (none)"
    else
        for i = 1, #classes do
            lines[#lines + 1] = aligned[#sums + i]
        end
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
    return lines, built
end

return M
