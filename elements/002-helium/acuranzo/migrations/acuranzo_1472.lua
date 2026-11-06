-- Migration: acuranzo_1472.lua
-- Reports.Render barcode fit (Reporting Phase 54)
--
-- The box is a free rectangle. The symbol keeps its aspect.
-- contain uses meet. cover uses slice. none, and scale-down
-- when the symbol fits, draw at the symbol's own pixel size.
-- sql_old is the acuranzo_1471.lua body. PDF is not emitted.
-- QueryRef 175 stays unused. No new lookup.
--
-- luacheck: no max line length
-- luacheck: no unused args
--
-- CHANGELOG
-- 1.0.0 - 2026-11-05 - Barcode fit inside a free rectangle

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "scripts"
cfg.MIGRATION = "1472"
cfg.GROUP_NAME = "Reports"
cfg.SCRIPT_NAME = "Render"

local sql_old = [==[
-- Reports.Render (Reporting Phase 54: barcode SVG)
-- typeKey 43 and 45 draw one SVG from require("zint.barcode").
-- The box is the frame. preserveAspectRatio is meet. Bar height is zint height.
-- Rotation is CSS on the symbol. PDF is not emitted.
-- Reports.Render (Reporting Phase 44c: static number, datetime, image, and memo)
-- typeKey 1 prints a masked number. A non-number warns once and prints blank.
-- typeKey 4 prints a Luxon datetime. A civil string is not shifted. A unix number is UTC.
-- typeKey 3 is plain text. typeKey 2 places one image from doc.images.
-- fitHeight grows or shrinks a row in line-height steps. The stored band height stays.
-- A grown item is drawn in Helvetica. embedFonts names a family from params.fonts.
-- HTML stores each display PNG once. PDF is not emitted.
-- Reports.Render (Reporting Phase 44b: shape fill and gradient)
-- fillStyle is none, solid, grid, dots, hatched, or noise.
-- fillPaint is solid, linear, or radial. A grid can use a gradient.
-- Hatch runs down to the right. Noise is seeded from the item id.
-- Grid and dots use the report grid spacing. PDF does not draw the shape.
-- Reports.Render (Reporting Phase 44a: static shapes)
-- A static shape (typeKey 25) passes its path through compose.
-- stage html draws an SVG. PDF does not draw the shape.
-- Reports.Render (Reporting Phase 44: first and last)
-- first and last walk row order and skip nulls. first is not min.
-- A null in the middle is skipped. An empty result prints blank.
-- A datetime item is typeKey 38. A text item is typeKey 34.
-- A number item is typeKey 35.
-- Reports.Render (Reporting Phase 43: min and max)
-- min and max compare numbers as numbers and strings as written.
-- The first finite number or string picks the kind. A later value
-- of the other kind warns once and is skipped. Nulls do not warn.
-- An empty result prints blank. A datetime item is typeKey 38.
-- A text item is typeKey 34. A number item is typeKey 35.
-- Reports.Render (Reporting Phase 42: mode)
-- mode returns the most common value. A tie returns the smallest
-- value under Lua < and warns once. Text and numbers compare apart.
-- A string result prints with no mask. A number uses the number mask.
-- A text mode item is typeKey 34. A number mode item is typeKey 35.
-- Reports.Render (Reporting Phase 41: median)
-- median sorts the numeric values. An odd count returns the middle.
-- An even count returns the mean of the two central numbers.
-- An empty median prints blank. A non-numeric value warns and is skipped.
-- Reports.Render (Reporting Phase 40: avg, mean, and weighted)
-- avg and mean are the arithmetic mean of the numeric values.
-- An empty mean prints blank. weighted stores weight as a field name
-- and divides the sum of value times weight by the sum of the weights.
-- A non-numeric value warns and is skipped. A missing weight, a zero
-- sum of weights, or no numeric pair prints blank and warns once.
-- Reports.Render (Reporting Phase 39: page totals)
-- An aggregate number on a page header or page footer reduces the detail
-- rows placed on that page. The page header and the page footer share
-- that total. The same rows serve every op. Group and report totals
-- are unchanged. Page totals are filled after the page list exists.
-- Reports.Render (Reporting Phase 38: count, filled, unique, and sum)
-- An aggregate number (typeKey 35) on a group or report header or footer
-- reduces one field. count is the row count. filled, unique, and sum skip
-- nulls. An empty count, filled, or unique is 0. An empty sum prints blank.
-- A value that is not a number is skipped by sum, with one warn per item.
-- The raw result is ctx.aggregates[op][field]. The item mask formats it.
-- The header and the footer of one scope share that result.
-- A function name warns and prints blank until the formula compiler.
-- params: { stage = "compose"|"html", definition = {...}, sources = { name = [rows] } }
-- A stored report may send report_id and rev_id instead of definition.
-- The script reads QueryRef 167 through H.altquery_sync on
-- params._hydrogen.database and parses the report text.
-- An inline definition is used when it is a table.
-- A query datasource with no array in sources loads queries.code where
-- query_ref matches and query_type_a28 = 3 (SQL - Reporting), then runs
-- that SQL with H.altquery_sync on the session database.
-- An array in sources wins and the script does not query.
-- Binds are datasource.params. A scalar is the value. {parameter} uses
-- params.parameters, then the definition parameter default. {literal}
-- uses that value. params.parameters overrides a same-named bind.
-- Types come from collection.reporting.parameters.
-- An omitted stage runs compose. compose returns { pages, log }.
-- stage "html" returns { html, log }: one <section> per page at the
-- definition page size, box-sizing border-box, padding the four margins.
-- Item boxes use the stored unit with the space removed (0 in -> 0in).
-- Bands: RH(12) PH(13) GH(14) Detail(15) GF(16) PF(17) RF(18), any count.
-- Groups nest in GH order (outermost first). Field values go through the item mask (Value format); null prints blank.
-- Paging: band heights against the page content box (page height less top/bottom margins).
-- A group header moves with its first detail row; headers do not repeat.
-- System datetime (5) prints one generation instant (params.now or H.system.now()) in the item timezone
-- (User | UTC | Explicit; report default zone definition.defaultZone). System user (6) prints _hydrogen.username.
-- IANA zones resolve through luatz on host tzdata (/usr/share/zoneinfo); unknown zone -> UTC + warn.
-- Datetime masks use the Luxon token subset (yyyy MMM MM dd EEE HH mm ss SSS W ...); defaults yyyy-MM-dd / yyyy-MM-dd HH:mm.
-- Page number (61) / count (62) / label (64, template {n} {c}) fill after paging; same on every band of a page.

local P = params or {}
local log = {}

local function add_log(level, message)
    log[#log + 1] = { level = level, message = message }
end

local function fail(message)
    add_log("error", message)
    H.set_result_json({ error = message, log = log })
    return 0
end

local FIELD_KEYS = { [20] = true, [21] = true, [24] = true }
local PAGE_KEYS = { [61] = true, [62] = true, [64] = true }
local PAGE_TEMPLATE_DEFAULT = "Page {n} of {c}"
local page_src = {}
local page_agg_src = {}
local page_detail = {}
local LATER_KEYS = {
    [42] = true, [44] = true,
    [61] = true, [62] = true, [63] = true, [28] = true, [29] = true, [32] = true,
    [34] = true, [38] = true,
}

local function plain(v)
    if v == nil then return "" end
    if math.type(v) == "float" then
        if v == math.floor(v) and math.abs(v) < 1e15 then
            return string.format("%d", math.floor(v))
        end
        local s = string.format("%.15g", v)
        if tonumber(s) ~= v then s = string.format("%.17g", v) end
        return s
    end
    return tostring(v)
end

-- Value format (Phase 20). Masks are locale-free; see REPORTING_PLAN Value format.
local function decimal_string(a)
    -- a >= 0. Shortest round-trip digits, never scientific.
    local s = string.format("%.15g", a)
    if tonumber(s) ~= a then s = string.format("%.17g", a) end
    local mant, exp = s:match("^([%d%.]+)e([-+]%d+)$")
    if not mant then return s end
    local ip, fp = mant:match("^(%d*)%.?(%d*)$")
    local digits, point = ip .. fp, #ip + tonumber(exp)
    if point <= 0 then return "0." .. string.rep("0", -point) .. digits end
    if point >= #digits then return digits .. string.rep("0", point - #digits) end
    return digits:sub(1, point) .. "." .. digits:sub(point + 1)
end

local function plain_number(v)
    if v ~= v or v == math.huge or v == -math.huge then return tostring(v) end
    if math.type(v) == "integer" then return tostring(v) end
    local s = decimal_string(math.abs(v))
    if s:find("%.") then s = s:gsub("0+$", ""):gsub("%.$", "") end
    if v < 0 and s:match("[1-9]") then s = "-" .. s end
    return s
end

local function round_digits(int, frac, places)
    -- Half away from zero on the decimal string (sign handled by caller).
    if #frac <= places then return int, frac .. string.rep("0", places - #frac) end
    local up = frac:byte(places + 1) >= 53
    local all = int .. frac:sub(1, places)
    if up then
        local d, i = {}, #all
        for k = 1, #all do d[k] = all:byte(k) - 48 end
        while i >= 1 do
            if d[i] == 9 then d[i] = 0 i = i - 1 else d[i] = d[i] + 1 break end
        end
        for k = 1, #all do d[k] = string.char(d[k] + 48) end
        all = (i == 0 and "1" or "") .. table.concat(d)
    end
    local n = #all - places
    return all:sub(1, n), all:sub(n + 1)
end

local function number_mask(mask)
    local m, paren = mask, false
    local inner = m:match("^%((.*)%)$")
    if inner then m, paren = inner, true end
    local prefix = ""
    if m:sub(1, 1) == "$" then prefix, m = "$", m:sub(2) end
    local pct = false
    if m:sub(-1) == "%" then pct, m = true, m:sub(1, -2) end
    local ip, fp = m:match("^([#0,]*)%.([#0]*)$")
    if not ip then ip, fp = m:match("^([#0,]*)$"), "" end
    if not ip or (ip .. fp) == "" or not (ip .. fp):find("[#0]") then return nil end
    if fp:find("#0") then return nil end -- zeros must lead the fraction
    local _, minint = ip:gsub("0", "")
    local _, minfrac = fp:gsub("0", "")
    return {
        paren = paren, prefix = prefix, pct = pct, comma = ip:find(",") ~= nil,
        minint = minint, minfrac = minfrac, maxfrac = #fp,
    }
end

local function apply_number(v, spec)
    local x = spec.pct and v * 100 or v
    local neg = x < 0
    local s = decimal_string(math.abs(x))
    local int, frac = s:match("^(%d*)%.?(%d*)$")
    int, frac = round_digits(int, frac, spec.maxfrac)
    int = int:gsub("^0+", "")
    if #int < spec.minint then int = string.rep("0", spec.minint - #int) .. int end
    while #frac > spec.minfrac and frac:sub(-1) == "0" do frac = frac:sub(1, -2) end
    if spec.comma and #int > 3 then
        int = int:reverse():gsub("(%d%d%d)", "%1,"):reverse():gsub("^,", "")
    end
    local body = spec.prefix .. int .. (frac ~= "" and ("." .. frac) or "") .. (spec.pct and "%" or "")
    if not (int .. frac):find("[1-9]") then neg = false end
    if neg then return spec.paren and ("(" .. body .. ")") or ("-" .. body) end
    return body
end

local MONTHS = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" }

local function civil_from_days(z)
    z = z + 719468
    local era = (z >= 0 and z or z - 146096) // 146097
    local doe = z - era * 146097
    local yoe = (doe - doe // 1460 + doe // 36524 - doe // 146096) // 365
    local doy = doe - (365 * yoe + yoe // 4 - yoe // 100)
    local mp = (5 * doy + 2) // 153
    local d = doy - (153 * mp + 2) // 5 + 1
    local m = mp < 10 and mp + 3 or mp - 9
    return yoe + era * 400 + (m <= 2 and 1 or 0), m, d
end

local function parse_datetime(v)
    if type(v) == "number" then
        if v ~= v or v == math.huge or v == -math.huge then return nil end
        local secs = math.floor(v)
        local days, rem = secs // 86400, secs % 86400
        local y, m, d = civil_from_days(days)
        local ms = math.min(999, math.floor((v - secs) * 1000 + 1e-6))
        return { y = y, m = m, d = d, hh = rem // 3600, nn = rem % 3600 // 60, ss = rem % 60, ms = ms, time = rem ~= 0 }
    end
    if type(v) ~= "string" then return nil end
    local y, m, d = v:match("^(%d%d%d%d)%-(%d%d)%-(%d%d)$")
    local hh, nn, ss
    if not y then
        y, m, d, hh, nn, ss = v:match("^(%d%d%d%d)%-(%d%d)%-(%d%d)T(%d%d):(%d%d):(%d%d)$")
        if not y then return nil end
    end
    local t = { y = tonumber(y), m = tonumber(m), d = tonumber(d),
        hh = tonumber(hh) or 0, nn = tonumber(nn) or 0, ss = tonumber(ss) or 0, time = hh ~= nil }
    if t.m < 1 or t.m > 12 or t.d < 1 or t.d > 31 or t.hh > 23 or t.nn > 59 or t.ss > 59 then return nil end
    return t
end

-- Phase 22b: Luxon token subset. Literals in '...' ('' prints a quote). Locale-free English names.
local MONTHS_LONG = { "January", "February", "March", "April", "May", "June", "July",
    "August", "September", "October", "November", "December" }
local DAYS = { "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday" }
local DATE_TOKENS = { "yyyy", "yy", "MMMM", "MMM", "MM", "M", "dd", "d", "EEEE", "EEE",
    "HH", "H", "mm", "m", "ss", "s", "SSS", "WW", "W" }

local function days_from_civil(y, m, d)
    y = m <= 2 and y - 1 or y
    local era = (y >= 0 and y or y - 399) // 400
    local yoe = y - era * 400
    local doy = (153 * (m > 2 and m - 3 or m + 9) + 2) // 5 + d - 1
    return era * 146097 + yoe * 365 + yoe // 4 - yoe // 100 + doy - 719468
end

-- ISO weekday 1 (Mon) .. 7 (Sun); ISO week: week 1 holds the year's first Thursday.
local function iso_weekday(t)
    return (days_from_civil(t.y, t.m, t.d) + 3) % 7 + 1
end
local function iso_week(t)
    local days = days_from_civil(t.y, t.m, t.d)
    local thursday = days - iso_weekday(t) + 4
    local ty = civil_from_days(thursday)
    return (thursday - days_from_civil(ty, 1, 1)) // 7 + 1
end

local function apply_date(t, mask)
    local out, i, found = {}, 1, false
    while i <= #mask do
        local c = mask:sub(i, i)
        if c == "'" then
            local j = mask:find("'", i + 1, true)
            if j == i + 1 then
                out[#out + 1] = "'"
                i = i + 2
            else
                if not j then return nil end
                out[#out + 1] = mask:sub(i + 1, j - 1)
                i = j + 1
            end
        else
            local hit
            for _, tok in ipairs(DATE_TOKENS) do
                if mask:sub(i, i + #tok - 1) == tok then hit = tok break end
            end
            if hit == "yyyy" then out[#out + 1] = string.format("%04d", t.y)
            elseif hit == "yy" then out[#out + 1] = string.format("%02d", t.y % 100)
            elseif hit == "MMMM" then out[#out + 1] = MONTHS_LONG[t.m]
            elseif hit == "MMM" then out[#out + 1] = MONTHS[t.m]
            elseif hit == "MM" then out[#out + 1] = string.format("%02d", t.m)
            elseif hit == "M" then out[#out + 1] = tostring(t.m)
            elseif hit == "dd" then out[#out + 1] = string.format("%02d", t.d)
            elseif hit == "d" then out[#out + 1] = tostring(t.d)
            elseif hit == "EEEE" then out[#out + 1] = DAYS[iso_weekday(t)]
            elseif hit == "EEE" then out[#out + 1] = DAYS[iso_weekday(t)]:sub(1, 3)
            elseif hit == "HH" then out[#out + 1] = string.format("%02d", t.hh)
            elseif hit == "H" then out[#out + 1] = tostring(t.hh)
            elseif hit == "mm" then out[#out + 1] = string.format("%02d", t.nn)
            elseif hit == "m" then out[#out + 1] = tostring(t.nn)
            elseif hit == "ss" then out[#out + 1] = string.format("%02d", t.ss)
            elseif hit == "s" then out[#out + 1] = tostring(t.ss)
            elseif hit == "SSS" then out[#out + 1] = string.format("%03d", t.ms or 0)
            elseif hit == "WW" then out[#out + 1] = string.format("%02d", iso_week(t))
            elseif hit == "W" then out[#out + 1] = tostring(iso_week(t))
            else out[#out + 1] = c end
            if hit then found = true end
            i = i + (hit and #hit or 1)
        end
    end
    return found and table.concat(out) or nil
end

local function raw(v)
    if type(v) == "number" then return plain_number(v) end
    return plain(v)
end

local warned_format = {}
local function format_warn(item_id, band_id, message)
    local k = tostring(band_id) .. "/" .. tostring(item_id)
    if warned_format[k] then return end
    warned_format[k] = true
    add_log("warn", "item " .. tostring(item_id) .. " in band " .. tostring(band_id) .. " " .. message)
end

-- typeKey 21 number, 24 datetime, 20 text. Null prints blank and skips the mask.
local function format_value(v, tk, mask, item_id, band_id)
    if v == nil then return "" end
    if mask == "" then mask = nil end
    if tk == 21 then
        local n = type(v) == "number" and v or (type(v) == "string" and tonumber(v)) or nil
        if mask == nil then return n and plain_number(n) or raw(v) end
        local spec = number_mask(mask)
        if not spec then
            format_warn(item_id, band_id, "has mask " .. mask .. " which does not parse; printed raw")
            return raw(v)
        end
        if not n or n ~= n or n == math.huge or n == -math.huge then
            format_warn(item_id, band_id, "value " .. raw(v) .. " is not a number; printed raw")
            return raw(v)
        end
        return apply_number(n, spec)
    elseif tk == 24 then
        local t = parse_datetime(v)
        if not t then
            format_warn(item_id, band_id, "value " .. raw(v) .. " is not a date; printed raw")
            return raw(v)
        end
        if mask == nil then
            return t.time and apply_date(t, "yyyy-MM-dd HH:mm") or apply_date(t, "yyyy-MM-dd")
        end
        local s = apply_date(t, mask)
        if not s then
            format_warn(item_id, band_id, "has mask " .. mask .. " which does not parse; printed raw")
            return raw(v)
        end
        return s
    end
    if type(v) == "number" then return plain_number(v) end
    return raw(v)
end


local CSS_UNIT = { ["in"] = true, mm = true, cm = true, pt = true, px = true }
local ALIGN_CSS = { Left = "left", Center = "center", Right = "right" }

local function css_len(len)
    if type(len) ~= "string" then return nil end
    local n, u = len:match("^%s*([%d%.]+)%s*(%a+)%s*$")
    if not n or not CSS_UNIT[u] then return nil end
    return n .. u
end

local function css_or(len, fallback, what)
    local css = css_len(len)
    if css then return css end
    if len ~= nil and len ~= "" then
        add_log("warn", what .. " " .. tostring(len) .. " does not parse; used " .. fallback)
    end
    return fallback
end

local function esc(s)
    if s == nil then return "" end
    return (tostring(s):gsub("&", "&amp;"):gsub("<", "&lt;"):gsub(">", "&gt;"))
end

local function css_family(name)
    if type(name) ~= "string" or name == "" then name = "Vanadium Sans" end
    name = name:gsub("\\", "\\\\"):gsub("'", "\\'")
    return "'" .. name .. "'"
end

local function font_css(style)
    if style == nil or style == "" then style = "Normal" end
    if style ~= "Normal" and style ~= "Bold" and style ~= "Italic" and style ~= "Bold + Italic" then
        add_log("warn", "font style " .. tostring(style) .. " is not a v1 style; printed Normal")
        return "400", "normal"
    end
    local bold = style == "Bold" or style == "Bold + Italic"
    local italic = style == "Italic" or style == "Bold + Italic"
    return bold and "700" or "400", italic and "italic" or "normal"
end

-- Phase 44a: static shapes. Geometry is code. Path kinds use lookup 078
-- when that lookup is loaded, otherwise these commands. PDF does not draw them.
local shape_rows = {}
local shape_loaded = false
local shape_catalog_on = false
local SHAPE_KAPPA = 0.5522847498
local SHAPE_GEOMETRY = { line = true, rectangle = true, roundrect = true, ellipse = true }
local SHAPE_UNIT_PX = { ["in"] = 96, mm = 96 / 25.4, cm = 96 / 2.54, pt = 96 / 72, px = 1 }
local SHAPE_EMBEDDED = {
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

local function shape_fmt(n)
    if type(n) ~= "number" or n ~= n or n == math.huge or n == -math.huge then n = 0 end
    local neg = n < 0
    local rounded = math.floor(math.abs(n) * 10000 + 0.5) / 10000
    if rounded == 0 then return "0" end
    local text = string.format("%.4f", rounded):gsub("0+$", ""):gsub("%.$", "")
    if neg then return "-" .. text end
    return text
end

local function shape_px(len)
    if type(len) ~= "string" then return nil end
    local n, u = len:match("^%s*([%+%-]?[%d%.]+)%s*(%a+)%s*$")
    n = tonumber(n)
    if not n or not SHAPE_UNIT_PX[u] then return nil end
    local px = n * SHAPE_UNIT_PX[u]
    local nearest = math.floor(px + 0.5)
    if math.abs(px - nearest) < 1e-9 then return nearest end
    return px
end

local function shape_len(value, fallback)
    if type(value) ~= "string" or value:match("^%s*$") then return fallback end
    local px = shape_px(value)
    if not px then return fallback end
    return px
end

local function shape_hex(value, fallback)
    if type(value) == "string" and value:match("^#[0-9A-Fa-f][0-9A-Fa-f][0-9A-Fa-f][0-9A-Fa-f][0-9A-Fa-f][0-9A-Fa-f]$") then
        return value:lower()
    end
    return fallback
end

local function shape_commands_ok(commands)
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

local function shape_source(kind)
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

local function shape_scale(commands, w, h)
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

local function shape_d(commands)
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

local function shape_clamp(w, h, radii)
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

local function shape_round(w, h, r)
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

local function shape_ellipse(w, h)
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

local function shape_line(w, h, side)
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
local SHAPE_FILL = {
    none = true, solid = true, grid = true, dots = true, hatched = true, noise = true,
}
local SHAPE_GRID_OK = { dots = true, grid = true, dotted = true, dashed = true, none = true }

local function shape_mul32(a, b)
    local a_lo = a % 65536
    local a_hi = math.floor(a / 65536) % 65536
    local b_lo = b % 65536
    local b_hi = math.floor(b / 65536) % 65536
    local p0 = a_lo * b_lo
    local p1 = a_lo * b_hi + a_hi * b_lo
    return (p0 + (p1 % 65536) * 65536) % 4294967296
end

local function shape_xor_byte(hash, byte)
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

local function shape_fnv(text)
    local hash = 2166136261
    local s = tostring(text or "")
    for i = 1, #s do
        hash = shape_mul32(shape_xor_byte(hash, string.byte(s, i)), 16777619)
    end
    return hash
end

local function shape_lcg(seed)
    local state = seed
    if state == 0 then state = 1 end
    return function()
        state = (shape_mul32(1664525, state) + 1013904223) % 4294967296
        return state / 4294967296
    end
end

local function shape_dom(prefix, id)
    local cleaned = tostring(id or ""):gsub("[^A-Za-z0-9_%-]", "")
    if cleaned == "" then cleaned = "item" end
    return prefix .. cleaned
end

local function shape_weight_ok(weight)
    if weight == nil or weight == "" then return true end
    local n = tonumber(weight)
    if not n or n ~= n or n < 0 or n == math.huge then return false end
    return true
end

local function shape_spacing(def)
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

local function shape_stops(stops)
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

local function shape_angle(paint)
    local n = tonumber(paint.angle)
    if n and n == n and n ~= math.huge and n ~= -math.huge then return n end
    return 0
end

local function shape_resolve(paint)
    local color = "#d3d3d3"
    local kind = "solid"
    if type(paint) == "table" then
        color = shape_hex(paint.color, "#d3d3d3")
        if type(paint.kind) == "string" then kind = paint.kind end
    end
    if kind ~= "linear" and kind ~= "radial" then return { color = color } end
    local stops = type(paint) == "table" and shape_stops(paint.stops) or nil
    if not stops then return { color = color } end
    if kind == "radial" then return { color = color, gradient = { kind = "radial", stops = stops } } end
    return { color = color, gradient = { kind = "linear", angle = shape_angle(paint), stops = stops } }
end

local function shape_linear(w, h, angle)
    local rad = angle * math.pi / 180
    local dx, dy = math.cos(rad), math.sin(rad)
    local cx, cy = w / 2, h / 2
    local extent = math.abs(dx) * w / 2 + math.abs(dy) * h / 2
    return cx - dx * extent, cy - dy * extent, cx + dx * extent, cy + dy * extent
end

local function shape_place(gradient, w, h)
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

local function shape_grid(w, h, spacing)
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

local function shape_hatch_add(points, x, y)
    for _, p in ipairs(points) do
        if math.abs(p.x - x) < 1e-6 and math.abs(p.y - y) < 1e-6 then return end
    end
    points[#points + 1] = { x = x, y = y }
end

local function shape_hatch_seg(points)
    if #points ~= 2 then return nil end
    local p, q = points[1], points[2]
    if not (p.x < q.x - 1e-9 or (math.abs(p.x - q.x) <= 1e-9 and p.y <= q.y)) then
        p, q = q, p
    end
    return "M " .. shape_fmt(p.x) .. " " .. shape_fmt(p.y) .. " L " .. shape_fmt(q.x) .. " " .. shape_fmt(q.y)
end

local function shape_hatch(w, h, spacing)
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

local function shape_dots(w, h, spacing)
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

local function shape_noise(id, w, h, spacing)
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

local function shape_step(spacing)
    if type(spacing) == "number" and spacing > 0 then return spacing end
    return shape_px("0.125 in")
end

local function shape_marks(style, item, w, h, spacing)
    local step = shape_step(spacing)
    if style == "grid" then return { kind = "lines", d = shape_grid(w, h, step), sw = 0.5 } end
    if style == "hatched" then return { kind = "lines", d = shape_hatch(w, h, step), sw = 0.5 } end
    if style == "dots" then return { kind = "circles", circles = shape_dots(w, h, step) } end
    return { kind = "circles", circles = shape_noise(item.id, w, h, step) }
end

local function shape_style(item, kind)
    if kind == "line" then return "none" end
    local style = item.fillStyle
    if style == nil or style == "" then return "solid" end
    if not SHAPE_FILL[style] then return "none" end
    return style
end

local function shape_plan(item, kind, w, h, spacing)
    local style = shape_style(item, kind)
    local raw = type(item.fillPaint) == "table" and item.fillPaint or nil
    local paint = shape_resolve(raw)
    if style == "none" then return { style = style, color = paint.color } end
    local gradient = shape_place(paint.gradient, w, h)
    if style == "solid" then return { style = style, color = paint.color, gradient = gradient } end
    return {
        style = style, color = paint.color, gradient = gradient,
        marks = shape_marks(style, item, w, h, spacing),
    }
end

local function shape_attr(attrs)
    local parts = {}
    for i = 1, #attrs do
        parts[i] = attrs[i][1] .. '="' .. attrs[i][2] .. '"'
    end
    return table.concat(parts, " ")
end

local function shape_gradient(draw)
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

local function shape_with_transform(attrs, transform)
    if transform ~= "" then attrs[#attrs + 1] = { "transform", transform } end
    return attrs
end

local function shape_mark_svg(draw)
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

local function shape_outline(draw)
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

local function shape_defs(draw)
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

local function shape_markup(draw)
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
local function shape_svg(item, spacing)
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

-- Phase 44c: one Helvetica AFM break for the designer, HTML, and later PDF.
-- A grown item is drawn in Helvetica. embedFonts does not switch the measure.
-- Index 1 is U+0020. A missing glyph uses 278. ASCII 32-126 only.
local AFM_ROMAN = {
    278, 278, 355, 556, 556, 889, 667, 222, 333, 333, 389, 584,
    278, 333, 278, 278, 556, 556, 556, 556, 556, 556, 556, 556,
    556, 556, 278, 278, 584, 584, 584, 556, 1015, 667, 667, 722,
    722, 667, 611, 778, 722, 278, 500, 667, 556, 833, 722, 778,
    667, 778, 722, 667, 611, 722, 667, 944, 667, 667, 611, 278,
    278, 278, 469, 556, 222, 556, 556, 500, 556, 556, 278, 556,
    556, 222, 222, 500, 222, 833, 556, 556, 556, 556, 333, 500,
    278, 556, 500, 722, 500, 500, 500, 334, 260, 334, 584,
}
local AFM_BOLD = {
    278, 333, 474, 556, 556, 889, 722, 278, 333, 333, 389, 584,
    278, 333, 278, 278, 556, 556, 556, 556, 556, 556, 556, 556,
    556, 556, 333, 333, 584, 584, 584, 611, 975, 722, 722, 722,
    722, 667, 611, 778, 722, 278, 556, 722, 611, 833, 722, 778,
    667, 778, 722, 667, 611, 722, 667, 944, 667, 667, 611, 333,
    278, 333, 584, 556, 278, 556, 611, 556, 611, 556, 333, 611,
    611, 278, 278, 556, 278, 889, 611, 611, 611, 611, 389, 556,
    333, 611, 556, 778, 556, 556, 500, 389, 280, 389, 584,
}
local AFM_MISSING = 278
local WRAP_MODES = { normal = true, pre = true, ["pre-line"] = true, ["pre-wrap"] = true, nowrap = true }
local FIT_HEIGHTS = { clip = true, grow = true, shrink = true, both = true }
local VALIGNS = { top = true, middle = true, bottom = true }
local LAYOUT_EPS = 1e-6
local MIN_BAND_PT = 0.15 * 72
local LAYOUT_UNIT = { ["in"] = 72, mm = 72 / 25.4, cm = 72 / 2.54, pt = 1, px = 0.75 }

local function fmt_num(n)
    if type(n) ~= "number" or n ~= n or n == math.huge or n == -math.huge then n = 0 end
    local neg = n < 0
    local s = string.format("%.6f", math.abs(n))
    s = s:gsub("0+$", ""):gsub("%.$", "")
    if s == "" then s = "0" end
    if neg and s ~= "0" then s = "-" .. s end
    return s
end

local function fmt_pt(n)
    return fmt_num(n) .. "pt"
end

local function length_pt(len)
    if type(len) ~= "string" then return nil end
    local n, u = len:match("^%s*([%+%-]?[%d%.]+)%s*(%a+)%s*$")
    n = tonumber(n)
    if not n or not LAYOUT_UNIT[u] then return nil end
    return n * LAYOUT_UNIT[u]
end

local function half_away(n)
    if type(n) ~= "number" or n ~= n or n == math.huge or n == -math.huge then return 0 end
    if n >= 0 then return math.floor(n + 0.5) end
    return -math.floor(-n + 0.5)
end

local function face_bold(style)
    return style == "Bold" or style == "Bold + Italic"
end

local function advance_units(ch, style)
    local b = string.byte(ch)
    local table = face_bold(style) and AFM_BOLD or AFM_ROMAN
    if not b or b < 32 or b > 126 then return AFM_MISSING end
    return table[b - 31]
end

local function advance_pt(ch, size, style)
    return advance_units(ch, style) * size / 1000
end

local function measure_pt(text, size, style)
    local width = 0
    for i = 1, #text do width = width + advance_pt(text:sub(i, i), size, style) end
    return width
end

local function line_height_pt(size, multiple)
    local factor = 1.2
    if type(multiple) == "number" and multiple == multiple and multiple > 0 and multiple ~= math.huge then
        factor = multiple
    end
    return factor * size
end

local function collapse_ws(text)
    local s = text:gsub("[ \t\n\r\f]+", " ")
    return (s:gsub("^[ ]+", ""):gsub("[ ]+$", ""))
end

local function collapse_line(text)
    local s = text:gsub("[ \t\f]+", " ")
    return (s:gsub("^[ ]+", ""):gsub("[ ]+$", ""))
end

local function split_chars(text)
    local out = {}
    for i = 1, #text do out[i] = text:sub(i, i) end
    return out
end

local function split_words(text)
    local out = {}
    for word in text:gmatch("%S+") do out[#out + 1] = word end
    return out
end

local function split_nl(text)
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

local function break_word(word, width, size, style)
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

local function pack_words(words, width, size, style, char_break)
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

local function wrap_pre(text, width, size, style)
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

local function wrap_text(text, width, font_size, style, mode)
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

local function default_fit(type_key)
    if type_key == 3 then return "both" end
    return "clip"
end

local function default_wrap(type_key)
    if type_key == 3 then return "pre-wrap" end
    if type_key == 1 or type_key == 4 then return "nowrap" end
    return "pre-line"
end

local function paints_lines(item, fit)
    local tk = tonumber(item and item.typeKey)
    if tk == 1 or tk == 3 or tk == 4 then return true end
    if (tk == 0 or tk == 20) and fit ~= "clip" then return true end
    return false
end

local function layout_warn(seen, key, message)
    if seen[key] then return end
    seen[key] = true
    add_log("warn", message)
end

local function font_of(doc, item)
    local own = type(item.font) == "table" and item.font or nil
    local report = type(doc) == "table" and type(doc.font) == "table" and doc.font or nil
    local source = own or report or {}
    return {
        family = (type(source.family) == "string" and source.family ~= "") and source.family or "Vanadium Sans",
        size = (type(source.size) == "string" and source.size ~= "") and source.size or "10 pt",
        style = (type(source.style) == "string" and source.style ~= "") and source.style or "Normal",
    }
end

local function resolve_wrap(item, seen)
    local tk = tonumber(item.typeKey) or 0
    local mode = item.wrap
    if mode == nil or mode == "" then return default_wrap(tk) end
    if WRAP_MODES[mode] then return mode end
    local fallback = default_wrap(tk)
    layout_warn(seen, "wrap:" .. tostring(item.id),
        "item " .. tostring(item.id) .. " wrap " .. tostring(mode) .. " is not a wrap mode; used " .. fallback)
    return fallback
end

local function resolve_fit_height(item, seen)
    local fallback = default_fit(tonumber(item.typeKey) or 0)
    local fit = item.fitHeight
    if fit == nil or fit == "" then return fallback end
    if FIT_HEIGHTS[fit] then return fit end
    layout_warn(seen, "fit:" .. tostring(item.id),
        "item " .. tostring(item.id) .. " fitHeight " .. tostring(fit)
        .. " is not clip, grow, shrink, or both; used " .. fallback)
    return fallback
end

local function resolve_valign(item, seen)
    local valign = item.valign
    if valign == nil or valign == "" then return "top" end
    if VALIGNS[valign] then return valign end
    layout_warn(seen, "valign:" .. tostring(item.id),
        "item " .. tostring(item.id) .. " valign " .. tostring(valign) .. " is not top, middle, or bottom; used top")
    return "top"
end

local function resolve_multiple(item, seen)
    if item.lineHeight == nil or item.lineHeight == "" then return 1.2 end
    local value = tonumber(item.lineHeight)
    if value and value == value and value > 0 and value ~= math.huge then return value end
    layout_warn(seen, "lh:" .. tostring(item.id),
        "item " .. tostring(item.id) .. " lineHeight " .. tostring(item.lineHeight)
        .. " is not a positive multiple; used 1.2")
    return 1.2
end

local function valign_offset(valign, box_pt, block_pt)
    local slack = box_pt - block_pt
    if slack <= LAYOUT_EPS then return 0 end
    if valign == "bottom" then return slack end
    if valign == "middle" then return slack / 2 end
    return 0
end

local function fitted_height(fit, stored_pt, content_pt)
    if fit == "grow" then return math.max(stored_pt, content_pt) end
    if fit == "shrink" then return math.min(stored_pt, content_pt) end
    if fit == "both" then return content_pt end
    return stored_pt
end

local function layout_band(doc, band, entries)
    local seen, placed, reflow = {}, {}, false
    for _, entry in ipairs(entries or {}) do
        local item = entry.item or {}
        local fit = entry.defer and "clip" or resolve_fit_height(item, seen)
        if fit ~= "clip" then reflow = true end
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
        local leading = line_height_pt(font_size, multiple)
        local mode = resolve_wrap(item, seen)
        local valign = resolve_valign(item, seen)
        local lines = nil
        if paints_lines(item, fit) and not entry.defer then
            lines = wrap_text(entry.text or "", width, font_size, font.style, mode)
        end
        local stored_h = (height and height > 0) and height or 0
        local content_h = math.max(1, lines and #lines or 1) * leading
        local used_h = (entry.defer or not lines) and stored_h or fitted_height(fit, stored_h, content_h)
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
        placed[#placed + 1] = {
            item = item, fit = fit, lines = painted, font = font, leading = leading,
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
        }
    end
    return { reflow = reflow, usedPt = used_pt, items = items }
end

local IMAGE_FITS = { stretch = true, contain = true, cover = true, none = true, ["scale-down"] = true }
local IMAGE_FLIPS = { none = true, horizontal = true, vertical = true, both = true }
local IMAGE_ALIGNS = { Left = true, Center = true, Right = true }
local IMAGE_VALIGNS = { Top = true, Middle = true, Bottom = true }
local SVG_TAGS = {
    svg = true, g = true, path = true, rect = true, circle = true,
    ellipse = true, line = true, polyline = true, polygon = true,
    defs = true, style = true,
}

local function svg_subset(uri)
    if type(uri) ~= "string" then return "" end
    local comma = uri:find(",", 1, true)
    if not comma then return "" end
    local meta = uri:sub(1, comma - 1):lower()
    if meta:find("base64", 1, true) then return "" end
    local payload = uri:sub(comma + 1):gsub("%%(%x%x)", function(h)
        return string.char(tonumber(h, 16))
    end)
    local lower = payload:lower()
    if lower:find("javascript:", 1, true) or lower:find("<script", 1, true)
        or lower:find("<foreignobject", 1, true) or lower:find("<text", 1, true)
        or lower:find("<image", 1, true) or lower:find("<filter", 1, true)
        or lower:find(" on%w+%s*=") then
        return ""
    end
    if not lower:find("<svg", 1, true) then return "" end
    for tag in payload:gmatch("</?([%a][%w:%-]*)") do
        local name = tag:match("^[%a]+") or tag
        if not SVG_TAGS[name:lower()] then return "" end
    end
    return payload
end

local function display_px(len, dpi)
    local pt = length_pt(len)
    if not pt then return 1 end
    local density = 72
    if type(dpi) == "number" and dpi == dpi and dpi > 0 and dpi ~= math.huge then density = dpi end
    local px = half_away(pt / 72 * density)
    if px < 1 then px = 1 end
    return px
end

local function raster_lookup(key)
    local rasters = type(P.rasters) == "table" and P.rasters or nil
    if not rasters then return nil end
    local entry = rasters[key]
    if type(entry) == "string" and entry ~= "" then return entry end
    if type(entry) == "table" and type(entry.data) == "string" and entry.data ~= "" then return entry.data end
    return nil
end

local function image_once(state, key, message)
    if state.warned[key] then return end
    state.warned[key] = true
    add_log("warn", message)
end

local function image_fit(o, state)
    local fit = o.fit
    if fit == nil or fit == "" or IMAGE_FITS[fit] then return (fit ~= nil and fit ~= "") and fit or "contain" end
    image_once(state, "fit:" .. tostring(o.id),
        "item " .. tostring(o.id) .. " fit " .. tostring(fit) .. " is not a fit; used contain")
    return "contain"
end

local function image_flip(o, state)
    local flip = o.flip
    if flip == nil or flip == "" or IMAGE_FLIPS[flip] then return (flip ~= nil and flip ~= "") and flip or "none" end
    image_once(state, "flip:" .. tostring(o.id),
        "item " .. tostring(o.id) .. " flip " .. tostring(flip) .. " is not a flip; used none")
    return "none"
end

local function image_anchor(o, state, horizontal)
    local value = horizontal and o.fitAlign or o.fitValign
    local allowed = horizontal and IMAGE_ALIGNS or IMAGE_VALIGNS
    local fallback = horizontal and "Center" or "Middle"
    if value == nil or value == "" or allowed[value] then
        return (value ~= nil and value ~= "") and value or fallback
    end
    image_once(state, (horizontal and "ax:" or "ay:") .. tostring(o.id),
        "item " .. tostring(o.id) .. " anchor " .. tostring(value) .. " is not an anchor; used " .. fallback)
    return fallback
end

local function image_opacity(o)
    if o.opacity == nil or o.opacity == "" then return "1" end
    local n = tonumber(o.opacity)
    if not n or n ~= n or n == math.huge or n == -math.huge then return "1" end
    if n < 0 then n = 0 elseif n > 1 then n = 1 end
    return fmt_num(n)
end

local function image_rotation(o, state)
    if o.rotation == nil or o.rotation == "" then return 0 end
    local n = tonumber(o.rotation)
    if not n or n ~= n or n == math.huge or n == -math.huge then
        image_once(state, "rot:" .. tostring(o.id),
            "item " .. tostring(o.id) .. " rotation " .. tostring(o.rotation) .. " is not a number; used 0")
        return 0
    end
    return n
end

local function image_transform(o, state, pxw, pxh)
    local flip = image_flip(o, state)
    local fx, fy = 1, 1
    if flip == "horizontal" or flip == "both" then fx = -1 end
    if flip == "vertical" or flip == "both" then fy = -1 end
    local rot = image_rotation(o, state)
    local cx, cy = pxw / 2, pxh / 2
    return string.format(
        "rotate(%s %s %s) translate(%s %s) scale(%s %s) translate(%s %s)",
        fmt_num(rot), fmt_num(cx), fmt_num(cy),
        fmt_num(cx), fmt_num(cy), fmt_num(fx), fmt_num(fy),
        fmt_num(-cx), fmt_num(-cy))
end

local function emit_image(def, o, box, state)
    local hash = o.image
    if type(hash) ~= "string" or hash == "" then
        image_once(state, "missing:" .. tostring(o.id),
            "item " .. tostring(o.id) .. " has no image; skipped")
        return nil
    end
    local images = type(def.images) == "table" and def.images or {}
    local canonical = images[hash]
    local left = css_or(box.left, "0in", "box left")
    local top = css_or(box.top, "0in", "box top")
    local width = css_or(box.width, "0in", "box width")
    local height = css_or(box.height, "0in", "box height")
    local opacity = image_opacity(o)
    if type(canonical) == "string" and canonical:lower():find("^data:image/svg%+xml") then
        local markup = svg_subset(canonical)
        if markup == "" then
            image_once(state, "svg:" .. tostring(o.id),
                "item " .. tostring(o.id) .. " svg is outside the path subset; skipped")
            return nil
        end
        return string.format(
            '<div style="position:absolute;left:%s;top:%s;width:%s;height:%s;overflow:hidden;padding:0;opacity:%s">%s</div>',
            left, top, width, height, opacity, markup)
    end
    local pxw = display_px(box.width, o.dpi)
    local pxh = display_px(box.height, o.dpi)
    local fit = image_fit(o, state)
    local align = image_anchor(o, state, true)
    local valign = image_anchor(o, state, false)
    local key = table.concat({
        hash, tostring(pxw), tostring(pxh), fit, align, valign,
        o.grayscale == true and "true" or "false",
        o.invert == true and "true" or "false",
    }, "|")
    local data = raster_lookup(key)
    if data == nil then
        if type(canonical) == "string" and canonical ~= "" then
            data = canonical
            if not state.dpi then
                state.dpi = true
                add_log("warn", "display dpi was not applied")
            end
        else
            image_once(state, "bytes:" .. hash,
                "item " .. tostring(o.id) .. " image " .. hash .. " has no bytes; skipped")
            return nil
        end
    end
    local id = state.ids[key]
    if not id then
        state.n = state.n + 1
        id = "rptimg" .. tostring(state.n)
        state.ids[key] = id
        state.list[#state.list + 1] = { id = id, data = data, w = pxw, h = pxh }
    end
    return string.format(
        '<div style="position:absolute;left:%s;top:%s;width:%s;height:%s;overflow:hidden;padding:0;opacity:%s"><svg width="100%%" height="100%%" viewBox="0 0 %d %d" preserveAspectRatio="none"><use href="#%s" transform="%s"/></svg></div>',
        left, top, width, height, opacity, pxw, pxh, id, image_transform(o, state, pxw, pxh))
end

local function embed_font_css(def, pages)
    if type(def) ~= "table" or def.embedFonts ~= true then return "" end
    local fonts = type(P.fonts) == "table" and P.fonts or nil
    if not fonts then
        add_log("warn", "embedFonts is on and params.fonts is missing; named the family")
        return ""
    end
    local used, order = {}, {}
    for _, page in ipairs(pages or {}) do
        for _, band in ipairs(page.bands or {}) do
            for _, o in ipairs(band.items or {}) do
                local fam = type(o.font) == "table" and o.font.family or nil
                if type(fam) == "string" and fam ~= "" and fam ~= "Helvetica" and not used[fam] then
                    used[fam] = true
                    order[#order + 1] = fam
                end
            end
        end
    end
    local rules, missing = {}, false
    for _, fam in ipairs(order) do
        local src = fonts[fam]
        if type(src) ~= "string" or src == "" then
            if not missing then
                missing = true
                add_log("warn", "embedFonts has no bytes for " .. fam .. "; named the family")
            end
        else
            local uri = src
            if not uri:find("^data:") then uri = "data:font/woff2;base64," .. uri end
            rules[#rules + 1] = "@font-face{font-family:" .. css_family(fam)
                .. ";src:url(" .. uri .. ") format('woff2');}"
        end
    end
    if #rules == 0 then return "" end
    return "<style>" .. table.concat(rules) .. "</style>"
end

local BARCODE_NAMES = {
    qr = "qrcode", code39 = "code39", code128 = "code128",
    upca = "upca", ["gs1-128"] = "gs1128",
}
local BARCODE_BOOL = {
    "gs1", "gs1parens", "nobackground", "heightperrow", "compliantheight",
    "bind", "bindtop", "box", "quietzones", "noquietzones", "notext", "small",
    "bold", "embedfont", "dotty", "reverse", "init", "esc", "extraesc",
}
local BARCODE_NUM = {
    "fg", "bg", "scale", "scalexdimdp", "height", "border", "whitesp",
    "vwhitesp", "textgap", "guarddescent", "dotsize",
}
local BARCODE_OMIT = { scale = 1, textgap = 1, guarddescent = 5, border = 0, whitesp = 0, vwhitesp = 0 }
local FIT_X = { Left = "Min", Center = "Mid", Right = "Max" }
local FIT_Y = { Top = "Min", Middle = "Mid", Bottom = "Max" }
local barcode_mod
local barcode_missing = false

local function barcode_symbol(bc)
    local name = bc.symbology
    if type(name) ~= "string" or name == "" then name = "code128" end
    if name == "upca" and bc.check == "supplied" then return "upcachk" end
    local sym = BARCODE_NAMES[name]
    if not sym then return nil, "unknown symbology" end
    return sym
end

local function barcode_key_ok(sym, key)
    if (key == "gs1" or key == "gs1parens" or key == "dotty" or key == "dotsize") and sym ~= "qrcode" then
        return false
    end
    if key == "extraesc" and sym ~= "code128" then return false end
    if key == "guarddescent" and sym ~= "upca" and sym ~= "upcachk" then return false end
    if key == "gs1" and sym == "gs1128" then return false end
    return true
end

local function barcode_color(value)
    if type(value) ~= "string" or value == "" then return nil end
    if value:sub(1, 1) == "#" then return value:sub(2) end
    return value
end

local function barcode_options(sym, bc, data)
    local opts = { barcode = sym, filetype = "SVG", data = data == nil and "" or tostring(data) }
    for _, key in ipairs(BARCODE_BOOL) do
        if barcode_key_ok(sym, key) and bc[key] == true then opts[key] = true end
    end
    for _, key in ipairs(BARCODE_NUM) do
        if barcode_key_ok(sym, key) and bc[key] ~= nil and bc[key] ~= "" then
            local value = bc[key]
            if key == "fg" or key == "bg" then
                value = barcode_color(value)
            else
                value = tonumber(value)
                if value ~= nil and BARCODE_OMIT[key] == value then value = nil end
            end
            if value ~= nil then opts[key] = value end
        end
    end
    return opts
end

local function barcode_fn()
    if barcode_mod or barcode_missing then return barcode_mod end
    local ok, fn = pcall(require, "zint.barcode")
    if not ok or type(fn) ~= "function" then
        barcode_missing = true
        return nil
    end
    barcode_mod = fn
    return fn
end

local function barcode_note(id, message, warned)
    local key = tostring(id) .. "\0" .. message
    if warned[key] then return end
    warned[key] = true
    add_log("warn", "item " .. tostring(id) .. " " .. message)
end

local function fit_aspect(item)
    local x = FIT_X[item.fitAlign] or "Mid"
    local y = FIT_Y[item.fitValign] or "Mid"
    return "x" .. x .. "Y" .. y .. " meet"
end

local function fit_svg(svg, item)
    if type(svg) ~= "string" then return nil end
    local w, h = svg:match('<svg width="(%d+)" height="(%d+)"')
    if not w then return nil end
    local open = string.format(
        '<svg width="100%%" height="100%%" viewBox="0 0 %s %s" preserveAspectRatio="%s" xmlns="http://www.w3.org/2000/svg">',
        w, h, fit_aspect(item))
    return (svg:gsub("<svg[^>]*>", function() return open end, 1))
end

local function emit_barcode(o, warned)
    local bc = type(o.barcode) == "table" and o.barcode or {}
    local sym, why = barcode_symbol(bc)
    if not sym then
        barcode_note(o.id, why or "unknown symbology", warned)
        return nil
    end
    if o.typeKey == 45 and (bc.data == nil or bc.data == "") then
        return nil
    end
    local fn = barcode_fn()
    if not fn then
        barcode_note(o.id, "zint.barcode is not loaded", warned)
        return nil
    end
    local opts = barcode_options(sym, bc, bc.data)
    local ok, out, warning = pcall(fn, opts)
    if not ok or out == nil or type(out) ~= "string" then
        local msg = "barcode failed"
        if ok and type(warning) == "string" then
            local err = warning:match("^(Error %d+)")
            if err then msg = err end
        end
        barcode_note(o.id, msg, warned)
        return nil
    end
    local fitted = fit_svg(out, o)
    if not fitted then
        barcode_note(o.id, "barcode failed", warned)
        return nil
    end
    local box = type(o.box) == "table" and o.box or {}
    local turn = tonumber(o.rotation) or 0
    local extra = ""
    if turn ~= 0 then extra = string.format(";transform:rotate(%ddeg)", turn) end
    return string.format(
        '<div style="position:absolute;left:%s;top:%s;width:%s;height:%s;overflow:hidden;padding:0%s">%s</div>',
        css_or(box.left, "0in", "box left"),
        css_or(box.top, "0in", "box top"),
        css_or(box.width, "0in", "box width"),
        css_or(box.height, "0in", "box height"),
        extra,
        fitted)
end

local function emit_html(def, pages)
    local shape_warned = {}
    local barcode_warned = {}
    local image_state = { ids = {}, list = {}, n = 0, warned = {}, dpi = false }
    local pg = type(def.page) == "table" and def.page or {}
    local mg = type(pg.margin) == "table" and pg.margin or {}
    local width = css_or(pg.width, "8.5in", "page width")
    local height = css_or(pg.height, "11in", "page height")
    local top = css_or(mg.top, "0.25in", "margin top")
    local right = css_or(mg.right, "0.25in", "margin right")
    local bottom = css_or(mg.bottom, "0.25in", "margin bottom")
    local left = css_or(mg.left, "0.25in", "margin left")
    local parts = {}
    for _, page in ipairs(pages or {}) do
        local bands = {}
        for _, band in ipairs(page.bands or {}) do
            local items = {}
            for _, o in ipairs(band.items or {}) do
                local box = type(o.box) == "table" and o.box or nil
                if not box then
                    add_log("warn", "item " .. tostring(o.id) .. " has no box; skipped")
                elseif o.typeKey == 25 then
                    local svg, swarn = shape_svg(o, shape_spacing(def))
                    if swarn ~= "" then
                        local skey = tostring(o.id) .. "\0" .. swarn
                        if not shape_warned[skey] then
                            shape_warned[skey] = true
                            add_log("warn", "item " .. tostring(o.id) .. " " .. swarn)
                        end
                    else
                        items[#items + 1] = string.format(
                            '<div style="position:absolute;left:%s;top:%s;width:%s;height:%s;overflow:visible;padding:0">%s</div>',
                            css_or(box.left, "0in", "box left"),
                            css_or(box.top, "0in", "box top"),
                            css_or(box.width, "0in", "box width"),
                            css_or(box.height, "0in", "box height"),
                            svg)
                    end
                elseif o.typeKey == 43 or o.typeKey == 45 then
                    local html = emit_barcode(o, barcode_warned)
                    if html then items[#items + 1] = html end
                elseif type(o.lines) == "table" then
                    local font = type(o.font) == "table" and o.font or {}
                    local weight, slant = font_css(font.style)
                    local align = ALIGN_CSS[o.align]
                    if not align then
                        if o.align ~= nil then
                            add_log("warn", "align " .. tostring(o.align) .. " is not Left, Center, or Right; printed left")
                        end
                        align = "left"
                    end
                    local pad = o.offsetPt or 0
                    local padding = math.abs(pad) < 1e-6 and "0" or (fmt_pt(pad) .. " 0 0 0")
                    local chunks = {}
                    for _, line in ipairs(o.lines) do chunks[#chunks + 1] = esc(line) end
                    items[#items + 1] = string.format(
                        '<div style="position:absolute;left:%s;top:%s;width:%s;height:%s;overflow:hidden;padding:%s;white-space:pre;line-height:%s;text-align:%s;font-family:%s;font-size:%s;font-weight:%s;font-style:%s">%s</div>',
                        css_or(box.left, "0in", "box left"),
                        css_or(box.top, "0in", "box top"),
                        css_or(box.width, "0in", "box width"),
                        css_or(box.height, "0in", "box height"),
                        padding,
                        fmt_pt(o.lineHeightPt or 12),
                        align,
                        css_family(font.family),
                        css_or(font.size, "10pt", "font size"),
                        weight,
                        slant,
                        table.concat(chunks, "\n"))
                elseif o.typeKey == 2 then
                    local html = emit_image(def, o, box, image_state)
                    if html then items[#items + 1] = html end
                else
                    local font = type(o.font) == "table" and o.font or {}
                    local weight, slant = font_css(font.style)
                    local align = ALIGN_CSS[o.align]
                    if not align then
                        if o.align ~= nil then
                            add_log("warn", "align " .. tostring(o.align) .. " is not Left, Center, or Right; printed left")
                        end
                        align = "left"
                    end
                    items[#items + 1] = string.format(
                        '<div style="position:absolute;left:%s;top:%s;width:%s;height:%s;overflow:hidden;white-space:pre-line;text-align:%s;font-family:%s;font-size:%s;font-weight:%s;font-style:%s">%s</div>',
                        css_or(box.left, "0in", "box left"),
                        css_or(box.top, "0in", "box top"),
                        css_or(box.width, "0in", "box width"),
                        css_or(box.height, "0in", "box height"),
                        align,
                        css_family(font.family),
                        css_or(font.size, "10pt", "font size"),
                        weight,
                        slant,
                        esc(o.text))
                end
            end
            bands[#bands + 1] = string.format(
                '<div style="position:relative;height:%s">%s</div>',
                css_or(band.usedHeight or band.height, "0in", "band height"),
                table.concat(items))
        end
        parts[#parts + 1] = string.format(
            '<section style="position:relative;box-sizing:border-box;width:%s;height:%s;padding:%s %s %s %s;break-after:page">%s</section>',
            width, height, top, right, bottom, left, table.concat(bands))
    end
    local defs = ""
    if #image_state.list > 0 then
        local tags = {}
        for _, im in ipairs(image_state.list) do
            tags[#tags + 1] = string.format('<image id="%s" href="%s" width="%d" height="%d"/>', im.id, im.data, im.w, im.h)
        end
        defs = '<svg width="0" height="0"><defs>' .. table.concat(tags) .. '</defs></svg>'
    end
    return embed_font_css(def, pages) .. defs .. table.concat(parts, "\n") .. "\n"
end

-- A stored report is text. Hydrogen does not parse that column into a table.
-- phase27:json-start
local function json_decode(text)
    if type(text) ~= "string" then
        return nil, "not json"
    end
    local s = text
    local n = #s
    local i = 1
    local depth = 0

    local function skip()
        local _, e = s:sub(i):find("^%s*")
        if e and e > 0 then
            i = i + e
        end
    end

    local parse_value

    local function parse_string()
        i = i + 1
        local parts = {}
        while i <= n do
            local c = s:sub(i, i)
            if c == '"' then
                i = i + 1
                return table.concat(parts), nil
            end
            if c == "\\" then
                local esc = s:sub(i + 1, i + 1)
                if esc == '"' or esc == "\\" or esc == "/" then
                    parts[#parts + 1] = esc
                    i = i + 2
                elseif esc == "b" then
                    parts[#parts + 1] = "\b"
                    i = i + 2
                elseif esc == "f" then
                    parts[#parts + 1] = "\f"
                    i = i + 2
                elseif esc == "n" then
                    parts[#parts + 1] = "\n"
                    i = i + 2
                elseif esc == "r" then
                    parts[#parts + 1] = "\r"
                    i = i + 2
                elseif esc == "t" then
                    parts[#parts + 1] = "\t"
                    i = i + 2
                elseif esc == "u" then
                    local hex = s:sub(i + 2, i + 5)
                    if not hex:match("^[0-9a-fA-F][0-9a-fA-F][0-9a-fA-F][0-9a-fA-F]$") then
                        return nil, "bad unicode escape"
                    end
                    local cp = tonumber(hex, 16)
                    i = i + 6
                    if cp >= 0xD800 and cp <= 0xDBFF then
                        if s:sub(i, i + 1) ~= "\\u" then
                            return nil, "bad unicode escape"
                        end
                        local hex2 = s:sub(i + 2, i + 5)
                        if not hex2:match("^[0-9a-fA-F][0-9a-fA-F][0-9a-fA-F][0-9a-fA-F]$") then
                            return nil, "bad unicode escape"
                        end
                        local low = tonumber(hex2, 16)
                        if low < 0xDC00 or low > 0xDFFF then
                            return nil, "bad unicode escape"
                        end
                        cp = 0x10000 + (cp - 0xD800) * 0x400 + (low - 0xDC00)
                        i = i + 6
                    elseif cp >= 0xDC00 and cp <= 0xDFFF then
                        return nil, "bad unicode escape"
                    end
                    local okc, ch = pcall(utf8.char, cp)
                    if not okc then
                        return nil, "bad unicode escape"
                    end
                    parts[#parts + 1] = ch
                else
                    return nil, "bad escape"
                end
            else
                if c:byte() < 32 then
                    return nil, "bad string"
                end
                parts[#parts + 1] = c
                i = i + 1
            end
        end
        return nil, "unterminated string"
    end

    local function parse_number()
        local start = i
        if s:sub(i, i) == "-" then
            i = i + 1
        end
        local d = s:sub(i, i)
        if d < "0" or d > "9" then
            return nil, "bad number"
        end
        if d == "0" then
            i = i + 1
        else
            while true do
                local c = s:sub(i, i)
                if c < "0" or c > "9" then break end
                i = i + 1
            end
        end
        if s:sub(i, i) == "." then
            i = i + 1
            local frac = i
            while true do
                local c = s:sub(i, i)
                if c < "0" or c > "9" then break end
                i = i + 1
            end
            if i == frac then
                return nil, "bad number"
            end
        end
        local exp = s:sub(i, i)
        if exp == "e" or exp == "E" then
            i = i + 1
            local sign = s:sub(i, i)
            if sign == "+" or sign == "-" then
                i = i + 1
            end
            local exp_at = i
            while true do
                local c = s:sub(i, i)
                if c < "0" or c > "9" then break end
                i = i + 1
            end
            if i == exp_at then
                return nil, "bad number"
            end
        end
        local num = tonumber(s:sub(start, i - 1))
        if num == nil then
            return nil, "bad number"
        end
        return num, nil
    end

    local function parse_object()
        depth = depth + 1
        if depth > 64 then
            return nil, "too deep"
        end
        i = i + 1
        local obj = {}
        skip()
        if s:sub(i, i) == "}" then
            i = i + 1
            depth = depth - 1
            return obj, nil
        end
        while true do
            skip()
            if s:sub(i, i) ~= '"' then
                return nil, "bad object"
            end
            local key, kerr = parse_string()
            if kerr then
                return nil, kerr
            end
            skip()
            if s:sub(i, i) ~= ":" then
                return nil, "bad object"
            end
            i = i + 1
            local val, verr = parse_value()
            if verr then
                return nil, verr
            end
            obj[key] = val
            skip()
            local c = s:sub(i, i)
            if c == "}" then
                i = i + 1
                depth = depth - 1
                return obj, nil
            end
            if c ~= "," then
                return nil, "bad object"
            end
            i = i + 1
        end
    end

    local function parse_array()
        depth = depth + 1
        if depth > 64 then
            return nil, "too deep"
        end
        i = i + 1
        local arr = {}
        local idx = 1
        skip()
        if s:sub(i, i) == "]" then
            i = i + 1
            depth = depth - 1
            return arr, nil
        end
        while true do
            local val, verr = parse_value()
            if verr then
                return nil, verr
            end
            arr[idx] = val
            idx = idx + 1
            skip()
            local c = s:sub(i, i)
            if c == "]" then
                i = i + 1
                depth = depth - 1
                return arr, nil
            end
            if c ~= "," then
                return nil, "bad array"
            end
            i = i + 1
        end
    end

    parse_value = function()
        skip()
        local c = s:sub(i, i)
        if c == '"' then
            return parse_string()
        end
        if c == "{" then
            return parse_object()
        end
        if c == "[" then
            return parse_array()
        end
        if c == "t" then
            if s:sub(i, i + 3) ~= "true" then
                return nil, "bad value"
            end
            i = i + 4
            return true, nil
        end
        if c == "f" then
            if s:sub(i, i + 4) ~= "false" then
                return nil, "bad value"
            end
            i = i + 5
            return false, nil
        end
        if c == "n" then
            if s:sub(i, i + 3) ~= "null" then
                return nil, "bad value"
            end
            i = i + 4
            return nil, nil
        end
        if c == "-" or (c >= "0" and c <= "9") then
            return parse_number()
        end
        return nil, "bad value"
    end

    local val, verr = parse_value()
    if verr then
        return nil, verr
    end
    skip()
    if i <= n then
        return nil, "trailing data"
    end
    return val, nil
end
-- phase27:json-end

local function integer_id(value)
    local n = tonumber(value)
    if n == nil or n ~= n or n == math.huge or n == -math.huge then
        return nil
    end
    if n ~= math.floor(n) or n < 1 then
        return nil
    end
    return math.floor(n)
end

local function row_field(row, name)
    if type(row) ~= "table" then
        return nil
    end
    if row[name] ~= nil then
        return row[name]
    end
    local upper = name:upper()
    if row[upper] ~= nil then
        return row[upper]
    end
    for key, value in pairs(row) do
        if type(key) == "string" and key:upper() == upper then
            return value
        end
    end
    return nil
end

local function shape_collection(value)
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

local function load_shape_rows()
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


local function first_datasource_name(def)
    local list = def.datasources
    if type(list) ~= "table" then
        return nil
    end
    for _, entry in ipairs(list) do
        if type(entry) == "string" and entry ~= "" then
            return entry
        end
        if type(entry) == "table" and type(entry.name) == "string" and entry.name ~= "" then
            return entry.name
        end
    end
    return nil
end

-- Detail, group header, and group footer with no datasource use the first one.
local function fill_band_datasources(def)
    if type(def) ~= "table" or type(def.bands) ~= "table" then
        return def
    end
    local name = first_datasource_name(def)
    if not name then
        return def
    end
    for _, band in ipairs(def.bands) do
        if type(band) == "table" then
            local key = band.typeKey
            if (key == 14 or key == 15 or key == 16)
                and (band.datasource == nil or band.datasource == "") then
                band.datasource = name
            end
        end
    end
    return def
end

local function report_where(id, rev)
    return "report " .. tostring(id) .. " revision " .. tostring(rev)
end

local function resolve_definition()
    if type(P.definition) == "table" then
        return P.definition, nil
    end
    local id = integer_id(P.report_id)
    local rev = integer_id(P.rev_id)
    if not id or not rev then
        return nil, nil
    end
    local hy = type(P._hydrogen) == "table" and P._hydrogen or {}
    local database = hy.database
    if type(database) ~= "string" or database == "" then
        return nil, "no session database; cannot load " .. report_where(id, rev)
    end
    if type(H) ~= "table" or type(H.altquery_sync) ~= "function" then
        return nil, "could not read QueryRef 167: H.altquery_sync is missing"
    end
    local ok, res, err = pcall(H.altquery_sync, database, [[
        SELECT code FROM ${SCHEMA}queries
        WHERE query_ref = 167 AND query_type_a28 = 1
    ]], {})
    if not ok then
        return nil, "could not read QueryRef 167: " .. tostring(res)
    end
    if err then
        return nil, "could not read QueryRef 167: " .. tostring(err)
    end
    local code = row_field(type(res) == "table" and type(res.rows) == "table" and res.rows[1] or nil, "code")
    if type(code) ~= "string" or code == "" then
        return nil, "could not read QueryRef 167: query is not installed"
    end
    local rok, rres, rerr = pcall(H.altquery_sync, database, code, {
        REPORT_ID = id,
        REV_ID = rev,
    })
    if not rok then
        return nil, "could not read " .. report_where(id, rev) .. ": " .. tostring(rres)
    end
    if rerr then
        return nil, "could not read " .. report_where(id, rev) .. ": " .. tostring(rerr)
    end
    local row = type(rres) == "table" and type(rres.rows) == "table" and rres.rows[1] or nil
    if type(row) ~= "table" then
        return nil, report_where(id, rev) .. " was not found"
    end
    local cell = row_field(row, "report")
    if cell == nil or cell == "" then
        return nil, report_where(id, rev) .. " has no definition"
    end
    local decoded
    if type(cell) == "table" then
        decoded = cell
    elseif type(cell) == "string" then
        if cell:match("^%s*$") then
            return nil, report_where(id, rev) .. " has no definition"
        end
        local parsed, jerr = json_decode(cell)
        if jerr then
            return nil, "definition is not valid JSON"
        end
        decoded = parsed
    else
        return nil, report_where(id, rev) .. " has no definition"
    end
    if type(decoded) ~= "table" then
        return nil, report_where(id, rev) .. " has no definition"
    end
    return fill_band_datasources(decoded), nil
end

local function compose()
    load_shape_rows()
    local stage = P.stage or "compose"
    if stage ~= "compose" and stage ~= "html" then
        return fail("stage " .. tostring(stage) .. " is not available yet; use compose or html")
    end
    local def, def_err = resolve_definition()
    if def_err then
        return fail(def_err)
    end
    if type(def) ~= "table" then
        return fail("definition must be a JSON object")
    end
    if def.format ~= 1 then
        return fail("definition format " .. tostring(def.format) .. " is not supported; expected 1")
    end
    local sources = type(P.sources) == "table" and P.sources or {}
    local rfont = type(def.font) == "table" and def.font or {}

    -- Phase 22: one generation instant for every item; timezone User | UTC | Explicit.
    local HY = type(P._hydrogen) == "table" and P._hydrogen or {}
    local def_zone = type(def.defaultZone) == "string" and def.defaultZone ~= "" and def.defaultZone or nil
    local gen_now
    local function now()
        if gen_now == nil then
            local n = tonumber(P.now)
            if n == nil and H and H.system and H.system.now then n = tonumber(H.system.now()) end
            gen_now = math.floor(n or os.time())
        end
        return gen_now
    end
    local zone_logged = {}
    local function zone_log(level, message)
        if zone_logged[message] then return end
        zone_logged[message] = true
        add_log(level, message)
    end
    -- Phase 22a: IANA zones resolve through luatz (group luatz) on host tzdata.
    -- Config hooks (env, defaults hardcoded): REPORTS_TZ_ZONEINFO_PATH (reports.tz.zoneinfo_path),
    -- REPORTS_TZ_DEFAULT_ZONE (reports.tz.default_zone, used when the report has no defaultZone).
    local function cfg_env(name)
        local ok, v = pcall(function() return os.getenv and os.getenv(name) end)
        return ok and type(v) == "string" and v ~= "" and v or nil
    end
    local TZ_PATH = cfg_env("REPORTS_TZ_ZONEINFO_PATH") or "/usr/share/zoneinfo"
    def_zone = def_zone or cfg_env("REPORTS_TZ_DEFAULT_ZONE")
    local tzc
    local function iana_offset(zone, instant)
        if not zone:match("^[%w_+%-/]+$") or zone:find("..", 1, true) or zone:sub(1, 1) == "/" then
            zone_log("warn", "zone " .. zone .. " is not an IANA name; printed UTC")
            return nil
        end
        if tzc == nil then
            local ok, m = pcall(require, "luatz.tzcache")
            tzc = ok and type(m) == "table" and m or false
            if not tzc then
                zone_log("warn", "luatz is not available (" .. tostring(m) .. "); printed UTC")
            elseif tzc.set_base_path then
                tzc.set_base_path(TZ_PATH)
            end
        end
        if not tzc then return nil end
        if io == nil then
            zone_log("warn", "zone " .. zone .. " needs read access to " .. TZ_PATH .. " (io is sandboxed); printed UTC")
            return nil
        end
        local ok, tz = pcall(tzc.get_tz, zone)
        if not ok or type(tz) ~= "table" then
            zone_log("warn", "zone " .. zone .. " is unknown or unreadable under " .. TZ_PATH .. "; printed UTC")
            return nil
        end
        local ok2, info = pcall(tz.find_current, tz, instant)
        if not ok2 or type(info) ~= "table" then
            zone_log("warn", "zone " .. zone .. " has no offset at " .. tostring(instant) .. "; printed UTC")
            return nil
        end
        return info.gmtoff
    end
    local function zone_offset(item, instant)
        local mode = item.timezone or "User"
        if mode == "UTC" then return 0 end
        local zone
        if mode == "Explicit" then
            zone = type(item.zone) == "string" and item.zone ~= "" and item.zone or nil
            if not zone then
                zone_log("info", "item " .. tostring(item.id) .. " has Explicit timezone with no zone; printed UTC")
                return 0
            end
        else
            -- The caller's own claim keeps its login offset (tzoffset drift accepted).
            local off = tonumber(HY.tzoffset)
            if type(HY.tz) == "string" and HY.tz ~= "" and off then return math.floor(off * 60) end
            zone = type(HY.tz) == "string" and HY.tz ~= "" and HY.tz or def_zone
            if not zone then
                zone_log("info", "no user timezone or report default zone; printed UTC")
                return 0
            end
        end
        return iana_offset(zone, instant) or 0
    end
    local DT_DEFAULT = "yyyy-MM-dd HH:mm"
    local warned_user = false

    local declared = {}
    for _, ds in ipairs(def.datasources or {}) do
        if type(ds) == "table" and ds.name then declared[ds.name] = ds end
    end

    -- Sort bands into roles, keeping designer order inside each role.
    local rh, ph, gh, details, gf, pf, rf = {}, {}, {}, {}, {}, {}, {}
    local roles = { [12] = rh, [13] = ph, [14] = gh, [15] = details, [16] = gf, [17] = pf, [18] = rf }
    for _, b in ipairs(def.bands or {}) do
        local list = type(b) == "table" and roles[b.typeKey]
        if list then
            list[#list + 1] = b
        else
            add_log("warn", "band " .. tostring(type(b) == "table" and b.id) .. " has unknown typeKey "
                .. tostring(type(b) == "table" and b.typeKey) .. "; skipped")
        end
    end

    -- Pair each group header with its footer: same suffix (gh2/gf2), else nesting order.
    local used = {}
    local function suffix(id) return tostring(id or ""):match("^g[hf](.*)$") end
    local levels = {}
    for i, h in ipairs(gh) do
        local foot
        local hs = suffix(h.id)
        if hs then
            for j, f in ipairs(gf) do
                if not used[j] and suffix(f.id) == hs then foot = j break end
            end
        end
        if not foot then
            local j = #gf - i + 1
            if gf[j] and not used[j] then foot = j end
        end
        if foot then used[foot] = true end
        if h.field == nil or h.field == "" then
            add_log("warn", "group " .. tostring(h.id) .. " has no field; it wraps every row once")
        end
        levels[i] = { header = h, footer = foot and gf[foot] or nil }
    end
    for j, f in ipairs(gf) do
        if not used[j] then
            add_log("warn", "group footer " .. tostring(f.id) .. " has no group header; skipped")
        end
    end

    -- Rows: the first detail band with a datasource drives the report.
    local rows, driver
    local function reporting_types(collection)
        local doc = collection
        if type(doc) == "string" then
            if doc == "" then return {} end
            local parsed, jerr = json_decode(doc)
            if jerr or type(parsed) ~= "table" then return {} end
            doc = parsed
        end
        if type(doc) ~= "table" then return {} end
        if doc.reporting == nil and type(doc[1]) == "table" then
            doc = doc[1]
        end
        local rep = type(doc) == "table" and doc.reporting or nil
        if type(rep) ~= "table" or type(rep.parameters) ~= "table" then
            return {}
        end
        local types = {}
        for _, p in ipairs(rep.parameters) do
            if type(p) == "table" and type(p.name) == "string" and p.kind ~= "calculated" then
                types[p.name] = tostring(p.type or "string"):lower()
            end
        end
        return types
    end

    local function definition_defaults()
        local map = {}
        local list = def.parameters
        if type(list) ~= "table" then return map end
        for _, p in ipairs(list) do
            if type(p) == "table" and type(p.name) == "string" then
                map[p.name] = p.default
            end
        end
        return map
    end

    local function coerce_bind(name, value, types)
        if value == nil then return nil end
        local typ = types[name]
        if typ == nil then return value end
        if typ == "integer" or typ == "int" then
            local n = tonumber(value)
            if n == nil or n ~= n or n == math.huge or n == -math.huge then return value end
            return math.floor(n)
        end
        if typ == "float" or typ == "number" then
            local n = tonumber(value)
            if n == nil or n ~= n or n == math.huge or n == -math.huge then return value end
            return n
        end
        if typ == "boolean" or typ == "bool" then
            return value == true or value == 1 or value == "1" or value == "true"
        end
        if typ == "string" or typ == "text" or typ == "memo" or typ == "date" or typ == "datetime" then
            return plain(value)
        end
        return value
    end

    local function resolve_raw(raw, overrides, defaults)
        if type(raw) ~= "table" then return raw end
        if raw.parameter ~= nil then
            local pname = tostring(raw.parameter)
            if overrides[pname] ~= nil then return overrides[pname] end
            if defaults[pname] ~= nil then return defaults[pname] end
            return nil
        end
        if raw.literal ~= nil then return raw.literal end
        return nil
    end

    local function build_binds(ds, types)
        local overrides = type(P.parameters) == "table" and P.parameters or {}
        local defaults = definition_defaults()
        local stored = type(ds.params) == "table" and ds.params or {}
        local binds = {}
        local missing = false
        for name, raw in pairs(stored) do
            if type(name) == "string" then
                local value
                if overrides[name] ~= nil then
                    value = overrides[name]
                else
                    value = resolve_raw(raw, overrides, defaults)
                end
                value = coerce_bind(name, value, types)
                if value == nil then
                    missing = true
                    add_log("warn", "datasource " .. tostring(ds.name) .. " parameter " .. name
                        .. " is not set; no rows")
                else
                    binds[name] = value
                end
            end
        end
        return binds, missing
    end

    local function session_database()
        local hy = type(P._hydrogen) == "table" and P._hydrogen or {}
        local database = hy.database
        if type(database) ~= "string" or database == "" then return nil end
        return database
    end

    local function unconfigured(message)
        local s = tostring(message or "")
        if s:find("database_name is empty", 1, true) then return true end
        if s:find("no database specified", 1, true) then return true end
        if s:find("not found", 1, true) and s:find("database", 1, true) then return true end
        return false
    end

    local function query_rows(ds, name)
        local ref = tonumber(ds.queryRef)
        if ref == nil or ref ~= math.floor(ref) or ref < 1 then
            add_log("warn", "datasource " .. tostring(name) .. " has no queryRef; no rows")
            return {}
        end
        ref = math.floor(ref)
        local database = session_database()
        if not database then
            add_log("warn", "datasource " .. tostring(name) .. " has no session database; no rows")
            return {}
        end
        if type(H) ~= "table" or type(H.altquery_sync) ~= "function" then
            add_log("warn", "datasource " .. tostring(name) .. " query " .. tostring(ref)
                .. " failed: H.altquery_sync is missing; no rows")
            return {}
        end
        -- Type 3 is SQL - Reporting. Type 1 does not find QueryRef 174.
        local ok, res, err = pcall(H.altquery_sync, database, [[
            SELECT code, collection FROM ${SCHEMA}queries
            WHERE query_ref = :QREF AND query_type_a28 = 3
        ]], { QREF = ref })
        if not ok then
            err = res
            res = nil
        end
        if err then
            if unconfigured(err) then
                add_log("warn", "datasource " .. tostring(name) .. " database " .. database
                    .. " is not configured; no rows")
            else
                add_log("warn", "datasource " .. tostring(name) .. " query " .. tostring(ref)
                    .. " failed: " .. tostring(err) .. "; no rows")
            end
            return {}
        end
        local row = type(res) == "table" and type(res.rows) == "table" and res.rows[1] or nil
        local code = row_field(row, "code")
        if type(code) ~= "string" or code == "" then
            add_log("warn", "datasource " .. tostring(name) .. " query " .. tostring(ref)
                .. " is not installed; no rows")
            return {}
        end
        local binds, missing = build_binds(ds, reporting_types(row_field(row, "collection")))
        if missing then return {} end
        local rok, rres, rerr = pcall(H.altquery_sync, database, code, binds)
        if not rok then
            rerr = rres
            rres = nil
        end
        if rerr then
            if unconfigured(rerr) then
                add_log("warn", "datasource " .. tostring(name) .. " database " .. database
                    .. " is not configured; no rows")
            else
                add_log("warn", "datasource " .. tostring(name) .. " query " .. tostring(ref)
                    .. " failed: " .. tostring(rerr) .. "; no rows")
            end
            return {}
        end
        local got = type(rres) == "table" and type(rres.rows) == "table" and rres.rows or {}
        add_log("info", "datasource " .. tostring(name) .. " ran query " .. tostring(ref)
            .. "; " .. tostring(#got) .. " rows")
        return got
    end

    local function rows_for(name, band_id)
        if not declared[name] then
            add_log("warn", "detail " .. tostring(band_id) .. " names datasource " .. tostring(name)
                .. " which is not declared; no rows")
            return {}
        end
        local r = sources[name]
        if type(r) == "table" then
            return r
        end
        local ds = declared[name]
        if type(ds) == "table" and ds.kind == "query" then
            return query_rows(ds, name)
        end
        add_log("warn", "datasource " .. tostring(name) .. " has no array in sources; no rows")
        return {}
    end
    for _, d in ipairs(details) do
        if d.datasource ~= nil and d.datasource ~= "" then
            if not driver then
                driver = d.datasource
                rows = rows_for(driver, d.id)
            elseif d.datasource ~= driver then
                add_log("warn", "detail " .. tostring(d.id) .. " uses datasource " .. tostring(d.datasource)
                    .. "; rows come from " .. driver .. " in this version")
            end
        end
    end
    if not driver then
        rows = { {} } -- static report: each band once
    end

    -- Parameter items (63): "NAME" (first datasource with it) or "ds.NAME".
    local warned_param = {}
    local function param_value(ref)
        if ref == nil or ref == "" then return nil end
        ref = tostring(ref)
        local ds_name, pname = ref:match("^([^%.]+)%.(.+)$")
        for _, ds in ipairs(type(def.datasources) == "table" and def.datasources or {}) do
            if type(ds) == "table" and type(ds.params) == "table"
                and (ds_name == nil or ds.name == ds_name) then
                local v = ds.params[pname or ref]
                if v ~= nil then return v end
            end
        end
        if not warned_param[ref] then
            warned_param[ref] = true
            add_log("warn", "parameter " .. ref .. " is not set on any datasource; printed blank")
        end
        return nil
    end


    -- Phase 38 and 39. ctx.aggregates is the scope being emitted, keyed
    -- aggregates[op][field]. Report and group scopes are filled before
    -- paging. Page totals are filled after the page list exists, from
    -- the detail rows placed on that page. A detail band has no
    -- aggregates. The map is not copied into the emitted JSON.
    local ctx
    local report_aggs
    local group_aggs
    local AGG_OPS = { count = true, filled = true, unique = true, sum = true,
        avg = true, mean = true, weighted = true, median = true, mode = true,
        min = true, max = true, first = true, last = true }
    local agg_warned = {}
    local function agg_warn(item, band_id, message)
        local k = tostring(band_id) .. "/" .. tostring(item.id) .. "/" .. message
        if agg_warned[k] then return end
        agg_warned[k] = true
        add_log("warn", "item " .. tostring(item.id) .. " in band " .. tostring(band_id) .. " " .. message)
    end
    local function numeric_value(v)
        if type(v) ~= "number" then return nil end
        if v ~= v or v == math.huge or v == -math.huge then return nil end
        return v
    end
    local function unique_key(v)
        if type(v) == "number" then
            local n = numeric_value(v)
            if not n then return nil end
            return "n:" .. plain_number(n)
        end
        return "s:" .. plain(v)
    end
    local function format_aggregate(rawv, item, band_id)
        if type(item) == "table" and item.typeKey == 38
            and (item.op == "min" or item.op == "max"
                or item.op == "first" or item.op == "last") then
            return format_value(rawv, 24, item.mask, item.id, band_id)
        end
        if type(rawv) == "string" then return plain(rawv) end
        return format_value(rawv, 21, item.mask, item.id, band_id)
    end
    local function weight_name(item)
        if type(item) == "table" and item.op == "weighted" and type(item.weight) == "string" then
            return item.weight
        end
        return ""
    end
    local function store_key(item)
        local w = weight_name(item)
        if item.op == "weighted" and w ~= "" then return item.field .. "\0" .. w end
        return item.field
    end
    local function agg_raw(aggs, item)
        if type(aggs) ~= "table" or type(item) ~= "table" then return nil end
        local bucket = aggs[item.op]
        if type(bucket) ~= "table" then return nil end
        if item.op == "weighted" and weight_name(item) ~= "" then
            return bucket[store_key(item)]
        end
        return bucket[item.field]
    end
    local function reduce_numbers(field, list)
        local total, n, skipped = 0, 0, false
        for _, row in ipairs(list) do
            local v = type(row) == "table" and row[field] or nil
            if v ~= nil then
                local num = numeric_value(v)
                if num then
                    total = total + num
                    n = n + 1
                else
                    skipped = true
                end
            end
        end
        return total, n, skipped
    end
    local function mode_less(a, b)
        local ta, tb = type(a), type(b)
        if ta == tb then
            if ta == "number" or ta == "string" then return a < b end
            return false
        end
        if ta == "number" then return true end
        if tb == "number" then return false end
        return ta < tb
    end
    local function reduce_op(op, field, list, weight)
        if op == "count" then return #list, nil end
        if op == "filled" then
            local n = 0
            for _, row in ipairs(list) do
                if type(row) == "table" and row[field] ~= nil then n = n + 1 end
            end
            return n, nil
        end
        if op == "unique" then
            local seen, n = {}, 0
            for _, row in ipairs(list) do
                local v = type(row) == "table" and row[field] or nil
                if v ~= nil then
                    local uk = unique_key(v)
                    if uk and not seen[uk] then
                        seen[uk] = true
                        n = n + 1
                    end
                end
            end
            return n, nil
        end
        if op == "sum" or op == "avg" or op == "mean" then
            local total, n, skipped = reduce_numbers(field, list)
            local flags = skipped and { skip = true } or nil
            if n == 0 then return nil, flags end
            if op == "sum" then return total, flags end
            return total / n, flags
        end
        if op == "weighted" then
            local wname = type(weight) == "string" and weight or ""
            if wname == "" then return nil, { missing = true } end
            local num, den, pairs, skipped = 0, 0, false, false
            for _, row in ipairs(list) do
                if type(row) == "table" then
                    local v, w = row[field], row[wname]
                    if v ~= nil and w ~= nil then
                        local nv, nw = numeric_value(v), numeric_value(w)
                        if nv and nw then
                            num = num + nv * nw
                            den = den + nw
                            pairs = true
                        else
                            skipped = true
                        end
                    elseif v ~= nil and not numeric_value(v) then
                        skipped = true
                    end
                end
            end
            local flags = nil
            if skipped then flags = { skip = true } end
            if not pairs then
                flags = flags or {}
                flags.none = true
                return nil, flags
            end
            if den == 0 then
                flags = flags or {}
                flags.zero = true
                return nil, flags
            end
            return num / den, flags
        end
        if op == "median" then
            local nums, skipped = {}, false
            for _, row in ipairs(list) do
                local v = type(row) == "table" and row[field] or nil
                if v ~= nil then
                    local num = numeric_value(v)
                    if num then nums[#nums + 1] = num else skipped = true end
                end
            end
            local flags = skipped and { skip = true } or nil
            if #nums == 0 then return nil, flags end
            table.sort(nums)
            local count = #nums
            local mid = math.floor(count / 2)
            if count % 2 == 1 then return nums[mid + 1], flags end
            return (nums[mid] + nums[mid + 1]) / 2, flags
        end
        if op == "mode" then
            local counts, values, order, bad = {}, {}, {}, false
            for _, row in ipairs(list) do
                local v = type(row) == "table" and row[field] or nil
                if v ~= nil then
                    local key, stored
                    if type(v) == "number" then
                        local num = numeric_value(v)
                        if num then
                            key = "n:" .. plain_number(num)
                            stored = num
                        else
                            bad = true
                        end
                    elseif type(v) == "string" then
                        key = "s:" .. v
                        stored = v
                    else
                        bad = true
                    end
                    if key then
                        if not counts[key] then
                            counts[key] = 0
                            values[key] = stored
                            order[#order + 1] = key
                        end
                        counts[key] = counts[key] + 1
                    end
                end
            end
            local flags = bad and { bad = true } or nil
            if #order == 0 then return nil, flags end
            local best, keys = 0, {}
            for _, key in ipairs(order) do
                local n = counts[key]
                if n > best then
                    best = n
                    keys = { key }
                elseif n == best then
                    keys[#keys + 1] = key
                end
            end
            local winner = keys[1]
            for i = 2, #keys do
                if mode_less(values[keys[i]], values[winner]) then winner = keys[i] end
            end
            if #keys > 1 then
                flags = flags or {}
                flags.tie = true
            end
            return values[winner], flags
        end
        if op == "min" or op == "max" then
            local kind, best, other = nil, nil, false
            for _, row in ipairs(list) do
                local v = type(row) == "table" and row[field] or nil
                if v ~= nil then
                    local stored, k
                    if type(v) == "number" then
                        stored = numeric_value(v)
                        k = stored and "number" or nil
                    elseif type(v) == "string" then
                        stored, k = v, "string"
                    end
                    if not k then
                        other = true
                    elseif not kind then
                        kind, best = k, stored
                    elseif k ~= kind then
                        other = true
                    elseif (op == "min" and stored < best) or (op == "max" and best < stored) then
                        best = stored
                    end
                end
            end
            local flags = other and { other = true } or nil
            return best, flags
        end
        if op == "first" or op == "last" then
            local found = nil
            for _, row in ipairs(list) do
                local v = type(row) == "table" and row[field] or nil
                if v ~= nil then
                    if op == "first" then
                        if found == nil then found = v end
                    else
                        found = v
                    end
                end
            end
            return found, nil
        end
        return nil, nil
    end
    local function collect_aggs(bands)
        local specs = {}
        for _, band in ipairs(bands) do
            if type(band) == "table" then
                for _, item in ipairs(band.items or {}) do
                    if type(item) == "table" and (item.typeKey == 35 or item.typeKey == 34 or item.typeKey == 38)
                        and (type(item["function"]) ~= "string" or item["function"] == "")
                        and type(item.field) == "string" and item.field ~= ""
                        and AGG_OPS[item.op] then
                        specs[#specs + 1] = { item = item, band_id = band.id }
                    end
                end
            end
        end
        return specs
    end
    local function note_flags(item, band_id, flags)
        if type(flags) ~= "table" then return end
        if flags.skip then
            agg_warn(item, band_id, item.op .. " skipped a value that is not a number")
        end
        if flags.missing then
            agg_warn(item, band_id, "has no weight; printed blank")
        end
        if flags.none then
            agg_warn(item, band_id, "weighted has no numeric pair; printed blank")
        end
        if flags.zero then
            agg_warn(item, band_id, "weighted weights sum to zero; printed blank")
        end
        if flags.tie then
            agg_warn(item, band_id, "mode tied; printed the smallest value")
        end
        if flags.bad then
            agg_warn(item, band_id, "mode skipped a value that is not text or a number")
        end
        if flags.other then
            agg_warn(item, band_id, item.op .. " skipped a value of the other kind")
        end
    end
    local function fill_aggs(specs, list)
        local aggs, have, value, flags = {}, {}, {}, {}
        for _, spec in ipairs(specs) do
            local item = spec.item
            local op, field = item.op, item.field
            local id = op .. "\0" .. field .. "\0" .. weight_name(item)
            aggs[op] = aggs[op] or {}
            if not have[id] then
                have[id] = true
                value[id], flags[id] = reduce_op(op, field, list, item.weight)
            end
            aggs[op][store_key(item)] = value[id]
            if op == "weighted" then aggs[op][field] = value[id] end
            note_flags(item, spec.band_id, flags[id])
        end
        return aggs
    end

    local warned_field = {}
    local function item_out(item, row, band_id, band_tk)
        local tk = item.typeKey
        local text
        if tk == 0 then
            text = item.text ~= nil and plain(item.text) or ""
        elseif FIELD_KEYS[tk] then
            if item.field == nil or item.field == "" then
                local k = tostring(band_id) .. "/" .. tostring(item.id)
                if not warned_field[k] then
                    warned_field[k] = true
                    add_log("warn", "item " .. tostring(item.id) .. " in band " .. tostring(band_id)
                        .. " has no field; printed blank")
                end
                text = ""
            else
                local v = row and row[item.field]
                -- Unix-number datetimes are instants and take the item timezone when set;
                -- civil strings never shift.
                if tk == 24 and item.timezone and type(v) == "number" then v = v + zone_offset(item, v) end
                text = format_value(v, tk, item.mask, item.id, band_id)
            end
        elseif tk == 5 then
            local s = now()
            local m = item.mask
            if m == nil or m == "" then m = DT_DEFAULT end
            text = format_value(s + zone_offset(item, s), 24, m, item.id, band_id)
        elseif tk == 6 then
            local u = HY.username
            if u == nil or u == "" then
                if not warned_user then
                    warned_user = true
                    add_log("warn", "item " .. tostring(item.id) .. " in band " .. tostring(band_id)
                        .. " is System user but the caller has no username; printed blank")
                end
                text = ""
            else
                text = plain(u)
            end
        elseif PAGE_KEYS[tk] then
            text = "" -- filled after the page list exists
        elseif tk == 63 then
            local v = param_value(item.param)
            local ptk = type(v) == "number" and 21 or 20
            text = format_value(v, ptk, item.mask, item.id, band_id)
        elseif tk == 34 or tk == 35 or tk == 38 then
            local fname = item["function"]
            if type(fname) == "string" and fname ~= "" then
                agg_warn(item, band_id, "names function " .. fname
                    .. "; function totals are not available yet; printed blank")
                text = ""
            elseif item.field == nil or item.field == "" then
                agg_warn(item, band_id, "has no field; printed blank")
                text = ""
            elseif not AGG_OPS[item.op] then
                agg_warn(item, band_id, "has op " .. tostring(item.op)
                    .. " which this version does not reduce; printed blank")
                text = ""
            elseif band_tk == 13 or band_tk == 17 then
                text = "" -- filled after the page list exists
            elseif type(ctx) ~= "table" or type(ctx.aggregates) ~= "table" then
                agg_warn(item, band_id, "is not on a group, page, or report header or footer; printed blank")
                text = ""
            else
                text = format_aggregate(agg_raw(ctx.aggregates, item), item, band_id)
            end
        elseif tk == 1 then
            local v = item.value
            if v == nil then
                text = ""
            elseif type(v) ~= "number" or v ~= v or v == math.huge or v == -math.huge then
                format_warn(item.id, band_id, "value " .. raw(v) .. " is not a number; printed blank")
                text = ""
            else
                text = format_value(v, 21, item.mask, item.id, band_id)
            end
        elseif tk == 4 then
            local v = item.value
            if v == nil then
                text = ""
            else
                text = format_value(v, 24, item.mask, item.id, band_id)
            end
        elseif tk == 3 then
            text = item.text ~= nil and plain(item.text) or ""
        elseif tk == 2 then
            text = ""
        elseif tk == 25 then
            text = ""
        elseif tk == 43 or tk == 45 then
            text = ""
        elseif LATER_KEYS[tk] then
            text = ""
        else
            add_log("warn", "item " .. tostring(item.id) .. " in band " .. tostring(band_id)
                .. " has unknown typeKey " .. tostring(tk) .. "; skipped")
            return nil
        end
        local f = type(item.font) == "table" and item.font or {}
        local o = {
            id = item.id,
            typeKey = tk,
            text = text,
            align = item.align or "Left",
            font = {
                family = f.family or rfont.family,
                size = f.size or rfont.size,
                style = f.style or rfont.style,
            },
            box = item.box,
        }
        if PAGE_KEYS[tk] then page_src[o] = item end
        if tk == 2 then
            o.image = item.image
            o.dpi = item.dpi
            o.fit = item.fit
            o.fitAlign = item.fitAlign
            o.fitValign = item.fitValign
            o.flip = item.flip
            o.opacity = item.opacity
            o.grayscale = item.grayscale
            o.invert = item.invert
            o.rotation = item.rotation
        end
        if tk == 25 then
            o.kind = item.kind
            o.side = item.side
            o.corners = item.corners
            o.strokeWidth = item.strokeWidth
            o.strokeStyle = item.strokeStyle
            o.strokeColor = item.strokeColor
            o.fillStyle = item.fillStyle
            o.fillPaint = item.fillPaint
            o.rotation = item.rotation
        end
        if tk == 43 or tk == 45 then
            local bc = type(item.barcode) == "table" and item.barcode or {}
            local copy = {}
            for k, v in pairs(bc) do copy[k] = v end
            if tk == 45 then
                if item.field == nil or item.field == "" then
                    local fk = tostring(band_id) .. "/" .. tostring(item.id)
                    if not warned_field[fk] then
                        warned_field[fk] = true
                        add_log("warn", "item " .. tostring(item.id) .. " in band " .. tostring(band_id)
                            .. " has no field; printed blank")
                    end
                    copy.data = ""
                else
                    copy.data = plain(row and row[item.field])
                end
            end
            o.barcode = copy
            o.dpi = item.dpi
            o.fitAlign = item.fitAlign
            o.fitValign = item.fitValign
            o.rotation = item.rotation
        end
        return o
    end

    -- Paging (Phase 21): stored band heights against the page content box.
    local UNIT_PT = { ["in"] = 72, mm = 72 / 25.4, cm = 72 / 2.54, pt = 1, px = 0.75 }
    local function to_pt(len)
        if type(len) == "number" then return len end
        if type(len) ~= "string" then return nil end
        local n, u = len:match("^%s*([%d%.]+)%s*(%a+)%s*$")
        n = tonumber(n)
        if not n or not UNIT_PT[u] then return nil end
        return n * UNIT_PT[u]
    end
    local pg = type(def.page) == "table" and def.page or {}
    local mg = type(pg.margin) == "table" and pg.margin or {}
    local box_h = to_pt(pg.height)
    if box_h then box_h = box_h - (to_pt(mg.top) or 0) - (to_pt(mg.bottom) or 0) end
    if not box_h or box_h <= 0 then
        add_log("warn", "page height " .. tostring(pg.height) .. " leaves no content box; no page breaks")
        box_h = math.huge
    end
    local warned_height = {}
    local function stored_h(band)
        local h = to_pt(band.height)
        if h then return h end
        if not warned_height[band] then
            warned_height[band] = true
            add_log("warn", "band " .. tostring(band.id) .. " height " .. tostring(band.height)
                .. " does not parse; counted as 0")
        end
        return 0
    end
    local layout_cache = {}
    local function measure_band(band, row, aggs)
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
    local function h_of(band, row, aggs)
        local measured = measure_band(band, row, aggs)
        if measured.reflow then return measured.usedPt end
        return stored_h(band)
    end
    local EPS = 1e-6

    page_detail = {}
    page_agg_src = {}
    local pages, out, filled, body_start = {}, nil, 0, 0
    local function note_detail(row)
        local list = page_detail[#pages]
        if not list or row == nil or list[#list] == row then return end
        list[#list + 1] = row
    end
    local function clone_out(o)
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
    local function emit(band, row, aggs)
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
                if lay.grown then copy.font.family = "Helvetica" end
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
    local function emit_all(list, row, aggs)
        for _, b in ipairs(list) do emit(b, row, aggs) end
    end
    local function sum_at(list, row, aggs)
        local s = 0
        for _, b in ipairs(list) do s = s + h_of(b, row, aggs) end
        return s
    end

    local first = rows[1]
    local function open_page(row)
        out, filled = {}, 0
        pages[#pages + 1] = { bands = out }
        page_detail[#pages] = {}
        if #pages == 1 then emit_all(rh, first, report_aggs) end
        emit_all(ph, row, nil)
        body_start = filled
    end
    -- Make room for `need` before placing; break the page if the body already has bands.
    local function ensure(need, row, last, what)
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


    local function key(row, lv)
        local f = levels[lv].header.field
        if f == nil or f == "" or type(row) ~= "table" then return "" end
        return plain(row[f])
    end
    local function row_path(row, lv)
        local parts = {}
        for i = 1, lv do parts[#parts + 1] = key(row, i) end
        return table.concat(parts, "\0")
    end
    local span_lists = {}
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
    local span_at = {}
    group_aggs = function(lv, row)
        local path = row_path(row, lv)
        local spans = span_lists[lv] or {}
        local i = span_at[lv] or 1
        while spans[i] and spans[i].path ~= path do i = i + 1 end
        if spans[i] then span_at[lv] = i end
        return spans[i] and spans[i].aggs or {}
    end
    local report_specs = collect_aggs(rh)
    for _, spec in ipairs(collect_aggs(rf)) do report_specs[#report_specs + 1] = spec end
    report_aggs = fill_aggs(report_specs, rows)

    open_page(first)

    local prev
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
    local last = prev or first
    local rf_h = sum_at(rf, last, report_aggs)
    if rf_h > 0 then ensure(rf_h, last, last, "report footer") end
    emit_all(pf, last, nil)
    emit_all(rf, last, report_aggs)

    -- Page totals (Phase 39): detail rows placed on that page.
    -- The page header and the page footer share one reduction.
    local page_specs = collect_aggs(ph)
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
    local count = #pages
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

    if stage == "html" then
        H.set_result_json({ html = emit_html(def, pages), log = log })
    else
        H.set_result_json({ pages = pages, log = log })
    end
    return 0
end

local ok, res = pcall(compose)
if not ok then
    return fail("compose failed: " .. tostring(res))
end
return res or 0
]==]

local sql_new = [==[
-- Reports.Render (Reporting Phase 54: barcode fit)
-- typeKey 43 and 45 draw one SVG from require("zint.barcode").
-- The box is a free rectangle. The symbol keeps its aspect.
-- contain meets, cover slices, none and a fitting scale-down use the symbol size.
-- Bar height is zint height. Rotation is CSS on the symbol. PDF is not emitted.
-- Reports.Render (Reporting Phase 44c: static number, datetime, image, and memo)
-- typeKey 1 prints a masked number. A non-number warns once and prints blank.
-- typeKey 4 prints a Luxon datetime. A civil string is not shifted. A unix number is UTC.
-- typeKey 3 is plain text. typeKey 2 places one image from doc.images.
-- fitHeight grows or shrinks a row in line-height steps. The stored band height stays.
-- A grown item is drawn in Helvetica. embedFonts names a family from params.fonts.
-- HTML stores each display PNG once. PDF is not emitted.
-- Reports.Render (Reporting Phase 44b: shape fill and gradient)
-- fillStyle is none, solid, grid, dots, hatched, or noise.
-- fillPaint is solid, linear, or radial. A grid can use a gradient.
-- Hatch runs down to the right. Noise is seeded from the item id.
-- Grid and dots use the report grid spacing. PDF does not draw the shape.
-- Reports.Render (Reporting Phase 44a: static shapes)
-- A static shape (typeKey 25) passes its path through compose.
-- stage html draws an SVG. PDF does not draw the shape.
-- Reports.Render (Reporting Phase 44: first and last)
-- first and last walk row order and skip nulls. first is not min.
-- A null in the middle is skipped. An empty result prints blank.
-- A datetime item is typeKey 38. A text item is typeKey 34.
-- A number item is typeKey 35.
-- Reports.Render (Reporting Phase 43: min and max)
-- min and max compare numbers as numbers and strings as written.
-- The first finite number or string picks the kind. A later value
-- of the other kind warns once and is skipped. Nulls do not warn.
-- An empty result prints blank. A datetime item is typeKey 38.
-- A text item is typeKey 34. A number item is typeKey 35.
-- Reports.Render (Reporting Phase 42: mode)
-- mode returns the most common value. A tie returns the smallest
-- value under Lua < and warns once. Text and numbers compare apart.
-- A string result prints with no mask. A number uses the number mask.
-- A text mode item is typeKey 34. A number mode item is typeKey 35.
-- Reports.Render (Reporting Phase 41: median)
-- median sorts the numeric values. An odd count returns the middle.
-- An even count returns the mean of the two central numbers.
-- An empty median prints blank. A non-numeric value warns and is skipped.
-- Reports.Render (Reporting Phase 40: avg, mean, and weighted)
-- avg and mean are the arithmetic mean of the numeric values.
-- An empty mean prints blank. weighted stores weight as a field name
-- and divides the sum of value times weight by the sum of the weights.
-- A non-numeric value warns and is skipped. A missing weight, a zero
-- sum of weights, or no numeric pair prints blank and warns once.
-- Reports.Render (Reporting Phase 39: page totals)
-- An aggregate number on a page header or page footer reduces the detail
-- rows placed on that page. The page header and the page footer share
-- that total. The same rows serve every op. Group and report totals
-- are unchanged. Page totals are filled after the page list exists.
-- Reports.Render (Reporting Phase 38: count, filled, unique, and sum)
-- An aggregate number (typeKey 35) on a group or report header or footer
-- reduces one field. count is the row count. filled, unique, and sum skip
-- nulls. An empty count, filled, or unique is 0. An empty sum prints blank.
-- A value that is not a number is skipped by sum, with one warn per item.
-- The raw result is ctx.aggregates[op][field]. The item mask formats it.
-- The header and the footer of one scope share that result.
-- A function name warns and prints blank until the formula compiler.
-- params: { stage = "compose"|"html", definition = {...}, sources = { name = [rows] } }
-- A stored report may send report_id and rev_id instead of definition.
-- The script reads QueryRef 167 through H.altquery_sync on
-- params._hydrogen.database and parses the report text.
-- An inline definition is used when it is a table.
-- A query datasource with no array in sources loads queries.code where
-- query_ref matches and query_type_a28 = 3 (SQL - Reporting), then runs
-- that SQL with H.altquery_sync on the session database.
-- An array in sources wins and the script does not query.
-- Binds are datasource.params. A scalar is the value. {parameter} uses
-- params.parameters, then the definition parameter default. {literal}
-- uses that value. params.parameters overrides a same-named bind.
-- Types come from collection.reporting.parameters.
-- An omitted stage runs compose. compose returns { pages, log }.
-- stage "html" returns { html, log }: one <section> per page at the
-- definition page size, box-sizing border-box, padding the four margins.
-- Item boxes use the stored unit with the space removed (0 in -> 0in).
-- Bands: RH(12) PH(13) GH(14) Detail(15) GF(16) PF(17) RF(18), any count.
-- Groups nest in GH order (outermost first). Field values go through the item mask (Value format); null prints blank.
-- Paging: band heights against the page content box (page height less top/bottom margins).
-- A group header moves with its first detail row; headers do not repeat.
-- System datetime (5) prints one generation instant (params.now or H.system.now()) in the item timezone
-- (User | UTC | Explicit; report default zone definition.defaultZone). System user (6) prints _hydrogen.username.
-- IANA zones resolve through luatz on host tzdata (/usr/share/zoneinfo); unknown zone -> UTC + warn.
-- Datetime masks use the Luxon token subset (yyyy MMM MM dd EEE HH mm ss SSS W ...); defaults yyyy-MM-dd / yyyy-MM-dd HH:mm.
-- Page number (61) / count (62) / label (64, template {n} {c}) fill after paging; same on every band of a page.

local P = params or {}
local log = {}

local function add_log(level, message)
    log[#log + 1] = { level = level, message = message }
end

local function fail(message)
    add_log("error", message)
    H.set_result_json({ error = message, log = log })
    return 0
end

local FIELD_KEYS = { [20] = true, [21] = true, [24] = true }
local PAGE_KEYS = { [61] = true, [62] = true, [64] = true }
local PAGE_TEMPLATE_DEFAULT = "Page {n} of {c}"
local page_src = {}
local page_agg_src = {}
local page_detail = {}
local LATER_KEYS = {
    [42] = true, [44] = true,
    [61] = true, [62] = true, [63] = true, [28] = true, [29] = true, [32] = true,
    [34] = true, [38] = true,
}

local function plain(v)
    if v == nil then return "" end
    if math.type(v) == "float" then
        if v == math.floor(v) and math.abs(v) < 1e15 then
            return string.format("%d", math.floor(v))
        end
        local s = string.format("%.15g", v)
        if tonumber(s) ~= v then s = string.format("%.17g", v) end
        return s
    end
    return tostring(v)
end

-- Value format (Phase 20). Masks are locale-free; see REPORTING_PLAN Value format.
local function decimal_string(a)
    -- a >= 0. Shortest round-trip digits, never scientific.
    local s = string.format("%.15g", a)
    if tonumber(s) ~= a then s = string.format("%.17g", a) end
    local mant, exp = s:match("^([%d%.]+)e([-+]%d+)$")
    if not mant then return s end
    local ip, fp = mant:match("^(%d*)%.?(%d*)$")
    local digits, point = ip .. fp, #ip + tonumber(exp)
    if point <= 0 then return "0." .. string.rep("0", -point) .. digits end
    if point >= #digits then return digits .. string.rep("0", point - #digits) end
    return digits:sub(1, point) .. "." .. digits:sub(point + 1)
end

local function plain_number(v)
    if v ~= v or v == math.huge or v == -math.huge then return tostring(v) end
    if math.type(v) == "integer" then return tostring(v) end
    local s = decimal_string(math.abs(v))
    if s:find("%.") then s = s:gsub("0+$", ""):gsub("%.$", "") end
    if v < 0 and s:match("[1-9]") then s = "-" .. s end
    return s
end

local function round_digits(int, frac, places)
    -- Half away from zero on the decimal string (sign handled by caller).
    if #frac <= places then return int, frac .. string.rep("0", places - #frac) end
    local up = frac:byte(places + 1) >= 53
    local all = int .. frac:sub(1, places)
    if up then
        local d, i = {}, #all
        for k = 1, #all do d[k] = all:byte(k) - 48 end
        while i >= 1 do
            if d[i] == 9 then d[i] = 0 i = i - 1 else d[i] = d[i] + 1 break end
        end
        for k = 1, #all do d[k] = string.char(d[k] + 48) end
        all = (i == 0 and "1" or "") .. table.concat(d)
    end
    local n = #all - places
    return all:sub(1, n), all:sub(n + 1)
end

local function number_mask(mask)
    local m, paren = mask, false
    local inner = m:match("^%((.*)%)$")
    if inner then m, paren = inner, true end
    local prefix = ""
    if m:sub(1, 1) == "$" then prefix, m = "$", m:sub(2) end
    local pct = false
    if m:sub(-1) == "%" then pct, m = true, m:sub(1, -2) end
    local ip, fp = m:match("^([#0,]*)%.([#0]*)$")
    if not ip then ip, fp = m:match("^([#0,]*)$"), "" end
    if not ip or (ip .. fp) == "" or not (ip .. fp):find("[#0]") then return nil end
    if fp:find("#0") then return nil end -- zeros must lead the fraction
    local _, minint = ip:gsub("0", "")
    local _, minfrac = fp:gsub("0", "")
    return {
        paren = paren, prefix = prefix, pct = pct, comma = ip:find(",") ~= nil,
        minint = minint, minfrac = minfrac, maxfrac = #fp,
    }
end

local function apply_number(v, spec)
    local x = spec.pct and v * 100 or v
    local neg = x < 0
    local s = decimal_string(math.abs(x))
    local int, frac = s:match("^(%d*)%.?(%d*)$")
    int, frac = round_digits(int, frac, spec.maxfrac)
    int = int:gsub("^0+", "")
    if #int < spec.minint then int = string.rep("0", spec.minint - #int) .. int end
    while #frac > spec.minfrac and frac:sub(-1) == "0" do frac = frac:sub(1, -2) end
    if spec.comma and #int > 3 then
        int = int:reverse():gsub("(%d%d%d)", "%1,"):reverse():gsub("^,", "")
    end
    local body = spec.prefix .. int .. (frac ~= "" and ("." .. frac) or "") .. (spec.pct and "%" or "")
    if not (int .. frac):find("[1-9]") then neg = false end
    if neg then return spec.paren and ("(" .. body .. ")") or ("-" .. body) end
    return body
end

local MONTHS = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" }

local function civil_from_days(z)
    z = z + 719468
    local era = (z >= 0 and z or z - 146096) // 146097
    local doe = z - era * 146097
    local yoe = (doe - doe // 1460 + doe // 36524 - doe // 146096) // 365
    local doy = doe - (365 * yoe + yoe // 4 - yoe // 100)
    local mp = (5 * doy + 2) // 153
    local d = doy - (153 * mp + 2) // 5 + 1
    local m = mp < 10 and mp + 3 or mp - 9
    return yoe + era * 400 + (m <= 2 and 1 or 0), m, d
end

local function parse_datetime(v)
    if type(v) == "number" then
        if v ~= v or v == math.huge or v == -math.huge then return nil end
        local secs = math.floor(v)
        local days, rem = secs // 86400, secs % 86400
        local y, m, d = civil_from_days(days)
        local ms = math.min(999, math.floor((v - secs) * 1000 + 1e-6))
        return { y = y, m = m, d = d, hh = rem // 3600, nn = rem % 3600 // 60, ss = rem % 60, ms = ms, time = rem ~= 0 }
    end
    if type(v) ~= "string" then return nil end
    local y, m, d = v:match("^(%d%d%d%d)%-(%d%d)%-(%d%d)$")
    local hh, nn, ss
    if not y then
        y, m, d, hh, nn, ss = v:match("^(%d%d%d%d)%-(%d%d)%-(%d%d)T(%d%d):(%d%d):(%d%d)$")
        if not y then return nil end
    end
    local t = { y = tonumber(y), m = tonumber(m), d = tonumber(d),
        hh = tonumber(hh) or 0, nn = tonumber(nn) or 0, ss = tonumber(ss) or 0, time = hh ~= nil }
    if t.m < 1 or t.m > 12 or t.d < 1 or t.d > 31 or t.hh > 23 or t.nn > 59 or t.ss > 59 then return nil end
    return t
end

-- Phase 22b: Luxon token subset. Literals in '...' ('' prints a quote). Locale-free English names.
local MONTHS_LONG = { "January", "February", "March", "April", "May", "June", "July",
    "August", "September", "October", "November", "December" }
local DAYS = { "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday" }
local DATE_TOKENS = { "yyyy", "yy", "MMMM", "MMM", "MM", "M", "dd", "d", "EEEE", "EEE",
    "HH", "H", "mm", "m", "ss", "s", "SSS", "WW", "W" }

local function days_from_civil(y, m, d)
    y = m <= 2 and y - 1 or y
    local era = (y >= 0 and y or y - 399) // 400
    local yoe = y - era * 400
    local doy = (153 * (m > 2 and m - 3 or m + 9) + 2) // 5 + d - 1
    return era * 146097 + yoe * 365 + yoe // 4 - yoe // 100 + doy - 719468
end

-- ISO weekday 1 (Mon) .. 7 (Sun); ISO week: week 1 holds the year's first Thursday.
local function iso_weekday(t)
    return (days_from_civil(t.y, t.m, t.d) + 3) % 7 + 1
end
local function iso_week(t)
    local days = days_from_civil(t.y, t.m, t.d)
    local thursday = days - iso_weekday(t) + 4
    local ty = civil_from_days(thursday)
    return (thursday - days_from_civil(ty, 1, 1)) // 7 + 1
end

local function apply_date(t, mask)
    local out, i, found = {}, 1, false
    while i <= #mask do
        local c = mask:sub(i, i)
        if c == "'" then
            local j = mask:find("'", i + 1, true)
            if j == i + 1 then
                out[#out + 1] = "'"
                i = i + 2
            else
                if not j then return nil end
                out[#out + 1] = mask:sub(i + 1, j - 1)
                i = j + 1
            end
        else
            local hit
            for _, tok in ipairs(DATE_TOKENS) do
                if mask:sub(i, i + #tok - 1) == tok then hit = tok break end
            end
            if hit == "yyyy" then out[#out + 1] = string.format("%04d", t.y)
            elseif hit == "yy" then out[#out + 1] = string.format("%02d", t.y % 100)
            elseif hit == "MMMM" then out[#out + 1] = MONTHS_LONG[t.m]
            elseif hit == "MMM" then out[#out + 1] = MONTHS[t.m]
            elseif hit == "MM" then out[#out + 1] = string.format("%02d", t.m)
            elseif hit == "M" then out[#out + 1] = tostring(t.m)
            elseif hit == "dd" then out[#out + 1] = string.format("%02d", t.d)
            elseif hit == "d" then out[#out + 1] = tostring(t.d)
            elseif hit == "EEEE" then out[#out + 1] = DAYS[iso_weekday(t)]
            elseif hit == "EEE" then out[#out + 1] = DAYS[iso_weekday(t)]:sub(1, 3)
            elseif hit == "HH" then out[#out + 1] = string.format("%02d", t.hh)
            elseif hit == "H" then out[#out + 1] = tostring(t.hh)
            elseif hit == "mm" then out[#out + 1] = string.format("%02d", t.nn)
            elseif hit == "m" then out[#out + 1] = tostring(t.nn)
            elseif hit == "ss" then out[#out + 1] = string.format("%02d", t.ss)
            elseif hit == "s" then out[#out + 1] = tostring(t.ss)
            elseif hit == "SSS" then out[#out + 1] = string.format("%03d", t.ms or 0)
            elseif hit == "WW" then out[#out + 1] = string.format("%02d", iso_week(t))
            elseif hit == "W" then out[#out + 1] = tostring(iso_week(t))
            else out[#out + 1] = c end
            if hit then found = true end
            i = i + (hit and #hit or 1)
        end
    end
    return found and table.concat(out) or nil
end

local function raw(v)
    if type(v) == "number" then return plain_number(v) end
    return plain(v)
end

local warned_format = {}
local function format_warn(item_id, band_id, message)
    local k = tostring(band_id) .. "/" .. tostring(item_id)
    if warned_format[k] then return end
    warned_format[k] = true
    add_log("warn", "item " .. tostring(item_id) .. " in band " .. tostring(band_id) .. " " .. message)
end

-- typeKey 21 number, 24 datetime, 20 text. Null prints blank and skips the mask.
local function format_value(v, tk, mask, item_id, band_id)
    if v == nil then return "" end
    if mask == "" then mask = nil end
    if tk == 21 then
        local n = type(v) == "number" and v or (type(v) == "string" and tonumber(v)) or nil
        if mask == nil then return n and plain_number(n) or raw(v) end
        local spec = number_mask(mask)
        if not spec then
            format_warn(item_id, band_id, "has mask " .. mask .. " which does not parse; printed raw")
            return raw(v)
        end
        if not n or n ~= n or n == math.huge or n == -math.huge then
            format_warn(item_id, band_id, "value " .. raw(v) .. " is not a number; printed raw")
            return raw(v)
        end
        return apply_number(n, spec)
    elseif tk == 24 then
        local t = parse_datetime(v)
        if not t then
            format_warn(item_id, band_id, "value " .. raw(v) .. " is not a date; printed raw")
            return raw(v)
        end
        if mask == nil then
            return t.time and apply_date(t, "yyyy-MM-dd HH:mm") or apply_date(t, "yyyy-MM-dd")
        end
        local s = apply_date(t, mask)
        if not s then
            format_warn(item_id, band_id, "has mask " .. mask .. " which does not parse; printed raw")
            return raw(v)
        end
        return s
    end
    if type(v) == "number" then return plain_number(v) end
    return raw(v)
end


local CSS_UNIT = { ["in"] = true, mm = true, cm = true, pt = true, px = true }
local ALIGN_CSS = { Left = "left", Center = "center", Right = "right" }

local function css_len(len)
    if type(len) ~= "string" then return nil end
    local n, u = len:match("^%s*([%d%.]+)%s*(%a+)%s*$")
    if not n or not CSS_UNIT[u] then return nil end
    return n .. u
end

local function css_or(len, fallback, what)
    local css = css_len(len)
    if css then return css end
    if len ~= nil and len ~= "" then
        add_log("warn", what .. " " .. tostring(len) .. " does not parse; used " .. fallback)
    end
    return fallback
end

local function esc(s)
    if s == nil then return "" end
    return (tostring(s):gsub("&", "&amp;"):gsub("<", "&lt;"):gsub(">", "&gt;"))
end

local function css_family(name)
    if type(name) ~= "string" or name == "" then name = "Vanadium Sans" end
    name = name:gsub("\\", "\\\\"):gsub("'", "\\'")
    return "'" .. name .. "'"
end

local function font_css(style)
    if style == nil or style == "" then style = "Normal" end
    if style ~= "Normal" and style ~= "Bold" and style ~= "Italic" and style ~= "Bold + Italic" then
        add_log("warn", "font style " .. tostring(style) .. " is not a v1 style; printed Normal")
        return "400", "normal"
    end
    local bold = style == "Bold" or style == "Bold + Italic"
    local italic = style == "Italic" or style == "Bold + Italic"
    return bold and "700" or "400", italic and "italic" or "normal"
end

-- Phase 44a: static shapes. Geometry is code. Path kinds use lookup 078
-- when that lookup is loaded, otherwise these commands. PDF does not draw them.
local shape_rows = {}
local shape_loaded = false
local shape_catalog_on = false
local SHAPE_KAPPA = 0.5522847498
local SHAPE_GEOMETRY = { line = true, rectangle = true, roundrect = true, ellipse = true }
local SHAPE_UNIT_PX = { ["in"] = 96, mm = 96 / 25.4, cm = 96 / 2.54, pt = 96 / 72, px = 1 }
local SHAPE_EMBEDDED = {
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

local function shape_fmt(n)
    if type(n) ~= "number" or n ~= n or n == math.huge or n == -math.huge then n = 0 end
    local neg = n < 0
    local rounded = math.floor(math.abs(n) * 10000 + 0.5) / 10000
    if rounded == 0 then return "0" end
    local text = string.format("%.4f", rounded):gsub("0+$", ""):gsub("%.$", "")
    if neg then return "-" .. text end
    return text
end

local function shape_px(len)
    if type(len) ~= "string" then return nil end
    local n, u = len:match("^%s*([%+%-]?[%d%.]+)%s*(%a+)%s*$")
    n = tonumber(n)
    if not n or not SHAPE_UNIT_PX[u] then return nil end
    local px = n * SHAPE_UNIT_PX[u]
    local nearest = math.floor(px + 0.5)
    if math.abs(px - nearest) < 1e-9 then return nearest end
    return px
end

local function shape_len(value, fallback)
    if type(value) ~= "string" or value:match("^%s*$") then return fallback end
    local px = shape_px(value)
    if not px then return fallback end
    return px
end

local function shape_hex(value, fallback)
    if type(value) == "string" and value:match("^#[0-9A-Fa-f][0-9A-Fa-f][0-9A-Fa-f][0-9A-Fa-f][0-9A-Fa-f][0-9A-Fa-f]$") then
        return value:lower()
    end
    return fallback
end

local function shape_commands_ok(commands)
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

local function shape_source(kind)
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

local function shape_scale(commands, w, h)
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

local function shape_d(commands)
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

local function shape_clamp(w, h, radii)
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

local function shape_round(w, h, r)
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

local function shape_ellipse(w, h)
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

local function shape_line(w, h, side)
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
local SHAPE_FILL = {
    none = true, solid = true, grid = true, dots = true, hatched = true, noise = true,
}
local SHAPE_GRID_OK = { dots = true, grid = true, dotted = true, dashed = true, none = true }

local function shape_mul32(a, b)
    local a_lo = a % 65536
    local a_hi = math.floor(a / 65536) % 65536
    local b_lo = b % 65536
    local b_hi = math.floor(b / 65536) % 65536
    local p0 = a_lo * b_lo
    local p1 = a_lo * b_hi + a_hi * b_lo
    return (p0 + (p1 % 65536) * 65536) % 4294967296
end

local function shape_xor_byte(hash, byte)
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

local function shape_fnv(text)
    local hash = 2166136261
    local s = tostring(text or "")
    for i = 1, #s do
        hash = shape_mul32(shape_xor_byte(hash, string.byte(s, i)), 16777619)
    end
    return hash
end

local function shape_lcg(seed)
    local state = seed
    if state == 0 then state = 1 end
    return function()
        state = (shape_mul32(1664525, state) + 1013904223) % 4294967296
        return state / 4294967296
    end
end

local function shape_dom(prefix, id)
    local cleaned = tostring(id or ""):gsub("[^A-Za-z0-9_%-]", "")
    if cleaned == "" then cleaned = "item" end
    return prefix .. cleaned
end

local function shape_weight_ok(weight)
    if weight == nil or weight == "" then return true end
    local n = tonumber(weight)
    if not n or n ~= n or n < 0 or n == math.huge then return false end
    return true
end

local function shape_spacing(def)
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

local function shape_stops(stops)
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

local function shape_angle(paint)
    local n = tonumber(paint.angle)
    if n and n == n and n ~= math.huge and n ~= -math.huge then return n end
    return 0
end

local function shape_resolve(paint)
    local color = "#d3d3d3"
    local kind = "solid"
    if type(paint) == "table" then
        color = shape_hex(paint.color, "#d3d3d3")
        if type(paint.kind) == "string" then kind = paint.kind end
    end
    if kind ~= "linear" and kind ~= "radial" then return { color = color } end
    local stops = type(paint) == "table" and shape_stops(paint.stops) or nil
    if not stops then return { color = color } end
    if kind == "radial" then return { color = color, gradient = { kind = "radial", stops = stops } } end
    return { color = color, gradient = { kind = "linear", angle = shape_angle(paint), stops = stops } }
end

local function shape_linear(w, h, angle)
    local rad = angle * math.pi / 180
    local dx, dy = math.cos(rad), math.sin(rad)
    local cx, cy = w / 2, h / 2
    local extent = math.abs(dx) * w / 2 + math.abs(dy) * h / 2
    return cx - dx * extent, cy - dy * extent, cx + dx * extent, cy + dy * extent
end

local function shape_place(gradient, w, h)
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

local function shape_grid(w, h, spacing)
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

local function shape_hatch_add(points, x, y)
    for _, p in ipairs(points) do
        if math.abs(p.x - x) < 1e-6 and math.abs(p.y - y) < 1e-6 then return end
    end
    points[#points + 1] = { x = x, y = y }
end

local function shape_hatch_seg(points)
    if #points ~= 2 then return nil end
    local p, q = points[1], points[2]
    if not (p.x < q.x - 1e-9 or (math.abs(p.x - q.x) <= 1e-9 and p.y <= q.y)) then
        p, q = q, p
    end
    return "M " .. shape_fmt(p.x) .. " " .. shape_fmt(p.y) .. " L " .. shape_fmt(q.x) .. " " .. shape_fmt(q.y)
end

local function shape_hatch(w, h, spacing)
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

local function shape_dots(w, h, spacing)
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

local function shape_noise(id, w, h, spacing)
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

local function shape_step(spacing)
    if type(spacing) == "number" and spacing > 0 then return spacing end
    return shape_px("0.125 in")
end

local function shape_marks(style, item, w, h, spacing)
    local step = shape_step(spacing)
    if style == "grid" then return { kind = "lines", d = shape_grid(w, h, step), sw = 0.5 } end
    if style == "hatched" then return { kind = "lines", d = shape_hatch(w, h, step), sw = 0.5 } end
    if style == "dots" then return { kind = "circles", circles = shape_dots(w, h, step) } end
    return { kind = "circles", circles = shape_noise(item.id, w, h, step) }
end

local function shape_style(item, kind)
    if kind == "line" then return "none" end
    local style = item.fillStyle
    if style == nil or style == "" then return "solid" end
    if not SHAPE_FILL[style] then return "none" end
    return style
end

local function shape_plan(item, kind, w, h, spacing)
    local style = shape_style(item, kind)
    local raw = type(item.fillPaint) == "table" and item.fillPaint or nil
    local paint = shape_resolve(raw)
    if style == "none" then return { style = style, color = paint.color } end
    local gradient = shape_place(paint.gradient, w, h)
    if style == "solid" then return { style = style, color = paint.color, gradient = gradient } end
    return {
        style = style, color = paint.color, gradient = gradient,
        marks = shape_marks(style, item, w, h, spacing),
    }
end

local function shape_attr(attrs)
    local parts = {}
    for i = 1, #attrs do
        parts[i] = attrs[i][1] .. '="' .. attrs[i][2] .. '"'
    end
    return table.concat(parts, " ")
end

local function shape_gradient(draw)
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

local function shape_with_transform(attrs, transform)
    if transform ~= "" then attrs[#attrs + 1] = { "transform", transform } end
    return attrs
end

local function shape_mark_svg(draw)
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

local function shape_outline(draw)
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

local function shape_defs(draw)
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

local function shape_markup(draw)
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
local function shape_svg(item, spacing)
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

-- Phase 44c: one Helvetica AFM break for the designer, HTML, and later PDF.
-- A grown item is drawn in Helvetica. embedFonts does not switch the measure.
-- Index 1 is U+0020. A missing glyph uses 278. ASCII 32-126 only.
local AFM_ROMAN = {
    278, 278, 355, 556, 556, 889, 667, 222, 333, 333, 389, 584,
    278, 333, 278, 278, 556, 556, 556, 556, 556, 556, 556, 556,
    556, 556, 278, 278, 584, 584, 584, 556, 1015, 667, 667, 722,
    722, 667, 611, 778, 722, 278, 500, 667, 556, 833, 722, 778,
    667, 778, 722, 667, 611, 722, 667, 944, 667, 667, 611, 278,
    278, 278, 469, 556, 222, 556, 556, 500, 556, 556, 278, 556,
    556, 222, 222, 500, 222, 833, 556, 556, 556, 556, 333, 500,
    278, 556, 500, 722, 500, 500, 500, 334, 260, 334, 584,
}
local AFM_BOLD = {
    278, 333, 474, 556, 556, 889, 722, 278, 333, 333, 389, 584,
    278, 333, 278, 278, 556, 556, 556, 556, 556, 556, 556, 556,
    556, 556, 333, 333, 584, 584, 584, 611, 975, 722, 722, 722,
    722, 667, 611, 778, 722, 278, 556, 722, 611, 833, 722, 778,
    667, 778, 722, 667, 611, 722, 667, 944, 667, 667, 611, 333,
    278, 333, 584, 556, 278, 556, 611, 556, 611, 556, 333, 611,
    611, 278, 278, 556, 278, 889, 611, 611, 611, 611, 389, 556,
    333, 611, 556, 778, 556, 556, 500, 389, 280, 389, 584,
}
local AFM_MISSING = 278
local WRAP_MODES = { normal = true, pre = true, ["pre-line"] = true, ["pre-wrap"] = true, nowrap = true }
local FIT_HEIGHTS = { clip = true, grow = true, shrink = true, both = true }
local VALIGNS = { top = true, middle = true, bottom = true }
local LAYOUT_EPS = 1e-6
local MIN_BAND_PT = 0.15 * 72
local LAYOUT_UNIT = { ["in"] = 72, mm = 72 / 25.4, cm = 72 / 2.54, pt = 1, px = 0.75 }

local function fmt_num(n)
    if type(n) ~= "number" or n ~= n or n == math.huge or n == -math.huge then n = 0 end
    local neg = n < 0
    local s = string.format("%.6f", math.abs(n))
    s = s:gsub("0+$", ""):gsub("%.$", "")
    if s == "" then s = "0" end
    if neg and s ~= "0" then s = "-" .. s end
    return s
end

local function fmt_pt(n)
    return fmt_num(n) .. "pt"
end

local function length_pt(len)
    if type(len) ~= "string" then return nil end
    local n, u = len:match("^%s*([%+%-]?[%d%.]+)%s*(%a+)%s*$")
    n = tonumber(n)
    if not n or not LAYOUT_UNIT[u] then return nil end
    return n * LAYOUT_UNIT[u]
end

local function half_away(n)
    if type(n) ~= "number" or n ~= n or n == math.huge or n == -math.huge then return 0 end
    if n >= 0 then return math.floor(n + 0.5) end
    return -math.floor(-n + 0.5)
end

local function face_bold(style)
    return style == "Bold" or style == "Bold + Italic"
end

local function advance_units(ch, style)
    local b = string.byte(ch)
    local table = face_bold(style) and AFM_BOLD or AFM_ROMAN
    if not b or b < 32 or b > 126 then return AFM_MISSING end
    return table[b - 31]
end

local function advance_pt(ch, size, style)
    return advance_units(ch, style) * size / 1000
end

local function measure_pt(text, size, style)
    local width = 0
    for i = 1, #text do width = width + advance_pt(text:sub(i, i), size, style) end
    return width
end

local function line_height_pt(size, multiple)
    local factor = 1.2
    if type(multiple) == "number" and multiple == multiple and multiple > 0 and multiple ~= math.huge then
        factor = multiple
    end
    return factor * size
end

local function collapse_ws(text)
    local s = text:gsub("[ \t\n\r\f]+", " ")
    return (s:gsub("^[ ]+", ""):gsub("[ ]+$", ""))
end

local function collapse_line(text)
    local s = text:gsub("[ \t\f]+", " ")
    return (s:gsub("^[ ]+", ""):gsub("[ ]+$", ""))
end

local function split_chars(text)
    local out = {}
    for i = 1, #text do out[i] = text:sub(i, i) end
    return out
end

local function split_words(text)
    local out = {}
    for word in text:gmatch("%S+") do out[#out + 1] = word end
    return out
end

local function split_nl(text)
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

local function break_word(word, width, size, style)
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

local function pack_words(words, width, size, style, char_break)
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

local function wrap_pre(text, width, size, style)
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

local function wrap_text(text, width, font_size, style, mode)
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

local function default_fit(type_key)
    if type_key == 3 then return "both" end
    return "clip"
end

local function default_wrap(type_key)
    if type_key == 3 then return "pre-wrap" end
    if type_key == 1 or type_key == 4 then return "nowrap" end
    return "pre-line"
end

local function paints_lines(item, fit)
    local tk = tonumber(item and item.typeKey)
    if tk == 1 or tk == 3 or tk == 4 then return true end
    if (tk == 0 or tk == 20) and fit ~= "clip" then return true end
    return false
end

local function layout_warn(seen, key, message)
    if seen[key] then return end
    seen[key] = true
    add_log("warn", message)
end

local function font_of(doc, item)
    local own = type(item.font) == "table" and item.font or nil
    local report = type(doc) == "table" and type(doc.font) == "table" and doc.font or nil
    local source = own or report or {}
    return {
        family = (type(source.family) == "string" and source.family ~= "") and source.family or "Vanadium Sans",
        size = (type(source.size) == "string" and source.size ~= "") and source.size or "10 pt",
        style = (type(source.style) == "string" and source.style ~= "") and source.style or "Normal",
    }
end

local function resolve_wrap(item, seen)
    local tk = tonumber(item.typeKey) or 0
    local mode = item.wrap
    if mode == nil or mode == "" then return default_wrap(tk) end
    if WRAP_MODES[mode] then return mode end
    local fallback = default_wrap(tk)
    layout_warn(seen, "wrap:" .. tostring(item.id),
        "item " .. tostring(item.id) .. " wrap " .. tostring(mode) .. " is not a wrap mode; used " .. fallback)
    return fallback
end

local function resolve_fit_height(item, seen)
    local fallback = default_fit(tonumber(item.typeKey) or 0)
    local fit = item.fitHeight
    if fit == nil or fit == "" then return fallback end
    if FIT_HEIGHTS[fit] then return fit end
    layout_warn(seen, "fit:" .. tostring(item.id),
        "item " .. tostring(item.id) .. " fitHeight " .. tostring(fit)
        .. " is not clip, grow, shrink, or both; used " .. fallback)
    return fallback
end

local function resolve_valign(item, seen)
    local valign = item.valign
    if valign == nil or valign == "" then return "top" end
    if VALIGNS[valign] then return valign end
    layout_warn(seen, "valign:" .. tostring(item.id),
        "item " .. tostring(item.id) .. " valign " .. tostring(valign) .. " is not top, middle, or bottom; used top")
    return "top"
end

local function resolve_multiple(item, seen)
    if item.lineHeight == nil or item.lineHeight == "" then return 1.2 end
    local value = tonumber(item.lineHeight)
    if value and value == value and value > 0 and value ~= math.huge then return value end
    layout_warn(seen, "lh:" .. tostring(item.id),
        "item " .. tostring(item.id) .. " lineHeight " .. tostring(item.lineHeight)
        .. " is not a positive multiple; used 1.2")
    return 1.2
end

local function valign_offset(valign, box_pt, block_pt)
    local slack = box_pt - block_pt
    if slack <= LAYOUT_EPS then return 0 end
    if valign == "bottom" then return slack end
    if valign == "middle" then return slack / 2 end
    return 0
end

local function fitted_height(fit, stored_pt, content_pt)
    if fit == "grow" then return math.max(stored_pt, content_pt) end
    if fit == "shrink" then return math.min(stored_pt, content_pt) end
    if fit == "both" then return content_pt end
    return stored_pt
end

local function layout_band(doc, band, entries)
    local seen, placed, reflow = {}, {}, false
    for _, entry in ipairs(entries or {}) do
        local item = entry.item or {}
        local fit = entry.defer and "clip" or resolve_fit_height(item, seen)
        if fit ~= "clip" then reflow = true end
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
        local leading = line_height_pt(font_size, multiple)
        local mode = resolve_wrap(item, seen)
        local valign = resolve_valign(item, seen)
        local lines = nil
        if paints_lines(item, fit) and not entry.defer then
            lines = wrap_text(entry.text or "", width, font_size, font.style, mode)
        end
        local stored_h = (height and height > 0) and height or 0
        local content_h = math.max(1, lines and #lines or 1) * leading
        local used_h = (entry.defer or not lines) and stored_h or fitted_height(fit, stored_h, content_h)
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
        placed[#placed + 1] = {
            item = item, fit = fit, lines = painted, font = font, leading = leading,
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
        }
    end
    return { reflow = reflow, usedPt = used_pt, items = items }
end

local IMAGE_FITS = { stretch = true, contain = true, cover = true, none = true, ["scale-down"] = true }
local IMAGE_FLIPS = { none = true, horizontal = true, vertical = true, both = true }
local IMAGE_ALIGNS = { Left = true, Center = true, Right = true }
local IMAGE_VALIGNS = { Top = true, Middle = true, Bottom = true }
local SVG_TAGS = {
    svg = true, g = true, path = true, rect = true, circle = true,
    ellipse = true, line = true, polyline = true, polygon = true,
    defs = true, style = true,
}

local function svg_subset(uri)
    if type(uri) ~= "string" then return "" end
    local comma = uri:find(",", 1, true)
    if not comma then return "" end
    local meta = uri:sub(1, comma - 1):lower()
    if meta:find("base64", 1, true) then return "" end
    local payload = uri:sub(comma + 1):gsub("%%(%x%x)", function(h)
        return string.char(tonumber(h, 16))
    end)
    local lower = payload:lower()
    if lower:find("javascript:", 1, true) or lower:find("<script", 1, true)
        or lower:find("<foreignobject", 1, true) or lower:find("<text", 1, true)
        or lower:find("<image", 1, true) or lower:find("<filter", 1, true)
        or lower:find(" on%w+%s*=") then
        return ""
    end
    if not lower:find("<svg", 1, true) then return "" end
    for tag in payload:gmatch("</?([%a][%w:%-]*)") do
        local name = tag:match("^[%a]+") or tag
        if not SVG_TAGS[name:lower()] then return "" end
    end
    return payload
end

local function display_px(len, dpi)
    local pt = length_pt(len)
    if not pt then return 1 end
    local density = 72
    if type(dpi) == "number" and dpi == dpi and dpi > 0 and dpi ~= math.huge then density = dpi end
    local px = half_away(pt / 72 * density)
    if px < 1 then px = 1 end
    return px
end

local function raster_lookup(key)
    local rasters = type(P.rasters) == "table" and P.rasters or nil
    if not rasters then return nil end
    local entry = rasters[key]
    if type(entry) == "string" and entry ~= "" then return entry end
    if type(entry) == "table" and type(entry.data) == "string" and entry.data ~= "" then return entry.data end
    return nil
end

local function image_once(state, key, message)
    if state.warned[key] then return end
    state.warned[key] = true
    add_log("warn", message)
end

local function image_fit(o, state)
    local fit = o.fit
    if fit == nil or fit == "" or IMAGE_FITS[fit] then return (fit ~= nil and fit ~= "") and fit or "contain" end
    image_once(state, "fit:" .. tostring(o.id),
        "item " .. tostring(o.id) .. " fit " .. tostring(fit) .. " is not a fit; used contain")
    return "contain"
end

local function image_flip(o, state)
    local flip = o.flip
    if flip == nil or flip == "" or IMAGE_FLIPS[flip] then return (flip ~= nil and flip ~= "") and flip or "none" end
    image_once(state, "flip:" .. tostring(o.id),
        "item " .. tostring(o.id) .. " flip " .. tostring(flip) .. " is not a flip; used none")
    return "none"
end

local function image_anchor(o, state, horizontal)
    local value = horizontal and o.fitAlign or o.fitValign
    local allowed = horizontal and IMAGE_ALIGNS or IMAGE_VALIGNS
    local fallback = horizontal and "Center" or "Middle"
    if value == nil or value == "" or allowed[value] then
        return (value ~= nil and value ~= "") and value or fallback
    end
    image_once(state, (horizontal and "ax:" or "ay:") .. tostring(o.id),
        "item " .. tostring(o.id) .. " anchor " .. tostring(value) .. " is not an anchor; used " .. fallback)
    return fallback
end

local function image_opacity(o)
    if o.opacity == nil or o.opacity == "" then return "1" end
    local n = tonumber(o.opacity)
    if not n or n ~= n or n == math.huge or n == -math.huge then return "1" end
    if n < 0 then n = 0 elseif n > 1 then n = 1 end
    return fmt_num(n)
end

local function image_rotation(o, state)
    if o.rotation == nil or o.rotation == "" then return 0 end
    local n = tonumber(o.rotation)
    if not n or n ~= n or n == math.huge or n == -math.huge then
        image_once(state, "rot:" .. tostring(o.id),
            "item " .. tostring(o.id) .. " rotation " .. tostring(o.rotation) .. " is not a number; used 0")
        return 0
    end
    return n
end

local function image_transform(o, state, pxw, pxh)
    local flip = image_flip(o, state)
    local fx, fy = 1, 1
    if flip == "horizontal" or flip == "both" then fx = -1 end
    if flip == "vertical" or flip == "both" then fy = -1 end
    local rot = image_rotation(o, state)
    local cx, cy = pxw / 2, pxh / 2
    return string.format(
        "rotate(%s %s %s) translate(%s %s) scale(%s %s) translate(%s %s)",
        fmt_num(rot), fmt_num(cx), fmt_num(cy),
        fmt_num(cx), fmt_num(cy), fmt_num(fx), fmt_num(fy),
        fmt_num(-cx), fmt_num(-cy))
end

local function emit_image(def, o, box, state)
    local hash = o.image
    if type(hash) ~= "string" or hash == "" then
        image_once(state, "missing:" .. tostring(o.id),
            "item " .. tostring(o.id) .. " has no image; skipped")
        return nil
    end
    local images = type(def.images) == "table" and def.images or {}
    local canonical = images[hash]
    local left = css_or(box.left, "0in", "box left")
    local top = css_or(box.top, "0in", "box top")
    local width = css_or(box.width, "0in", "box width")
    local height = css_or(box.height, "0in", "box height")
    local opacity = image_opacity(o)
    if type(canonical) == "string" and canonical:lower():find("^data:image/svg%+xml") then
        local markup = svg_subset(canonical)
        if markup == "" then
            image_once(state, "svg:" .. tostring(o.id),
                "item " .. tostring(o.id) .. " svg is outside the path subset; skipped")
            return nil
        end
        return string.format(
            '<div style="position:absolute;left:%s;top:%s;width:%s;height:%s;overflow:hidden;padding:0;opacity:%s">%s</div>',
            left, top, width, height, opacity, markup)
    end
    local pxw = display_px(box.width, o.dpi)
    local pxh = display_px(box.height, o.dpi)
    local fit = image_fit(o, state)
    local align = image_anchor(o, state, true)
    local valign = image_anchor(o, state, false)
    local key = table.concat({
        hash, tostring(pxw), tostring(pxh), fit, align, valign,
        o.grayscale == true and "true" or "false",
        o.invert == true and "true" or "false",
    }, "|")
    local data = raster_lookup(key)
    if data == nil then
        if type(canonical) == "string" and canonical ~= "" then
            data = canonical
            if not state.dpi then
                state.dpi = true
                add_log("warn", "display dpi was not applied")
            end
        else
            image_once(state, "bytes:" .. hash,
                "item " .. tostring(o.id) .. " image " .. hash .. " has no bytes; skipped")
            return nil
        end
    end
    local id = state.ids[key]
    if not id then
        state.n = state.n + 1
        id = "rptimg" .. tostring(state.n)
        state.ids[key] = id
        state.list[#state.list + 1] = { id = id, data = data, w = pxw, h = pxh }
    end
    return string.format(
        '<div style="position:absolute;left:%s;top:%s;width:%s;height:%s;overflow:hidden;padding:0;opacity:%s"><svg width="100%%" height="100%%" viewBox="0 0 %d %d" preserveAspectRatio="none"><use href="#%s" transform="%s"/></svg></div>',
        left, top, width, height, opacity, pxw, pxh, id, image_transform(o, state, pxw, pxh))
end

local function embed_font_css(def, pages)
    if type(def) ~= "table" or def.embedFonts ~= true then return "" end
    local fonts = type(P.fonts) == "table" and P.fonts or nil
    if not fonts then
        add_log("warn", "embedFonts is on and params.fonts is missing; named the family")
        return ""
    end
    local used, order = {}, {}
    for _, page in ipairs(pages or {}) do
        for _, band in ipairs(page.bands or {}) do
            for _, o in ipairs(band.items or {}) do
                local fam = type(o.font) == "table" and o.font.family or nil
                if type(fam) == "string" and fam ~= "" and fam ~= "Helvetica" and not used[fam] then
                    used[fam] = true
                    order[#order + 1] = fam
                end
            end
        end
    end
    local rules, missing = {}, false
    for _, fam in ipairs(order) do
        local src = fonts[fam]
        if type(src) ~= "string" or src == "" then
            if not missing then
                missing = true
                add_log("warn", "embedFonts has no bytes for " .. fam .. "; named the family")
            end
        else
            local uri = src
            if not uri:find("^data:") then uri = "data:font/woff2;base64," .. uri end
            rules[#rules + 1] = "@font-face{font-family:" .. css_family(fam)
                .. ";src:url(" .. uri .. ") format('woff2');}"
        end
    end
    if #rules == 0 then return "" end
    return "<style>" .. table.concat(rules) .. "</style>"
end

local BARCODE_NAMES = {
    qr = "qrcode", code39 = "code39", code128 = "code128",
    upca = "upca", ["gs1-128"] = "gs1128",
}
local BARCODE_BOOL = {
    "gs1", "gs1parens", "nobackground", "heightperrow", "compliantheight",
    "bind", "bindtop", "box", "quietzones", "noquietzones", "notext", "small",
    "bold", "embedfont", "dotty", "reverse", "init", "esc", "extraesc",
}
local BARCODE_NUM = {
    "fg", "bg", "scale", "scalexdimdp", "height", "border", "whitesp",
    "vwhitesp", "textgap", "guarddescent", "dotsize",
}
local BARCODE_OMIT = { scale = 1, textgap = 1, guarddescent = 5, border = 0, whitesp = 0, vwhitesp = 0 }
local FIT_X = { Left = "Min", Center = "Mid", Right = "Max" }
local FIT_Y = { Top = "Min", Middle = "Mid", Bottom = "Max" }
local barcode_mod
local barcode_missing = false

local function barcode_symbol(bc)
    local name = bc.symbology
    if type(name) ~= "string" or name == "" then name = "code128" end
    if name == "upca" and bc.check == "supplied" then return "upcachk" end
    local sym = BARCODE_NAMES[name]
    if not sym then return nil, "unknown symbology" end
    return sym
end

local function barcode_key_ok(sym, key)
    if (key == "gs1" or key == "gs1parens" or key == "dotty" or key == "dotsize") and sym ~= "qrcode" then
        return false
    end
    if key == "extraesc" and sym ~= "code128" then return false end
    if key == "guarddescent" and sym ~= "upca" and sym ~= "upcachk" then return false end
    if key == "gs1" and sym == "gs1128" then return false end
    return true
end

local function barcode_color(value)
    if type(value) ~= "string" or value == "" then return nil end
    if value:sub(1, 1) == "#" then return value:sub(2) end
    return value
end

local function barcode_options(sym, bc, data)
    local opts = { barcode = sym, filetype = "SVG", data = data == nil and "" or tostring(data) }
    for _, key in ipairs(BARCODE_BOOL) do
        if barcode_key_ok(sym, key) and bc[key] == true then opts[key] = true end
    end
    for _, key in ipairs(BARCODE_NUM) do
        if barcode_key_ok(sym, key) and bc[key] ~= nil and bc[key] ~= "" then
            local value = bc[key]
            if key == "fg" or key == "bg" then
                value = barcode_color(value)
            else
                value = tonumber(value)
                if value ~= nil and BARCODE_OMIT[key] == value then value = nil end
            end
            if value ~= nil then opts[key] = value end
        end
    end
    return opts
end

local function barcode_fn()
    if barcode_mod or barcode_missing then return barcode_mod end
    local ok, fn = pcall(require, "zint.barcode")
    if not ok or type(fn) ~= "function" then
        barcode_missing = true
        return nil
    end
    barcode_mod = fn
    return fn
end

local function barcode_note(id, message, warned)
    local key = tostring(id) .. "\0" .. message
    if warned[key] then return end
    warned[key] = true
    add_log("warn", "item " .. tostring(id) .. " " .. message)
end

local function fit_anchor(name, horizontal)
    if horizontal then
        if name == "Left" then return 0 end
        if name == "Right" then return 1 end
        return 0.5
    end
    if name == "Top" then return 0 end
    if name == "Bottom" then return 1 end
    return 0.5
end

local function barcode_fit_name(item)
    local fit = item.fit
    if fit == "cover" or fit == "none" or fit == "scale-down" or fit == "contain" then
        return fit
    end
    return "contain"
end

local function fit_aspect(item, mode)
    local x = FIT_X[item.fitAlign] or "Mid"
    local y = FIT_Y[item.fitValign] or "Mid"
    if mode ~= "slice" then mode = "meet" end
    return "x" .. x .. "Y" .. y .. " " .. mode
end

local function fit_svg(svg, item, box)
    if type(svg) ~= "string" then return nil end
    local w, h = svg:match('<svg width="(%d+)" height="(%d+)"')
    if not w then return nil end
    local sw, sh = tonumber(w), tonumber(h)
    local fit = barcode_fit_name(item)
    local box_w = shape_px(type(box) == "table" and box.width or nil) or 0
    local box_h = shape_px(type(box) == "table" and box.height or nil) or 0
    local intrinsic = fit == "none"
    if fit == "scale-down" and box_w > 0 and box_h > 0 then
        intrinsic = sw <= box_w and sh <= box_h
    end
    local open
    local shift = nil
    if intrinsic then
        local ax = fit_anchor(item.fitAlign, true)
        local ay = fit_anchor(item.fitValign, false)
        shift = {
            x = shape_fmt((box_w - sw) * ax) .. "px",
            y = shape_fmt((box_h - sh) * ay) .. "px",
        }
        open = string.format(
            '<svg width="%d" height="%d" viewBox="0 0 %s %s" preserveAspectRatio="%s" xmlns="http://www.w3.org/2000/svg">',
            sw, sh, w, h, fit_aspect(item, "meet"))
    else
        local mode = fit == "cover" and "slice" or "meet"
        open = string.format(
            '<svg width="100%%" height="100%%" viewBox="0 0 %s %s" preserveAspectRatio="%s" xmlns="http://www.w3.org/2000/svg">',
            w, h, fit_aspect(item, mode))
    end
    local fitted = svg:gsub("<svg[^>]*>", function() return open end, 1)
    return fitted, shift
end

local function emit_barcode(o, warned)
    local bc = type(o.barcode) == "table" and o.barcode or {}
    local sym, why = barcode_symbol(bc)
    if not sym then
        barcode_note(o.id, why or "unknown symbology", warned)
        return nil
    end
    if o.typeKey == 45 and (bc.data == nil or bc.data == "") then
        return nil
    end
    local fn = barcode_fn()
    if not fn then
        barcode_note(o.id, "zint.barcode is not loaded", warned)
        return nil
    end
    local opts = barcode_options(sym, bc, bc.data)
    local ok, out, warning = pcall(fn, opts)
    if not ok or out == nil or type(out) ~= "string" then
        local msg = "barcode failed"
        if ok and type(warning) == "string" then
            local err = warning:match("^(Error %d+)")
            if err then msg = err end
        end
        barcode_note(o.id, msg, warned)
        return nil
    end
    local box = type(o.box) == "table" and o.box or {}
    local fitted, shift = fit_svg(out, o, box)
    if not fitted then
        barcode_note(o.id, "barcode failed", warned)
        return nil
    end
    if shift then
        fitted = string.format(
            '<div style="margin-left:%s;margin-top:%s">%s</div>',
            shift.x, shift.y, fitted)
    end
    local turn = tonumber(o.rotation) or 0
    local extra = ""
    if turn ~= 0 then extra = string.format(";transform:rotate(%ddeg)", turn) end
    return string.format(
        '<div style="position:absolute;left:%s;top:%s;width:%s;height:%s;overflow:hidden;padding:0%s">%s</div>',
        css_or(box.left, "0in", "box left"),
        css_or(box.top, "0in", "box top"),
        css_or(box.width, "0in", "box width"),
        css_or(box.height, "0in", "box height"),
        extra,
        fitted)
end

local function emit_html(def, pages)
    local shape_warned = {}
    local barcode_warned = {}
    local image_state = { ids = {}, list = {}, n = 0, warned = {}, dpi = false }
    local pg = type(def.page) == "table" and def.page or {}
    local mg = type(pg.margin) == "table" and pg.margin or {}
    local width = css_or(pg.width, "8.5in", "page width")
    local height = css_or(pg.height, "11in", "page height")
    local top = css_or(mg.top, "0.25in", "margin top")
    local right = css_or(mg.right, "0.25in", "margin right")
    local bottom = css_or(mg.bottom, "0.25in", "margin bottom")
    local left = css_or(mg.left, "0.25in", "margin left")
    local parts = {}
    for _, page in ipairs(pages or {}) do
        local bands = {}
        for _, band in ipairs(page.bands or {}) do
            local items = {}
            for _, o in ipairs(band.items or {}) do
                local box = type(o.box) == "table" and o.box or nil
                if not box then
                    add_log("warn", "item " .. tostring(o.id) .. " has no box; skipped")
                elseif o.typeKey == 25 then
                    local svg, swarn = shape_svg(o, shape_spacing(def))
                    if swarn ~= "" then
                        local skey = tostring(o.id) .. "\0" .. swarn
                        if not shape_warned[skey] then
                            shape_warned[skey] = true
                            add_log("warn", "item " .. tostring(o.id) .. " " .. swarn)
                        end
                    else
                        items[#items + 1] = string.format(
                            '<div style="position:absolute;left:%s;top:%s;width:%s;height:%s;overflow:visible;padding:0">%s</div>',
                            css_or(box.left, "0in", "box left"),
                            css_or(box.top, "0in", "box top"),
                            css_or(box.width, "0in", "box width"),
                            css_or(box.height, "0in", "box height"),
                            svg)
                    end
                elseif o.typeKey == 43 or o.typeKey == 45 then
                    local html = emit_barcode(o, barcode_warned)
                    if html then items[#items + 1] = html end
                elseif type(o.lines) == "table" then
                    local font = type(o.font) == "table" and o.font or {}
                    local weight, slant = font_css(font.style)
                    local align = ALIGN_CSS[o.align]
                    if not align then
                        if o.align ~= nil then
                            add_log("warn", "align " .. tostring(o.align) .. " is not Left, Center, or Right; printed left")
                        end
                        align = "left"
                    end
                    local pad = o.offsetPt or 0
                    local padding = math.abs(pad) < 1e-6 and "0" or (fmt_pt(pad) .. " 0 0 0")
                    local chunks = {}
                    for _, line in ipairs(o.lines) do chunks[#chunks + 1] = esc(line) end
                    items[#items + 1] = string.format(
                        '<div style="position:absolute;left:%s;top:%s;width:%s;height:%s;overflow:hidden;padding:%s;white-space:pre;line-height:%s;text-align:%s;font-family:%s;font-size:%s;font-weight:%s;font-style:%s">%s</div>',
                        css_or(box.left, "0in", "box left"),
                        css_or(box.top, "0in", "box top"),
                        css_or(box.width, "0in", "box width"),
                        css_or(box.height, "0in", "box height"),
                        padding,
                        fmt_pt(o.lineHeightPt or 12),
                        align,
                        css_family(font.family),
                        css_or(font.size, "10pt", "font size"),
                        weight,
                        slant,
                        table.concat(chunks, "\n"))
                elseif o.typeKey == 2 then
                    local html = emit_image(def, o, box, image_state)
                    if html then items[#items + 1] = html end
                else
                    local font = type(o.font) == "table" and o.font or {}
                    local weight, slant = font_css(font.style)
                    local align = ALIGN_CSS[o.align]
                    if not align then
                        if o.align ~= nil then
                            add_log("warn", "align " .. tostring(o.align) .. " is not Left, Center, or Right; printed left")
                        end
                        align = "left"
                    end
                    items[#items + 1] = string.format(
                        '<div style="position:absolute;left:%s;top:%s;width:%s;height:%s;overflow:hidden;white-space:pre-line;text-align:%s;font-family:%s;font-size:%s;font-weight:%s;font-style:%s">%s</div>',
                        css_or(box.left, "0in", "box left"),
                        css_or(box.top, "0in", "box top"),
                        css_or(box.width, "0in", "box width"),
                        css_or(box.height, "0in", "box height"),
                        align,
                        css_family(font.family),
                        css_or(font.size, "10pt", "font size"),
                        weight,
                        slant,
                        esc(o.text))
                end
            end
            bands[#bands + 1] = string.format(
                '<div style="position:relative;height:%s">%s</div>',
                css_or(band.usedHeight or band.height, "0in", "band height"),
                table.concat(items))
        end
        parts[#parts + 1] = string.format(
            '<section style="position:relative;box-sizing:border-box;width:%s;height:%s;padding:%s %s %s %s;break-after:page">%s</section>',
            width, height, top, right, bottom, left, table.concat(bands))
    end
    local defs = ""
    if #image_state.list > 0 then
        local tags = {}
        for _, im in ipairs(image_state.list) do
            tags[#tags + 1] = string.format('<image id="%s" href="%s" width="%d" height="%d"/>', im.id, im.data, im.w, im.h)
        end
        defs = '<svg width="0" height="0"><defs>' .. table.concat(tags) .. '</defs></svg>'
    end
    return embed_font_css(def, pages) .. defs .. table.concat(parts, "\n") .. "\n"
end

-- A stored report is text. Hydrogen does not parse that column into a table.
-- phase27:json-start
local function json_decode(text)
    if type(text) ~= "string" then
        return nil, "not json"
    end
    local s = text
    local n = #s
    local i = 1
    local depth = 0

    local function skip()
        local _, e = s:sub(i):find("^%s*")
        if e and e > 0 then
            i = i + e
        end
    end

    local parse_value

    local function parse_string()
        i = i + 1
        local parts = {}
        while i <= n do
            local c = s:sub(i, i)
            if c == '"' then
                i = i + 1
                return table.concat(parts), nil
            end
            if c == "\\" then
                local esc = s:sub(i + 1, i + 1)
                if esc == '"' or esc == "\\" or esc == "/" then
                    parts[#parts + 1] = esc
                    i = i + 2
                elseif esc == "b" then
                    parts[#parts + 1] = "\b"
                    i = i + 2
                elseif esc == "f" then
                    parts[#parts + 1] = "\f"
                    i = i + 2
                elseif esc == "n" then
                    parts[#parts + 1] = "\n"
                    i = i + 2
                elseif esc == "r" then
                    parts[#parts + 1] = "\r"
                    i = i + 2
                elseif esc == "t" then
                    parts[#parts + 1] = "\t"
                    i = i + 2
                elseif esc == "u" then
                    local hex = s:sub(i + 2, i + 5)
                    if not hex:match("^[0-9a-fA-F][0-9a-fA-F][0-9a-fA-F][0-9a-fA-F]$") then
                        return nil, "bad unicode escape"
                    end
                    local cp = tonumber(hex, 16)
                    i = i + 6
                    if cp >= 0xD800 and cp <= 0xDBFF then
                        if s:sub(i, i + 1) ~= "\\u" then
                            return nil, "bad unicode escape"
                        end
                        local hex2 = s:sub(i + 2, i + 5)
                        if not hex2:match("^[0-9a-fA-F][0-9a-fA-F][0-9a-fA-F][0-9a-fA-F]$") then
                            return nil, "bad unicode escape"
                        end
                        local low = tonumber(hex2, 16)
                        if low < 0xDC00 or low > 0xDFFF then
                            return nil, "bad unicode escape"
                        end
                        cp = 0x10000 + (cp - 0xD800) * 0x400 + (low - 0xDC00)
                        i = i + 6
                    elseif cp >= 0xDC00 and cp <= 0xDFFF then
                        return nil, "bad unicode escape"
                    end
                    local okc, ch = pcall(utf8.char, cp)
                    if not okc then
                        return nil, "bad unicode escape"
                    end
                    parts[#parts + 1] = ch
                else
                    return nil, "bad escape"
                end
            else
                if c:byte() < 32 then
                    return nil, "bad string"
                end
                parts[#parts + 1] = c
                i = i + 1
            end
        end
        return nil, "unterminated string"
    end

    local function parse_number()
        local start = i
        if s:sub(i, i) == "-" then
            i = i + 1
        end
        local d = s:sub(i, i)
        if d < "0" or d > "9" then
            return nil, "bad number"
        end
        if d == "0" then
            i = i + 1
        else
            while true do
                local c = s:sub(i, i)
                if c < "0" or c > "9" then break end
                i = i + 1
            end
        end
        if s:sub(i, i) == "." then
            i = i + 1
            local frac = i
            while true do
                local c = s:sub(i, i)
                if c < "0" or c > "9" then break end
                i = i + 1
            end
            if i == frac then
                return nil, "bad number"
            end
        end
        local exp = s:sub(i, i)
        if exp == "e" or exp == "E" then
            i = i + 1
            local sign = s:sub(i, i)
            if sign == "+" or sign == "-" then
                i = i + 1
            end
            local exp_at = i
            while true do
                local c = s:sub(i, i)
                if c < "0" or c > "9" then break end
                i = i + 1
            end
            if i == exp_at then
                return nil, "bad number"
            end
        end
        local num = tonumber(s:sub(start, i - 1))
        if num == nil then
            return nil, "bad number"
        end
        return num, nil
    end

    local function parse_object()
        depth = depth + 1
        if depth > 64 then
            return nil, "too deep"
        end
        i = i + 1
        local obj = {}
        skip()
        if s:sub(i, i) == "}" then
            i = i + 1
            depth = depth - 1
            return obj, nil
        end
        while true do
            skip()
            if s:sub(i, i) ~= '"' then
                return nil, "bad object"
            end
            local key, kerr = parse_string()
            if kerr then
                return nil, kerr
            end
            skip()
            if s:sub(i, i) ~= ":" then
                return nil, "bad object"
            end
            i = i + 1
            local val, verr = parse_value()
            if verr then
                return nil, verr
            end
            obj[key] = val
            skip()
            local c = s:sub(i, i)
            if c == "}" then
                i = i + 1
                depth = depth - 1
                return obj, nil
            end
            if c ~= "," then
                return nil, "bad object"
            end
            i = i + 1
        end
    end

    local function parse_array()
        depth = depth + 1
        if depth > 64 then
            return nil, "too deep"
        end
        i = i + 1
        local arr = {}
        local idx = 1
        skip()
        if s:sub(i, i) == "]" then
            i = i + 1
            depth = depth - 1
            return arr, nil
        end
        while true do
            local val, verr = parse_value()
            if verr then
                return nil, verr
            end
            arr[idx] = val
            idx = idx + 1
            skip()
            local c = s:sub(i, i)
            if c == "]" then
                i = i + 1
                depth = depth - 1
                return arr, nil
            end
            if c ~= "," then
                return nil, "bad array"
            end
            i = i + 1
        end
    end

    parse_value = function()
        skip()
        local c = s:sub(i, i)
        if c == '"' then
            return parse_string()
        end
        if c == "{" then
            return parse_object()
        end
        if c == "[" then
            return parse_array()
        end
        if c == "t" then
            if s:sub(i, i + 3) ~= "true" then
                return nil, "bad value"
            end
            i = i + 4
            return true, nil
        end
        if c == "f" then
            if s:sub(i, i + 4) ~= "false" then
                return nil, "bad value"
            end
            i = i + 5
            return false, nil
        end
        if c == "n" then
            if s:sub(i, i + 3) ~= "null" then
                return nil, "bad value"
            end
            i = i + 4
            return nil, nil
        end
        if c == "-" or (c >= "0" and c <= "9") then
            return parse_number()
        end
        return nil, "bad value"
    end

    local val, verr = parse_value()
    if verr then
        return nil, verr
    end
    skip()
    if i <= n then
        return nil, "trailing data"
    end
    return val, nil
end
-- phase27:json-end

local function integer_id(value)
    local n = tonumber(value)
    if n == nil or n ~= n or n == math.huge or n == -math.huge then
        return nil
    end
    if n ~= math.floor(n) or n < 1 then
        return nil
    end
    return math.floor(n)
end

local function row_field(row, name)
    if type(row) ~= "table" then
        return nil
    end
    if row[name] ~= nil then
        return row[name]
    end
    local upper = name:upper()
    if row[upper] ~= nil then
        return row[upper]
    end
    for key, value in pairs(row) do
        if type(key) == "string" and key:upper() == upper then
            return value
        end
    end
    return nil
end

local function shape_collection(value)
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

local function load_shape_rows()
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


local function first_datasource_name(def)
    local list = def.datasources
    if type(list) ~= "table" then
        return nil
    end
    for _, entry in ipairs(list) do
        if type(entry) == "string" and entry ~= "" then
            return entry
        end
        if type(entry) == "table" and type(entry.name) == "string" and entry.name ~= "" then
            return entry.name
        end
    end
    return nil
end

-- Detail, group header, and group footer with no datasource use the first one.
local function fill_band_datasources(def)
    if type(def) ~= "table" or type(def.bands) ~= "table" then
        return def
    end
    local name = first_datasource_name(def)
    if not name then
        return def
    end
    for _, band in ipairs(def.bands) do
        if type(band) == "table" then
            local key = band.typeKey
            if (key == 14 or key == 15 or key == 16)
                and (band.datasource == nil or band.datasource == "") then
                band.datasource = name
            end
        end
    end
    return def
end

local function report_where(id, rev)
    return "report " .. tostring(id) .. " revision " .. tostring(rev)
end

local function resolve_definition()
    if type(P.definition) == "table" then
        return P.definition, nil
    end
    local id = integer_id(P.report_id)
    local rev = integer_id(P.rev_id)
    if not id or not rev then
        return nil, nil
    end
    local hy = type(P._hydrogen) == "table" and P._hydrogen or {}
    local database = hy.database
    if type(database) ~= "string" or database == "" then
        return nil, "no session database; cannot load " .. report_where(id, rev)
    end
    if type(H) ~= "table" or type(H.altquery_sync) ~= "function" then
        return nil, "could not read QueryRef 167: H.altquery_sync is missing"
    end
    local ok, res, err = pcall(H.altquery_sync, database, [[
        SELECT code FROM ${SCHEMA}queries
        WHERE query_ref = 167 AND query_type_a28 = 1
    ]], {})
    if not ok then
        return nil, "could not read QueryRef 167: " .. tostring(res)
    end
    if err then
        return nil, "could not read QueryRef 167: " .. tostring(err)
    end
    local code = row_field(type(res) == "table" and type(res.rows) == "table" and res.rows[1] or nil, "code")
    if type(code) ~= "string" or code == "" then
        return nil, "could not read QueryRef 167: query is not installed"
    end
    local rok, rres, rerr = pcall(H.altquery_sync, database, code, {
        REPORT_ID = id,
        REV_ID = rev,
    })
    if not rok then
        return nil, "could not read " .. report_where(id, rev) .. ": " .. tostring(rres)
    end
    if rerr then
        return nil, "could not read " .. report_where(id, rev) .. ": " .. tostring(rerr)
    end
    local row = type(rres) == "table" and type(rres.rows) == "table" and rres.rows[1] or nil
    if type(row) ~= "table" then
        return nil, report_where(id, rev) .. " was not found"
    end
    local cell = row_field(row, "report")
    if cell == nil or cell == "" then
        return nil, report_where(id, rev) .. " has no definition"
    end
    local decoded
    if type(cell) == "table" then
        decoded = cell
    elseif type(cell) == "string" then
        if cell:match("^%s*$") then
            return nil, report_where(id, rev) .. " has no definition"
        end
        local parsed, jerr = json_decode(cell)
        if jerr then
            return nil, "definition is not valid JSON"
        end
        decoded = parsed
    else
        return nil, report_where(id, rev) .. " has no definition"
    end
    if type(decoded) ~= "table" then
        return nil, report_where(id, rev) .. " has no definition"
    end
    return fill_band_datasources(decoded), nil
end

local function compose()
    load_shape_rows()
    local stage = P.stage or "compose"
    if stage ~= "compose" and stage ~= "html" then
        return fail("stage " .. tostring(stage) .. " is not available yet; use compose or html")
    end
    local def, def_err = resolve_definition()
    if def_err then
        return fail(def_err)
    end
    if type(def) ~= "table" then
        return fail("definition must be a JSON object")
    end
    if def.format ~= 1 then
        return fail("definition format " .. tostring(def.format) .. " is not supported; expected 1")
    end
    local sources = type(P.sources) == "table" and P.sources or {}
    local rfont = type(def.font) == "table" and def.font or {}

    -- Phase 22: one generation instant for every item; timezone User | UTC | Explicit.
    local HY = type(P._hydrogen) == "table" and P._hydrogen or {}
    local def_zone = type(def.defaultZone) == "string" and def.defaultZone ~= "" and def.defaultZone or nil
    local gen_now
    local function now()
        if gen_now == nil then
            local n = tonumber(P.now)
            if n == nil and H and H.system and H.system.now then n = tonumber(H.system.now()) end
            gen_now = math.floor(n or os.time())
        end
        return gen_now
    end
    local zone_logged = {}
    local function zone_log(level, message)
        if zone_logged[message] then return end
        zone_logged[message] = true
        add_log(level, message)
    end
    -- Phase 22a: IANA zones resolve through luatz (group luatz) on host tzdata.
    -- Config hooks (env, defaults hardcoded): REPORTS_TZ_ZONEINFO_PATH (reports.tz.zoneinfo_path),
    -- REPORTS_TZ_DEFAULT_ZONE (reports.tz.default_zone, used when the report has no defaultZone).
    local function cfg_env(name)
        local ok, v = pcall(function() return os.getenv and os.getenv(name) end)
        return ok and type(v) == "string" and v ~= "" and v or nil
    end
    local TZ_PATH = cfg_env("REPORTS_TZ_ZONEINFO_PATH") or "/usr/share/zoneinfo"
    def_zone = def_zone or cfg_env("REPORTS_TZ_DEFAULT_ZONE")
    local tzc
    local function iana_offset(zone, instant)
        if not zone:match("^[%w_+%-/]+$") or zone:find("..", 1, true) or zone:sub(1, 1) == "/" then
            zone_log("warn", "zone " .. zone .. " is not an IANA name; printed UTC")
            return nil
        end
        if tzc == nil then
            local ok, m = pcall(require, "luatz.tzcache")
            tzc = ok and type(m) == "table" and m or false
            if not tzc then
                zone_log("warn", "luatz is not available (" .. tostring(m) .. "); printed UTC")
            elseif tzc.set_base_path then
                tzc.set_base_path(TZ_PATH)
            end
        end
        if not tzc then return nil end
        if io == nil then
            zone_log("warn", "zone " .. zone .. " needs read access to " .. TZ_PATH .. " (io is sandboxed); printed UTC")
            return nil
        end
        local ok, tz = pcall(tzc.get_tz, zone)
        if not ok or type(tz) ~= "table" then
            zone_log("warn", "zone " .. zone .. " is unknown or unreadable under " .. TZ_PATH .. "; printed UTC")
            return nil
        end
        local ok2, info = pcall(tz.find_current, tz, instant)
        if not ok2 or type(info) ~= "table" then
            zone_log("warn", "zone " .. zone .. " has no offset at " .. tostring(instant) .. "; printed UTC")
            return nil
        end
        return info.gmtoff
    end
    local function zone_offset(item, instant)
        local mode = item.timezone or "User"
        if mode == "UTC" then return 0 end
        local zone
        if mode == "Explicit" then
            zone = type(item.zone) == "string" and item.zone ~= "" and item.zone or nil
            if not zone then
                zone_log("info", "item " .. tostring(item.id) .. " has Explicit timezone with no zone; printed UTC")
                return 0
            end
        else
            -- The caller's own claim keeps its login offset (tzoffset drift accepted).
            local off = tonumber(HY.tzoffset)
            if type(HY.tz) == "string" and HY.tz ~= "" and off then return math.floor(off * 60) end
            zone = type(HY.tz) == "string" and HY.tz ~= "" and HY.tz or def_zone
            if not zone then
                zone_log("info", "no user timezone or report default zone; printed UTC")
                return 0
            end
        end
        return iana_offset(zone, instant) or 0
    end
    local DT_DEFAULT = "yyyy-MM-dd HH:mm"
    local warned_user = false

    local declared = {}
    for _, ds in ipairs(def.datasources or {}) do
        if type(ds) == "table" and ds.name then declared[ds.name] = ds end
    end

    -- Sort bands into roles, keeping designer order inside each role.
    local rh, ph, gh, details, gf, pf, rf = {}, {}, {}, {}, {}, {}, {}
    local roles = { [12] = rh, [13] = ph, [14] = gh, [15] = details, [16] = gf, [17] = pf, [18] = rf }
    for _, b in ipairs(def.bands or {}) do
        local list = type(b) == "table" and roles[b.typeKey]
        if list then
            list[#list + 1] = b
        else
            add_log("warn", "band " .. tostring(type(b) == "table" and b.id) .. " has unknown typeKey "
                .. tostring(type(b) == "table" and b.typeKey) .. "; skipped")
        end
    end

    -- Pair each group header with its footer: same suffix (gh2/gf2), else nesting order.
    local used = {}
    local function suffix(id) return tostring(id or ""):match("^g[hf](.*)$") end
    local levels = {}
    for i, h in ipairs(gh) do
        local foot
        local hs = suffix(h.id)
        if hs then
            for j, f in ipairs(gf) do
                if not used[j] and suffix(f.id) == hs then foot = j break end
            end
        end
        if not foot then
            local j = #gf - i + 1
            if gf[j] and not used[j] then foot = j end
        end
        if foot then used[foot] = true end
        if h.field == nil or h.field == "" then
            add_log("warn", "group " .. tostring(h.id) .. " has no field; it wraps every row once")
        end
        levels[i] = { header = h, footer = foot and gf[foot] or nil }
    end
    for j, f in ipairs(gf) do
        if not used[j] then
            add_log("warn", "group footer " .. tostring(f.id) .. " has no group header; skipped")
        end
    end

    -- Rows: the first detail band with a datasource drives the report.
    local rows, driver
    local function reporting_types(collection)
        local doc = collection
        if type(doc) == "string" then
            if doc == "" then return {} end
            local parsed, jerr = json_decode(doc)
            if jerr or type(parsed) ~= "table" then return {} end
            doc = parsed
        end
        if type(doc) ~= "table" then return {} end
        if doc.reporting == nil and type(doc[1]) == "table" then
            doc = doc[1]
        end
        local rep = type(doc) == "table" and doc.reporting or nil
        if type(rep) ~= "table" or type(rep.parameters) ~= "table" then
            return {}
        end
        local types = {}
        for _, p in ipairs(rep.parameters) do
            if type(p) == "table" and type(p.name) == "string" and p.kind ~= "calculated" then
                types[p.name] = tostring(p.type or "string"):lower()
            end
        end
        return types
    end

    local function definition_defaults()
        local map = {}
        local list = def.parameters
        if type(list) ~= "table" then return map end
        for _, p in ipairs(list) do
            if type(p) == "table" and type(p.name) == "string" then
                map[p.name] = p.default
            end
        end
        return map
    end

    local function coerce_bind(name, value, types)
        if value == nil then return nil end
        local typ = types[name]
        if typ == nil then return value end
        if typ == "integer" or typ == "int" then
            local n = tonumber(value)
            if n == nil or n ~= n or n == math.huge or n == -math.huge then return value end
            return math.floor(n)
        end
        if typ == "float" or typ == "number" then
            local n = tonumber(value)
            if n == nil or n ~= n or n == math.huge or n == -math.huge then return value end
            return n
        end
        if typ == "boolean" or typ == "bool" then
            return value == true or value == 1 or value == "1" or value == "true"
        end
        if typ == "string" or typ == "text" or typ == "memo" or typ == "date" or typ == "datetime" then
            return plain(value)
        end
        return value
    end

    local function resolve_raw(raw, overrides, defaults)
        if type(raw) ~= "table" then return raw end
        if raw.parameter ~= nil then
            local pname = tostring(raw.parameter)
            if overrides[pname] ~= nil then return overrides[pname] end
            if defaults[pname] ~= nil then return defaults[pname] end
            return nil
        end
        if raw.literal ~= nil then return raw.literal end
        return nil
    end

    local function build_binds(ds, types)
        local overrides = type(P.parameters) == "table" and P.parameters or {}
        local defaults = definition_defaults()
        local stored = type(ds.params) == "table" and ds.params or {}
        local binds = {}
        local missing = false
        for name, raw in pairs(stored) do
            if type(name) == "string" then
                local value
                if overrides[name] ~= nil then
                    value = overrides[name]
                else
                    value = resolve_raw(raw, overrides, defaults)
                end
                value = coerce_bind(name, value, types)
                if value == nil then
                    missing = true
                    add_log("warn", "datasource " .. tostring(ds.name) .. " parameter " .. name
                        .. " is not set; no rows")
                else
                    binds[name] = value
                end
            end
        end
        return binds, missing
    end

    local function session_database()
        local hy = type(P._hydrogen) == "table" and P._hydrogen or {}
        local database = hy.database
        if type(database) ~= "string" or database == "" then return nil end
        return database
    end

    local function unconfigured(message)
        local s = tostring(message or "")
        if s:find("database_name is empty", 1, true) then return true end
        if s:find("no database specified", 1, true) then return true end
        if s:find("not found", 1, true) and s:find("database", 1, true) then return true end
        return false
    end

    local function query_rows(ds, name)
        local ref = tonumber(ds.queryRef)
        if ref == nil or ref ~= math.floor(ref) or ref < 1 then
            add_log("warn", "datasource " .. tostring(name) .. " has no queryRef; no rows")
            return {}
        end
        ref = math.floor(ref)
        local database = session_database()
        if not database then
            add_log("warn", "datasource " .. tostring(name) .. " has no session database; no rows")
            return {}
        end
        if type(H) ~= "table" or type(H.altquery_sync) ~= "function" then
            add_log("warn", "datasource " .. tostring(name) .. " query " .. tostring(ref)
                .. " failed: H.altquery_sync is missing; no rows")
            return {}
        end
        -- Type 3 is SQL - Reporting. Type 1 does not find QueryRef 174.
        local ok, res, err = pcall(H.altquery_sync, database, [[
            SELECT code, collection FROM ${SCHEMA}queries
            WHERE query_ref = :QREF AND query_type_a28 = 3
        ]], { QREF = ref })
        if not ok then
            err = res
            res = nil
        end
        if err then
            if unconfigured(err) then
                add_log("warn", "datasource " .. tostring(name) .. " database " .. database
                    .. " is not configured; no rows")
            else
                add_log("warn", "datasource " .. tostring(name) .. " query " .. tostring(ref)
                    .. " failed: " .. tostring(err) .. "; no rows")
            end
            return {}
        end
        local row = type(res) == "table" and type(res.rows) == "table" and res.rows[1] or nil
        local code = row_field(row, "code")
        if type(code) ~= "string" or code == "" then
            add_log("warn", "datasource " .. tostring(name) .. " query " .. tostring(ref)
                .. " is not installed; no rows")
            return {}
        end
        local binds, missing = build_binds(ds, reporting_types(row_field(row, "collection")))
        if missing then return {} end
        local rok, rres, rerr = pcall(H.altquery_sync, database, code, binds)
        if not rok then
            rerr = rres
            rres = nil
        end
        if rerr then
            if unconfigured(rerr) then
                add_log("warn", "datasource " .. tostring(name) .. " database " .. database
                    .. " is not configured; no rows")
            else
                add_log("warn", "datasource " .. tostring(name) .. " query " .. tostring(ref)
                    .. " failed: " .. tostring(rerr) .. "; no rows")
            end
            return {}
        end
        local got = type(rres) == "table" and type(rres.rows) == "table" and rres.rows or {}
        add_log("info", "datasource " .. tostring(name) .. " ran query " .. tostring(ref)
            .. "; " .. tostring(#got) .. " rows")
        return got
    end

    local function rows_for(name, band_id)
        if not declared[name] then
            add_log("warn", "detail " .. tostring(band_id) .. " names datasource " .. tostring(name)
                .. " which is not declared; no rows")
            return {}
        end
        local r = sources[name]
        if type(r) == "table" then
            return r
        end
        local ds = declared[name]
        if type(ds) == "table" and ds.kind == "query" then
            return query_rows(ds, name)
        end
        add_log("warn", "datasource " .. tostring(name) .. " has no array in sources; no rows")
        return {}
    end
    for _, d in ipairs(details) do
        if d.datasource ~= nil and d.datasource ~= "" then
            if not driver then
                driver = d.datasource
                rows = rows_for(driver, d.id)
            elseif d.datasource ~= driver then
                add_log("warn", "detail " .. tostring(d.id) .. " uses datasource " .. tostring(d.datasource)
                    .. "; rows come from " .. driver .. " in this version")
            end
        end
    end
    if not driver then
        rows = { {} } -- static report: each band once
    end

    -- Parameter items (63): "NAME" (first datasource with it) or "ds.NAME".
    local warned_param = {}
    local function param_value(ref)
        if ref == nil or ref == "" then return nil end
        ref = tostring(ref)
        local ds_name, pname = ref:match("^([^%.]+)%.(.+)$")
        for _, ds in ipairs(type(def.datasources) == "table" and def.datasources or {}) do
            if type(ds) == "table" and type(ds.params) == "table"
                and (ds_name == nil or ds.name == ds_name) then
                local v = ds.params[pname or ref]
                if v ~= nil then return v end
            end
        end
        if not warned_param[ref] then
            warned_param[ref] = true
            add_log("warn", "parameter " .. ref .. " is not set on any datasource; printed blank")
        end
        return nil
    end


    -- Phase 38 and 39. ctx.aggregates is the scope being emitted, keyed
    -- aggregates[op][field]. Report and group scopes are filled before
    -- paging. Page totals are filled after the page list exists, from
    -- the detail rows placed on that page. A detail band has no
    -- aggregates. The map is not copied into the emitted JSON.
    local ctx
    local report_aggs
    local group_aggs
    local AGG_OPS = { count = true, filled = true, unique = true, sum = true,
        avg = true, mean = true, weighted = true, median = true, mode = true,
        min = true, max = true, first = true, last = true }
    local agg_warned = {}
    local function agg_warn(item, band_id, message)
        local k = tostring(band_id) .. "/" .. tostring(item.id) .. "/" .. message
        if agg_warned[k] then return end
        agg_warned[k] = true
        add_log("warn", "item " .. tostring(item.id) .. " in band " .. tostring(band_id) .. " " .. message)
    end
    local function numeric_value(v)
        if type(v) ~= "number" then return nil end
        if v ~= v or v == math.huge or v == -math.huge then return nil end
        return v
    end
    local function unique_key(v)
        if type(v) == "number" then
            local n = numeric_value(v)
            if not n then return nil end
            return "n:" .. plain_number(n)
        end
        return "s:" .. plain(v)
    end
    local function format_aggregate(rawv, item, band_id)
        if type(item) == "table" and item.typeKey == 38
            and (item.op == "min" or item.op == "max"
                or item.op == "first" or item.op == "last") then
            return format_value(rawv, 24, item.mask, item.id, band_id)
        end
        if type(rawv) == "string" then return plain(rawv) end
        return format_value(rawv, 21, item.mask, item.id, band_id)
    end
    local function weight_name(item)
        if type(item) == "table" and item.op == "weighted" and type(item.weight) == "string" then
            return item.weight
        end
        return ""
    end
    local function store_key(item)
        local w = weight_name(item)
        if item.op == "weighted" and w ~= "" then return item.field .. "\0" .. w end
        return item.field
    end
    local function agg_raw(aggs, item)
        if type(aggs) ~= "table" or type(item) ~= "table" then return nil end
        local bucket = aggs[item.op]
        if type(bucket) ~= "table" then return nil end
        if item.op == "weighted" and weight_name(item) ~= "" then
            return bucket[store_key(item)]
        end
        return bucket[item.field]
    end
    local function reduce_numbers(field, list)
        local total, n, skipped = 0, 0, false
        for _, row in ipairs(list) do
            local v = type(row) == "table" and row[field] or nil
            if v ~= nil then
                local num = numeric_value(v)
                if num then
                    total = total + num
                    n = n + 1
                else
                    skipped = true
                end
            end
        end
        return total, n, skipped
    end
    local function mode_less(a, b)
        local ta, tb = type(a), type(b)
        if ta == tb then
            if ta == "number" or ta == "string" then return a < b end
            return false
        end
        if ta == "number" then return true end
        if tb == "number" then return false end
        return ta < tb
    end
    local function reduce_op(op, field, list, weight)
        if op == "count" then return #list, nil end
        if op == "filled" then
            local n = 0
            for _, row in ipairs(list) do
                if type(row) == "table" and row[field] ~= nil then n = n + 1 end
            end
            return n, nil
        end
        if op == "unique" then
            local seen, n = {}, 0
            for _, row in ipairs(list) do
                local v = type(row) == "table" and row[field] or nil
                if v ~= nil then
                    local uk = unique_key(v)
                    if uk and not seen[uk] then
                        seen[uk] = true
                        n = n + 1
                    end
                end
            end
            return n, nil
        end
        if op == "sum" or op == "avg" or op == "mean" then
            local total, n, skipped = reduce_numbers(field, list)
            local flags = skipped and { skip = true } or nil
            if n == 0 then return nil, flags end
            if op == "sum" then return total, flags end
            return total / n, flags
        end
        if op == "weighted" then
            local wname = type(weight) == "string" and weight or ""
            if wname == "" then return nil, { missing = true } end
            local num, den, pairs, skipped = 0, 0, false, false
            for _, row in ipairs(list) do
                if type(row) == "table" then
                    local v, w = row[field], row[wname]
                    if v ~= nil and w ~= nil then
                        local nv, nw = numeric_value(v), numeric_value(w)
                        if nv and nw then
                            num = num + nv * nw
                            den = den + nw
                            pairs = true
                        else
                            skipped = true
                        end
                    elseif v ~= nil and not numeric_value(v) then
                        skipped = true
                    end
                end
            end
            local flags = nil
            if skipped then flags = { skip = true } end
            if not pairs then
                flags = flags or {}
                flags.none = true
                return nil, flags
            end
            if den == 0 then
                flags = flags or {}
                flags.zero = true
                return nil, flags
            end
            return num / den, flags
        end
        if op == "median" then
            local nums, skipped = {}, false
            for _, row in ipairs(list) do
                local v = type(row) == "table" and row[field] or nil
                if v ~= nil then
                    local num = numeric_value(v)
                    if num then nums[#nums + 1] = num else skipped = true end
                end
            end
            local flags = skipped and { skip = true } or nil
            if #nums == 0 then return nil, flags end
            table.sort(nums)
            local count = #nums
            local mid = math.floor(count / 2)
            if count % 2 == 1 then return nums[mid + 1], flags end
            return (nums[mid] + nums[mid + 1]) / 2, flags
        end
        if op == "mode" then
            local counts, values, order, bad = {}, {}, {}, false
            for _, row in ipairs(list) do
                local v = type(row) == "table" and row[field] or nil
                if v ~= nil then
                    local key, stored
                    if type(v) == "number" then
                        local num = numeric_value(v)
                        if num then
                            key = "n:" .. plain_number(num)
                            stored = num
                        else
                            bad = true
                        end
                    elseif type(v) == "string" then
                        key = "s:" .. v
                        stored = v
                    else
                        bad = true
                    end
                    if key then
                        if not counts[key] then
                            counts[key] = 0
                            values[key] = stored
                            order[#order + 1] = key
                        end
                        counts[key] = counts[key] + 1
                    end
                end
            end
            local flags = bad and { bad = true } or nil
            if #order == 0 then return nil, flags end
            local best, keys = 0, {}
            for _, key in ipairs(order) do
                local n = counts[key]
                if n > best then
                    best = n
                    keys = { key }
                elseif n == best then
                    keys[#keys + 1] = key
                end
            end
            local winner = keys[1]
            for i = 2, #keys do
                if mode_less(values[keys[i]], values[winner]) then winner = keys[i] end
            end
            if #keys > 1 then
                flags = flags or {}
                flags.tie = true
            end
            return values[winner], flags
        end
        if op == "min" or op == "max" then
            local kind, best, other = nil, nil, false
            for _, row in ipairs(list) do
                local v = type(row) == "table" and row[field] or nil
                if v ~= nil then
                    local stored, k
                    if type(v) == "number" then
                        stored = numeric_value(v)
                        k = stored and "number" or nil
                    elseif type(v) == "string" then
                        stored, k = v, "string"
                    end
                    if not k then
                        other = true
                    elseif not kind then
                        kind, best = k, stored
                    elseif k ~= kind then
                        other = true
                    elseif (op == "min" and stored < best) or (op == "max" and best < stored) then
                        best = stored
                    end
                end
            end
            local flags = other and { other = true } or nil
            return best, flags
        end
        if op == "first" or op == "last" then
            local found = nil
            for _, row in ipairs(list) do
                local v = type(row) == "table" and row[field] or nil
                if v ~= nil then
                    if op == "first" then
                        if found == nil then found = v end
                    else
                        found = v
                    end
                end
            end
            return found, nil
        end
        return nil, nil
    end
    local function collect_aggs(bands)
        local specs = {}
        for _, band in ipairs(bands) do
            if type(band) == "table" then
                for _, item in ipairs(band.items or {}) do
                    if type(item) == "table" and (item.typeKey == 35 or item.typeKey == 34 or item.typeKey == 38)
                        and (type(item["function"]) ~= "string" or item["function"] == "")
                        and type(item.field) == "string" and item.field ~= ""
                        and AGG_OPS[item.op] then
                        specs[#specs + 1] = { item = item, band_id = band.id }
                    end
                end
            end
        end
        return specs
    end
    local function note_flags(item, band_id, flags)
        if type(flags) ~= "table" then return end
        if flags.skip then
            agg_warn(item, band_id, item.op .. " skipped a value that is not a number")
        end
        if flags.missing then
            agg_warn(item, band_id, "has no weight; printed blank")
        end
        if flags.none then
            agg_warn(item, band_id, "weighted has no numeric pair; printed blank")
        end
        if flags.zero then
            agg_warn(item, band_id, "weighted weights sum to zero; printed blank")
        end
        if flags.tie then
            agg_warn(item, band_id, "mode tied; printed the smallest value")
        end
        if flags.bad then
            agg_warn(item, band_id, "mode skipped a value that is not text or a number")
        end
        if flags.other then
            agg_warn(item, band_id, item.op .. " skipped a value of the other kind")
        end
    end
    local function fill_aggs(specs, list)
        local aggs, have, value, flags = {}, {}, {}, {}
        for _, spec in ipairs(specs) do
            local item = spec.item
            local op, field = item.op, item.field
            local id = op .. "\0" .. field .. "\0" .. weight_name(item)
            aggs[op] = aggs[op] or {}
            if not have[id] then
                have[id] = true
                value[id], flags[id] = reduce_op(op, field, list, item.weight)
            end
            aggs[op][store_key(item)] = value[id]
            if op == "weighted" then aggs[op][field] = value[id] end
            note_flags(item, spec.band_id, flags[id])
        end
        return aggs
    end

    local warned_field = {}
    local function item_out(item, row, band_id, band_tk)
        local tk = item.typeKey
        local text
        if tk == 0 then
            text = item.text ~= nil and plain(item.text) or ""
        elseif FIELD_KEYS[tk] then
            if item.field == nil or item.field == "" then
                local k = tostring(band_id) .. "/" .. tostring(item.id)
                if not warned_field[k] then
                    warned_field[k] = true
                    add_log("warn", "item " .. tostring(item.id) .. " in band " .. tostring(band_id)
                        .. " has no field; printed blank")
                end
                text = ""
            else
                local v = row and row[item.field]
                -- Unix-number datetimes are instants and take the item timezone when set;
                -- civil strings never shift.
                if tk == 24 and item.timezone and type(v) == "number" then v = v + zone_offset(item, v) end
                text = format_value(v, tk, item.mask, item.id, band_id)
            end
        elseif tk == 5 then
            local s = now()
            local m = item.mask
            if m == nil or m == "" then m = DT_DEFAULT end
            text = format_value(s + zone_offset(item, s), 24, m, item.id, band_id)
        elseif tk == 6 then
            local u = HY.username
            if u == nil or u == "" then
                if not warned_user then
                    warned_user = true
                    add_log("warn", "item " .. tostring(item.id) .. " in band " .. tostring(band_id)
                        .. " is System user but the caller has no username; printed blank")
                end
                text = ""
            else
                text = plain(u)
            end
        elseif PAGE_KEYS[tk] then
            text = "" -- filled after the page list exists
        elseif tk == 63 then
            local v = param_value(item.param)
            local ptk = type(v) == "number" and 21 or 20
            text = format_value(v, ptk, item.mask, item.id, band_id)
        elseif tk == 34 or tk == 35 or tk == 38 then
            local fname = item["function"]
            if type(fname) == "string" and fname ~= "" then
                agg_warn(item, band_id, "names function " .. fname
                    .. "; function totals are not available yet; printed blank")
                text = ""
            elseif item.field == nil or item.field == "" then
                agg_warn(item, band_id, "has no field; printed blank")
                text = ""
            elseif not AGG_OPS[item.op] then
                agg_warn(item, band_id, "has op " .. tostring(item.op)
                    .. " which this version does not reduce; printed blank")
                text = ""
            elseif band_tk == 13 or band_tk == 17 then
                text = "" -- filled after the page list exists
            elseif type(ctx) ~= "table" or type(ctx.aggregates) ~= "table" then
                agg_warn(item, band_id, "is not on a group, page, or report header or footer; printed blank")
                text = ""
            else
                text = format_aggregate(agg_raw(ctx.aggregates, item), item, band_id)
            end
        elseif tk == 1 then
            local v = item.value
            if v == nil then
                text = ""
            elseif type(v) ~= "number" or v ~= v or v == math.huge or v == -math.huge then
                format_warn(item.id, band_id, "value " .. raw(v) .. " is not a number; printed blank")
                text = ""
            else
                text = format_value(v, 21, item.mask, item.id, band_id)
            end
        elseif tk == 4 then
            local v = item.value
            if v == nil then
                text = ""
            else
                text = format_value(v, 24, item.mask, item.id, band_id)
            end
        elseif tk == 3 then
            text = item.text ~= nil and plain(item.text) or ""
        elseif tk == 2 then
            text = ""
        elseif tk == 25 then
            text = ""
        elseif tk == 43 or tk == 45 then
            text = ""
        elseif LATER_KEYS[tk] then
            text = ""
        else
            add_log("warn", "item " .. tostring(item.id) .. " in band " .. tostring(band_id)
                .. " has unknown typeKey " .. tostring(tk) .. "; skipped")
            return nil
        end
        local f = type(item.font) == "table" and item.font or {}
        local o = {
            id = item.id,
            typeKey = tk,
            text = text,
            align = item.align or "Left",
            font = {
                family = f.family or rfont.family,
                size = f.size or rfont.size,
                style = f.style or rfont.style,
            },
            box = item.box,
        }
        if PAGE_KEYS[tk] then page_src[o] = item end
        if tk == 2 then
            o.image = item.image
            o.dpi = item.dpi
            o.fit = item.fit
            o.fitAlign = item.fitAlign
            o.fitValign = item.fitValign
            o.flip = item.flip
            o.opacity = item.opacity
            o.grayscale = item.grayscale
            o.invert = item.invert
            o.rotation = item.rotation
        end
        if tk == 25 then
            o.kind = item.kind
            o.side = item.side
            o.corners = item.corners
            o.strokeWidth = item.strokeWidth
            o.strokeStyle = item.strokeStyle
            o.strokeColor = item.strokeColor
            o.fillStyle = item.fillStyle
            o.fillPaint = item.fillPaint
            o.rotation = item.rotation
        end
        if tk == 43 or tk == 45 then
            local bc = type(item.barcode) == "table" and item.barcode or {}
            local copy = {}
            for k, v in pairs(bc) do copy[k] = v end
            if tk == 45 then
                if item.field == nil or item.field == "" then
                    local fk = tostring(band_id) .. "/" .. tostring(item.id)
                    if not warned_field[fk] then
                        warned_field[fk] = true
                        add_log("warn", "item " .. tostring(item.id) .. " in band " .. tostring(band_id)
                            .. " has no field; printed blank")
                    end
                    copy.data = ""
                else
                    copy.data = plain(row and row[item.field])
                end
            end
            o.barcode = copy
            o.dpi = item.dpi
            o.fitAlign = item.fitAlign
            o.fitValign = item.fitValign
            o.rotation = item.rotation
        end
        return o
    end

    -- Paging (Phase 21): stored band heights against the page content box.
    local UNIT_PT = { ["in"] = 72, mm = 72 / 25.4, cm = 72 / 2.54, pt = 1, px = 0.75 }
    local function to_pt(len)
        if type(len) == "number" then return len end
        if type(len) ~= "string" then return nil end
        local n, u = len:match("^%s*([%d%.]+)%s*(%a+)%s*$")
        n = tonumber(n)
        if not n or not UNIT_PT[u] then return nil end
        return n * UNIT_PT[u]
    end
    local pg = type(def.page) == "table" and def.page or {}
    local mg = type(pg.margin) == "table" and pg.margin or {}
    local box_h = to_pt(pg.height)
    if box_h then box_h = box_h - (to_pt(mg.top) or 0) - (to_pt(mg.bottom) or 0) end
    if not box_h or box_h <= 0 then
        add_log("warn", "page height " .. tostring(pg.height) .. " leaves no content box; no page breaks")
        box_h = math.huge
    end
    local warned_height = {}
    local function stored_h(band)
        local h = to_pt(band.height)
        if h then return h end
        if not warned_height[band] then
            warned_height[band] = true
            add_log("warn", "band " .. tostring(band.id) .. " height " .. tostring(band.height)
                .. " does not parse; counted as 0")
        end
        return 0
    end
    local layout_cache = {}
    local function measure_band(band, row, aggs)
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
    local function h_of(band, row, aggs)
        local measured = measure_band(band, row, aggs)
        if measured.reflow then return measured.usedPt end
        return stored_h(band)
    end
    local EPS = 1e-6

    page_detail = {}
    page_agg_src = {}
    local pages, out, filled, body_start = {}, nil, 0, 0
    local function note_detail(row)
        local list = page_detail[#pages]
        if not list or row == nil or list[#list] == row then return end
        list[#list + 1] = row
    end
    local function clone_out(o)
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
    local function emit(band, row, aggs)
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
                if lay.grown then copy.font.family = "Helvetica" end
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
    local function emit_all(list, row, aggs)
        for _, b in ipairs(list) do emit(b, row, aggs) end
    end
    local function sum_at(list, row, aggs)
        local s = 0
        for _, b in ipairs(list) do s = s + h_of(b, row, aggs) end
        return s
    end

    local first = rows[1]
    local function open_page(row)
        out, filled = {}, 0
        pages[#pages + 1] = { bands = out }
        page_detail[#pages] = {}
        if #pages == 1 then emit_all(rh, first, report_aggs) end
        emit_all(ph, row, nil)
        body_start = filled
    end
    -- Make room for `need` before placing; break the page if the body already has bands.
    local function ensure(need, row, last, what)
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


    local function key(row, lv)
        local f = levels[lv].header.field
        if f == nil or f == "" or type(row) ~= "table" then return "" end
        return plain(row[f])
    end
    local function row_path(row, lv)
        local parts = {}
        for i = 1, lv do parts[#parts + 1] = key(row, i) end
        return table.concat(parts, "\0")
    end
    local span_lists = {}
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
    local span_at = {}
    group_aggs = function(lv, row)
        local path = row_path(row, lv)
        local spans = span_lists[lv] or {}
        local i = span_at[lv] or 1
        while spans[i] and spans[i].path ~= path do i = i + 1 end
        if spans[i] then span_at[lv] = i end
        return spans[i] and spans[i].aggs or {}
    end
    local report_specs = collect_aggs(rh)
    for _, spec in ipairs(collect_aggs(rf)) do report_specs[#report_specs + 1] = spec end
    report_aggs = fill_aggs(report_specs, rows)

    open_page(first)

    local prev
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
    local last = prev or first
    local rf_h = sum_at(rf, last, report_aggs)
    if rf_h > 0 then ensure(rf_h, last, last, "report footer") end
    emit_all(pf, last, nil)
    emit_all(rf, last, report_aggs)

    -- Page totals (Phase 39): detail rows placed on that page.
    -- The page header and the page footer share one reduction.
    local page_specs = collect_aggs(ph)
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
    local count = #pages
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

    if stage == "html" then
        H.set_result_json({ html = emit_html(def, pages), log = log })
    else
        H.set_result_json({ pages = pages, log = log })
    end
    return 0
end

local ok, res = pcall(compose)
if not ok then
    return fail("compose failed: " .. tostring(res))
end
return res or 0
]==]

-- ----------------------------------------------------------------------------
-- Forward: barcode fit
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
        'Reports.Render barcode fit'                                        AS name,
        [=[
            # Forward Migration ${MIGRATION}: Reports.Render barcode fit

            The rectangle is free. The symbol keeps its aspect.
            contain meets, cover slices, and none uses the symbol size.
            PDF is not emitted.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]====]})

-- ----------------------------------------------------------------------------
-- Reverse: restore the 1471 body
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
        'Restore 1471 Reports.Render body'                                  AS name,
        [=[
            # Reverse Migration ${MIGRATION}: restore 1471 Reports.Render

            Puts the acuranzo_1471.lua body back.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]====]})

return queries
end
