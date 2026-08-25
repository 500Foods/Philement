-- schemahelper_explore.lua
-- Explore-mode helpers: line wrapping, flattening the view into render rows,
-- and cursor navigation across pair rows.
--
-- CHANGELOG
-- 0.5.8 - 2026-08-25 - Extracted from schemahelper.lua (explore helpers)

local P = require("schemahelper_paint")

local display_text = P.display_text
local swidth = P.swidth

local function wrap_display_line(text, width)
    text = display_text(text)
    if width < 8 then
        width = 8
    end
    local out = {}
    if text == "" then
        out[1] = ""
        return out
    end
    while swidth(text) > width do
        local cut = width
        if cut > #text then
            cut = #text
        end
        while cut > 1 and swidth(text:sub(1, cut)) > width do
            cut = cut - 1
        end
        out[#out + 1] = text:sub(1, cut)
        text = text:sub(cut + 1)
    end
    out[#out + 1] = text
    return out
end

local function flatten_explore_rows(view, left_text_w, right_text_w)
    local out = {}
    local rows = (view and view.rows) or {}
    local layout = (view and view.layout) or "both"
    left_text_w = math.max(4, left_text_w or 8)
    right_text_w = math.max(4, right_text_w or left_text_w)
    for i = 1, #rows do
        local r = rows[i]
        if r.kind == "label" then
            out[#out + 1] = { kind = "label", text = r.text or "" }
        else
            local L = wrap_display_line(r.left or "", left_text_w)
            local R = wrap_display_line(r.right or "", right_text_w)
            if layout == "left" then
                R = { "" }
            elseif layout == "right" then
                L = { "" }
            end
            local nwrap = math.max(#L, #R)
            for w = 1, nwrap do
                out[#out + 1] = {
                    kind = "pair",
                    n = (w == 1) and r.n or nil,
                    line = r.n,
                    left = L[w] or "",
                    right = R[w] or "",
                    same = r.same,
                }
            end
        end
    end
    return out
end

local function explore_current_pair(app)
    local flat = app.explore_flat or {}
    local cur = app.explore_cursor or 0
    local r = flat[cur]
    if not r or r.kind ~= "pair" or not r.line then
        return nil
    end
    local rows = (app.explore_view and app.explore_view.rows) or {}
    for i = 1, #rows do
        local row = rows[i]
        if row.kind == "pair" and row.n == r.line then
            return row
        end
    end
    return nil
end

local function next_line_index(rows, from, dir)
    local i = from + dir
    while i >= 1 and i <= #rows do
        if rows[i].kind == "pair" and rows[i].n then
            return i
        end
        i = i + dir
    end
    return from
end

local function first_diff_line_index(rows, first_diff)
    if first_diff and first_diff > 0 then
        for i = 1, #rows do
            local r = rows[i]
            if r.kind == "pair" and r.n == first_diff then
                return i
            end
        end
    end
    for i = 1, #rows do
        local r = rows[i]
        if r.kind == "pair" and not r.same then
            return i
        end
    end
    return next_line_index(rows, 0, 1)
end

return {
    wrap_display_line = wrap_display_line,
    flatten_explore_rows = flatten_explore_rows,
    explore_current_pair = explore_current_pair,
    next_line_index = next_line_index,
    first_diff_line_index = first_diff_line_index,
}
