-- schemahelper_qrender.lua
-- Dashboard and review-line rendering for SchemaHelper: label generators
-- (generate/promote/update), build_review_lines, build_review_lines_detailed,
-- build_dashboard_lines, and the queue build orchestrator. Depends on
-- schemahelper_qutil, schemahelper_qstate, schemahelper_qload,
-- schemahelper_qexplain, schemahelper_qdiff, and schemahelper_qview.
--
-- CHANGELOG
-- 0.6.3 - 2026-10-09 - Extracted from schemahelper_queue.lua (render + build)

local U = require("schemahelper_qutil")
local S = require("schemahelper_qstate")
local L = require("schemahelper_qload")
local X = require("schemahelper_qexplain")
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
    for _, row in ipairs(X.explain_check(finding)) do
        lines[#lines + 1] = row
    end
    local diffs = DF.compare_lines(finding, "short")
    for _, row in ipairs(diffs) do
        lines[#lines + 1] = row
    end
    lines[#lines + 1] = ""
    lines[#lines + 1] = "What would you like to do?"
    lines[#lines + 1] = "  [E]xplore in more detail"
    lines[#lines + 1] = "  [S]kip for now"
    lines[#lines + 1] = "  [A]ccept permanent variance"
    lines[#lines + 1] = M.u_label(u_reason, finding)
    lines[#lines + 1] = M.g_label(next_ref, g_reason)
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
    for _, row in ipairs(X.explain_check(finding)) do
        lines[#lines + 1] = row
    end
    local diffs = DF.compare_lines(finding, "short")
    for _, row in ipairs(diffs) do
        lines[#lines + 1] = row
    end
    local note = V.note_for(finding.id, out_dir, state)
    if note and note ~= "" then
        lines[#lines + 1] = "  operator: " .. note
    end
    lines[#lines + 1] = ""
    lines[#lines + 1] = "What would you like to do?"
    lines[#lines + 1] = "  [E]xplore in more detail"
    lines[#lines + 1] = "  [S]kip for now"
    lines[#lines + 1] = "  [A]ccept permanent variance"
    lines[#lines + 1] = M.u_label(u_reason, finding)
    lines[#lines + 1] = M.g_label(next_ref, g_reason)
    lines[#lines + 1] = promote_label(finding, state, allow_write)
    lines[#lines + 1] = "  [N]ext  [P]rev  [R]e-audit  [Q]uit to dashboard"
    return lines
end

local function is_info_extra(item)
    local kind = item.kind or ""
    if kind == "extra_table" or kind == "extra_column" then
        return true
    end
    return item.class == "catalog live extra"
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
    local accepted_list = {}
    local accepted = 0
    local applied = 0
    local packet = 0
    local skipped = 0

    for _, item in ipairs(all) do
        if is_info_extra(item) then
            info[#info + 1] = item
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
         },
     }
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
    lines[#lines + 1] = string.format(
        "Live extras (not applicable) %d", built.totals.info or 0)
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

return M
