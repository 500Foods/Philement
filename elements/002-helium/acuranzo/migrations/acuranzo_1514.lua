-- Migration: acuranzo_1514.lua
-- Reports.Render.Paging (Reporting)
-- Phase 80: Paging copies Layout font (Shrink/Fit size)
--
-- Rule 12: one Render module. sql_old is the current body. Reverse restores it.
-- QueryRef 175 stays unused. No lookup.
--
-- luacheck: no max line length
-- luacheck: no unused args
--
-- CHANGELOG
-- 1.0.0 - 2026-11-09 - Phase 80 Render.Paging Layout font

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "scripts"
cfg.MIGRATION = "1514"
cfg.GROUP_NAME = "Reports"
cfg.SCRIPT_NAME = "Render.Paging"

local sql_old = [==[


-- Reports.Render.Paging (Reporting). Always loaded: measure, emit, page breaks, page totals, page number/count/label.
-- Loaded by Reports.Render into its per-request environment.
-- Names defined here are shared with Reports.Render and its other modules.

-- Paging (Phase 21): stored band heights against the page content box.
UNIT_PT = { ["in"] = 72, mm = 72 / 25.4, cm = 72 / 2.54, pt = 1, px = 0.75 }
function to_pt(len)
    if type(len) == "number" then return len end
    if type(len) ~= "string" then return nil end
    local n, u = len:match("^%s*([%d%.]+)%s*(%a+)%s*$")
    n = tonumber(n)
    if not n or not UNIT_PT[u] then return nil end
    return n * UNIT_PT[u]
end
pg = type(def.page) == "table" and def.page or {}
mg = type(pg.margin) == "table" and pg.margin or {}
box_h = to_pt(pg.height)
if box_h then box_h = box_h - (to_pt(mg.top) or 0) - (to_pt(mg.bottom) or 0) end
if not box_h or box_h <= 0 then
    add_log("warn", "page height " .. tostring(pg.height) .. " leaves no content box; no page breaks")
    box_h = math.huge
end
warned_height = {}
function stored_h(band)
    local h = to_pt(band.height)
    if h then return h end
    if not warned_height[band] then
        warned_height[band] = true
        add_log("warn", "band " .. tostring(band.id) .. " height " .. tostring(band.height)
            .. " does not parse; counted as 0")
    end
    return 0
end
layout_cache = {}
function measure_band(band, row, aggs)
    local by_band = layout_cache[band]
    if not by_band then
        by_band = {}
        layout_cache[band] = by_band
    end
    local row_key = row or false
    local by_row = by_band[row_key]
    if not by_row then
        by_row = {}
        by_band[row_key] = by_row
    end
    local agg_key = aggs or false
    local hit = by_row[agg_key]
    if hit then return hit end
    local saved = ctx
    if type(aggs) == "table" then
        ctx = { aggregates = aggs, row = row }
    else
        ctx = { row = row }
    end
    local outs, entries, sources = {}, {}, {}
    for _, item in ipairs(band.items or {}) do
        if type(item) == "table" then
            local o = item_out(item, row, band.id, band.typeKey)
            if o then
                local tk = o.typeKey
                local defer = PAGE_KEYS[tk]
                    or ((band.typeKey == 13 or band.typeKey == 17)
                        and (tk == 34 or tk == 35 or tk == 38))
                outs[#outs + 1] = o
                sources[#sources + 1] = item
                entries[#entries + 1] = { item = item, text = o.text or "", defer = defer }
            end
        end
    end
    ctx = saved
    local laid = layout_band(def, band, entries)
    local result = {
        reflow = laid.reflow, usedPt = laid.usedPt,
        outs = outs, laid = laid.items, sources = sources,
    }
    by_row[agg_key] = result
    return result
end
function h_of(band, row, aggs)
    local measured = measure_band(band, row, aggs)
    if measured.reflow then return measured.usedPt end
    return stored_h(band)
end
EPS = 1e-6

page_detail = {}
page_agg_src = {}
pages, out, filled, body_start = {}, nil, 0, 0
function note_detail(row)
    local list = page_detail[#pages]
    if not list or row == nil or list[#list] == row then return end
    list[#list + 1] = row
end
function clone_out(o)
    local copy = {}
    for k, v in pairs(o) do
        if k == "font" and type(v) == "table" then
            copy.font = { family = v.family, size = v.size, style = v.style }
        elseif k == "box" and type(v) == "table" then
            local box = {}
            for bk, bv in pairs(v) do box[bk] = bv end
            copy.box = box
        else
            copy[k] = v
        end
    end
    return copy
end
function emit(band, row, aggs)
    if type(aggs) == "table" then
        ctx = { aggregates = aggs, row = row }
    else
        ctx = { row = row }
    end
    local measured = measure_band(band, row, aggs)
    local items = {}
    for i, o in ipairs(measured.outs) do
        local copy = clone_out(o)
        local lay = measured.laid[i]
        local src = measured.sources[i]
        if lay and lay.lines ~= nil then
            local lines = {}
            for _, line in ipairs(lay.lines) do lines[#lines + 1] = line end
            copy.lines = lines
            copy.valign = lay.valign
            copy.lineHeightPt = lay.lineHeightPt
            copy.offsetPt = lay.offsetPt
            -- Phase 79: keep the item font on grown lines (Metrics measures the real face)
        end
        if measured.reflow and lay and type(copy.box) == "table" then
            copy.box.top = fmt_pt(lay.topPt)
            copy.box.height = fmt_pt(lay.heightPt)
        end
        if page_src[o] then page_src[copy] = page_src[o] end
        items[#items + 1] = copy
        if src and (band.typeKey == 13 or band.typeKey == 17)
            and (copy.typeKey == 35 or copy.typeKey == 34 or copy.typeKey == 38)
            and (type(src["function"]) ~= "string" or src["function"] == "")
            and type(src.field) == "string" and src.field ~= ""
            and AGG_OPS[src.op] then
            page_agg_src[copy] = src
        end
    end
    local band_out = { id = band.id, typeKey = band.typeKey, height = band.height, items = items }
    if measured.reflow then band_out.usedHeight = fmt_pt(measured.usedPt) end
    out[#out + 1] = band_out
    if measured.reflow then
        filled = filled + measured.usedPt
    else
        filled = filled + stored_h(band)
    end
    if band.typeKey == 15 then note_detail(row) end
end
function emit_all(list, row, aggs)
    for _, b in ipairs(list) do emit(b, row, aggs) end
end
function sum_at(list, row, aggs)
    local s = 0
    for _, b in ipairs(list) do s = s + h_of(b, row, aggs) end
    return s
end

first = rows[1]
function open_page(row)
    out, filled = {}, 0
    pages[#pages + 1] = { bands = out }
    page_detail[#pages] = {}
    if #pages == 1 then emit_all(rh, first, report_aggs) end
    emit_all(ph, row, nil)
    body_start = filled
end
-- Make room for `need` before placing; break the page if the body already has bands.
function ensure(need, row, last, what)
    local foot = sum_at(pf, last, nil)
    if filled + need + foot <= box_h + EPS then return end
    if filled > body_start + EPS then
        emit_all(pf, last, nil)
        open_page(row)
    end
    if filled + need + foot > box_h + EPS then
        add_log("warn", what .. " does not fit on an empty page; placed anyway")
    end
end


function key(row, lv)
    local f = levels[lv].header.field
    if f == nil or f == "" or type(row) ~= "table" then return "" end
    return plain(row[f])
end
function row_path(row, lv)
    local parts = {}
    for i = 1, lv do parts[#parts + 1] = key(row, i) end
    return table.concat(parts, "\0")
end
span_lists = {}
for lv = 1, #levels do
    local specs = collect_aggs({ levels[lv].header })
    if levels[lv].footer then
        for _, spec in ipairs(collect_aggs({ levels[lv].footer })) do
            specs[#specs + 1] = spec
        end
    end
    local spans, cur, cur_path = {}, nil, nil
    for _, row in ipairs(rows) do
        local path = row_path(row, lv)
        if not cur or path ~= cur_path then
            cur = { path = path, rows = {} }
            spans[#spans + 1] = cur
            cur_path = path
        end
        cur.rows[#cur.rows + 1] = row
    end
    for _, span in ipairs(spans) do
        span.aggs = fill_aggs(specs, span.rows)
    end
    span_lists[lv] = spans
end
span_at = {}
group_aggs = function(lv, row)
    local path = row_path(row, lv)
    local spans = span_lists[lv] or {}
    local i = span_at[lv] or 1
    while spans[i] and spans[i].path ~= path do i = i + 1 end
    if spans[i] then span_at[lv] = i end
    return spans[i] and spans[i].aggs or {}
end
report_specs = collect_aggs(rh)
for _, spec in ipairs(collect_aggs(rf)) do report_specs[#report_specs + 1] = spec end
report_aggs = fill_aggs(report_specs, rows)

open_page(first)

prev = nil
for _, row in ipairs(rows) do
    local brk = #levels + 1
    if prev == nil then
        brk = 1
    else
        for lv = 1, #levels do
            if key(row, lv) ~= key(prev, lv) then brk = lv break end
        end
    end
    if prev ~= nil then
        for lv = #levels, brk, -1 do
            local f = levels[lv].footer
            if f then
                ensure(h_of(f, prev, group_aggs(lv, prev)), prev, prev, "group footer " .. tostring(f.id))
                emit(f, prev, group_aggs(lv, prev))
            end
        end
    end
    if brk <= #levels then
        -- Group headers travel with the first detail row of the group.
        local need, ids = sum_at(details, row, nil), {}
        for lv = brk, #levels do
            need = need + h_of(levels[lv].header, row, group_aggs(lv, row))
            ids[#ids + 1] = tostring(levels[lv].header.id)
        end
        ensure(need, row, prev or first, "group header " .. table.concat(ids, ", ") .. " with its first detail row")
        for lv = brk, #levels do emit(levels[lv].header, row, group_aggs(lv, row)) end
        emit_all(details, row)
    else
        for _, d in ipairs(details) do
            ensure(h_of(d, row, nil), row, prev, "detail " .. tostring(d.id))
            emit(d, row, nil)
        end
    end
    prev = row
end
if prev ~= nil then
    for lv = #levels, 1, -1 do
        local f = levels[lv].footer
        if f then
            ensure(h_of(f, prev, group_aggs(lv, prev)), prev, prev, "group footer " .. tostring(f.id))
            emit(f, prev, group_aggs(lv, prev))
        end
    end
end

-- Report footer sits on the last page; it prints after the page footer as before.
last = prev or first
rf_h = sum_at(rf, last, report_aggs)
if rf_h > 0 then ensure(rf_h, last, last, "report footer") end
emit_all(pf, last, nil)
emit_all(rf, last, report_aggs)

-- Page totals (Phase 39): detail rows placed on that page.
-- The page header and the page footer share one reduction.
page_specs = collect_aggs(ph)
for _, spec in ipairs(collect_aggs(pf)) do page_specs[#page_specs + 1] = spec end
for n, page in ipairs(pages) do
    local aggs = fill_aggs(page_specs, page_detail[n] or {})
    for _, band in ipairs(page.bands) do
        if band.typeKey == 13 or band.typeKey == 17 then
            for _, o in ipairs(band.items) do
                local src = page_agg_src[o]
                if src then
                    o.text = format_aggregate(agg_raw(aggs, src), src, band.id)
                end
            end
        end
    end
end

-- Page number / count / label (Phase 23): fill once the page list exists.
count = #pages
for n, page in ipairs(pages) do
    for _, band in ipairs(page.bands) do
        for _, o in ipairs(band.items) do
            local src = page_src[o]
            if src then
                if o.typeKey == 64 then
                    local t = src.template
                    if type(t) ~= "string" or t == "" then t = PAGE_TEMPLATE_DEFAULT end
                    o.text = (t:gsub("{n}", tostring(n)):gsub("{c}", tostring(count)))
                else
                    local v = o.typeKey == 61 and n or count
                    o.text = format_value(v, 21, src.mask, o.id, band.id)
                end
            end
        end
    end
end

]==]

local sql_new = [==[


-- Reports.Render.Paging (Reporting). Always loaded: measure, emit, page breaks, page totals, page number/count/label.
-- Loaded by Reports.Render into its per-request environment.
-- Names defined here are shared with Reports.Render and its other modules.

-- Paging (Phase 21): stored band heights against the page content box.
UNIT_PT = { ["in"] = 72, mm = 72 / 25.4, cm = 72 / 2.54, pt = 1, px = 0.75 }
function to_pt(len)
    if type(len) == "number" then return len end
    if type(len) ~= "string" then return nil end
    local n, u = len:match("^%s*([%d%.]+)%s*(%a+)%s*$")
    n = tonumber(n)
    if not n or not UNIT_PT[u] then return nil end
    return n * UNIT_PT[u]
end
pg = type(def.page) == "table" and def.page or {}
mg = type(pg.margin) == "table" and pg.margin or {}
box_h = to_pt(pg.height)
if box_h then box_h = box_h - (to_pt(mg.top) or 0) - (to_pt(mg.bottom) or 0) end
if not box_h or box_h <= 0 then
    add_log("warn", "page height " .. tostring(pg.height) .. " leaves no content box; no page breaks")
    box_h = math.huge
end
warned_height = {}
function stored_h(band)
    local h = to_pt(band.height)
    if h then return h end
    if not warned_height[band] then
        warned_height[band] = true
        add_log("warn", "band " .. tostring(band.id) .. " height " .. tostring(band.height)
            .. " does not parse; counted as 0")
    end
    return 0
end
layout_cache = {}
function measure_band(band, row, aggs)
    local by_band = layout_cache[band]
    if not by_band then
        by_band = {}
        layout_cache[band] = by_band
    end
    local row_key = row or false
    local by_row = by_band[row_key]
    if not by_row then
        by_row = {}
        by_band[row_key] = by_row
    end
    local agg_key = aggs or false
    local hit = by_row[agg_key]
    if hit then return hit end
    local saved = ctx
    if type(aggs) == "table" then
        ctx = { aggregates = aggs, row = row }
    else
        ctx = { row = row }
    end
    local outs, entries, sources = {}, {}, {}
    for _, item in ipairs(band.items or {}) do
        if type(item) == "table" then
            local o = item_out(item, row, band.id, band.typeKey)
            if o then
                local tk = o.typeKey
                local defer = PAGE_KEYS[tk]
                    or ((band.typeKey == 13 or band.typeKey == 17)
                        and (tk == 34 or tk == 35 or tk == 38))
                outs[#outs + 1] = o
                sources[#sources + 1] = item
                entries[#entries + 1] = { item = item, text = o.text or "", defer = defer }
            end
        end
    end
    ctx = saved
    local laid = layout_band(def, band, entries)
    local result = {
        reflow = laid.reflow, usedPt = laid.usedPt,
        outs = outs, laid = laid.items, sources = sources,
    }
    by_row[agg_key] = result
    return result
end
function h_of(band, row, aggs)
    local measured = measure_band(band, row, aggs)
    if measured.reflow then return measured.usedPt end
    return stored_h(band)
end
EPS = 1e-6

page_detail = {}
page_agg_src = {}
pages, out, filled, body_start = {}, nil, 0, 0
function note_detail(row)
    local list = page_detail[#pages]
    if not list or row == nil or list[#list] == row then return end
    list[#list + 1] = row
end
function clone_out(o)
    local copy = {}
    for k, v in pairs(o) do
        if k == "font" and type(v) == "table" then
            copy.font = { family = v.family, size = v.size, style = v.style }
        elseif k == "box" and type(v) == "table" then
            local box = {}
            for bk, bv in pairs(v) do box[bk] = bv end
            copy.box = box
        else
            copy[k] = v
        end
    end
    return copy
end
function emit(band, row, aggs)
    if type(aggs) == "table" then
        ctx = { aggregates = aggs, row = row }
    else
        ctx = { row = row }
    end
    local measured = measure_band(band, row, aggs)
    local items = {}
    for i, o in ipairs(measured.outs) do
        local copy = clone_out(o)
        local lay = measured.laid[i]
        local src = measured.sources[i]
        if lay and lay.lines ~= nil then
            local lines = {}
            for _, line in ipairs(lay.lines) do lines[#lines + 1] = line end
            copy.lines = lines
            copy.valign = lay.valign
            copy.lineHeightPt = lay.lineHeightPt
            copy.offsetPt = lay.offsetPt
            -- Phase 80: Layout may resolve Shrink/Fit font.size; Grow keeps family.
            if type(lay.font) == "table" then
                copy.font = {
                    family = lay.font.family or (copy.font and copy.font.family),
                    size = lay.font.size or (copy.font and copy.font.size),
                    style = lay.font.style or (copy.font and copy.font.style),
                }
            end
            if lay.overflow then copy.overflow = lay.overflow end
        elseif lay and type(lay.font) == "table" then
            -- Clip / no lines still picks up Shrink/Fit size when Layout set one.
            copy.font = {
                family = lay.font.family or (copy.font and copy.font.family),
                size = lay.font.size or (copy.font and copy.font.size),
                style = lay.font.style or (copy.font and copy.font.style),
            }
            if lay.overflow then copy.overflow = lay.overflow end
        end
        if measured.reflow and lay and type(copy.box) == "table" then
            copy.box.top = fmt_pt(lay.topPt)
            copy.box.height = fmt_pt(lay.heightPt)
        end
        if page_src[o] then page_src[copy] = page_src[o] end
        items[#items + 1] = copy
        if src and (band.typeKey == 13 or band.typeKey == 17)
            and (copy.typeKey == 35 or copy.typeKey == 34 or copy.typeKey == 38)
            and (type(src["function"]) ~= "string" or src["function"] == "")
            and type(src.field) == "string" and src.field ~= ""
            and AGG_OPS[src.op] then
            page_agg_src[copy] = src
        end
    end
    local band_out = { id = band.id, typeKey = band.typeKey, height = band.height, items = items }
    if measured.reflow then band_out.usedHeight = fmt_pt(measured.usedPt) end
    out[#out + 1] = band_out
    if measured.reflow then
        filled = filled + measured.usedPt
    else
        filled = filled + stored_h(band)
    end
    if band.typeKey == 15 then note_detail(row) end
end
function emit_all(list, row, aggs)
    for _, b in ipairs(list) do emit(b, row, aggs) end
end
function sum_at(list, row, aggs)
    local s = 0
    for _, b in ipairs(list) do s = s + h_of(b, row, aggs) end
    return s
end

first = rows[1]
function open_page(row)
    out, filled = {}, 0
    pages[#pages + 1] = { bands = out }
    page_detail[#pages] = {}
    if #pages == 1 then emit_all(rh, first, report_aggs) end
    emit_all(ph, row, nil)
    body_start = filled
end
-- Make room for `need` before placing; break the page if the body already has bands.
function ensure(need, row, last, what)
    local foot = sum_at(pf, last, nil)
    if filled + need + foot <= box_h + EPS then return end
    if filled > body_start + EPS then
        emit_all(pf, last, nil)
        open_page(row)
    end
    if filled + need + foot > box_h + EPS then
        add_log("warn", what .. " does not fit on an empty page; placed anyway")
    end
end


function key(row, lv)
    local f = levels[lv].header.field
    if f == nil or f == "" or type(row) ~= "table" then return "" end
    return plain(row[f])
end
function row_path(row, lv)
    local parts = {}
    for i = 1, lv do parts[#parts + 1] = key(row, i) end
    return table.concat(parts, "\0")
end
span_lists = {}
for lv = 1, #levels do
    local specs = collect_aggs({ levels[lv].header })
    if levels[lv].footer then
        for _, spec in ipairs(collect_aggs({ levels[lv].footer })) do
            specs[#specs + 1] = spec
        end
    end
    local spans, cur, cur_path = {}, nil, nil
    for _, row in ipairs(rows) do
        local path = row_path(row, lv)
        if not cur or path ~= cur_path then
            cur = { path = path, rows = {} }
            spans[#spans + 1] = cur
            cur_path = path
        end
        cur.rows[#cur.rows + 1] = row
    end
    for _, span in ipairs(spans) do
        span.aggs = fill_aggs(specs, span.rows)
    end
    span_lists[lv] = spans
end
span_at = {}
group_aggs = function(lv, row)
    local path = row_path(row, lv)
    local spans = span_lists[lv] or {}
    local i = span_at[lv] or 1
    while spans[i] and spans[i].path ~= path do i = i + 1 end
    if spans[i] then span_at[lv] = i end
    return spans[i] and spans[i].aggs or {}
end
report_specs = collect_aggs(rh)
for _, spec in ipairs(collect_aggs(rf)) do report_specs[#report_specs + 1] = spec end
report_aggs = fill_aggs(report_specs, rows)

open_page(first)

prev = nil
for _, row in ipairs(rows) do
    local brk = #levels + 1
    if prev == nil then
        brk = 1
    else
        for lv = 1, #levels do
            if key(row, lv) ~= key(prev, lv) then brk = lv break end
        end
    end
    if prev ~= nil then
        for lv = #levels, brk, -1 do
            local f = levels[lv].footer
            if f then
                ensure(h_of(f, prev, group_aggs(lv, prev)), prev, prev, "group footer " .. tostring(f.id))
                emit(f, prev, group_aggs(lv, prev))
            end
        end
    end
    if brk <= #levels then
        -- Group headers travel with the first detail row of the group.
        local need, ids = sum_at(details, row, nil), {}
        for lv = brk, #levels do
            need = need + h_of(levels[lv].header, row, group_aggs(lv, row))
            ids[#ids + 1] = tostring(levels[lv].header.id)
        end
        ensure(need, row, prev or first, "group header " .. table.concat(ids, ", ") .. " with its first detail row")
        for lv = brk, #levels do emit(levels[lv].header, row, group_aggs(lv, row)) end
        emit_all(details, row)
    else
        for _, d in ipairs(details) do
            ensure(h_of(d, row, nil), row, prev, "detail " .. tostring(d.id))
            emit(d, row, nil)
        end
    end
    prev = row
end
if prev ~= nil then
    for lv = #levels, 1, -1 do
        local f = levels[lv].footer
        if f then
            ensure(h_of(f, prev, group_aggs(lv, prev)), prev, prev, "group footer " .. tostring(f.id))
            emit(f, prev, group_aggs(lv, prev))
        end
    end
end

-- Report footer sits on the last page; it prints after the page footer as before.
last = prev or first
rf_h = sum_at(rf, last, report_aggs)
if rf_h > 0 then ensure(rf_h, last, last, "report footer") end
emit_all(pf, last, nil)
emit_all(rf, last, report_aggs)

-- Page totals (Phase 39): detail rows placed on that page.
-- The page header and the page footer share one reduction.
page_specs = collect_aggs(ph)
for _, spec in ipairs(collect_aggs(pf)) do page_specs[#page_specs + 1] = spec end
for n, page in ipairs(pages) do
    local aggs = fill_aggs(page_specs, page_detail[n] or {})
    for _, band in ipairs(page.bands) do
        if band.typeKey == 13 or band.typeKey == 17 then
            for _, o in ipairs(band.items) do
                local src = page_agg_src[o]
                if src then
                    o.text = format_aggregate(agg_raw(aggs, src), src, band.id)
                end
            end
        end
    end
end

-- Page number / count / label (Phase 23): fill once the page list exists.
count = #pages
for n, page in ipairs(pages) do
    for _, band in ipairs(page.bands) do
        for _, o in ipairs(band.items) do
            local src = page_src[o]
            if src then
                if o.typeKey == 64 then
                    local t = src.template
                    if type(t) ~= "string" or t == "" then t = PAGE_TEMPLATE_DEFAULT end
                    o.text = (t:gsub("{n}", tostring(n)):gsub("{c}", tostring(count)))
                else
                    local v = o.typeKey == 61 and n or count
                    o.text = format_value(v, 21, src.mask, o.id, band.id)
                end
            end
        end
    end
end

]==]

-- ----------------------------------------------------------------------------
-- Forward: amend Reports.Render.Paging
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
        'Amend Reports.Render.Paging Layout font'                                  AS name,
        [=[
            # Forward Migration ${MIGRATION}: Amend Reports.Render.Paging

            Copies Layout font (Shrink/Fit size) onto item_out; Grow keeps family (no Helvetica force).
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]====]})

-- ----------------------------------------------------------------------------
-- Reverse: restore prior Reports.Render.Paging body
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
        'Restore prior Reports.Render.Paging'                                 AS name,
        [=[
            # Reverse Migration ${MIGRATION}: restore Reports.Render.Paging

            Puts the prior Paging body back (grown items forced Helvetica).
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]====]})

return queries end
