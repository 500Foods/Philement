-- Migration: acuranzo_1445.lua
-- Reports.Render compose applies value format masks (Reporting Phase 20)
--
-- Does not edit acuranzo_1444.lua. Copies that body and routes field
-- items through format_value: number masks (# 0 , . % ( ) $), date
-- masks (yyyy yy mmm mm d dd hh nn ss), null prints blank. Reverse
-- restores the 1444 body.
--
-- luacheck: no max line length
-- luacheck: no unused args
--
-- CHANGELOG
-- 1.0.0 - 2026-11-03 - Value format masks in compose

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "scripts"
cfg.MIGRATION = "1445"
cfg.GROUP_NAME = "Reports"
cfg.SCRIPT_NAME = "Render"

local sql_old = [==[
-- Reports.Render (Reporting Phase 19: compose skeleton)
-- params: { stage = "compose", definition = {...}, sources = { name = [rows] } }
-- Returns the structural intermediate { pages = [ { bands = [...] } ], log = [...] }.
-- Bands: RH(12) PH(13) GH(14) Detail(15) GF(16) PF(17) RF(18), any count.
-- Groups nest in GH order (outermost first). Values print as-is (masks: Phase 20).

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
local LATER_KEYS = {
    [5] = true, [6] = true, [42] = true, [43] = true, [44] = true, [45] = true,
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

local function compose()
    local stage = P.stage or "compose"
    if stage ~= "compose" then
        return fail("stage " .. tostring(stage) .. " is not available yet; use compose")
    end
    local def = P.definition
    if type(def) ~= "table" then
        return fail("definition must be a JSON object")
    end
    if def.format ~= 1 then
        return fail("definition format " .. tostring(def.format) .. " is not supported; expected 1")
    end
    local sources = type(P.sources) == "table" and P.sources or {}
    local rfont = type(def.font) == "table" and def.font or {}

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
                text = plain(row and row[item.field])
            end
        elseif LATER_KEYS[tk] then
            text = ""
        else
            add_log("warn", "item " .. tostring(item.id) .. " in band " .. tostring(band_id)
                .. " has unknown typeKey " .. tostring(tk) .. "; skipped")
            return nil
        end
        local f = type(item.font) == "table" and item.font or {}
        return {
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
    end

    local out = {}
    local function emit(band, row)
        local items = {}
        for _, item in ipairs(band.items or {}) do
            if type(item) == "table" then
                local o = item_out(item, row, band.id)
                if o then items[#items + 1] = o end
            end
        end
        out[#out + 1] = { id = band.id, typeKey = band.typeKey, height = band.height, items = items }
    end
    local function emit_all(list, row)
        for _, b in ipairs(list) do emit(b, row) end
    end

    local first = rows[1]
    emit_all(rh, first)
    emit_all(ph, first)

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
                if levels[lv].footer then emit(levels[lv].footer, prev) end
            end
        end
        for lv = brk, #levels do
            emit(levels[lv].header, row)
        end
        emit_all(details, row)
        prev = row
    end
    if prev ~= nil then
        for lv = #levels, 1, -1 do
            if levels[lv].footer then emit(levels[lv].footer, prev) end
        end
    end

    emit_all(pf, prev or first)
    emit_all(rf, prev or first)

    H.set_result_json({ pages = { { bands = out } }, log = log })
    return 0
end

local ok, res = pcall(compose)
if not ok then
    return fail("compose failed: " .. tostring(res))
end
return res or 0
            ]==]
local sql_new = [==[
-- Reports.Render (Reporting Phase 20: compose with value format)
-- params: { stage = "compose", definition = {...}, sources = { name = [rows] } }
-- Returns the formatted intermediate { pages = [ { bands = [...] } ], log = [...] }.
-- Bands: RH(12) PH(13) GH(14) Detail(15) GF(16) PF(17) RF(18), any count.
-- Groups nest in GH order (outermost first). Field values go through the item mask (Value format); null prints blank.

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
local LATER_KEYS = {
    [5] = true, [6] = true, [42] = true, [43] = true, [44] = true, [45] = true,
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
        return { y = y, m = m, d = d, hh = rem // 3600, nn = rem % 3600 // 60, ss = rem % 60, time = rem ~= 0 }
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

local DATE_TOKENS = { "yyyy", "mmm", "yy", "mm", "dd", "hh", "nn", "ss", "d" }

local function apply_date(t, mask)
    local out, i, found = {}, 1, false
    while i <= #mask do
        local hit
        for _, tok in ipairs(DATE_TOKENS) do
            if mask:sub(i, i + #tok - 1) == tok then hit = tok break end
        end
        if hit == "yyyy" then out[#out + 1] = string.format("%04d", t.y)
        elseif hit == "yy" then out[#out + 1] = string.format("%02d", t.y % 100)
        elseif hit == "mmm" then out[#out + 1] = MONTHS[t.m]
        elseif hit == "mm" then out[#out + 1] = string.format("%02d", t.m)
        elseif hit == "dd" then out[#out + 1] = string.format("%02d", t.d)
        elseif hit == "d" then out[#out + 1] = tostring(t.d)
        elseif hit == "hh" then out[#out + 1] = string.format("%02d", t.hh)
        elseif hit == "nn" then out[#out + 1] = string.format("%02d", t.nn)
        elseif hit == "ss" then out[#out + 1] = string.format("%02d", t.ss)
        else out[#out + 1] = mask:sub(i, i) end
        if hit then found = true end
        i = i + (hit and #hit or 1)
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
            return t.time and apply_date(t, "yyyy-mm-dd hh:nn") or apply_date(t, "yyyy-mm-dd")
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

local function compose()
    local stage = P.stage or "compose"
    if stage ~= "compose" then
        return fail("stage " .. tostring(stage) .. " is not available yet; use compose")
    end
    local def = P.definition
    if type(def) ~= "table" then
        return fail("definition must be a JSON object")
    end
    if def.format ~= 1 then
        return fail("definition format " .. tostring(def.format) .. " is not supported; expected 1")
    end
    local sources = type(P.sources) == "table" and P.sources or {}
    local rfont = type(def.font) == "table" and def.font or {}

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
                text = format_value(row and row[item.field], tk, item.mask, item.id, band_id)
            end
        elseif LATER_KEYS[tk] then
            text = ""
        else
            add_log("warn", "item " .. tostring(item.id) .. " in band " .. tostring(band_id)
                .. " has unknown typeKey " .. tostring(tk) .. "; skipped")
            return nil
        end
        local f = type(item.font) == "table" and item.font or {}
        return {
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
    end

    local out = {}
    local function emit(band, row)
        local items = {}
        for _, item in ipairs(band.items or {}) do
            if type(item) == "table" then
                local o = item_out(item, row, band.id)
                if o then items[#items + 1] = o end
            end
        end
        out[#out + 1] = { id = band.id, typeKey = band.typeKey, height = band.height, items = items }
    end
    local function emit_all(list, row)
        for _, b in ipairs(list) do emit(b, row) end
    end

    local first = rows[1]
    emit_all(rh, first)
    emit_all(ph, first)

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
                if levels[lv].footer then emit(levels[lv].footer, prev) end
            end
        end
        for lv = brk, #levels do
            emit(levels[lv].header, row)
        end
        emit_all(details, row)
        prev = row
    end
    if prev ~= nil then
        for lv = #levels, 1, -1 do
            if levels[lv].footer then emit(levels[lv].footer, prev) end
        end
    end

    emit_all(pf, prev or first)
    emit_all(rf, prev or first)

    H.set_result_json({ pages = { { bands = out } }, log = log })
    return 0
end

local ok, res = pcall(compose)
if not ok then
    return fail("compose failed: " .. tostring(res))
end
return res or 0
            ]==]

-- ----------------------------------------------------------------------------
-- Forward
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
        'Reports.Render applies value format masks'                         AS name,
        [=[
            # Forward Migration ${MIGRATION}: Reports.Render value format

            Field items apply their mask (number and date masks per the
            REPORTING_PLAN Value format locks). Null prints blank. A mask
            that does not parse, or a value that is not a number or date,
            warns once per item and prints the raw value.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})
-- ----------------------------------------------------------------------------
-- Reverse: restore the 1444 body
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
        'Restore 1444 Reports.Render body'                                  AS name,
        [=[
            # Reverse Migration ${MIGRATION}: restore 1444 Reports.Render

            Puts the acuranzo_1444.lua body back. Field values print
            unformatted again (no masks, null as blank).
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})

return queries end
