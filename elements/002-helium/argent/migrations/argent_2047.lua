-- Migration: argent_2047.lua
-- Argent.MatchReserved
--
-- luacheck: no max line length
-- luacheck: no unused args
--
-- CHANGELOG
-- 1.0.0 - 2026-10-08 - Match a Reserved row to a Recorded actual

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "scripts"
cfg.MIGRATION = "2047"
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
                'MatchReserved',
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

-- Argent.MatchReserved
-- Turns one Reserved row into a Recorded actual and marks that Reserved
-- row Rescinded (lookup 2003 key 5). The amount window is checked before
-- any write. Calendar sync runs on the actual. A down host leaves it saved.

if type(params) ~= "table" then
    return fail("params_required", "params must be an object")
end
local reserved_id = int(params.reserved_txn_id)
if not reserved_id then return fail("reserved_txn_id", "reserved_txn_id is required") end
local window = 0
if params.amount_window ~= nil then
    window = int(params.amount_window)
    if window == nil or window < 0 then
        return fail("amount_window", "amount_window must be a non-negative integer")
    end
end
local actor = actor_id()

local function load_txn(id)
    local res, err = H.query_sync([[
        SELECT txn_id, organization_id, status_a2003, txn_on, description,
               schedule_id, replaces_txn_id, calendar_state_a2011
        FROM ${SCHEMA}transactions
        WHERE txn_id = :TXN_ID
    ]], { TXN_ID = id })
    if err then return nil, err end
    local row = qrows(res)[1]
    if not row then return nil, "missing" end
    return {
        txn_id = tonumber(pick(row, "txn_id")),
        organization_id = tonumber(pick(row, "organization_id")),
        status = tonumber(pick(row, "status_a2003")),
        txn_on = day_of(pick(row, "txn_on")),
        description = pick(row, "description"),
        schedule_id = tonumber(pick(row, "schedule_id")),
        replaces_txn_id = tonumber(pick(row, "replaces_txn_id")),
    }
end

local reserved, rerr = load_txn(reserved_id)
if rerr == "missing" then return fail("not_found", "reserved transaction not found") end
if rerr then return fail("query_failed", tostring(rerr)) end
if reserved.status ~= 1 or not reserved.schedule_id or not reserved.txn_on then
    return fail("not_reserved", "transaction is not an open reserved row")
end
if params.txn_on ~= nil and params.txn_on ~= reserved.txn_on then
    return fail("txn_on", "txn_on must match the reserved date")
end

local sres, serr = H.query_sync([[
    SELECT schedule_id, name, from_ledger_id, to_ledger_id, amount_cents
    FROM ${SCHEMA}schedules
    WHERE schedule_id = :SCHEDULE_ID
]], { SCHEDULE_ID = reserved.schedule_id })
if serr then return fail("query_failed", tostring(serr)) end
local sched = qrows(sres)[1]
if not sched then return fail("not_found", "schedule not found") end
local sched_amt = tonumber(pick(sched, "amount_cents"))
local from_id = tonumber(pick(sched, "from_ledger_id"))
local to_id = tonumber(pick(sched, "to_ledger_id"))
local sched_name = trim(tostring(pick(sched, "name") or "")) or "schedule"
if not sched_amt or not from_id or not to_id then
    return fail("schedule", "schedule is missing a required field")
end

local function within(actual)
    local diff = actual - sched_amt
    if diff < 0 then diff = -diff end
    return diff <= window
end

local override = nil
if params.amount_cents ~= nil then
    override = int(params.amount_cents)
    if override == nil or override == 0 then
        return fail("amount", "amount_cents must be a non-zero integer")
    end
    if not within(override) then
        return fail("match_amount", "amount is outside the schedule window")
    end
end

local actual_id = int(params.actual_txn_id)
if params.actual_txn_id ~= nil and not actual_id then
    return fail("actual_txn_id", "actual_txn_id must be an integer")
