-- Migration: argent_2033.lua
-- Argent ledger terms and contacts
--
-- luacheck: no max line length
-- luacheck: no unused args
--
-- CHANGELOG
-- 1.0.0 - 2026-10-07 - MCP ledger terms and contact upserts

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "scripts"
cfg.MIGRATION = "2033"
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
                'UpsertLedgerTerms',
                1,
                NULL, NULL, NULL, NULL,
                1,
                [==[
-- Argent.UpsertLedgerTerms
-- Updates ledger_term_id when it is present. Otherwise updates the row
-- with the same ledger and effective_on, or inserts one.
-- Omitted amounts stay as stored on update and are null on insert.

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

local function pair(value)
    if value == nil then return 0, 0 end
    return 1, value
end

if type(params) ~= "table" then
    return fail("params_required", "params must be an object")
end

local actor = actor_id()
local term_id = int(params.ledger_term_id)
local ledger_id = int(params.ledger_id)
local effective_on = nonempty(params.effective_on)
local summary = nonempty(params.summary) or ""

local function opt_int(field, label, lo, hi)
    if params[field] == nil then return nil end
    local n = int(params[field])
    if not n then return nil, label .. " must be an integer" end
    if lo and (n < lo or n > hi) then
        return nil, label .. " must be " .. lo .. " through " .. hi
    end
    return n
end

local credit, e1 = opt_int("credit_limit_cents", "credit_limit_cents")
if e1 then return fail("credit_limit_cents", e1) end
local od, e2 = opt_int("od_limit_cents", "od_limit_cents")
if e2 then return fail("od_limit_cents", e2) end
local apr_p, e3 = opt_int("apr_purchase_bps", "apr_purchase_bps")
if e3 then return fail("apr_purchase_bps", e3) end
local apr_c, e4 = opt_int("apr_cash_bps", "apr_cash_bps")
if e4 then return fail("apr_cash_bps", e4) end
local fee, e5 = opt_int("annual_fee_cents", "annual_fee_cents")
if e5 then return fail("annual_fee_cents", e5) end
local close_day, e6 = opt_int("statement_close_day", "statement_close_day", 1, 31)
if e6 then return fail("statement_close_day", e6) end
local due_off, e7 = opt_int("payment_due_offset_days", "payment_due_offset_days")
if e7 then return fail("payment_due_offset_days", e7) end

local existing = nil
if term_id then
    local res, err = H.query_sync([[
        SELECT ledger_term_id, ledger_id, effective_on, credit_limit_cents,
               od_limit_cents, apr_purchase_bps, apr_cash_bps, annual_fee_cents,
               statement_close_day, payment_due_offset_days, summary
        FROM ${SCHEMA}ledger_terms
        WHERE ledger_term_id = :TERM_ID
    ]], { TERM_ID = term_id })
    if err then return fail("query_failed", tostring(err)) end
    local rows = qrows(res)
    if not rows[1] then return fail("not_found", "ledger terms not found") end
    existing = rows[1]
    if not ledger_id then ledger_id = int(pick(existing, "ledger_id")) end
    if not effective_on then effective_on = pick(existing, "effective_on") end
    if params.credit_limit_cents == nil then credit = int(pick(existing, "credit_limit_cents")) end
    if params.od_limit_cents == nil then od = int(pick(existing, "od_limit_cents")) end
    if params.apr_purchase_bps == nil then apr_p = int(pick(existing, "apr_purchase_bps")) end
    if params.apr_cash_bps == nil then apr_c = int(pick(existing, "apr_cash_bps")) end
    if params.annual_fee_cents == nil then fee = int(pick(existing, "annual_fee_cents")) end
    if params.statement_close_day == nil then
        close_day = int(pick(existing, "statement_close_day"))
    end
    if params.payment_due_offset_days == nil then
        due_off = int(pick(existing, "payment_due_offset_days"))
    end
    if params.summary == nil then summary = pick(existing, "summary") or "" end
end

if not ledger_id then return fail("ledger_id_required", "ledger_id is required") end
if not date_ok(effective_on) then
    return fail("effective_on", "effective_on must be YYYY-MM-DD")
end

local lres, lerr = H.query_sync([[
    SELECT ledger_id FROM ${SCHEMA}ledgers WHERE ledger_id = :LEDGER_ID
]], { LEDGER_ID = ledger_id })
if lerr then return fail("query_failed", tostring(lerr)) end
if not qrows(lres)[1] then return fail("not_found", "ledger not found") end

local pres, perr = H.query_sync([[
    SELECT ledger_term_id, credit_limit_cents, od_limit_cents,
           apr_purchase_bps, apr_cash_bps, annual_fee_cents,
           statement_close_day, payment_due_offset_days, summary
    FROM ${SCHEMA}ledger_terms
    WHERE ledger_id = :LEDGER_ID AND effective_on = :EFFECTIVE_ON
    ORDER BY ledger_term_id
]], { LEDGER_ID = ledger_id, EFFECTIVE_ON = effective_on })
if perr then return fail("query_failed", tostring(perr)) end
local pair_row = qrows(pres)[1]
local pair_id = pair_row and int(pick(pair_row, "ledger_term_id")) or nil
if term_id and pair_id and pair_id ~= term_id then
    return fail("duplicate", "ledger terms already exist for that date")
end
if not term_id and pair_row then
    term_id = pair_id
    if params.credit_limit_cents == nil then credit = int(pick(pair_row, "credit_limit_cents")) end
    if params.od_limit_cents == nil then od = int(pick(pair_row, "od_limit_cents")) end
    if params.apr_purchase_bps == nil then apr_p = int(pick(pair_row, "apr_purchase_bps")) end
    if params.apr_cash_bps == nil then apr_c = int(pick(pair_row, "apr_cash_bps")) end
    if params.annual_fee_cents == nil then fee = int(pick(pair_row, "annual_fee_cents")) end
    if params.statement_close_day == nil then
        close_day = int(pick(pair_row, "statement_close_day"))
    end
    if params.payment_due_offset_days == nil then
        due_off = int(pick(pair_row, "payment_due_offset_days"))
    end
    if params.summary == nil then summary = pick(pair_row, "summary") or "" end
end

local use_credit, credit_b = pair(credit)
local use_od, od_b = pair(od)
local use_apr_p, apr_p_b = pair(apr_p)
local use_apr_c, apr_c_b = pair(apr_c)
local use_fee, fee_b = pair(fee)
local use_close, close_b = pair(close_day)
local use_due, due_b = pair(due_off)

local binds = {
    LEDGER_ID = ledger_id, EFFECTIVE_ON = effective_on,
    USE_CREDIT = use_credit, CREDIT_LIMIT = credit_b,
    USE_OD = use_od, OD_LIMIT = od_b,
    USE_APR_P = use_apr_p, APR_PURCHASE = apr_p_b,
    USE_APR_C = use_apr_c, APR_CASH = apr_c_b,
    USE_FEE = use_fee, ANNUAL_FEE = fee_b,
    USE_CLOSE = use_close, CLOSE_DAY = close_b,
    USE_DUE = use_due, DUE_OFFSET = due_b,
    TERM_SUMMARY = summary, ACTOR_UPDATED = actor,
}

if term_id then
    binds.TERM_ID = term_id
    local _, uerr = H.query_sync([[
        UPDATE ${SCHEMA}ledger_terms
        SET ledger_id = :LEDGER_ID,
            effective_on = :EFFECTIVE_ON,
            credit_limit_cents = CASE WHEN :USE_CREDIT = 0 THEN NULL ELSE :CREDIT_LIMIT END,
            od_limit_cents = CASE WHEN :USE_OD = 0 THEN NULL ELSE :OD_LIMIT END,
            apr_purchase_bps = CASE WHEN :USE_APR_P = 0 THEN NULL ELSE :APR_PURCHASE END,
            apr_cash_bps = CASE WHEN :USE_APR_C = 0 THEN NULL ELSE :APR_CASH END,
            annual_fee_cents = CASE WHEN :USE_FEE = 0 THEN NULL ELSE :ANNUAL_FEE END,
            statement_close_day = CASE WHEN :USE_CLOSE = 0 THEN NULL ELSE :CLOSE_DAY END,
            payment_due_offset_days = CASE WHEN :USE_DUE = 0 THEN NULL ELSE :DUE_OFFSET END,
            summary = NULLIF(:TERM_SUMMARY, ''),
            updated_id = :ACTOR_UPDATED,
            updated_at = ${NOW}
        WHERE ledger_term_id = :TERM_ID
    ]], binds)
    if uerr then return fail("update_failed", tostring(uerr)) end
    return ok({ ledger_term_id = term_id, updated = true, created = false })
end

local nres, nerr = H.query_sync([[
    SELECT COALESCE(MAX(ledger_term_id), 0) + 1 AS next_id
    FROM ${SCHEMA}ledger_terms
]], {})
if nerr then return fail("query_failed", tostring(nerr)) end
local new_id = tonumber(pick(qrows(nres)[1], "next_id")) or 1
binds.TERM_ID = new_id
binds.ACTOR_CREATED = actor
binds.TERM_COLLECTION = "{}"

local _, ierr = H.query_sync([[
    INSERT INTO ${SCHEMA}ledger_terms (
        ledger_term_id, ledger_id, effective_on,
        credit_limit_cents, od_limit_cents, apr_purchase_bps, apr_cash_bps,
        annual_fee_cents, statement_close_day, payment_due_offset_days,
        summary, collection,
        valid_after, valid_until, created_id, created_at, updated_id, updated_at
    ) VALUES (
        :TERM_ID, :LEDGER_ID, :EFFECTIVE_ON,
        CASE WHEN :USE_CREDIT = 0 THEN NULL ELSE :CREDIT_LIMIT END,
        CASE WHEN :USE_OD = 0 THEN NULL ELSE :OD_LIMIT END,
        CASE WHEN :USE_APR_P = 0 THEN NULL ELSE :APR_PURCHASE END,
        CASE WHEN :USE_APR_C = 0 THEN NULL ELSE :APR_CASH END,
        CASE WHEN :USE_FEE = 0 THEN NULL ELSE :ANNUAL_FEE END,
        CASE WHEN :USE_CLOSE = 0 THEN NULL ELSE :CLOSE_DAY END,
        CASE WHEN :USE_DUE = 0 THEN NULL ELSE :DUE_OFFSET END,
        NULLIF(:TERM_SUMMARY, ''), ${JIS}:TERM_COLLECTION${JIE},
        NULL, NULL, :ACTOR_CREATED, ${NOW}, :ACTOR_UPDATED, ${NOW}
    )
]], binds)
if ierr then return fail("insert_failed", tostring(ierr)) end
return ok({ ledger_term_id = new_id, created = true, updated = false })
                ]==],
                'Create or update Argent ledger terms',
                0,
                1,
                '{"inputSchema":{"type":"object","properties":{"ledger_term_id":{"type":"integer"},"ledger_id":{"type":"integer"},"effective_on":{"type":"string"},"credit_limit_cents":{"type":"integer"},"od_limit_cents":{"type":"integer"},"apr_purchase_bps":{"type":"integer"},"apr_cash_bps":{"type":"integer"},"annual_fee_cents":{"type":"integer"},"statement_close_day":{"type":"integer"},"payment_due_offset_days":{"type":"integer"},"summary":{"type":"string"}},"additionalProperties":true}}',
                '{"title":"Upsert ledger terms","idempotentHint":true}',
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
                'UpsertContact',
                1,
                NULL, NULL, NULL, NULL,
                1,
                [==[
-- Argent.UpsertContact
-- Insert when contact_id is omitted. Update when it is present.
-- role_a2005 is lookup 2005. Address is stored in collection.

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

local function nonempty(v)
    if type(v) ~= "string" or v == "" then return nil end
    return v
end

local function actor_id()
    local h = {}
    if type(params) == "table" and type(params._hydrogen) == "table" then
        h = params._hydrogen
    end
    return tonumber(h.user_id) or tonumber(h.sub) or 0
end

local function json_quote(s)
    local out = {}
    for i = 1, #s do
        local c = s:sub(i, i)
        local b = string.byte(c)
        if c == '"' then
            out[#out + 1] = '\\"'
        elseif c == "\\" then
            out[#out + 1] = "\\\\"
        elseif b < 32 then
            out[#out + 1] = string.format("\\u%04x", b)
        else
            out[#out + 1] = c
        end
    end
    return '"' .. table.concat(out) .. '"'
end

if type(params) ~= "table" then
    return fail("params_required", "params must be an object")
end

local actor = actor_id()
local id = int(params.contact_id)
local ledger_id = int(params.ledger_id)
local role = int(params.role_a2005)
local name = nonempty(params.name)
local email = nonempty(params.email) or ""
local phone = nonempty(params.phone) or ""
local summary = nonempty(params.summary) or ""
local address = params.address
if address ~= nil and type(address) ~= "string" then
    return fail("address", "address must be a string")
end

local collection = "{}"
if id then
    local res, err = H.query_sync([[
        SELECT contact_id, ledger_id, role_a2005, name, email, phone,
               summary, collection
        FROM ${SCHEMA}contacts
        WHERE contact_id = :CONTACT_ID
    ]], { CONTACT_ID = id })
    if err then return fail("query_failed", tostring(err)) end
    local rows = qrows(res)
    if not rows[1] then return fail("not_found", "contact not found") end
    local row = rows[1]
    if not ledger_id then ledger_id = int(pick(row, "ledger_id")) end
    if params.role_a2005 == nil then role = int(pick(row, "role_a2005")) end
    if not name then name = pick(row, "name") end
    if params.email == nil then email = pick(row, "email") or "" end
    if params.phone == nil then phone = pick(row, "phone") or "" end
    if params.summary == nil then summary = pick(row, "summary") or "" end
    local stored = pick(row, "collection")
    if type(stored) == "string" and stored ~= "" then collection = stored end
end

if address ~= nil then
    collection = '{"address":' .. json_quote(address) .. "}"
end

if not ledger_id then return fail("ledger_id_required", "ledger_id is required") end
if not name then return fail("name_required", "name is required") end
if not role then role = 4 end
if role < 1 or role > 4 then
    return fail("role", "role_a2005 must be 1 through 4")
end

local lres, lerr = H.query_sync([[
    SELECT ledger_id FROM ${SCHEMA}ledgers WHERE ledger_id = :LEDGER_ID
]], { LEDGER_ID = ledger_id })
if lerr then return fail("query_failed", tostring(lerr)) end
if not qrows(lres)[1] then return fail("not_found", "ledger not found") end

if id then
    local _, uerr = H.query_sync([[
        UPDATE ${SCHEMA}contacts
        SET ledger_id = :LEDGER_ID,
            role_a2005 = :ROLE_A2005,
            name = :CONTACT_NAME,
            email = NULLIF(:CONTACT_EMAIL, ''),
            phone = NULLIF(:CONTACT_PHONE, ''),
            summary = NULLIF(:CONTACT_SUMMARY, ''),
            collection = ${JIS}:CONTACT_COLLECTION${JIE},
            updated_id = :ACTOR_UPDATED,
            updated_at = ${NOW}
        WHERE contact_id = :CONTACT_ID
    ]], {
        LEDGER_ID = ledger_id, ROLE_A2005 = role, CONTACT_NAME = name,
        CONTACT_EMAIL = email, CONTACT_PHONE = phone,
        CONTACT_SUMMARY = summary, CONTACT_COLLECTION = collection,
        ACTOR_UPDATED = actor, CONTACT_ID = id,
    })
    if uerr then return fail("update_failed", tostring(uerr)) end
    return ok({ contact_id = id, updated = true, created = false })
end

local nres, nerr = H.query_sync([[
    SELECT COALESCE(MAX(contact_id), 0) + 1 AS next_id
    FROM ${SCHEMA}contacts
]], {})
if nerr then return fail("query_failed", tostring(nerr)) end
local new_id = tonumber(pick(qrows(nres)[1], "next_id")) or 1

local _, ierr = H.query_sync([[
    INSERT INTO ${SCHEMA}contacts (
        contact_id, ledger_id, role_a2005, name, email, phone,
        summary, collection,
        valid_after, valid_until, created_id, created_at, updated_id, updated_at
    ) VALUES (
        :CONTACT_ID, :LEDGER_ID, :ROLE_A2005, :CONTACT_NAME,
        NULLIF(:CONTACT_EMAIL, ''), NULLIF(:CONTACT_PHONE, ''),
        NULLIF(:CONTACT_SUMMARY, ''), ${JIS}:CONTACT_COLLECTION${JIE},
        NULL, NULL, :ACTOR_CREATED, ${NOW}, :ACTOR_UPDATED, ${NOW}
    )
]], {
    CONTACT_ID = new_id, LEDGER_ID = ledger_id, ROLE_A2005 = role,
    CONTACT_NAME = name, CONTACT_EMAIL = email, CONTACT_PHONE = phone,
    CONTACT_SUMMARY = summary, CONTACT_COLLECTION = collection,
    ACTOR_CREATED = actor, ACTOR_UPDATED = actor,
})
if ierr then return fail("insert_failed", tostring(ierr)) end
return ok({ contact_id = new_id, created = true, updated = false })
                ]==],
                'Create or update an Argent ledger contact',
                0,
                1,
                '{"inputSchema":{"type":"object","properties":{"contact_id":{"type":"integer"},"ledger_id":{"type":"integer"},"role_a2005":{"type":"integer"},"name":{"type":"string"},"email":{"type":"string"},"phone":{"type":"string"},"address":{"type":"string"},"summary":{"type":"string"}},"additionalProperties":true}}',
                '{"title":"Upsert contact","idempotentHint":true}',
                ${COMMON_VALUES}
            );
            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Seed Argent terms and contact tools'                                                    AS name,
        [=[
            # Forward Migration ${MIGRATION}: Argent ledger terms and contacts

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
              AND script_name IN ('UpsertLedgerTerms', 'UpsertContact');

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Remove Argent terms and contact tools'                                                    AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Remove Argent ledger terms and contacts

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
        'Diagram Argent ledger terms and contacts'                                                   AS name,
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
                        "object_id": "script.Argent.UpsertLedgerTerms",
                        "object_ref": "${MIGRATION}",
                        "name": "Argent.UpsertLedgerTerms"
                    },
                    {
                        "object_type": "script",
                        "object_id": "script.Argent.UpsertContact",
                        "object_ref": "${MIGRATION}",
                        "name": "Argent.UpsertContact"
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
