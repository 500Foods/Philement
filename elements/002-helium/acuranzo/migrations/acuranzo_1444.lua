-- Migration: acuranzo_1444.lua
-- Reporting Phase 19: seed Reports.Render (compose skeleton)
--
-- luacheck: no max line length
-- luacheck: no unused args
--
-- CHANGELOG
-- 1.0.0 - 2026-11-03 - Reports.Render stage=compose; all band roles; multi-level groups

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "scripts"
cfg.MIGRATION = "1444"
cfg.GROUP_NAME = "Reports"
cfg.SCRIPT_NAME = "Render"
-- ----------------------------------------------------------------------------
-- Forward: seed Reports.Render
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
                '${SCRIPT_NAME}',
                1,
                NULL, NULL, NULL, NULL,
                1,
                [==[
-- Reports.Render (Reporting Phase 19: compose skeleton)
-- params: { stage = "compose", definition = {...}, sources = { name = [rows] } }
-- Returns the structural intermediate { pages = [ { bands = [...] } ], log = [...] }.
-- Bands: RH(12) PH(13) GH(14) Detail(15) GF(16) PF(17) RF(18), any count.
-- Groups nest in GH order (outermost first). Values print as-is (masks: Phase 20).

local P = params or {}
local log = {}

local function add_log(level, message)
    log[#log + 1] = { level = level, message = message }
end

local function fail(message)
    add_log("error", message)
    H.set_result_json({ error = message, log = log })
    return 0
end

local FIELD_KEYS = { [20] = true, [21] = true, [24] = true }
local LATER_KEYS = {
    [5] = true, [6] = true, [42] = true, [43] = true, [44] = true, [45] = true,
    [61] = true, [62] = true, [63] = true, [28] = true, [29] = true, [32] = true,
    [34] = true, [35] = true, [38] = true,
}

local function plain(v)
    if v == nil then return "" end
    if math.type(v) == "float" then
        if v == math.floor(v) and math.abs(v) < 1e15 then
            return string.format("%d", math.floor(v))
        end
        local s = string.format("%.15g", v)
        if tonumber(s) ~= v then s = string.format("%.17g", v) end
        return s
    end
    return tostring(v)
end

local function compose()
    local stage = P.stage or "compose"
    if stage ~= "compose" then
        return fail("stage " .. tostring(stage) .. " is not available yet; use compose")
    end
    local def = P.definition
    if type(def) ~= "table" then
        return fail("definition must be a JSON object")
    end
    if def.format ~= 1 then
        return fail("definition format " .. tostring(def.format) .. " is not supported; expected 1")
    end
    local sources = type(P.sources) == "table" and P.sources or {}
    local rfont = type(def.font) == "table" and def.font or {}

    local declared = {}
    for _, ds in ipairs(def.datasources or {}) do
        if type(ds) == "table" and ds.name then declared[ds.name] = ds end
    end

    -- Sort bands into roles, keeping designer order inside each role.
    local rh, ph, gh, details, gf, pf, rf = {}, {}, {}, {}, {}, {}, {}
    local roles = { [12] = rh, [13] = ph, [14] = gh, [15] = details, [16] = gf, [17] = pf, [18] = rf }
    for _, b in ipairs(def.bands or {}) do
        local list = type(b) == "table" and roles[b.typeKey]
        if list then
            list[#list + 1] = b
        else
            add_log("warn", "band " .. tostring(type(b) == "table" and b.id) .. " has unknown typeKey "
                .. tostring(type(b) == "table" and b.typeKey) .. "; skipped")
        end
    end

    -- Pair each group header with its footer: same suffix (gh2/gf2), else nesting order.
    local used = {}
    local function suffix(id) return tostring(id or ""):match("^g[hf](.*)$") end
    local levels = {}
    for i, h in ipairs(gh) do
        local foot
        local hs = suffix(h.id)
        if hs then
            for j, f in ipairs(gf) do
                if not used[j] and suffix(f.id) == hs then foot = j break end
            end
        end
        if not foot then
            local j = #gf - i + 1
            if gf[j] and not used[j] then foot = j end
        end
        if foot then used[foot] = true end
        if h.field == nil or h.field == "" then
            add_log("warn", "group " .. tostring(h.id) .. " has no field; it wraps every row once")
        end
        levels[i] = { header = h, footer = foot and gf[foot] or nil }
    end
    for j, f in ipairs(gf) do
        if not used[j] then
            add_log("warn", "group footer " .. tostring(f.id) .. " has no group header; skipped")
        end
    end

    -- Rows: the first detail band with a datasource drives the report.
    local rows, driver
    local function rows_for(name, band_id)
        if not declared[name] then
            add_log("warn", "detail " .. tostring(band_id) .. " names datasource " .. tostring(name)
                .. " which is not declared; no rows")
            return {}
        end
        local r = sources[name]
        if type(r) ~= "table" then
            add_log("warn", "datasource " .. tostring(name) .. " has no array in sources; no rows")
            return {}
        end
        return r
    end
    for _, d in ipairs(details) do
        if d.datasource ~= nil and d.datasource ~= "" then
            if not driver then
                driver = d.datasource
                rows = rows_for(driver, d.id)
            elseif d.datasource ~= driver then
                add_log("warn", "detail " .. tostring(d.id) .. " uses datasource " .. tostring(d.datasource)
                    .. "; rows come from " .. driver .. " in this version")
            end
        end
    end
    if not driver then
        rows = { {} } -- static report: each band once
    end

    local warned_field = {}
    local function item_out(item, row, band_id)
        local tk = item.typeKey
        local text
        if tk == 0 then
            text = item.text ~= nil and plain(item.text) or ""
        elseif FIELD_KEYS[tk] then
            if item.field == nil or item.field == "" then
                local k = tostring(band_id) .. "/" .. tostring(item.id)
                if not warned_field[k] then
                    warned_field[k] = true
                    add_log("warn", "item " .. tostring(item.id) .. " in band " .. tostring(band_id)
                        .. " has no field; printed blank")
                end
                text = ""
            else
                text = plain(row and row[item.field])
            end
        elseif LATER_KEYS[tk] then
            text = ""
        else
            add_log("warn", "item " .. tostring(item.id) .. " in band " .. tostring(band_id)
                .. " has unknown typeKey " .. tostring(tk) .. "; skipped")
            return nil
        end
        local f = type(item.font) == "table" and item.font or {}
        return {
            id = item.id,
            typeKey = tk,
            text = text,
            align = item.align or "Left",
            font = {
                family = f.family or rfont.family,
                size = f.size or rfont.size,
                style = f.style or rfont.style,
            },
            box = item.box,
        }
    end

    local out = {}
    local function emit(band, row)
        local items = {}
        for _, item in ipairs(band.items or {}) do
            if type(item) == "table" then
                local o = item_out(item, row, band.id)
                if o then items[#items + 1] = o end
            end
        end
        out[#out + 1] = { id = band.id, typeKey = band.typeKey, height = band.height, items = items }
    end
    local function emit_all(list, row)
        for _, b in ipairs(list) do emit(b, row) end
    end

    local first = rows[1]
    emit_all(rh, first)
    emit_all(ph, first)

    local function key(row, lv)
        local f = levels[lv].header.field
        if f == nil or f == "" then return "" end
        return plain(row[f])
    end

    local prev
    for _, row in ipairs(rows) do
        local brk = #levels + 1
        if prev == nil then
            brk = 1
        else
            for lv = 1, #levels do
                if key(row, lv) ~= key(prev, lv) then brk = lv break end
            end
        end
        if prev ~= nil then
            for lv = #levels, brk, -1 do
                if levels[lv].footer then emit(levels[lv].footer, prev) end
            end
        end
        for lv = brk, #levels do
            emit(levels[lv].header, row)
        end
        emit_all(details, row)
        prev = row
    end
    if prev ~= nil then
        for lv = #levels, 1, -1 do
            if levels[lv].footer then emit(levels[lv].footer, prev) end
        end
    end

    emit_all(pf, prev or first)
    emit_all(rf, prev or first)

    H.set_result_json({ pages = { { bands = out } }, log = log })
    return 0
end

local ok, res = pcall(compose)
if not ok then
    return fail("compose failed: " .. tostring(res))
end
return res or 0
                ]==],
                'Report compose: definition + sources to the structural intermediate',
                1,
                0,
                NULL,
                NULL,
                ${COMMON_VALUES}
            );

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Seed Reports.Render'                                         AS name,
        [=[
            # Forward Migration ${MIGRATION}: Seed Reports.Render

            Reporting Phase 19. Invokable script in the `Reports` group.
            `stage=compose` turns an inline `definition` plus `sources`
            into the structural intermediate (pages of bands of items, plus
            `log`). Every band role, any count, groups nest. Values as-is.

            - `script_type = 1`
            - `invokable = 1` (POST /api/conduit/script)
            - `mcp_access = 0`
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
            WHERE group_name = 'Reports'
              AND script_name = 'Render';

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Remove Reports.Render'                                       AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Remove Reports.Render
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]====]})

return queries end
