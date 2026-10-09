-- Migration: argent_2069.lua
-- Argent.GetBocRate
--
-- luacheck: no max line length
-- luacheck: no unused args
--
-- CHANGELOG
-- 1.0.0 - 2026-10-08 - Fetch and cache a Bank of Canada rate
-- 1.0.1 - 2026-10-08 - MariaDB casts and same-type text compares

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "scripts"
cfg.MIGRATION = "2069"
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
                'GetBocRate',
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

local function currency_code(v)
    if type(v) ~= "string" then return nil end
    local s = string.lower(v)
    if not s:match("^[a-z][a-z][a-z]$") then return nil end
    return s
end

local function currency_known(code)
    local res, err = H.query_sync([[
        SELECT currency_code FROM ${SCHEMA}currencies WHERE currency_code = :CODE
    ]], { CODE = code })
    if err then return nil, err end
    if not qrows(res)[1] then return nil, "missing" end
    return true, nil
end

local function save_rate(base, quote, source, as_of, n, d, summary, collection, actor)
    local found, ferr = H.query_sync([[
        SELECT rate_id
        FROM ${SCHEMA}rates
        WHERE base_currency = :BASE
          AND quote_currency = :QUOTE
          AND source_a2012 = :SOURCE
          AND as_of = CAST(:AS_OF AS ${DATE})
    ]], { BASE = base, QUOTE = quote, SOURCE = source, AS_OF = as_of })
    if ferr then return nil, nil, ferr end
    local existing = int(pick(qrows(found)[1], "rate_id"))
    if existing then
        local _, uerr = H.query_sync([[
            UPDATE ${SCHEMA}rates
            SET rate_n = CAST(:RATE_N AS ${CAST_BIG}),
                rate_d = CAST(:RATE_D AS ${CAST_BIG}),
                summary = NULLIF(CAST(:SUMMARY AS ${CAST_TEXT}), ''),
                collection = ${JIS}CAST(:COLLECTION AS ${CAST_TEXT})${JIE},
                txn_id = NULL,
                updated_id = :ACTOR,
                updated_at = ${NOW}
            WHERE rate_id = :RATE_ID
        ]], {
            RATE_N = n, RATE_D = d, SUMMARY = summary, COLLECTION = collection,
            ACTOR = actor, RATE_ID = existing,
        })
        if uerr then return nil, nil, uerr end
        return existing, false, nil
    end
    local id_res, id_err = H.query_sync([[
        SELECT COALESCE(MAX(rate_id), 0) + 1 AS next_id FROM ${SCHEMA}rates
    ]], {})
    if id_err then return nil, nil, id_err end
    local new_id = int(pick(qrows(id_res)[1], "next_id")) or 1
    local _, ierr = H.query_sync([[
        INSERT INTO ${SCHEMA}rates (
            rate_id, base_currency, quote_currency, source_a2012, as_of,
            rate_n, rate_d, txn_id, summary, collection,
            valid_after, valid_until, created_id, created_at, updated_id, updated_at
        ) VALUES (
            :RATE_ID, :BASE, :QUOTE, :SOURCE, CAST(:AS_OF AS ${DATE}),
            CAST(:RATE_N AS ${CAST_BIG}), CAST(:RATE_D AS ${CAST_BIG}),
            NULL, NULLIF(CAST(:SUMMARY AS ${CAST_TEXT}), ''),
            ${JIS}CAST(:COLLECTION AS ${CAST_TEXT})${JIE},
            NULL, NULL, :ACTOR_CREATED, ${NOW}, :ACTOR_UPDATED, ${NOW}
        )
    ]], {
        RATE_ID = new_id, BASE = base, QUOTE = quote, SOURCE = source, AS_OF = as_of,
        RATE_N = n, RATE_D = d, SUMMARY = summary, COLLECTION = collection,
        ACTOR_CREATED = actor, ACTOR_UPDATED = actor,
    })
    if ierr then return nil, nil, ierr end
    return new_id, true, nil
end

local BOC_LIST = "aud,brl,cny,eur,hkd,inr,idr,jpy,myr,mxn,nzd,nok,pen,pln,rub,sar,sgd,zar,krw,sek,chf,twd,thb,try,gbp,usd,vnd"

local function boc_known(code)
    return ("," .. BOC_LIST .. ","):find("," .. code .. ",", 1, true) ~= nil
end

local function gcd(a, b)
    a = math.abs(math.floor(a))
    b = math.abs(math.floor(b))
    while b ~= 0 do a, b = b, a % b end
    if a == 0 then return 1 end
    return a
