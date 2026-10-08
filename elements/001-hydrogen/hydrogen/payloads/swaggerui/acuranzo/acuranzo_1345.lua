-- Migration: acuranzo_1345.lua
-- PRIORITIZE 2.34: seed Stripe.EnsureProduct (not invokable)
--
-- Paid courses only. Create missing Stripe Product + Prices from
-- Lithium title / course_prices. action=rotate mints a new Price
-- when Lithium unit_amount_cents differs; old Price stays (receipts).
-- Never archive/delete Stripe objects. invokable=0 until Course
-- Manager + 2.23. Operator/DQM runs it; Part 5 is the button.
--
-- luacheck: no max line length
-- luacheck: no unused args
--
-- CHANGELOG
-- 1.0.0 - 2026-08-21 - Seed Stripe.EnsureProduct (PRIORITIZE 2.34)

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "scripts"
cfg.MIGRATION = "1345"
cfg.GROUP_NAME = "Stripe"
cfg.SCRIPT_NAME = "EnsureProduct"
-- ----------------------------------------------------------------------------
-- Forward: seed Stripe.EnsureProduct (not invokable)
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
-- Stripe.EnsureProduct (PRIORITIZE 2.34)
-- invokable=0. Operator/DQM (later Course Manager + 2.23).
-- Params: course_id and/or action=ensure|rotate|all.
-- Paid + course_prices only. Free never calls Stripe.
-- Idempotent on existing prod_ / price_ ids. Rotate compares
-- Stripe unit_amount to Lithium cents and mints a new Price.
-- Unpublish must not archive Stripe (2.29). Retire is 2.35.
-- Secrets: os.getenv("STRIPE_SECRET_KEY"). Never log sk_ / Authorization.

local STRIPE_VERSION = "2026-07-29.dahlia"
local STRIPE_BASE = "https://api.stripe.com/v1"
local LOG_TAG = "Stripe.EnsureProduct"

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

local function trim(s)
    s = tostring(s or "")
    return (s:gsub("^%s+", ""):gsub("%s+$", ""))
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

local function json_number(body, key)
    if type(body) ~= "string" then return nil end
    return tonumber(body:match('"' .. key .. '"%s*:%s*(%-?%d+)'))
end

local function json_prod_id(body)
    local id = json_string(body, "id")
    if id and id:sub(1, 5) == "prod_" then return id end
    return nil
end

local function json_price_id(body)
    local id = json_string(body, "id")
    if id and id:sub(1, 6) == "price_" then return id end
    return nil
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

local function retrieve_product(prod)
    local res, err = stripe_call("GET", "/products/" .. url_encode(prod), nil, nil)
    if err then return nil, err end
    if res.status == 200 then
        return json_prod_id(res.body or ""), nil
    end
    local err_type = json_string(res.body or "", "type") or ("http_" .. tostring(res.status))
    local code = json_string(res.body or "", "code")
    if res.status == 404 or code == "resource_missing" then
        return nil, "resource_missing"
    end
    H.log.error("%s HTTP GET failed: endpoint=/products/:id type=%s", LOG_TAG, err_type)
    return nil, err_type
end

