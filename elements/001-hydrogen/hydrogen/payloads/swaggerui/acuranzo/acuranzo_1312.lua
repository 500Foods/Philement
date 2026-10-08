-- Migration: acuranzo_1312.lua
-- STRIPE_PLAN Phase 2: seed Stripe.EnsureCustomer (invokable)
--
-- JWT POST /api/conduit/script. One Stripe Customer per Hydrogen account.
-- H.http + os.getenv("STRIPE_SECRET_KEY"). No Hydrogen C. No keys in git.

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.0.0 - 2026-08-13 - Seed Stripe.EnsureCustomer invokable=1 (STRIPE_PLAN Phase 2)

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "scripts"
cfg.MIGRATION = "1312"
cfg.GROUP_NAME = "Stripe"
cfg.SCRIPT_NAME = "EnsureCustomer"
-- ----------------------------------------------------------------------------
-- Forward: seed Stripe.EnsureCustomer (invokable)
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
-- Stripe.EnsureCustomer (STRIPE_PLAN Phase 2)
-- JWT POST /api/conduit/script. Identity from params._hydrogen.
-- Creates or returns the Stripe Customer for this account.
-- Secrets: os.getenv("STRIPE_SECRET_KEY"). Never log sk_ / Authorization.
-- Idempotency: user:{keycloak_sub} from account_oidc_identities.subject
-- (_hydrogen.sub is the Hydrogen account id, not Keycloak).

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

local function succeed(cus, created)
    H.set_result_json({
        ok = true,
        stripe_customer_id = cus,
        created = created and true or false,
    })
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