end
if actual_id and actual_id == reserved_id then
    return fail("actual_txn_id", "actual_txn_id must be a different transaction")
end

local description = sched_name
if params.description ~= nil then
    description = trim(params.description)
    if not description then return fail("description_required", "description is required") end
    if #description > 250 then return fail("description", "description is longer than 250 characters") end
end
local memo = ""
if params.memo ~= nil then
    if type(params.memo) ~= "string" then return fail("memo", "memo must be a string") end
    if #params.memo > 200 then return fail("memo", "memo is longer than 200 characters") end
    memo = params.memo
end

local function rescind()
    local res, err = H.query_sync([[
        SELECT txn_id FROM ${SCHEMA}transactions WHERE txn_id = :TXN_ID
    ]], { TXN_ID = reserved_id })
    if err then return err end
    if not qrows(res)[1] then return "missing transaction" end
    local _, uerr = H.query_sync([[
        UPDATE ${SCHEMA}transactions
        SET status_a2003 = 5,
            updated_id = :ACTOR_UPDATED,
            updated_at = ${NOW}
        WHERE txn_id = :TXN_ID
    ]], { ACTOR_UPDATED = actor, TXN_ID = reserved_id })
    return uerr
end

local function finish(saved_id, created)
    local uerr = rescind()
    if uerr then
        return fail("partial_write", tostring(uerr), { txn_id = saved_id, reserved_txn_id = reserved_id })
    end
    local cal = sync_calendar(saved_id, actor)
    return ok({
        txn_id = saved_id,
        reserved_txn_id = reserved_id,
        schedule_id = reserved.schedule_id,
        replaces_txn_id = reserved_id,
        status_a2003 = 3,
        reserved_status_a2003 = 5,
        calendar_state_a2011 = cal.calendar_state_a2011,
        calendar_attempts = cal.calendar_attempts,
        calendar_error = cal.calendar_error,
        created = created,
    })
end

if actual_id then
    local actual, aerr = load_txn(actual_id)
    if aerr == "missing" then return fail("not_found", "actual transaction not found") end
    if aerr then return fail("query_failed", tostring(aerr)) end
    if actual.txn_on ~= reserved.txn_on then
        return fail("txn_on", "actual date must match the reserved date")
    end
    if actual.organization_id ~= reserved.organization_id then
        return fail("organization", "actual is in another organization")
    end
    if actual.status == 4 or actual.status == 5 or actual.status == nil then
        return fail("status", "actual cannot be reconciled or rescinded")
    end
    local lres, lerr = H.query_sync([[
        SELECT ledger_id, amount_cents FROM ${SCHEMA}lines WHERE txn_id = :TXN_ID
    ]], { TXN_ID = actual_id })
    if lerr then return fail("query_failed", tostring(lerr)) end
    local matched = false
    local rows = qrows(lres)
    for i = 1, #rows do
        if tonumber(pick(rows[i], "ledger_id")) == to_id then
            local cents = tonumber(pick(rows[i], "amount_cents"))
            if cents and within(cents) then matched = true end
        end
    end
    if not matched then
        return fail("match_amount", "to-ledger amount is outside the schedule window")
    end
    local pres, perr = H.query_sync([[
        SELECT txn_id FROM ${SCHEMA}transactions WHERE txn_id = :TXN_ID
    ]], { TXN_ID = actual_id })
    if perr then return fail("query_failed", tostring(perr)) end
    if not qrows(pres)[1] then return fail("not_found", "actual transaction not found") end
    local _, uerr = H.query_sync([[
        UPDATE ${SCHEMA}transactions
        SET replaces_txn_id = :RESERVED_ID,
            schedule_id = :SCHEDULE_ID,
            updated_id = :ACTOR_UPDATED,
            updated_at = ${NOW}
        WHERE txn_id = :TXN_ID
    ]], {
        RESERVED_ID = reserved_id, SCHEDULE_ID = reserved.schedule_id,
        ACTOR_UPDATED = actor, TXN_ID = actual_id,
    })
    if uerr then return fail("update_failed", tostring(uerr)) end
    return finish(actual_id, false)
