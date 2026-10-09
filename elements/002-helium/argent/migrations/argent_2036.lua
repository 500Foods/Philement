-- Migration: argent_2036.lua
-- Argent tax writes
--
-- luacheck: no max line length
-- luacheck: no unused args
--
-- CHANGELOG
-- 1.0.0 - 2026-10-07 - MCP tax code and tax rate upserts
-- 1.0.1 - 2026-10-07 - Cast optional NULL, dates, and empty strings
-- 1.0.2 - 2026-10-07 - MySQL CAST targets; json parameters are cast before ingest

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "scripts"
cfg.MIGRATION = "2036"
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
                'UpsertTaxCode',
                1,
                NULL, NULL, NULL, NULL,
                1,
                [==[
-- Argent.UpsertTaxCode
-- Insert when tax_code_id is omitted. The target ledger must post
-- and must belong to the same organization.

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

if type(params) ~= "table" then
    return fail("params_required", "params must be an object")
end

local actor = actor_id()
local id = int(params.tax_code_id)
local org = int(params.organization_id)
local code = nonempty(params.code)
local name = nonempty(params.name)
local target = int(params.target_ledger_id)
local summary = nonempty(params.summary) or ""

if id then
    local res, err = H.query_sync([[
        SELECT tax_code_id, organization_id, code, name, target_ledger_id, summary
        FROM ${SCHEMA}tax_codes
        WHERE tax_code_id = :TAX_CODE_ID
    ]], { TAX_CODE_ID = id })
    if err then return fail("query_failed", tostring(err)) end
    local row = qrows(res)[1]
    if not row then return fail("not_found", "tax code not found") end
    if params.organization_id == nil then org = int(pick(row, "organization_id")) end
    if not code then code = pick(row, "code") end
    if not name then name = pick(row, "name") end
    if params.target_ledger_id == nil then target = int(pick(row, "target_ledger_id")) end
    if params.summary == nil then summary = pick(row, "summary") or "" end
end

if not org then return fail("organization_id_required", "organization_id is required") end
if not code then return fail("code_required", "code is required") end
if #code > 50 then return fail("code", "code is longer than 50 characters") end
if not name then return fail("name_required", "name is required") end
if not target then return fail("target_required", "target_ledger_id is required") end

local ores, oerr = H.query_sync([[
    SELECT organization_id FROM ${SCHEMA}organizations
    WHERE organization_id = :ORG_ID
]], { ORG_ID = org })
if oerr then return fail("query_failed", tostring(oerr)) end
if not qrows(ores)[1] then return fail("not_found", "organization not found") end

local lres, lerr = H.query_sync([[
    SELECT ledger_id, organization_id, is_posting
    FROM ${SCHEMA}ledgers
    WHERE ledger_id = :LEDGER_ID
]], { LEDGER_ID = target })
if lerr then return fail("query_failed", tostring(lerr)) end
local ledger = qrows(lres)[1]
if not ledger then return fail("not_found", "target ledger not found") end
if tonumber(pick(ledger, "organization_id")) ~= org then
    return fail("target", "target ledger is in another organization")
end
if tonumber(pick(ledger, "is_posting")) ~= 1 then
    return fail("target", "target ledger must be posting")
end

if id then
    local _, uerr = H.query_sync([[
        UPDATE ${SCHEMA}tax_codes
        SET organization_id = :ORG_ID,
            code = :TAX_CODE,
            name = :TAX_NAME,
            target_ledger_id = :LEDGER_ID,
            summary = NULLIF(CAST(:TAX_SUMMARY AS ${CAST_TEXT}), ''),
            updated_id = :ACTOR_UPDATED,
            updated_at = ${NOW}
        WHERE tax_code_id = :TAX_CODE_ID
    ]], {
        ORG_ID = org, TAX_CODE = code, TAX_NAME = name, LEDGER_ID = target,
        TAX_SUMMARY = summary, ACTOR_UPDATED = actor, TAX_CODE_ID = id,
    })
    if uerr then return fail("update_failed", tostring(uerr)) end
    return ok({ tax_code_id = id, updated = true, created = false })
end

local nres, nerr = H.query_sync([[
    SELECT COALESCE(MAX(tax_code_id), 0) + 1 AS next_id FROM ${SCHEMA}tax_codes
]], {})
if nerr then return fail("query_failed", tostring(nerr)) end
local new_id = tonumber(pick(qrows(nres)[1], "next_id")) or 1

local _, ierr = H.query_sync([[
    INSERT INTO ${SCHEMA}tax_codes (
        tax_code_id, organization_id, code, name, target_ledger_id,
        summary, collection,
        valid_after, valid_until, created_id, created_at, updated_id, updated_at
    ) VALUES (
        :TAX_CODE_ID, :ORG_ID, :TAX_CODE, :TAX_NAME, :LEDGER_ID,
        NULLIF(CAST(:TAX_SUMMARY AS ${CAST_TEXT}), ''), ${JIS}CAST(:TAX_COLLECTION AS ${CAST_TEXT})${JIE},
        NULL, NULL, :ACTOR_CREATED, ${NOW}, :ACTOR_UPDATED, ${NOW}
    )
]], {
    TAX_CODE_ID = new_id, ORG_ID = org, TAX_CODE = code, TAX_NAME = name,
    LEDGER_ID = target, TAX_SUMMARY = summary, TAX_COLLECTION = "{}",
    ACTOR_CREATED = actor, ACTOR_UPDATED = actor,
})
if ierr then return fail("insert_failed", tostring(ierr)) end
return ok({ tax_code_id = new_id, created = true, updated = false })
                ]==],
                'Create or update an Argent tax code',
                0,
                1,
                '{"inputSchema":{"type":"object","properties":{"tax_code_id":{"type":"integer"},"organization_id":{"type":"integer"},"code":{"type":"string"},"name":{"type":"string"},"target_ledger_id":{"type":"integer"},"summary":{"type":"string"}},"additionalProperties":true}}',
                '{"title":"Upsert tax code","idempotentHint":true}',
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
                'UpsertTaxRate',
                1,
                NULL, NULL, NULL, NULL,
                1,
                [==[
-- Argent.UpsertTaxRate
-- Updates tax_rate_id when it is present. Otherwise updates the first
-- row for that code and effective_on, or inserts one.
-- rate_bps 500 means 5.00 percent. Any integer is stored.

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

if type(params) ~= "table" then
    return fail("params_required", "params must be an object")
end

local actor = actor_id()
local id = int(params.tax_rate_id)
local tax_code_id = int(params.tax_code_id)
local effective_on = nonempty(params.effective_on)
local rate_bps = int(params.rate_bps)
local summary = nonempty(params.summary) or ""

if id then
    local res, err = H.query_sync([[
        SELECT tax_rate_id, tax_code_id, effective_on, rate_bps, summary
        FROM ${SCHEMA}tax_rates
        WHERE tax_rate_id = :TAX_RATE_ID
    ]], { TAX_RATE_ID = id })
    if err then return fail("query_failed", tostring(err)) end
    local row = qrows(res)[1]
    if not row then return fail("not_found", "tax rate not found") end
    if params.tax_code_id == nil then tax_code_id = int(pick(row, "tax_code_id")) end
    if not effective_on then effective_on = pick(row, "effective_on") end
    if params.rate_bps == nil then rate_bps = int(pick(row, "rate_bps")) end
    if params.summary == nil then summary = pick(row, "summary") or "" end
end

if not tax_code_id then return fail("tax_code_id_required", "tax_code_id is required") end
if not date_ok(effective_on) then
    return fail("effective_on", "effective_on must be YYYY-MM-DD")
end
if rate_bps == nil then return fail("rate_bps_required", "rate_bps is required") end

local cres, cerr = H.query_sync([[
    SELECT tax_code_id FROM ${SCHEMA}tax_codes WHERE tax_code_id = :TAX_CODE_ID
]], { TAX_CODE_ID = tax_code_id })
if cerr then return fail("query_failed", tostring(cerr)) end
if not qrows(cres)[1] then return fail("not_found", "tax code not found") end

local pres, perr = H.query_sync([[
    SELECT tax_rate_id, summary
    FROM ${SCHEMA}tax_rates
    WHERE tax_code_id = :TAX_CODE_ID AND effective_on = CAST(:EFFECTIVE_ON AS ${DATE})
    ORDER BY tax_rate_id
]], { TAX_CODE_ID = tax_code_id, EFFECTIVE_ON = effective_on })
if perr then return fail("query_failed", tostring(perr)) end
local pair = qrows(pres)[1]
local pair_id = pair and int(pick(pair, "tax_rate_id")) or nil
if id and pair_id and pair_id ~= id then
    return fail("duplicate", "a tax rate already exists for that date")
end
if not id and pair then
    id = pair_id
    if params.summary == nil then summary = pick(pair, "summary") or "" end
end

if id then
    local _, uerr = H.query_sync([[
        UPDATE ${SCHEMA}tax_rates
        SET tax_code_id = :TAX_CODE_ID,
            effective_on = CAST(:EFFECTIVE_ON AS ${DATE}),
            rate_bps = :RATE_BPS,
            summary = NULLIF(CAST(:RATE_SUMMARY AS ${CAST_TEXT}), ''),
            updated_id = :ACTOR_UPDATED,
            updated_at = ${NOW}
        WHERE tax_rate_id = :TAX_RATE_ID
    ]], {
        TAX_CODE_ID = tax_code_id, EFFECTIVE_ON = effective_on,
        RATE_BPS = rate_bps, RATE_SUMMARY = summary,
        ACTOR_UPDATED = actor, TAX_RATE_ID = id,
    })
    if uerr then return fail("update_failed", tostring(uerr)) end
    return ok({ tax_rate_id = id, updated = true, created = false })
end

local nres, nerr = H.query_sync([[
    SELECT COALESCE(MAX(tax_rate_id), 0) + 1 AS next_id FROM ${SCHEMA}tax_rates
]], {})
if nerr then return fail("query_failed", tostring(nerr)) end
local new_id = tonumber(pick(qrows(nres)[1], "next_id")) or 1

local _, ierr = H.query_sync([[
    INSERT INTO ${SCHEMA}tax_rates (
        tax_rate_id, tax_code_id, effective_on, rate_bps, summary, collection,
        valid_after, valid_until, created_id, created_at, updated_id, updated_at
    ) VALUES (
        :TAX_RATE_ID, :TAX_CODE_ID, CAST(:EFFECTIVE_ON AS ${DATE}), :RATE_BPS,
        NULLIF(CAST(:RATE_SUMMARY AS ${CAST_TEXT}), ''), ${JIS}CAST(:RATE_COLLECTION AS ${CAST_TEXT})${JIE},
        NULL, NULL, :ACTOR_CREATED, ${NOW}, :ACTOR_UPDATED, ${NOW}
    )
]], {
    TAX_RATE_ID = new_id, TAX_CODE_ID = tax_code_id, EFFECTIVE_ON = effective_on,
    RATE_BPS = rate_bps, RATE_SUMMARY = summary, RATE_COLLECTION = "{}",
    ACTOR_CREATED = actor, ACTOR_UPDATED = actor,
})
if ierr then return fail("insert_failed", tostring(ierr)) end
return ok({ tax_rate_id = new_id, created = true, updated = false })
                ]==],
                'Create or update an Argent tax rate',
                0,
                1,
                '{"inputSchema":{"type":"object","properties":{"tax_rate_id":{"type":"integer"},"tax_code_id":{"type":"integer"},"effective_on":{"type":"string"},"rate_bps":{"type":"integer"},"summary":{"type":"string"}},"additionalProperties":true}}',
                '{"title":"Upsert tax rate","idempotentHint":true}',
                ${COMMON_VALUES}
            );
            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Seed Argent tax write tools'                                                    AS name,
        [=[
            # Forward Migration ${MIGRATION}: Argent tax writes

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
              AND script_name IN ('UpsertTaxCode', 'UpsertTaxRate');

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Remove Argent tax write tools'                                                    AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Remove Argent tax writes

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
        'Diagram Argent tax writes'                                                   AS name,
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
                        "object_id": "script.Argent.UpsertTaxCode",
                        "object_ref": "${MIGRATION}",
                        "name": "Argent.UpsertTaxCode"
                    },
                    {
                        "object_type": "script",
                        "object_id": "script.Argent.UpsertTaxRate",
                        "object_ref": "${MIGRATION}",
                        "name": "Argent.UpsertTaxRate"
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
