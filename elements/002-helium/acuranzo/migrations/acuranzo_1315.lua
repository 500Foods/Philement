-- Migration: acuranzo_1315.lua
-- STRIPE_PLAN Phase 5: seed Stripe.Checkout (invokable)
--
-- JWT POST /api/conduit/script. PaymentIntent + client_secret.
-- Re-reads course_prices. Does not trust client cents.

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.0.0 - 2026-08-13 - Seed Stripe.Checkout invokable=1 (STRIPE_PLAN Phase 5)

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "scripts"
cfg.MIGRATION = "1315"
cfg.GROUP_NAME = "Stripe"
cfg.SCRIPT_NAME = "Checkout"
-- ----------------------------------------------------------------------------
-- Forward: seed Stripe.Checkout (invokable)
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
-- Stripe.Checkout (STRIPE_PLAN Phase 5; 1317 bind fix)
-- JWT POST /api/conduit/script. Identity from params._hydrogen.
-- Input: cart.checkoutPayload().params { currency, items[] }.
-- Creates a PaymentIntent; returns client_secret. Fulfill on webhook.
-- Secrets: os.getenv("STRIPE_SECRET_KEY"). Never log sk_ / client_secret.

local STRIPE_VERSION = "2026-07-29.dahlia"
local STRIPE_BASE = "https://api.stripe.com/v1"
local CURRENCIES = { cad = true, usd = true, eur = true, gbp = true }

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

local function json_escape(s)
    s = tostring(s or "")
    return (s:gsub("\\", "\\\\"):gsub('"', '\\"'))
end

local function items_json(lines)
    local parts = {}
    for i = 1, #lines do
        local it = lines[i]
        local price = "null"
        if it.stripePriceId and it.stripePriceId ~= "" then
            price = '"' .. json_escape(it.stripePriceId) .. '"'
        end
        local canvas = "null"
        if it.canvasCourseId then
            canvas = tostring(it.canvasCourseId)
        end
        parts[i] = string.format(
            '{"courseId":%s,"currency":"%s","unitAmountCents":%s,"stripePriceId":%s,"canvasCourseId":%s,"pricingType":"%s","lineType":"%s"}',
            tostring(it.courseId),
            json_escape(it.currency),
            tostring(it.unitAmountCents),
            price,
            canvas,
            json_escape(it.pricingType),
            json_escape(it.lineType)
        )
    end
    return "[" .. table.concat(parts, ",") .. "]"
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
            H.log.error("Stripe.Checkout HTTP %s failed: endpoint=%s idempotency=%s type=network attempt=%s",
                method, path, tostring(idem or ""), tostring(attempt))
        elseif res then
            local status = tonumber(res.status) or 0
            if status == 429 or status >= 500 then
                last_err = "http_" .. tostring(status)
                H.log.error("Stripe.Checkout HTTP %s failed: endpoint=%s idempotency=%s type=%s attempt=%s",
                    method, path, tostring(idem or ""), last_err, tostring(attempt))
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

local function retrieve_customer(cus)
    local res, err = stripe_call("GET", "/customers/" .. url_encode(cus), nil, nil)
    if err then return nil, err end
    if res.status == 200 then
        local id = json_string(res.body or "", "id")
        if id and id:sub(1, 4) == "cus_" then return id, nil end
    end
    if res.status == 404 then return nil, "resource_missing" end
    local code = json_string(res.body or "", "code")
    if code == "resource_missing" then return nil, "resource_missing" end
    return nil, json_string(res.body or "", "type") or ("http_" .. tostring(res.status))
end

