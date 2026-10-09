-- Migration: argent_2045.lua
-- Argent.UpsertSchedule
--
-- luacheck: no max line length
-- luacheck: no unused args
--
-- CHANGELOG
-- 1.0.0 - 2026-10-08 - Store a schedule and an optional ledger calendar URL

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "scripts"
cfg.MIGRATION = "2045"
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
                'UpsertSchedule',
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

-- Argent.UpsertSchedule
-- Stores a schedule. Does not expand dates and does not call HTTP.
-- calendar_url, when sent, is written on from_ledger_id after the save.
-- A repeat idempotency_key returns the existing row and writes nothing.

if type(params) ~= "table" then
    return fail("params_required", "params must be an object")
end

local actor = actor_id()
local schedule_id = int(params.schedule_id)
local existing = nil

if schedule_id then
    local res, err = H.query_sync([[
        SELECT schedule_id, organization_id, status_a2007, name,
               from_ledger_id, to_ledger_id, amount_cents, currency,
               tax_code_id, rrule, anchor_on, end_on, estimate_flag,
               horizon_mode_a2008, summary, collection
        FROM ${SCHEMA}schedules
        WHERE schedule_id = :SCHEDULE_ID
    ]], { SCHEDULE_ID = schedule_id })
    if err then return fail("query_failed", tostring(err)) end
    local row = qrows(res)[1]
    if not row then return fail("not_found", "schedule not found") end
    existing = {
        schedule_id = tonumber(pick(row, "schedule_id")),
        organization_id = tonumber(pick(row, "organization_id")),
        status = tonumber(pick(row, "status_a2007")),
        name = pick(row, "name"),
        from_ledger_id = tonumber(pick(row, "from_ledger_id")),
        to_ledger_id = tonumber(pick(row, "to_ledger_id")),
        amount = tonumber(pick(row, "amount_cents")),
        currency = pick(row, "currency"),
        tax_code_id = tonumber(pick(row, "tax_code_id")),
        rrule = pick(row, "rrule"),
        anchor_on = day_of(pick(row, "anchor_on")),
        end_on = day_of(pick(row, "end_on")),
        estimate_flag = tonumber(pick(row, "estimate_flag")) or 0,
        horizon_mode = tonumber(pick(row, "horizon_mode_a2008")),
        summary = pick(row, "summary"),
        collection = pick(row, "collection"),
    }
elseif params.schedule_id ~= nil then
    return fail("schedule_id", "schedule_id must be an integer")
end

local org_id = int(params.organization_id) or (existing and existing.organization_id)
if params.organization_id ~= nil and not int(params.organization_id) then
    return fail("organization_id", "organization_id must be an integer")
end
if not org_id then return fail("organization_id", "organization_id is required") end
if existing and int(params.organization_id) and int(params.organization_id) ~= existing.organization_id then
    return fail("organization", "organization_id does not match the schedule")
end

local key, kerr = idem_key(params.idempotency_key)
if kerr then return fail("idempotency_key", kerr) end
if key == "" and existing then
    key = col_str(existing.collection, "idempotency_key") or ""
end

if key ~= "" and not existing then
    local ires, ierr = H.query_sync([[
        SELECT schedule_id, collection
        FROM ${SCHEMA}schedules
        WHERE organization_id = :ORG_ID
    ]], { ORG_ID = org_id })
    if ierr then return fail("query_failed", tostring(ierr)) end
    local rows = qrows(ires)
    for i = 1, #rows do
        if has_idem(pick(rows[i], "collection"), key) then
            return ok({
                schedule_id = tonumber(pick(rows[i], "schedule_id")),
                idempotent = true, created = false,
            })
        end
    end
end

local name = trim(params.name) or (existing and trim(tostring(existing.name or "")))
if params.name ~= nil and type(params.name) ~= "string" then
    return fail("name_required", "name must be a string")
end
if not name then return fail("name_required", "name is required") end
if #name > 250 then return fail("name", "name is longer than 250 characters") end

local rrule_text = trim(params.rrule) or (existing and trim(tostring(existing.rrule or "")))
if params.rrule ~= nil and type(params.rrule) ~= "string" then
    return fail("rrule", "rrule must be a string")
end
if not rrule_text then return fail("rrule", "rrule is required") end
local rule, rerr = parse_rrule(rrule_text)
if not rule then return fail("rrule", rerr) end

local amount = int(params.amount_cents)
if params.amount_cents == nil and existing then amount = existing.amount end
if params.amount_cents ~= nil and amount == nil then
    return fail("amount", "amount_cents must be an integer")
end
if amount == nil or amount == 0 then
    return fail("amount", "amount_cents must be a non-zero integer")
end

local currency = trim(params.currency)
if currency then currency = string.lower(currency) end
if not currency and existing and type(existing.currency) == "string" then
    currency = string.lower(existing.currency)