end

local use_amount = override or sched_amt
local nres, nerr = H.query_sync([[
    SELECT COALESCE(MAX(txn_id), 0) + 1 AS next_id FROM ${SCHEMA}transactions
]], {})
if nerr then return fail("query_failed", tostring(nerr)) end
local txn_id = tonumber(pick(qrows(nres)[1], "next_id")) or 1
local _, herr = H.query_sync([[
    INSERT INTO ${SCHEMA}transactions (
        txn_id, organization_id, status_a2003, kind_a2004, txn_on,
        description, memo, schedule_id, replaces_txn_id,
        calendar_state_a2011, calendar_event_id, calendar_error,
        calendar_attempts, calendar_synced_at, summary, collection,
        valid_after, valid_until, created_id, created_at, updated_id, updated_at
    ) VALUES (
        :TXN_ID, :ORG_ID, 3, 11, CAST(:TXN_ON AS ${DATE}),
        :TXN_DESCRIPTION, NULLIF(CAST(:TXN_MEMO AS ${CAST_TEXT}), ''),
        :SCHEDULE_ID, :REPLACES_TXN_ID,
        1, NULL, NULL,
        0, NULL, NULL, ${JIS}CAST(:TXN_COLLECTION AS ${CAST_TEXT})${JIE},
        NULL, NULL, :ACTOR_CREATED, ${NOW}, :ACTOR_UPDATED, ${NOW}
    )
]], {
    TXN_ID = txn_id, ORG_ID = reserved.organization_id, TXN_ON = reserved.txn_on,
    TXN_DESCRIPTION = description, TXN_MEMO = memo,
    SCHEDULE_ID = reserved.schedule_id, REPLACES_TXN_ID = reserved_id,
    TXN_COLLECTION = "{}", ACTOR_CREATED = actor, ACTOR_UPDATED = actor,
})
if herr then return fail("insert_failed", tostring(herr)) end

local function insert_line(seq, ledger_id, cents)
    local lres, lerr = H.query_sync([[
        SELECT COALESCE(MAX(line_id), 0) + 1 AS next_id FROM ${SCHEMA}lines
    ]], {})
    if lerr then return lerr end
    local line_id = tonumber(pick(qrows(lres)[1], "next_id")) or 1
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
    return ierr
end

local lerr = insert_line(1, to_id, use_amount)
if lerr then return fail("partial_write", tostring(lerr), { txn_id = txn_id }) end
lerr = insert_line(2, from_id, -use_amount)
if lerr then return fail("partial_write", tostring(lerr), { txn_id = txn_id }) end
return finish(txn_id, true)
]==],
                'Match a Reserved row to a Recorded actual',
                0,
                1,
                '{"inputSchema":{"type":"object","properties":{"reserved_txn_id":{"type":"integer"},"amount_cents":{"type":"integer"},"amount_window":{"type":"integer"},"txn_on":{"type":"string"},"actual_txn_id":{"type":"integer"},"description":{"type":"string"},"memo":{"type":"string"}},"required":["reserved_txn_id"],"additionalProperties":true}}',
                '{"title":"Match reserved"}',
                ${COMMON_VALUES}
            );

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Seed Argent MatchReserved'                                                    AS name,
        [=[
            # Forward Migration ${MIGRATION}: Argent.MatchReserved

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
              AND script_name IN ('MatchReserved');

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Remove Argent MatchReserved'                                                  AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Remove Argent.MatchReserved

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
        'Diagram Argent MatchReserved'                                                 AS name,
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
                        "object_id": "script.Argent.MatchReserved",
                        "object_ref": "${MIGRATION}",
                        "name": "Argent.MatchReserved"
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
