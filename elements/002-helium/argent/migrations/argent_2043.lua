-- Migration: argent_2043.lua
-- Argent.ClearLines
--
-- luacheck: no max line length
-- luacheck: no unused args
--
-- CHANGELOG
-- 1.0.0 - 2026-10-08 - Choose lines for an open reconciliation
-- 1.0.1 - 2026-10-08 - Skip the unlink when no line is selected (DB2 SQL0100W)

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "scripts"
cfg.MIGRATION = "2043"
cfg.GROUP_NAME = "Argent"
if engine == "mysql" then
    cfg.CAST_INTEGER = "signed"
    cfg.CAST_TEXT = "char(255)"
else
    cfg.CAST_INTEGER = cfg.INTEGER
    cfg.CAST_TEXT = cfg.TEXT
end
if engine == "firebird" then
    cfg.BODY_VALUE = "CAST(:BODY AS VARCHAR(8191))"
else
    cfg.BODY_VALUE = ":BODY"
end
if engine == "sqlite" then
    cfg.EXPIRES_AT = "datetime('now', '+15 minutes')"
    cfg.NOW_CMP = "datetime('now')"
elseif engine == "mysql" or engine == "mariadb" then
    cfg.EXPIRES_AT = "DATE_ADD(CURRENT_TIMESTAMP, INTERVAL 15 MINUTE)"
    cfg.NOW_CMP = "CURRENT_TIMESTAMP"
elseif engine == "db2" then
    cfg.EXPIRES_AT = "(CURRENT TIMESTAMP + 15 MINUTES)"
    cfg.NOW_CMP = "CURRENT TIMESTAMP"
elseif engine == "mssql" then
    cfg.EXPIRES_AT = "DATEADD(minute, 15, SYSDATETIMEOFFSET())"
    cfg.NOW_CMP = "SYSDATETIMEOFFSET()"
elseif engine == "firebird" then
    cfg.EXPIRES_AT = "DATEADD(MINUTE, 15, CURRENT_TIMESTAMP)"
    cfg.NOW_CMP = "CURRENT_TIMESTAMP"
