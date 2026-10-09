-- Migration: argent_2044.lua
-- Argent.CompleteReconciliation
--
-- luacheck: no max line length
-- luacheck: no unused args
--
-- CHANGELOG
-- 1.0.0 - 2026-10-08 - Complete a reconciliation or reject a bare override
-- 1.0.1 - 2026-10-08 - Skip cleared=1 when no line is selected (DB2 SQL0100W)

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "scripts"
cfg.MIGRATION = "2044"
cfg.GROUP_NAME = "Argent"
if engine == "mysql" then
    cfg.CAST_INTEGER = "signed"
    cfg.CAST_TEXT = "char(255)"
else
    cfg.CAST_INTEGER = cfg.INTEGER
    cfg.CAST_TEXT = cfg.TEXT
end
if engine == "firebird" then
    cfg.BALANCE_SUM = "CAST(COALESCE(SUM(ln.amount_cents), 0) AS BIGINT)"
else
    cfg.BALANCE_SUM = "COALESCE(SUM(ln.amount_cents), 0)"
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
                'CompleteReconciliation',
                1,
                NULL, NULL, NULL, NULL,
                1,
                [==[

-- Argent.CompleteReconciliation
-- Sets cleared on the chosen lines, writes the ledger's latest
-- reconciliation, and moves a transaction to Reconciled (key 4) when
-- every one of its lines on that ledger is cleared. When the statement
-- balance and the book balance differ, override_reason is required.
-- No confirm token. An already completed row is returned as already.

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

local function date_ok(v)
    return type(v) == "string" and v:match("^%d%d%d%d%-%d%d%-%d%d$") ~= nil
end

local function day_of(v)
    if type(v) ~= "string" then return nil end
    local d = v:sub(1, 10)
    if not date_ok(d) then return nil end
    return d
end

local function actor_id()
    local h = {}
    if type(params) == "table" and type(params._hydrogen) == "table" then
        h = params._hydrogen
    end
    return tonumber(h.user_id) or tonumber(h.sub) or 0
end

if type(params) ~= "table" then
    return fail("params_required", "params must be an object")
end
local recon_id = int(params.reconciliation_id)
if not recon_id then return fail("reconciliation_id_required", "reconciliation_id is required") end
local reason = ""
if params.override_reason ~= nil then
    if type(params.override_reason) ~= "string" then
        return fail("override_reason", "override_reason must be a string")
    end
    if #params.override_reason > 200 then
        return fail("override_reason", "override_reason is longer than 200 characters")
    end
    reason = params.override_reason
end
local actor = actor_id()

local rres, rerr = H.query_sync([[
    SELECT reconciliation_id, ledger_id, reconciled_on, statement_balance_cents, status_a2006
    FROM ${SCHEMA}reconciliations
    WHERE reconciliation_id = :RECON_ID
]], { RECON_ID = recon_id })
if rerr then return fail("query_failed", tostring(rerr)) end
local recon = qrows(rres)[1]
if not recon then return fail("not_found", "reconciliation not found") end
local status = tonumber(pick(recon, "status_a2006"))
if status == 2 then
    return ok({ reconciliation_id = recon_id, status_a2006 = 2, already = true })
end
if status ~= 1 then return fail("reconciliation_closed", "reconciliation is not open") end
local ledger_id = tonumber(pick(recon, "ledger_id"))
local reconciled_on = day_of(pick(recon, "reconciled_on"))
local statement_balance = tonumber(pick(recon, "statement_balance_cents"))
if not reconciled_on then return fail("reconciled_on", "stored reconciled_on is not a date") end
if statement_balance == nil then return fail("query_failed", "statement balance missing") end

local bres, berr = H.query_sync([[
    SELECT ${BALANCE_SUM} AS balance_cents
    FROM ${SCHEMA}lines ln
    INNER JOIN ${SCHEMA}transactions t ON t.txn_id = ln.txn_id
    WHERE ln.ledger_id = :LEDGER_ID
      AND t.status_a2003 IN (3, 4)
      AND t.txn_on <= CAST(:RECONCILED_ON AS ${DATE})
]], { LEDGER_ID = ledger_id, RECONCILED_ON = reconciled_on })
if berr then return fail("query_failed", tostring(berr)) end
local book = tonumber(pick(qrows(bres)[1] or {}, "balance_cents"))
if book == nil then return fail("query_failed", "balance missing") end
if book ~= statement_balance and reason == "" then
    return fail("override_reason_required", "balances differ", {
        statement_balance_cents = statement_balance, book_balance_cents = book,
    })
end

-- DB2 reports SQL0100W when an UPDATE matches zero rows. An open
-- reconciliation may have no selected lines.
local sres, serr = H.query_sync([[
    SELECT line_id
    FROM ${SCHEMA}lines
    WHERE reconciliation_id = :RECON_ID
]], { RECON_ID = recon_id })
if serr then return fail("query_failed", tostring(serr), { reconciliation_id = recon_id }) end
if qrows(sres)[1] then
    local _, cerr = H.query_sync([[
        UPDATE ${SCHEMA}lines
        SET cleared = 1,
            updated_id = :ACTOR_UPDATED,
            updated_at = ${NOW}
        WHERE reconciliation_id = :RECON_ID
    ]], { ACTOR_UPDATED = actor, RECON_ID = recon_id })
    if cerr then return fail("update_failed", tostring(cerr), { reconciliation_id = recon_id }) end
end

local tres, terr = H.query_sync([[
    SELECT t.txn_id
    FROM ${SCHEMA}transactions t
    INNER JOIN ${SCHEMA}lines ln ON ln.txn_id = t.txn_id
    WHERE ln.ledger_id = :LEDGER_ID
      AND t.status_a2003 IN (1, 2, 3)
    GROUP BY t.txn_id
    HAVING SUM(CASE WHEN ln.cleared <> 0 THEN 1 ELSE 0 END) = COUNT(*)
]], { LEDGER_ID = ledger_id })
if terr then return fail("partial_write", tostring(terr), { reconciliation_id = recon_id }) end
local trows = qrows(tres)
local promoted = {}
for i = 1, #trows do
    local txn_id = tonumber(pick(trows[i], "txn_id"))
    local _, uerr = H.query_sync([[
        UPDATE ${SCHEMA}transactions
        SET status_a2003 = 4,
            updated_id = :ACTOR_UPDATED,
            updated_at = ${NOW}
        WHERE txn_id = :TXN_ID
    ]], { ACTOR_UPDATED = actor, TXN_ID = txn_id })
    if uerr then
        return fail("partial_write", tostring(uerr), { reconciliation_id = recon_id })
    end
    promoted[#promoted + 1] = txn_id
end

local _, uerr = H.query_sync([[
    UPDATE ${SCHEMA}reconciliations
    SET status_a2006 = 2,
        book_balance_cents = :BOOK_BALANCE,
        override_reason = NULLIF(CAST(:OVERRIDE_REASON AS ${CAST_TEXT}), ''),
        updated_id = :ACTOR_UPDATED,
        updated_at = ${NOW}
    WHERE reconciliation_id = :RECON_ID
]], {
    BOOK_BALANCE = book, OVERRIDE_REASON = reason,
    ACTOR_UPDATED = actor, RECON_ID = recon_id,
})
if uerr then return fail("partial_write", tostring(uerr), { reconciliation_id = recon_id }) end

local _, lerr = H.query_sync([[
    UPDATE ${SCHEMA}ledgers
    SET latest_reconciliation_id = :RECON_ID,
        latest_reconciled_on = CAST(:RECONCILED_ON AS ${DATE}),
        updated_id = :ACTOR_UPDATED,
        updated_at = ${NOW}
    WHERE ledger_id = :LEDGER_ID
]], {
    RECON_ID = recon_id, RECONCILED_ON = reconciled_on,
    ACTOR_UPDATED = actor, LEDGER_ID = ledger_id,
})
if lerr then return fail("partial_write", tostring(lerr), { reconciliation_id = recon_id }) end

return ok({
    reconciliation_id = recon_id, status_a2006 = 2,
    statement_balance_cents = statement_balance, book_balance_cents = book,
    override = reason ~= "", txn_ids = promoted,
})

                ]==],
                'Complete a reconciliation',
                0,
                1,
                '{"inputSchema":{"type":"object","properties":{"reconciliation_id":{"type":"integer"},"override_reason":{"type":"string"}},"required":["reconciliation_id"],"additionalProperties":true}}',
                '{"title":"Complete reconciliation"}',
                ${COMMON_VALUES}
            );

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Seed Argent CompleteReconciliation'                                                     AS name,
        [=[
            # Forward Migration ${MIGRATION}: Argent.CompleteReconciliation

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
              AND script_name IN ('CompleteReconciliation');

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Remove Argent CompleteReconciliation'                                                     AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Remove Argent CompleteReconciliation

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
        'Diagram Argent CompleteReconciliation'                                                     AS name,
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
                        "object_id": "script.Argent.CompleteReconciliation",
                        "object_ref": "${MIGRATION}",
                        "name": "Argent.CompleteReconciliation"
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
