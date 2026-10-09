-- Migration: argent_2039.lua
-- Argent.EditTransaction and Argent.RescindTransaction
--
-- luacheck: no max line length
-- luacheck: no unused args
--
-- CHANGELOG
-- 1.0.0 - 2026-10-08 - Confirm token, edit, and rescind

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "scripts"
cfg.MIGRATION = "2039"
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
                'EditTransaction',
                1,
                NULL, NULL, NULL, NULL,
                1,
                [==[
-- Argent.EditTransaction
-- A reconciled transaction, a date on or before latest_reconciled_on,
-- or a period close that covers the date writes nothing and returns
-- needs_confirm. The same body plus confirm_token applies the edit,
-- sets status 3, and clears recon links. A rescinded row is rejected.
-- Line patches replace amount_cents. Each currency must still sum to 0.

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

local function gate(tool_name, actor, warning, txn_id)
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
    fail("needs_confirm", warning, {
        needs_confirm = true, warning = warning, confirm_token = token, txn_id = txn_id,
    })
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

if type(params) ~= "table" then
    return fail("params_required", "params must be an object")
end
local txn_id = int(params.txn_id)
if not txn_id then return fail("txn_id_required", "txn_id is required") end
local actor = actor_id()

local tres, terr = H.query_sync([[
    SELECT txn_id, organization_id, status_a2003, kind_a2004, txn_on, description, memo
    FROM ${SCHEMA}transactions
    WHERE txn_id = :TXN_ID
]], { TXN_ID = txn_id })
if terr then return fail("query_failed", tostring(terr)) end
local txn = qrows(tres)[1]
if not txn then return fail("not_found", "transaction not found") end
local status = tonumber(pick(txn, "status_a2003"))
local kind = tonumber(pick(txn, "kind_a2004"))
if status == 5 then return fail("rescinded", "transaction is rescinded") end
local org = tonumber(pick(txn, "organization_id"))
local old_on = day_of(pick(txn, "txn_on"))
if not old_on then return fail("txn_on", "stored txn_on is not a date") end

local changed = false
local new_on = old_on
if params.txn_on ~= nil then
    if not date_ok(params.txn_on) then return fail("txn_on", "txn_on must be YYYY-MM-DD") end
    new_on = params.txn_on
    changed = true
end
local new_desc = pick(txn, "description")
if type(new_desc) ~= "string" or new_desc == "" then
    return fail("description_required", "stored description is empty")
end
if params.description ~= nil then
    new_desc = nonempty(params.description)
    if not new_desc then return fail("description_required", "description is required") end
    changed = true
end
local new_memo = pick(txn, "memo")
if type(new_memo) ~= "string" then new_memo = "" end
if params.memo ~= nil then
    if type(params.memo) ~= "string" then return fail("memo", "memo must be a string") end
    new_memo = params.memo
    changed = true
end

local lres, lerr = H.query_sync([[
    SELECT ln.line_id, ln.amount_cents, ln.memo, lg.currency, lg.latest_reconciled_on
    FROM ${SCHEMA}lines ln
    INNER JOIN ${SCHEMA}ledgers lg ON lg.ledger_id = ln.ledger_id
    WHERE ln.txn_id = :TXN_ID
    ORDER BY ln.line_seq
]], { TXN_ID = txn_id })
if lerr then return fail("query_failed", tostring(lerr)) end
local lrows = qrows(lres)
local by_id = {}
for i = 1, #lrows do
    local lid = tonumber(pick(lrows[i], "line_id"))
    by_id[lid] = lrows[i]
end

local patches = {}
if params.lines ~= nil then
    if type(params.lines) ~= "table" then return fail("lines", "lines must be an array") end
    for i = 1, #params.lines do
        local item = params.lines[i]
        if type(item) ~= "table" then return fail("lines", "each line must be an object") end
        local lid = int(item.line_id)
        local amt = int(item.amount_cents)
        if not lid or amt == nil then
            return fail("lines", "line_id and amount_cents are required")
        end
        if item.memo ~= nil and type(item.memo) ~= "string" then
            return fail("lines", "memo must be a string")
        end
        if not by_id[lid] then return fail("line_not_found", "line is not on this transaction") end
        patches[lid] = { amount = amt, memo = item.memo }
    end
    changed = true
end
if not changed then return fail("edit_empty", "no editable field was sent") end

local sums = {}
local writes = {}
for i = 1, #lrows do
    local line = lrows[i]
    local lid = tonumber(pick(line, "line_id"))
    local amt = tonumber(pick(line, "amount_cents")) or 0
    local memo = pick(line, "memo")
    if type(memo) ~= "string" then memo = "" end
    local patch = patches[lid]
    if patch then
        amt = patch.amount
        if patch.memo ~= nil then memo = patch.memo end
        writes[#writes + 1] = { line_id = lid, amount = amt, memo = memo }
    end
    local ccy = tostring(pick(line, "currency") or "")
    sums[ccy] = (sums[ccy] or 0) + amt
end
for _, total in pairs(sums) do
    if total ~= 0 then return fail("unbalanced", "each currency must sum to 0") end
end

local warning = nil
if status == 4 then
    warning = "reconciled transaction"
elseif kind == 8 then
    warning = "period close"
end
if not warning then
    for i = 1, #lrows do
        local boundary = pick(lrows[i], "latest_reconciled_on")
        if on_or_before(old_on, boundary) or on_or_before(new_on, boundary) then
            warning = "reconciled date"
            break
        end
    end
end
if not warning then
    local covered, cerr = close_covers(org, old_on, txn_id)
    if cerr then return fail("query_failed", tostring(cerr)) end
    if not covered and new_on ~= old_on then
        covered, cerr = close_covers(org, new_on, txn_id)
        if cerr then return fail("query_failed", tostring(cerr)) end
    end
    if covered then warning = "period close boundary" end
end

local held = gate("Argent.EditTransaction", actor, warning, txn_id)
if held == nil then return 0 end

local new_status = warning and 3 or status
local _, herr = H.query_sync([[
    UPDATE ${SCHEMA}transactions
    SET txn_on = CAST(:TXN_ON AS ${DATE}),
        description = :TXN_DESCRIPTION,
        memo = NULLIF(CAST(:TXN_MEMO AS ${CAST_TEXT}), ''),
        status_a2003 = :STATUS_A2003,
        updated_id = :ACTOR_UPDATED,
        updated_at = ${NOW}
    WHERE txn_id = :TXN_ID
]], {
    TXN_ON = new_on, TXN_DESCRIPTION = new_desc, TXN_MEMO = new_memo,
    STATUS_A2003 = new_status, ACTOR_UPDATED = actor, TXN_ID = txn_id,
})
if herr then return fail("update_failed", tostring(herr), { txn_id = txn_id }) end

for i = 1, #writes do
    local line = writes[i]
    local _, uerr = H.query_sync([[
        UPDATE ${SCHEMA}lines
        SET amount_cents = :AMOUNT_CENTS,
            memo = NULLIF(CAST(:LINE_MEMO AS ${CAST_TEXT}), ''),
            updated_id = :ACTOR_UPDATED,
            updated_at = ${NOW}
        WHERE line_id = :LINE_ID
          AND txn_id = :TXN_ID
    ]], {
        AMOUNT_CENTS = line.amount, LINE_MEMO = line.memo,
        ACTOR_UPDATED = actor, LINE_ID = line.line_id, TXN_ID = txn_id,
    })
    if uerr then
        return fail("partial_write", tostring(uerr), { txn_id = txn_id })
    end
end

if warning then
    local _, cerr = H.query_sync([[
        UPDATE ${SCHEMA}lines
        SET cleared = 0,
            reconciliation_id = NULL,
            statement_txn_id = NULL,
            updated_id = :ACTOR_UPDATED,
            updated_at = ${NOW}
        WHERE txn_id = :TXN_ID
    ]], { ACTOR_UPDATED = actor, TXN_ID = txn_id })
    if cerr then return fail("partial_write", tostring(cerr), { txn_id = txn_id }) end
end

if type(held) == "number" then
    local uerr = consume_token(held, actor)
    if uerr then
        return fail("confirm_consume_failed", tostring(uerr), { txn_id = txn_id })
    end
end

return ok({
    txn_id = txn_id, status_a2003 = new_status, knocked = warning ~= nil,
})
                ]==],
                'Edit an Argent transaction, with confirm on a closed period',
                0,
                1,
                '{"inputSchema":{"type":"object","properties":{"txn_id":{"type":"integer"},"txn_on":{"type":"string"},"description":{"type":"string"},"memo":{"type":"string"},"lines":{"type":"array"},"confirm_token":{"type":"string"}},"required":["txn_id"],"additionalProperties":true}}',
                '{"title":"Edit transaction"}',
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
                'RescindTransaction',
                1,
                NULL, NULL, NULL, NULL,
                1,
                [==[
-- Argent.RescindTransaction
-- Sets status 5. The warn path matches EditTransaction. A confirmed
-- rescind clears recon links and does not leave the row Recorded.
-- An already rescinded row returns without a second write.

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

local function gate(tool_name, actor, warning, txn_id)
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
    fail("needs_confirm", warning, {
        needs_confirm = true, warning = warning, confirm_token = token, txn_id = txn_id,
    })
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

if type(params) ~= "table" then
    return fail("params_required", "params must be an object")
end
local txn_id = int(params.txn_id)
if not txn_id then return fail("txn_id_required", "txn_id is required") end
local actor = actor_id()

local tres, terr = H.query_sync([[
    SELECT txn_id, organization_id, status_a2003, kind_a2004, txn_on
    FROM ${SCHEMA}transactions
    WHERE txn_id = :TXN_ID
]], { TXN_ID = txn_id })
if terr then return fail("query_failed", tostring(terr)) end
local txn = qrows(tres)[1]
if not txn then return fail("not_found", "transaction not found") end
local status = tonumber(pick(txn, "status_a2003"))
if status == 5 then
    return ok({ txn_id = txn_id, status_a2003 = 5, already = true })
end
local kind = tonumber(pick(txn, "kind_a2004"))
local org = tonumber(pick(txn, "organization_id"))
local txn_on = day_of(pick(txn, "txn_on"))
if not txn_on then return fail("txn_on", "stored txn_on is not a date") end

local lres, lerr = H.query_sync([[
    SELECT lg.latest_reconciled_on
    FROM ${SCHEMA}lines ln
    INNER JOIN ${SCHEMA}ledgers lg ON lg.ledger_id = ln.ledger_id
    WHERE ln.txn_id = :TXN_ID
]], { TXN_ID = txn_id })
if lerr then return fail("query_failed", tostring(lerr)) end
local lrows = qrows(lres)

local warning = nil
if status == 4 then
    warning = "reconciled transaction"
elseif kind == 8 then
    warning = "period close"
end
if not warning then
    for i = 1, #lrows do
        if on_or_before(txn_on, pick(lrows[i], "latest_reconciled_on")) then
            warning = "reconciled date"
            break
        end
    end
end
if not warning then
    local covered, cerr = close_covers(org, txn_on, txn_id)
    if cerr then return fail("query_failed", tostring(cerr)) end
    if covered then warning = "period close boundary" end
end

local held = gate("Argent.RescindTransaction", actor, warning, txn_id)
if held == nil then return 0 end

local _, herr = H.query_sync([[
    UPDATE ${SCHEMA}transactions
    SET status_a2003 = 5,
        updated_id = :ACTOR_UPDATED,
        updated_at = ${NOW}
    WHERE txn_id = :TXN_ID
]], { ACTOR_UPDATED = actor, TXN_ID = txn_id })
if herr then return fail("update_failed", tostring(herr), { txn_id = txn_id }) end

local _, cerr = H.query_sync([[
    UPDATE ${SCHEMA}lines
    SET cleared = 0,
        reconciliation_id = NULL,
        statement_txn_id = NULL,
        updated_id = :ACTOR_UPDATED,
        updated_at = ${NOW}
    WHERE txn_id = :TXN_ID
]], { ACTOR_UPDATED = actor, TXN_ID = txn_id })
if cerr then return fail("partial_write", tostring(cerr), { txn_id = txn_id }) end

if type(held) == "number" then
    local uerr = consume_token(held, actor)
    if uerr then
        return fail("confirm_consume_failed", tostring(uerr), { txn_id = txn_id })
    end
end

return ok({ txn_id = txn_id, status_a2003 = 5, knocked = warning ~= nil })
                ]==],
                'Rescind an Argent transaction, with confirm on a closed period',
                0,
                1,
                '{"inputSchema":{"type":"object","properties":{"txn_id":{"type":"integer"},"confirm_token":{"type":"string"}},"required":["txn_id"],"additionalProperties":true}}',
                '{"title":"Rescind transaction"}',
                ${COMMON_VALUES}
            );

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Seed Argent edit and rescind tools'                                AS name,
        [=[
            # Forward Migration ${MIGRATION}: Argent edit and rescind

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
              AND script_name IN ('EditTransaction', 'RescindTransaction');

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Remove Argent edit and rescind tools'                              AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Remove Argent edit and rescind

            Deletes those script rows. Does not drop confirm_tokens.
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
        'Diagram Argent edit and rescind'                                   AS name,
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
                        "object_id": "script.Argent.EditTransaction",
                        "object_ref": "${MIGRATION}",
                        "name": "Argent.EditTransaction"
                    },
                    {
                        "object_type": "script",
                        "object_id": "script.Argent.RescindTransaction",
                        "object_ref": "${MIGRATION}",
                        "name": "Argent.RescindTransaction"
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
