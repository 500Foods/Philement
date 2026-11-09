-- Migration: acuranzo_1474.lua
-- Reports.Render.Aggregate (Reporting)
-- Count, sum, mean, median, mode, min, max, first, last, and page totals.
-- Loaded when the definition has an aggregate item.
--
-- One module of the split Reports.Render. Reports.Render (acuranzo_1479.lua)
-- reads this row with H.altquery_sync and runs it with load() in its
-- per-request environment. The row is not invokable and has no MCP access.
-- Reverse deletes the row. QueryRef 175 stays unused. No lookup.
--
-- luacheck: no max line length
-- luacheck: no unused args
--
-- CHANGELOG
-- 1.0.0 - 2026-11-09 - Split from the acuranzo_1479.lua Reports.Render body

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "scripts"
cfg.MIGRATION = "1474"
cfg.GROUP_NAME = "Reports"
cfg.SCRIPT_NAME = "Render.Aggregate"

local sql_new = [==[
-- Reports.Render.Aggregate (Reporting). Lazy: loaded when the definition has a count, sum, or other aggregate item.
-- Loaded by Reports.Render into its per-request environment.
-- Names defined here are shared with Reports.Render and its other modules.


-- Phase 38 and 39. ctx.aggregates is the scope being emitted, keyed
-- aggregates[op][field]. Report and group scopes are filled before
-- paging. Page totals are filled after the page list exists, from
-- the detail rows placed on that page. A detail band has no
-- aggregates. The map is not copied into the emitted JSON.
ctx = nil
report_aggs = nil
group_aggs = nil
AGG_OPS = { count = true, filled = true, unique = true, sum = true,
    avg = true, mean = true, weighted = true, median = true, mode = true,
    min = true, max = true, first = true, last = true }
agg_warned = {}
function agg_warn(item, band_id, message)
    local k = tostring(band_id) .. "/" .. tostring(item.id) .. "/" .. message
    if agg_warned[k] then return end
    agg_warned[k] = true
    add_log("warn", "item " .. tostring(item.id) .. " in band " .. tostring(band_id) .. " " .. message)
end
function numeric_value(v)
    if type(v) ~= "number" then return nil end
    if v ~= v or v == math.huge or v == -math.huge then return nil end
    return v
end
function unique_key(v)
    if type(v) == "number" then
        local n = numeric_value(v)
        if not n then return nil end
        return "n:" .. plain_number(n)
    end
    return "s:" .. plain(v)
end
function format_aggregate(rawv, item, band_id)
    if type(item) == "table" and item.typeKey == 38
        and (item.op == "min" or item.op == "max"
            or item.op == "first" or item.op == "last") then
        return format_value(rawv, 24, item.mask, item.id, band_id)
    end
    if type(rawv) == "string" then return plain(rawv) end
    return format_value(rawv, 21, item.mask, item.id, band_id)
end
function weight_name(item)
    if type(item) == "table" and item.op == "weighted" and type(item.weight) == "string" then
        return item.weight
    end
    return ""
end
function store_key(item)
    local w = weight_name(item)
    if item.op == "weighted" and w ~= "" then return item.field .. "\0" .. w end
    return item.field
end
function agg_raw(aggs, item)
    if type(aggs) ~= "table" or type(item) ~= "table" then return nil end
    local bucket = aggs[item.op]
    if type(bucket) ~= "table" then return nil end
    if item.op == "weighted" and weight_name(item) ~= "" then
        return bucket[store_key(item)]
    end
    return bucket[item.field]
end
function reduce_numbers(field, list)
    local total, n, skipped = 0, 0, false
    for _, row in ipairs(list) do
        local v = type(row) == "table" and row[field] or nil
        if v ~= nil then
            local num = numeric_value(v)
            if num then
                total = total + num
                n = n + 1
            else
                skipped = true
            end
        end
    end
    return total, n, skipped
end
function mode_less(a, b)
    local ta, tb = type(a), type(b)
    if ta == tb then
        if ta == "number" or ta == "string" then return a < b end
        return false
    end
    if ta == "number" then return true end
    if tb == "number" then return false end
    return ta < tb
end
function reduce_op(op, field, list, weight)
    if op == "count" then return #list, nil end
    if op == "filled" then
        local n = 0
        for _, row in ipairs(list) do
            if type(row) == "table" and row[field] ~= nil then n = n + 1 end
        end
        return n, nil
    end
    if op == "unique" then
        local seen, n = {}, 0
        for _, row in ipairs(list) do
            local v = type(row) == "table" and row[field] or nil
            if v ~= nil then
                local uk = unique_key(v)
                if uk and not seen[uk] then
                    seen[uk] = true
                    n = n + 1
                end
            end
        end
        return n, nil
    end
    if op == "sum" or op == "avg" or op == "mean" then
        local total, n, skipped = reduce_numbers(field, list)
        local flags = skipped and { skip = true } or nil
        if n == 0 then return nil, flags end
        if op == "sum" then return total, flags end
        return total / n, flags
    end
    if op == "weighted" then
        local wname = type(weight) == "string" and weight or ""
        if wname == "" then return nil, { missing = true } end
        local num, den, pairs, skipped = 0, 0, false, false
        for _, row in ipairs(list) do
            if type(row) == "table" then
                local v, w = row[field], row[wname]
                if v ~= nil and w ~= nil then
                    local nv, nw = numeric_value(v), numeric_value(w)
                    if nv and nw then
                        num = num + nv * nw
                        den = den + nw
                        pairs = true
                    else
                        skipped = true
                    end
                elseif v ~= nil and not numeric_value(v) then
                    skipped = true
                end
            end
        end
        local flags = nil
        if skipped then flags = { skip = true } end
        if not pairs then
            flags = flags or {}
            flags.none = true
            return nil, flags
        end
        if den == 0 then
            flags = flags or {}
            flags.zero = true
            return nil, flags
        end
        return num / den, flags
    end
    if op == "median" then
        local nums, skipped = {}, false
        for _, row in ipairs(list) do
            local v = type(row) == "table" and row[field] or nil
            if v ~= nil then
                local num = numeric_value(v)
                if num then nums[#nums + 1] = num else skipped = true end
            end
        end
        local flags = skipped and { skip = true } or nil
        if #nums == 0 then return nil, flags end
        table.sort(nums)
        local count = #nums
        local mid = math.floor(count / 2)
        if count % 2 == 1 then return nums[mid + 1], flags end
        return (nums[mid] + nums[mid + 1]) / 2, flags
    end
    if op == "mode" then
        local counts, values, order, bad = {}, {}, {}, false
        for _, row in ipairs(list) do
            local v = type(row) == "table" and row[field] or nil
            if v ~= nil then
                local key, stored
                if type(v) == "number" then
                    local num = numeric_value(v)
                    if num then
                        key = "n:" .. plain_number(num)
                        stored = num
                    else
                        bad = true
                    end
                elseif type(v) == "string" then
                    key = "s:" .. v
                    stored = v
                else
                    bad = true
                end
                if key then
                    if not counts[key] then
                        counts[key] = 0
                        values[key] = stored
                        order[#order + 1] = key
                    end
                    counts[key] = counts[key] + 1
                end
            end
        end
        local flags = bad and { bad = true } or nil
        if #order == 0 then return nil, flags end
        local best, keys = 0, {}
        for _, key in ipairs(order) do
            local n = counts[key]
            if n > best then
                best = n
                keys = { key }
            elseif n == best then
                keys[#keys + 1] = key
            end
        end
        local winner = keys[1]
        for i = 2, #keys do
            if mode_less(values[keys[i]], values[winner]) then winner = keys[i] end
        end
        if #keys > 1 then
            flags = flags or {}
            flags.tie = true
        end
        return values[winner], flags
    end
    if op == "min" or op == "max" then
        local kind, best, other = nil, nil, false
        for _, row in ipairs(list) do
            local v = type(row) == "table" and row[field] or nil
            if v ~= nil then
                local stored, k
                if type(v) == "number" then
                    stored = numeric_value(v)
                    k = stored and "number" or nil
                elseif type(v) == "string" then
                    stored, k = v, "string"
                end
                if not k then
                    other = true
                elseif not kind then
                    kind, best = k, stored
                elseif k ~= kind then
                    other = true
                elseif (op == "min" and stored < best) or (op == "max" and best < stored) then
                    best = stored
                end
            end
        end
        local flags = other and { other = true } or nil
        return best, flags
    end
    if op == "first" or op == "last" then
        local found = nil
        for _, row in ipairs(list) do
            local v = type(row) == "table" and row[field] or nil
            if v ~= nil then
                if op == "first" then
                    if found == nil then found = v end
                else
                    found = v
                end
            end
        end
        return found, nil
    end
    return nil, nil
end
function collect_aggs(bands)
    local specs = {}
    for _, band in ipairs(bands) do
        if type(band) == "table" then
            for _, item in ipairs(band.items or {}) do
                if type(item) == "table" and (item.typeKey == 35 or item.typeKey == 34 or item.typeKey == 38)
                    and (type(item["function"]) ~= "string" or item["function"] == "")
                    and type(item.field) == "string" and item.field ~= ""
                    and AGG_OPS[item.op] then
                    specs[#specs + 1] = { item = item, band_id = band.id }
                end
            end
        end
    end
    return specs
end
function note_flags(item, band_id, flags)
    if type(flags) ~= "table" then return end
    if flags.skip then
        agg_warn(item, band_id, item.op .. " skipped a value that is not a number")
    end
    if flags.missing then
        agg_warn(item, band_id, "has no weight; printed blank")
    end
    if flags.none then
        agg_warn(item, band_id, "weighted has no numeric pair; printed blank")
    end
    if flags.zero then
        agg_warn(item, band_id, "weighted weights sum to zero; printed blank")
    end
    if flags.tie then
        agg_warn(item, band_id, "mode tied; printed the smallest value")
    end
    if flags.bad then
        agg_warn(item, band_id, "mode skipped a value that is not text or a number")
    end
    if flags.other then
        agg_warn(item, band_id, item.op .. " skipped a value of the other kind")
    end
end
function fill_aggs(specs, list)
    local aggs, have, value, flags = {}, {}, {}, {}
    for _, spec in ipairs(specs) do
        local item = spec.item
        local op, field = item.op, item.field
        local id = op .. "\0" .. field .. "\0" .. weight_name(item)
        aggs[op] = aggs[op] or {}
        if not have[id] then
            have[id] = true
            value[id], flags[id] = reduce_op(op, field, list, item.weight)
        end
        aggs[op][store_key(item)] = value[id]
        if op == "weighted" then aggs[op][field] = value[id] end
        note_flags(item, spec.band_id, flags[id])
    end
    return aggs
end

]==]

-- ----------------------------------------------------------------------------
-- Forward: insert Reports.Render.Aggregate
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
]====] .. sql_new .. [====[
                ]==],
                'Reports.Render.Aggregate module (Reporting)',
                0,
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
        'Insert Reports.Render.Aggregate'                                        AS name,
        [=[
            # Forward Migration ${MIGRATION}: Reports.Render.Aggregate

            Inserts group Reports, script Render.Aggregate. Count, sum, mean, median, mode, min, max, first, last, and page totals. Loaded when the definition has an aggregate item.
            Reports.Render loads it. It is not invokable.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]====]})

-- ----------------------------------------------------------------------------
-- Reverse: delete Reports.Render.Aggregate
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
            WHERE group_name = '${GROUP_NAME}'
              AND script_name = '${SCRIPT_NAME}';

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Delete Reports.Render.Aggregate'                                        AS name,
        [=[
            # Reverse Migration ${MIGRATION}: delete Reports.Render.Aggregate

            Deletes the group Reports script Render.Aggregate row.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]====]})

return queries end