local function json_cus_id(body)
    local id = json_string(body, "id")
    if id and id:sub(1, 4) == "cus_" then return id end
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
            H.log.error("Stripe.EnsureCustomer HTTP %s failed: endpoint=%s idempotency=%s type=network attempt=%s",
                method, path, tostring(idem or ""), tostring(attempt))
        elseif res then
            local status = tonumber(res.status) or 0
            if status == 429 or status >= 500 then
                last_err = "http_" .. tostring(status)
                H.log.error("Stripe.EnsureCustomer HTTP %s failed: endpoint=%s idempotency=%s type=%s attempt=%s",
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
        return json_cus_id(res.body or ""), nil
    end
    local err_type = json_string(res.body or "", "type") or ("http_" .. tostring(res.status))
    if res.status == 404 or err_type == "invalid_request_error" then
        local code = json_string(res.body or "", "code")
        if code == "resource_missing" or res.status == 404 then
            return nil, "resource_missing"
        end
    end
    H.log.error("Stripe.EnsureCustomer HTTP GET failed: endpoint=/customers/:id idempotency= type=%s",
        err_type)
    return nil, err_type
end

local function create_customer(email, name, keycloak_sub, account_id)
    local idem = "user:" .. keycloak_sub
    local fields = {
        { "metadata[keycloak_sub]", keycloak_sub },
        { "metadata[internal_user_id]", tostring(account_id) },
    }
    if email and email ~= "" then
        fields[#fields + 1] = { "email", email }
    end
    if name and name ~= "" then
        fields[#fields + 1] = { "name", name }
    end
    local res, err = stripe_call("POST", "/customers", form_encode(fields), idem)
    if err then return nil, err end
    if res.status >= 200 and res.status < 300 then
        local id = json_cus_id(res.body or "")
        if not id then return nil, "stripe_create_no_id" end
        return id, nil
    end
    local err_type = json_string(res.body or "", "type") or ("http_" .. tostring(res.status))
    H.log.error("Stripe.EnsureCustomer HTTP POST failed: endpoint=/customers idempotency=%s type=%s",
        idem, err_type)
    return nil, err_type
end

local function find_by_email(email)
    if not email or email == "" then return nil, "not_found" end
    local path = "/customers?email=" .. url_encode(email) .. "&limit=2"
    local res, err = stripe_call("GET", path, nil, nil)
    if err or not res or res.status < 200 or res.status >= 300 then
        return nil, "lookup_failed"
    end
    local ids = {}
    for id in (res.body or ""):gmatch('"id"%s*:%s*"(cus_[^"]+)"') do
        ids[#ids + 1] = id
    end
    if #ids == 0 then return nil, "not_found" end
    if #ids > 1 then
        H.log.warn("Stripe.EnsureCustomer: duplicate email, using first cus")
    end
    return ids[1], nil
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
    H.log.error("Stripe.EnsureCustomer: STRIPE_SECRET_KEY not set in environment")
    return fail("stripe_unconfigured", "Payments are temporarily unavailable")
end

local _ar, aerr = H.query_sync([[
    SELECT account_id, name, first_name, last_name, stripe_customer_id
    FROM ${SCHEMA}accounts
    WHERE account_id = :ACCOUNTID
]], { ACCOUNTID = account_id })
if aerr then
    H.log.warn("Stripe.EnsureCustomer: account lookup err: %s", tostring(aerr))
    return fail("account_error", "Could not load account")
end
local arows = qrows(_ar)
if not arows or not arows[1] then
    return fail("account_not_found", "Account not found")
end
local acct = arows[1]
local existing = pick(acct, "stripe_customer_id", "STRIPE_CUSTOMER_ID")
if existing and existing ~= "" then
    local got, rerr = retrieve_customer(existing)
    if got then
        H.set_current_state("done")
        return succeed(got, false)
    end
    if rerr ~= "resource_missing" then
        return fail("stripe_retrieve_failed", "Could not reach Stripe")
    end
    H.log.warn("Stripe.EnsureCustomer: resource_missing cus for account_id=%s",
        tostring(account_id))
end

local _or, oerr = H.query_sync([[
    SELECT subject, email
    FROM ${SCHEMA}account_oidc_identities
    WHERE account_id = :ACCOUNTID
    ORDER BY last_seen_at DESC
]], { ACCOUNTID = account_id })
if oerr then
    H.log.warn("Stripe.EnsureCustomer: oidc lookup err: %s", tostring(oerr))
end
local orows = qrows(_or)
local oidc = orows and orows[1] or nil
local keycloak_sub = pick(oidc, "subject", "SUBJECT")
if not keycloak_sub or keycloak_sub == "" then
    keycloak_sub = "account:" .. tostring(account_id)
    H.log.warn("Stripe.EnsureCustomer: no OIDC subject, idempotency=%s",
        "user:" .. keycloak_sub)
end

local email = h.email or pick(oidc, "email", "EMAIL")
local name = pick(acct, "name", "NAME") or h.username or email

local cus, cerr = create_customer(email, name, keycloak_sub, account_id)
if not cus then
    local fallback, ferr = find_by_email(email)
    if fallback then
        cus = fallback
        H.log.warn("Stripe.EnsureCustomer: create failed (%s); email fallback %s",
            tostring(cerr), cus)
    else
        return fail(cerr or ferr or "stripe_create_failed", "Could not create Stripe customer")
    end
end

local serr = store_customer(account_id, cus)
if serr then
    H.log.warn("Stripe.EnsureCustomer: store err: %s", tostring(serr))
    return fail("store_failed", "Could not save Stripe customer")
end

H.set_current_state("done")
return succeed(cus, true)
                ]==],
                'Phase 2: invokable Stripe customer ensure (H.http)',
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
        'Seed Stripe.EnsureCustomer invokable script'                       AS name,
        [=[
            # Forward Migration ${MIGRATION}: Seed Stripe.EnsureCustomer

            Inserts `Stripe.EnsureCustomer` with `invokable = 1` for
            Reception `POST /api/conduit/script` (STRIPE_PLAN Phase 2).

            The worker:

            1. Reads `params._hydrogen` (`user_id` / Hydrogen `sub`).
            2. Loads `${SCHEMA}accounts.stripe_customer_id`.
            3. If set, GET `/v1/customers/:id`. `resource_missing` → recreate.
            4. Resolves Keycloak subject from
               `${SCHEMA}account_oidc_identities` (not `_hydrogen.sub`).
               Fallback idempotency `user:account:{id}`.
            5. POST `/v1/customers` form-urlencoded, `Stripe-Version:
               2026-07-29.dahlia`, Idempotency-Key `user:{subject}`,
               metadata `keycloak_sub` + `internal_user_id`.
            6. Duplicate-email fallback: GET `/v1/customers?email=`.
            7. UPDATE `accounts.stripe_customer_id`.

            Retry/backoff on network / 429 / 5xx. Logs endpoint +
            idempotency + error type only. `STRIPE_SECRET_KEY` from env.
            No diagram (data seed, same as 1300).
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
              AND script_name = 'EnsureCustomer';

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Remove Stripe.EnsureCustomer script'                               AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Remove Stripe.EnsureCustomer
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]====]})

return queries end
