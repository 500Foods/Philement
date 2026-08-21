-- Migration: acuranzo_1360.lua
-- PRIORITIZE 2.18: seed Account.Orders (invokable)
--
-- JWT POST /api/conduit/script. Own orders only. Learner receipts.
-- Refund/deactivate stay admin scripts (1361/1362).
--
-- luacheck: no max line length
-- luacheck: no unused args
--
-- CHANGELOG
-- 1.0.0 - 2026-08-21 - Seed Account.Orders invokable=1 (PRIORITIZE 2.18)

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "scripts"
cfg.MIGRATION = "1360"
cfg.GROUP_NAME = "Account"
cfg.SCRIPT_NAME = "Orders"
-- ----------------------------------------------------------------------------
-- Forward: seed Account.Orders (invokable)
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
-- Account.Orders (PRIORITIZE 2.18)
-- JWT POST /api/conduit/script. Identity from params._hydrogen.
-- Own orders only. Never log sk_ / Authorization.

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
    if v == nil then v = row[b] end
    return v
end

local function str_or_empty(v)
    if v == nil then return "" end
    local s = tostring(v)
    if s == "null" or s == "NULL" then return "" end
    return s
end

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

local _or, qerr = H.query_sync([[
    SELECT order_id, order_number, status, currency, total_cents,
           items_json, created_at, refunded_at, refund_reason
    FROM ${SCHEMA}orders
    WHERE account_id = :ACCOUNTID
    ORDER BY created_at DESC, order_id DESC
]], { ACCOUNTID = account_id })
if qerr then
    H.log.warn("Account.Orders: lookup err: %s", tostring(qerr))
    return fail("lookup_error", "Could not load orders")
end

local rows = qrows(_or) or {}
local orders = {}
for i = 1, #rows do
    local r = rows[i]
    orders[#orders + 1] = {
        order_id = tonumber(pick(r, "order_id", "ORDER_ID")) or 0,
        order_number = str_or_empty(pick(r, "order_number", "ORDER_NUMBER")),
        status = str_or_empty(pick(r, "status", "STATUS")),
        currency = str_or_empty(pick(r, "currency", "CURRENCY")),
        total_cents = tonumber(pick(r, "total_cents", "TOTAL_CENTS")) or 0,
        items_json = str_or_empty(pick(r, "items_json", "ITEMS_JSON")),
        created_at = str_or_empty(pick(r, "created_at", "CREATED_AT")),
        refunded_at = str_or_empty(pick(r, "refunded_at", "REFUNDED_AT")),
        refund_reason = str_or_empty(pick(r, "refund_reason", "REFUND_REASON")),
    }
end

H.set_result_json({
    ok = true,
    orders = orders,
})
return 0
                ]==],
                'PRIORITIZE 2.18: invokable learner order history',
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
        'Seed Account.Orders invokable script'                              AS name,
        [=[
            # Forward Migration ${MIGRATION}: Seed Account.Orders

            Inserts `Account.Orders` with `invokable = 1` for Reception
            Settings receipts (PRIORITIZE 2.18). Own `orders` rows only.
            Does not return `stripe_intent_id`. No diagram.
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
            WHERE group_name = 'Account'
              AND script_name = 'Orders';

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Remove Account.Orders script'                                      AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Remove Account.Orders
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]====]})

return queries end
