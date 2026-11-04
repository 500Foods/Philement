-- Migration: acuranzo_1457.lua
-- Reports.Render runs a query datasource (Reporting Phase 29)
--
-- Does not edit acuranzo_1456.lua. Copies that Render body. When
-- sources omits a query datasource, the script loads queries.code
-- where query_ref matches and query_type_a28 = 3, then runs that
-- SQL with H.altquery_sync on the session database. An array in
-- sources still wins. Reverse restores the 1456 body.
--
-- luacheck: no max line length
-- luacheck: no unused args
--
-- CHANGELOG
-- 1.0.0 - 2026-11-04 - Run a query datasource when sources omits its array

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "scripts"
cfg.MIGRATION = "1457"
cfg.GROUP_NAME = "Reports"
cfg.SCRIPT_NAME = "Render"

local sql_old = [==[
-- Reports.Render (Reporting Phase 27: a stored report loads by id)
-- params: { stage = "compose"|"html", definition = {...}, sources = { name = [rows] } }
-- A stored report may send report_id and rev_id instead of definition.
-- The script reads QueryRef 167 through H.altquery_sync on
-- params._hydrogen.database and parses the report text.
-- An inline definition is used when it is a table.
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
local LATER_KEYS = {
    [42] = true, [43] = true, [44] = true, [45] = true,
    [61] = true, [62] = true, [63] = true, [28] = true, [29] = true, [32] = true,
    [34] = true, [35] = true, [38] = true,
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

local function emit_html(def, pages)
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
                css_or(band.height, "0in", "band height"),
                table.concat(items))
        end
        parts[#parts + 1] = string.format(
            '<section style="position:relative;box-sizing:border-box;width:%s;height:%s;padding:%s %s %s %s;break-after:page">%s</section>',
            width, height, top, right, bottom, left, table.concat(bands))
    end
    return table.concat(parts, "\n") .. "\n"
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
    local function rows_for(name, band_id)
        if not declared[name] then
            add_log("warn", "detail " .. tostring(band_id) .. " names datasource " .. tostring(name)
                .. " which is not declared; no rows")
            return {}
        end
        local r = sources[name]
        if type(r) ~= "table" then
            add_log("warn", "datasource " .. tostring(name) .. " has no array in sources; no rows")
            return {}
        end
        return r
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

    local warned_field = {}
    local function item_out(item, row, band_id)
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
    local function h_of(band)
        local h = to_pt(band.height)
        if h then return h end
        if not warned_height[band] then
            warned_height[band] = true
            add_log("warn", "band " .. tostring(band.id) .. " height " .. tostring(band.height)
                .. " does not parse; counted as 0")
        end
        return 0
    end
    local EPS = 1e-6

    local pages, out, filled, body_start = {}, nil, 0, 0
    local function emit(band, row)
        local items = {}
        for _, item in ipairs(band.items or {}) do
            if type(item) == "table" then
                local o = item_out(item, row, band.id)
                if o then items[#items + 1] = o end
            end
        end
        out[#out + 1] = { id = band.id, typeKey = band.typeKey, height = band.height, items = items }
        filled = filled + h_of(band)
    end
    local function emit_all(list, row)
        for _, b in ipairs(list) do emit(b, row) end
    end
    local function sum(list)
        local s = 0
        for _, b in ipairs(list) do s = s + h_of(b) end
        return s
    end
    local pf_h = sum(pf)

    local first = rows[1]
    local function open_page(row)
        out, filled = {}, 0
        pages[#pages + 1] = { bands = out }
        if #pages == 1 then emit_all(rh, first) end
        emit_all(ph, row)
        body_start = filled
    end
    -- Make room for `need` before placing; break the page if the body already has bands.
    local function ensure(need, row, last, what)
        if filled + need + pf_h <= box_h + EPS then return end
        if filled > body_start + EPS then
            emit_all(pf, last)
            open_page(row)
        end
        if filled + need + pf_h > box_h + EPS then
            add_log("warn", what .. " does not fit on an empty page; placed anyway")
        end
    end

    open_page(first)

    local function key(row, lv)
        local f = levels[lv].header.field
        if f == nil or f == "" then return "" end
        return plain(row[f])
    end

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
                    ensure(h_of(f), prev, prev, "group footer " .. tostring(f.id))
                    emit(f, prev)
                end
            end
        end
        if brk <= #levels then
            -- Group headers travel with the first detail row of the group.
            local need, ids = sum(details), {}
            for lv = brk, #levels do
                need = need + h_of(levels[lv].header)
                ids[#ids + 1] = tostring(levels[lv].header.id)
            end
            ensure(need, row, prev or first, "group header " .. table.concat(ids, ", ") .. " with its first detail row")
            for lv = brk, #levels do emit(levels[lv].header, row) end
            emit_all(details, row)
        else
            for _, d in ipairs(details) do
                ensure(h_of(d), row, prev, "detail " .. tostring(d.id))
                emit(d, row)
            end
        end
        prev = row
    end
    if prev ~= nil then
        for lv = #levels, 1, -1 do
            local f = levels[lv].footer
            if f then
                ensure(h_of(f), prev, prev, "group footer " .. tostring(f.id))
                emit(f, prev)
            end
        end
    end

    -- Report footer sits on the last page; it prints after the page footer as before.
    local last = prev or first
    local rf_h = sum(rf)
    if rf_h > 0 then ensure(rf_h, last, last, "report footer") end
    emit_all(pf, last)
    emit_all(rf, last)

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
-- Reports.Render (Reporting Phase 29: a query datasource runs on the server)
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
local LATER_KEYS = {
    [42] = true, [43] = true, [44] = true, [45] = true,
    [61] = true, [62] = true, [63] = true, [28] = true, [29] = true, [32] = true,
    [34] = true, [35] = true, [38] = true,
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

local function emit_html(def, pages)
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
                css_or(band.height, "0in", "band height"),
                table.concat(items))
        end
        parts[#parts + 1] = string.format(
            '<section style="position:relative;box-sizing:border-box;width:%s;height:%s;padding:%s %s %s %s;break-after:page">%s</section>',
            width, height, top, right, bottom, left, table.concat(bands))
    end
    return table.concat(parts, "\n") .. "\n"
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

    local warned_field = {}
    local function item_out(item, row, band_id)
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
    local function h_of(band)
        local h = to_pt(band.height)
        if h then return h end
        if not warned_height[band] then
            warned_height[band] = true
            add_log("warn", "band " .. tostring(band.id) .. " height " .. tostring(band.height)
                .. " does not parse; counted as 0")
        end
        return 0
    end
    local EPS = 1e-6

    local pages, out, filled, body_start = {}, nil, 0, 0
    local function emit(band, row)
        local items = {}
        for _, item in ipairs(band.items or {}) do
            if type(item) == "table" then
                local o = item_out(item, row, band.id)
                if o then items[#items + 1] = o end
            end
        end
        out[#out + 1] = { id = band.id, typeKey = band.typeKey, height = band.height, items = items }
        filled = filled + h_of(band)
    end
    local function emit_all(list, row)
        for _, b in ipairs(list) do emit(b, row) end
    end
    local function sum(list)
        local s = 0
        for _, b in ipairs(list) do s = s + h_of(b) end
        return s
    end
    local pf_h = sum(pf)

    local first = rows[1]
    local function open_page(row)
        out, filled = {}, 0
        pages[#pages + 1] = { bands = out }
        if #pages == 1 then emit_all(rh, first) end
        emit_all(ph, row)
        body_start = filled
    end
    -- Make room for `need` before placing; break the page if the body already has bands.
    local function ensure(need, row, last, what)
        if filled + need + pf_h <= box_h + EPS then return end
        if filled > body_start + EPS then
            emit_all(pf, last)
            open_page(row)
        end
        if filled + need + pf_h > box_h + EPS then
            add_log("warn", what .. " does not fit on an empty page; placed anyway")
        end
    end

    open_page(first)

    local function key(row, lv)
        local f = levels[lv].header.field
        if f == nil or f == "" then return "" end
        return plain(row[f])
    end

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
                    ensure(h_of(f), prev, prev, "group footer " .. tostring(f.id))
                    emit(f, prev)
                end
            end
        end
        if brk <= #levels then
            -- Group headers travel with the first detail row of the group.
            local need, ids = sum(details), {}
            for lv = brk, #levels do
                need = need + h_of(levels[lv].header)
                ids[#ids + 1] = tostring(levels[lv].header.id)
            end
            ensure(need, row, prev or first, "group header " .. table.concat(ids, ", ") .. " with its first detail row")
            for lv = brk, #levels do emit(levels[lv].header, row) end
            emit_all(details, row)
        else
            for _, d in ipairs(details) do
                ensure(h_of(d), row, prev, "detail " .. tostring(d.id))
                emit(d, row)
            end
        end
        prev = row
    end
    if prev ~= nil then
        for lv = #levels, 1, -1 do
            local f = levels[lv].footer
            if f then
                ensure(h_of(f), prev, prev, "group footer " .. tostring(f.id))
                emit(f, prev)
            end
        end
    end

    -- Report footer sits on the last page; it prints after the page footer as before.
    local last = prev or first
    local rf_h = sum(rf)
    if rf_h > 0 then ensure(rf_h, last, last, "report footer") end
    emit_all(pf, last)
    emit_all(rf, last)

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
-- Forward: replace Reports.Render so a query datasource runs on the server
-- ----------------------------------------------------------------------------
table.insert(queries,{sql=[[

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
]] .. sql_new .. [[
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
        'Reports.Render run query datasource'                               AS name,
        [=[
            # Forward Migration ${MIGRATION}: Reports.Render query datasource

            A query datasource with no array in sources loads the
            SQL - Reporting row (query_type_a28 = 3) and runs it on
            the session database. An array in sources is still used
            and the script does not query. An inline definition and
            a stored report_id load stay as they were.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})
-- ----------------------------------------------------------------------------
-- Reverse: restore the 1456 body
-- ----------------------------------------------------------------------------
table.insert(queries,{sql=[[

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
]] .. sql_old .. [[
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
        'Restore 1456 Reports.Render body'                                  AS name,
        [=[
            # Reverse Migration ${MIGRATION}: restore 1456 Reports.Render

            Puts the acuranzo_1456.lua body back. A query datasource
            with no array in sources warns and contributes no rows.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})

return queries end
