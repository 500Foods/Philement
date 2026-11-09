-- Migration: acuranzo_1468.lua
-- Reports.Render.Format (Reporting)
-- Number and datetime masks and format_value. Always loaded.
--
-- One module of the split Reports.Render. Reports.Render (acuranzo_1479.lua)
-- reads this row with H.altquery_sync and runs it with load() in its
-- per-request environment. The row is not invokable and has no MCP access.
-- Reverse deletes the row. QueryRef 175 stays unused. No lookup.
--
-- luacheck: no max line length
-- luacheck: no unused args
--
-- CHANGELOG
-- 1.0.0 - 2026-11-09 - Split from the acuranzo_1479.lua Reports.Render body

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "scripts"
cfg.MIGRATION = "1468"
cfg.GROUP_NAME = "Reports"
cfg.SCRIPT_NAME = "Render.Format"

local sql_new = [==[
-- Reports.Render.Format (Reporting). Always loaded: number and datetime masks, format_value.
-- Loaded by Reports.Render into its per-request environment.
-- Names defined here are shared with Reports.Render and its other modules.

-- Value format (Phase 20). Masks are locale-free; see REPORTING_PLAN Value format.
function decimal_string(a)
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

function plain_number(v)
    if v ~= v or v == math.huge or v == -math.huge then return tostring(v) end
    if math.type(v) == "integer" then return tostring(v) end
    local s = decimal_string(math.abs(v))
    if s:find("%.") then s = s:gsub("0+$", ""):gsub("%.$", "") end
    if v < 0 and s:match("[1-9]") then s = "-" .. s end
    return s
end

function round_digits(int, frac, places)
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

function number_mask(mask)
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

function apply_number(v, spec)
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

MONTHS = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" }

function civil_from_days(z)
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

function parse_datetime(v)
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
MONTHS_LONG = { "January", "February", "March", "April", "May", "June", "July",
    "August", "September", "October", "November", "December" }
DAYS = { "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday", "Sunday" }
DATE_TOKENS = { "yyyy", "yy", "MMMM", "MMM", "MM", "M", "dd", "d", "EEEE", "EEE",
    "HH", "H", "mm", "m", "ss", "s", "SSS", "WW", "W" }

function days_from_civil(y, m, d)
    y = m <= 2 and y - 1 or y
    local era = (y >= 0 and y or y - 399) // 400
    local yoe = y - era * 400
    local doy = (153 * (m > 2 and m - 3 or m + 9) + 2) // 5 + d - 1
    return era * 146097 + yoe * 365 + yoe // 4 - yoe // 100 + doy - 719468
end

-- ISO weekday 1 (Mon) .. 7 (Sun); ISO week: week 1 holds the year's first Thursday.
function iso_weekday(t)
    return (days_from_civil(t.y, t.m, t.d) + 3) % 7 + 1
end
function iso_week(t)
    local days = days_from_civil(t.y, t.m, t.d)
    local thursday = days - iso_weekday(t) + 4
    local ty = civil_from_days(thursday)
    return (thursday - days_from_civil(ty, 1, 1)) // 7 + 1
end

function apply_date(t, mask)
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

function raw(v)
    if type(v) == "number" then return plain_number(v) end
    return plain(v)
end

warned_format = {}
function format_warn(item_id, band_id, message)
    local k = tostring(band_id) .. "/" .. tostring(item_id)
    if warned_format[k] then return end
    warned_format[k] = true
    add_log("warn", "item " .. tostring(item_id) .. " in band " .. tostring(band_id) .. " " .. message)
end

-- typeKey 21 number, 24 datetime, 20 text. Null prints blank and skips the mask.
function format_value(v, tk, mask, item_id, band_id)
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


]==]

-- ----------------------------------------------------------------------------
-- Forward: insert Reports.Render.Format
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
            INSERT INTO ${SCHEMA}scripts (
                group_name,
                script_name,
                script_type,
                schedule,
                next_run,
                last_run_start,
                last_run_end,
                status,
                code,
                summary,
                invokable,
                mcp_access,
                mcp_schema,
                mcp_annotations,
                ${COMMON_FIELDS}
            )
            VALUES (
                '${GROUP_NAME}',
                '${SCRIPT_NAME}',
                1,
                NULL, NULL, NULL, NULL,
                1,
                [==[
]====] .. sql_new .. [====[
                ]==],
                'Reports.Render.Format module (Reporting)',
                0,
                0,
                NULL,
                NULL,
                ${COMMON_VALUES}
            );

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Insert Reports.Render.Format'                                        AS name,
        [=[
            # Forward Migration ${MIGRATION}: Reports.Render.Format

            Inserts group Reports, script Render.Format. Number and datetime masks and format_value. Always loaded.
            Reports.Render loads it. It is not invokable.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]====]})

-- ----------------------------------------------------------------------------
-- Reverse: delete Reports.Render.Format
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
            DELETE FROM ${SCHEMA}scripts
            WHERE group_name = '${GROUP_NAME}'
              AND script_name = '${SCRIPT_NAME}';

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Delete Reports.Render.Format'                                        AS name,
        [=[
            # Reverse Migration ${MIGRATION}: delete Reports.Render.Format

            Deletes the group Reports script Render.Format row.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]====]})

return queries end
