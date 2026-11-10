-- Migration: acuranzo_1518.lua
-- Reports.Render.Shapes (Reporting)
-- Fix line Side placement (normalize + stroke inset)
--
-- Rule 12: one Render module. sql_old is the current body. Reverse restores it.
-- QueryRef 175 stays unused. No lookup.
--
-- luacheck: no max line length
-- luacheck: no unused args
--
-- CHANGELOG
-- 1.0.0 - 2026-11-09 - Line side case + inset inside box

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "scripts"
cfg.MIGRATION = "1518"
cfg.GROUP_NAME = "Reports"
cfg.SCRIPT_NAME = "Render.Shapes"

local sql_old = [==[

-- Reports.Render.Shapes (Reporting). Lazy: static and field shapes, fill, gradient, lookup 078 kinds.
-- Loaded by Reports.Render into its per-request environment.
-- Names defined here are shared with Reports.Render and its other modules.

-- Phase 44a: static shapes. Geometry is code. Path kinds use lookup 078
-- when that lookup is loaded, otherwise these commands. PDF does not draw them.
shape_rows = {}
shape_loaded = false
shape_catalog_on = false
SHAPE_KAPPA = 0.5522847498
SHAPE_GEOMETRY = { line = true, rectangle = true, roundrect = true, ellipse = true }
SHAPE_EMBEDDED = {
    x = { closed = false, commands = { { "M", 0, 0 }, { "L", 1, 1 }, { "M", 1, 0 }, { "L", 0, 1 } } },
    trapezoid = { closed = true, commands = { { "M", 0.25, 0 }, { "L", 0.75, 0 }, { "L", 1, 1 }, { "L", 0, 1 }, { "Z" } } },
    star = { closed = true, commands = {
        { "M", 0.5, 0 }, { "L", 0.618034, 0.381966 }, { "L", 1, 0.381966 }, { "L", 0.690983, 0.618034 },
        { "L", 0.809017, 1 }, { "L", 0.5, 0.763932 }, { "L", 0.190983, 1 }, { "L", 0.309017, 0.618034 },
        { "L", 0, 0.381966 }, { "L", 0.381966, 0.381966 }, { "Z" },
    } },
    pentagon = { closed = true, commands = {
        { "M", 0.5, 0 }, { "L", 1, 0.381966 }, { "L", 0.809017, 1 }, { "L", 0.190983, 1 }, { "L", 0, 0.381966 }, { "Z" },
    } },
    hexagon = { closed = true, commands = {
        { "M", 0.5, 0 }, { "L", 1, 0.25 }, { "L", 1, 0.75 }, { "L", 0.5, 1 }, { "L", 0, 0.75 }, { "L", 0, 0.25 }, { "Z" },
    } },
}

function shape_len(value, fallback)
    if type(value) ~= "string" or value:match("^%s*$") then return fallback end
    local px = shape_px(value)
    if not px then return fallback end
    return px
end

function shape_hex(value, fallback)
    if type(value) == "string" and value:match("^#[0-9A-Fa-f][0-9A-Fa-f][0-9A-Fa-f][0-9A-Fa-f][0-9A-Fa-f][0-9A-Fa-f]$") then
        return value:lower()
    end
    return fallback
end

function shape_commands_ok(commands)
    if type(commands) ~= "table" or #commands == 0 then return false end
    local arity = { M = 2, L = 2, C = 6, Z = 0 }
    for _, cmd in ipairs(commands) do
        if type(cmd) ~= "table" then return false end
        local need = arity[cmd[1]]
        if need == nil or #cmd ~= need + 1 then return false end
        for i = 2, #cmd do
            local num = cmd[i]
            if type(num) ~= "number" or num ~= num or num < 0 or num > 1 then return false end
        end
    end
    return true
end

function shape_source(kind)
    if SHAPE_GEOMETRY[kind] then return { geometry = kind } end
    local row = shape_rows[kind]
    if row then
        if row.bad then return { bad = true } end
        return { commands = row.commands, closed = row.closed }
    end
    local embedded = SHAPE_EMBEDDED[kind]
    if embedded then return { commands = embedded.commands, closed = embedded.closed } end
    return nil
end

function shape_scale(commands, w, h)
    local out = {}
    for _, cmd in ipairs(commands) do
        if cmd[1] == "Z" then
            out[#out + 1] = { "Z" }
        else
            local row = { cmd[1] }
            for i = 2, #cmd do
                local axis = ((i - 1) % 2 == 1) and w or h
                row[#row + 1] = axis * cmd[i]
            end
            out[#out + 1] = row
        end
    end
    return out
end

function shape_d(commands)
    local parts = {}
    for _, cmd in ipairs(commands) do
        if cmd[1] == "Z" then
            parts[#parts + 1] = "Z"
        else
            local nums = {}
            for i = 2, #cmd do nums[#nums + 1] = shape_fmt(cmd[i]) end
            parts[#parts + 1] = cmd[1] .. " " .. table.concat(nums, " ")
        end
    end
    return table.concat(parts, " ")
end

function shape_clamp(w, h, radii)
    local factor = 1
    local pairs = {
        { radii.tl + radii.tr, w },
        { radii.bl + radii.br, w },
        { radii.tl + radii.bl, h },
        { radii.tr + radii.br, h },
    }
    for _, pair in ipairs(pairs) do
        if pair[1] > 0 and pair[2] / pair[1] < factor then factor = pair[2] / pair[1] end
    end
    if factor ~= factor or factor < 0 then factor = 0 end
    if factor > 1 then factor = 1 end
    return {
        tl = radii.tl * factor, tr = radii.tr * factor,
        br = radii.br * factor, bl = radii.bl * factor,
    }
end

function shape_round(w, h, r)
    local tl, tr, br, bl = r.tl, r.tr, r.br, r.bl
    return {
        { "M", tl, 0 },
        { "L", w - tr, 0 },
        { "C", w - tr * (1 - SHAPE_KAPPA), 0, w, tr * (1 - SHAPE_KAPPA), w, tr },
        { "L", w, h - br },
        { "C", w, h - br * (1 - SHAPE_KAPPA), w - br * (1 - SHAPE_KAPPA), h, w - br, h },
        { "L", bl, h },
        { "C", bl * (1 - SHAPE_KAPPA), h, 0, h - bl * (1 - SHAPE_KAPPA), 0, h - bl },
        { "L", 0, tl },
        { "C", 0, tl * (1 - SHAPE_KAPPA), tl * (1 - SHAPE_KAPPA), 0, tl, 0 },
        { "Z" },
    }
end

function shape_ellipse(w, h)
    local cx, cy, rx, ry = w / 2, h / 2, w / 2, h / 2
    return {
        { "M", cx, cy - ry },
        { "C", cx + SHAPE_KAPPA * rx, cy - ry, cx + rx, cy - SHAPE_KAPPA * ry, cx + rx, cy },
        { "C", cx + rx, cy + SHAPE_KAPPA * ry, cx + SHAPE_KAPPA * rx, cy + ry, cx, cy + ry },
        { "C", cx - SHAPE_KAPPA * rx, cy + ry, cx - rx, cy + SHAPE_KAPPA * ry, cx - rx, cy },
        { "C", cx - rx, cy - SHAPE_KAPPA * ry, cx - SHAPE_KAPPA * rx, cy - ry, cx, cy - ry },
        { "Z" },
    }
end

function shape_line(w, h, side)
    if side == "top" then return { { "M", 0, 0 }, { "L", w, 0 } } end
    if side == "right" then return { { "M", w, 0 }, { "L", w, h } } end
    if side == "left" then return { { "M", 0, 0 }, { "L", 0, h } } end
    return { { "M", 0, h }, { "L", w, h } }
end

-- Phase 44b. Style and paint are separate. A line draws no fill.
-- Grid, dots, and hatch use the report grid spacing. The pattern
-- stroke is 0.5 CSS pixels and is not the grid weight. Hatch is one
-- diagonal, down to the right. Noise is one dot per cell, seeded from
-- the item id, the same on every run. No SVG turbulence filter.
SHAPE_FILL = {
    none = true, solid = true, grid = true, dots = true, hatched = true, noise = true,
}
SHAPE_GRID_OK = { dots = true, grid = true, dotted = true, dashed = true, none = true }

function shape_mul32(a, b)
    local a_lo = a % 65536
    local a_hi = math.floor(a / 65536) % 65536
    local b_lo = b % 65536
    local b_hi = math.floor(b / 65536) % 65536
    local p0 = a_lo * b_lo
    local p1 = a_lo * b_hi + a_hi * b_lo
    return (p0 + (p1 % 65536) * 65536) % 4294967296
end

function shape_xor_byte(hash, byte)
    local low = hash % 256
    local x, bitv, a, bv = 0, 1, low, byte
    for _ = 1, 8 do
        if (a % 2) ~= (bv % 2) then x = x + bitv end
        a = math.floor(a / 2)
        bv = math.floor(bv / 2)
        bitv = bitv * 2
    end
    return math.floor(hash / 256) * 256 + x
end

function shape_fnv(text)
    local hash = 2166136261
    local s = tostring(text or "")
    for i = 1, #s do
        hash = shape_mul32(shape_xor_byte(hash, string.byte(s, i)), 16777619)
    end
    return hash
end

function shape_lcg(seed)
    local state = seed
    if state == 0 then state = 1 end
    return function()
        state = (shape_mul32(1664525, state) + 1013904223) % 4294967296
        return state / 4294967296
    end
end

function shape_dom(prefix, id)
    local cleaned = tostring(id or ""):gsub("[^A-Za-z0-9_%-]", "")
    if cleaned == "" then cleaned = "item" end
    return prefix .. cleaned
end

function shape_weight_ok(weight)
    if weight == nil or weight == "" then return true end
    local n = tonumber(weight)
    if not n or n ~= n or n < 0 or n == math.huge then return false end
    return true
end

function shape_spacing(def)
    local fallback = shape_px("0.125 in")
    if type(def) ~= "table" or type(def.grid) ~= "table" then return fallback end
    local grid = def.grid
    local style = grid.style
    if type(style) == "string" and style ~= "" and not SHAPE_GRID_OK[style] then return fallback end
    local spacing = grid.spacing
    if type(spacing) ~= "string" or spacing == "" then spacing = "0.125 in" end
    local px = shape_px(spacing)
    if not px or px <= 0 then return fallback end
    if not shape_weight_ok(grid.weight) then return fallback end
    return px
end

function shape_stops(stops)
    if type(stops) ~= "table" then return nil end
    local out = {}
    for _, stop in ipairs(stops) do
        local at = type(stop) == "table" and tonumber(stop.at) or nil
        local color = type(stop) == "table" and shape_hex(stop.color, "") or ""
        if at and at == at and at >= 0 and at <= 1 and color ~= "" then
            out[#out + 1] = { at = at, color = color }
        end
    end
    if #out < 2 then return nil end
    return out
end

function shape_angle(paint)
    local n = tonumber(paint.angle)
    if n and n == n and n ~= math.huge and n ~= -math.huge then return n end
    return 0
end

function shape_resolve(paint, backgroundColor)
    local function is_none(v)
        if type(v) ~= "string" then return false end
        local s = string.lower(v:match("^%s*(.-)%s*$") or "")
        return s == "none" or s == "transparent"
    end
    if is_none(backgroundColor) then
        return { color = "none" }
    end
    local color = "#d3d3d3"
    local kind = "solid"
    if type(paint) == "table" then
        if type(paint.kind) == "string" then kind = paint.kind end
        if type(paint.color) == "string" and paint.color ~= "" and not is_none(paint.color) then
            color = shape_hex(paint.color, color)
        elseif type(paint.stops) == "table" and paint.stops[1]
            and type(paint.stops[1].color) == "string" and paint.stops[1].color ~= ""
            and not is_none(paint.stops[1].color) then
            color = shape_hex(paint.stops[1].color, color)
        end
    end
    if type(backgroundColor) == "string" and backgroundColor ~= "" then
        color = shape_hex(backgroundColor, color)
    end
    if kind ~= "linear" and kind ~= "radial" then return { color = color } end
    local stops = type(paint) == "table" and shape_stops(paint.stops) or nil
    if not stops then return { color = color } end
    if kind == "radial" then return { color = color, gradient = { kind = "radial", stops = stops } } end
    return { color = color, gradient = { kind = "linear", angle = shape_angle(paint), stops = stops } }
end

function shape_linear(w, h, angle)
    local rad = angle * math.pi / 180
    local dx, dy = math.cos(rad), math.sin(rad)
    local cx, cy = w / 2, h / 2
    local extent = math.abs(dx) * w / 2 + math.abs(dy) * h / 2
    return cx - dx * extent, cy - dy * extent, cx + dx * extent, cy + dy * extent
end

function shape_place(gradient, w, h)
    if not gradient then return nil end
    if gradient.kind == "radial" then
        return {
            kind = "radial", cx = w / 2, cy = h / 2,
            r = math.sqrt((w / 2) * (w / 2) + (h / 2) * (h / 2)),
            stops = gradient.stops,
        }
    end
    local x1, y1, x2, y2 = shape_linear(w, h, gradient.angle)
    return { kind = "linear", x1 = x1, y1 = y1, x2 = x2, y2 = y2, stops = gradient.stops }
end

function shape_grid(w, h, spacing)
    local parts = {}
    local nx = math.floor(w / spacing + 1e-6)
    local ny = math.floor(h / spacing + 1e-6)
    for i = 0, nx do
        local x = i * spacing
        parts[#parts + 1] = "M " .. shape_fmt(x) .. " 0 L " .. shape_fmt(x) .. " " .. shape_fmt(h)
    end
    for j = 0, ny do
        local y = j * spacing
        parts[#parts + 1] = "M 0 " .. shape_fmt(y) .. " L " .. shape_fmt(w) .. " " .. shape_fmt(y)
    end
    return table.concat(parts, " ")
end

function shape_hatch_add(points, x, y)
    for _, p in ipairs(points) do
        if math.abs(p.x - x) < 1e-6 and math.abs(p.y - y) < 1e-6 then return end
    end
    points[#points + 1] = { x = x, y = y }
end

function shape_hatch_seg(points)
    if #points ~= 2 then return nil end
    local p, q = points[1], points[2]
    if not (p.x < q.x - 1e-9 or (math.abs(p.x - q.x) <= 1e-9 and p.y <= q.y)) then
        p, q = q, p
    end
    return "M " .. shape_fmt(p.x) .. " " .. shape_fmt(p.y) .. " L " .. shape_fmt(q.x) .. " " .. shape_fmt(q.y)
end

function shape_hatch(w, h, spacing)
    local step = spacing * math.sqrt(2)
    if not (step > 0) then return "" end
    local n0 = math.ceil(-w / step - 1e-8)
    local n1 = math.floor(h / step + 1e-8)
    local parts = {}
    for n = n0, n1 do
        local b = n * step
        local points = {}
        if b >= -1e-6 and b <= h + 1e-6 then shape_hatch_add(points, 0, math.min(h, math.max(0, b))) end
        if w + b >= -1e-6 and w + b <= h + 1e-6 then shape_hatch_add(points, w, math.min(h, math.max(0, w + b))) end
        if -b >= -1e-6 and -b <= w + 1e-6 then shape_hatch_add(points, math.min(w, math.max(0, -b)), 0) end
        if h - b >= -1e-6 and h - b <= w + 1e-6 then shape_hatch_add(points, math.min(w, math.max(0, h - b)), h) end
        local seg = shape_hatch_seg(points)
        if seg then parts[#parts + 1] = seg end
    end
    return table.concat(parts, " ")
end

function shape_dots(w, h, spacing)
    local r = math.min(1, spacing / 4)
    local circles = {}
    local nx = math.floor(w / spacing + 1e-6)
    local ny = math.floor(h / spacing + 1e-6)
    for j = 0, ny do
        for i = 0, nx do
            circles[#circles + 1] = { x = i * spacing, y = j * spacing, r = r }
        end
    end
    return circles
end

function shape_noise(id, w, h, spacing)
    local next_n = shape_lcg(shape_fnv(id))
    local nx = math.max(1, math.floor(w / spacing + 0.5))
    local ny = math.max(1, math.floor(h / spacing + 0.5))
    local cw, ch = w / nx, h / ny
    local radius = math.min(math.max(spacing * 0.12, 0.75), cw * 0.3, ch * 0.3)
    local circles = {}
    for j = 0, ny - 1 do
        for i = 0, nx - 1 do
            local rx, ry = next_n(), next_n()
            circles[#circles + 1] = {
                x = (i + 0.2 + 0.6 * rx) * cw,
                y = (j + 0.2 + 0.6 * ry) * ch,
                r = radius,
            }
        end
    end
    return circles
end

function shape_step(spacing)
    if type(spacing) == "number" and spacing > 0 then return spacing end
    return shape_px("0.125 in")
end

function shape_marks(style, item, w, h, spacing)
    local step = shape_step(spacing)
    if style == "grid" then return { kind = "lines", d = shape_grid(w, h, step), sw = 0.5 } end
    if style == "hatched" then return { kind = "lines", d = shape_hatch(w, h, step), sw = 0.5 } end
    if style == "dots" then return { kind = "circles", circles = shape_dots(w, h, step) } end
    return { kind = "circles", circles = shape_noise(item.id, w, h, step) }
end

function shape_style(item, kind)
    if kind == "line" then return "none" end
    local style = item.fillStyle
    if style == nil or style == "" then return "solid" end
    if not SHAPE_FILL[style] then return "none" end
    return style
end

function shape_plan(item, kind, w, h, spacing)
    local style = shape_style(item, kind)
    local raw = type(item.fillPaint) == "table" and item.fillPaint or nil
    local paint = shape_resolve(raw, item.backgroundColor)
    if paint.color == "none" then return { style = "none", color = "none" } end
    if style == "none" then return { style = style, color = paint.color } end
    local gradient = shape_place(paint.gradient, w, h)
    if style == "solid" then return { style = style, color = paint.color, gradient = gradient } end
    return {
        style = style, color = paint.color, gradient = gradient,
        marks = shape_marks(style, item, w, h, spacing),
    }
end

function shape_attr(attrs)
    local parts = {}
    for i = 1, #attrs do
        parts[i] = attrs[i][1] .. '="' .. attrs[i][2] .. '"'
    end
    return table.concat(parts, " ")
end

function shape_gradient(draw)
    local g = draw.gradient
    local attrs
    if g.kind == "radial" then
        attrs = {
            { "id", draw.paintId }, { "gradientUnits", "userSpaceOnUse" },
            { "cx", shape_fmt(g.cx) }, { "cy", shape_fmt(g.cy) }, { "r", shape_fmt(g.r) },
        }
    else
        attrs = {
            { "id", draw.paintId }, { "gradientUnits", "userSpaceOnUse" },
            { "x1", shape_fmt(g.x1) }, { "y1", shape_fmt(g.y1) },
            { "x2", shape_fmt(g.x2) }, { "y2", shape_fmt(g.y2) },
        }
    end
    local stops = {}
    for _, stop in ipairs(g.stops) do
        stops[#stops + 1] = '<stop offset="' .. shape_fmt(stop.at) .. '" stop-color="' .. stop.color .. '"/>'
    end
    local tag = g.kind == "radial" and "radialGradient" or "linearGradient"
    return "<" .. tag .. " " .. shape_attr(attrs) .. ">" .. table.concat(stops, "") .. "</" .. tag .. ">"
end

function shape_with_transform(attrs, transform)
    if transform ~= "" then attrs[#attrs + 1] = { "transform", transform } end
    return attrs
end

function shape_mark_svg(draw)
    local chunks = {}
    if draw.marks.kind == "lines" then
        local attrs = shape_with_transform({
            { "d", draw.marks.d }, { "fill", "none" },
            { "stroke", draw.paintRef }, { "stroke-width", shape_fmt(draw.marks.sw) },
        }, draw.transform)
        chunks[1] = "<path " .. shape_attr(attrs) .. "/>"
    else
        for _, circle in ipairs(draw.marks.circles) do
            local attrs = shape_with_transform({
                { "cx", shape_fmt(circle.x) }, { "cy", shape_fmt(circle.y) },
                { "r", shape_fmt(circle.r) }, { "fill", draw.paintRef },
            }, draw.transform)
            chunks[#chunks + 1] = "<circle " .. shape_attr(attrs) .. "/>"
        end
    end
    return table.concat(chunks, "")
end

function shape_outline(draw)
    local path = { { "d", draw.d }, { "fill", draw.fill }, { "stroke", draw.stroke } }
    if draw.stroke ~= "none" then
        path[#path + 1] = { "stroke-width", shape_fmt(draw.sw) }
        path[#path + 1] = { "stroke-linejoin", "miter" }
        path[#path + 1] = { "stroke-linecap", draw.linecap }
        if draw.dash ~= "" then path[#path + 1] = { "stroke-dasharray", draw.dash } end
    end
    shape_with_transform(path, draw.transform)
    return "<path " .. shape_attr(path) .. "/>"
end

function shape_defs(draw)
    if not draw.gradient and not draw.marks then return "" end
    local chunks = { "<defs>" }
    if draw.gradient then chunks[#chunks + 1] = shape_gradient(draw) end
    if draw.marks then
        local clip = shape_with_transform({ { "d", draw.d } }, draw.transform)
        chunks[#chunks + 1] = '<clipPath id="' .. draw.clipId .. '"><path ' .. shape_attr(clip) .. "/></clipPath>"
    end
    chunks[#chunks + 1] = "</defs>"
    return table.concat(chunks, "")
end

function shape_markup(draw)
    local chunks = {
        '<svg xmlns="http://www.w3.org/2000/svg" width="100%" height="100%" overflow="visible" viewBox="0 0 '
            .. shape_fmt(draw.w) .. " " .. shape_fmt(draw.h) .. '" preserveAspectRatio="none">',
        shape_defs(draw),
    }
    if draw.marks then
        chunks[#chunks + 1] = '<g clip-path="url(#' .. draw.clipId .. ')">'
        chunks[#chunks + 1] = shape_mark_svg(draw)
        chunks[#chunks + 1] = "</g>"
    end
    chunks[#chunks + 1] = shape_outline(draw)
    chunks[#chunks + 1] = "</svg>"
    return table.concat(chunks, "")
end
function shape_svg(item, spacing)
    local box = type(item.box) == "table" and item.box or nil
    if not box then return "", "Shape box is empty." end
    local w, h = shape_px(box.width), shape_px(box.height)
    if not w or not h or w <= 0 or h <= 0 then return "", "Shape box is empty." end
    local kind = item.kind
    if type(kind) ~= "string" or kind == "" then kind = "rectangle" end
    local source = shape_source(kind)
    if not source then return "", "Shape kind " .. kind .. " is not in the list." end
    if source.bad then return "", "Shape has a bad command." end
    local commands
    if source.geometry == "line" then
        local side = item.side
        if side ~= "top" and side ~= "right" and side ~= "bottom" and side ~= "left" then side = "bottom" end
        commands = shape_line(w, h, side)
    elseif source.geometry == "rectangle" then
        commands = { { "M", 0, 0 }, { "L", w, 0 }, { "L", w, h }, { "L", 0, h }, { "Z" } }
    elseif source.geometry == "roundrect" then
        local corners = type(item.corners) == "table" and item.corners or {}
        local fallback = shape_px("0.125 in")
        local function read(name)
            local raw = corners[name]
            if type(raw) ~= "string" or raw:match("^%s*$") then return math.max(0, fallback) end
            local px = shape_px(raw)
            if not px then return math.max(0, fallback) end
            return math.max(0, px)
        end
        commands = shape_round(w, h, shape_clamp(w, h, {
            tl = read("topLeft"), tr = read("topRight"), br = read("bottomRight"), bl = read("bottomLeft"),
        }))
    elseif source.geometry == "ellipse" then
        commands = shape_ellipse(w, h)
    else
        commands = shape_scale(source.commands, w, h)
        if source.closed and commands[#commands][1] ~= "Z" then
            local copy = {}
            for i, cmd in ipairs(commands) do copy[i] = cmd end
            copy[#copy + 1] = { "Z" }
            commands = copy
        end
    end
    local sw = shape_len(item.strokeWidth, shape_px("1 pt"))
    local style = item.strokeStyle
    if style ~= "solid" and style ~= "dashed" and style ~= "dotted" and style ~= "none" then style = "solid" end
    local stroke = (style == "none" or not (sw > 0)) and "none" or shape_hex(item.strokeColor, "#000000")
    local dash, linecap = "", "butt"
    if stroke ~= "none" and style == "dashed" then dash = shape_fmt(sw * 4) .. " " .. shape_fmt(sw * 2) end
    if stroke ~= "none" and style == "dotted" then
        dash = shape_fmt(sw) .. " " .. shape_fmt(sw * 2)
        linecap = "round"
    end
    local deg = tonumber(item.rotation)
    if not deg or deg ~= deg or deg == math.huge or deg == -math.huge then deg = 0 end
    local transform = ""
    if deg ~= 0 then
        transform = "rotate(" .. shape_fmt(deg) .. " " .. shape_fmt(w / 2) .. " " .. shape_fmt(h / 2) .. ")"
    end
    local plan = shape_plan(item, kind, w, h, spacing)
    local paint_id = shape_dom("reports-fill-", item.id)
    local paint_ref = plan.color
    if plan.gradient then paint_ref = "url(#" .. paint_id .. ")" end
    local fill = "none"
    if plan.style == "solid" then fill = paint_ref end
    return shape_markup({
        w = w, h = h, d = shape_d(commands),
        fill = fill, paintRef = paint_ref, paintId = paint_id,
        clipId = shape_dom("reports-clip-", item.id),
        gradient = plan.gradient, marks = plan.marks,
        stroke = stroke, sw = sw, dash = dash, linecap = linecap,
        transform = transform,
    }), ""
end

function shape_collection(value)
    if type(value) == "string" then
        local parsed, err = json_decode(value)
        if err or type(parsed) ~= "table" then return nil end
        value = parsed
    end
    if type(value) ~= "table" then return nil end
    if value.commands == nil and value.closed == nil and type(value[1]) == "table" and value[2] == nil then
        value = value[1]
    end
    if type(value) ~= "table" then return nil end
    return value
end

function load_shape_rows()
    if shape_loaded then return end
    shape_loaded = true
    if type(H) ~= "table" or type(H.altquery_sync) ~= "function" then return end
    local hy = type(P._hydrogen) == "table" and P._hydrogen or {}
    local database = hy.database
    if type(database) ~= "string" or database == "" then return end
    local ok, res, err = pcall(H.altquery_sync, database, [[
        SELECT code, value_txt, collection FROM ${SCHEMA}lookups
        WHERE lookup_id = 78 AND key_idx > 0
        ORDER BY key_idx
    ]], {})
    if not ok then
        err = res
        res = nil
    end
    if err or type(res) ~= "table" or type(res.rows) ~= "table" then
        add_log("warn", "shape kinds could not be read; using the built-in list")
        return
    end
    shape_catalog_on = true
    for _, row in ipairs(res.rows) do
        local code = row_field(row, "code")
        if type(code) == "string" and code ~= "" and shape_rows[code] == nil then
            if SHAPE_GEOMETRY[code] then
                shape_rows[code] = { geometry = true }
            else
                local obj = shape_collection(row_field(row, "collection"))
                local commands = type(obj) == "table" and obj.commands or nil
                if not shape_commands_ok(commands) then
                    shape_rows[code] = { bad = true }
                else
                    shape_rows[code] = { commands = commands, closed = not (obj.closed == false) }
                end
            end
        end
    end
end



-- Shape kinds load once, when the report first draws a shape.
load_shape_rows()

]==]

local sql_new = [==[

-- Reports.Render.Shapes (Reporting). Lazy: static and field shapes, fill, gradient, lookup 078 kinds.
-- Loaded by Reports.Render into its per-request environment.
-- Names defined here are shared with Reports.Render and its other modules.

-- Phase 44a: static shapes. Geometry is code. Path kinds use lookup 078
-- when that lookup is loaded, otherwise these commands. PDF does not draw them.
shape_rows = {}
shape_loaded = false
shape_catalog_on = false
SHAPE_KAPPA = 0.5522847498
SHAPE_GEOMETRY = { line = true, rectangle = true, roundrect = true, ellipse = true }
SHAPE_EMBEDDED = {
    x = { closed = false, commands = { { "M", 0, 0 }, { "L", 1, 1 }, { "M", 1, 0 }, { "L", 0, 1 } } },
    trapezoid = { closed = true, commands = { { "M", 0.25, 0 }, { "L", 0.75, 0 }, { "L", 1, 1 }, { "L", 0, 1 }, { "Z" } } },
    star = { closed = true, commands = {
        { "M", 0.5, 0 }, { "L", 0.618034, 0.381966 }, { "L", 1, 0.381966 }, { "L", 0.690983, 0.618034 },
        { "L", 0.809017, 1 }, { "L", 0.5, 0.763932 }, { "L", 0.190983, 1 }, { "L", 0.309017, 0.618034 },
        { "L", 0, 0.381966 }, { "L", 0.381966, 0.381966 }, { "Z" },
    } },
    pentagon = { closed = true, commands = {
        { "M", 0.5, 0 }, { "L", 1, 0.381966 }, { "L", 0.809017, 1 }, { "L", 0.190983, 1 }, { "L", 0, 0.381966 }, { "Z" },
    } },
    hexagon = { closed = true, commands = {
        { "M", 0.5, 0 }, { "L", 1, 0.25 }, { "L", 1, 0.75 }, { "L", 0.5, 1 }, { "L", 0, 0.75 }, { "L", 0, 0.25 }, { "Z" },
    } },
}

function shape_len(value, fallback)
    if type(value) ~= "string" or value:match("^%s*$") then return fallback end
    local px = shape_px(value)
    if not px then return fallback end
    return px
end

function shape_hex(value, fallback)
    if type(value) == "string" and value:match("^#[0-9A-Fa-f][0-9A-Fa-f][0-9A-Fa-f][0-9A-Fa-f][0-9A-Fa-f][0-9A-Fa-f]$") then
        return value:lower()
    end
    return fallback
end

function shape_commands_ok(commands)
    if type(commands) ~= "table" or #commands == 0 then return false end
    local arity = { M = 2, L = 2, C = 6, Z = 0 }
    for _, cmd in ipairs(commands) do
        if type(cmd) ~= "table" then return false end
        local need = arity[cmd[1]]
        if need == nil or #cmd ~= need + 1 then return false end
        for i = 2, #cmd do
            local num = cmd[i]
            if type(num) ~= "number" or num ~= num or num < 0 or num > 1 then return false end
        end
    end
    return true
end

function shape_source(kind)
    if SHAPE_GEOMETRY[kind] then return { geometry = kind } end
    local row = shape_rows[kind]
    if row then
        if row.bad then return { bad = true } end
        return { commands = row.commands, closed = row.closed }
    end
    local embedded = SHAPE_EMBEDDED[kind]
    if embedded then return { commands = embedded.commands, closed = embedded.closed } end
    return nil
end

function shape_scale(commands, w, h)
    local out = {}
    for _, cmd in ipairs(commands) do
        if cmd[1] == "Z" then
            out[#out + 1] = { "Z" }
        else
            local row = { cmd[1] }
            for i = 2, #cmd do
                local axis = ((i - 1) % 2 == 1) and w or h
                row[#row + 1] = axis * cmd[i]
            end
            out[#out + 1] = row
        end
    end
    return out
end

function shape_d(commands)
    local parts = {}
    for _, cmd in ipairs(commands) do
        if cmd[1] == "Z" then
            parts[#parts + 1] = "Z"
        else
            local nums = {}
            for i = 2, #cmd do nums[#nums + 1] = shape_fmt(cmd[i]) end
            parts[#parts + 1] = cmd[1] .. " " .. table.concat(nums, " ")
        end
    end
    return table.concat(parts, " ")
end

function shape_clamp(w, h, radii)
    local factor = 1
    local pairs = {
        { radii.tl + radii.tr, w },
        { radii.bl + radii.br, w },
        { radii.tl + radii.bl, h },
        { radii.tr + radii.br, h },
    }
    for _, pair in ipairs(pairs) do
        if pair[1] > 0 and pair[2] / pair[1] < factor then factor = pair[2] / pair[1] end
    end
    if factor ~= factor or factor < 0 then factor = 0 end
    if factor > 1 then factor = 1 end
    return {
        tl = radii.tl * factor, tr = radii.tr * factor,
        br = radii.br * factor, bl = radii.bl * factor,
    }
end

function shape_round(w, h, r)
    local tl, tr, br, bl = r.tl, r.tr, r.br, r.bl
    return {
        { "M", tl, 0 },
        { "L", w - tr, 0 },
        { "C", w - tr * (1 - SHAPE_KAPPA), 0, w, tr * (1 - SHAPE_KAPPA), w, tr },
        { "L", w, h - br },
        { "C", w, h - br * (1 - SHAPE_KAPPA), w - br * (1 - SHAPE_KAPPA), h, w - br, h },
        { "L", bl, h },
        { "C", bl * (1 - SHAPE_KAPPA), h, 0, h - bl * (1 - SHAPE_KAPPA), 0, h - bl },
        { "L", 0, tl },
        { "C", 0, tl * (1 - SHAPE_KAPPA), tl * (1 - SHAPE_KAPPA), 0, tl, 0 },
        { "Z" },
    }
end

function shape_ellipse(w, h)
    local cx, cy, rx, ry = w / 2, h / 2, w / 2, h / 2
    return {
        { "M", cx, cy - ry },
        { "C", cx + SHAPE_KAPPA * rx, cy - ry, cx + rx, cy - SHAPE_KAPPA * ry, cx + rx, cy },
        { "C", cx + rx, cy + SHAPE_KAPPA * ry, cx + SHAPE_KAPPA * rx, cy + ry, cx, cy + ry },
        { "C", cx - SHAPE_KAPPA * rx, cy + ry, cx - rx, cy + SHAPE_KAPPA * ry, cx - rx, cy },
        { "C", cx - rx, cy - SHAPE_KAPPA * ry, cx - SHAPE_KAPPA * rx, cy - ry, cx, cy - ry },
        { "Z" },
    }
end

function shape_resolve_side(value)
    local side = type(value) == "string" and string.lower((value:match("^%s*(.-)%s*$")) or "") or ""
    if side == "top" or side == "right" or side == "bottom" or side == "left" then return side end
    return "bottom"
end

-- Path inset by half the stroke so a centered stroke stays inside the box.
function shape_line(w, h, side, stroke_width)
    local sw = (type(stroke_width) == "number" and stroke_width > 0) and stroke_width or 0
    local inset = sw / 2
    if inset > w / 2 then inset = w / 2 end
    if inset > h / 2 then inset = h / 2 end
    if inset < 0 then inset = 0 end
    side = shape_resolve_side(side)
    if side == "top" then return { { "M", 0, inset }, { "L", w, inset } } end
    if side == "right" then return { { "M", w - inset, 0 }, { "L", w - inset, h } } end
    if side == "left" then return { { "M", inset, 0 }, { "L", inset, h } } end
    return { { "M", 0, h - inset }, { "L", w, h - inset } }
end

-- Phase 44b. Style and paint are separate. A line draws no fill.
-- Grid, dots, and hatch use the report grid spacing. The pattern
-- stroke is 0.5 CSS pixels and is not the grid weight. Hatch is one
-- diagonal, down to the right. Noise is one dot per cell, seeded from
-- the item id, the same on every run. No SVG turbulence filter.
SHAPE_FILL = {
    none = true, solid = true, grid = true, dots = true, hatched = true, noise = true,
}
SHAPE_GRID_OK = { dots = true, grid = true, dotted = true, dashed = true, none = true }

function shape_mul32(a, b)
    local a_lo = a % 65536
    local a_hi = math.floor(a / 65536) % 65536
    local b_lo = b % 65536
    local b_hi = math.floor(b / 65536) % 65536
    local p0 = a_lo * b_lo
    local p1 = a_lo * b_hi + a_hi * b_lo
    return (p0 + (p1 % 65536) * 65536) % 4294967296
end

function shape_xor_byte(hash, byte)
    local low = hash % 256
    local x, bitv, a, bv = 0, 1, low, byte
    for _ = 1, 8 do
        if (a % 2) ~= (bv % 2) then x = x + bitv end
        a = math.floor(a / 2)
        bv = math.floor(bv / 2)
        bitv = bitv * 2
    end
    return math.floor(hash / 256) * 256 + x
end

function shape_fnv(text)
    local hash = 2166136261
    local s = tostring(text or "")
    for i = 1, #s do
        hash = shape_mul32(shape_xor_byte(hash, string.byte(s, i)), 16777619)
    end
    return hash
end

function shape_lcg(seed)
    local state = seed
    if state == 0 then state = 1 end
    return function()
        state = (shape_mul32(1664525, state) + 1013904223) % 4294967296
        return state / 4294967296
    end
end

function shape_dom(prefix, id)
    local cleaned = tostring(id or ""):gsub("[^A-Za-z0-9_%-]", "")
    if cleaned == "" then cleaned = "item" end
    return prefix .. cleaned
end

function shape_weight_ok(weight)
    if weight == nil or weight == "" then return true end
    local n = tonumber(weight)
    if not n or n ~= n or n < 0 or n == math.huge then return false end
    return true
end

function shape_spacing(def)
    local fallback = shape_px("0.125 in")
    if type(def) ~= "table" or type(def.grid) ~= "table" then return fallback end
    local grid = def.grid
    local style = grid.style
    if type(style) == "string" and style ~= "" and not SHAPE_GRID_OK[style] then return fallback end
    local spacing = grid.spacing
    if type(spacing) ~= "string" or spacing == "" then spacing = "0.125 in" end
    local px = shape_px(spacing)
    if not px or px <= 0 then return fallback end
    if not shape_weight_ok(grid.weight) then return fallback end
    return px
end

function shape_stops(stops)
    if type(stops) ~= "table" then return nil end
    local out = {}
    for _, stop in ipairs(stops) do
        local at = type(stop) == "table" and tonumber(stop.at) or nil
        local color = type(stop) == "table" and shape_hex(stop.color, "") or ""
        if at and at == at and at >= 0 and at <= 1 and color ~= "" then
            out[#out + 1] = { at = at, color = color }
        end
    end
    if #out < 2 then return nil end
    return out
end

function shape_angle(paint)
    local n = tonumber(paint.angle)
    if n and n == n and n ~= math.huge and n ~= -math.huge then return n end
    return 0
end

function shape_resolve(paint, backgroundColor)
    local function is_none(v)
        if type(v) ~= "string" then return false end
        local s = string.lower(v:match("^%s*(.-)%s*$") or "")
        return s == "none" or s == "transparent"
    end
    if is_none(backgroundColor) then
        return { color = "none" }
    end
    local color = "#d3d3d3"
    local kind = "solid"
    if type(paint) == "table" then
        if type(paint.kind) == "string" then kind = paint.kind end
        if type(paint.color) == "string" and paint.color ~= "" and not is_none(paint.color) then
            color = shape_hex(paint.color, color)
        elseif type(paint.stops) == "table" and paint.stops[1]
            and type(paint.stops[1].color) == "string" and paint.stops[1].color ~= ""
            and not is_none(paint.stops[1].color) then
            color = shape_hex(paint.stops[1].color, color)
        end
    end
    if type(backgroundColor) == "string" and backgroundColor ~= "" then
        color = shape_hex(backgroundColor, color)
    end
    if kind ~= "linear" and kind ~= "radial" then return { color = color } end
    local stops = type(paint) == "table" and shape_stops(paint.stops) or nil
    if not stops then return { color = color } end
    if kind == "radial" then return { color = color, gradient = { kind = "radial", stops = stops } } end
    return { color = color, gradient = { kind = "linear", angle = shape_angle(paint), stops = stops } }
end

function shape_linear(w, h, angle)
    local rad = angle * math.pi / 180
    local dx, dy = math.cos(rad), math.sin(rad)
    local cx, cy = w / 2, h / 2
    local extent = math.abs(dx) * w / 2 + math.abs(dy) * h / 2
    return cx - dx * extent, cy - dy * extent, cx + dx * extent, cy + dy * extent
end

function shape_place(gradient, w, h)
    if not gradient then return nil end
    if gradient.kind == "radial" then
        return {
            kind = "radial", cx = w / 2, cy = h / 2,
            r = math.sqrt((w / 2) * (w / 2) + (h / 2) * (h / 2)),
            stops = gradient.stops,
        }
    end
    local x1, y1, x2, y2 = shape_linear(w, h, gradient.angle)
    return { kind = "linear", x1 = x1, y1 = y1, x2 = x2, y2 = y2, stops = gradient.stops }
end

function shape_grid(w, h, spacing)
    local parts = {}
    local nx = math.floor(w / spacing + 1e-6)
    local ny = math.floor(h / spacing + 1e-6)
    for i = 0, nx do
        local x = i * spacing
        parts[#parts + 1] = "M " .. shape_fmt(x) .. " 0 L " .. shape_fmt(x) .. " " .. shape_fmt(h)
    end
    for j = 0, ny do
        local y = j * spacing
        parts[#parts + 1] = "M 0 " .. shape_fmt(y) .. " L " .. shape_fmt(w) .. " " .. shape_fmt(y)
    end
    return table.concat(parts, " ")
end

function shape_hatch_add(points, x, y)
    for _, p in ipairs(points) do
        if math.abs(p.x - x) < 1e-6 and math.abs(p.y - y) < 1e-6 then return end
    end
    points[#points + 1] = { x = x, y = y }
end

function shape_hatch_seg(points)
    if #points ~= 2 then return nil end
    local p, q = points[1], points[2]
    if not (p.x < q.x - 1e-9 or (math.abs(p.x - q.x) <= 1e-9 and p.y <= q.y)) then
        p, q = q, p
    end
    return "M " .. shape_fmt(p.x) .. " " .. shape_fmt(p.y) .. " L " .. shape_fmt(q.x) .. " " .. shape_fmt(q.y)
end

function shape_hatch(w, h, spacing)
    local step = spacing * math.sqrt(2)
    if not (step > 0) then return "" end
    local n0 = math.ceil(-w / step - 1e-8)
    local n1 = math.floor(h / step + 1e-8)
    local parts = {}
    for n = n0, n1 do
        local b = n * step
        local points = {}
        if b >= -1e-6 and b <= h + 1e-6 then shape_hatch_add(points, 0, math.min(h, math.max(0, b))) end
        if w + b >= -1e-6 and w + b <= h + 1e-6 then shape_hatch_add(points, w, math.min(h, math.max(0, w + b))) end
        if -b >= -1e-6 and -b <= w + 1e-6 then shape_hatch_add(points, math.min(w, math.max(0, -b)), 0) end
        if h - b >= -1e-6 and h - b <= w + 1e-6 then shape_hatch_add(points, math.min(w, math.max(0, h - b)), h) end
        local seg = shape_hatch_seg(points)
        if seg then parts[#parts + 1] = seg end
    end
    return table.concat(parts, " ")
end

function shape_dots(w, h, spacing)
    local r = math.min(1, spacing / 4)
    local circles = {}
    local nx = math.floor(w / spacing + 1e-6)
    local ny = math.floor(h / spacing + 1e-6)
    for j = 0, ny do
        for i = 0, nx do
            circles[#circles + 1] = { x = i * spacing, y = j * spacing, r = r }
        end
    end
    return circles
end

function shape_noise(id, w, h, spacing)
    local next_n = shape_lcg(shape_fnv(id))
    local nx = math.max(1, math.floor(w / spacing + 0.5))
    local ny = math.max(1, math.floor(h / spacing + 0.5))
    local cw, ch = w / nx, h / ny
    local radius = math.min(math.max(spacing * 0.12, 0.75), cw * 0.3, ch * 0.3)
    local circles = {}
    for j = 0, ny - 1 do
        for i = 0, nx - 1 do
            local rx, ry = next_n(), next_n()
            circles[#circles + 1] = {
                x = (i + 0.2 + 0.6 * rx) * cw,
                y = (j + 0.2 + 0.6 * ry) * ch,
                r = radius,
            }
        end
    end
    return circles
end

function shape_step(spacing)
    if type(spacing) == "number" and spacing > 0 then return spacing end
    return shape_px("0.125 in")
end

function shape_marks(style, item, w, h, spacing)
    local step = shape_step(spacing)
    if style == "grid" then return { kind = "lines", d = shape_grid(w, h, step), sw = 0.5 } end
    if style == "hatched" then return { kind = "lines", d = shape_hatch(w, h, step), sw = 0.5 } end
    if style == "dots" then return { kind = "circles", circles = shape_dots(w, h, step) } end
    return { kind = "circles", circles = shape_noise(item.id, w, h, step) }
end

function shape_style(item, kind)
    if kind == "line" then return "none" end
    local style = item.fillStyle
    if style == nil or style == "" then return "solid" end
    if not SHAPE_FILL[style] then return "none" end
    return style
end

function shape_plan(item, kind, w, h, spacing)
    local style = shape_style(item, kind)
    local raw = type(item.fillPaint) == "table" and item.fillPaint or nil
    local paint = shape_resolve(raw, item.backgroundColor)
    if paint.color == "none" then return { style = "none", color = "none" } end
    if style == "none" then return { style = style, color = paint.color } end
    local gradient = shape_place(paint.gradient, w, h)
    if style == "solid" then return { style = style, color = paint.color, gradient = gradient } end
    return {
        style = style, color = paint.color, gradient = gradient,
        marks = shape_marks(style, item, w, h, spacing),
    }
end

function shape_attr(attrs)
    local parts = {}
    for i = 1, #attrs do
        parts[i] = attrs[i][1] .. '="' .. attrs[i][2] .. '"'
    end
    return table.concat(parts, " ")
end

function shape_gradient(draw)
    local g = draw.gradient
    local attrs
    if g.kind == "radial" then
        attrs = {
            { "id", draw.paintId }, { "gradientUnits", "userSpaceOnUse" },
            { "cx", shape_fmt(g.cx) }, { "cy", shape_fmt(g.cy) }, { "r", shape_fmt(g.r) },
        }
    else
        attrs = {
            { "id", draw.paintId }, { "gradientUnits", "userSpaceOnUse" },
            { "x1", shape_fmt(g.x1) }, { "y1", shape_fmt(g.y1) },
            { "x2", shape_fmt(g.x2) }, { "y2", shape_fmt(g.y2) },
        }
    end
    local stops = {}
    for _, stop in ipairs(g.stops) do
        stops[#stops + 1] = '<stop offset="' .. shape_fmt(stop.at) .. '" stop-color="' .. stop.color .. '"/>'
    end
    local tag = g.kind == "radial" and "radialGradient" or "linearGradient"
    return "<" .. tag .. " " .. shape_attr(attrs) .. ">" .. table.concat(stops, "") .. "</" .. tag .. ">"
end

function shape_with_transform(attrs, transform)
    if transform ~= "" then attrs[#attrs + 1] = { "transform", transform } end
    return attrs
end

function shape_mark_svg(draw)
    local chunks = {}
    if draw.marks.kind == "lines" then
        local attrs = shape_with_transform({
            { "d", draw.marks.d }, { "fill", "none" },
            { "stroke", draw.paintRef }, { "stroke-width", shape_fmt(draw.marks.sw) },
        }, draw.transform)
        chunks[1] = "<path " .. shape_attr(attrs) .. "/>"
    else
        for _, circle in ipairs(draw.marks.circles) do
            local attrs = shape_with_transform({
                { "cx", shape_fmt(circle.x) }, { "cy", shape_fmt(circle.y) },
                { "r", shape_fmt(circle.r) }, { "fill", draw.paintRef },
            }, draw.transform)
            chunks[#chunks + 1] = "<circle " .. shape_attr(attrs) .. "/>"
        end
    end
    return table.concat(chunks, "")
end

function shape_outline(draw)
    local path = { { "d", draw.d }, { "fill", draw.fill }, { "stroke", draw.stroke } }
    if draw.stroke ~= "none" then
        path[#path + 1] = { "stroke-width", shape_fmt(draw.sw) }
        path[#path + 1] = { "stroke-linejoin", "miter" }
        path[#path + 1] = { "stroke-linecap", draw.linecap }
        if draw.dash ~= "" then path[#path + 1] = { "stroke-dasharray", draw.dash } end
    end
    shape_with_transform(path, draw.transform)
    return "<path " .. shape_attr(path) .. "/>"
end

function shape_defs(draw)
    if not draw.gradient and not draw.marks then return "" end
    local chunks = { "<defs>" }
    if draw.gradient then chunks[#chunks + 1] = shape_gradient(draw) end
    if draw.marks then
        local clip = shape_with_transform({ { "d", draw.d } }, draw.transform)
        chunks[#chunks + 1] = '<clipPath id="' .. draw.clipId .. '"><path ' .. shape_attr(clip) .. "/></clipPath>"
    end
    chunks[#chunks + 1] = "</defs>"
    return table.concat(chunks, "")
end

function shape_markup(draw)
    local chunks = {
        '<svg xmlns="http://www.w3.org/2000/svg" width="100%" height="100%" style="display:block" overflow="visible" viewBox="0 0 '
            .. shape_fmt(draw.w) .. " " .. shape_fmt(draw.h) .. '" preserveAspectRatio="none">',
        shape_defs(draw),
    }
    if draw.marks then
        chunks[#chunks + 1] = '<g clip-path="url(#' .. draw.clipId .. ')">'
        chunks[#chunks + 1] = shape_mark_svg(draw)
        chunks[#chunks + 1] = "</g>"
    end
    chunks[#chunks + 1] = shape_outline(draw)
    chunks[#chunks + 1] = "</svg>"
    return table.concat(chunks, "")
end
function shape_svg(item, spacing)
    local box = type(item.box) == "table" and item.box or nil
    if not box then return "", "Shape box is empty." end
    local w, h = shape_px(box.width), shape_px(box.height)
    if not w or not h or w <= 0 or h <= 0 then return "", "Shape box is empty." end
    local kind = item.kind
    if type(kind) ~= "string" or kind == "" then kind = "rectangle" end
    local source = shape_source(kind)
    if not source then return "", "Shape kind " .. kind .. " is not in the list." end
    if source.bad then return "", "Shape has a bad command." end
    local sw = shape_len(item.strokeWidth, shape_px("1 pt"))
    local commands
    if source.geometry == "line" then
        commands = shape_line(w, h, item.side, sw)
    elseif source.geometry == "rectangle" then
        commands = { { "M", 0, 0 }, { "L", w, 0 }, { "L", w, h }, { "L", 0, h }, { "Z" } }
    elseif source.geometry == "roundrect" then
        local corners = type(item.corners) == "table" and item.corners or {}
        local fallback = shape_px("0.125 in")
        local function read(name)
            local raw = corners[name]
            if type(raw) ~= "string" or raw:match("^%s*$") then return math.max(0, fallback) end
            local px = shape_px(raw)
            if not px then return math.max(0, fallback) end
            return math.max(0, px)
        end
        commands = shape_round(w, h, shape_clamp(w, h, {
            tl = read("topLeft"), tr = read("topRight"), br = read("bottomRight"), bl = read("bottomLeft"),
        }))
    elseif source.geometry == "ellipse" then
        commands = shape_ellipse(w, h)
    else
        commands = shape_scale(source.commands, w, h)
        if source.closed and commands[#commands][1] ~= "Z" then
            local copy = {}
            for i, cmd in ipairs(commands) do copy[i] = cmd end
            copy[#copy + 1] = { "Z" }
            commands = copy
        end
    end
    local style = item.strokeStyle
    if style ~= "solid" and style ~= "dashed" and style ~= "dotted" and style ~= "none" then style = "solid" end
    local stroke = (style == "none" or not (sw > 0)) and "none" or shape_hex(item.strokeColor, "#000000")
    local dash, linecap = "", "butt"
    if stroke ~= "none" and style == "dashed" then dash = shape_fmt(sw * 4) .. " " .. shape_fmt(sw * 2) end
    if stroke ~= "none" and style == "dotted" then
        dash = shape_fmt(sw) .. " " .. shape_fmt(sw * 2)
        linecap = "round"
    end
    local deg = tonumber(item.rotation)
    if not deg or deg ~= deg or deg == math.huge or deg == -math.huge then deg = 0 end
    local transform = ""
    if deg ~= 0 then
        transform = "rotate(" .. shape_fmt(deg) .. " " .. shape_fmt(w / 2) .. " " .. shape_fmt(h / 2) .. ")"
    end
    local plan = shape_plan(item, kind, w, h, spacing)
    local paint_id = shape_dom("reports-fill-", item.id)
    local paint_ref = plan.color
    if plan.gradient then paint_ref = "url(#" .. paint_id .. ")" end
    local fill = "none"
    if plan.style == "solid" then fill = paint_ref end
    return shape_markup({
        w = w, h = h, d = shape_d(commands),
        fill = fill, paintRef = paint_ref, paintId = paint_id,
        clipId = shape_dom("reports-clip-", item.id),
        gradient = plan.gradient, marks = plan.marks,
        stroke = stroke, sw = sw, dash = dash, linecap = linecap,
        transform = transform,
    }), ""
end

function shape_collection(value)
    if type(value) == "string" then
        local parsed, err = json_decode(value)
        if err or type(parsed) ~= "table" then return nil end
        value = parsed
    end
    if type(value) ~= "table" then return nil end
    if value.commands == nil and value.closed == nil and type(value[1]) == "table" and value[2] == nil then
        value = value[1]
    end
    if type(value) ~= "table" then return nil end
    return value
end

function load_shape_rows()
    if shape_loaded then return end
    shape_loaded = true
    if type(H) ~= "table" or type(H.altquery_sync) ~= "function" then return end
    local hy = type(P._hydrogen) == "table" and P._hydrogen or {}
    local database = hy.database
    if type(database) ~= "string" or database == "" then return end
    local ok, res, err = pcall(H.altquery_sync, database, [[
        SELECT code, value_txt, collection FROM ${SCHEMA}lookups
        WHERE lookup_id = 78 AND key_idx > 0
        ORDER BY key_idx
    ]], {})
    if not ok then
        err = res
        res = nil
    end
    if err or type(res) ~= "table" or type(res.rows) ~= "table" then
        add_log("warn", "shape kinds could not be read; using the built-in list")
        return
    end
    shape_catalog_on = true
    for _, row in ipairs(res.rows) do
        local code = row_field(row, "code")
        if type(code) == "string" and code ~= "" and shape_rows[code] == nil then
            if SHAPE_GEOMETRY[code] then
                shape_rows[code] = { geometry = true }
            else
                local obj = shape_collection(row_field(row, "collection"))
                local commands = type(obj) == "table" and obj.commands or nil
                if not shape_commands_ok(commands) then
                    shape_rows[code] = { bad = true }
                else
                    shape_rows[code] = { commands = commands, closed = not (obj.closed == false) }
                end
            end
        end
    end
end



-- Shape kinds load once, when the report first draws a shape.
load_shape_rows()

]==]

-- ----------------------------------------------------------------------------
-- Forward: amend Reports.Render.Shapes
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
        'Amend Reports.Render.Shapes line Side'                               AS name,
        [=[
            # Forward Migration ${MIGRATION}: Fix line Side placement (normalize + stroke inset)

            shape_resolve_side lowercases Side; shape_line insets by half stroke so Top/Bottom sit inside the box. SVG display:block.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]====]})

-- ----------------------------------------------------------------------------
-- Reverse: restore prior Reports.Render.Shapes body
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
        'Restore prior Reports.Render.Shapes'                               AS name,
        [=[
            # Reverse Migration ${MIGRATION}: restore Reports.Render.Shapes

            Puts the Phase-77 Shapes body back.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]====]})

return queries end