end
if not currency or not currency:match("^[a-z][a-z][a-z]$") then
    return fail("currency", "currency must be a 3-letter code")
end

local from_id = int(params.from_ledger_id) or (existing and existing.from_ledger_id)
local to_id = int(params.to_ledger_id) or (existing and existing.to_ledger_id)
if params.from_ledger_id ~= nil and not int(params.from_ledger_id) then
    return fail("ledger", "from_ledger_id must be an integer")
end
if params.to_ledger_id ~= nil and not int(params.to_ledger_id) then
    return fail("ledger", "to_ledger_id must be an integer")
end
if not from_id or not to_id then
    return fail("ledger", "from_ledger_id and to_ledger_id are required")
end
if from_id == to_id then
    return fail("ledger", "from_ledger_id and to_ledger_id must differ")
end

local anchor = nil
if params.anchor_on == nil then
    anchor = existing and existing.anchor_on or nil
elseif not date_ok(params.anchor_on) then
    return fail("anchor_on", "anchor_on must be YYYY-MM-DD")
else
    anchor = params.anchor_on
end
if not anchor then return fail("anchor_on", "anchor_on is required") end

local end_on = nil
if params.end_on == nil then
    end_on = existing and existing.end_on or nil
elseif params.end_on == "" then
    end_on = nil
elseif not date_ok(params.end_on) then
    return fail("end_on", "end_on must be YYYY-MM-DD")
else
    end_on = params.end_on
end
if end_on and end_on < anchor then return fail("end_on", "end_on is before anchor_on") end
local has_end = end_on and 1 or 0

local estimate = int(params.estimate_flag)
if params.estimate_flag == nil then
    estimate = existing and existing.estimate_flag or 0
end
if estimate ~= 0 and estimate ~= 1 then
    return fail("estimate_flag", "estimate_flag must be 0 or 1")
end

local mode = int(params.horizon_mode_a2008)
if params.horizon_mode_a2008 == nil then
    mode = existing and existing.horizon_mode or 1
end
if mode ~= 1 and mode ~= 2 and mode ~= 3 then
    return fail("horizon_mode", "horizon_mode_a2008 must be 1, 2, or 3")
end

local days = int(params.horizon_days)
if params.horizon_days == nil and existing then
    days = col_num(existing.collection, "horizon_days")
end
if params.horizon_days ~= nil and days == nil then
    return fail("horizon_days", "horizon_days must be an integer")
end
if days ~= nil and (days < 1 or days > 366) then
    return fail("horizon_days", "horizon_days must be 1 through 366")
end
if mode == 2 and days == nil then
    return fail("horizon_days", "horizon_days is required for horizon mode 2")
end

local status = int(params.status_a2007)
if params.status_a2007 == nil then
    status = existing and existing.status or 1
end
if status ~= 1 and status ~= 2 and status ~= 3 then
    return fail("status", "status_a2007 must be 1, 2, or 3")
end

local summary = ""
if params.summary == nil then
    if existing and type(existing.summary) == "string" then summary = existing.summary end
elseif type(params.summary) ~= "string" then
    return fail("summary", "summary must be a string")
else
    summary = params.summary
end
if #summary > 200 then return fail("summary", "summary is longer than 200 characters") end

local tax_id = nil
if params.tax_code_id == nil then
    tax_id = existing and existing.tax_code_id or nil
else
    tax_id = int(params.tax_code_id)
    if not tax_id or tax_id <= 0 then return fail("tax_code", "tax_code_id must be an integer") end
end
local has_tax = 0

local url_mode = nil
local url_value = ""
if params.calendar_url ~= nil then
    if type(params.calendar_url) ~= "string" then
        return fail("calendar_url", "calendar_url must be a string")
    end
    local u = trim(params.calendar_url)
    if not u then
        url_mode = "clear"
    elseif #u > 200 or not u:match("^https?://") or u:find("%s") or u:find("%c") then
        return fail("calendar_url", "calendar_url must be an http(s) URL of at most 200 characters")
    else
        url_mode = "set"
        url_value = u
    end
end

local ores, oerr = H.query_sync([[
    SELECT organization_id FROM ${SCHEMA}organizations WHERE organization_id = :ORG_ID
]], { ORG_ID = org_id })
if oerr then return fail("query_failed", tostring(oerr)) end
if not qrows(ores)[1] then return fail("organization", "organization not found") end

