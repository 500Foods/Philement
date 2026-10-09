-- Migration: argent_2032.lua
-- Argent ledger upsert
--
-- luacheck: no max line length
-- luacheck: no unused args
--
-- CHANGELOG
-- 1.0.0 - 2026-10-07 - MCP ledger upsert and opening transaction
-- 1.0.1 - 2026-10-07 - Cast optional NULL, dates, and empty strings
-- 1.0.2 - 2026-10-07 - MySQL CAST targets; json parameters are cast before ingest

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "scripts"
cfg.MIGRATION = "2032"
cfg.GROUP_NAME = "Argent"
if engine == "mysql" then
    cfg.CAST_INTEGER = "signed"
    cfg.CAST_TEXT = "char(255)"
else
    cfg.CAST_INTEGER = cfg.INTEGER
    cfg.CAST_TEXT = cfg.TEXT
end
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
                'UpsertLedger',
                1,
                NULL, NULL, NULL, NULL,
                1,
                [==[
-- Argent.UpsertLedger
-- Insert when ledger_id is omitted. Update when it is present.
-- Currency is fixed after create. A posting create writes one opening
-- transaction (kind 6, status 3). A non-posting create writes none.
-- A nonzero opening requires offset_ledger_id. The new ledger receives
-- opening_balance_cents as given. The offset receives the negation.

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

local function partial(message, extra)
    extra = extra or {}
    extra.ok = false
    extra.code = "partial_write"
    extra.message = message
    H.set_result_json(extra)
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

local function flag01(v, default)
    if v == nil then return default end
    if v == true or v == 1 then return 1 end
    if v == false or v == 0 then return 0 end
    local n = num(v)
    if n == 1 then return 1 end
    if n == 0 then return 0 end
    return nil
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
    if type(col) == "table" then
        return col.idempotency_key == key
    end
    if type(col) ~= "string" then return false end
    if col:find('"idempotency_key":"' .. key .. '"', 1, true) then return true end
    if col:find('"idempotency_key": "' .. key .. '"', 1, true) then return true end
    return false
end

if type(params) ~= "table" then
    return fail("params_required", "params must be an object")
end

local actor = actor_id()
local id = int(params.ledger_id)
local key, kerr = idem_key(params.idempotency_key)
if kerr then return fail("idempotency_key", kerr) end

local function currency_known(code)
    local res, err = H.query_sync([[
        SELECT currency_code FROM ${SCHEMA}currencies
        WHERE currency_code = :CUR_CODE
    ]], { CUR_CODE = code })
    if err then return nil, err end
    return qrows(res)[1] ~= nil, nil
end

local function load_org(org_id)
    local res, err = H.query_sync([[
        SELECT organization_id FROM ${SCHEMA}organizations
        WHERE organization_id = :ORG_ID
    ]], { ORG_ID = org_id })
    if err then return nil, err end
    return qrows(res)[1] ~= nil, nil
end

local function load_ledger(ledger_id)
    local res, err = H.query_sync([[
        SELECT ledger_id, organization_id, parent_id, status_a2002,
               ledger_type_a2001, is_posting, name, currency,
               opening_on, opening_balance_cents, opening_txn_id,
               mask, external_ref, summary
        FROM ${SCHEMA}ledgers
        WHERE ledger_id = :LEDGER_ID
    ]], { LEDGER_ID = ledger_id })
    if err then return nil, err end
    local rows = qrows(res)
    return rows[1], nil
end

local existing = nil
if id then
    local row, lerr = load_ledger(id)
    if lerr then return fail("query_failed", tostring(lerr)) end
    if not row then return fail("not_found", "ledger not found") end
    existing = row
end

local org = int(params.organization_id)
local parent = int(params.parent_id)
local status = int(params.status_a2002)
local ledger_type = int(params.ledger_type_a2001)
local posting = flag01(params.is_posting, nil)
local name = nonempty(params.name)
local currency = nonempty(params.currency)
if currency then currency = string.lower(currency) end
local opening_on = nonempty(params.opening_on)
local opening_cents = int(params.opening_balance_cents)
local mask = nonempty(params.mask) or ""
local external_ref = nonempty(params.external_ref) or ""
local summary = nonempty(params.summary) or ""

if existing then
    if org and org ~= tonumber(pick(existing, "organization_id")) then
        return fail("organization_fixed", "organization_id cannot change")
    end
    org = tonumber(pick(existing, "organization_id"))
    if params.parent_id == nil then parent = int(pick(existing, "parent_id")) end
    if params.status_a2002 == nil then status = int(pick(existing, "status_a2002")) end
    if params.ledger_type_a2001 == nil then
        ledger_type = int(pick(existing, "ledger_type_a2001"))
    end
    if params.is_posting == nil then posting = int(pick(existing, "is_posting")) end
    if not name then name = pick(existing, "name") end
    local stored_ccy = string.lower(tostring(pick(existing, "currency") or ""))
    if currency and currency ~= stored_ccy then
        return fail("currency_fixed", "currency cannot change")
    end
    currency = stored_ccy
    if not opening_on then opening_on = pick(existing, "opening_on") end
    if opening_cents == nil then
        opening_cents = int(pick(existing, "opening_balance_cents"))
    end
    if params.mask == nil then mask = pick(existing, "mask") or "" end
    if params.external_ref == nil then
        external_ref = pick(existing, "external_ref") or ""
    end
    if params.summary == nil then summary = pick(existing, "summary") or "" end
end

if not org then return fail("organization_id_required", "organization_id is required") end
if not name then return fail("name_required", "name is required") end
if not status then status = 1 end
if status ~= 1 and status ~= 2 and status ~= 3 then
    return fail("status", "status_a2002 must be 1, 2, or 3")
end
if not ledger_type or ledger_type < 1 or ledger_type > 5 then
    return fail("ledger_type", "ledger_type_a2001 must be 1 through 5")
end
if posting == nil then posting = 1 end
if posting ~= 0 and posting ~= 1 then
    return fail("is_posting", "is_posting must be 0 or 1")
end
if not currency or not currency:match("^[a-z][a-z][a-z]$") then
    return fail("currency", "currency must be a lowercase ISO code")
end
if not date_ok(opening_on) then
    return fail("opening_on", "opening_on must be YYYY-MM-DD")
end
if opening_cents == nil then
    return fail("opening_balance", "opening_balance_cents is required")
end
if #mask > 50 then return fail("mask", "mask is longer than 50 characters") end
if #external_ref > 100 then
    return fail("external_ref", "external_ref is longer than 100 characters")
end
if parent and id and parent == id then
    return fail("parent", "parent_id cannot be the ledger itself")
end

local known, cerr = currency_known(currency)
if cerr then return fail("query_failed", tostring(cerr)) end
if not known then return fail("currency", "currency is not in currencies") end

local org_ok, oerr = load_org(org)
if oerr then return fail("query_failed", tostring(oerr)) end
if not org_ok then return fail("not_found", "organization not found") end

if parent then
    local prow, perr = load_ledger(parent)
    if perr then return fail("query_failed", tostring(perr)) end
    if not prow then return fail("parent", "parent ledger not found") end
    if tonumber(pick(prow, "organization_id")) ~= org then
        return fail("parent", "parent ledger is in another organization")
    end
end

local function find_idem_txn()
    if key == "" then return nil end
    local res, err = H.query_sync([[
        SELECT txn_id, collection
        FROM ${SCHEMA}transactions
        WHERE organization_id = :ORG_ID
    ]], { ORG_ID = org })
    if err then return nil, err end
    local rows = qrows(res)
    for i = 1, #rows do
        if has_idem(pick(rows[i], "collection"), key) then
            return tonumber(pick(rows[i], "txn_id"))
        end
    end
    return nil
end

if not existing then
    local prior, ierr = find_idem_txn()
    if ierr then return fail("query_failed", tostring(ierr)) end
    if prior then
        local lres, lerr = H.query_sync([[
            SELECT ledger_id FROM ${SCHEMA}ledgers
            WHERE opening_txn_id = :TXN_ID
        ]], { TXN_ID = prior })
        if lerr then return fail("query_failed", tostring(lerr)) end
        local lrows = qrows(lres)
        local lid = lrows[1] and tonumber(pick(lrows[1], "ledger_id")) or nil
        return ok({
            idempotent = true, created = false, txn_id = prior, ledger_id = lid,
        })
    end
end

local use_parent = parent and 1 or 0
local parent_bind = parent or 0

if existing then
    local _, uerr = H.query_sync([[
        UPDATE ${SCHEMA}ledgers
        SET parent_id = CASE WHEN CAST(:USE_PARENT AS ${CAST_INTEGER}) = 0 THEN CAST(NULL AS ${CAST_INTEGER}) ELSE CAST(:PARENT_ID AS ${CAST_INTEGER}) END,
            status_a2002 = :STATUS_A2002,
            ledger_type_a2001 = :LEDGER_TYPE,
            is_posting = :IS_POSTING,
            name = :LEDGER_NAME,
            currency = :CUR_CODE,
            opening_on = CAST(:OPENING_ON AS ${DATE}),
            opening_balance_cents = :OPENING_CENTS,
            mask = NULLIF(CAST(:LEDGER_MASK AS ${CAST_TEXT}), ''),
            external_ref = NULLIF(CAST(:LEDGER_REF AS ${CAST_TEXT}), ''),
            summary = NULLIF(CAST(:LEDGER_SUMMARY AS ${CAST_TEXT}), ''),
            updated_id = :ACTOR_UPDATED,
            updated_at = ${NOW}
        WHERE ledger_id = :LEDGER_ID
    ]], {
        USE_PARENT = use_parent, PARENT_ID = parent_bind,
        STATUS_A2002 = status, LEDGER_TYPE = ledger_type,
        IS_POSTING = posting, LEDGER_NAME = name, CUR_CODE = currency,
        OPENING_ON = opening_on, OPENING_CENTS = opening_cents,
        LEDGER_MASK = mask, LEDGER_REF = external_ref,
        LEDGER_SUMMARY = summary, ACTOR_UPDATED = actor, LEDGER_ID = id,
    })
    if uerr then return fail("update_failed", tostring(uerr)) end
    return ok({
        ledger_id = id, updated = true, created = false,
        opening_txn_id = int(pick(existing, "opening_txn_id")),
    })
end

local id_res, id_err = H.query_sync([[
    SELECT COALESCE(MAX(ledger_id), 0) + 1 AS next_id
    FROM ${SCHEMA}ledgers
]], {})
if id_err then return fail("query_failed", tostring(id_err)) end
local new_id = tonumber(pick(qrows(id_res)[1], "next_id")) or 1

local offset_id = int(params.offset_ledger_id)
if posting == 1 and opening_cents ~= 0 then
    if not offset_id then
        return fail("offset_required", "offset_ledger_id is required when opening_balance_cents is not 0")
    end
    if offset_id == new_id then
        return fail("offset", "offset_ledger_id cannot be the new ledger")
    end
    local orow, oerr2 = load_ledger(offset_id)
    if oerr2 then return fail("query_failed", tostring(oerr2)) end
    if not orow then return fail("offset", "offset ledger not found") end
    if tonumber(pick(orow, "organization_id")) ~= org then
        return fail("offset", "offset ledger is in another organization")
    end
    if string.lower(tostring(pick(orow, "currency") or "")) ~= currency then
        return fail("offset", "offset ledger currency does not match")
    end
    if tonumber(pick(orow, "is_posting")) ~= 1 then
        return fail("offset", "offset ledger must be posting")
    end
end

local _, ierr = H.query_sync([[
    INSERT INTO ${SCHEMA}ledgers (
        ledger_id, organization_id, parent_id, status_a2002, ledger_type_a2001,
        is_posting, name, currency, opening_on, opening_balance_cents,
        opening_txn_id, latest_reconciliation_id, latest_reconciled_on,
        calendar_url, calendar_id, mask, external_ref, summary, collection,
        valid_after, valid_until, created_id, created_at, updated_id, updated_at
    ) VALUES (
        :LEDGER_ID, :ORG_ID,
        CASE WHEN CAST(:USE_PARENT AS ${CAST_INTEGER}) = 0 THEN CAST(NULL AS ${CAST_INTEGER}) ELSE CAST(:PARENT_ID AS ${CAST_INTEGER}) END,
        :STATUS_A2002, :LEDGER_TYPE, :IS_POSTING, :LEDGER_NAME, :CUR_CODE,
        CAST(:OPENING_ON AS ${DATE}), :OPENING_CENTS, NULL,
        NULL, NULL, NULL, NULL,
        NULLIF(CAST(:LEDGER_MASK AS ${CAST_TEXT}), ''), NULLIF(CAST(:LEDGER_REF AS ${CAST_TEXT}), ''),
        NULLIF(CAST(:LEDGER_SUMMARY AS ${CAST_TEXT}), ''), ${JIS}CAST(:LEDGER_COLLECTION AS ${CAST_TEXT})${JIE},
        NULL, NULL, :ACTOR_CREATED, ${NOW}, :ACTOR_UPDATED, ${NOW}
    )
]], {
    LEDGER_ID = new_id, ORG_ID = org,
    USE_PARENT = use_parent, PARENT_ID = parent_bind,
    STATUS_A2002 = status, LEDGER_TYPE = ledger_type, IS_POSTING = posting,
    LEDGER_NAME = name, CUR_CODE = currency,
    OPENING_ON = opening_on, OPENING_CENTS = opening_cents,
    LEDGER_MASK = mask, LEDGER_REF = external_ref, LEDGER_SUMMARY = summary,
    LEDGER_COLLECTION = "{}",
    ACTOR_CREATED = actor, ACTOR_UPDATED = actor,
})
if ierr then return fail("insert_failed", tostring(ierr)) end

if posting ~= 1 then
    return ok({
        ledger_id = new_id, created = true, opening_txn_id = nil,
    })
end

local collection = "{}"
if key ~= "" then
    collection = '{"idempotency_key":"' .. key .. '"}'
end

local txn_res, txn_err = H.query_sync([[
    SELECT COALESCE(MAX(txn_id), 0) + 1 AS next_id
    FROM ${SCHEMA}transactions
]], {})
if txn_err then
    return partial("ledger inserted but the opening transaction id failed", {
        ledger_id = new_id,
    })
end
local txn_id = tonumber(pick(qrows(txn_res)[1], "next_id")) or 1

local _, terr = H.query_sync([[
    INSERT INTO ${SCHEMA}transactions (
        txn_id, organization_id, status_a2003, kind_a2004, txn_on,
        description, memo, schedule_id, replaces_txn_id,
        calendar_state_a2011, calendar_event_id, calendar_error,
        calendar_attempts, calendar_synced_at, summary, collection,
        valid_after, valid_until, created_id, created_at, updated_id, updated_at
    ) VALUES (
        :TXN_ID, :ORG_ID, 3, 6, CAST(:TXN_ON AS ${DATE}),
        :TXN_DESCRIPTION, NULL, NULL, NULL,
        1, NULL, NULL,
        0, NULL, NULL, ${JIS}CAST(:TXN_COLLECTION AS ${CAST_TEXT})${JIE},
        NULL, NULL, :ACTOR_CREATED, ${NOW}, :ACTOR_UPDATED, ${NOW}
    )
]], {
    TXN_ID = txn_id, ORG_ID = org, TXN_ON = opening_on,
    TXN_DESCRIPTION = "Opening balance", TXN_COLLECTION = collection,
    ACTOR_CREATED = actor, ACTOR_UPDATED = actor,
})
if terr then
    return partial("ledger inserted but the opening transaction was not", {
        ledger_id = new_id,
    })
end

local function insert_line(seq, ledger_id, amount)
    local nres, nerr = H.query_sync([[
        SELECT COALESCE(MAX(line_id), 0) + 1 AS next_id
        FROM ${SCHEMA}lines
    ]], {})
    if nerr then return nerr end
    local line_id = tonumber(pick(qrows(nres)[1], "next_id")) or 1
    local _, lerr = H.query_sync([[
        INSERT INTO ${SCHEMA}lines (
            line_id, txn_id, line_seq, ledger_id, amount_cents,
            tax_code_id, tax_cents, tax_manual, cleared,
            reconciliation_id, statement_txn_id, memo, collection,
            valid_after, valid_until, created_id, created_at, updated_id, updated_at
        ) VALUES (
            :LINE_ID, :TXN_ID, :LINE_SEQ, :LEDGER_ID, :AMOUNT_CENTS,
            NULL, NULL, 0, 0,
            NULL, NULL, NULL, ${JIS}CAST(:LINE_COLLECTION AS ${CAST_TEXT})${JIE},
            NULL, NULL, :ACTOR_CREATED, ${NOW}, :ACTOR_UPDATED, ${NOW}
        )
    ]], {
        LINE_ID = line_id, TXN_ID = txn_id, LINE_SEQ = seq,
        LEDGER_ID = ledger_id, AMOUNT_CENTS = amount,
        LINE_COLLECTION = "{}",
        ACTOR_CREATED = actor, ACTOR_UPDATED = actor,
    })
    if lerr then return lerr end
    return nil
end

local lerr1 = insert_line(1, new_id, opening_cents)
if lerr1 then
    return partial(tostring(lerr1), { ledger_id = new_id, txn_id = txn_id })
end
if opening_cents ~= 0 then
    local lerr2 = insert_line(2, offset_id, -opening_cents)
    if lerr2 then
        return partial(tostring(lerr2), { ledger_id = new_id, txn_id = txn_id })
    end
end

local _, uerr = H.query_sync([[
    UPDATE ${SCHEMA}ledgers
    SET opening_txn_id = :OPENING_TXN,
        updated_id = :ACTOR_UPDATED,
        updated_at = ${NOW}
    WHERE ledger_id = :LEDGER_ID
]], {
    OPENING_TXN = txn_id, ACTOR_UPDATED = actor, LEDGER_ID = new_id,
})
if uerr then
    return partial(tostring(uerr), { ledger_id = new_id, txn_id = txn_id })
end

return ok({
    ledger_id = new_id, created = true, opening_txn_id = txn_id,
})
                ]==],
                'Create or update an Argent ledger and its opening transaction',
                0,
                1,
                '{"inputSchema":{"type":"object","properties":{"ledger_id":{"type":"integer"},"organization_id":{"type":"integer"},"parent_id":{"type":"integer"},"status_a2002":{"type":"integer"},"ledger_type_a2001":{"type":"integer"},"is_posting":{"type":"boolean"},"name":{"type":"string"},"currency":{"type":"string"},"opening_on":{"type":"string"},"opening_balance_cents":{"type":"integer"},"offset_ledger_id":{"type":"integer"},"mask":{"type":"string"},"external_ref":{"type":"string"},"summary":{"type":"string"},"idempotency_key":{"type":"string"}},"additionalProperties":true}}',
                '{"title":"Upsert ledger","idempotentHint":true}',
                ${COMMON_VALUES}
            );
            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Seed Argent UpsertLedger'                                                    AS name,
        [=[
            # Forward Migration ${MIGRATION}: Argent ledger upsert

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
              AND script_name IN ('UpsertLedger');

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Remove Argent UpsertLedger'                                                    AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Remove Argent ledger upsert

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
        'Diagram Argent ledger upsert'                                                   AS name,
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
                        "object_id": "script.Argent.UpsertLedger",
                        "object_ref": "${MIGRATION}",
                        "name": "Argent.UpsertLedger"
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
