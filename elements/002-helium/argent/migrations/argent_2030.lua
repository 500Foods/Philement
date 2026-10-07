-- Migration: argent_2030.lua
-- Argent.ListOrganizations and Argent.UpsertOrganization
--
-- luacheck: no max line length
-- luacheck: no unused args
--
-- CHANGELOG
-- 1.0.0 - 2026-10-07 - MCP organization list and upsert

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "scripts"
cfg.MIGRATION = "2030"
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
                'ListOrganizations',
                1,
                NULL, NULL, NULL, NULL,
                1,
                [==[
-- Argent.ListOrganizations
-- Lists organizations. Actor is params._hydrogen.

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

local res, err = H.query_sync([[
    SELECT organization_id, status_a2000, name,
           fiscal_year_start_month, fiscal_year_start_day,
           default_currency, summary
    FROM ${SCHEMA}organizations
    ORDER BY organization_id
]], {})
if err then return fail("query_failed", tostring(err)) end
local rows = qrows(res)
local out = {}
for i = 1, #rows do
    local row = rows[i]
    out[i] = {
        organization_id = pick(row, "organization_id"),
        status_a2000 = pick(row, "status_a2000"),
        name = pick(row, "name"),
        fiscal_year_start_month = pick(row, "fiscal_year_start_month"),
        fiscal_year_start_day = pick(row, "fiscal_year_start_day"),
        default_currency = pick(row, "default_currency"),
        summary = pick(row, "summary"),
    }
end
return ok({ organizations = out })
                ]==],
                'List Argent organizations',
                0,
                1,
                '{"inputSchema":{"type":"object","properties":{},"additionalProperties":true}}',
                '{"title":"List organizations","readOnlyHint":true}',
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
                'UpsertOrganization',
                1,
                NULL, NULL, NULL, NULL,
                1,
                [==[
-- Argent.UpsertOrganization
-- Insert when organization_id is omitted. Update when it is present.
-- fiscal month is 1-12 and fiscal day is 1-31. Currency is a lowercase
-- ISO code that already exists in currencies. status_a2000 defaults to 1.

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
local id = num(params.organization_id)
local name = nonempty(params.name)
local month = num(params.fiscal_year_start_month)
local day = num(params.fiscal_year_start_day)
local currency = nonempty(params.default_currency)
if currency then currency = string.lower(currency) end
local status = num(params.status_a2000) or 1
local summary = nonempty(params.summary) or ""

if status ~= 1 and status ~= 2 then
    return fail("status", "status_a2000 must be 1 or 2")
end

local function currency_ok(code)
    if not code or not code:match("^[a-z][a-z][a-z]$") then
        return false
    end
    local res, err = H.query_sync([[
        SELECT currency_code
        FROM ${SCHEMA}currencies
        WHERE currency_code = :CUR_CODE
    ]], { CUR_CODE = code })
    if err then return nil, err end
    local rows = qrows(res)
    return rows[1] ~= nil, nil
end

if id then
    local res, err = H.query_sync([[
        SELECT organization_id, status_a2000, name,
               fiscal_year_start_month, fiscal_year_start_day,
               default_currency, summary
        FROM ${SCHEMA}organizations
        WHERE organization_id = :ORG_ID
    ]], { ORG_ID = id })
    if err then return fail("query_failed", tostring(err)) end
    local rows = qrows(res)
    if not rows[1] then return fail("not_found", "organization not found") end
    local row = rows[1]
    if not name then name = pick(row, "name") end
    if not month then month = tonumber(pick(row, "fiscal_year_start_month")) end
    if not day then day = tonumber(pick(row, "fiscal_year_start_day")) end
    if not currency then currency = pick(row, "default_currency") end
    if params.status_a2000 == nil then status = tonumber(pick(row, "status_a2000")) or 1 end
    if params.summary == nil then summary = pick(row, "summary") or "" end
end

if not name then return fail("name_required", "name is required") end
if not month or month < 1 or month > 12 or month % 1 ~= 0 then
    return fail("fiscal_month", "fiscal_year_start_month must be 1 through 12")
end
if not day or day < 1 or day > 31 or day % 1 ~= 0 then
    return fail("fiscal_day", "fiscal_year_start_day must be 1 through 31")
end
local known, cerr = currency_ok(currency)
if cerr then return fail("query_failed", tostring(cerr)) end
if not known then return fail("currency", "default_currency is not in currencies") end

if id then
    local _, uerr = H.query_sync([[
        UPDATE ${SCHEMA}organizations
        SET status_a2000 = :STATUS_A2000,
            name = :ORG_NAME,
            fiscal_year_start_month = :FY_MONTH,
            fiscal_year_start_day = :FY_DAY,
            default_currency = :CUR_CODE,
            summary = NULLIF(:ORG_SUMMARY, ''),
            updated_id = :ACTOR_UPDATED,
            updated_at = ${NOW}
        WHERE organization_id = :ORG_ID
    ]], {
        STATUS_A2000 = status,
        ORG_NAME = name,
        FY_MONTH = month,
        FY_DAY = day,
        CUR_CODE = currency,
        ORG_SUMMARY = summary,
        ACTOR_UPDATED = actor,
        ORG_ID = id,
    })
    if uerr then return fail("update_failed", tostring(uerr)) end
    return ok({ organization_id = id, updated = true })
end

local id_res, id_err = H.query_sync([[
    SELECT COALESCE(MAX(organization_id), 0) + 1 AS next_id
    FROM ${SCHEMA}organizations
]], {})
if id_err then return fail("query_failed", tostring(id_err)) end
local id_rows = qrows(id_res)
local new_id = tonumber(pick(id_rows[1], "next_id")) or 1

local _, ierr = H.query_sync([[
    INSERT INTO ${SCHEMA}organizations (
        organization_id, status_a2000, name,
        fiscal_year_start_month, fiscal_year_start_day,
        default_currency, summary, collection,
        valid_after, valid_until, created_id, created_at, updated_id, updated_at
    ) VALUES (
        :ORG_ID, :STATUS_A2000, :ORG_NAME,
        :FY_MONTH, :FY_DAY,
        :CUR_CODE, NULLIF(:ORG_SUMMARY, ''), ${JIS}:ORG_COLLECTION${JIE},
        NULL, NULL, :ACTOR_CREATED, ${NOW}, :ACTOR_UPDATED, ${NOW}
    )
]], {
    ORG_ID = new_id,
    STATUS_A2000 = status,
    ORG_NAME = name,
    FY_MONTH = month,
    FY_DAY = day,
    CUR_CODE = currency,
    ORG_SUMMARY = summary,
    ORG_COLLECTION = "{}",
    ACTOR_CREATED = actor,
    ACTOR_UPDATED = actor,
})
if ierr then return fail("insert_failed", tostring(ierr)) end
return ok({ organization_id = new_id, created = true })
                ]==],
                'Create or update an Argent organization',
                0,
                1,
                '{"inputSchema":{"type":"object","properties":{"organization_id":{"type":"integer"},"name":{"type":"string"},"fiscal_year_start_month":{"type":"integer"},"fiscal_year_start_day":{"type":"integer"},"default_currency":{"type":"string"},"status_a2000":{"type":"integer"},"summary":{"type":"string"}},"additionalProperties":true}}',
                '{"title":"Upsert organization","idempotentHint":true}',
                ${COMMON_VALUES}
            );

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Seed Argent organization tools'                                    AS name,
        [=[
            # Forward Migration ${MIGRATION}: Argent organization tools

            Inserts `Argent.ListOrganizations` and `Argent.UpsertOrganization`
            into `scripts`. Group `Argent`. `mcp_access=1`, `invokable=0`.
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
              AND script_name IN ('ListOrganizations', 'UpsertOrganization');

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Remove Argent organization tools'                                  AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Remove Argent organization tools

            Deletes the two script rows. Does not drop organizations.
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
        'Diagram Argent organization tools'                                 AS name,
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
                        "object_id": "script.Argent.ListOrganizations",
                        "object_ref": "${MIGRATION}",
                        "name": "Argent.ListOrganizations"
                    },
                    {
                        "object_type": "script",
                        "object_id": "script.Argent.UpsertOrganization",
                        "object_ref": "${MIGRATION}",
                        "name": "Argent.UpsertOrganization"
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