else
    cfg.EXPIRES_AT = "(CURRENT_TIMESTAMP + INTERVAL '15 minutes')"
    cfg.NOW_CMP = "CURRENT_TIMESTAMP"
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
                'ClearLines',
                1,
                NULL, NULL, NULL, NULL,
                1,
                [==[
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

local function day_of(v)
    if type(v) ~= "string" then return nil end
    local d = v:sub(1, 10)
    if not date_ok(d) then return nil end
    return d
end

local function on_or_before(txn_on, boundary)
    local a, b = day_of(txn_on), day_of(boundary)
    if not a or not b then return false end
    return a <= b
end

local function actor_id()
    local h = {}
    if type(params) == "table" and type(params._hydrogen) == "table" then
        h = params._hydrogen
    end
    return tonumber(h.user_id) or tonumber(h.sub) or 0
end

local function esc(s)
    return (s:gsub("[%z\1-\31\\\"]", function(c)
        if c == "\\" then return "\\\\" end
        if c == "\"" then return "\\\"" end
        if c == "\n" then return "\\n" end
        if c == "\r" then return "\\r" end
        if c == "\t" then return "\\t" end
        return string.format("\\u%04x", string.byte(c))
    end))
end

local function canonical(v, skip)
    local t = type(v)
    if v == nil then return "null" end
    if t == "boolean" then return v and "true" or "false" end
    if t == "number" then
        if v ~= v or v == math.huge or v == -math.huge then return "null" end
        if v % 1 == 0 and math.abs(v) < 1e15 then return string.format("%d", v) end
        return string.format("%.15g", v)
    end
    if t == "string" then return "\"" .. esc(v) .. "\"" end
    if t ~= "table" then return "null" end
    local n, count, arr = #v, 0, true
    for k in pairs(v) do
        count = count + 1
        if type(k) ~= "number" or k < 1 or k % 1 ~= 0 then arr = false end
    end
    if arr and count == n then
        local parts = {}
        for i = 1, n do parts[i] = canonical(v[i], false) end
        return "[" .. table.concat(parts, ",") .. "]"
    end
    local keys = {}
    for k in pairs(v) do
        if not (skip and (k == "confirm_token" or k == "_hydrogen")) then
            keys[#keys + 1] = k
        end
    end
    table.sort(keys, function(a, b) return tostring(a) < tostring(b) end)
    local parts = {}
    for i = 1, #keys do
        local key = keys[i]
        parts[i] = "\"" .. esc(tostring(key)) .. "\":" .. canonical(v[key], false)
    end
    return "{" .. table.concat(parts, ",") .. "}"
end

local function make_token(confirm_id)
    local frac = math.floor((os.clock() % 1) * 1000000)
    return string.format("c%d-%d-%d", confirm_id, os.time(), frac)
end

local function issue_token(tool_name, body, actor)
    if #body > 4000 then return nil, "body_too_long" end
    local nres, nerr = H.query_sync([[
        SELECT COALESCE(MAX(confirm_id), 0) + 1 AS next_id
        FROM ${SCHEMA}confirm_tokens
    ]], {})
    if nerr then return nil, nerr end
    local confirm_id = tonumber(pick(qrows(nres)[1], "next_id")) or 1
    local token = make_token(confirm_id)
    local _, ierr = H.query_sync([[
        INSERT INTO ${SCHEMA}confirm_tokens (
            confirm_id, token, tool_name, body, account_id,
            expires_at, used_at,
            valid_after, valid_until, created_id, created_at, updated_id, updated_at
        ) VALUES (
            :CONFIRM_ID, :TOKEN, :TOOL_NAME, ${BODY_VALUE}, :ACCOUNT_ID,
            ${EXPIRES_AT}, NULL,
            NULL, NULL, :ACTOR_CREATED, ${NOW}, :ACTOR_UPDATED, ${NOW}
        )
    ]], {
        CONFIRM_ID = confirm_id, TOKEN = token, TOOL_NAME = tool_name,
        BODY = body, ACCOUNT_ID = actor,
        ACTOR_CREATED = actor, ACTOR_UPDATED = actor,
    })
    if ierr then return nil, ierr end
    return token
end

local function take_token(tool_name, body, actor, token)
    local res, err = H.query_sync([[
        SELECT confirm_id, tool_name, body, account_id,
               CASE WHEN used_at IS NULL THEN 0 ELSE 1 END AS is_used,
               CASE WHEN expires_at > ${NOW_CMP} THEN 0 ELSE 1 END AS is_expired
        FROM ${SCHEMA}confirm_tokens
        WHERE token = :TOKEN
    ]], { TOKEN = token })
    if err then return nil, "confirm_failed" end
    local row = qrows(res)[1]
    if not row then return nil, "confirm_not_found" end
    if (tonumber(pick(row, "is_used")) or 1) ~= 0 then return nil, "confirm_used" end
    if (tonumber(pick(row, "is_expired")) or 1) ~= 0 then return nil, "confirm_expired" end
    if tostring(pick(row, "tool_name") or "") ~= tool_name then return nil, "confirm_tool" end
    if (tonumber(pick(row, "account_id")) or -1) ~= actor then return nil, "confirm_account" end
    if tostring(pick(row, "body") or "") ~= body then return nil, "confirm_mismatch" end
    return tonumber(pick(row, "confirm_id"))
end

local function consume_token(confirm_id, actor)
    local _, err = H.query_sync([[
        UPDATE ${SCHEMA}confirm_tokens
        SET used_at = ${NOW}, updated_id = :ACTOR_UPDATED, updated_at = ${NOW}
        WHERE confirm_id = :CONFIRM_ID AND used_at IS NULL
    ]], { CONFIRM_ID = confirm_id, ACTOR_UPDATED = actor })
    return err
end

local function gate(tool_name, actor, warning, ref_id, ref_key)
    local body = canonical(params, true)
    local token_text = nonempty(params.confirm_token)
    if token_text then
        local id, code = take_token(tool_name, body, actor, token_text)
        if not id then
            fail(code or "confirm_failed", code or "confirm_failed")
            return nil
        end
        return id
    end
    if not warning then return false end
    local token, ierr = issue_token(tool_name, body, actor)
    if not token then
        local code = ierr == "body_too_long" and "body_too_long" or "confirm_insert_failed"
        fail(code, tostring(ierr or code))
        return nil
    end
    local extra = { needs_confirm = true, warning = warning, confirm_token = token }
    if ref_id ~= nil then extra[ref_key or "txn_id"] = ref_id end
    fail("needs_confirm", warning, extra)
    return nil
end

local function close_covers(org, txn_on, self_id)
    local res, err = H.query_sync([[
        SELECT txn_id, txn_on
        FROM ${SCHEMA}transactions
        WHERE organization_id = :ORG_ID
          AND kind_a2004 = 8
          AND status_a2003 <> 5
    ]], { ORG_ID = org })
    if err then return nil, err end
    local day = day_of(txn_on)
    if not day then return false end
    local rows = qrows(res)
    for i = 1, #rows do
        local id = tonumber(pick(rows[i], "txn_id"))
        if id ~= self_id and on_or_before(day, pick(rows[i], "txn_on")) then
            return true
        end
    end
    return false
end

-- Argent.ClearLines
-- Replaces the uncleared selection on an open reconciliation.
-- Does not set cleared. That happens in CompleteReconciliation.
-- A line dated on or before latest_reconciled_on, or inside a period
-- close, returns needs_confirm and writes nothing.

if type(params) ~= "table" then
    return fail("params_required", "params must be an object")
end
local recon_id = int(params.reconciliation_id)
if not recon_id then return fail("reconciliation_id_required", "reconciliation_id is required") end
if type(params.line_ids) ~= "table" then return fail("line_ids", "line_ids must be an array") end
local n, count = #params.line_ids, 0
for k in pairs(params.line_ids) do
    count = count + 1
    if type(k) ~= "number" then return fail("line_ids", "line_ids must be an array") end
end
if count ~= n then return fail("line_ids", "line_ids must be an array") end
local actor = actor_id()

local rres, rerr = H.query_sync([[
    SELECT r.reconciliation_id, r.ledger_id, r.statement_txn_id, r.reconciled_on,
           r.status_a2006, lg.organization_id
    FROM ${SCHEMA}reconciliations r
    INNER JOIN ${SCHEMA}ledgers lg ON lg.ledger_id = r.ledger_id
    WHERE r.reconciliation_id = :RECON_ID
]], { RECON_ID = recon_id })
if rerr then return fail("query_failed", tostring(rerr)) end
local recon = qrows(rres)[1]
if not recon then return fail("not_found", "reconciliation not found") end
if tonumber(pick(recon, "status_a2006")) ~= 1 then
    return fail("reconciliation_closed", "reconciliation is not open")
end
local ledger_id = tonumber(pick(recon, "ledger_id"))
local statement_txn_id = tonumber(pick(recon, "statement_txn_id"))
local reconciled_on = day_of(pick(recon, "reconciled_on"))
local org = tonumber(pick(recon, "organization_id"))
if not reconciled_on then return fail("reconciled_on", "stored reconciled_on is not a date") end

local ids = {}
local seen = {}
local warning = nil
for i = 1, n do
    local line_id = int(params.line_ids[i])
    if not line_id then return fail("line_ids", "each line id must be an integer") end
    if not seen[line_id] then
        seen[line_id] = true
        local lres, lerr = H.query_sync([[
            SELECT ln.line_id, ln.ledger_id, ln.cleared, ln.reconciliation_id,
                   t.status_a2003, t.txn_on, t.organization_id, lg.latest_reconciled_on
            FROM ${SCHEMA}lines ln
            INNER JOIN ${SCHEMA}transactions t ON t.txn_id = ln.txn_id
            INNER JOIN ${SCHEMA}ledgers lg ON lg.ledger_id = ln.ledger_id
            WHERE ln.line_id = :LINE_ID
        ]], { LINE_ID = line_id })
        if lerr then return fail("query_failed", tostring(lerr)) end
        local line = qrows(lres)[1]
        if not line then return fail("line_not_found", "line not found") end
        if tonumber(pick(line, "ledger_id")) ~= ledger_id then
            return fail("ledger_mismatch", "line is on another ledger")
        end
        if tonumber(pick(line, "organization_id")) ~= org then
            return fail("organization_id", "line is in another organization")
        end
        local status = tonumber(pick(line, "status_a2003"))
        if status == 4 then return fail("reconciled_line", "line belongs to a reconciled transaction") end
        if status == 5 then return fail("rescinded_line", "line belongs to a rescinded transaction") end
        local linked = tonumber(pick(line, "reconciliation_id"))
        local cleared = tonumber(pick(line, "cleared")) or 0
        if cleared ~= 0 and linked ~= recon_id then
            return fail("already_cleared", "line is already cleared")
        end
        if linked and linked ~= recon_id then
            return fail("line_linked", "line is on another reconciliation")
        end
        local txn_on = day_of(pick(line, "txn_on"))
        if not txn_on then return fail("txn_on", "stored txn_on is not a date") end
        if not on_or_before(txn_on, reconciled_on) then
            return fail("line_after_statement", "line is after reconciled_on")
        end
        if not warning and on_or_before(txn_on, pick(line, "latest_reconciled_on")) then
            warning = "reconciled date"
        end
        if not warning then
            local covered, cerr = close_covers(org, txn_on, nil)
            if cerr then return fail("query_failed", tostring(cerr)) end
            if covered then warning = "period close boundary" end
        end
        ids[#ids + 1] = line_id
    end
end

local held = gate("Argent.ClearLines", actor, warning, recon_id, "reconciliation_id")
if held == nil then return 0 end

-- DB2 reports SQL0100W when an UPDATE matches zero rows. A fresh
-- reconciliation has nothing linked, so probe before the unlink.
local pres, perr = H.query_sync([[
    SELECT line_id
    FROM ${SCHEMA}lines
    WHERE reconciliation_id = :RECON_ID AND cleared = 0
]], { RECON_ID = recon_id })
if perr then return fail("query_failed", tostring(perr), { reconciliation_id = recon_id }) end
if qrows(pres)[1] then
    local _, uerr = H.query_sync([[
        UPDATE ${SCHEMA}lines
        SET reconciliation_id = NULL,
            statement_txn_id = NULL,
            updated_id = :ACTOR_UPDATED,
            updated_at = ${NOW}
        WHERE reconciliation_id = :RECON_ID AND cleared = 0
    ]], { ACTOR_UPDATED = actor, RECON_ID = recon_id })
    if uerr then return fail("update_failed", tostring(uerr), { reconciliation_id = recon_id }) end
end

for i = 1, #ids do
    local _, lerr = H.query_sync([[
        UPDATE ${SCHEMA}lines
        SET reconciliation_id = :RECON_ID,
            statement_txn_id = :STATEMENT_TXN_ID,
            updated_id = :ACTOR_UPDATED,
            updated_at = ${NOW}
        WHERE line_id = :LINE_ID AND cleared = 0
    ]], {
        RECON_ID = recon_id, STATEMENT_TXN_ID = statement_txn_id,
        ACTOR_UPDATED = actor, LINE_ID = ids[i],
    })
    if lerr then
        return fail("partial_write", tostring(lerr), { reconciliation_id = recon_id })
    end
end

if type(held) == "number" then
    local cerr = consume_token(held, actor)
    if cerr then
        return fail("confirm_consume_failed", tostring(cerr), { reconciliation_id = recon_id })
    end
end

return ok({ reconciliation_id = recon_id, line_ids = ids, status_a2006 = 1 })

                ]==],
                'Choose lines for an open reconciliation',
                0,
                1,
                '{"inputSchema":{"type":"object","properties":{"reconciliation_id":{"type":"integer"},"line_ids":{"type":"array","items":{"type":"integer"}},"confirm_token":{"type":"string"}},"required":["reconciliation_id","line_ids"],"additionalProperties":true}}',
                '{"title":"Clear lines"}',
                ${COMMON_VALUES}
            );

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Seed Argent ClearLines'                                                     AS name,
        [=[
            # Forward Migration ${MIGRATION}: Argent.ClearLines

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
              AND script_name IN ('ClearLines');

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Remove Argent ClearLines'                                                     AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Remove Argent ClearLines

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
        'Diagram Argent ClearLines'                                                     AS name,
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
                        "object_id": "script.Argent.ClearLines",
                        "object_ref": "${MIGRATION}",
                        "name": "Argent.ClearLines"
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
