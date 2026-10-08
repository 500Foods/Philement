-- Migration: acuranzo_1314.lua
-- STRIPE_PLAN Phase 2 follow-up: seed Stripe.SyncCustomer (invokable)
--
-- JWT POST /api/conduit/script. Push email/name to Stripe on explicit
-- profile update (not login). H.http POST /v1/customers/:id.
-- Account.UpdatePrefs does not write name/email (Keycloak).

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.0.0 - 2026-08-13 - Seed Stripe.SyncCustomer invokable=1 (STRIPE_PLAN Phase 2)

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "scripts"
cfg.MIGRATION = "1314"
cfg.GROUP_NAME = "Stripe"
cfg.SCRIPT_NAME = "SyncCustomer"
-- ----------------------------------------------------------------------------
-- Forward: seed Stripe.SyncCustomer (invokable)
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
-- Stripe.SyncCustomer (STRIPE_PLAN Phase 2)
-- JWT POST /api/conduit/script. Identity from params._hydrogen.
-- Updates Stripe Customer email/name when the profile changes.
-- Does not create a customer (use Stripe.EnsureCustomer).
-- Secrets: os.getenv("STRIPE_SECRET_KEY"). Never log sk_ / Authorization.

local STRIPE_VERSION = "2026-07-29.dahlia"
local STRIPE_BASE = "https://api.stripe.com/v1"

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

local SECRET = getenv("STRIPE_SECRET_KEY", nil)

local function stripe_headers()
    return {
        Authorization = "Bearer " .. SECRET,
        Accept = "application/json",
        ["Stripe-Version"] = STRIPE_VERSION,
    }
end

local function stripe_post(path, body)
    local url = STRIPE_BASE .. path
    local delays = { 200, 800, 2000 }
    local last_err = "stripe_http"
    for attempt = 1, 3 do
        local res, err = H.http.post_sync(url, body or "", stripe_headers(), {
            timeout = 15,
            content_type = "application/x-www-form-urlencoded",
        })
        if err then
            last_err = tostring(err)
            H.log.error("Stripe.SyncCustomer HTTP POST failed: endpoint=%s idempotency= type=network attempt=%s",
                path, tostring(attempt))
        elseif res then
            local status = tonumber(res.status) or 0
            if status == 429 or status >= 500 then
                last_err = "http_" .. tostring(status)
                H.log.error("Stripe.SyncCustomer HTTP POST failed: endpoint=%s idempotency= type=%s attempt=%s",
                    path, last_err, tostring(attempt))
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
    H.log.error("Stripe.SyncCustomer: STRIPE_SECRET_KEY not set in environment")
    return fail("stripe_unconfigured", "Payments are temporarily unavailable")
end

local _ar, aerr = H.query_sync([[
    SELECT account_id, name, first_name, last_name, stripe_customer_id
    FROM ${SCHEMA}accounts
    WHERE account_id = :ACCOUNTID
]], { ACCOUNTID = account_id })
if aerr then
    H.log.warn("Stripe.SyncCustomer: account lookup err: %s", tostring(aerr))
    return fail("account_error", "Could not load account")
end
local arows = qrows(_ar)
if not arows or not arows[1] then
    return fail("account_not_found", "Account not found")
end
local acct = arows[1]
local cus = pick(acct, "stripe_customer_id", "STRIPE_CUSTOMER_ID")
if not cus or cus == "" then
    H.set_result_json({
        ok = true,
        skipped = true,
        code = "no_customer",
        message = "No Stripe customer to update",
    })
    return 0
end

local _or, oerr = H.query_sync([[
    SELECT email
    FROM ${SCHEMA}account_oidc_identities
    WHERE account_id = :ACCOUNTID
    ORDER BY last_seen_at DESC
]], { ACCOUNTID = account_id })
if oerr then
    H.log.warn("Stripe.SyncCustomer: oidc lookup err: %s", tostring(oerr))
end
local orows = qrows(_or)
local oidc = orows and orows[1] or nil
local email = h.email or pick(oidc, "email", "EMAIL")
local name = pick(acct, "name", "NAME") or h.username or email

local fields = {}
if email and email ~= "" then
    fields[#fields + 1] = { "email", email }
end
if name and name ~= "" then
    fields[#fields + 1] = { "name", name }
end
if #fields == 0 then
    H.set_result_json({
        ok = true,
        skipped = true,
        stripe_customer_id = cus,
        code = "nothing_to_sync",
    })
    return 0
end

local path = "/customers/" .. url_encode(cus)
local res, err = stripe_post(path, form_encode(fields))
if err then
    return fail("stripe_update_failed", "Could not update Stripe customer")
end
if res.status == 404 then
    H.log.warn("Stripe.SyncCustomer: resource_missing cus for account_id=%s",
        tostring(account_id))
    return fail("resource_missing", "Stripe customer no longer exists")
end
if res.status < 200 or res.status >= 300 then
    local err_type = json_string(res.body or "", "type") or ("http_" .. tostring(res.status))
    H.log.error("Stripe.SyncCustomer HTTP POST failed: endpoint=%s idempotency= type=%s",
        path, err_type)
    return fail("stripe_update_failed", "Could not update Stripe customer")
end

H.set_current_state("done")
H.set_result_json({
    ok = true,
    stripe_customer_id = cus,
    synced = true,
})
return 0
                ]==],
                'Phase 2: invokable Stripe customer email/name sync (H.http)',
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
        'Seed Stripe.SyncCustomer invokable script'                         AS name,
        [=[
            # Forward Migration ${MIGRATION}: Seed Stripe.SyncCustomer

            Inserts `Stripe.SyncCustomer` with `invokable = 1` for
            `POST /api/conduit/script` (STRIPE_PLAN Phase 2 action 4).

            `Account.UpdatePrefs` does not write name/email (Keycloak
            account console). Reception or a later profile writer calls
            this sibling when those fields change.

            The worker:

            1. Reads `params._hydrogen` (`user_id` / Hydrogen `sub`).
            2. If `accounts.stripe_customer_id` is empty, returns
               `{ ok, skipped, code=no_customer }` (does not create).
            3. POST `/v1/customers/:id` form-urlencoded with email/name
               from OIDC + `accounts.name`. Pin `Stripe-Version:
               2026-07-29.dahlia`.
            4. `resource_missing` (404) → fail so caller can
               `Stripe.EnsureCustomer`.
            5. Never syncs avatar, password, or prefs.

            Retry/backoff on network / 429 / 5xx. Logs endpoint + type
            only. `STRIPE_SECRET_KEY` from env. No diagram (data seed).
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
              AND script_name = 'SyncCustomer';

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Remove Stripe.SyncCustomer script'                                 AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Remove Stripe.SyncCustomer
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]====]})

return queries end