local function load_ledger(id)
    local res, err = H.query_sync([[
        SELECT ledger_id, organization_id, is_posting, currency
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
if from_l.organization_id ~= org_id or to_l.organization_id ~= org_id then
    return fail("organization", "ledgers must belong to the organization")
end
if from_l.currency ~= currency or to_l.currency ~= currency then
    return fail("currency", "both ledgers must use the schedule currency")
end

local cres, cerr = H.query_sync([[
    SELECT currency_code FROM ${SCHEMA}currencies WHERE currency_code = :CURRENCY
]], { CURRENCY = currency })
if cerr then return fail("query_failed", tostring(cerr)) end
if not qrows(cres)[1] then return fail("currency", "currency is not known") end

if tax_id then
    local tres, txerr = H.query_sync([[
        SELECT tax_code_id, organization_id
        FROM ${SCHEMA}tax_codes
        WHERE tax_code_id = :TAX_CODE_ID
    ]], { TAX_CODE_ID = tax_id })
    if txerr then return fail("query_failed", tostring(txerr)) end
    local tax = qrows(tres)[1]
    if not tax then return fail("tax_code", "tax code not found") end
    if tonumber(pick(tax, "organization_id")) ~= org_id then
        return fail("tax_code", "tax code is in another organization")
    end
    has_tax = 1
end

if existing and key ~= "" then
    local ires, ierr = H.query_sync([[
        SELECT schedule_id, collection
        FROM ${SCHEMA}schedules
        WHERE organization_id = :ORG_ID
    ]], { ORG_ID = org_id })
    if ierr then return fail("query_failed", tostring(ierr)) end
    local rows = qrows(ires)
    for i = 1, #rows do
        local other = tonumber(pick(rows[i], "schedule_id"))
        if other ~= schedule_id and has_idem(pick(rows[i], "collection"), key) then
            return fail("idempotency_key", "idempotency_key is already in use")
        end
    end
end

local collection = build_col(days, key)

local function apply_url()
    if not url_mode then return nil end
    local sres, serr = H.query_sync([[
        SELECT ledger_id FROM ${SCHEMA}ledgers WHERE ledger_id = :LEDGER_ID
    ]], { LEDGER_ID = from_id })
    if serr then return serr end
    if not qrows(sres)[1] then return "missing ledger" end
    if url_mode == "clear" then
        local _, uerr = H.query_sync([[
            UPDATE ${SCHEMA}ledgers
            SET calendar_url = NULL,
                updated_id = :ACTOR_UPDATED,
                updated_at = ${NOW}
            WHERE ledger_id = :LEDGER_ID
        ]], { ACTOR_UPDATED = actor, LEDGER_ID = from_id })
        return uerr
    end
    local _, uerr = H.query_sync([[
        UPDATE ${SCHEMA}ledgers
        SET calendar_url = CAST(:CAL_URL AS ${CAST_TEXT}),
            updated_id = :ACTOR_UPDATED,
            updated_at = ${NOW}
        WHERE ledger_id = :LEDGER_ID
    ]], { CAL_URL = url_value, ACTOR_UPDATED = actor, LEDGER_ID = from_id })
    return uerr
end

local binds = {
    STATUS_A2007 = status, SCHED_NAME = name,
    FROM_LEDGER_ID = from_id, TO_LEDGER_ID = to_id,
    AMOUNT_CENTS = amount, CURRENCY = currency,
    HAS_TAX = has_tax, TAX_CODE_ID = tax_id or 0,
    RRULE = rrule_text, ANCHOR_ON = anchor,
    HAS_END = has_end, END_ON = end_on or "1970-01-01",
    ESTIMATE_FLAG = estimate, HORIZON_MODE = mode,
    SCHED_SUMMARY = summary, SCHED_COLLECTION = collection,
    ACTOR_UPDATED = actor,
}

if existing then
    local sres, serr = H.query_sync([[
        SELECT schedule_id FROM ${SCHEMA}schedules WHERE schedule_id = :SCHEDULE_ID
    ]], { SCHEDULE_ID = schedule_id })
    if serr then return fail("query_failed", tostring(serr)) end
    if not qrows(sres)[1] then return fail("not_found", "schedule not found") end
    binds.SCHEDULE_ID = schedule_id
    local _, uerr = H.query_sync([[
        UPDATE ${SCHEMA}schedules
        SET status_a2007 = :STATUS_A2007,
            name = :SCHED_NAME,
            from_ledger_id = :FROM_LEDGER_ID,
            to_ledger_id = :TO_LEDGER_ID,
            amount_cents = :AMOUNT_CENTS,
            currency = :CURRENCY,
            tax_code_id = CASE WHEN CAST(:HAS_TAX AS ${CAST_INTEGER}) = 0 THEN CAST(NULL AS ${CAST_INTEGER}) ELSE CAST(:TAX_CODE_ID AS ${CAST_INTEGER}) END,
            rrule = :RRULE,
            anchor_on = CAST(:ANCHOR_ON AS ${DATE}),
            end_on = CASE WHEN CAST(:HAS_END AS ${CAST_INTEGER}) = 0 THEN CAST(NULL AS ${DATE}) ELSE CAST(:END_ON AS ${DATE}) END,
            estimate_flag = :ESTIMATE_FLAG,
            horizon_mode_a2008 = :HORIZON_MODE,
            summary = NULLIF(CAST(:SCHED_SUMMARY AS ${CAST_TEXT}), ''),
            collection = ${JIS}CAST(:SCHED_COLLECTION AS ${CAST_TEXT})${JIE},
            updated_id = :ACTOR_UPDATED,
            updated_at = ${NOW}
        WHERE schedule_id = :SCHEDULE_ID
    ]], binds)
    if uerr then return fail("update_failed", tostring(uerr)) end
    local url_err = apply_url()
    if url_err then return fail("update_failed", tostring(url_err), { schedule_id = schedule_id }) end
    return ok({ schedule_id = schedule_id, created = false, idempotent = false })
end

local nres, nerr = H.query_sync([[
    SELECT COALESCE(MAX(schedule_id), 0) + 1 AS next_id FROM ${SCHEMA}schedules
]], {})
if nerr then return fail("query_failed", tostring(nerr)) end
local new_id = tonumber(pick(qrows(nres)[1], "next_id")) or 1
binds.SCHEDULE_ID = new_id
binds.ORG_ID = org_id
binds.ACTOR_CREATED = actor
local _, ierr = H.query_sync([[
    INSERT INTO ${SCHEMA}schedules (
        schedule_id, organization_id, status_a2007, name,
        from_ledger_id, to_ledger_id, amount_cents, currency, tax_code_id,
        rrule, anchor_on, end_on, estimate_flag, horizon_mode_a2008,
        summary, collection,
        valid_after, valid_until, created_id, created_at, updated_id, updated_at
    ) VALUES (
        :SCHEDULE_ID, :ORG_ID, :STATUS_A2007, :SCHED_NAME,
        :FROM_LEDGER_ID, :TO_LEDGER_ID, :AMOUNT_CENTS, :CURRENCY,
        CASE WHEN CAST(:HAS_TAX AS ${CAST_INTEGER}) = 0 THEN CAST(NULL AS ${CAST_INTEGER}) ELSE CAST(:TAX_CODE_ID AS ${CAST_INTEGER}) END,
        :RRULE, CAST(:ANCHOR_ON AS ${DATE}),
        CASE WHEN CAST(:HAS_END AS ${CAST_INTEGER}) = 0 THEN CAST(NULL AS ${DATE}) ELSE CAST(:END_ON AS ${DATE}) END,
        :ESTIMATE_FLAG, :HORIZON_MODE,
        NULLIF(CAST(:SCHED_SUMMARY AS ${CAST_TEXT}), ''),
        ${JIS}CAST(:SCHED_COLLECTION AS ${CAST_TEXT})${JIE},
        NULL, NULL, :ACTOR_CREATED, ${NOW}, :ACTOR_UPDATED, ${NOW}
    )
]], binds)
if ierr then return fail("insert_failed", tostring(ierr)) end
local url_err = apply_url()
if url_err then return fail("update_failed", tostring(url_err), { schedule_id = new_id }) end
return ok({ schedule_id = new_id, created = true, idempotent = false })
]==],
                'Store a schedule and an optional ledger calendar URL',
                0,
                1,
                '{"inputSchema":{"type":"object","properties":{"organization_id":{"type":"integer"},"schedule_id":{"type":"integer"},"name":{"type":"string"},"from_ledger_id":{"type":"integer"},"to_ledger_id":{"type":"integer"},"amount_cents":{"type":"integer"},"currency":{"type":"string"},"rrule":{"type":"string"},"anchor_on":{"type":"string"},"end_on":{"type":"string"},"estimate_flag":{"type":"integer"},"horizon_mode_a2008":{"type":"integer"},"horizon_days":{"type":"integer"},"status_a2007":{"type":"integer"},"summary":{"type":"string"},"tax_code_id":{"type":"integer"},"idempotency_key":{"type":"string"},"calendar_url":{"type":"string"}},"additionalProperties":true}}',
                '{"title":"Upsert schedule"}',
                ${COMMON_VALUES}
            );

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Seed Argent UpsertSchedule'                                                    AS name,
        [=[
            # Forward Migration ${MIGRATION}: Argent.UpsertSchedule

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
              AND script_name IN ('UpsertSchedule');

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Remove Argent UpsertSchedule'                                                  AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Remove Argent.UpsertSchedule

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
        'Diagram Argent UpsertSchedule'                                                 AS name,
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
                        "object_id": "script.Argent.UpsertSchedule",
                        "object_ref": "${MIGRATION}",
                        "name": "Argent.UpsertSchedule"
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
