-- schemahelper_paint.lua
-- Low-level terminal painting: span/centered writers, rule joins, the framed
-- panel painter, and hotspot recording used by mouse input.
--
-- CHANGELOG
-- 0.5.8 - 2026-08-25 - Extracted from schemahelper.lua (paint cluster)

local C = require("schemahelper_const")
local UI = require("schemahelper_ui")

local t = C.t
local ATTR = C.ATTR

local function display_text(text)
    text = tostring(text or "")
    text = text:gsub("\t", "    ")
    text = text:gsub("\27%[[%d;?]*[ -/]*[@-~]", "")
    text = text:gsub("[%z\1-\8\11-\31\127]", "")
    return text
end

local function swidth(text)
    text = display_text(text)
    local ok, w = pcall(t.text.width.utf8swidth, text)
    if ok and type(w) == "number" then
        return w
    end
    return #text
end

local function write_span(row, col, width, text, attr)
    width = tonumber(width) or 0
    if width < 1 then
        return
    end
    text = display_text(text)
    local ok, shown = pcall(t.text.width.truncate_ellipsis, width, text, "right")
    if not ok or type(shown) ~= "string" then
        shown = text:sub(1, width)
    end
    t.cursor.position.set(row, col)
    t.output.write(t.text.push_seq(attr), shown, t.text.pop_seq())
end

local function write_centered(row, col, width, text, attr, trunc)
    text = display_text(text)
    local ok, shown = pcall(t.text.width.truncate_ellipsis, width, text, trunc or "right")
    if not ok or type(shown) ~= "string" then
        shown = text
    end
    local w = swidth(shown)
    if w < 1 then
        return
    end
    write_span(row, col + math.max(0, math.floor((width - w) / 2)), w, shown, attr)
end

local function paint_hotspot_highlight_for_row(row, col, width, text, attr)
    local hotspots = UI.hotspots_get()
    local mx, my = UI.mouse_hot_get()
    if not my or my ~= row then
        for _, h in ipairs(hotspots) do
            if h.row == row then
                if h.is_hotrow then
                    write_span(row, col + 1, width - 2, h.subtext, ATTR.HOTLINK)
                elseif h.subtext then
                    t.cursor.position.set(h.row, h.c0)
                    t.output.write(t.text.push_seq(ATTR.HOTLINK), h.subtext, t.text.pop_seq())
                end
            end
        end
        return
    end
    for _, h in ipairs(hotspots) do
        if h.row == row and mx >= h.c0 and mx <= h.c1 then
            if h.is_hotrow then
                write_span(row, col + 1, width - 2, h.subtext, ATTR.HOT)
            elseif h.subtext then
                t.cursor.position.set(h.row, h.c0)
                t.output.write(t.text.push_seq(ATTR.HOT), h.subtext, t.text.pop_seq())
            end
            return
        end
    end
end

local function add_hotspots_for_line(text, row, col)
    if type(text) ~= "string" or #text == 0 then
        return
    end
    local p = 1
    while true do
        local s, e, key = text:find("%[(%a+)%]", p)
        if not s then
            break
        end
        UI.hotspots_add({ row = row, c0 = col + s, c1 = col + e, key = key, subtext = text:sub(s, e) })
        p = e + 1
    end
end

local function paint_hline_join(self, row)
    local col = self.inner_col
    local width = self.inner_width
    if not row or width < 1 or col < 2 then
        return
    end
    t.cursor.position.set(row, col - 1)
    t.output.write(
        t.text.push_seq(ATTR.RULE),
        "├" .. string.rep("─", width) .. "┤",
        t.text.pop_seq())
end

local function paint_vline_join(self, split_col, top, bot)
    if not split_col or not top or not bot or bot <= top then
        return
    end
    t.cursor.position.set(top, split_col)
    t.output.write(t.text.push_seq(ATTR.RULE), "┬", t.text.pop_seq())
    for r = top + 1, bot - 1 do
        t.cursor.position.set(r, split_col)
        t.output.write(t.text.push_seq(ATTR.RULE), "│", t.text.pop_seq())
    end
    t.cursor.position.set(bot, split_col)
    t.output.write(t.text.push_seq(ATTR.RULE), "┴", t.text.pop_seq())
end

local function paint_framed(self, header, body, footer, hotrows)
    local row = self.inner_row
    local col = self.inner_col
    local height = self.inner_height
    local width = self.inner_width
    if height < 3 or width < 4 then
        return
    end
    UI.hotspots_reset()
    local last = row + height - 1
    local footer_row = last
    local footer_rule = last - 1
    local y = row
    for i = 1, #header do
        if y >= footer_rule then
            break
        end
        add_hotspots_for_line(header[i][1], y, col)
        write_span(y, col + 1, width - 2, header[i][1], header[i][2] or ATTR.PATH)
        paint_hotspot_highlight_for_row(y, col, width, header[i][1], header[i][2] or ATTR.PATH)
        y = y + 1
    end
    if y < footer_rule then
        paint_hline_join(self, y)
        y = y + 1
    end
    local vis = footer_rule - y
    if vis < 0 then
        vis = 0
    end
    for i = 1, math.min(#body, vis) do
        add_hotspots_for_line(body[i][1], y + i - 1, col)
        write_span(y + i - 1, col + 1, width - 2, body[i][1], body[i][2] or ATTR.PATH)
        paint_hotspot_highlight_for_row(y + i - 1, col, width, body[i][1], body[i][2] or ATTR.PATH)
    end
    if hotrows then
        for bi, key in pairs(hotrows) do
            UI.hotspots_add({
                row = y + bi - 1,
                c0 = col + 1,
                c1 = col + width - 1,
                key = key,
                subtext = body[bi] and body[bi][1] or string.rep(" ", width - 2),
                attr = body[bi] and (body[bi][2] or ATTR.PATH),
                is_hotrow = true,
            })
        end
    end
    paint_hline_join(self, footer_rule)
    add_hotspots_for_line(footer or "", footer_row, col)
    write_span(footer_row, col + 1, width - 2, footer or "", ATTR.PROMPT)
    paint_hotspot_highlight_for_row(footer_row, col, width, footer or "", ATTR.PROMPT)
end

return {
    display_text = display_text,
    swidth = swidth,
    write_span = write_span,
    write_centered = write_centered,
    paint_hotspot_highlight_for_row = paint_hotspot_highlight_for_row,
    add_hotspots_for_line = add_hotspots_for_line,
    paint_hline_join = paint_hline_join,
    paint_vline_join = paint_vline_join,
    paint_framed = paint_framed,
}
