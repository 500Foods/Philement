-- Migration: argent_2031.lua
-- Argent ledger reads
--
-- luacheck: no max line length
-- luacheck: no unused args
--
-- CHANGELOG
-- 1.0.0 - 2026-10-07 - MCP ledger list and get

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "scripts"
cfg.MIGRATION = "2031"
cfg.GROUP_NAME = "Argent"
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
                'ListLedgers',
                1,
                NULL, NULL, NULL, NULL,
                1,
                [==[
-- Argent.ListLedgers
-- organization_id, type, and tag are optional. include_non_posting defaults off.
-- type is lookup 2001. tag matches tags.name on entity type ledger (2).

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

local function fail(code, message)
    H.set_result_json({ ok = false, code = code, message = message or code })
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

if type(params) ~= "table" then params = {} end
local org = num(params.organization_id)
local ledger_type = num(params.type)
local tag = params.tag
if type(tag) ~= "string" or tag == "" then tag = "" end
local include_non = 0
if params.include_non_posting == true or params.include_non_posting == 1 then
    include_non = 1
end

local res, err = H.query_sync([[
    SELECT l.ledger_id, l.organization_id, l.parent_id, l.status_a2002,
           l.ledger_type_a2001, l.is_posting, l.name, l.currency,
           l.opening_balance_cents
    FROM ${SCHEMA}ledgers l
    WHERE (:USE_ORG = 0 OR l.organization_id = :ORG_ID)
      AND (:USE_TYPE = 0 OR l.ledger_type_a2001 = :LEDGER_TYPE)
      AND (:INCLUDE_NON_POSTING = 1 OR l.is_posting = 1)
      AND (
            :USE_TAG = 0
            OR l.ledger_id IN (
                SELECT tl.entity_id
                FROM ${SCHEMA}tag_links tl
                INNER JOIN ${SCHEMA}tags tg ON tg.tag_id = tl.tag_id
                WHERE tl.entity_type_a2009 = 2
                  AND tg.name = :TAG_NAME
            )
          )
    ORDER BY l.ledger_id
]], {
    USE_ORG = org and 1 or 0,
    ORG_ID = org or 0,
    USE_TYPE = ledger_type and 1 or 0,
    LEDGER_TYPE = ledger_type or 0,
    INCLUDE_NON_POSTING = include_non,
    USE_TAG = tag ~= "" and 1 or 0,
    TAG_NAME = tag,
})
if err then return fail("query_failed", tostring(err)) end
local rows = qrows(res)
local out = {}
for i = 1, #rows do
    local row = rows[i]
    out[i] = {
        ledger_id = pick(row, "ledger_id"),
        organization_id = pick(row, "organization_id"),
        parent_id = pick(row, "parent_id"),
        status_a2002 = pick(row, "status_a2002"),
        ledger_type_a2001 = pick(row, "ledger_type_a2001"),
        is_posting = pick(row, "is_posting"),
        name = pick(row, "name"),
        currency = pick(row, "currency"),
        opening_balance_cents = pick(row, "opening_balance_cents"),
    }
end
return ok({ ledgers = out })
                ]==],
                'List Argent ledger summaries',
                0,
                1,
                '{"inputSchema":{"type":"object","properties":{"organization_id":{"type":"integer"},"type":{"type":"integer"},"tag":{"type":"string"},"include_non_posting":{"type":"boolean"}},"additionalProperties":true}}',
                '{"title":"List ledgers","readOnlyHint":true}',
                ${COMMON_VALUES}
            );

            ${SUBQUERY_DELIMITER}

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
                'GetLedger',
                1,
                NULL, NULL, NULL, NULL,
                1,
                [==[
-- Argent.GetLedger
-- Returns the ledger, the terms row in force on as_of, contacts, and the
-- QueryRef 2000 balance for a posting ledger. Omitted status uses keys 3 and 4.

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

local function fail(code, message)
    H.set_result_json({ ok = false, code = code, message = message or code })
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

local function today()
    if type(H.system) == "table" and type(H.system.now_iso) == "function" then
        local s = H.system.now_iso()
        if type(s) == "string" and #s >= 10 then return s:sub(1, 10) end
    end
    return nil
end

local function status_binds(list)
    local slots = { 0, 0, 0, 0, 0 }
    local use_default = 1
    if type(list) == "table" and #list > 0 then
        if #list > 5 then return nil, "status accepts at most 5 lookup 2003 keys" end
        use_default = 0
        for i = 1, #list do
            local n = num(list[i])
            if not n then return nil, "status keys must be numbers" end
            slots[i] = n
        end
    end
    return {
        USE_DEFAULT = use_default,
        STATUS_1 = slots[1], STATUS_2 = slots[2], STATUS_3 = slots[3],
        STATUS_4 = slots[4], STATUS_5 = slots[5],
    }, nil
end

local function run_ref(ref, binds)
    local res, err = H.query_sync([[
        SELECT code FROM ${SCHEMA}queries
        WHERE query_ref = :QREF AND query_type_a28 = 1
    ]], { QREF = ref })
    if err then return nil, err end
    local rows = qrows(res)
    local code = rows[1] and pick(rows[1], "code")
    if type(code) ~= "string" or code == "" then
        return nil, "query " .. tostring(ref) .. " is not installed"
    end
    return H.query_sync(code, binds)
end

if type(params) ~= "table" then
    return fail("params_required", "params must be an object")
end
local ledger_id = num(params.ledger_id)
if not ledger_id then return fail("ledger_id_required", "ledger_id is required") end
local as_of = params.as_of
if type(as_of) ~= "string" or as_of == "" then as_of = today() end
if not as_of then return fail("as_of_required", "as_of is required") end

local res, err = H.query_sync([[
    SELECT ledger_id, organization_id, parent_id, status_a2002,
           ledger_type_a2001, is_posting, name, currency,
           opening_on, opening_balance_cents, opening_txn_id,
           latest_reconciliation_id, latest_reconciled_on, summary
    FROM ${SCHEMA}ledgers
    WHERE ledger_id = :LEDGER_ID
]], { LEDGER_ID = ledger_id })
if err then return fail("query_failed", tostring(err)) end
local rows = qrows(res)
if not rows[1] then return fail("not_found", "ledger not found") end
local row = rows[1]
local ledger = {
    ledger_id = pick(row, "ledger_id"),
    organization_id = pick(row, "organization_id"),
    parent_id = pick(row, "parent_id"),
    status_a2002 = pick(row, "status_a2002"),
    ledger_type_a2001 = pick(row, "ledger_type_a2001"),
    is_posting = pick(row, "is_posting"),
    name = pick(row, "name"),
    currency = pick(row, "currency"),
    opening_on = pick(row, "opening_on"),
    opening_balance_cents = pick(row, "opening_balance_cents"),
    opening_txn_id = pick(row, "opening_txn_id"),
    latest_reconciliation_id = pick(row, "latest_reconciliation_id"),
    latest_reconciled_on = pick(row, "latest_reconciled_on"),
    summary = pick(row, "summary"),
}

local tres, terr = H.query_sync([[
    SELECT ledger_term_id, effective_on, credit_limit_cents, od_limit_cents,
           apr_purchase_bps, apr_cash_bps, annual_fee_cents,
           statement_close_day, payment_due_offset_days, summary
    FROM ${SCHEMA}ledger_terms
    WHERE ledger_id = :LEDGER_ID
      AND effective_on <= :AS_OF
    ORDER BY effective_on DESC, ledger_term_id DESC
]], { LEDGER_ID = ledger_id, AS_OF = as_of })
if terr then return fail("query_failed", tostring(terr)) end
local trows = qrows(tres)
local terms = nil
if trows[1] then
    local t = trows[1]
    terms = {
        ledger_term_id = pick(t, "ledger_term_id"),
        effective_on = pick(t, "effective_on"),
        credit_limit_cents = pick(t, "credit_limit_cents"),
        od_limit_cents = pick(t, "od_limit_cents"),
        apr_purchase_bps = pick(t, "apr_purchase_bps"),
        apr_cash_bps = pick(t, "apr_cash_bps"),
        annual_fee_cents = pick(t, "annual_fee_cents"),
        statement_close_day = pick(t, "statement_close_day"),
        payment_due_offset_days = pick(t, "payment_due_offset_days"),
        summary = pick(t, "summary"),
    }
end

local cres, cerr = H.query_sync([[
    SELECT contact_id, role_a2005, name, email, phone, summary
    FROM ${SCHEMA}contacts
    WHERE ledger_id = :LEDGER_ID
    ORDER BY contact_id
]], { LEDGER_ID = ledger_id })
if cerr then return fail("query_failed", tostring(cerr)) end
local crows = qrows(cres)
local contacts = {}
for i = 1, #crows do
    local c = crows[i]
    contacts[i] = {
        contact_id = pick(c, "contact_id"),
        role_a2005 = pick(c, "role_a2005"),
        name = pick(c, "name"),
        email = pick(c, "email"),
        phone = pick(c, "phone"),
        summary = pick(c, "summary"),
    }
end

local balance = nil
if tonumber(ledger.is_posting) == 1 then
    local binds, serr = status_binds(params.status)
    if not binds then return fail("status", serr) end
    binds.ORGANIZATION_ID = tonumber(ledger.organization_id)
    binds.AS_OF = as_of
    local bres, berr = run_ref(2000, binds)
    if berr then return fail("balance_failed", tostring(berr)) end
    local brows = qrows(bres)
    for i = 1, #brows do
        if tonumber(pick(brows[i], "ledger_id")) == ledger_id then
            balance = {
                ledger_id = pick(brows[i], "ledger_id"),
                name = pick(brows[i], "name"),
                currency = pick(brows[i], "currency"),
                balance_cents = pick(brows[i], "balance_cents"),
                as_of = as_of,
            }
            break
        end
    end
end

return ok({
    ledger = ledger,
    terms = terms,
    contacts = contacts,
    balance = balance,
    as_of = as_of,
})
                ]==],
                'Get one Argent ledger, terms, contacts, and balance',
                0,
                1,
                '{"inputSchema":{"type":"object","properties":{"ledger_id":{"type":"integer"},"as_of":{"type":"string"},"status":{"type":"array","items":{"type":"integer"}}},"required":["ledger_id"],"additionalProperties":true}}',
                '{"title":"Get ledger","readOnlyHint":true}',
                ${COMMON_VALUES}
            );
            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Seed Argent ledger read tools'                                                    AS name,
        [=[
            # Forward Migration ${MIGRATION}: Argent ledger reads

            Inserts the Argent script rows named in the reverse migration.
            Group `Argent`. `mcp_access=1`, `invokable=0`.
            Does not install a QueryRef. Does not create a table.
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
              AND script_name IN ('ListLedgers', 'GetLedger');

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Remove Argent ledger read tools'                                                    AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Remove Argent ledger reads

            Deletes those script rows. Does not drop Argent tables.
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
        'Diagram Argent ledger reads'                                                   AS name,
        [=[
            # Diagram Migration ${MIGRATION}

            Script rows. This does not define a table.
        ]=]
                                                                            AS summary,
                                                                            -- DIAGRAM_START
        ${JSON_INGEST_START}
        [=[
            {
                "diagram": [
                    {
                        "object_type": "script",
                        "object_id": "script.Argent.ListLedgers",
                        "object_ref": "${MIGRATION}",
                        "name": "Argent.ListLedgers"
                    },
                    {
                        "object_type": "script",
                        "object_id": "script.Argent.GetLedger",
                        "object_ref": "${MIGRATION}",
                        "name": "Argent.GetLedger"
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
