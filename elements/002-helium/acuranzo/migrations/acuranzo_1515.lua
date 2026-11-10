-- Migration: acuranzo_1515.lua
-- Reports.Render.Layout (Reporting)
-- Phase 81: Justified word spacings
--
-- Rule 12: one Render module. sql_old is the current body. Reverse restores it.
-- QueryRef 175 stays unused. No lookup.
--
-- luacheck: no max line length
-- luacheck: no unused args
--
-- CHANGELOG
-- 1.0.0 - 2026-11-09 - Phase 81 Render.Layout Justified

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "scripts"
cfg.MIGRATION = "1515"
cfg.GROUP_NAME = "Reports"
cfg.SCRIPT_NAME = "Render.Layout"

local sql_old = [==[



-- Reports.Render.Layout (Reporting). Always loaded: metrics, text wrap, Overflow, band layout.
-- Loaded by Reports.Render into its per-request environment.
-- Names defined here are shared with Reports.Render and its other modules.

-- Phase 44c: one Helvetica AFM break for the designer, HTML, and later PDF.
-- Grow keeps the item font (Metrics measures the real face). embedFonts does not switch the measure.
-- Index 1 is U+0020. A missing glyph uses 278. ASCII 32-126 only.
AFM_ROMAN = {
    278, 278, 355, 556, 556, 889, 667, 222, 333, 333, 389, 584,
    278, 333, 278, 278, 556, 556, 556, 556, 556, 556, 556, 556,
    556, 556, 278, 278, 584, 584, 584, 556, 1015, 667, 667, 722,
    722, 667, 611, 778, 722, 278, 500, 667, 556, 833, 722, 778,
    667, 778, 722, 667, 611, 722, 667, 944, 667, 667, 611, 278,
    278, 278, 469, 556, 222, 556, 556, 500, 556, 556, 278, 556,
    556, 222, 222, 500, 222, 833, 556, 556, 556, 556, 333, 500,
    278, 556, 500, 722, 500, 500, 500, 334, 260, 334, 584,
}
AFM_BOLD = {
    278, 333, 474, 556, 556, 889, 722, 278, 333, 333, 389, 584,
    278, 333, 278, 278, 556, 556, 556, 556, 556, 556, 556, 556,
    556, 556, 333, 333, 584, 584, 584, 611, 975, 722, 722, 722,
    722, 667, 611, 778, 722, 278, 556, 722, 611, 833, 722, 778,
    667, 778, 722, 667, 611, 722, 667, 944, 667, 667, 611, 333,
    278, 333, 584, 556, 278, 556, 611, 556, 611, 556, 333, 611,
    611, 278, 278, 556, 278, 889, 611, 611, 611, 611, 389, 556,
    333, 611, 556, 778, 556, 556, 500, 389, 280, 389, 584,
}
AFM_MISSING = 278
WRAP_MODES = { normal = true, pre = true, ["pre-line"] = true, ["pre-wrap"] = true, nowrap = true }
OVERFLOW_MODES = { Clip = true, Ellipsis = true, Grow = true, Shrink = true, Fit = true }
FIT_HEIGHT_MAP = { clip = "Clip", grow = "Grow", shrink = "Shrink", both = "Grow" }
FIT_HEIGHTS = { clip = true, grow = true, shrink = true, both = true }
VALIGNS = { top = true, middle = true, bottom = true }
LAYOUT_EPS = 1e-6
MIN_BAND_PT = 0.15 * 72
LAYOUT_UNIT = { ["in"] = 72, mm = 72 / 25.4, cm = 72 / 2.54, pt = 1, px = 0.75 }

function fmt_num(n)
    if type(n) ~= "number" or n ~= n or n == math.huge or n == -math.huge then n = 0 end
    local neg = n < 0
    local s = string.format("%.6f", math.abs(n))
    s = s:gsub("0+$", ""):gsub("%.$", "")
    if s == "" then s = "0" end
    if neg and s ~= "0" then s = "-" .. s end
    return s
end

function fmt_pt(n)
    return fmt_num(n) .. "pt"
end

function length_pt(len)
    if type(len) ~= "string" then return nil end
    local n, u = len:match("^%s*([%+%-]?[%d%.]+)%s*(%a+)%s*$")
    n = tonumber(n)
    if not n or not LAYOUT_UNIT[u] then return nil end
    return n * LAYOUT_UNIT[u]
end

function half_away(n)
    if type(n) ~= "number" or n ~= n or n == math.huge or n == -math.huge then return 0 end
    if n >= 0 then return math.floor(n + 0.5) end
    return -math.floor(-n + 0.5)
end

-- Current face for advance_units / measure_pt. layout_band sets this from the item font.
-- Helvetica (and empty) keep the inlined AFM path; other families call Metrics.
measure_family = "Helvetica"

function face_bold(style)
    return style == "Bold" or style == "Bold + Italic"
end

function advance_units(ch, style)
    local fam = measure_family
    if type(fam) == "string" and fam ~= "" and fam ~= "Helvetica" then
        need_module("Metrics")
        return metrics_advance_units(ch, fam, style)
    end
    local b = string.byte(ch)
    local table = face_bold(style) and AFM_BOLD or AFM_ROMAN
    if not b or b < 32 or b > 126 then return AFM_MISSING end
    return table[b - 31]
end

function advance_pt(ch, size, style)
    return advance_units(ch, style) * size / 1000
end

function measure_pt(text, size, style)
    local width = 0
    for i = 1, #text do width = width + advance_pt(text:sub(i, i), size, style) end
    return width
end

function line_height_pt(size, multiple)
    local factor = 1.2
    if type(multiple) == "number" and multiple == multiple and multiple > 0 and multiple ~= math.huge then
        factor = multiple
    end
    return factor * size
end

function collapse_ws(text)
    local s = text:gsub("[ \t\n\r\f]+", " ")
    return (s:gsub("^[ ]+", ""):gsub("[ ]+$", ""))
end

function collapse_line(text)
    local s = text:gsub("[ \t\f]+", " ")
    return (s:gsub("^[ ]+", ""):gsub("[ ]+$", ""))
end

function split_chars(text)
    local out = {}
    for i = 1, #text do out[i] = text:sub(i, i) end
    return out
end

function split_words(text)
    local out = {}
    for word in text:gmatch("%S+") do out[#out + 1] = word end
    return out
end

function split_nl(text)
    local out, start = {}, 1
    while true do
        local i = string.find(text, "\n", start, true)
        if not i then
            out[#out + 1] = text:sub(start):gsub("\r", "")
            return out
        end
        out[#out + 1] = text:sub(start, i - 1):gsub("\r", "")
        start = i + 1
    end
end

function break_word(word, width, size, style)
    if not (width > LAYOUT_EPS) then return split_chars(word) end
    local parts, line, used = {}, "", 0
    for i = 1, #word do
        local ch = word:sub(i, i)
        local w = advance_pt(ch, size, style)
        if line ~= "" and used + w > width + LAYOUT_EPS then
            parts[#parts + 1] = line
            line, used = ch, w
        else
            line = line .. ch
            used = used + w
        end
    end
    if line ~= "" then parts[#parts + 1] = line end
    if #parts == 0 then parts[1] = "" end
    return parts
end

function pack_words(words, width, size, style, char_break)
    if #words == 0 then return { "" } end
    if not (width > LAYOUT_EPS) then
        local joined = table.concat(words, " ")
        if not char_break then return { joined } end
        return split_chars(joined)
    end
    local space = advance_pt(" ", size, style)
    local lines, line, used = {}, "", 0
    local function put(word)
        local pieces = { word }
        if char_break and measure_pt(word, size, style) > width + LAYOUT_EPS then
            pieces = break_word(word, width, size, style)
        end
        for _, piece in ipairs(pieces) do
            local w = measure_pt(piece, size, style)
            if line == "" then
                line, used = piece, w
            elseif used + space + w <= width + LAYOUT_EPS then
                line = line .. " " .. piece
                used = used + space + w
            else
                lines[#lines + 1] = line
                line, used = piece, w
            end
        end
    end
    for _, word in ipairs(words) do put(word) end
    if line ~= "" or #lines == 0 then lines[#lines + 1] = line end
    return lines
end

function wrap_pre(text, width, size, style)
    if text == "" then return { "" } end
    if not (width > LAYOUT_EPS) then return split_chars(text) end
    local lines, line, used, last_space = {}, "", 0, 0
    local function finish(count)
        lines[#lines + 1] = line:sub(1, count)
        line = line:sub(count + 1)
        used = measure_pt(line, size, style)
        last_space = 0
        for i = 1, #line do
            local ch = line:sub(i, i)
            if ch == " " or ch == "\t" then last_space = i end
        end
    end
    for i = 1, #text do
        local ch = text:sub(i, i)
        local w = advance_pt((ch == "\t") and " " or ch, size, style)
        if line ~= "" and used + w > width + LAYOUT_EPS then
            if last_space > 0 then finish(last_space) else finish(#line) end
        end
        line = line .. ch
        used = used + w
        if ch == " " or ch == "\t" then last_space = #line end
    end
    if line ~= "" or #lines == 0 then lines[#lines + 1] = line end
    return lines
end

function wrap_text(text, width, font_size, style, mode)
    local source = text == nil and "" or tostring(text)
    local size = (type(font_size) == "number" and font_size > 0) and font_size or 10
    style = style or "Normal"
    mode = mode or "pre-line"
    width = width or 0
    if mode == "pre" then
        if source == "" then return { "" } end
        return split_nl(source)
    end
    if mode == "nowrap" then return { collapse_ws(source) } end
    if mode == "normal" then
        local collapsed = collapse_ws(source)
        if collapsed == "" then return { "" } end
        return pack_words(split_words(collapsed), width, size, style, true)
    end
    if mode == "pre-wrap" then
        if source == "" then return { "" } end
        local lines = {}
        for _, block in ipairs(split_nl(source)) do
            for _, piece in ipairs(wrap_pre(block, width, size, style)) do
                lines[#lines + 1] = piece
            end
        end
        if #lines == 0 then lines[1] = "" end
        return lines
    end
    local lines = {}
    for _, block in ipairs(split_nl(source)) do
        local collapsed = collapse_line(block)
        if collapsed == "" then
            lines[#lines + 1] = ""
        else
            for _, piece in ipairs(pack_words(split_words(collapsed), width, size, style, true)) do
                lines[#lines + 1] = piece
            end
        end
    end
    if #lines == 0 then lines[1] = "" end
    return lines
end

function default_overflow(type_key)
    if type_key == 3 or type_key == 23 then return "Grow" end
    return "Clip"
end

function default_fit(type_key)
    if type_key == 3 or type_key == 23 then return "both" end
    return "clip"
end

function map_fit_height(fit)
    if type(fit) ~= "string" or fit == "" then return nil end
    return FIT_HEIGHT_MAP[fit]
end

function resolve_overflow(item, seen)
    local tk = tonumber(item.typeKey) or 0
    local fallback = default_overflow(tk)
    local raw = item.overflow
    if type(raw) == "string" and raw ~= "" then
        if OVERFLOW_MODES[raw] then return raw end
        layout_warn(seen, "overflow:" .. tostring(item.id),
            "item " .. tostring(item.id) .. " overflow " .. tostring(raw)
            .. " is not Clip, Ellipsis, Grow, Shrink, or Fit; used " .. fallback)
        return fallback
    end
    local mapped = map_fit_height(item.fitHeight)
    if mapped then return mapped end
    if type(item.fitHeight) == "string" and item.fitHeight ~= "" then
        layout_warn(seen, "fit:" .. tostring(item.id),
            "item " .. tostring(item.id) .. " fitHeight " .. tostring(item.fitHeight)
            .. " is not clip, grow, shrink, or both; used " .. fallback)
    end
    return fallback
end

function resolve_min_font_pt(item, set_pt)
    local raw = item.minFontSize
    local pt = nil
    if type(raw) == "number" then pt = raw
    elseif type(raw) == "string" then pt = length_pt(raw) end
    if pt and pt > 0 then return math.min(pt, set_pt) end
    return math.min(6, set_pt)
end

function resolve_max_font_pt(item, set_pt)
    local raw = item.maxFontSize
    local pt = nil
    if type(raw) == "number" then pt = raw
    elseif type(raw) == "string" then pt = length_pt(raw) end
    if pt and pt > 0 then return math.max(pt, set_pt) end
    return math.max(72, set_pt)
end

function is_width_only_wrap(mode)
    return mode == "nowrap"
end

function ellipsis_line(text, width_pt, font_size, style, family)
    local source = text or ""
    local ell = "…"
    if not (width_pt > LAYOUT_EPS) then return ell end
    local prev = measure_family
    if family then measure_family = family end
    if measure_pt(source, font_size, style) <= width_pt + LAYOUT_EPS then
        measure_family = prev
        return source
    end
    local ell_w = measure_pt(ell, font_size, style)
    if ell_w > width_pt + LAYOUT_EPS then
        measure_family = prev
        return ell
    end
    local chars = split_chars(source)
    local lo, hi, best = 0, #chars, ell
    while lo <= hi do
        local mid = math.floor((lo + hi) / 2)
        local cand = table.concat(chars, "", 1, mid) .. ell
        if measure_pt(cand, font_size, style) <= width_pt + LAYOUT_EPS then
            best = cand
            lo = mid + 1
        else
            hi = mid - 1
        end
    end
    measure_family = prev
    return best
end

function apply_ellipsis(lines, width_pt, height_pt, leading, font_size, style, family, width_only)
    if not lines or #lines == 0 then return lines end
    if width_only then
        return { ellipsis_line(lines[1] or "", width_pt, font_size, style, family) }
    end
    local max_lines = math.max(1, math.floor((height_pt + LAYOUT_EPS) / leading))
    local kept = {}
    for i = 1, math.min(#lines, max_lines) do kept[i] = lines[i] end
    if #kept == 0 then return { ellipsis_line("", width_pt, font_size, style, family) } end
    kept[#kept] = ellipsis_line(kept[#kept], width_pt, font_size, style, family)
    return kept
end

function default_wrap(type_key)
    if type_key == 3 or type_key == 23 then return "pre-wrap" end
    if type_key == 1 or type_key == 4 then return "nowrap" end
    return "pre-line"
end

function paints_lines(item, overflow)
    local tk = tonumber(item and item.typeKey)
    if tk == 1 or tk == 3 or tk == 4 or tk == 23 then return true end
    if (tk == 0 or tk == 20) and overflow ~= "Clip" and overflow ~= "clip" then return true end
    return false
end

function layout_warn(seen, key, message)
    if seen[key] then return end
    seen[key] = true
    add_log("warn", message)
end

function font_of(doc, item)
    local own = type(item.font) == "table" and item.font or nil
    local report = type(doc) == "table" and type(doc.font) == "table" and doc.font or nil
    local source = own or report or {}
    return {
        family = (type(source.family) == "string" and source.family ~= "") and source.family or "Vanadium Sans",
        size = (type(source.size) == "string" and source.size ~= "") and source.size or "10 pt",
        style = (type(source.style) == "string" and source.style ~= "") and source.style or "Normal",
    }
end

function resolve_wrap(item, seen)
    local tk = tonumber(item.typeKey) or 0
    local mode = item.wrap
    if mode == nil or mode == "" then return default_wrap(tk) end
    if WRAP_MODES[mode] then return mode end
    local fallback = default_wrap(tk)
    layout_warn(seen, "wrap:" .. tostring(item.id),
        "item " .. tostring(item.id) .. " wrap " .. tostring(mode) .. " is not a wrap mode; used " .. fallback)
    return fallback
end

function resolve_fit_height(item, seen)
    local fallback = default_fit(tonumber(item.typeKey) or 0)
    local fit = item.fitHeight
    if fit == nil or fit == "" then return fallback end
    if FIT_HEIGHTS[fit] then return fit end
    layout_warn(seen, "fit:" .. tostring(item.id),
        "item " .. tostring(item.id) .. " fitHeight " .. tostring(fit)
        .. " is not clip, grow, shrink, or both; used " .. fallback)
    return fallback
end

function resolve_valign(item, seen)
    local valign = item.valign
    if valign == nil or valign == "" then return "top" end
    if VALIGNS[valign] then return valign end
    layout_warn(seen, "valign:" .. tostring(item.id),
        "item " .. tostring(item.id) .. " valign " .. tostring(valign) .. " is not top, middle, or bottom; used top")
    return "top"
end

function resolve_multiple(item, seen)
    if item.lineHeight == nil or item.lineHeight == "" then return 1.2 end
    local value = tonumber(item.lineHeight)
    if value and value == value and value > 0 and value ~= math.huge then return value end
    layout_warn(seen, "lh:" .. tostring(item.id),
        "item " .. tostring(item.id) .. " lineHeight " .. tostring(item.lineHeight)
        .. " is not a positive multiple; used 1.2")
    return 1.2
end

function valign_offset(valign, box_pt, block_pt)
    local slack = box_pt - block_pt
    if slack <= LAYOUT_EPS then return 0 end
    if valign == "bottom" then return slack end
    if valign == "middle" then return slack / 2 end
    return 0
end

function fitted_height(overflow, stored_pt, content_pt)
    if overflow == "Grow" or overflow == "grow" or overflow == "both" then
        return math.max(stored_pt, content_pt)
    end
    return stored_pt
end

function layout_band(doc, band, entries)
    local seen, placed, reflow = {}, {}, false
    for _, entry in ipairs(entries or {}) do
        local item = entry.item or {}
        local overflow = entry.defer and "Clip" or resolve_overflow(item, seen)
        if overflow == "Grow" then reflow = true end
        local box = type(item.box) == "table" and item.box or {}
        local top = length_pt(box.top) or 0
        local height = length_pt(box.height)
        local width = length_pt(box.width) or 0
        local font = font_of(doc, item)
        local size = length_pt(font.size)
        if size == nil and font.size then
            layout_warn(seen, "size:" .. tostring(item.id),
                "item " .. tostring(item.id) .. " font size " .. tostring(font.size) .. " does not parse; used 10 pt")
        end
        local font_size = (size and size > 0) and size or 10
        local multiple = resolve_multiple(item, seen)
        local mode = resolve_wrap(item, seen)
        local valign = resolve_valign(item, seen)
        local width_only = is_width_only_wrap(mode)
        local stored_h = (height and height > 0) and height or 0

        local function measure_at(pt)
            local lead = line_height_pt(pt, multiple)
            local lines = nil
            if paints_lines(item, overflow) and not entry.defer then
                measure_family = font.family or "Helvetica"
                lines = wrap_text(entry.text or "", width, pt, font.style, mode)
                measure_family = "Helvetica"
                if overflow == "Ellipsis" then
                    lines = apply_ellipsis(lines, width, stored_h, lead, pt, font.style, font.family, width_only)
                end
            end
            return lines, lead
        end

        if not entry.defer and (overflow == "Shrink" or overflow == "Fit") then
            local min_pt = resolve_min_font_pt(item, font_size)
            local max_pt = overflow == "Fit" and resolve_max_font_pt(item, font_size) or font_size
            local lo = math.min(min_pt, max_pt)
            local hi = math.max(min_pt, max_pt)
            local set_pt = font_size
            if overflow == "Shrink" then
                local at_set_lines, at_set_lead = measure_at(set_pt)
                local content_h = math.max(1, at_set_lines and #at_set_lines or 1) * at_set_lead
                local fits_set = true
                if at_set_lines then
                    if width_only then
                        measure_family = font.family or "Helvetica"
                        fits_set = measure_pt(at_set_lines[1] or "", set_pt, font.style) <= width + LAYOUT_EPS
                        measure_family = "Helvetica"
                    else
                        fits_set = content_h <= stored_h + LAYOUT_EPS
                    end
                end
                if not fits_set then
                    local best, low, high = lo, lo, set_pt
                    for _ = 1, 16 do
                        local mid = (low + high) / 2
                        local lines, lead = measure_at(mid)
                        local ch = math.max(1, lines and #lines or 1) * lead
                        local fits = true
                        if lines then
                            if width_only then
                                measure_family = font.family or "Helvetica"
                                fits = measure_pt(lines[1] or "", mid, font.style) <= width + LAYOUT_EPS
                                measure_family = "Helvetica"
                            else
                                fits = ch <= stored_h + LAYOUT_EPS
                            end
                        end
                        if fits then best = mid; low = mid else high = mid end
                    end
                    font_size = math.max(lo, best)
                end
            else
                local best, low, high = lo, lo, hi
                for _ = 1, 16 do
                    local mid = (low + high) / 2
                    local lines, lead = measure_at(mid)
                    local ch = math.max(1, lines and #lines or 1) * lead
                    local fits = true
                    if lines then
                        if width_only then
                            measure_family = font.family or "Helvetica"
                            fits = measure_pt(lines[1] or "", mid, font.style) <= width + LAYOUT_EPS
                            measure_family = "Helvetica"
                        else
                            fits = ch <= stored_h + LAYOUT_EPS
                        end
                    end
                    if fits then best = mid; low = mid else high = mid end
                end
                font_size = math.max(lo, math.min(hi, best))
            end
        end

        local leading = line_height_pt(font_size, multiple)
        local lines = nil
        if paints_lines(item, overflow) and not entry.defer then
            measure_family = font.family or "Helvetica"
            lines = wrap_text(entry.text or "", width, font_size, font.style, mode)
            measure_family = "Helvetica"
            if overflow == "Ellipsis" then
                lines = apply_ellipsis(lines, width, stored_h, leading, font_size, font.style, font.family, width_only)
            end
        end
        local content_h = math.max(1, lines and #lines or 1) * leading
        local used_h = (entry.defer or not lines) and stored_h or fitted_height(overflow, stored_h, content_h)
        local painted = nil
        if lines then
            local visible = {}
            for index, line in ipairs(lines) do
                local i = index - 1
                if (i + 1) * leading <= used_h + LAYOUT_EPS or (i == 0 and used_h > LAYOUT_EPS) then
                    visible[#visible + 1] = line
                end
            end
            painted = {}
            for index, line in ipairs(visible) do
                local i = index - 1
                if (i + 1) * leading <= used_h + LAYOUT_EPS then
                    painted[#painted + 1] = line
                end
            end
        end
        local out_font = {
            family = font.family,
            size = string.format("%.4f pt", font_size):gsub("0+ pt", " pt"):gsub(" %.pt", " pt"),
            style = font.style,
        }
        out_font.size = fmt_pt(font_size)
        local fit = (overflow == "Grow") and "grow" or "clip"
        placed[#placed + 1] = {
            item = item, fit = fit, overflow = overflow, lines = painted, font = out_font, leading = leading,
            valign = valign, orig_top = top, stored_h = stored_h, used_h = used_h,
            delta = used_h - stored_h,
        }
    end
    for _, row in ipairs(placed) do
        local shift = 0
        for _, other in ipairs(placed) do
            if other ~= row and other.orig_top + other.stored_h <= row.orig_top + LAYOUT_EPS then
                shift = shift + other.delta
            end
        end
        row.used_top = row.orig_top + shift
    end
    local stored_band = length_pt(band and band.height)
    local used_pt = (stored_band and stored_band > 0) and stored_band or 0
    if reflow then
        local bottom = 0
        for _, row in ipairs(placed) do
            local edge = row.used_top + row.used_h
            if edge > bottom then bottom = edge end
        end
        used_pt = math.max(MIN_BAND_PT, bottom)
    end
    local items = {}
    for _, row in ipairs(placed) do
        local block = row.lines and (#row.lines * row.leading) or 0
        items[#items + 1] = {
            lines = row.lines,
            valign = row.valign,
            lineHeightPt = row.leading,
            offsetPt = row.lines and valign_offset(row.valign, row.used_h, block) or 0,
            grown = math.abs(row.used_h - row.stored_h) > LAYOUT_EPS,
            topPt = row.used_top,
            heightPt = row.used_h,
            -- Shrink/Fit resolve a new size; Grow keeps family. Paging copies this onto item_out.
            font = row.font,
            overflow = row.overflow,
        }
    end
    return { reflow = reflow, usedPt = used_pt, items = items }
end



]==]

local sql_new = [==[



-- Reports.Render.Layout (Reporting). Always loaded: metrics, text wrap, Overflow, band layout.
-- Loaded by Reports.Render into its per-request environment.
-- Names defined here are shared with Reports.Render and its other modules.

-- Phase 44c: one Helvetica AFM break for the designer, HTML, and later PDF.
-- Grow keeps the item font (Metrics measures the real face). embedFonts does not switch the measure.
-- Index 1 is U+0020. A missing glyph uses 278. ASCII 32-126 only.
AFM_ROMAN = {
    278, 278, 355, 556, 556, 889, 667, 222, 333, 333, 389, 584,
    278, 333, 278, 278, 556, 556, 556, 556, 556, 556, 556, 556,
    556, 556, 278, 278, 584, 584, 584, 556, 1015, 667, 667, 722,
    722, 667, 611, 778, 722, 278, 500, 667, 556, 833, 722, 778,
    667, 778, 722, 667, 611, 722, 667, 944, 667, 667, 611, 278,
    278, 278, 469, 556, 222, 556, 556, 500, 556, 556, 278, 556,
    556, 222, 222, 500, 222, 833, 556, 556, 556, 556, 333, 500,
    278, 556, 500, 722, 500, 500, 500, 334, 260, 334, 584,
}
AFM_BOLD = {
    278, 333, 474, 556, 556, 889, 722, 278, 333, 333, 389, 584,
    278, 333, 278, 278, 556, 556, 556, 556, 556, 556, 556, 556,
    556, 556, 333, 333, 584, 584, 584, 611, 975, 722, 722, 722,
    722, 667, 611, 778, 722, 278, 556, 722, 611, 833, 722, 778,
    667, 778, 722, 667, 611, 722, 667, 944, 667, 667, 611, 333,
    278, 333, 584, 556, 278, 556, 611, 556, 611, 556, 333, 611,
    611, 278, 278, 556, 278, 889, 611, 611, 611, 611, 389, 556,
    333, 611, 556, 778, 556, 556, 500, 389, 280, 389, 584,
}
AFM_MISSING = 278
WRAP_MODES = { normal = true, pre = true, ["pre-line"] = true, ["pre-wrap"] = true, nowrap = true }
OVERFLOW_MODES = { Clip = true, Ellipsis = true, Grow = true, Shrink = true, Fit = true }
FIT_HEIGHT_MAP = { clip = "Clip", grow = "Grow", shrink = "Shrink", both = "Grow" }
FIT_HEIGHTS = { clip = true, grow = true, shrink = true, both = true }
VALIGNS = { top = true, middle = true, bottom = true }
LAYOUT_EPS = 1e-6
MIN_BAND_PT = 0.15 * 72
LAYOUT_UNIT = { ["in"] = 72, mm = 72 / 25.4, cm = 72 / 2.54, pt = 1, px = 0.75 }

function fmt_num(n)
    if type(n) ~= "number" or n ~= n or n == math.huge or n == -math.huge then n = 0 end
    local neg = n < 0
    local s = string.format("%.6f", math.abs(n))
    s = s:gsub("0+$", ""):gsub("%.$", "")
    if s == "" then s = "0" end
    if neg and s ~= "0" then s = "-" .. s end
    return s
end

function fmt_pt(n)
    return fmt_num(n) .. "pt"
end

function length_pt(len)
    if type(len) ~= "string" then return nil end
    local n, u = len:match("^%s*([%+%-]?[%d%.]+)%s*(%a+)%s*$")
    n = tonumber(n)
    if not n or not LAYOUT_UNIT[u] then return nil end
    return n * LAYOUT_UNIT[u]
end

function half_away(n)
    if type(n) ~= "number" or n ~= n or n == math.huge or n == -math.huge then return 0 end
    if n >= 0 then return math.floor(n + 0.5) end
    return -math.floor(-n + 0.5)
end

-- Current face for advance_units / measure_pt. layout_band sets this from the item font.
-- Helvetica (and empty) keep the inlined AFM path; other families call Metrics.
measure_family = "Helvetica"

function face_bold(style)
    return style == "Bold" or style == "Bold + Italic"
end

function advance_units(ch, style)
    local fam = measure_family
    if type(fam) == "string" and fam ~= "" and fam ~= "Helvetica" then
        need_module("Metrics")
        return metrics_advance_units(ch, fam, style)
    end
    local b = string.byte(ch)
    local table = face_bold(style) and AFM_BOLD or AFM_ROMAN
    if not b or b < 32 or b > 126 then return AFM_MISSING end
    return table[b - 31]
end

function advance_pt(ch, size, style)
    return advance_units(ch, style) * size / 1000
end

function measure_pt(text, size, style)
    local width = 0
    for i = 1, #text do width = width + advance_pt(text:sub(i, i), size, style) end
    return width
end

function line_height_pt(size, multiple)
    local factor = 1.2
    if type(multiple) == "number" and multiple == multiple and multiple > 0 and multiple ~= math.huge then
        factor = multiple
    end
    return factor * size
end

function collapse_ws(text)
    local s = text:gsub("[ \t\n\r\f]+", " ")
    return (s:gsub("^[ ]+", ""):gsub("[ ]+$", ""))
end

function collapse_line(text)
    local s = text:gsub("[ \t\f]+", " ")
    return (s:gsub("^[ ]+", ""):gsub("[ ]+$", ""))
end

function split_chars(text)
    local out = {}
    for i = 1, #text do out[i] = text:sub(i, i) end
    return out
end

function split_words(text)
    local out = {}
    for word in text:gmatch("%S+") do out[#out + 1] = word end
    return out
end

function split_nl(text)
    local out, start = {}, 1
    while true do
        local i = string.find(text, "\n", start, true)
        if not i then
            out[#out + 1] = text:sub(start):gsub("\r", "")
            return out
        end
        out[#out + 1] = text:sub(start, i - 1):gsub("\r", "")
        start = i + 1
    end
end

function break_word(word, width, size, style)
    if not (width > LAYOUT_EPS) then return split_chars(word) end
    local parts, line, used = {}, "", 0
    for i = 1, #word do
        local ch = word:sub(i, i)
        local w = advance_pt(ch, size, style)
        if line ~= "" and used + w > width + LAYOUT_EPS then
            parts[#parts + 1] = line
            line, used = ch, w
        else
            line = line .. ch
            used = used + w
        end
    end
    if line ~= "" then parts[#parts + 1] = line end
    if #parts == 0 then parts[1] = "" end
    return parts
end

function pack_words(words, width, size, style, char_break)
    if #words == 0 then return { "" } end
    if not (width > LAYOUT_EPS) then
        local joined = table.concat(words, " ")
        if not char_break then return { joined } end
        return split_chars(joined)
    end
    local space = advance_pt(" ", size, style)
    local lines, line, used = {}, "", 0
    local function put(word)
        local pieces = { word }
        if char_break and measure_pt(word, size, style) > width + LAYOUT_EPS then
            pieces = break_word(word, width, size, style)
        end
        for _, piece in ipairs(pieces) do
            local w = measure_pt(piece, size, style)
            if line == "" then
                line, used = piece, w
            elseif used + space + w <= width + LAYOUT_EPS then
                line = line .. " " .. piece
                used = used + space + w
            else
                lines[#lines + 1] = line
                line, used = piece, w
            end
        end
    end
    for _, word in ipairs(words) do put(word) end
    if line ~= "" or #lines == 0 then lines[#lines + 1] = line end
    return lines
end

function wrap_pre(text, width, size, style)
    if text == "" then return { "" } end
    if not (width > LAYOUT_EPS) then return split_chars(text) end
    local lines, line, used, last_space = {}, "", 0, 0
    local function finish(count)
        lines[#lines + 1] = line:sub(1, count)
        line = line:sub(count + 1)
        used = measure_pt(line, size, style)
        last_space = 0
        for i = 1, #line do
            local ch = line:sub(i, i)
            if ch == " " or ch == "\t" then last_space = i end
        end
    end
    for i = 1, #text do
        local ch = text:sub(i, i)
        local w = advance_pt((ch == "\t") and " " or ch, size, style)
        if line ~= "" and used + w > width + LAYOUT_EPS then
            if last_space > 0 then finish(last_space) else finish(#line) end
        end
        line = line .. ch
        used = used + w
        if ch == " " or ch == "\t" then last_space = #line end
    end
    if line ~= "" or #lines == 0 then lines[#lines + 1] = line end
    return lines
end

function wrap_text(text, width, font_size, style, mode)
    local source = text == nil and "" or tostring(text)
    local size = (type(font_size) == "number" and font_size > 0) and font_size or 10
    style = style or "Normal"
    mode = mode or "pre-line"
    width = width or 0
    if mode == "pre" then
        if source == "" then return { "" } end
        return split_nl(source)
    end
    if mode == "nowrap" then return { collapse_ws(source) } end
    if mode == "normal" then
        local collapsed = collapse_ws(source)
        if collapsed == "" then return { "" } end
        return pack_words(split_words(collapsed), width, size, style, true)
    end
    if mode == "pre-wrap" then
        if source == "" then return { "" } end
        local lines = {}
        for _, block in ipairs(split_nl(source)) do
            for _, piece in ipairs(wrap_pre(block, width, size, style)) do
                lines[#lines + 1] = piece
            end
        end
        if #lines == 0 then lines[1] = "" end
        return lines
    end
    local lines = {}
    for _, block in ipairs(split_nl(source)) do
        local collapsed = collapse_line(block)
        if collapsed == "" then
            lines[#lines + 1] = ""
        else
            for _, piece in ipairs(pack_words(split_words(collapsed), width, size, style, true)) do
                lines[#lines + 1] = piece
            end
        end
    end
    if #lines == 0 then lines[1] = "" end
    return lines
end

function default_overflow(type_key)
    if type_key == 3 or type_key == 23 then return "Grow" end
    return "Clip"
end

function default_fit(type_key)
    if type_key == 3 or type_key == 23 then return "both" end
    return "clip"
end

function map_fit_height(fit)
    if type(fit) ~= "string" or fit == "" then return nil end
    return FIT_HEIGHT_MAP[fit]
end

function resolve_overflow(item, seen)
    local tk = tonumber(item.typeKey) or 0
    local fallback = default_overflow(tk)
    local raw = item.overflow
    if type(raw) == "string" and raw ~= "" then
        if OVERFLOW_MODES[raw] then return raw end
        layout_warn(seen, "overflow:" .. tostring(item.id),
            "item " .. tostring(item.id) .. " overflow " .. tostring(raw)
            .. " is not Clip, Ellipsis, Grow, Shrink, or Fit; used " .. fallback)
        return fallback
    end
    local mapped = map_fit_height(item.fitHeight)
    if mapped then return mapped end
    if type(item.fitHeight) == "string" and item.fitHeight ~= "" then
        layout_warn(seen, "fit:" .. tostring(item.id),
            "item " .. tostring(item.id) .. " fitHeight " .. tostring(item.fitHeight)
            .. " is not clip, grow, shrink, or both; used " .. fallback)
    end
    return fallback
end

function resolve_min_font_pt(item, set_pt)
    local raw = item.minFontSize
    local pt = nil
    if type(raw) == "number" then pt = raw
    elseif type(raw) == "string" then pt = length_pt(raw) end
    if pt and pt > 0 then return math.min(pt, set_pt) end
    return math.min(6, set_pt)
end

function resolve_max_font_pt(item, set_pt)
    local raw = item.maxFontSize
    local pt = nil
    if type(raw) == "number" then pt = raw
    elseif type(raw) == "string" then pt = length_pt(raw) end
    if pt and pt > 0 then return math.max(pt, set_pt) end
    return math.max(72, set_pt)
end

function is_width_only_wrap(mode)
    return mode == "nowrap"
end

function ellipsis_line(text, width_pt, font_size, style, family)
    local source = text or ""
    local ell = "…"
    if not (width_pt > LAYOUT_EPS) then return ell end
    local prev = measure_family
    if family then measure_family = family end
    if measure_pt(source, font_size, style) <= width_pt + LAYOUT_EPS then
        measure_family = prev
        return source
    end
    local ell_w = measure_pt(ell, font_size, style)
    if ell_w > width_pt + LAYOUT_EPS then
        measure_family = prev
        return ell
    end
    local chars = split_chars(source)
    local lo, hi, best = 0, #chars, ell
    while lo <= hi do
        local mid = math.floor((lo + hi) / 2)
        local cand = table.concat(chars, "", 1, mid) .. ell
        if measure_pt(cand, font_size, style) <= width_pt + LAYOUT_EPS then
            best = cand
            lo = mid + 1
        else
            hi = mid - 1
        end
    end
    measure_family = prev
    return best
end

function apply_ellipsis(lines, width_pt, height_pt, leading, font_size, style, family, width_only)
    if not lines or #lines == 0 then return lines end
    if width_only then
        return { ellipsis_line(lines[1] or "", width_pt, font_size, style, family) }
    end
    local max_lines = math.max(1, math.floor((height_pt + LAYOUT_EPS) / leading))
    local kept = {}
    for i = 1, math.min(#lines, max_lines) do kept[i] = lines[i] end
    if #kept == 0 then return { ellipsis_line("", width_pt, font_size, style, family) } end
    kept[#kept] = ellipsis_line(kept[#kept], width_pt, font_size, style, family)
    return kept
end

function default_wrap(type_key)
    if type_key == 3 or type_key == 23 then return "pre-wrap" end
    if type_key == 1 or type_key == 4 then return "nowrap" end
    return "pre-line"
end


function justify_word_spacings(lines, width_pt, font_size, style, family, mode, source, force_last)
    local out = {}
    if not lines or #lines == 0 then return out end
    for i = 1, #lines do out[i] = 0 end
    mode = mode or "pre-line"
    if mode == "nowrap" then return out end
    if not (width_pt and width_pt > LAYOUT_EPS) then return out end
    local prev = measure_family
    if family then measure_family = family end
    local last = {}
    local function mark(i) last[i] = true end
    local src = source == nil and "" or tostring(source)
    local function mark_wrapped(blocks, block_mode)
        local index = 0
        for _, block in ipairs(blocks) do
            local wrapped = wrap_text(block, width_pt, font_size, style, block_mode)
            if #wrapped > 0 then mark(index + #wrapped) end
            index = index + #wrapped
        end
        return index
    end
    if mode == "normal" then
        mark_wrapped({ collapse_ws(src) }, "normal")
    elseif mode == "pre" then
        local blocks = split_nl(src)
        for i = 1, #blocks do mark(i) end
    elseif mode == "pre-wrap" then
        local n = mark_wrapped(split_nl(src), "pre-wrap")
        if n ~= #lines and #lines > 0 then mark(#lines) end
    else
        local index = 0
        for _, block in ipairs(split_nl(src)) do
            local collapsed = collapse_line(block)
            local wrapped
            if collapsed == "" then
                wrapped = { "" }
            else
                wrapped = wrap_text(collapsed, width_pt, font_size, style, "normal")
            end
            if #wrapped > 0 then mark(index + #wrapped) end
            index = index + #wrapped
        end
        if index ~= #lines and #lines > 0 then mark(#lines) end
    end
    if type(force_last) == "table" then
        for i, flag in ipairs(force_last) do
            if flag then mark(i) end
        end
    end
    local any = false
    for _ in pairs(last) do any = true; break end
    if not any and #lines > 0 then mark(#lines) end
    local space_w = measure_pt(" ", font_size, style)
    for i = 1, #lines do
        if not last[i] then
            local line = lines[i] == nil and "" or tostring(lines[i])
            local parts = {}
            for word in line:gmatch("%S+") do parts[#parts + 1] = word end
            if #parts >= 2 then
                local natural = 0
                for w = 1, #parts do
                    natural = natural + measure_pt(parts[w], font_size, style)
                    if w < #parts then natural = natural + space_w end
                end
                local gaps = #parts - 1
                local slack = width_pt - natural
                if slack > LAYOUT_EPS then out[i] = slack / gaps end
            end
        end
    end
    measure_family = prev
    return out
end

function paints_lines(item, overflow)
    local tk = tonumber(item and item.typeKey)
    if tk == 1 or tk == 3 or tk == 4 or tk == 23 then return true end
    if (tk == 0 or tk == 20) and overflow ~= "Clip" and overflow ~= "clip" then return true end
    return false
end

function layout_warn(seen, key, message)
    if seen[key] then return end
    seen[key] = true
    add_log("warn", message)
end

function font_of(doc, item)
    local own = type(item.font) == "table" and item.font or nil
    local report = type(doc) == "table" and type(doc.font) == "table" and doc.font or nil
    local source = own or report or {}
    return {
        family = (type(source.family) == "string" and source.family ~= "") and source.family or "Vanadium Sans",
        size = (type(source.size) == "string" and source.size ~= "") and source.size or "10 pt",
        style = (type(source.style) == "string" and source.style ~= "") and source.style or "Normal",
    }
end

function resolve_wrap(item, seen)
    local tk = tonumber(item.typeKey) or 0
    local mode = item.wrap
    if mode == nil or mode == "" then return default_wrap(tk) end
    if WRAP_MODES[mode] then return mode end
    local fallback = default_wrap(tk)
    layout_warn(seen, "wrap:" .. tostring(item.id),
        "item " .. tostring(item.id) .. " wrap " .. tostring(mode) .. " is not a wrap mode; used " .. fallback)
    return fallback
end

function resolve_fit_height(item, seen)
    local fallback = default_fit(tonumber(item.typeKey) or 0)
    local fit = item.fitHeight
    if fit == nil or fit == "" then return fallback end
    if FIT_HEIGHTS[fit] then return fit end
    layout_warn(seen, "fit:" .. tostring(item.id),
        "item " .. tostring(item.id) .. " fitHeight " .. tostring(fit)
        .. " is not clip, grow, shrink, or both; used " .. fallback)
    return fallback
end

function resolve_valign(item, seen)
    local valign = item.valign
    if valign == nil or valign == "" then return "top" end
    if VALIGNS[valign] then return valign end
    layout_warn(seen, "valign:" .. tostring(item.id),
        "item " .. tostring(item.id) .. " valign " .. tostring(valign) .. " is not top, middle, or bottom; used top")
    return "top"
end

function resolve_multiple(item, seen)
    if item.lineHeight == nil or item.lineHeight == "" then return 1.2 end
    local value = tonumber(item.lineHeight)
    if value and value == value and value > 0 and value ~= math.huge then return value end
    layout_warn(seen, "lh:" .. tostring(item.id),
        "item " .. tostring(item.id) .. " lineHeight " .. tostring(item.lineHeight)
        .. " is not a positive multiple; used 1.2")
    return 1.2
end

function valign_offset(valign, box_pt, block_pt)
    local slack = box_pt - block_pt
    if slack <= LAYOUT_EPS then return 0 end
    if valign == "bottom" then return slack end
    if valign == "middle" then return slack / 2 end
    return 0
end

function fitted_height(overflow, stored_pt, content_pt)
    if overflow == "Grow" or overflow == "grow" or overflow == "both" then
        return math.max(stored_pt, content_pt)
    end
    return stored_pt
end

function layout_band(doc, band, entries)
    local seen, placed, reflow = {}, {}, false
    for _, entry in ipairs(entries or {}) do
        local item = entry.item or {}
        local overflow = entry.defer and "Clip" or resolve_overflow(item, seen)
        if overflow == "Grow" then reflow = true end
        local box = type(item.box) == "table" and item.box or {}
        local top = length_pt(box.top) or 0
        local height = length_pt(box.height)
        local width = length_pt(box.width) or 0
        local font = font_of(doc, item)
        local size = length_pt(font.size)
        if size == nil and font.size then
            layout_warn(seen, "size:" .. tostring(item.id),
                "item " .. tostring(item.id) .. " font size " .. tostring(font.size) .. " does not parse; used 10 pt")
        end
        local font_size = (size and size > 0) and size or 10
        local multiple = resolve_multiple(item, seen)
        local mode = resolve_wrap(item, seen)
        local valign = resolve_valign(item, seen)
        local width_only = is_width_only_wrap(mode)
        local stored_h = (height and height > 0) and height or 0

        local function measure_at(pt)
            local lead = line_height_pt(pt, multiple)
            local lines = nil
            if paints_lines(item, overflow) and not entry.defer then
                measure_family = font.family or "Helvetica"
                lines = wrap_text(entry.text or "", width, pt, font.style, mode)
                measure_family = "Helvetica"
                if overflow == "Ellipsis" then
                    lines = apply_ellipsis(lines, width, stored_h, lead, pt, font.style, font.family, width_only)
                end
            end
            return lines, lead
        end

        if not entry.defer and (overflow == "Shrink" or overflow == "Fit") then
            local min_pt = resolve_min_font_pt(item, font_size)
            local max_pt = overflow == "Fit" and resolve_max_font_pt(item, font_size) or font_size
            local lo = math.min(min_pt, max_pt)
            local hi = math.max(min_pt, max_pt)
            local set_pt = font_size
            if overflow == "Shrink" then
                local at_set_lines, at_set_lead = measure_at(set_pt)
                local content_h = math.max(1, at_set_lines and #at_set_lines or 1) * at_set_lead
                local fits_set = true
                if at_set_lines then
                    if width_only then
                        measure_family = font.family or "Helvetica"
                        fits_set = measure_pt(at_set_lines[1] or "", set_pt, font.style) <= width + LAYOUT_EPS
                        measure_family = "Helvetica"
                    else
                        fits_set = content_h <= stored_h + LAYOUT_EPS
                    end
                end
                if not fits_set then
                    local best, low, high = lo, lo, set_pt
                    for _ = 1, 16 do
                        local mid = (low + high) / 2
                        local lines, lead = measure_at(mid)
                        local ch = math.max(1, lines and #lines or 1) * lead
                        local fits = true
                        if lines then
                            if width_only then
                                measure_family = font.family or "Helvetica"
                                fits = measure_pt(lines[1] or "", mid, font.style) <= width + LAYOUT_EPS
                                measure_family = "Helvetica"
                            else
                                fits = ch <= stored_h + LAYOUT_EPS
                            end
                        end
                        if fits then best = mid; low = mid else high = mid end
                    end
                    font_size = math.max(lo, best)
                end
            else
                local best, low, high = lo, lo, hi
                for _ = 1, 16 do
                    local mid = (low + high) / 2
                    local lines, lead = measure_at(mid)
                    local ch = math.max(1, lines and #lines or 1) * lead
                    local fits = true
                    if lines then
                        if width_only then
                            measure_family = font.family or "Helvetica"
                            fits = measure_pt(lines[1] or "", mid, font.style) <= width + LAYOUT_EPS
                            measure_family = "Helvetica"
                        else
                            fits = ch <= stored_h + LAYOUT_EPS
                        end
                    end
                    if fits then best = mid; low = mid else high = mid end
                end
                font_size = math.max(lo, math.min(hi, best))
            end
        end

        local leading = line_height_pt(font_size, multiple)
        local lines = nil
        if paints_lines(item, overflow) and not entry.defer then
            measure_family = font.family or "Helvetica"
            lines = wrap_text(entry.text or "", width, font_size, font.style, mode)
            measure_family = "Helvetica"
            if overflow == "Ellipsis" then
                lines = apply_ellipsis(lines, width, stored_h, leading, font_size, font.style, font.family, width_only)
            end
        end
        local content_h = math.max(1, lines and #lines or 1) * leading
        local used_h = (entry.defer or not lines) and stored_h or fitted_height(overflow, stored_h, content_h)
        local painted = nil
        if lines then
            local visible = {}
            for index, line in ipairs(lines) do
                local i = index - 1
                if (i + 1) * leading <= used_h + LAYOUT_EPS or (i == 0 and used_h > LAYOUT_EPS) then
                    visible[#visible + 1] = line
                end
            end
            painted = {}
            for index, line in ipairs(visible) do
                local i = index - 1
                if (i + 1) * leading <= used_h + LAYOUT_EPS then
                    painted[#painted + 1] = line
                end
            end
        end
        local out_font = {
            family = font.family,
            size = string.format("%.4f pt", font_size):gsub("0+ pt", " pt"):gsub(" %.pt", " pt"),
            style = font.style,
        }
        out_font.size = fmt_pt(font_size)
        local fit = (overflow == "Grow") and "grow" or "clip"
        placed[#placed + 1] = {
            item = item, fit = fit, overflow = overflow, lines = painted, font = out_font, leading = leading,
            valign = valign, orig_top = top, stored_h = stored_h, used_h = used_h,
            delta = used_h - stored_h,
            width = width, font_size = font_size, text = entry.text or "", mode = mode,
        }
    end
    for _, row in ipairs(placed) do
        local shift = 0
        for _, other in ipairs(placed) do
            if other ~= row and other.orig_top + other.stored_h <= row.orig_top + LAYOUT_EPS then
                shift = shift + other.delta
            end
        end
        row.used_top = row.orig_top + shift
    end
    local stored_band = length_pt(band and band.height)
    local used_pt = (stored_band and stored_band > 0) and stored_band or 0
    if reflow then
        local bottom = 0
        for _, row in ipairs(placed) do
            local edge = row.used_top + row.used_h
            if edge > bottom then bottom = edge end
        end
        used_pt = math.max(MIN_BAND_PT, bottom)
    end
    local items = {}
    for _, row in ipairs(placed) do
        local block = row.lines and (#row.lines * row.leading) or 0
        local align = (type(row.item.align) == "string" and row.item.align ~= "") and row.item.align or "Left"
        local word_spacings = nil
        if align == "Justified" and row.lines and #row.lines > 0 then
            local force_last = nil
            if row.overflow == "Ellipsis" then
                force_last = {}
                for i = 1, #row.lines do force_last[i] = (i == #row.lines) end
            end
            word_spacings = justify_word_spacings(
                row.lines, row.width or 0, row.font_size or 10, row.font.style, row.font.family,
                row.mode or "pre-line", row.text or "", force_last)
        end
        items[#items + 1] = {
            lines = row.lines,
            align = align,
            wordSpacings = word_spacings,
            widthPt = row.width,
            valign = row.valign,
            lineHeightPt = row.leading,
            offsetPt = row.lines and valign_offset(row.valign, row.used_h, block) or 0,
            grown = math.abs(row.used_h - row.stored_h) > LAYOUT_EPS,
            topPt = row.used_top,
            heightPt = row.used_h,
            -- Shrink/Fit resolve a new size; Grow keeps family. Paging copies this onto item_out.
            font = row.font,
            overflow = row.overflow,
        }
    end
    return { reflow = reflow, usedPt = used_pt, items = items }
end



]==]

-- ----------------------------------------------------------------------------
-- Forward: amend Reports.Render.Layout
-- ----------------------------------------------------------------------------
table.insert(queries,{sql=[====[

    INSERT INTO ${SCHEMA}${QUERIES} (
        ${QUERIES_INSERT}
    )
    WITH next_query_id AS (
        SELECT COALESCE(MAX(query_id), 0) + 1 AS new_query_id
        FROM ${SCHEMA}${QUERIES}
    )
    SELECT
        new_query_id                                                        AS query_id,
        ${MIGRATION}                                                        AS query_ref,
        ${STATUS_ACTIVE}                                                    AS query_status_a27,
        ${TYPE_FORWARD_MIGRATION}                                           AS query_type_a28,
        ${DIALECT}                                                          AS query_dialect_a30,
        ${QTC_SLOW}                                                         AS query_queue_a58,
        ${TIMEOUT}                                                          AS query_timeout,
        [=[
            UPDATE ${SCHEMA}scripts
            SET code = [==[
]====] .. sql_new .. [====[
            ]==]
            WHERE group_name = '${GROUP_NAME}'
              AND script_name = '${SCRIPT_NAME}';

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Amend Reports.Render.Layout Justified'                                  AS name,
        [=[
            # Forward Migration ${MIGRATION}: Phase 81: Justified word spacings

            layout_band returns wordSpacings for Justified lines (Metrics advances).
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]====]})

-- ----------------------------------------------------------------------------
-- Reverse: restore prior Reports.Render.Layout body
-- ----------------------------------------------------------------------------
table.insert(queries,{sql=[====[

    INSERT INTO ${SCHEMA}${QUERIES} (
        ${QUERIES_INSERT}
    )
    WITH next_query_id AS (
        SELECT COALESCE(MAX(query_id), 0) + 1 AS new_query_id
        FROM ${SCHEMA}${QUERIES}
    )
    SELECT
        new_query_id                                                        AS query_id,
        ${MIGRATION}                                                        AS query_ref,
        ${STATUS_ACTIVE}                                                    AS query_status_a27,
        ${TYPE_REVERSE_MIGRATION}                                           AS query_type_a28,
        ${DIALECT}                                                          AS query_dialect_a30,
        ${QTC_SLOW}                                                         AS query_queue_a58,
        ${TIMEOUT}                                                          AS query_timeout,
        [=[
            UPDATE ${SCHEMA}scripts
            SET code = [==[
]====] .. sql_old .. [====[
            ]==]
            WHERE group_name = '${GROUP_NAME}'
              AND script_name = '${SCRIPT_NAME}';

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Restore prior Reports.Render.Layout'                                 AS name,
        [=[
            # Reverse Migration ${MIGRATION}: restore Reports.Render.Layout

            Puts the Phase-80 Layout body back.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]====]})

return queries end
