-- Migration: argent_2040.lua
-- Argent.PostStatement
--
-- luacheck: no max line length
-- luacheck: no unused args
--
-- CHANGELOG
-- 1.0.0 - 2026-10-08 - Post a zero-amount statement

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "scripts"
cfg.MIGRATION = "2040"
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
                'PostStatement',
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

-- Argent.PostStatement
-- Kind 7. One line of amount 0. A date on or before latest_reconciled_on,
-- or a period close that covers that date, returns needs_confirm and
-- writes nothing. The same body plus confirm_token creates the row.

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

local function collection_json(key, balance)
    local parts = {}
    if key ~= "" then parts[#parts + 1] = '"idempotency_key":"' .. key .. '"' end
    if balance ~= nil then
        parts[#parts + 1] = '"statement_balance_cents":' .. string.format("%d", balance)
    end
    if #parts == 0 then return "{}" end
    return "{" .. table.concat(parts, ",") .. "}"
end

if type(params) ~= "table" then
    return fail("params_required", "params must be an object")
end
local ledger_id = int(params.ledger_id)
if not ledger_id then return fail("ledger_id_required", "ledger_id is required") end
local txn_on = params.txn_on
if not date_ok(txn_on) then return fail("txn_on", "txn_on must be YYYY-MM-DD") end
local description = nonempty(params.description)
if not description then return fail("description_required", "description is required") end
local memo = ""
if params.memo ~= nil then
    if type(params.memo) ~= "string" then return fail("memo", "memo must be a string") end
    memo = params.memo
end
local balance = nil
if params.statement_balance_cents ~= nil then
    balance = int(params.statement_balance_cents)
    if balance == nil then
        return fail("statement_balance_cents", "statement_balance_cents must be an integer")
    end
end
local key, kerr = idem_key(params.idempotency_key)
if kerr then return fail("idempotency_key", kerr) end
local actor = actor_id()

local lres, lerr = H.query_sync([[
    SELECT ledger_id, organization_id, is_posting, latest_reconciled_on
    FROM ${SCHEMA}ledgers
    WHERE ledger_id = :LEDGER_ID
]], { LEDGER_ID = ledger_id })
if lerr then return fail("query_failed", tostring(lerr)) end
local ledger = qrows(lres)[1]
if not ledger then return fail("not_found", "ledger not found") end
if (tonumber(pick(ledger, "is_posting")) or 0) ~= 1 then
    return fail("ledger", "ledger is not posting")
end
local org = tonumber(pick(ledger, "organization_id"))
local requested = int(params.organization_id)
if requested and requested ~= org then
    return fail("organization_id", "ledger is in another organization")
end

if key ~= "" then
    local ires, ierr = H.query_sync([[
        SELECT txn_id, kind_a2004, collection
        FROM ${SCHEMA}transactions
        WHERE organization_id = :ORG_ID
    ]], { ORG_ID = org })
    if ierr then return fail("query_failed", tostring(ierr)) end
    local irows = qrows(ires)
    for i = 1, #irows do
        if tonumber(pick(irows[i], "kind_a2004")) == 7
            and has_idem(pick(irows[i], "collection"), key) then
            return ok({
                txn_id = tonumber(pick(irows[i], "txn_id")),
                kind_a2004 = 7, idempotent = true, created = false,
            })
        end
    end
end

local warning = nil
if on_or_before(txn_on, pick(ledger, "latest_reconciled_on")) then
    warning = "reconciled date"
end
if not warning then
    local covered, cerr = close_covers(org, txn_on, nil)
    if cerr then return fail("query_failed", tostring(cerr)) end
    if covered then warning = "period close boundary" end
end

local held = gate("Argent.PostStatement", actor, warning, ledger_id, "ledger_id")
if held == nil then return 0 end
if type(held) == "number" then
    local uerr = consume_token(held, actor)
    if uerr then return fail("confirm_consume_failed", tostring(uerr)) end
end

local nres, nerr = H.query_sync([[
    SELECT COALESCE(MAX(txn_id), 0) + 1 AS next_id
    FROM ${SCHEMA}transactions
]], {})
if nerr then return fail("query_failed", tostring(nerr)) end
local txn_id = tonumber(pick(qrows(nres)[1], "next_id")) or 1
local collection = collection_json(key, balance)

local _, herr = H.query_sync([[
    INSERT INTO ${SCHEMA}transactions (
        txn_id, organization_id, status_a2003, kind_a2004, txn_on,
        description, memo, schedule_id, replaces_txn_id,
        calendar_state_a2011, calendar_event_id, calendar_error,
        calendar_attempts, calendar_synced_at, summary, collection,
        valid_after, valid_until, created_id, created_at, updated_id, updated_at
    ) VALUES (
        :TXN_ID, :ORG_ID, 3, 7, CAST(:TXN_ON AS ${DATE}),
        :TXN_DESCRIPTION, NULLIF(CAST(:TXN_MEMO AS ${CAST_TEXT}), ''), NULL, NULL,
        1, NULL, NULL,
        0, NULL, NULL, ${JIS}CAST(:TXN_COLLECTION AS ${CAST_TEXT})${JIE},
        NULL, NULL, :ACTOR_CREATED, ${NOW}, :ACTOR_UPDATED, ${NOW}
    )
]], {
    TXN_ID = txn_id, ORG_ID = org, TXN_ON = txn_on,
    TXN_DESCRIPTION = description, TXN_MEMO = memo,
    TXN_COLLECTION = collection, ACTOR_CREATED = actor, ACTOR_UPDATED = actor,
})
if herr then return fail("insert_failed", tostring(herr)) end

local idres, iderr = H.query_sync([[
    SELECT COALESCE(MAX(line_id), 0) + 1 AS next_id FROM ${SCHEMA}lines
]], {})
if iderr then return fail("partial_write", tostring(iderr), { txn_id = txn_id }) end
local line_id = tonumber(pick(qrows(idres)[1], "next_id")) or 1
local _, ierr = H.query_sync([[
    INSERT INTO ${SCHEMA}lines (
        line_id, txn_id, line_seq, ledger_id, amount_cents,
        tax_code_id, tax_cents, tax_manual, cleared,
        reconciliation_id, statement_txn_id, memo, collection,
        valid_after, valid_until, created_id, created_at, updated_id, updated_at
    ) VALUES (
        :LINE_ID, :TXN_ID, 1, :LEDGER_ID, 0,
        NULL, NULL, 0, 0,
        NULL, NULL, NULL, ${JIS}CAST(:LINE_COLLECTION AS ${CAST_TEXT})${JIE},
        NULL, NULL, :ACTOR_CREATED, ${NOW}, :ACTOR_UPDATED, ${NOW}
    )
]], {
    LINE_ID = line_id, TXN_ID = txn_id, LEDGER_ID = ledger_id,
    LINE_COLLECTION = "{}", ACTOR_CREATED = actor, ACTOR_UPDATED = actor,
})
if ierr then return fail("partial_write", tostring(ierr), { txn_id = txn_id }) end

return ok({
    txn_id = txn_id, line_id = line_id, kind_a2004 = 7, created = true,
    statement_balance_cents = balance,
})

                ]==],
                'Post a zero-amount statement transaction',
                0,
                1,
                '{"inputSchema":{"type":"object","properties":{"ledger_id":{"type":"integer"},"txn_on":{"type":"string"},"description":{"type":"string"},"memo":{"type":"string"},"organization_id":{"type":"integer"},"idempotency_key":{"type":"string"},"statement_balance_cents":{"type":"integer"},"confirm_token":{"type":"string"}},"required":["ledger_id","txn_on","description"],"additionalProperties":true}}',
                '{"title":"Post statement","idempotentHint":true}',
                ${COMMON_VALUES}
            );

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Seed Argent PostStatement'                                                     AS name,
        [=[
            # Forward Migration ${MIGRATION}: Argent.PostStatement

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
              AND script_name IN ('PostStatement');

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Remove Argent PostStatement'                                                     AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Remove Argent PostStatement

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
        'Diagram Argent PostStatement'                                                     AS name,
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
                        "object_id": "script.Argent.PostStatement",
                        "object_ref": "${MIGRATION}",
                        "name": "Argent.PostStatement"
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
