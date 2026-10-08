-- Migration: argent_2037.lua
-- Argent transaction reads
--
-- luacheck: no max line length
-- luacheck: no unused args
--
-- CHANGELOG
-- 1.0.0 - 2026-10-07 - MCP transaction reads and balance query
-- 1.0.1 - 2026-10-07 - Cast optional NULL, dates, and empty strings

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "scripts"
cfg.MIGRATION = "2037"
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
                'GetTransaction',
                1,
                NULL, NULL, NULL, NULL,
                1,
                [==[
-- Argent.GetTransaction
-- Header, lines, transaction tags (entity type 3), and attachment
-- meta. file_data and file_text are not returned.

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

if type(params) ~= "table" then
    return fail("params_required", "params must be an object")
end
local txn_id = num(params.txn_id)
if not txn_id then return fail("txn_id_required", "txn_id is required") end

local res, err = H.query_sync([[
    SELECT txn_id, organization_id, status_a2003, kind_a2004, txn_on,
           description, memo, schedule_id, replaces_txn_id,
           calendar_state_a2011, summary, collection
    FROM ${SCHEMA}transactions
    WHERE txn_id = :TXN_ID
]], { TXN_ID = txn_id })
if err then return fail("query_failed", tostring(err)) end
local row = qrows(res)[1]
if not row then return fail("not_found", "transaction not found") end
local header = {
    txn_id = pick(row, "txn_id"),
    organization_id = pick(row, "organization_id"),
    status_a2003 = pick(row, "status_a2003"),
    kind_a2004 = pick(row, "kind_a2004"),
    txn_on = pick(row, "txn_on"),
    description = pick(row, "description"),
    memo = pick(row, "memo"),
    schedule_id = pick(row, "schedule_id"),
    replaces_txn_id = pick(row, "replaces_txn_id"),
    calendar_state_a2011 = pick(row, "calendar_state_a2011"),
    summary = pick(row, "summary"),
    collection = pick(row, "collection"),
}

local lres, lerr = H.query_sync([[
    SELECT line_id, line_seq, ledger_id, amount_cents, tax_code_id,
           tax_cents, tax_manual, cleared, memo
    FROM ${SCHEMA}lines
    WHERE txn_id = :TXN_ID
    ORDER BY line_seq
]], { TXN_ID = txn_id })
if lerr then return fail("query_failed", tostring(lerr)) end
local lrows = qrows(lres)
local lines = {}
for i = 1, #lrows do
    local line = lrows[i]
    lines[i] = {
        line_id = pick(line, "line_id"),
        line_seq = pick(line, "line_seq"),
        ledger_id = pick(line, "ledger_id"),
        amount_cents = pick(line, "amount_cents"),
        tax_code_id = pick(line, "tax_code_id"),
        tax_cents = pick(line, "tax_cents"),
        tax_manual = pick(line, "tax_manual"),
        cleared = pick(line, "cleared"),
        memo = pick(line, "memo"),
    }
end

local tres, terr = H.query_sync([[
    SELECT tg.tag_id, tg.name, tg.organization_id, tl.tag_link_id
    FROM ${SCHEMA}tag_links tl
    INNER JOIN ${SCHEMA}tags tg ON tg.tag_id = tl.tag_id
    WHERE tl.entity_type_a2009 = 3 AND tl.entity_id = :TXN_ID
    ORDER BY tg.tag_id
]], { TXN_ID = txn_id })
if terr then return fail("query_failed", tostring(terr)) end
local trows = qrows(tres)
local tags = {}
for i = 1, #trows do
    local tag = trows[i]
    tags[i] = {
        tag_id = pick(tag, "tag_id"),
        name = pick(tag, "name"),
        organization_id = pick(tag, "organization_id"),
        tag_link_id = pick(tag, "tag_link_id"),
    }
end

local ares, aerr = H.query_sync([[
    SELECT attachment_id, rev_id, entity_type_a2009, entity_id, txn_id,
           att_type_a2010, mime_type, file_name, byte_len, name, summary
    FROM ${SCHEMA}attachments
    WHERE txn_id = :TXN_ID
       OR (entity_type_a2009 = 3 AND entity_id = :ENTITY_ID)
    ORDER BY attachment_id, rev_id
]], { TXN_ID = txn_id, ENTITY_ID = txn_id })
if aerr then return fail("query_failed", tostring(aerr)) end
local arows = qrows(ares)
local attachments = {}
for i = 1, #arows do
    local att = arows[i]
    attachments[i] = {
        attachment_id = pick(att, "attachment_id"),
        rev_id = pick(att, "rev_id"),
        entity_type_a2009 = pick(att, "entity_type_a2009"),
        entity_id = pick(att, "entity_id"),
        txn_id = pick(att, "txn_id"),
        att_type_a2010 = pick(att, "att_type_a2010"),
        mime_type = pick(att, "mime_type"),
        file_name = pick(att, "file_name"),
        byte_len = pick(att, "byte_len"),
        name = pick(att, "name"),
        summary = pick(att, "summary"),
    }
end

return ok({
    transaction = header, lines = lines, tags = tags, attachments = attachments,
})
                ]==],
                'Get one Argent transaction, lines, tags, and attachment meta',
                0,
                1,
                '{"inputSchema":{"type":"object","properties":{"txn_id":{"type":"integer"}},"required":["txn_id"],"additionalProperties":true}}',
                '{"title":"Get transaction","readOnlyHint":true}',
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
                'ListTransactions',
                1,
                NULL, NULL, NULL, NULL,
                1,
                [==[
-- Argent.ListTransactions
-- Requires organization_id or ledger_id. Headers only. No row limit.
-- An omitted status list returns every status.

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

local function int(v)
    local n = num(v)
    if not n or n ~= n or n % 1 ~= 0 then return nil end
    return n
end

local function date_ok(v)
    return type(v) == "string" and v:match("^%d%d%d%d%-%d%d%-%d%d$") ~= nil
end

if type(params) ~= "table" then params = {} end
local org = int(params.organization_id)
local ledger_id = int(params.ledger_id)
if not org and not ledger_id then
    return fail("filter_required", "organization_id or ledger_id is required")
end

local date_from = params.date_from
local date_to = params.date_to
if date_from ~= nil and date_from ~= "" and not date_ok(date_from) then
    return fail("date_from", "date_from must be YYYY-MM-DD")
end
if date_to ~= nil and date_to ~= "" and not date_ok(date_to) then
    return fail("date_to", "date_to must be YYYY-MM-DD")
end
if date_from == "" then date_from = nil end
if date_to == "" then date_to = nil end

local kind = int(params.kind)
if params.kind ~= nil and not kind then
    return fail("kind", "kind must be an integer")
end

local slots = { 0, 0, 0, 0, 0 }
local use_status = 0
if type(params.status) == "table" and #params.status > 0 then
    if #params.status > 5 then
        return fail("status", "status accepts at most 5 lookup 2003 keys")
    end
    use_status = 1
    for i = 1, #params.status do
        local n = int(params.status[i])
        if not n then return fail("status", "status keys must be integers") end
        slots[i] = n
    end
end

local res, err = H.query_sync([[
    SELECT DISTINCT t.txn_id, t.organization_id, t.status_a2003, t.kind_a2004,
           t.txn_on, t.description, t.memo
    FROM ${SCHEMA}transactions t
    WHERE (CAST(:USE_ORG AS ${INTEGER}) = 0 OR t.organization_id = CAST(:ORG_ID AS ${INTEGER}))
      AND (CAST(:USE_LEDGER AS ${INTEGER}) = 0 OR t.txn_id IN (
            SELECT ln.txn_id FROM ${SCHEMA}lines ln WHERE ln.ledger_id = CAST(:LEDGER_ID AS ${INTEGER})
          ))
      AND (CAST(:USE_FROM AS ${INTEGER}) = 0 OR t.txn_on >= CAST(:DATE_FROM AS ${DATE}))
      AND (CAST(:USE_TO AS ${INTEGER}) = 0 OR t.txn_on <= CAST(:DATE_TO AS ${DATE}))
      AND (CAST(:USE_KIND AS ${INTEGER}) = 0 OR t.kind_a2004 = CAST(:KIND_A2004 AS ${INTEGER}))
      AND (CAST(:USE_STATUS AS ${INTEGER}) = 0 OR t.status_a2003 IN (
            CAST(:STATUS_1 AS ${INTEGER}), CAST(:STATUS_2 AS ${INTEGER}), CAST(:STATUS_3 AS ${INTEGER}), CAST(:STATUS_4 AS ${INTEGER}), CAST(:STATUS_5 AS ${INTEGER})
          ))
    ORDER BY t.txn_on, t.txn_id
]], {
    USE_ORG = org and 1 or 0, ORG_ID = org or 0,
    USE_LEDGER = ledger_id and 1 or 0, LEDGER_ID = ledger_id or 0,
    USE_FROM = date_from and 1 or 0, DATE_FROM = date_from or "0001-01-01",
    USE_TO = date_to and 1 or 0, DATE_TO = date_to or "9999-12-31",
    USE_KIND = kind and 1 or 0, KIND_A2004 = kind or 0,
    USE_STATUS = use_status,
    STATUS_1 = slots[1], STATUS_2 = slots[2], STATUS_3 = slots[3],
    STATUS_4 = slots[4], STATUS_5 = slots[5],
})
if err then return fail("query_failed", tostring(err)) end
local rows = qrows(res)
local out = {}
for i = 1, #rows do
    local row = rows[i]
    out[i] = {
        txn_id = pick(row, "txn_id"),
        organization_id = pick(row, "organization_id"),
        status_a2003 = pick(row, "status_a2003"),
        kind_a2004 = pick(row, "kind_a2004"),
        txn_on = pick(row, "txn_on"),
        description = pick(row, "description"),
        memo = pick(row, "memo"),
    }
end
return ok({ transactions = out })
                ]==],
                'List Argent transaction headers',
                0,
                1,
                '{"inputSchema":{"type":"object","properties":{"organization_id":{"type":"integer"},"ledger_id":{"type":"integer"},"date_from":{"type":"string"},"date_to":{"type":"string"},"status":{"type":"array","items":{"type":"integer"}},"kind":{"type":"integer"}},"additionalProperties":true}}',
                '{"title":"List transactions","readOnlyHint":true}',
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
                'QueryBalances',
                1,
                NULL, NULL, NULL, NULL,
                1,
                [==[
-- Argent.QueryBalances
-- organization_id and as_of are required. Omitted status uses lookup
-- 2003 keys 3 and 4. include_parents also calls QueryRef 2001.
-- The stored query text is loaded and run. This file does not install it.

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

local function int(v)
    local n = num(v)
    if not n or n ~= n or n % 1 ~= 0 then return nil end
    return n
end

local function date_ok(v)
    return type(v) == "string" and v:match("^%d%d%d%d%-%d%d%-%d%d$") ~= nil
end

local function status_binds(list)
    local slots = { 0, 0, 0, 0, 0 }
    local use_default = 1
    if type(list) == "table" and #list > 0 then
        if #list > 5 then return nil, "status accepts at most 5 lookup 2003 keys" end
        use_default = 0
        for i = 1, #list do
            local n = int(list[i])
            if not n then return nil, "status keys must be integers" end
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

local function map_rows(rows, fields)
    local out = {}
    for i = 1, #rows do
        local item = {}
        for f = 1, #fields do
            item[fields[f]] = pick(rows[i], fields[f])
        end
        out[i] = item
    end
    return out
end

if type(params) ~= "table" then
    return fail("params_required", "params must be an object")
end
local org = int(params.organization_id)
if not org then
    return fail("organization_id_required", "organization_id is required")
end
local as_of = params.as_of
if not date_ok(as_of) then return fail("as_of", "as_of must be YYYY-MM-DD") end

local binds, serr = status_binds(params.status)
if not binds then return fail("status", serr) end
binds.ORGANIZATION_ID = org
binds.AS_OF = as_of

local bres, berr = run_ref(2000, binds)
if berr then return fail("balance_failed", tostring(berr)) end
local balances = map_rows(qrows(bres), {
    "ledger_id", "name", "currency", "balance_cents",
})

local include = params.include_parents == true or params.include_parents == 1
local parents = nil
if include then
    local rate = int(params.rate_source)
    if params.rate_source ~= nil and not rate then
        return fail("rate_source", "rate_source must be an integer")
    end
    if rate and (rate < 1 or rate > 6) then
        return fail("rate_source", "rate_source must be lookup 2012 key 1 through 6")
    end
    binds.USE_RATE_DEFAULT = rate and 0 or 1
    binds.RATE_SOURCE = rate or 0
    local pres, perr = run_ref(2001, binds)
    if perr then return fail("rollup_failed", tostring(perr)) end
    parents = map_rows(qrows(pres), {
        "parent_ledger_id", "parent_name", "parent_currency",
        "child_ledger_id", "child_name", "child_currency",
        "balance_cents", "rate_n", "rate_d", "rate_as_of",
        "converted_cents", "rate_warning",
    })
end

return ok({
    balances = balances, parents = parents, include_parents = include, as_of = as_of,
})
                ]==],
                'Query Argent balances and optional parent rollup',
                0,
                1,
                '{"inputSchema":{"type":"object","properties":{"organization_id":{"type":"integer"},"as_of":{"type":"string"},"status":{"type":"array","items":{"type":"integer"}},"include_parents":{"type":"boolean"},"rate_source":{"type":"integer"}},"required":["organization_id","as_of"],"additionalProperties":true}}',
                '{"title":"Query balances","readOnlyHint":true}',
                ${COMMON_VALUES}
            );
            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Seed Argent transaction read tools'                                                    AS name,
        [=[
            # Forward Migration ${MIGRATION}: Argent transaction reads

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
              AND script_name IN ('GetTransaction', 'ListTransactions', 'QueryBalances');

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Remove Argent transaction read tools'                                                    AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Remove Argent transaction reads

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
        'Diagram Argent transaction reads'                                                   AS name,
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
                        "object_id": "script.Argent.GetTransaction",
                        "object_ref": "${MIGRATION}",
                        "name": "Argent.GetTransaction"
                    },
                    {
                        "object_type": "script",
                        "object_id": "script.Argent.ListTransactions",
                        "object_ref": "${MIGRATION}",
                        "name": "Argent.ListTransactions"
                    },
                    {
                        "object_type": "script",
                        "object_id": "script.Argent.QueryBalances",
                        "object_ref": "${MIGRATION}",
                        "name": "Argent.QueryBalances"
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
