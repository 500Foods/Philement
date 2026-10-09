-- schemahelper_qdiff.lua
-- Line-by-line diff rendering for SchemaHelper: field payload maps,
-- focus-pair windows, and full/short comparison line generation.
-- Depends on schemahelper_qutil, schemahelper_qdecode, and
-- schemahelper_qexplain (for sides_of).
--
-- CHANGELOG
-- 0.6.3 - 2026-10-09 - Extracted from schemahelper_queue.lua (diff cluster)

local U = require("schemahelper_qutil")
local D = require("schemahelper_qdecode")
local X = require("schemahelper_qexplain")

local M = {}

-- Forward references satisfied by siblings.
M.has_embed = U.has_embed
M.split_lines = U.split_lines

function M.focus_pair(left, right, width)
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

function M.payload_map(blob)
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

function M.line_diff_lines(label, left, right, lname, rname, max_lines, width)
    local a = M.split_lines(left)
    local e = M.split_lines(right)
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
            local pair = M.focus_pair(left, right, math.max(40, width - 4))
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
                for _, row in ipairs(M.focus_pair(al, el, width - 4)) do
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

function M.field_pair(finding)
    local left, right, lname, rname = X.sides_of(finding)
    local field = finding.field
    local view = finding.view or "raw"
    local lp = M.payload_map(left)
    local rp = M.payload_map(right)
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

function M.compare_lines(finding, mode, width)
    local lines = {}
    width = width or 100
    local lv, rv, lname, rname, field, view = M.field_pair(finding)
    if lv == "" and rv == "" then
        local left, right = X.sides_of(finding)
        if left == "" and right == "" then
            return lines
        end
    end
    local label = field
    if view == "decoded" then
        label = field .. " (decoded)"
    elseif M.has_embed(lv) or M.has_embed(rv) then
        label = field .. " (encoded)"
    end
    if lv == rv then
        lines[#lines + 1] = "  " .. label .. ": identical"
        return lines
    end
    if mode == "short" then
        if field == "code" or field == "summary" or view == "decoded"
            or #lv > 80 or #rv > 80 then
            local a = M.split_lines(lv)
            local b = M.split_lines(rv)
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
    local part = M.line_diff_lines(label, lv, rv, lname, rname, 80, width)
    for _, row in ipairs(part) do
        lines[#lines + 1] = row
    end
    return lines
end

return M
