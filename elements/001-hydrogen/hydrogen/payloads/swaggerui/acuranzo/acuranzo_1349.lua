-- Migration: acuranzo_1349.lua
-- PRIORITIZE 2.35: seed Catalog.Retire (not invokable)
--
-- action=retire|unretire. SET courses.retired. Paid courses with a
-- stripe_product_id POST Product active=false|true (do not delete).
-- Lithium write only after Stripe succeeds (or there is no Product).
-- LogEvent retired/unretired. invokable=0 until Course Manager + 2.23.
--
-- luacheck: no max line length
-- luacheck: no unused args
--
-- CHANGELOG
-- 1.0.0 - 2026-08-21 - Seed Catalog.Retire (PRIORITIZE 2.35)

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "scripts"
cfg.MIGRATION = "1349"
cfg.GROUP_NAME = "Catalog"
cfg.SCRIPT_NAME = "Retire"
-- ----------------------------------------------------------------------------
-- Forward: seed Catalog.Retire (not invokable)
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
-- Catalog.Retire (PRIORITIZE 2.35)
-- invokable=0. Operator/DQM (later Course Manager + 2.23).
-- Params: course_id, action=retire|unretire.
-- SET courses.retired. Do not unenroll Canvas. Do not delete Stripe.
-- Paid + stripe_product_id: POST /v1/products/:id active=false|true
-- before the Lithium write. Free / missing Product skip Stripe.
-- Idempotent. Secrets: os.getenv("STRIPE_SECRET_KEY"). Never log sk_.

local STRIPE_VERSION = "2026-07-29.dahlia"
local STRIPE_BASE = "https://api.stripe.com/v1"
local LOG_TAG = "Catalog.Retire"

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
            H.log.error("%s HTTP %s failed: endpoint=%s idempotency=%s type=network attempt=%s",
                LOG_TAG, method, path, tostring(idem or ""), tostring(attempt))
        elseif res then
            local status = tonumber(res.status) or 0
            if status == 429 or status >= 500 then
                last_err = "http_" .. tostring(status)
                H.log.error("%s HTTP %s failed: endpoint=%s idempotency=%s type=%s attempt=%s",
                    LOG_TAG, method, path, tostring(idem or ""), last_err, tostring(attempt))
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

local function set_product_active(prod, active, idem)
    local flag = active and "true" or "false"
    local res, err = stripe_call(
        "POST",
        "/products/" .. url_encode(prod),
        form_encode({ { "active", flag } }),
        idem)
    if err then return nil, err end
    if res.status >= 200 and res.status < 300 then
        local id = json_string(res.body or "", "id")
        if id and id:sub(1, 5) == "prod_" then return id, nil end
        return prod, nil
    end
    local err_type = json_string(res.body or "", "type") or ("http_" .. tostring(res.status))
    local code = json_string(res.body or "", "code")
    if res.status == 404 or code == "resource_missing" then
        return nil, "resource_missing"
    end
    H.log.error("%s HTTP POST failed: endpoint=/products/:id idempotency=%s type=%s",
        LOG_TAG, tostring(idem or ""), err_type)
    return nil, err_type
end

local function try_log_event(opts)
    local ok, mod = pcall(require, "Catalog.LogEvent")
    if not ok or type(mod) ~= "table" or type(mod.record) ~= "function" then
        H.log.warn("%s LogEvent unavailable: %s", LOG_TAG, tostring(mod))
        return
    end
    local r = mod.record(opts)
    if type(r) == "table" and r.ok == false then
        H.log.warn("%s LogEvent: %s", LOG_TAG, tostring(r.code or r.message))
    end
end

if type(params) ~= "table" then
    return fail("validation", "Missing params")
end

local course_id = tonumber(params.course_id or params.COURSE_ID)
if not course_id then
    return fail("validation", "course_id is required")
end

local action = string.lower(tostring(params.action or params.ACTION or "retire"))
if action ~= "retire" and action ~= "unretire" then
    return fail("validation", "action must be retire or unretire")
end

local want = (action == "retire") and 1 or 0

local _cr, cerr = H.query_sync([[
    SELECT course_id, pricing_type, retired, stripe_product_id, canvas_course_id
      FROM ${SCHEMA}courses
     WHERE course_id = :COURSEID
]], { COURSEID = course_id })
if cerr then
    return fail("course_error", "Could not load course")
end
local rows = qrows(_cr)
if not rows or not rows[1] then
    return fail("course_not_found", "Course not found")
end
local course = rows[1]
local already = tonumber(pick(course, "retired", "RETIRED")) or 0
local prod = str_or_empty(pick(course, "stripe_product_id", "STRIPE_PRODUCT_ID"))
local pricing = string.lower(tostring(pick(course, "pricing_type", "PRICING_TYPE") or ""))
local canvas_id = tonumber(pick(course, "canvas_course_id", "CANVAS_COURSE_ID"))
local stripe_ok = true
local stripe_skipped = true

if prod ~= "" and pricing == "paid" then
    stripe_skipped = false
    if not SECRET or SECRET == "" then
        H.log.error("%s: STRIPE_SECRET_KEY not set in environment", LOG_TAG)
        return fail("stripe_unconfigured", "Payments are temporarily unavailable")
    end
    local idem = string.format("helium-catalog-%s-%s", action, tostring(course_id))
    local _, serr = set_product_active(prod, want == 0, idem)
    if serr == "resource_missing" then
        H.log.warn("%s: product missing course_id=%s prod=%s; continuing",
            LOG_TAG, tostring(course_id), prod:sub(1, 8))
        stripe_ok = false
    elseif serr then
        return fail("stripe_product_failed", "Could not update Stripe Product")
    end
end

if already ~= want then
    local _, uerr = H.query_sync([[
        UPDATE ${SCHEMA}courses
           SET retired = :RETIRED,
               updated_at = NOW(),
               updated_id = 0
         WHERE course_id = :COURSEID
    ]], { COURSEID = course_id, RETIRED = want })
    if uerr then
        return fail("write_error", "Could not update course")
    end
    try_log_event({
        course_id = course_id,
        canvas_course_id = canvas_id,
        event_type = action == "retire" and "retired" or "unretired",
        actor = "system",
        detail = tostring(want),
    })
end

H.set_result_json({
    ok = true,
    course_id = course_id,
    action = action,
    retired = want,
    unchanged = already == want,
    stripe_skipped = stripe_skipped,
    stripe_ok = stripe_ok,
})
return 0
                ]==],
                'PRIORITIZE 2.35: retire/unretire catalog row (not invokable)',
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
        'Seed Catalog.Retire (not invokable)'                               AS name,
        [=[
            # Forward Migration ${MIGRATION}: Seed Catalog.Retire

            Inserts `Catalog.Retire` with `invokable = 0` (PRIORITIZE 2.35).
            Operator/DQM until Course Manager + 2.23.

            `action=retire|unretire` plus `course_id`. Sets
            `courses.retired`. Existing Canvas seats stay. Stripe
            Product is deactivated (retire) or reactivated (unretire)
            when `stripe_product_id` is set; never deleted. Lithium
            flips only after Stripe succeeds (or there is no Product).
            Appends `catalog_events` via `Catalog.LogEvent`.
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
            WHERE group_name = 'Catalog'
              AND script_name = 'Retire';

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Remove Catalog.Retire script'                                      AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Remove Catalog.Retire
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]====]})

return queries end
