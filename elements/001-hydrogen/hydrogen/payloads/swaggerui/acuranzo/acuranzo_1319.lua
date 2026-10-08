-- Migration: acuranzo_1319.lua
-- STRIPE_PLAN Phase 7: seed Stripe.Webhook (invokable)
--
-- C HMAC POST /api/conduit/webhook/stripe submits this script with
-- { hook, body, headers, content_type }. No params._hydrogen.
-- JWT invoke injects _hydrogen — this script rejects that so a
-- signed-in client cannot forge a fulfill.

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.0.0 - 2026-08-13 - Seed Stripe.Webhook invokable=1 (STRIPE_PLAN Phase 7)

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "scripts"
cfg.MIGRATION = "1319"
cfg.GROUP_NAME = "Stripe"
cfg.SCRIPT_NAME = "Webhook"
-- ----------------------------------------------------------------------------
-- Forward: seed Stripe.Webhook (invokable)
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
-- Stripe.Webhook (STRIPE_PLAN Phase 7)
-- C HMAC ingress only. params = { hook, body, headers, content_type }.
-- JWT POST /api/conduit/script injects _hydrogen — reject that.
-- require("Enroll.PaidCourse") on payment_intent.succeeded.

local function fail(code, message)
    H.set_result_json({ ok = false, code = code, message = message or code })
    return 0
end

local function json_string(body, key)
    if type(body) ~= "string" then return nil end
    return body:match('"' .. key .. '"%s*:%s*"([^"]*)"')
end

local function json_number(body, key)
    if type(body) ~= "string" then return nil end
    return tonumber(body:match('"' .. key .. '"%s*:%s*(%-?%d+)'))
end

local function mark_failed(pi)
    if not pi or pi == "" then return nil end
    local _, uerr = H.query_sync([[
        UPDATE ${SCHEMA}orders
           SET status = 'failed',
               updated_at = NOW(),
               updated_id = 0
         WHERE stripe_intent_id = :INTENT
           AND status = 'pending'
    ]], { INTENT = pi })
    return uerr
end

H.set_current_state("start")

if type(params) ~= "table" then
    return fail("validation", "Missing params")
end

if params._hydrogen ~= nil then
    H.log.warn("Stripe.Webhook: rejected JWT invoke")
    return fail("webhook_only", "This script is not callable from the SPA")
end

local hook = tostring(params.hook or "")
if hook ~= "" and string.lower(hook) ~= "stripe" then
    return fail("wrong_hook", "Unexpected hook")
end

local body = params.body
if type(body) ~= "string" or body == "" then
    return fail("validation", "Missing event body")
end

local ev_type = json_string(body, "type")
if not ev_type or ev_type == "" then
    return fail("validation", "Missing event type")
end

local ev_id = json_string(body, "id")
H.log.info("Stripe.Webhook: type=%s event=%s", ev_type, tostring(ev_id or ""))

if ev_type == "payment_intent.succeeded" then
    -- Event id is evt_; PaymentIntent is data.object.id (pi_…).
    local pi = body:match('"id"%s*:%s*"(pi_[^"]+)"')
    if not pi then
        return fail("validation", "Missing payment intent")
    end
    local amount = json_number(body, "amount")
    local currency = json_string(body, "currency")
    local order_meta = body:match('"order_id"%s*:%s*"([^"]*)"')
        or body:match('"order_id"%s*:%s*(%d+)')

    local ok, paid = pcall(require, "Enroll.PaidCourse")
    if not ok or type(paid) ~= "table" or type(paid.fulfill) ~= "function" then
        H.log.error("Stripe.Webhook: require Enroll.PaidCourse failed")
        return fail("module_missing", "Fulfill module unavailable")
    end

    local result = paid.fulfill({
        stripe_intent_id = pi,
        order_id = tonumber(order_meta),
        amount_cents = amount,
        currency = currency,
    })
    if type(result) ~= "table" then
        return fail("fulfill_error", "Fulfill returned nothing")
    end
    H.set_current_state("done")
    H.set_result_json(result)
    return 0
end

if ev_type == "payment_intent.payment_failed" then
    local pi = body:match('"id"%s*:%s*"(pi_[^"]+)"')
    if pi then
        local uerr = mark_failed(pi)
        if uerr then
            H.log.warn("Stripe.Webhook: mark failed err type=query")
            return fail("order_error", "Could not mark order failed")
        end
    end
    H.set_current_state("done")
    H.set_result_json({
        ok = true,
        ignored = false,
        type = ev_type,
        failed = true,
        stripe_intent_id = pi,
    })
    return 0
end

H.set_current_state("done")
H.set_result_json({
    ok = true,
    ignored = true,
    type = ev_type,
})
return 0
                ]==],
                'Phase 7: invokable Stripe webhook router (HMAC C ingress)',
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
        'Seed Stripe.Webhook invokable script'                              AS name,
        [=[
            # Forward Migration ${MIGRATION}: Seed Stripe.Webhook

            Inserts `Stripe.Webhook` with `invokable = 1` for Hydrogen
            `POST /api/conduit/webhook/stripe` (STRIPE_PLAN Phase 7).
            C HMAC already verified. Lua is the router.

            The worker:

            1. Rejects `params._hydrogen` (JWT SPA path). C webhook
               never injects that key.
            2. Parses `params.body` for Stripe `type`.
            3. `payment_intent.succeeded` →
               `require("Enroll.PaidCourse").fulfill`. Duplicate
               `stripe_intent_id` already completed → `{ ok, already }`.
            4. `payment_intent.payment_failed` → `orders.status=failed`
               when still `pending`.
            5. Other types → `{ ok, ignored=true }` (Stripe retries
               only care about 2xx).
            6. Logs event type + id only. No payment_method.

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
              AND script_name = 'Webhook';

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Remove Stripe.Webhook script'                                      AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Remove Stripe.Webhook
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]====]})

return queries end