local function create_customer(email, name, keycloak_sub, account_id)
    local idem = "user:" .. keycloak_sub
    local fields = {
        { "metadata[keycloak_sub]", keycloak_sub },
        { "metadata[internal_user_id]", tostring(account_id) },
    }
    if email and email ~= "" then fields[#fields + 1] = { "email", email } end
    if name and name ~= "" then fields[#fields + 1] = { "name", name } end
    local res, err = stripe_call("POST", "/customers", form_encode(fields), idem)
    if err then return nil, err end
    if res.status >= 200 and res.status < 300 then
        local id = json_string(res.body or "", "id")
        if id and id:sub(1, 4) == "cus_" then return id, nil end
        return nil, "stripe_create_no_id"
    end
    return nil, json_string(res.body or "", "type") or ("http_" .. tostring(res.status))
end

local function store_customer(account_id, cus)
    local _, err = H.query_sync([[
        UPDATE ${SCHEMA}accounts
           SET stripe_customer_id = :CUS,
               updated_at = NOW(),
               updated_id = 0
         WHERE account_id = :ACCOUNTID
    ]], { ACCOUNTID = account_id, CUS = cus })
    return err
end

local function ensure_customer(account_id, h)
    local _ar, aerr = H.query_sync([[
        SELECT account_id, name, stripe_customer_id
        FROM ${SCHEMA}accounts
        WHERE account_id = :ACCOUNTID
    ]], { ACCOUNTID = account_id })
    if aerr then return nil, "account_error" end
    local arows = qrows(_ar)
    if not arows or not arows[1] then return nil, "account_not_found" end
    local acct = arows[1]
    local existing = pick(acct, "stripe_customer_id", "STRIPE_CUSTOMER_ID")
    if existing and existing ~= "" then
        local got, rerr = retrieve_customer(existing)
        if got then return got, nil end
        if rerr ~= "resource_missing" then return nil, "stripe_retrieve_failed" end
    end
    local _or, oerr = H.query_sync([[
        SELECT subject, email
        FROM ${SCHEMA}account_oidc_identities
        WHERE account_id = :ACCOUNTID
        ORDER BY last_seen_at DESC
    ]], { ACCOUNTID = account_id })
    if oerr then
        H.log.warn("Stripe.Checkout: oidc lookup err: %s", tostring(oerr))
    end
    local oidc = qrows(_or) and qrows(_or)[1] or nil
    local keycloak_sub = pick(oidc, "subject", "SUBJECT")
    if not keycloak_sub or keycloak_sub == "" then
        keycloak_sub = "account:" .. tostring(account_id)
    end
    local email = h.email or pick(oidc, "email", "EMAIL")
    local name = pick(acct, "name", "NAME") or h.username or email
    local cus, cerr = create_customer(email, name, keycloak_sub, account_id)
    if not cus then return nil, cerr or "stripe_create_failed" end
    local serr = store_customer(account_id, cus)
    if serr then return nil, "store_failed" end
    return cus, nil
end

local function oidc_sub(account_id)
    local _or = H.query_sync([[
        SELECT subject
        FROM ${SCHEMA}account_oidc_identities
        WHERE account_id = :ACCOUNTID
        ORDER BY last_seen_at DESC
    ]], { ACCOUNTID = account_id })
    local rows = qrows(_or)
    local sub = pick(rows and rows[1], "subject", "SUBJECT")
    if sub and sub ~= "" then return sub end
    return "account:" .. tostring(account_id)
end

local function load_course(course_id)
    local _cr, err = H.query_sync([[
        SELECT course_id, pricing_type, canvas_course_id, stripe_product_id
        FROM ${SCHEMA}courses
        WHERE course_id = :COURSEID
    ]], { COURSEID = course_id })
    if err then return nil, err end
    local rows = qrows(_cr)
    if not rows or not rows[1] then return nil, "not_found" end
    return rows[1], nil
end

local function load_price(course_id, currency)
    local _pr, err = H.query_sync([[
        SELECT unit_amount_cents, stripe_price_id, is_active
        FROM ${SCHEMA}course_prices
        WHERE course_id = :COURSEID
          AND currency = :CURRENCY
          AND is_active = 1
    ]], { COURSEID = course_id, CURRENCY = currency })
    if err then return nil, err end
    local rows = qrows(_pr)
    if not rows or not rows[1] then return nil, "missing" end
    return rows[1], nil
end

local function next_order_id()
    local _nr = H.query_sync([[
        SELECT COALESCE(MAX(order_id), 0) + 1 AS next_id
        FROM ${SCHEMA}orders
    ]], {})
    local rows = qrows(_nr)
    return tonumber(pick(rows and rows[1], "next_id", "NEXT_ID")) or 1
end

local function find_order(idem)
    local _or, err = H.query_sync([[
        SELECT order_id, stripe_intent_id, status, total_cents, currency
        FROM ${SCHEMA}orders
        WHERE idempotency_key = :IDEM
    ]], { IDEM = idem })
    if err then return nil, err end
    local rows = qrows(_or)
    if not rows or not rows[1] then return nil, nil end
    return rows[1], nil
end

local function insert_order(row)
    -- Hydrogen drops nil named binds; never pass a missing :INTENT.
    local _, err = H.query_sync([[
        INSERT INTO ${SCHEMA}orders (
            order_id, order_number, account_id, stripe_intent_id,
            status, currency, total_cents, items_json, idempotency_key,
            valid_after, valid_until, created_id, created_at, updated_id, updated_at
        ) VALUES (
            :ORDERID, :ORDERNUMBER, :ACCOUNTID, :INTENT,
            'pending', :CURRENCY, :TOTAL, :ITEMS, :IDEM,
            NULL, NULL, 0, NOW(), 0, NOW()
        )
    ]], {
        ORDERID = row.order_id,
        ORDERNUMBER = row.order_number,
        ACCOUNTID = row.account_id,
        INTENT = row.stripe_intent_id,
        CURRENCY = row.currency,
        TOTAL = row.total_cents,
        ITEMS = row.items_json,
        IDEM = row.idempotency_key,
    })
    return err
end

local function set_intent(order_id, pi)
    local _, err = H.query_sync([[
        UPDATE ${SCHEMA}orders
           SET stripe_intent_id = :PI,
               updated_at = NOW(),
               updated_id = 0
         WHERE order_id = :ORDERID
    ]], { ORDERID = order_id, PI = pi })
    return err
end

local function create_intent(cus, currency, amount, keycloak_sub, account_id, order_id, idem)
    local fields = {
        { "amount", tostring(amount) },
        { "currency", currency },
        { "customer", cus },
        { "payment_method_types[0]", "card" },
        { "metadata[keycloak_sub]", keycloak_sub },
        { "metadata[internal_user_id]", tostring(account_id) },
        { "metadata[order_id]", tostring(order_id) },
    }
    local res, err = stripe_call("POST", "/payment_intents", form_encode(fields), idem)
    if err then return nil, nil, err end
    if res.status >= 200 and res.status < 300 then
        local pi = json_string(res.body or "", "id")
        local secret = json_string(res.body or "", "client_secret")
        if not pi or pi:sub(1, 3) ~= "pi_" then return nil, nil, "stripe_pi_no_id" end
        if not secret then return nil, nil, "stripe_pi_no_secret" end
        return pi, secret, nil
    end
    local err_type = json_string(res.body or "", "type") or ("http_" .. tostring(res.status))
    H.log.error("Stripe.Checkout HTTP POST failed: endpoint=/payment_intents idempotency=%s type=%s",
        idem, err_type)
    return nil, nil, err_type
end

local function retrieve_intent_secret(pi)
    local res, err = stripe_call("GET", "/payment_intents/" .. url_encode(pi), nil, nil)
    if err or not res or res.status < 200 or res.status >= 300 then
        return nil, "stripe_pi_retrieve"
    end
    return json_string(res.body or "", "client_secret"), nil
end

H.set_current_state("start")

if type(params) ~= "table" then
    return fail("validation", "Missing params")
end

local h = params._hydrogen
if type(h) ~= "table" then
    return fail("missing_identity", "Sign in required")
end

local account_id = tonumber(h.user_id) or tonumber(h.sub)
if not account_id then
    return fail("missing_identity", "Sign in required")
end

if not SECRET or SECRET == "" then
    H.log.error("Stripe.Checkout: STRIPE_SECRET_KEY not set in environment")
    return fail("stripe_unconfigured", "Payments are temporarily unavailable")
end

local currency = tostring(params.currency or ""):lower()
if not CURRENCIES[currency] then
    return fail("validation", "Invalid currency")
end

local raw_items = params.items
if type(raw_items) ~= "table" or #raw_items == 0 then
    return fail("validation", "Cart is empty")
end

local paid = {}
for i = 1, #raw_items do
    local it = raw_items[i]
    if type(it) ~= "table" then
        return fail("validation", "Invalid cart line")
    end
    local course_id = tonumber(it.courseId or it.course_id)
    if not course_id then
        return fail("validation", "Missing courseId")
    end
    local line_cur = tostring(it.currency or currency):lower()
    if line_cur ~= currency then
        return fail("mixed_currency", "Cart has mixed currencies")
    end
    local course, cerr = load_course(course_id)
    if not course then
        return fail("course_not_found", "Course not found")
    end
    local pricing = tostring(pick(course, "pricing_type", "PRICING_TYPE") or ""):lower()
    local line_type = tostring(it.lineType or it.line_type or "purchase")
    if line_type ~= "purchase" and line_type ~= "renew" then
        return fail("validation", "Invalid lineType")
    end
    if pricing == "free" then
        -- Reception enrolls free via Enroll.FreeCourse; omit from PI.
    elseif pricing == "paid" then
        local price, perr = load_price(course_id, currency)
        if not price then
            H.log.warn("Stripe.Checkout: currency_unavailable course_id=%s currency=%s err=%s",
                tostring(course_id), currency, tostring(perr))
            return fail("currency_unavailable", "This course is not priced in that currency")
        end
        local cents = tonumber(pick(price, "unit_amount_cents", "UNIT_AMOUNT_CENTS"))
        if not cents or cents < 1 then
            return fail("currency_unavailable", "This course is not priced in that currency")
        end
        paid[#paid + 1] = {
            courseId = course_id,
            currency = currency,
            unitAmountCents = cents,
            stripePriceId = pick(price, "stripe_price_id", "STRIPE_PRICE_ID"),
            canvasCourseId = tonumber(pick(course, "canvas_course_id", "CANVAS_COURSE_ID")),
            pricingType = "paid",
            lineType = line_type,
        }
    else
        return fail("validation", "Unknown pricing_type")
    end
end

if #paid == 0 then
    H.set_result_json({ ok = true, skip_stripe = true })
    return 0
end

local cus, cuerr = ensure_customer(account_id, h)
if not cus then
    return fail(cuerr or "stripe_customer_failed", "Could not prepare Stripe customer")
end

table.sort(paid, function(a, b)
    if a.courseId == b.courseId then return a.lineType < b.lineType end
    return a.courseId < b.courseId
end)

local total = 0
local idem_parts = {}
for i = 1, #paid do
    total = total + paid[i].unitAmountCents
    idem_parts[i] = tostring(paid[i].courseId) .. ":" .. paid[i].lineType
end
local idem = "checkout:" .. tostring(account_id) .. ":" .. currency .. ":" .. table.concat(idem_parts, ",")
if #idem > 128 then
    idem = string.sub(idem, 1, 128)
end

local existing, ferr = find_order(idem)
if ferr then
    H.log.warn("Stripe.Checkout: order lookup err: %s", tostring(ferr))
    return fail("order_error", "Could not load order")
end

if existing then
    local status = tostring(pick(existing, "status", "STATUS") or "")
    if status == "completed" then
        return fail("already_paid", "This cart was already paid")
    end
    local pi = pick(existing, "stripe_intent_id", "STRIPE_INTENT_ID")
    local oid = tonumber(pick(existing, "order_id", "ORDER_ID"))
    if pi and pi ~= "" then
        local secret, rerr = retrieve_intent_secret(pi)
        if secret then
            H.set_current_state("done")
            H.set_result_json({
                ok = true,
                client_secret = secret,
                order_id = oid,
                stripe_intent_id = pi,
            })
            return 0
        end
        H.log.warn("Stripe.Checkout: reuse PI failed type=%s", tostring(rerr))
    end
    local keycloak_sub = oidc_sub(account_id)
    local new_pi, secret, ierr = create_intent(cus, currency, total, keycloak_sub, account_id, oid, idem)
    if not new_pi then
        return fail(ierr or "stripe_pi_failed", "Could not start checkout")
    end
    set_intent(oid, new_pi)
    H.set_current_state("done")
    H.set_result_json({
        ok = true,
        client_secret = secret,
        order_id = oid,
        stripe_intent_id = new_pi,
    })
    return 0
end

local order_id = next_order_id()
local order_number = "500c-" .. tostring(order_id)
local keycloak_sub = oidc_sub(account_id)
local pi, secret, perr = create_intent(cus, currency, total, keycloak_sub, account_id, order_id, idem)
if not pi then
    return fail(perr or "stripe_pi_failed", "Could not start checkout")
end
local ierr = insert_order({
    order_id = order_id,
    order_number = order_number,
    account_id = account_id,
    stripe_intent_id = pi,
    currency = currency,
    total_cents = total,
    items_json = items_json(paid),
    idempotency_key = idem,
})
if ierr then
    H.log.warn("Stripe.Checkout: insert order err: %s", tostring(ierr))
    local again = find_order(idem)
    if again then
        local existing_pi = pick(again, "stripe_intent_id", "STRIPE_INTENT_ID")
        local oid = tonumber(pick(again, "order_id", "ORDER_ID"))
        if existing_pi and existing_pi ~= "" then
            local reuse = retrieve_intent_secret(existing_pi)
            if reuse then
                H.set_result_json({
                    ok = true,
                    client_secret = reuse,
                    order_id = oid,
                    stripe_intent_id = existing_pi,
                })
                return 0
            end
        end
    end
    return fail("order_error", "Could not create order")
end

H.set_current_state("done")
H.set_result_json({
    ok = true,
    client_secret = secret,
    order_id = order_id,
    stripe_intent_id = pi,
})
return 0
                ]==],
                'Phase 5: invokable Stripe PaymentIntent checkout (H.http)',
                1,
                ${COMMON_VALUES}
            );

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Seed Stripe.Checkout invokable script'                             AS name,
        [=[
            # Forward Migration ${MIGRATION}: Seed Stripe.Checkout

            Inserts `Stripe.Checkout` with `invokable = 1` for
            Reception `POST /api/conduit/script` (STRIPE_PLAN Phase 5).

            The worker:

            1. Reads `params._hydrogen` and `{ currency, items[] }`
               (`cart.checkoutPayload().params`).
            2. Re-reads `courses` + `course_prices` for the requested
               currency. Client cents are ignored. Missing price →
               `currency_unavailable` (no FX).
            3. Free lines are omitted (Reception uses
               `Enroll.FreeCourse`). All-free → `{ ok, skip_stripe }`.
            4. Ensures a Stripe Customer (same rules as
               `Stripe.EnsureCustomer`).
            5. INSERT `orders` pending. Idempotency
               `checkout:{account_id}:{currency}:{course:lineType,…}`.
            6. POST `/v1/payment_intents` (`card`), pin
               `Stripe-Version: 2026-07-29.dahlia`.
            7. Returns `{ ok, client_secret, order_id, stripe_intent_id }`.

            No diagram (data seed).
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
              AND script_name = 'Checkout';

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Remove Stripe.Checkout script'                                     AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Remove Stripe.Checkout
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]====]})

return queries end
