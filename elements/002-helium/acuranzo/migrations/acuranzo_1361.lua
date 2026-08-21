-- Migration: acuranzo_1361.lua
-- PRIORITIZE 2.18: seed Stripe.Refund (not invokable)
--
-- Operator/DQM until Course Manager + 2.23. Refunds a completed
-- order via Stripe, records why, revokes Helium seats for that
-- order, Canvas-unenrolls. invokable=0.
--
-- luacheck: no max line length
-- luacheck: no unused args
--
-- CHANGELOG
-- 1.0.0 - 2026-08-21 - Seed Stripe.Refund (PRIORITIZE 2.18)

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "scripts"
cfg.MIGRATION = "1361"
cfg.GROUP_NAME = "Stripe"
cfg.SCRIPT_NAME = "Refund"
-- ----------------------------------------------------------------------------
-- Forward: seed Stripe.Refund (not invokable)
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
                ${COMMON_FIELDS}
            )
            VALUES (
                '${GROUP_NAME}',
                '${SCRIPT_NAME}',
                1,
                NULL, NULL, NULL, NULL,
                1,
                [==[
-- Stripe.Refund (PRIORITIZE 2.18)
-- invokable=0. Operator/DQM (later Course Manager + 2.23).
-- Params: order_id, reason (bind '' never nil).
-- Completed orders only. Stripe first, then Helium revoke, then
-- Canvas unenroll. Secrets: os.getenv("STRIPE_SECRET_KEY").
-- Never log sk_ / Authorization.

local STRIPE_VERSION = "2026-07-29.dahlia"
local STRIPE_BASE = "https://api.stripe.com/v1"
local LOG_TAG = "Stripe.Refund"

local function getenv(k, default)
    if type(os) == "table" and type(os.getenv) == "function" then
        local v = os.getenv(k)
        if v and v ~= "" then return v end
    end
    return default
end

local function qrows(res)
    if type(res) ~= "table" then return nil end
    if type(res.rows) == "table" then return res.rows end
    return res
end

local function fail(code, message)
    H.set_result_json({ ok = false, code = code, message = message or code })
    return 0
end

local function pick(row, a, b)
    if not row then return nil end
    local v = row[a]
    if v == nil and b then v = row[b] end
    return v
end

local function nzi(v)
    local n = tonumber(v)
    if not n or n == 0 then return 0 end
    return n
end

local function str_or_empty(v)
    if v == nil then return "" end
    local s = tostring(v)
    if s == "null" or s == "NULL" then return "" end
    return s
end

local function url_encode(s)
    s = tostring(s or "")
    return (s:gsub("([^%w%-_%.~])", function(c)
        return string.format("%%%02X", string.byte(c))
    end))
end

local function form_encode(fields)
    local parts = {}
    for i = 1, #fields do
        local kv = fields[i]
        parts[#parts + 1] = url_encode(kv[1]) .. "=" .. url_encode(kv[2])
    end
    return table.concat(parts, "&")
end

local function json_string(body, key)
    if type(body) ~= "string" then return nil end
    return body:match('"' .. key .. '"%s*:%s*"([^"]*)"')
end

local function try_log_event(opts)
    local ok, mod = pcall(require, "Enroll.LogEvent")
    if not ok or type(mod) ~= "table" or type(mod.record) ~= "function" then
        H.log.warn("%s: LogEvent unavailable: %s", LOG_TAG, tostring(mod))
        return
    end
    local r = mod.record(opts)
    if type(r) == "table" and r.ok == false then
        H.log.warn("%s: LogEvent: %s", LOG_TAG, tostring(r.code or r.message))
    end
end

local SECRET = getenv("STRIPE_SECRET_KEY", nil)

local function stripe_headers(idem)
    local h = {
        Authorization = "Bearer " .. SECRET,
        Accept = "application/json",
        ["Stripe-Version"] = STRIPE_VERSION,
    }
    if idem and idem ~= "" then
        h["Idempotency-Key"] = idem
    end
    return h
end

local function stripe_call(method, path, body, idem)
    local url = STRIPE_BASE .. path
    local delays = { 200, 800, 2000 }
    local last_err = "stripe_http"
    for attempt = 1, 3 do
        local res, err
        if method == "GET" then
            res, err = H.http.get_sync(url, stripe_headers(nil), { timeout = 15 })
        else
            res, err = H.http.post_sync(url, body or "", stripe_headers(idem), {
                timeout = 15,
                content_type = "application/x-www-form-urlencoded",
            })
        end
        if err then
            last_err = tostring(err)
            H.log.error("%s HTTP %s failed: endpoint=%s type=network attempt=%s",
                LOG_TAG, method, path, tostring(attempt))
        elseif res then
            local status = tonumber(res.status) or 0
            if status == 429 or status >= 500 then
                last_err = "http_" .. tostring(status)
                H.log.error("%s HTTP %s failed: endpoint=%s type=%s attempt=%s",
                    LOG_TAG, method, path, last_err, tostring(attempt))
            else
                return res, nil
            end
        end
        if attempt < 3 then
            H.sleep(delays[attempt])
        end
    end
    return nil, last_err
end

if type(params) ~= "table" then
    return fail("validation", "Missing params")
end

local order_id = tonumber(params.order_id)
if not order_id or order_id < 1 then
    return fail("validation", "order_id is required")
end

local reason = str_or_empty(params.reason)
if #reason > 500 then
    reason = reason:sub(1, 500)
end

if not SECRET or SECRET == "" then
    return fail("stripe_unconfigured", "Stripe is temporarily unavailable")
end

local _or, qerr = H.query_sync([[
    SELECT order_id, account_id, stripe_intent_id, status, currency, total_cents,
           stripe_refund_id
    FROM ${SCHEMA}orders
    WHERE order_id = :ORDERID
]], { ORDERID = order_id })
if qerr then
    H.log.warn("%s lookup err: %s", LOG_TAG, tostring(qerr))
    return fail("lookup_error", "Could not load order")
end
local rows = qrows(_or)
if not rows or not rows[1] then
    return fail("not_found", "Order not found")
end
local order = rows[1]
local status = string.lower(str_or_empty(pick(order, "status", "STATUS")))
local account_id = nzi(pick(order, "account_id", "ACCOUNT_ID"))
local pi = str_or_empty(pick(order, "stripe_intent_id", "STRIPE_INTENT_ID"))
local existing_re = str_or_empty(pick(order, "stripe_refund_id", "STRIPE_REFUND_ID"))

if status == "refunded" or existing_re ~= "" then
    H.set_result_json({
        ok = true,
        already = true,
        order_id = order_id,
        stripe_refund_id = existing_re,
    })
    return 0
end

if status ~= "completed" then
    return fail("not_refundable", "Order is not completed")
end
if pi == "" or pi:sub(1, 3) ~= "pi_" then
    return fail("missing_intent", "Order has no PaymentIntent")
end

local idem = "refund:" .. tostring(order_id)
local fields = {
    { "payment_intent", pi },
    { "reason", "requested_by_customer" },
    { "metadata[order_id]", tostring(order_id) },
    { "metadata[helium_reason]", reason },
}
local res, serr = stripe_call("POST", "/refunds", form_encode(fields), idem)
if serr then
    return fail("stripe_http", "Could not refund")
end
if res.status < 200 or res.status >= 300 then
    local err_type = json_string(res.body or "", "type") or ("http_" .. tostring(res.status))
    H.log.error("%s HTTP POST failed: endpoint=/refunds type=%s", LOG_TAG, err_type)
    return fail("stripe_refund_failed", "Could not refund")
end
local re_id = json_string(res.body or "", "id") or ""
if re_id:sub(1, 3) ~= "re_" then
    return fail("stripe_refund_no_id", "Could not refund")
end

local _, werr = H.query_sync([[
    UPDATE ${SCHEMA}orders
       SET status = 'refunded',
           refunded_at = NOW(),
           refund_reason = :REASON,
           stripe_refund_id = :REID,
           updated_at = NOW()
     WHERE order_id = :ORDERID
]], { ORDERID = order_id, REASON = reason, REID = re_id })
if werr then
    H.log.warn("%s order write err: %s", LOG_TAG, tostring(werr))
    return fail("write_error", "Refunded in Stripe but could not record")
end

local canvas_user_id = 0
local _lr = H.query_sync([[
    SELECT canvas_user_id
    FROM ${SCHEMA}account_canvas_links
    WHERE account_id = :ACCOUNTID
]], { ACCOUNTID = account_id })
local lrows = qrows(_lr)
if lrows and lrows[1] then
    canvas_user_id = nzi(pick(lrows[1], "canvas_user_id", "CANVAS_USER_ID"))
end

local ok_seat, seat = pcall(require, "Enroll.CanvasSeat")
if not ok_seat then
    seat = nil
end

local _er = H.query_sync([[
    SELECT enrollment_id, course_id, canvas_course_id, canvas_enrollment_id
    FROM ${SCHEMA}user_enrollments
    WHERE order_id = :ORDERID
      AND account_id = :ACCOUNTID
      AND status IN ('active', 'completed')
]], { ORDERID = order_id, ACCOUNTID = account_id })
local erows = qrows(_er) or {}
local revoked = 0
for _, erow in ipairs(erows) do
    local eid = nzi(pick(erow, "enrollment_id", "ENROLLMENT_ID"))
    local course_id = nzi(pick(erow, "course_id", "COURSE_ID"))
    local cid = nzi(pick(erow, "canvas_course_id", "CANVAS_COURSE_ID"))
    local ceid = nzi(pick(erow, "canvas_enrollment_id", "CANVAS_ENROLLMENT_ID"))
    if type(seat) == "table" and type(seat.unenroll) == "function" then
        local u = seat.unenroll({
            canvas_user_id = canvas_user_id,
            canvas_course_id = cid,
            canvas_enrollment_id = ceid,
        })
        if type(u) ~= "table" or u.ok ~= true then
            H.log.warn("%s canvas unenroll failed enrollment_id=%s code=%s",
                LOG_TAG, tostring(eid), tostring(u and u.code))
        end
    end
    local _, uerr = H.query_sync([[
        UPDATE ${SCHEMA}user_enrollments
           SET status = 'revoked',
               canvas_enrollment_id = NULL,
               updated_at = NOW()
         WHERE enrollment_id = :EID
           AND account_id = :ACCOUNTID
    ]], { EID = eid, ACCOUNTID = account_id })
    if uerr then
        H.log.warn("%s enroll write err enrollment_id=%s: %s",
            LOG_TAG, tostring(eid), tostring(uerr))
    else
        revoked = revoked + 1
        try_log_event({
            account_id = account_id,
            course_id = course_id,
            canvas_user_id = canvas_user_id,
            canvas_course_id = cid,
            enrollment_id = eid,
            event_type = "revoked",
            actor = "ops",
        })
    end
end

H.set_result_json({
    ok = true,
    already = false,
    order_id = order_id,
    stripe_refund_id = re_id,
    revoked = revoked,
})
return 0
                ]==],
                'PRIORITIZE 2.18: Stripe refund + revoke seats',
                0,
                ${COMMON_VALUES}
            );

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Seed Stripe.Refund script'                                         AS name,
        [=[
            # Forward Migration ${MIGRATION}: Seed Stripe.Refund

            `invokable=0`. DQM / Course Manager. Params `order_id` +
            `reason`. Stripe refund, `orders.status=refunded`, Helium
            seats `status=revoked`, Canvas unenroll. No diagram.
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
            WHERE group_name = 'Stripe'
              AND script_name = 'Refund';

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Remove Stripe.Refund script'                                       AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Remove Stripe.Refund
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]====]})

return queries end
