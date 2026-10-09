-- Migration: argent_2046.lua
-- Argent.GenerateSchedule
--
-- luacheck: no max line length
-- luacheck: no unused args
--
-- CHANGELOG
-- 1.0.0 - 2026-10-08 - Expand a schedule into Reserved transactions

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "scripts"
cfg.MIGRATION = "2046"
cfg.GROUP_NAME = "Argent"
if engine == "mysql" then
    cfg.CAST_INTEGER = "signed"
    cfg.CAST_TEXT = "char(255)"
else
    cfg.CAST_INTEGER = cfg.INTEGER
    cfg.CAST_TEXT = cfg.TEXT
end
-- ----------------------------------------------------------------------------
-- Forward
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
                'GenerateSchedule',
                1,
                NULL, NULL, NULL, NULL,
                1,
                [==[
local function leap(y)
    return (y % 4 == 0 and y % 100 ~= 0) or (y % 400 == 0)
end

local MDAYS = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 }

local function dim(y, m)
    if m == 2 and leap(y) then return 29 end
    return MDAYS[m]
end

local function ymd(s)
    if type(s) ~= "string" then return nil end
    local y, m, d = s:match("^(%d%d%d%d)%-(%d%d)%-(%d%d)")
    if not y then return nil end
    return tonumber(y), tonumber(m), tonumber(d)
end

local function ymd_str(y, m, d)
    return string.format("%04d-%02d-%02d", y, m, d)
end

local function real_date(y, m, d)
    if not y or m < 1 or m > 12 or d < 1 then return false end
    local cap = dim(y, m)
    if not cap then return false end
    return d <= cap
end

local function civil_to_days(y, m, d)
    local yy = y
    if m <= 2 then yy = yy - 1 end
    local era = math.floor(yy / 400)
    local yoe = yy - era * 400
    local mp = m + (m > 2 and -3 or 9)
    local doy = math.floor((153 * mp + 2) / 5) + d - 1
    local doe = yoe * 365 + math.floor(yoe / 4) - math.floor(yoe / 100) + doy
    return era * 146097 + doe - 719468
end

local function days_to_civil(z)
    local zz = z + 719468
    local era = math.floor(zz / 146097)
    local doe = zz - era * 146097
    local yoe = math.floor((doe - math.floor(doe / 1460) + math.floor(doe / 36524) - math.floor(doe / 146096)) / 365)
    local y = yoe + era * 400
    local doy = doe - (365 * yoe + math.floor(yoe / 4) - math.floor(yoe / 100))
    local mp = math.floor((5 * doy + 2) / 153)
    local d = doy - math.floor((153 * mp + 2) / 5) + 1
    local m = mp + (mp < 10 and 3 or -9)
    if m <= 2 then y = y + 1 end
    return y, m, d
end

local function add_days(y, m, d, n)
    return days_to_civil(civil_to_days(y, m, d) + n)
end

local function add_months(y, m, d, n, byday)
    local idx = y * 12 + (m - 1) + n
    local ny = math.floor(idx / 12)
    local nm = idx % 12 + 1
    local nd = byday or d
    local cap = dim(ny, nm)
    if nd > cap then nd = cap end
    return ny, nm, nd
end

local function add_years(y, m, d, n, byday)
    local ny = y + n
    local nd = byday or d
    local cap = dim(ny, m)
    if nd > cap then nd = cap end
    return ny, m, nd
end

local function fye(y, m, d, start_month, start_day)
    local function start_of(year)
        local sd = start_day
        local cap = dim(year, start_month)
        if sd > cap then sd = cap end
        return civil_to_days(year, start_month, sd)
    end
    local sy = y
    if civil_to_days(y, m, d) < start_of(y) then sy = y - 1 end
    local ny = sy + 1
    local nd = start_day
    local cap = dim(ny, start_month)
    if nd > cap then nd = cap end
    return days_to_civil(civil_to_days(ny, start_month, nd) - 1)
end

local function date_ok(v)
    if type(v) ~= "string" then return false end
    local y, m, d = v:match("^(%d%d%d%d)%-(%d%d)%-(%d%d)$")
    if not y then return false end
    return real_date(tonumber(y), tonumber(m), tonumber(d))
end

local function day_of(v)
    if type(v) ~= "string" then return nil end
    local d = v:sub(1, 10)
    if not date_ok(d) then return nil end
    return d
end

local function parse_rrule(rrule)
    if type(rrule) ~= "string" then return nil, "rrule is required" end
    if #rrule > 250 then return nil, "rrule is longer than 250 characters" end
    local parts = {}
    if rrule == "" then return nil, "rrule is required" end
    for token in rrule:gmatch("[^;]+") do
        local k, v = token:match("^([A-Z]+)=(.*)$")
        if not k or v == "" or parts[k] then return nil, "rrule is not valid" end
        parts[k] = v
    end
    local freq = parts.FREQ
    if freq ~= "DAILY" and freq ~= "WEEKLY" and freq ~= "MONTHLY" and freq ~= "YEARLY" then
        return nil, "rrule FREQ must be DAILY, WEEKLY, MONTHLY, or YEARLY"
    end
    local interval = 1
    if parts.INTERVAL then
        interval = tonumber(parts.INTERVAL)
        if not interval or interval ~= math.floor(interval) or interval < 1 or interval > 366 then
            return nil, "rrule INTERVAL must be 1 through 366"
        end
    end
    local bymonthday = nil
    if parts.BYMONTHDAY then
        if freq ~= "MONTHLY" and freq ~= "YEARLY" then
            return nil, "rrule BYMONTHDAY is only valid for MONTHLY or YEARLY"
        end
        bymonthday = tonumber(parts.BYMONTHDAY)
        if not bymonthday or bymonthday ~= math.floor(bymonthday) or bymonthday < 1 or bymonthday > 31 then
            return nil, "rrule BYMONTHDAY must be 1 through 31"
        end
    end
    local count = nil
    if parts.COUNT then
        count = tonumber(parts.COUNT)
        if not count or count ~= math.floor(count) or count < 1 or count > 400 then
            return nil, "rrule COUNT must be 1 through 400"
        end
    end
    local until_on = nil
    if parts.UNTIL then
        local u = parts.UNTIL
        local y, m, d = u:match("^(%d%d%d%d)(%d%d)(%d%d)$")
        if not y then y, m, d = u:match("^(%d%d%d%d)%-(%d%d)%-(%d%d)$") end
        if not y then return nil, "rrule UNTIL must be YYYYMMDD or YYYY-MM-DD" end
        y, m, d = tonumber(y), tonumber(m), tonumber(d)
        if not real_date(y, m, d) then return nil, "rrule UNTIL is not a date" end
        until_on = ymd_str(y, m, d)
    end
    local known = { FREQ = true, INTERVAL = true, BYMONTHDAY = true, COUNT = true, UNTIL = true }
    for k in pairs(parts) do
        if not known[k] then return nil, "rrule key is not supported" end
    end
    return {
        freq = freq, interval = interval, bymonthday = bymonthday,
        count = count, until_on = until_on,
    }
end

local function occurrence(rule, y, m, d, i)
    if i == 0 then return y, m, d end
    local step = i * rule.interval
    if rule.freq == "DAILY" then return add_days(y, m, d, step) end
    if rule.freq == "WEEKLY" then return add_days(y, m, d, step * 7) end
    if rule.freq == "MONTHLY" then return add_months(y, m, d, step, rule.bymonthday) end
    return add_years(y, m, d, step, rule.bymonthday)
end

local function expand_dates(rule, anchor, horizon, cap)
    local y, m, d = ymd(anchor)
    if not y or not real_date(y, m, d) then return nil, "anchor_on" end
    if not date_ok(horizon) then return nil, "through" end
    local dates = {}
    local i = 0
    while true do
        if rule.count and i >= rule.count then break end
        local ny, nm, nd = occurrence(rule, y, m, d, i)
        local s = ymd_str(ny, nm, nd)
        if rule.until_on and s > rule.until_on then break end
        if s > horizon then break end
        if #dates > 0 and s <= dates[#dates] then return nil, "rrule" end
        dates[#dates + 1] = s
        if #dates > cap then return nil, "horizon_cap" end
        i = i + 1
        if i > cap + 2 then return nil, "horizon_cap" end
    end
    return dates
end

local function earlier(a, b)
    if not a then return b end
    if not b then return a end
    if a < b then return a end
    return b
end

local function b64(s)
    local chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"
    local out = {}
    local n = #s
    local i = 1
    while i <= n do
        local a = string.byte(s, i)
        local b = (i + 1 <= n) and string.byte(s, i + 1) or nil
        local c = (i + 2 <= n) and string.byte(s, i + 2) or nil
        local v = a * 65536
        if b then v = v + b * 256 end
        if c then v = v + c end
        local c0 = math.floor(v / 262144) % 64
        local c1 = math.floor(v / 4096) % 64
        local c2 = math.floor(v / 64) % 64
        local c3 = v % 64
        out[#out + 1] = chars:sub(c0 + 1, c0 + 1)
        out[#out + 1] = chars:sub(c1 + 1, c1 + 1)
        if b then out[#out + 1] = chars:sub(c2 + 1, c2 + 1) else out[#out + 1] = "=" end
        if c then out[#out + 1] = chars:sub(c3 + 1, c3 + 1) else out[#out + 1] = "=" end
        i = i + 3
    end
    return table.concat(out)
end

local function qrows(res)
    if type(res) ~= "table" then return {} end
    if type(res.rows) == "table" then return res.rows end
    return res
end

local function pick(row, name)
    if type(row) ~= "table" then return nil end
    if row[name] ~= nil then return row[name] end
    local up = string.upper(name)
    if row[up] ~= nil then return row[up] end
    return nil
end

local function fail(code, message, extra)
    local body = extra or {}
    body.ok = false
    body.code = code
    body.message = message or code
    H.set_result_json(body)
    return 0
end

local function ok(body)
    body.ok = true
    H.set_result_json(body)
    return 0
end

local function num(v)
    if type(v) == "number" then return v end
    if type(v) == "string" and v ~= "" then return tonumber(v) end
    return nil
end

local function int(v)
    local n = num(v)
    if not n or n ~= n or n % 1 ~= 0 then return nil end
    return n
end

local function trim(v)
    if type(v) ~= "string" then return nil end
    local s = v:match("^%s*(.-)%s*$")
    if not s or s == "" then return nil end
    return s
end

local function actor_id()
    local h = {}
    if type(params) == "table" and type(params._hydrogen) == "table" then
        h = params._hydrogen
    end
    return tonumber(h.user_id) or tonumber(h.sub) or 0
end

local function idem_key(raw)
    if raw == nil or raw == "" then return "" end
    if type(raw) ~= "string" then return nil, "idempotency_key must be a string" end
    if #raw > 80 then return nil, "idempotency_key is longer than 80 characters" end
    if raw:find('[%c"\\]') then return nil, "idempotency_key contains a forbidden character" end
    return raw
end

local function has_idem(col, key)
    if key == "" then return false end
    if type(col) == "table" then return col.idempotency_key == key end
    if type(col) ~= "string" then return false end
    if col:find('"idempotency_key":"' .. key .. '"', 1, true) then return true end
    if col:find('"idempotency_key": "' .. key .. '"', 1, true) then return true end
    return false
end

local function col_num(col, key)
    if type(col) == "table" then return tonumber(col[key]) end
    if type(col) ~= "string" then return nil end
    return tonumber(col:match('"' .. key .. '":%s*(%-?%d+)'))
end

local function col_str(col, key)
    if type(col) == "table" then
        if type(col[key]) == "string" then return col[key] end
        return nil
    end
    if type(col) ~= "string" then return nil end
    return col:match('"' .. key .. '":%s*"([^"]*)"')
end

local function build_col(days, key)
    if days and key ~= "" then
        return string.format('{"horizon_days":%d,"idempotency_key":"%s"}', days, key)
    end
    if days then return string.format('{"horizon_days":%d}', days) end
    if key ~= "" then return string.format('{"idempotency_key":"%s"}', key) end
    return "{}"
end

local function env_get(name)
    if type(os) ~= "table" or type(os.getenv) ~= "function" then return nil end
    local ok_env, value = pcall(os.getenv, name)
    if not ok_env or type(value) ~= "string" or value == "" then return nil end
    return value
end

local function clip_err(msg)
    msg = tostring(msg or "calendar sync failed")
    msg = msg:gsub("[%z\1-\31]", " ")
    if #msg > 180 then msg = msg:sub(1, 180) end
    return msg
end

local function ics_text(s)
    s = tostring(s or "Argent")
    s = s:gsub("\\", "\\\\")
    s = s:gsub("[\r\n]", " ")
    s = s:gsub(",", "\\,")
    s = s:gsub(";", "\\;")
    if #s > 120 then s = s:sub(1, 120) end
    return s
end

local function ics_body(txn_id, txn_on, summary)
    local compact = (day_of(txn_on) or "1970-01-01"):gsub("-", "")
    local stamp = "19700101T000000Z"
    if type(os) == "table" and type(os.date) == "function" then
        local ok_d, value = pcall(os.date, "!%Y%m%dT%H%M%SZ")
        if ok_d and type(value) == "string" then stamp = value end
    end
    local uid = "argent-txn-" .. tostring(txn_id)
    return table.concat({
        "BEGIN:VCALENDAR",
        "VERSION:2.0",
        "PRODID:-//Philement//Argent//EN",
        "BEGIN:VEVENT",
        "UID:" .. uid,
        "DTSTAMP:" .. stamp,
        "DTSTART;VALUE=DATE:" .. compact,
        "SUMMARY:" .. ics_text(summary),
        "END:VEVENT",
        "END:VCALENDAR",
        "",
    }, "\r\n")
end

local function join_url(calendar_url, txn_id)
    local base = trim(calendar_url)
    if not base then return nil end
    if not base:match("^https?://") then
        local root = env_get("ARGENT_CAL_HTTP")
        if not root then return nil end
        if root:sub(-1) ~= "/" then root = root .. "/" end
        if base:sub(1, 1) == "/" then base = base:sub(2) end
        base = root .. base
    end
    if base:sub(-1) ~= "/" then base = base .. "/" end
    return base .. "argent-txn-" .. tostring(txn_id) .. ".ics"
end

local function put_ics(url, body)
    if type(H) ~= "table" or type(H.http) ~= "table" or type(H.http.request_sync) ~= "function" then
        return nil, "H.http.request_sync is not available"
    end
    local headers = {}
    local user = env_get("ARGENT_CAL_USER")
    if user then
        local pass = env_get("ARGENT_CAL_PASS") or ""
        headers.Authorization = "Basic " .. b64(user .. ":" .. pass)
    end
    local ok_call, result, err = pcall(H.http.request_sync, "PUT", url, body, headers, {
        timeout = 3,
        content_type = "text/calendar; charset=utf-8",
    })
    if not ok_call then return nil, clip_err(result) end
    if err then return nil, clip_err(err) end
    if type(result) ~= "table" then return nil, "empty HTTP result" end
    local status = tonumber(result.status) or 0
    if status < 200 or status > 299 then
        return nil, string.format("HTTP %d", status)
    end
    return true
end

local function sync_calendar(txn_id, actor)
    local res, err = H.query_sync([[
        SELECT txn_id, description, txn_on, calendar_attempts, calendar_state_a2011
        FROM ${SCHEMA}transactions
        WHERE txn_id = :TXN_ID
    ]], { TXN_ID = txn_id })
    if err or not qrows(res)[1] then
        return {
            calendar_state_a2011 = 1, calendar_attempts = 0,
            calendar_error = clip_err(err or "missing transaction"),
        }
    end
    local row = qrows(res)[1]
    local attempts = tonumber(pick(row, "calendar_attempts")) or 0
    local state = tonumber(pick(row, "calendar_state_a2011")) or 1
    local lres, lerr = H.query_sync([[
        SELECT ledger_id FROM ${SCHEMA}lines WHERE txn_id = :TXN_ID
    ]], { TXN_ID = txn_id })
    if lerr then
        return { calendar_state_a2011 = state, calendar_attempts = attempts, calendar_error = clip_err(lerr) }
    end
    local url = nil
    local lrows = qrows(lres)
    for i = 1, #lrows do
        local ledger_id = tonumber(pick(lrows[i], "ledger_id"))
        if ledger_id then
            local ures, uerr = H.query_sync([[
                SELECT calendar_url FROM ${SCHEMA}ledgers WHERE ledger_id = :LEDGER_ID
            ]], { LEDGER_ID = ledger_id })
            if not uerr then
                url = join_url(pick(qrows(ures)[1] or {}, "calendar_url"), txn_id)
                if url then break end
            end
        end
    end
    if not url then
        return { calendar_state_a2011 = state, calendar_attempts = attempts, calendar_error = nil }
    end
    local pres, perr = H.query_sync([[
        SELECT txn_id FROM ${SCHEMA}transactions WHERE txn_id = :TXN_ID
    ]], { TXN_ID = txn_id })
    if perr or not qrows(pres)[1] then
        return { calendar_state_a2011 = state, calendar_attempts = attempts, calendar_error = clip_err(perr or "missing transaction") }
    end
    local _, pend_err = H.query_sync([[
        UPDATE ${SCHEMA}transactions
        SET calendar_state_a2011 = 2,
            updated_id = :ACTOR_UPDATED,
            updated_at = ${NOW}
        WHERE txn_id = :TXN_ID
    ]], { ACTOR_UPDATED = actor, TXN_ID = txn_id })
    if pend_err then
        return { calendar_state_a2011 = state, calendar_attempts = attempts, calendar_error = clip_err(pend_err) }
    end
    local event_id = "argent-txn-" .. tostring(txn_id)
    local put_ok, put_err = put_ics(url, ics_body(txn_id, pick(row, "txn_on"), pick(row, "description")))
    if put_ok then
        local sres, serr = H.query_sync([[
            SELECT txn_id FROM ${SCHEMA}transactions WHERE txn_id = :TXN_ID
        ]], { TXN_ID = txn_id })
        if serr or not qrows(sres)[1] then
            return { calendar_state_a2011 = 2, calendar_attempts = attempts, calendar_error = clip_err(serr or "missing transaction") }
        end
        local _, uerr = H.query_sync([[
            UPDATE ${SCHEMA}transactions
            SET calendar_state_a2011 = 3,
                calendar_event_id = :EVENT_ID,
                calendar_error = NULL,
                calendar_synced_at = ${NOW},
                updated_id = :ACTOR_UPDATED,
                updated_at = ${NOW}
            WHERE txn_id = :TXN_ID
        ]], { EVENT_ID = event_id, ACTOR_UPDATED = actor, TXN_ID = txn_id })
        if uerr then
            return { calendar_state_a2011 = 2, calendar_attempts = attempts, calendar_error = clip_err(uerr) }
        end
        return { calendar_state_a2011 = 3, calendar_attempts = attempts, calendar_error = nil }
    end
    local next_attempts = attempts + 1
    local ferr = clip_err(put_err)
    local fres, ferr_sel = H.query_sync([[
        SELECT txn_id FROM ${SCHEMA}transactions WHERE txn_id = :TXN_ID
    ]], { TXN_ID = txn_id })
    if ferr_sel or not qrows(fres)[1] then
        return { calendar_state_a2011 = 2, calendar_attempts = attempts, calendar_error = ferr }
    end
    local _, uerr = H.query_sync([[
        UPDATE ${SCHEMA}transactions
        SET calendar_state_a2011 = 4,
            calendar_attempts = :ATTEMPTS,
            calendar_error = CAST(:CAL_ERROR AS ${CAST_TEXT}),
            updated_id = :ACTOR_UPDATED,
            updated_at = ${NOW}
        WHERE txn_id = :TXN_ID
    ]], {
        ATTEMPTS = next_attempts, CAL_ERROR = ferr,
        ACTOR_UPDATED = actor, TXN_ID = txn_id,
    })
    if uerr then
        return { calendar_state_a2011 = 2, calendar_attempts = attempts, calendar_error = clip_err(uerr) }
    end
    return { calendar_state_a2011 = 4, calendar_attempts = next_attempts, calendar_error = ferr }
end

-- Argent.GenerateSchedule
-- Expands the stored RRULE through the horizon and inserts Reserved rows
-- (lookup 2003 key 1, kind 11). A date that already has any row for this
-- schedule is skipped, including a rescinded row. Calendar sync runs only
-- when a line ledger already has calendar_url. HTTP failure leaves the row.

if type(params) ~= "table" then
    return fail("params_required", "params must be an object")
end
local schedule_id = int(params.schedule_id)
if not schedule_id then return fail("schedule_id", "schedule_id is required") end
local through = nil
if params.through ~= nil then
    if not date_ok(params.through) then return fail("through", "through must be YYYY-MM-DD") end
    through = params.through
end
local actor = actor_id()

local sres, serr = H.query_sync([[
    SELECT schedule_id, organization_id, status_a2007, name,
           from_ledger_id, to_ledger_id, amount_cents, currency,
           rrule, anchor_on, end_on, horizon_mode_a2008, collection
    FROM ${SCHEMA}schedules
    WHERE schedule_id = :SCHEDULE_ID
]], { SCHEDULE_ID = schedule_id })
if serr then return fail("query_failed", tostring(serr)) end
local sched = qrows(sres)[1]
if not sched then return fail("not_found", "schedule not found") end
if tonumber(pick(sched, "status_a2007")) ~= 1 then
    return fail("inactive", "schedule is not active")
end
local org_id = tonumber(pick(sched, "organization_id"))
local from_id = tonumber(pick(sched, "from_ledger_id"))
local to_id = tonumber(pick(sched, "to_ledger_id"))
local amount = tonumber(pick(sched, "amount_cents"))
local currency = string.lower(tostring(pick(sched, "currency") or ""))
local anchor = day_of(pick(sched, "anchor_on"))
local end_on = day_of(pick(sched, "end_on"))
local mode = tonumber(pick(sched, "horizon_mode_a2008")) or 1
local name = trim(tostring(pick(sched, "name") or "")) or "schedule"
local rule, rerr = parse_rrule(trim(tostring(pick(sched, "rrule") or "")) or "")
if not rule then return fail("rrule", rerr or "rrule is not valid") end
if not anchor or not amount or amount == 0 or not from_id or not to_id then
    return fail("schedule", "schedule is missing a required field")
end

local ores, oerr = H.query_sync([[
    SELECT fiscal_year_start_month, fiscal_year_start_day
    FROM ${SCHEMA}organizations
    WHERE organization_id = :ORG_ID
]], { ORG_ID = org_id })
if oerr then return fail("query_failed", tostring(oerr)) end
local org = qrows(ores)[1]
if not org then return fail("organization", "organization not found") end
local sm = tonumber(pick(org, "fiscal_year_start_month"))
local sd = tonumber(pick(org, "fiscal_year_start_day"))
if not sm or sm < 1 or sm > 12 or not sd or sd < 1 or sd > 31 then
    return fail("fiscal", "fiscal year start is not valid")
end
local ay, am, ad = ymd(anchor)
local fye_s = ymd_str(fye(ay, am, ad, sm, sd))
local days = col_num(pick(sched, "collection"), "horizon_days")
local horizon = anchor
if mode == 1 then
    horizon = fye_s
elseif mode == 2 then
    if not days or days < 1 or days > 366 then
        return fail("horizon_days", "horizon_days must be 1 through 366")
    end
    horizon = ymd_str(add_days(ay, am, ad, days))
else
    horizon = end_on or through or anchor
end
if mode ~= 3 then
    horizon = earlier(horizon, end_on)
    horizon = earlier(horizon, through)
elseif end_on and through then
    horizon = earlier(end_on, through)
end

local dates, derr = expand_dates(rule, anchor, horizon, 400)
if derr == "horizon_cap" then
    return fail("horizon_cap", "horizon has more than 400 occurrences")
end
if not dates then return fail(derr or "rrule", "could not expand rrule") end

local function load_ledger(id)
    local res, err = H.query_sync([[
        SELECT ledger_id, organization_id, is_posting, currency, calendar_url
        FROM ${SCHEMA}ledgers
        WHERE ledger_id = :LEDGER_ID
    ]], { LEDGER_ID = id })
    if err then return nil, err end
    local row = qrows(res)[1]
    if not row then return nil, "missing" end
    return {
        organization_id = tonumber(pick(row, "organization_id")),
        is_posting = tonumber(pick(row, "is_posting")),
        currency = string.lower(tostring(pick(row, "currency") or "")),
        calendar_url = pick(row, "calendar_url"),
    }
end

local from_l, ferr = load_ledger(from_id)
if ferr == "missing" then return fail("ledger", "from ledger not found") end
if ferr then return fail("query_failed", tostring(ferr)) end
local to_l, terr = load_ledger(to_id)
if terr == "missing" then return fail("ledger", "to ledger not found") end
if terr then return fail("query_failed", tostring(terr)) end
if from_l.is_posting ~= 1 or to_l.is_posting ~= 1 then
    return fail("ledger", "both ledgers must be posting")
end
if from_l.currency ~= currency or to_l.currency ~= currency then
    return fail("currency", "both ledgers must use the schedule currency")
end
local do_sync = join_url(from_l.calendar_url, 0) or join_url(to_l.calendar_url, 0)

local seen = {}
local xres, xerr = H.query_sync([[
    SELECT txn_on FROM ${SCHEMA}transactions WHERE schedule_id = :SCHEDULE_ID
]], { SCHEDULE_ID = schedule_id })
if xerr then return fail("query_failed", tostring(xerr)) end
local xrows = qrows(xres)
for i = 1, #xrows do
    local seen_on = day_of(pick(xrows[i], "txn_on"))
    if seen_on then seen[seen_on] = true end
end

local function next_txn()
    local res, err = H.query_sync([[
        SELECT COALESCE(MAX(txn_id), 0) + 1 AS next_id FROM ${SCHEMA}transactions
    ]], {})
    if err then return nil, err end
    return tonumber(pick(qrows(res)[1], "next_id")) or 1
end

local function next_line()
    local res, err = H.query_sync([[
        SELECT COALESCE(MAX(line_id), 0) + 1 AS next_id FROM ${SCHEMA}lines
    ]], {})
    if err then return nil, err end
    return tonumber(pick(qrows(res)[1], "next_id")) or 1
end

local function insert_line(txn_id, seq, ledger_id, cents)
    local line_id, err = next_line()
    if err then return nil, err end
    local _, ierr = H.query_sync([[
        INSERT INTO ${SCHEMA}lines (
            line_id, txn_id, line_seq, ledger_id, amount_cents,
            tax_code_id, tax_cents, tax_manual, cleared,
            reconciliation_id, statement_txn_id, memo, collection,
            valid_after, valid_until, created_id, created_at, updated_id, updated_at
        ) VALUES (
            :LINE_ID, :TXN_ID, :LINE_SEQ, :LEDGER_ID, :AMOUNT_CENTS,
            NULL, NULL, 0, 0,
            NULL, NULL, NULL, ${JIS}CAST(:LINE_COLLECTION AS ${CAST_TEXT})${JIE},
            NULL, NULL, :ACTOR_CREATED, ${NOW}, :ACTOR_UPDATED, ${NOW}
        )
    ]], {
        LINE_ID = line_id, TXN_ID = txn_id, LINE_SEQ = seq,
        LEDGER_ID = ledger_id, AMOUNT_CENTS = cents,
        LINE_COLLECTION = "{}", ACTOR_CREATED = actor, ACTOR_UPDATED = actor,
    })
    if ierr then return nil, ierr end
    return line_id
end

local created = {}
local txn_ids = {}
local skipped = 0
for i = 1, #dates do
    local txn_on = dates[i]
    if seen[txn_on] then
        skipped = skipped + 1
    else
        local txn_id, nerr = next_txn()
        if nerr then
            return fail("partial_write", tostring(nerr), {
                schedule_id = schedule_id, created = created, txn_ids = txn_ids,
            })
        end
        local _, herr = H.query_sync([[
            INSERT INTO ${SCHEMA}transactions (
                txn_id, organization_id, status_a2003, kind_a2004, txn_on,
                description, memo, schedule_id, replaces_txn_id,
                calendar_state_a2011, calendar_event_id, calendar_error,
                calendar_attempts, calendar_synced_at, summary, collection,
                valid_after, valid_until, created_id, created_at, updated_id, updated_at
            ) VALUES (
                :TXN_ID, :ORG_ID, 1, 11, CAST(:TXN_ON AS ${DATE}),
                :TXN_DESCRIPTION, NULL, :SCHEDULE_ID, NULL,
                1, NULL, NULL,
                0, NULL, NULL, ${JIS}CAST(:TXN_COLLECTION AS ${CAST_TEXT})${JIE},
                NULL, NULL, :ACTOR_CREATED, ${NOW}, :ACTOR_UPDATED, ${NOW}
            )
        ]], {
            TXN_ID = txn_id, ORG_ID = org_id, TXN_ON = txn_on,
            TXN_DESCRIPTION = name, SCHEDULE_ID = schedule_id,
            TXN_COLLECTION = "{}", ACTOR_CREATED = actor, ACTOR_UPDATED = actor,
        })
        if herr then
            return fail("partial_write", tostring(herr), {
                schedule_id = schedule_id, created = created, txn_ids = txn_ids,
            })
        end
        local _, lerr1 = insert_line(txn_id, 1, to_id, amount)
        if lerr1 then
            return fail("partial_write", tostring(lerr1), {
                schedule_id = schedule_id, txn_id = txn_id, created = created, txn_ids = txn_ids,
            })
        end
        local _, lerr2 = insert_line(txn_id, 2, from_id, -amount)
        if lerr2 then
            return fail("partial_write", tostring(lerr2), {
                schedule_id = schedule_id, txn_id = txn_id, created = created, txn_ids = txn_ids,
            })
        end
        local cal = { calendar_state_a2011 = 1, calendar_attempts = 0, calendar_error = nil }
        if do_sync then cal = sync_calendar(txn_id, actor) end
        created[#created + 1] = {
            txn_id = txn_id, txn_on = txn_on, status_a2003 = 1,
            calendar_state_a2011 = cal.calendar_state_a2011,
            calendar_attempts = cal.calendar_attempts,
            calendar_error = cal.calendar_error,
        }
        txn_ids[#txn_ids + 1] = txn_id
        seen[txn_on] = true
    end
end

return ok({
    schedule_id = schedule_id, created = created, skipped = skipped, txn_ids = txn_ids,
})
]==],
                'Expand a schedule into Reserved transactions',
                0,
                1,
                '{"inputSchema":{"type":"object","properties":{"schedule_id":{"type":"integer"},"through":{"type":"string"}},"required":["schedule_id"],"additionalProperties":true}}',
                '{"title":"Generate schedule"}',
                ${COMMON_VALUES}
            );

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Seed Argent GenerateSchedule'                                                    AS name,
        [=[
            # Forward Migration ${MIGRATION}: Argent.GenerateSchedule

            Inserts the Argent script row named in the reverse migration.
            Group `Argent`. `mcp_access=1`, `invokable=0`.
            Does not install a QueryRef. Does not create a table.
            Does not add a column.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]====]})

-- ----------------------------------------------------------------------------
-- Reverse
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
            WHERE group_name = 'Argent'
              AND script_name IN ('GenerateSchedule');

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Remove Argent GenerateSchedule'                                                  AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Remove Argent.GenerateSchedule

            Deletes that script row. Does not drop Argent tables.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]====]})

-- ----------------------------------------------------------------------------
-- Diagram
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
        ${TYPE_DIAGRAM_MIGRATION}                                           AS query_type_a28,
        ${DIALECT}                                                          AS query_dialect_a30,
        ${QTC_SLOW}                                                         AS query_queue_a58,
        ${TIMEOUT}                                                          AS query_timeout,
        'JSON script definition in collection'                              AS code,
        'Diagram Argent GenerateSchedule'                                                 AS name,
        [=[
            # Diagram Migration ${MIGRATION}

            Script row. This does not define a table.
        ]=]
                                                                            AS summary,
                                                                            -- DIAGRAM_START
        ${JSON_INGEST_START}
        [=[
            {
                "diagram": [
                    {
                        "object_type": "script",
                        "object_id": "script.Argent.GenerateSchedule",
                        "object_ref": "${MIGRATION}",
                        "name": "Argent.GenerateSchedule"
                    }
                ]
            }
        ]=]
        ${JSON_INGEST_END}
                                                                            -- DIAGRAM_END
                                                                            AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]====]})

return queries end
