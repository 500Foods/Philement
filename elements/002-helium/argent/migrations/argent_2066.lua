-- Migration: argent_2066.lua
-- Argent.Search
--
-- luacheck: no max line length
-- luacheck: no unused args
--
-- CHANGELOG
-- 1.0.0 - 2026-10-08 - Search names, descriptions, and file text
-- 1.0.1 - 2026-10-08 - MariaDB casts and same-type text compares

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "scripts"
cfg.MIGRATION = "2066"
cfg.GROUP_NAME = "Argent"
if engine == "mysql" or engine == "mariadb" then
    cfg.CAST_INTEGER = "signed"
    cfg.CAST_TEXT = "char(255)"
    cfg.CAST_BIG = "signed"
    cfg.NULL_BIG = "CAST(NULL AS signed)"
    cfg.CURRENCY_TYPE = "char(20)"
else
    cfg.CAST_INTEGER = cfg.INTEGER
    cfg.CAST_TEXT = cfg.TEXT
    cfg.CAST_BIG = cfg.INTEGER_BIG
    cfg.NULL_BIG = "CAST(NULL AS " .. cfg.INTEGER_BIG .. ")"
    cfg.CURRENCY_TYPE = cfg.VARCHAR_20
end
cfg.NULL_CURRENCY = "CAST(NULL AS " .. cfg.CURRENCY_TYPE .. ")"
if engine == "firebird" then
    cfg.AMOUNT_SUM = "CAST(COALESCE(SUM(ln.amount_cents), 0) AS BIGINT)"
    cfg.TAX_SUM = "CAST(COALESCE(SUM(ln.tax_cents), 0) AS BIGINT)"
    cfg.NET_SUM = "CAST(COALESCE(SUM(ln.amount_cents), 0) AS BIGINT)"
    cfg.RUN_SUM = "CAST(SUM(m.amount_cents) OVER (ORDER BY m.txn_on, m.txn_id, m.line_seq ROWS UNBOUNDED PRECEDING) AS BIGINT)"
else
    cfg.AMOUNT_SUM = "COALESCE(SUM(ln.amount_cents), 0)"
    cfg.TAX_SUM = "COALESCE(SUM(ln.tax_cents), 0)"
    cfg.NET_SUM = "COALESCE(SUM(ln.amount_cents), 0)"
    cfg.RUN_SUM = "SUM(m.amount_cents) OVER (ORDER BY m.txn_on, m.txn_id, m.line_seq ROWS UNBOUNDED PRECEDING)"
end
if engine == "postgresql" or engine == "firebird" then
    cfg.FILE_TEXT = "LOWER(SUBSTRING(a.file_text FROM 1 FOR 240))"
elseif engine == "db2" then
    cfg.FILE_TEXT = "LOWER(SUBSTR(a.file_text, 1, 240))"
else
    cfg.FILE_TEXT = "LOWER(SUBSTRING(a.file_text, 1, 240))"
end

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
                'Search',
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

local function day_of(v)
    if type(v) ~= "string" then return v end
    if #v >= 10 and v:sub(1, 10):match("^%d%d%d%d%-%d%d%-%d%d$") then
        return v:sub(1, 10)
    end
    return v
end

local function actor_id()
    local h = {}
    if type(params) == "table" and type(params._hydrogen) == "table" then
        h = params._hydrogen
    end
    return tonumber(h.user_id) or tonumber(h.sub) or 0
end

local DATE_FIELDS = {
    on_date = true, end_on = true, txn_on = true, as_of = true,
    reconciled_on = true, latest_reconciled_on = true, rate_as_of = true,
}

local function map_rows(rows, fields)
    local out = {}
    for i = 1, #rows do
        local item = {}
        for f = 1, #fields do
            local name = fields[f]
            local value = pick(rows[i], name)
            if DATE_FIELDS[name] then value = day_of(value) end
            item[name] = value
        end
        out[i] = item
    end
    return out
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

local function org_row(org)
    local res, err = H.query_sync([[
        SELECT organization_id, default_currency
        FROM ${SCHEMA}organizations
        WHERE organization_id = :ORG_ID
    ]], { ORG_ID = org })
    if err then return nil, err end
    local row = qrows(res)[1]
    if not row then return nil, "missing" end
    return row, nil
end

local function ledger_exists(ledger_id)
    local res, err = H.query_sync([[
        SELECT ledger_id FROM ${SCHEMA}ledgers WHERE ledger_id = :LEDGER_ID
    ]], { LEDGER_ID = ledger_id })
    if err then return nil, err end
    if not qrows(res)[1] then return nil, "missing" end
    return true, nil
end

local ALLOWED = {
    organization = true, ledger = true, transaction = true,
    schedule = true, contact = true, tag = true, attachment = true,
}

if type(params) ~= "table" then
    return fail("params_required", "params must be an object")
end
local q = params.q
if type(q) ~= "string" or #q < 1 or #q > 80 then
    return fail("q", "q must be 1 to 80 characters")
end
if q:find("[%z\1-\31]") or q:find("%%") or q:find("_", 1, true) or q:find("\\", 1, true) then
    return fail("q", "q must not contain wildcards or controls")
end
local slots = { "", "", "", "", "", "", "" }
local use_types = 1
if type(params.types) == "table" and #params.types > 0 then
    if #params.types > 7 then return fail("types", "types accepts at most 7 names") end
    use_types = 0
    for i = 1, #params.types do
        local name = params.types[i]
        if type(name) ~= "string" or not ALLOWED[name] then
            return fail("types", "types must name a known entity")
        end
        slots[i] = name
    end
end
local res, err = run_ref(2010, {
    Q = "%" .. q .. "%",
    USE_TYPES = use_types,
    TYPE_1 = slots[1], TYPE_2 = slots[2], TYPE_3 = slots[3], TYPE_4 = slots[4],
    TYPE_5 = slots[5], TYPE_6 = slots[6], TYPE_7 = slots[7],
})
if err then return fail("query_failed", tostring(err)) end
local rows = map_rows(qrows(res), { "entity_type", "entity_id", "label", "organization_id" })
local truncated = false
if #rows > 200 then
    truncated = true
    local cut = {}
    for i = 1, 200 do cut[i] = rows[i] end
    rows = cut
end
return ok({ q = q, truncated = truncated, rows = rows })

]==],
                'Search names, descriptions, and file text',
                0,
                1,
                '{"inputSchema":{"type":"object","properties":{"q":{"type":"string"},"types":{"type":"array","items":{"type":"string"}}},"required":["q"],"additionalProperties":true}}',
                '{"title":"Search","readOnlyHint":true}',
                ${COMMON_VALUES}
            );

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Seed Argent Search'                                                    AS name,
        [=[
            # Forward Migration ${MIGRATION}: Argent.Search

            Inserts the Argent script row named in the reverse migration.
            Group `Argent`. `mcp_access=1`, `invokable=0`.
            Does not install a QueryRef. Does not create a table.
            Does not add a column.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]====]})
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
              AND script_name IN ('Search');

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Remove Argent Search'                                                  AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Remove Argent.Search

            Deletes that script row. Does not drop Argent tables.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]====]})
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
        'Diagram Argent.Search'                                                  AS name,
        [=[
            # Diagram Migration ${MIGRATION}

            Argent.Search is a script row. This does not define a table.
        ]=]
                                                                            AS summary,
                                                                            -- DIAGRAM_START
        ${JSON_INGEST_START}
        [=[
            {
                "diagram": [
                    {
                        "object_type": "script",
                        "object_id": "script.Search",
                        "object_ref": "${MIGRATION}",
                        "name": "Search"
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