local function create_product(title, course_id, code, idem)
    local fields = {
        { "name", title },
        { "metadata[helium_course_id]", tostring(course_id) },
    }
    if code and code ~= "" then
        fields[#fields + 1] = { "metadata[code]", code }
    end
    local res, err = stripe_call("POST", "/products", form_encode(fields), idem)
    if err then return nil, err end
    if res.status >= 200 and res.status < 300 then
        local id = json_prod_id(res.body or "")
        if not id then return nil, "stripe_create_no_id" end
        return id, nil
    end
    local err_type = json_string(res.body or "", "type") or ("http_" .. tostring(res.status))
    H.log.error("%s HTTP POST failed: endpoint=/products idempotency=%s type=%s",
        LOG_TAG, tostring(idem or ""), err_type)
    return nil, err_type
end

local function retrieve_price(price_id)
    local res, err = stripe_call("GET", "/prices/" .. url_encode(price_id), nil, nil)
    if err then return nil, nil, err end
    if res.status == 200 then
        local id = json_price_id(res.body or "")
        local cents = json_number(res.body or "", "unit_amount")
        if not id then return nil, nil, "stripe_retrieve_no_id" end
        return id, cents, nil
    end
    local err_type = json_string(res.body or "", "type") or ("http_" .. tostring(res.status))
    local code = json_string(res.body or "", "code")
    if res.status == 404 or code == "resource_missing" then
        return nil, nil, "resource_missing"
    end
    H.log.error("%s HTTP GET failed: endpoint=/prices/:id type=%s", LOG_TAG, err_type)
    return nil, nil, err_type
end

local function create_price(prod, currency, cents, course_id, idem)
    local fields = {
        { "product", prod },
        { "currency", currency },
        { "unit_amount", tostring(cents) },
        { "metadata[helium_course_id]", tostring(course_id) },
        { "metadata[currency]", currency },
    }
    local res, err = stripe_call("POST", "/prices", form_encode(fields), idem)
    if err then return nil, err end
    if res.status >= 200 and res.status < 300 then
        local id = json_price_id(res.body or "")
        if not id then return nil, "stripe_create_no_id" end
        return id, nil
    end
    local err_type = json_string(res.body or "", "type") or ("http_" .. tostring(res.status))
    H.log.error("%s HTTP POST failed: endpoint=/prices idempotency=%s type=%s",
        LOG_TAG, tostring(idem or ""), err_type)
    return nil, err_type
end

local function store_product(course_id, prod)
    local _, err = H.query_sync([[
        UPDATE ${SCHEMA}courses
           SET stripe_product_id = :PRODID,
               updated_at = NOW(),
               updated_id = 0
         WHERE course_id = :COURSEID
    ]], { COURSEID = course_id, PRODID = prod })
    return err
end

local function store_price(price_id, stripe_price_id)
    local _, err = H.query_sync([[
        UPDATE ${SCHEMA}course_prices
           SET stripe_price_id = :STRIPEPRICE,
               updated_at = NOW(),
               updated_id = 0
         WHERE price_id = :PRICEID
    ]], { PRICEID = price_id, STRIPEPRICE = stripe_price_id })
    return err
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

local function load_course(course_id)
    local _cr, cerr = H.query_sync([[
        SELECT course_id, code, slug, title, pricing_type,
               stripe_product_id, canvas_course_id
          FROM ${SCHEMA}courses
         WHERE course_id = :COURSEID
    ]], { COURSEID = course_id })
    if cerr then
        return nil, "course_error"
    end
    local rows = qrows(_cr)
    if not rows or not rows[1] then
        return nil, "course_not_found"
    end
    return rows[1], nil
end

local function load_prices(course_id)
    local _pr, perr = H.query_sync([[
        SELECT price_id, course_id, currency, unit_amount_cents,
               stripe_price_id, is_active
          FROM ${SCHEMA}course_prices
         WHERE course_id = :COURSEID
         ORDER BY currency
    ]], { COURSEID = course_id })
    if perr then
        return nil, "price_error"
    end
    return qrows(_pr) or {}, nil
end

local function ensure_one(row, rotate)
    local course_id = nzi(pick(row, "course_id", "COURSE_ID"))
    local pricing = trim(str_or_empty(pick(row, "pricing_type", "PRICING_TYPE"))):lower()
    local title = trim(str_or_empty(pick(row, "title", "TITLE")))
    local code = trim(str_or_empty(pick(row, "code", "CODE")))
    local canvas_course = nzi(pick(row, "canvas_course_id", "CANVAS_COURSE_ID"))
    local out = {
        course_id = course_id,
        stripe_product_id = "",
        product_created = false,
        prices = {},
    }

    if pricing ~= "paid" then
        return "skip", "free", out
    end
    if title == "" then
        return "skip", "empty_title", out
    end

    local prices, perr = load_prices(course_id)
    if not prices then
        return "error", perr, out
    end
    if #prices == 0 then
        return "skip", "missing_prices", out
    end

    local existing = trim(str_or_empty(pick(row, "stripe_product_id", "STRIPE_PRODUCT_ID")))
    local prod = nil
    local product_created = false
    if existing ~= "" then
        local got, rerr = retrieve_product(existing)
        if got then
            prod = got
        elseif rerr ~= "resource_missing" then
            return "error", "stripe_retrieve_failed", out
        else
            H.log.warn("%s resource_missing prod for course_id=%s", LOG_TAG, tostring(course_id))
        end
    end
    if not prod then
        local idem = "course:" .. tostring(course_id) .. ":product"
        if existing ~= "" then
            idem = idem .. ":replace:" .. existing
        end
        local created, cerr = create_product(title, course_id, code, idem)
        if not created then
            return "error", cerr or "stripe_create_failed", out
        end
        local serr = store_product(course_id, created)
        if serr then
            H.log.warn("%s store product err: %s", LOG_TAG, tostring(serr))
            return "error", "store_failed", out
        end
        prod = created
        product_created = true
        try_log_event({
            course_id = course_id,
            canvas_course_id = canvas_course,
            event_type = "stripe_product",
            actor = "system",
            detail = created,
        })
    end
    out.stripe_product_id = prod
    out.product_created = product_created

    local price_errors = 0
    for i = 1, #prices do
        local pr = prices[i]
        local price_id = nzi(pick(pr, "price_id", "PRICE_ID"))
        local currency = trim(str_or_empty(pick(pr, "currency", "CURRENCY"))):lower()
        local cents = nzi(pick(pr, "unit_amount_cents", "UNIT_AMOUNT_CENTS"))
        local existing_price = trim(str_or_empty(pick(pr, "stripe_price_id", "STRIPE_PRICE_ID")))
        local entry = {
            currency = currency,
            stripe_price_id = existing_price,
            created = false,
            rotated = false,
        }
        if currency == "" or cents <= 0 or price_id == 0 then
            price_errors = price_errors + 1
            entry.error = "invalid_price_row"
            out.prices[#out.prices + 1] = entry
        else
            local keep = nil
            local stripe_cents = nil
            if existing_price ~= "" then
                local got, got_cents, rerr = retrieve_price(existing_price)
                if got then
                    keep = got
                    stripe_cents = got_cents
                elseif rerr ~= "resource_missing" then
                    price_errors = price_errors + 1
                    entry.error = "stripe_retrieve_failed"
                    out.prices[#out.prices + 1] = entry
                else
                    H.log.warn("%s resource_missing price for course_id=%s currency=%s",
                        LOG_TAG, tostring(course_id), currency)
                end
            end
            local need_new = (keep == nil)
            local rotating = false
            if keep and rotate and stripe_cents ~= nil and stripe_cents ~= cents then
                need_new = true
                rotating = true
            end
            if not need_new then
                entry.stripe_price_id = keep
                out.prices[#out.prices + 1] = entry
            else
                local idem = "course:" .. tostring(course_id) .. ":price:" .. currency .. ":" .. tostring(cents)
                if rotating and existing_price ~= "" then
                    idem = idem .. ":from:" .. existing_price
                elseif existing_price ~= "" and not keep then
                    idem = idem .. ":replace:" .. existing_price
                end
                local created, cerr = create_price(prod, currency, cents, course_id, idem)
                if not created then
                    price_errors = price_errors + 1
                    entry.error = cerr or "stripe_create_failed"
                    out.prices[#out.prices + 1] = entry
                else
                    local serr = store_price(price_id, created)
                    if serr then
                        H.log.warn("%s store price err: %s", LOG_TAG, tostring(serr))
                        price_errors = price_errors + 1
                        entry.error = "store_failed"
                        out.prices[#out.prices + 1] = entry
                    else
                        entry.stripe_price_id = created
                        entry.created = not rotating
                        entry.rotated = rotating
                        out.prices[#out.prices + 1] = entry
                        try_log_event({
                            course_id = course_id,
                            canvas_course_id = canvas_course,
                            event_type = rotating and "price_rotate" or "stripe_price",
                            actor = "system",
                            detail = currency .. " " .. tostring(cents) .. " " .. created,
                        })
                    end
                end
            end
        end
    end

    if price_errors > 0 then
        return "error", "price_failed", out
    end
    return "ok", nil, out
end

H.set_current_state("start")

if type(params) ~= "table" then
    return fail("validation", "Missing params")
end

if not SECRET or SECRET == "" then
    H.log.error("%s STRIPE_SECRET_KEY not set in environment", LOG_TAG)
    return fail("stripe_unconfigured", "Payments are temporarily unavailable")
end

local action = trim(str_or_empty(params.action or params.ACTION)):lower()
if action == "" then action = "ensure" end
if action ~= "ensure" and action ~= "rotate" and action ~= "all" then
    return fail("validation", "action must be ensure, rotate, or all")
end

local course_id = nzi(params.course_id or params.COURSE_ID)
local rotate = (action == "rotate")
local do_all = (action == "all") or (action == "rotate" and course_id == 0)

if not do_all and course_id == 0 then
    return fail("validation", "course_id or action=all required")
end

local rows = {}
if do_all then
    local _ar, aerr = H.query_sync([[
        SELECT course_id, code, slug, title, pricing_type,
               stripe_product_id, canvas_course_id
          FROM ${SCHEMA}courses
         WHERE pricing_type = 'paid'
         ORDER BY course_id
    ]], {})
    if aerr then
        H.log.warn("%s list err: %s", LOG_TAG, tostring(aerr))
        return fail("course_error", "Could not list courses")
    end
    rows = qrows(_ar) or {}
else
    local row, lerr = load_course(course_id)
    if not row then
        return fail(lerr or "course_not_found", "Course not found")
    end
    rows = { row }
end

local ensured = 0
local skipped = 0
local errors = 0
local results = {}
for i = 1, #rows do
    local status, reason, detail = ensure_one(rows[i], rotate)
    detail.status = status
    if reason then detail.reason = reason end
    results[#results + 1] = detail
    if status == "ok" then
        ensured = ensured + 1
    elseif status == "skip" then
        skipped = skipped + 1
    else
        errors = errors + 1
        H.log.warn("%s course=%s %s", LOG_TAG,
            tostring(detail.course_id), tostring(reason))
    end
end

H.set_current_state("done")
if not do_all then
    local one = results[1]
    if not one then
        return fail("course_not_found", "Course not found")
    end
    if one.status == "error" then
        return fail(one.reason or "stripe_failed", "Could not ensure Stripe product")
    end
    if one.status == "skip" and one.reason == "free" then
        H.set_result_json({
            ok = true,
            skipped = true,
            code = "free",
            course_id = one.course_id,
        })
        return 0
    end
    if one.status == "skip" then
        H.set_result_json({
            ok = true,
            skipped = true,
            code = one.reason,
            course_id = one.course_id,
        })
        return 0
    end
    H.set_result_json({
        ok = true,
        course_id = one.course_id,
        stripe_product_id = one.stripe_product_id,
        product_created = one.product_created,
        prices = one.prices,
        rotated = rotate,
    })
    return 0
end

H.set_result_json({
    ok = true,
    scanned = #rows,
    ensured = ensured,
    skipped = skipped,
    errors = errors,
    rotated = rotate,
})
return 0
                ]==],
                'PRIORITIZE 2.34: Stripe Product + Price ensure/rotate (invokable=0)',
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
        'Seed Stripe.EnsureProduct script'                                  AS name,
        [=[
            # Forward Migration ${MIGRATION}: Seed Stripe.EnsureProduct

            Inserts `Stripe.EnsureProduct` with `invokable = 0`.
            Operator/DQM invoke until Course Manager + 2.23.

            Params: `course_id` and/or `action=ensure|rotate|all`.
            Default action is ensure. `action=all` walks paid courses.
            `action=rotate` without `course_id` rotates all paid.

            For each paid row with `course_prices`: create missing
            Stripe Product from Lithium title; create one Price per
            currency row; write `prod_` / `price_` back. Existing ids
            are retrieved and left alone (idempotent). Rotate GETs the
            Price and, when `unit_amount` differs from Lithium cents,
            POSTs a new Price and updates `stripe_price_id`. Old Price
            stays on Stripe for receipts. Free courses skip Stripe.
            Never archive or delete Product/Prices.

            Real id writes append `catalog_events` via
            `Catalog.LogEvent` (wiki best-effort).
            `STRIPE_SECRET_KEY` from env. No diagram (data seed).
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
              AND script_name = 'EnsureProduct';

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Remove Stripe.EnsureProduct script'                                AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Remove Stripe.EnsureProduct
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]====]})

return queries end
