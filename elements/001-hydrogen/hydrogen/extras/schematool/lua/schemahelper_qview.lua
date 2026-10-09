-- schemahelper_qview.lua
-- Explore-view builders for SchemaHelper: finding detail lines, JSON
-- explore content, explore row assembly, and operator note lookup.
-- Depends on schemahelper_qutil, schemahelper_qstate, schemahelper_qdecode,
-- schemahelper_qexplain, and schemahelper_qdiff.
--
-- CHANGELOG
-- 0.6.3 - 2026-10-09 - Extracted from schemahelper_queue.lua (view cluster)

local U = require("schemahelper_qutil")
local S = require("schemahelper_qstate")
local D = require("schemahelper_qdecode")
local X = require("schemahelper_qexplain")
local DF = require("schemahelper_qdiff")

local M = {}

function M.load_detail_section(out_dir, finding)
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

function M.decode_embedded(s)
    return D.decode_embedded(s)
end

function M.has_embed(s)
    return U.has_embed(s)
end

function M.build_line_decode_view(left_line, right_line)
    return D.build_line_decode_view(left_line, right_line)
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
        for _, r in ipairs(X.explain_check(finding)) do
            facts[#facts + 1] = r:gsub("^%s+", "")
        end
    end
    local rows = {}
    if not finding then
        return { facts = facts, rows = rows, first_diff = 0 }
    end
    local lv, rv, _, _, field, view = DF.field_pair(finding)
    local a = DF.split_lines(lv)
    local b = DF.split_lines(rv)
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
    elseif DF.has_embed(lv) or DF.has_embed(rv) then
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
    local loaded = S.load_state(S.default_state_path(out_dir or "", "", ""))
    return note_for_state(finding_id, loaded)
end

function M.default_state_path(...)
    return S.default_state_path(...)
end

function M.load_state(...)
    return S.load_state(...)
end

function M.find_finding(findings, id)
    for _, f in ipairs(findings) do
        if f.id == id then
            return f
        end
    end
    return nil
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
    for _, row in ipairs(X.explain_check(finding)) do
        lines[#lines + 1] = row
    end
    lines[#lines + 1] = ""
    lines[#lines + 1] = "--- Migration vs Database ---"
    local diffs = DF.compare_lines(finding, "full", width)
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
        finding = M.find_finding(findings, finding_id)
    end
    if not finding then
        return { "Finding not found: " .. finding_id }
    end
    local lines = M.json_explore_lines(finding, out_dir, state, width)
    local detail = filter_detail_for_finding(
        M.load_detail_section(out_dir, finding), finding)
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

return M
