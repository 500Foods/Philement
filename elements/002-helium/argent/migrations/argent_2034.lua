-- Migration: argent_2034.lua
-- Argent post transaction
--
-- luacheck: no max line length
-- luacheck: no unused args
--
-- CHANGELOG
-- 1.0.0 - 2026-10-07 - MCP PostTransaction with balance, tax, and idempotency

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "scripts"
cfg.MIGRATION = "2034"
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
                'PostTransaction',
                1,
                NULL, NULL, NULL, NULL,
                1,
                [==[
-- Argent.PostTransaction
-- Stores caller amounts as given. After tax companions, every currency
-- sums to 0. Kind 6, 7, and 8 are rejected. Status may be 1, 2, or 3.
-- A manual tax more than 1 cent from the computed tax writes nothing.
-- There is no multi-statement transaction. A later insert failure
-- returns partial_write and the txn_id already stored.

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

local function nonempty(v)
    if type(v) ~= "string" or v == "" then return nil end
    return v
end

local function date_ok(v)
    return type(v) == "string" and v:match("^%d%d%d%d%-%d%d%-%d%d$") ~= nil
end

local function actor_id()
    local h = {}
    if type(params) == "table" and type(params._hydrogen) == "table" then
        h = params._hydrogen
    end
    return tonumber(h.user_id) or tonumber(h.sub) or 0
end

local function idem_key(raw)
    if raw == nil or raw == "" then return "" end
    if type(raw) ~= "string" then
        return nil, "idempotency_key must be a string"
    end
    if #raw > 80 then
        return nil, "idempotency_key is longer than 80 characters"
    end
    if raw:find('[%c"\\]') then
        return nil, "idempotency_key contains a forbidden character"
    end
    return raw
end

local function has_idem(col, key)
    if key == "" then return false end
    if type(col) == "table" then return col.idempotency_key == key end
    if type(col) ~= "string" then return false end
    if col:find('"idempotency_key":"' .. key .. '"', 1, true) then return true end
    if col:find('"idempotency_key": "' .. key .. '"', 1, true) then return true end
    return false
end

local function div_half_up(numer, denom)
    if denom < 0 then
        numer = -numer
        denom = -denom
    end
    if denom == 0 then return nil end
    local neg = numer < 0
    if neg then numer = -numer end
    local q = math.floor(numer / denom)
    local rem = numer - q * denom
    if rem * 2 >= denom then q = q + 1 end
    if neg then q = -q end
    return q
end

if type(params) ~= "table" then
    return fail("params_required", "params must be an object")
end
if type(params.lines) ~= "table" or #params.lines == 0 then
    return fail("lines_required", "lines must be a non-empty array")
end

local actor = actor_id()
local txn_on = params.txn_on
if not date_ok(txn_on) then return fail("txn_on", "txn_on must be YYYY-MM-DD") end
local description = nonempty(params.description)
if not description then return fail("description_required", "description is required") end
local memo = nonempty(params.memo) or ""
local kind = int(params.kind_a2004) or 11
if kind == 6 or kind == 7 or kind == 8 then
    return fail("kind", "kind 6, 7, and 8 are not posted by this tool")
end
if kind < 1 or kind > 11 then
    return fail("kind", "kind_a2004 must be 1 through 11")
end
local status = int(params.status_a2003) or 3
if status ~= 1 and status ~= 2 and status ~= 3 then
    return fail("status", "status_a2003 must be 1, 2, or 3")
end
local key, kerr = idem_key(params.idempotency_key)
if kerr then return fail("idempotency_key", kerr) end
local requested_org = int(params.organization_id)

local ledgers = {}
local function load_ledger(ledger_id)
    if ledgers[ledger_id] then return ledgers[ledger_id] end
    local res, err = H.query_sync([[
        SELECT ledger_id, organization_id, is_posting, currency
        FROM ${SCHEMA}ledgers
        WHERE ledger_id = :LEDGER_ID
    ]], { LEDGER_ID = ledger_id })
    if err then return nil, err end
    local row = qrows(res)[1]
    if not row then return nil, "missing" end
    local item = {
        ledger_id = ledger_id,
        organization_id = tonumber(pick(row, "organization_id")),
        is_posting = tonumber(pick(row, "is_posting")),
        currency = string.lower(tostring(pick(row, "currency") or "")),
    }
    ledgers[ledger_id] = item
    return item
end

local rates = {}
local function load_rate(tax_code_id)
    if rates[tax_code_id] then return rates[tax_code_id] end
    local cres, cerr = H.query_sync([[
        SELECT tax_code_id, organization_id, target_ledger_id
        FROM ${SCHEMA}tax_codes
        WHERE tax_code_id = :TAX_CODE_ID
    ]], { TAX_CODE_ID = tax_code_id })
    if cerr then return nil, cerr end
    local code = qrows(cres)[1]
    if not code then return nil, "missing" end
    local rres, rerr = H.query_sync([[
        SELECT rate_bps
        FROM ${SCHEMA}tax_rates
        WHERE tax_code_id = :TAX_CODE_ID
          AND effective_on <= :TXN_ON
        ORDER BY effective_on DESC, tax_rate_id DESC
    ]], { TAX_CODE_ID = tax_code_id, TXN_ON = txn_on })
    if rerr then return nil, rerr end
    local rate = qrows(rres)[1]
    if not rate then return nil, "no_rate" end
    local item = {
        organization_id = tonumber(pick(code, "organization_id")),
        target_ledger_id = tonumber(pick(code, "target_ledger_id")),
        rate_bps = tonumber(pick(rate, "rate_bps")),
    }
    rates[tax_code_id] = item
    return item
end

local org = requested_org
local prepared = {}
for i = 1, #params.lines do
    local line = params.lines[i]
    if type(line) ~= "table" then
        return fail("lines", "each line must be an object")
    end
    local ledger_id = int(line.ledger_id)
    local amount = int(line.amount_cents)
    if not ledger_id then return fail("ledger_id_required", "each line needs ledger_id") end
    if amount == nil then return fail("amount", "each line needs an integer amount_cents") end
    local ledger, lerr = load_ledger(ledger_id)
    if lerr == "missing" then return fail("ledger", "ledger not found") end
    if lerr then return fail("query_failed", tostring(lerr)) end
    if ledger.is_posting ~= 1 then
        return fail("ledger", "every line ledger must be posting")
    end
    if not org then org = ledger.organization_id end
    if ledger.organization_id ~= org then
        return fail("organization", "lines must share one organization")
    end
    local tax_code_id = int(line.tax_code_id)
    local tax_cents = nil
    local tax_manual = 0
    local manual = nil
    if line.tax_cents ~= nil then
        manual = int(line.tax_cents)
        if manual == nil then return fail("tax_cents", "tax_cents must be an integer") end
    end
    if tax_code_id then
        local rate, rerr = load_rate(tax_code_id)
        if rerr == "missing" then return fail("tax_code", "tax code not found") end
        if rerr == "no_rate" then
            return fail("no_tax_rate", "no tax rate is in force on txn_on")
        end
        if rerr then return fail("query_failed", tostring(rerr)) end
        if rate.organization_id ~= org then
            return fail("tax_code", "tax code is in another organization")
        end
        local target, terr = load_ledger(rate.target_ledger_id)
        if terr == "missing" then return fail("tax_ledger", "tax ledger not found") end
        if terr then return fail("query_failed", tostring(terr)) end
        if target.is_posting ~= 1 then
            return fail("tax_ledger", "tax ledger must be posting")
        end
        if target.currency ~= ledger.currency then
            return fail("tax_currency", "tax ledger currency must match the line")
        end
        if target.organization_id ~= org then
            return fail("tax_ledger", "tax ledger is in another organization")
        end
        local bps = rate.rate_bps or 0
        local computed
        local gross = line.tax_gross == true or line.tax_gross == 1
        if gross then
            computed = div_half_up(amount * bps, 10000 + bps)
        else
            computed = div_half_up(amount * bps, 10000)
        end
        if computed == nil then
            return fail("tax_rate", "tax rate cannot be divided")
        end
        local used = computed
        if manual ~= nil then
            local diff = manual - computed
            if diff < 0 then diff = -diff end
            if diff > 1 then
                return fail("needs_confirm", "tax differs by more than 1 cent", {
                    tax_code_id = tax_code_id,
                    computed_tax_cents = computed,
                    tax_cents = manual,
                })
            end
            used = manual
            tax_manual = 1
        end
        if gross then amount = amount - used end
        tax_cents = used
        prepared[#prepared + 1] = {
            ledger_id = ledger_id, amount = amount, currency = ledger.currency,
            tax_code_id = tax_code_id, tax_cents = tax_cents,
            tax_manual = tax_manual, memo = nonempty(line.memo) or "",
        }
        if used ~= 0 then
            prepared[#prepared + 1] = {
                ledger_id = target.ledger_id, amount = used, currency = target.currency,
                tax_code_id = nil, tax_cents = nil, tax_manual = 0, memo = "",
            }
        end
    else
        if manual ~= nil then
            return fail("tax_cents", "tax_cents requires tax_code_id")
        end
        prepared[#prepared + 1] = {
            ledger_id = ledger_id, amount = amount, currency = ledger.currency,
            tax_code_id = nil, tax_cents = nil, tax_manual = 0,
            memo = nonempty(line.memo) or "",
        }
    end
end

if requested_org and requested_org ~= org then
    return fail("organization", "organization_id does not match the lines")
end

if key ~= "" then
    local ires, ierr = H.query_sync([[
        SELECT txn_id, collection
        FROM ${SCHEMA}transactions
        WHERE organization_id = :ORG_ID
    ]], { ORG_ID = org })
    if ierr then return fail("query_failed", tostring(ierr)) end
    local irows = qrows(ires)
    for i = 1, #irows do
        if has_idem(pick(irows[i], "collection"), key) then
            return ok({
                txn_id = tonumber(pick(irows[i], "txn_id")),
                idempotent = true, created = false,
            })
        end
    end
end

local sums = {}
for i = 1, #prepared do
    local ccy = prepared[i].currency
    sums[ccy] = (sums[ccy] or 0) + prepared[i].amount
end
for _, total in pairs(sums) do
    if total ~= 0 then
        return fail("unbalanced", "each currency must sum to 0")
    end
end

local collection = "{}"
if key ~= "" then collection = '{"idempotency_key":"' .. key .. '"}' end

local nres, nerr = H.query_sync([[
    SELECT COALESCE(MAX(txn_id), 0) + 1 AS next_id
    FROM ${SCHEMA}transactions
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
        :TXN_ID, :ORG_ID, :STATUS_A2003, :KIND_A2004, :TXN_ON,
        :TXN_DESCRIPTION, NULLIF(:TXN_MEMO, ''), NULL, NULL,
        1, NULL, NULL,
        0, NULL, NULL, ${JIS}:TXN_COLLECTION${JIE},
        NULL, NULL, :ACTOR_CREATED, ${NOW}, :ACTOR_UPDATED, ${NOW}
    )
]], {
    TXN_ID = txn_id, ORG_ID = org, STATUS_A2003 = status, KIND_A2004 = kind,
    TXN_ON = txn_on, TXN_DESCRIPTION = description, TXN_MEMO = memo,
    TXN_COLLECTION = collection, ACTOR_CREATED = actor, ACTOR_UPDATED = actor,
})
if herr then return fail("insert_failed", tostring(herr)) end

local written = {}
for i = 1, #prepared do
    local line = prepared[i]
    local lres, lerr = H.query_sync([[
        SELECT COALESCE(MAX(line_id), 0) + 1 AS next_id FROM ${SCHEMA}lines
    ]], {})
    if lerr then
        return fail("partial_write", tostring(lerr), { txn_id = txn_id, lines = written })
    end
    local line_id = tonumber(pick(qrows(lres)[1], "next_id")) or 1
    local use_tax = line.tax_code_id and 1 or 0
    local use_cents = line.tax_cents ~= nil and 1 or 0
    local _, ierr = H.query_sync([[
        INSERT INTO ${SCHEMA}lines (
            line_id, txn_id, line_seq, ledger_id, amount_cents,
            tax_code_id, tax_cents, tax_manual, cleared,
            reconciliation_id, statement_txn_id, memo, collection,
            valid_after, valid_until, created_id, created_at, updated_id, updated_at
        ) VALUES (
            :LINE_ID, :TXN_ID, :LINE_SEQ, :LEDGER_ID, :AMOUNT_CENTS,
            CASE WHEN :USE_TAX = 0 THEN NULL ELSE :TAX_CODE_ID END,
            CASE WHEN :USE_TAX_CENTS = 0 THEN NULL ELSE :TAX_CENTS END,
            :TAX_MANUAL, 0,
            NULL, NULL, NULLIF(:LINE_MEMO, ''), ${JIS}:LINE_COLLECTION${JIE},
            NULL, NULL, :ACTOR_CREATED, ${NOW}, :ACTOR_UPDATED, ${NOW}
        )
    ]], {
        LINE_ID = line_id, TXN_ID = txn_id, LINE_SEQ = i,
        LEDGER_ID = line.ledger_id, AMOUNT_CENTS = line.amount,
        USE_TAX = use_tax, TAX_CODE_ID = line.tax_code_id or 0,
        USE_TAX_CENTS = use_cents, TAX_CENTS = line.tax_cents or 0,
        TAX_MANUAL = line.tax_manual, LINE_MEMO = line.memo,
        LINE_COLLECTION = "{}", ACTOR_CREATED = actor, ACTOR_UPDATED = actor,
    })
    if ierr then
        return fail("partial_write", tostring(ierr), { txn_id = txn_id, lines = written })
    end
    written[#written + 1] = {
        line_id = line_id, line_seq = i, ledger_id = line.ledger_id,
        amount_cents = line.amount, tax_code_id = line.tax_code_id,
        tax_cents = line.tax_cents, tax_manual = line.tax_manual,
    }
end

return ok({ txn_id = txn_id, created = true, lines = written })
                ]==],
                'Post a balanced Argent transaction',
                0,
                1,
                '{"inputSchema":{"type":"object","properties":{"organization_id":{"type":"integer"},"txn_on":{"type":"string"},"description":{"type":"string"},"memo":{"type":"string"},"kind_a2004":{"type":"integer"},"status_a2003":{"type":"integer"},"idempotency_key":{"type":"string"},"lines":{"type":"array"}},"required":["txn_on","description","lines"],"additionalProperties":true}}',
                '{"title":"Post transaction","idempotentHint":true}',
                ${COMMON_VALUES}
            );
            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Seed Argent PostTransaction'                                                    AS name,
        [=[
            # Forward Migration ${MIGRATION}: Argent post transaction

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
              AND script_name IN ('PostTransaction');

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Remove Argent PostTransaction'                                                    AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Remove Argent post transaction

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
        'Diagram Argent post transaction'                                                   AS name,
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
                        "object_id": "script.Argent.PostTransaction",
                        "object_ref": "${MIGRATION}",
                        "name": "Argent.PostTransaction"
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