end

local function parse_rate(text)
    if type(text) ~= "string" then return nil end
    local whole, frac = text:match("^(%d+)%.(%d+)$")
    local n, d
    if whole then
        if #frac < 1 or #frac > 6 then return nil end
        n = tonumber(whole .. frac)
        d = math.floor(10 ^ #frac)
    else
        if not text:match("^%d+$") then return nil end
        n = tonumber(text)
        d = 1
    end
    if not n or n < 1 or d < 1 then return nil end
    n = math.floor(n)
    local g = gcd(n, d)
    return math.floor(n / g), math.floor(d / g)
end

local function observation(body, series, as_of)
    local needle = '"' .. as_of .. '"'
    local at = 1
    while true do
        local found = string.find(body, needle, at, true)
        if not found then return nil end
        local window = string.sub(body, found, found + 500)
        local mark = '"' .. series .. '"'
        local spot = string.find(window, mark, 1, true)
        if spot then
            local slice = string.sub(window, spot, spot + 120)
            local value = string.match(slice, '"v"%s*:%s*"(%d+%.%d+)"')
            if not value then value = string.match(slice, '"v"%s*:%s*"(%d+)"') end
            return value
        end
        at = found + #needle
    end
end

local function http_get(url)
    if type(H.http) ~= "table" or type(H.http.get) ~= "function" then
        return nil, "H.http.get is not available"
    end
    local ok_call, res, err = pcall(function()
        local handle = H.http.get(url, nil, { timeout = 10 })
        return H.wait(handle)
    end)
    if not ok_call then return nil, tostring(res) end
    if res == nil then return nil, tostring(err or "fetch failed") end
    if type(res) ~= "table" then return nil, "empty response" end
    local status = tonumber(res.status) or 0
    if status < 200 or status > 299 then
        return nil, "http " .. tostring(status)
    end
    if type(res.body) ~= "string" or res.body == "" then
        return nil, "empty body"
    end
    return res.body, nil
end

local function load_rate(base, quote, as_of)
    local res, err = H.query_sync([[
        SELECT rate_id, rate_n, rate_d
        FROM ${SCHEMA}rates
        WHERE base_currency = :BASE
          AND quote_currency = :QUOTE
          AND source_a2012 = :SOURCE
          AND as_of = CAST(:AS_OF AS ${DATE})
    ]], { BASE = base, QUOTE = quote, SOURCE = 1, AS_OF = as_of })
    if err then return nil, err end
    return qrows(res)[1], nil
end

local fetched = false

local function ensure_foreign(ccy, as_of, actor)
    local row, err = load_rate(ccy, "cad", as_of)
    if err then return nil, nil, "query_failed", tostring(err) end
    if row then
        local n = int(pick(row, "rate_n"))
        local d = int(pick(row, "rate_d"))
        if not n or not d or n < 1 or d < 1 then
            return nil, nil, "boc_parse", "stored boc rate is not usable"
        end
        return n, d, nil, nil
    end
    local series = "FX" .. string.upper(ccy) .. "CAD"
    local url = "https://www.bankofcanada.ca/valet/observations/" .. series
        .. "/json?start_date=" .. as_of .. "&end_date=" .. as_of
    local body, herr = http_get(url)
    if not body then return nil, nil, "boc_fetch", herr end
    local value = observation(body, series, as_of)
    if not value then return nil, nil, "boc_missing", "no Bank of Canada observation" end
    local n, d = parse_rate(value)
    if not n then return nil, nil, "boc_parse", "Bank of Canada value is not a rate" end
    local snippet = '{"series":"' .. series .. '","v":"' .. value .. '"}'
    local id, _, serr = save_rate(ccy, "cad", 1, as_of, n, d, "BoC " .. series, snippet, actor)
    if serr then return nil, nil, "save_failed", tostring(serr) end
    fetched = true
    return n, d, nil, id
end

if type(params) ~= "table" then
    return fail("params_required", "params must be an object")
end
local base = currency_code(params.base_currency)
local quote = currency_code(params.quote_currency)
if not base or not quote or base == quote then
    return fail("pair", "base_currency and quote_currency must be different 3-letter codes")
end
local foreign_ok = (base == "cad" or boc_known(base)) and (quote == "cad" or boc_known(quote))
if not foreign_ok or (base ~= "cad" and quote ~= "cad" and not (boc_known(base) and boc_known(quote))) then
    return fail("pair", "pair must be CAD and a Bank of Canada series, or two of those series")
end
local as_of = params.as_of
if not date_ok(as_of) then return fail("as_of", "as_of must be YYYY-MM-DD") end
local known, kerr = currency_known(base)
if kerr == "missing" then return fail("currency", "base_currency is not in currencies") end
if not known then return fail("query_failed", tostring(kerr)) end
known, kerr = currency_known(quote)
if kerr == "missing" then return fail("currency", "quote_currency is not in currencies") end
if not known then return fail("query_failed", tostring(kerr)) end

local actor = actor_id()
local existing, lerr = load_rate(base, quote, as_of)
if lerr then return fail("query_failed", tostring(lerr)) end
if existing then
    return ok({
        cached = true,
        rate_id = pick(existing, "rate_id"),
        base_currency = base,
        quote_currency = quote,
        source_a2012 = 1,
        as_of = as_of,
        rate_n = pick(existing, "rate_n"),
        rate_d = pick(existing, "rate_d"),
    })
end

local n, d, code, message, series
if quote == "cad" then
    n, d, code, message = ensure_foreign(base, as_of, actor)
    series = "FX" .. string.upper(base) .. "CAD"
elseif base == "cad" then
    local fn, fd
    fn, fd, code, message = ensure_foreign(quote, as_of, actor)
    if fn then
        n, d = fd, fn
        local g = gcd(n, d)
        n = math.floor(n / g)
        d = math.floor(d / g)
        series = "FX" .. string.upper(quote) .. "CAD"
    end
else
    local n1, d1, n2, d2
    n1, d1, code, message = ensure_foreign(base, as_of, actor)
    if n1 then
        n2, d2, code, message = ensure_foreign(quote, as_of, actor)
    end
    if n1 and n2 then
        n = n1 * d2
        d = d1 * n2
        if d < 1 or n < 1 then
            return fail("boc_parse", "cross rate is not usable")
        end
        local g = gcd(n, d)
        n = math.floor(n / g)
        d = math.floor(d / g)
        series = "FX" .. string.upper(base) .. "CAD/FX" .. string.upper(quote) .. "CAD"
    end
end
if not n then return fail(code or "boc_fetch", message or code or "boc_fetch") end

local again, aerr = load_rate(base, quote, as_of)
if aerr then return fail("query_failed", tostring(aerr)) end
if again then
    return ok({
        cached = not fetched,
        created = fetched,
        rate_id = pick(again, "rate_id"),
        base_currency = base,
        quote_currency = quote,
        source_a2012 = 1,
        as_of = as_of,
        rate_n = pick(again, "rate_n"),
        rate_d = pick(again, "rate_d"),
        series = series,
    })
end
local snippet = '{"series":"' .. series .. '"}'
if #snippet > 240 then snippet = "{}" end
local id, created, serr = save_rate(
    base, quote, 1, as_of, n, d, "BoC " .. series, snippet, actor
)
if serr then return fail("save_failed", tostring(serr)) end
return ok({
    cached = not fetched,
    created = created,
    rate_id = id,
    base_currency = base,
    quote_currency = quote,
    source_a2012 = 1,
    as_of = as_of,
    rate_n = n,
    rate_d = d,
    series = series,
})

]==],
                'Fetch and cache a Bank of Canada rate',
                0,
                1,
                '{"inputSchema":{"type":"object","properties":{"base_currency":{"type":"string"},"quote_currency":{"type":"string"},"as_of":{"type":"string"}},"required":["base_currency","quote_currency","as_of"],"additionalProperties":true}}',
                '{"title":"GetBocRate","readOnlyHint":false}',
                ${COMMON_VALUES}
            );

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Seed Argent GetBocRate'                                                    AS name,
        [=[
            # Forward Migration ${MIGRATION}: Argent.GetBocRate

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
              AND script_name IN ('GetBocRate');

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Remove Argent GetBocRate'                                                  AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Remove Argent.GetBocRate

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
        'Diagram Argent.GetBocRate'                                                  AS name,
        [=[
            # Diagram Migration ${MIGRATION}

            Argent.GetBocRate is a script row. This does not define a table.
        ]=]
                                                                            AS summary,
                                                                            -- DIAGRAM_START
        ${JSON_INGEST_START}
        [=[
            {
                "diagram": [
                    {
                        "object_type": "script",
                        "object_id": "script.GetBocRate",
                        "object_ref": "${MIGRATION}",
                        "name": "GetBocRate"
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
