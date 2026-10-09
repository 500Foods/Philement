-- schematool_rows_live.lua
-- Live probe, compare, and findings encoding for schematool_rows.
--
-- Joins the expected default-row model against live database probe output
-- (or SQLite row probes) and emits findings: row_missing, row_diff,
-- row_present (migration deleted but row still live), and unkeyed DML.
-- Also encodes a live probe snapshot to/from JSON for caching.
--
-- Usage: required by schematool_rows.lua
--
-- CHANGELOG
-- 1.0.0 - 2026-10-09 - Split from schematool_rows.lua
--
-- luacheck: globals arg package

local json = require("schematool_rows_json")
local model = require("schematool_rows_model")

local M = {}

function M.expected_payload(row, cols)
    local nums = {}
    local nulls = {}
    local parts = {}
    parts[#parts + 1] = '{"__keys":"'
        .. json.json_escape(table.concat(row.key_cols, ",")) .. '"'
    parts[#parts + 1] = ',"__order":"' .. json.json_escape(table.concat(cols, ",")) .. '"'
    for _, col in ipairs(cols) do
        if row.nulls[col] then
            nulls[#nulls + 1] = col
        else
            local text = row.values[col]
            if text ~= nil then
                if row.nums[col] then
                    nums[#nums + 1] = col
                end
                parts[#parts + 1] = ',"' .. json.json_escape(col) .. '":"'
                    .. json.json_escape(text) .. '"'
            end
        end
    end
    if #nums > 0 then
        parts[#parts + 1] = ',"__nums":"' .. json.json_escape(table.concat(nums, ",")) .. '"'
    end
    if #nulls > 0 then
        parts[#parts + 1] = ',"__nulls":"' .. json.json_escape(table.concat(nulls, ",")) .. '"'
    end
    parts[#parts + 1] = "}"
    return table.concat(parts)
end

local function live_equal(row, col, live_val)
    if row.nulls[col] then
        return live_val == nil
    end
    if live_val == nil then
        return false
    end
    return tostring(live_val) == tostring(row.values[col])
end

local function live_bits(cols, live_row)
    local bits = {}
    for _, col in ipairs(cols) do
        local v = live_row and live_row[col]
        if v == nil then
            bits[#bits + 1] = col .. "=NULL"
        else
            bits[#bits + 1] = col .. "=" .. tostring(v)
        end
    end
    return table.concat(bits, ", ")
end

local function push_finding(list, item)
    list[#list + 1] = item
end

function M.compare(model_obj, live, sql_only)
    live = live or {}
    local findings = {}
    local counts = {
        row_missing = 0,
        row_diff = 0,
        row_present = 0,
        unkeyed = 0,
    }
    for i, u in ipairs(model_obj.unkeyed or {}) do
        counts.unkeyed = counts.unkeyed + 1
        local tname = u.table ~= "" and u.table or "-"
        push_finding(findings, {
            id = string.format("row:%s:unkeyed:%s:%d", tname, tostring(u.ref or 0), i),
            class = "unkeyed DML",
            check = "unkeyed",
            object = u.table or "",
            column = "-",
            ref = u.ref or 0,
            expected = "",
            live = "",
            notes = "unkeyed DML — run Hydrogen AutoMigration",
        })
    end
    if sql_only then
        return findings, counts
    end
    local probed = live.probed or {}
    for _, tname in ipairs(model_obj.table_order) do
        if probed[tname] then
            local st = model_obj.tables[tname]
            local got = (live.tables and live.tables[tname]) or {}
            local keys = {}
            for key, _ in pairs(st.rows) do
                keys[#keys + 1] = key
            end
            table.sort(keys, model.key_sort)
            for _, key in ipairs(keys) do
                local row = st.rows[key]
                local have = got[key]
                if not have then
                    counts.row_missing = counts.row_missing + 1
                    push_finding(findings, {
                        id = string.format("row:%s:%s:row_missing", tname, key),
                        class = "default row",
                        check = "row_missing",
                        object = tname,
                        column = key,
                        ref = row.ref,
                        expected = M.expected_payload(row, row.order),
                        live = "absent",
                        notes = "missing default row " .. tname .. "." .. key,
                    })
                else
                    local diff_cols = {}
                    for _, col in ipairs(row.order) do
                        local is_key = false
                        for _, kc in ipairs(row.key_cols) do
                            if kc == col then
                                is_key = true
                            end
                        end
                        if not is_key and not live_equal(row, col, have[col]) then
                            diff_cols[#diff_cols + 1] = col
                        end
                    end
                    if #diff_cols > 0 then
                        local payload_cols = {}
                        for _, col in ipairs(row.key_cols) do
                            payload_cols[#payload_cols + 1] = col
                        end
                        for _, col in ipairs(diff_cols) do
                            payload_cols[#payload_cols + 1] = col
                        end
                        counts.row_diff = counts.row_diff + 1
                        push_finding(findings, {
                            id = string.format("row:%s:%s:row_diff", tname, key),
                            class = "default row",
                            check = "row_diff",
                            object = tname,
                            column = key,
                            ref = row.ref,
                            expected = M.expected_payload(row, payload_cols),
                            live = live_bits(diff_cols, have),
                            notes = "migration-owned columns differ on "
                                .. tname .. "." .. key,
                        })
                    end
                end
            end
            local tombs = {}
            for key, _ in pairs(st.tombs) do
                tombs[#tombs + 1] = key
            end
            table.sort(tombs, model.key_sort)
            for _, key in ipairs(tombs) do
                if got[key] then
                    local tomb = st.tombs[key]
                    counts.row_present = counts.row_present + 1
                    push_finding(findings, {
                        id = string.format("row:%s:%s:row_present", tname, key),
                        class = "default row",
                        check = "row_present",
                        object = tname,
                        column = key,
                        ref = tomb.ref,
                        expected = M.expected_payload(tomb, tomb.key_cols),
                        live = "present",
                        notes = "migration deleted " .. tname .. "." .. key
                            .. "; the row is still present",
                    })
                end
            end
        end
    end
    return findings, counts
end

function M.encode_findings(findings, counts)
    counts = counts or {}
    local exit_code = 0
    if (counts.row_missing or 0) > 0 or (counts.row_diff or 0) > 0
        or (counts.row_present or 0) > 0 then
        exit_code = 2
    end
    local parts = {
        '{"exit_code":', tostring(exit_code),
        ',"counts":{"row_missing":', tostring(counts.row_missing or 0),
        ',"row_diff":', tostring(counts.row_diff or 0),
        ',"row_present":', tostring(counts.row_present or 0),
        ',"unkeyed":', tostring(counts.unkeyed or 0),
        '},"failures":[',
    }
    for i, f in ipairs(findings or {}) do
        if i > 1 then
            parts[#parts + 1] = ","
        end
        parts[#parts + 1] = '{"id":' .. json.encode_string(f.id or "")
        parts[#parts + 1] = ',"class":' .. json.encode_string(f.class or "")
        parts[#parts + 1] = ',"object":' .. json.encode_string(f.object or "")
        parts[#parts + 1] = ',"column":' .. json.encode_string(f.column or "")
        parts[#parts + 1] = ',"check":' .. json.encode_string(f.check or "")
        parts[#parts + 1] = ',"status":"N"'
        parts[#parts + 1] = ',"expected":' .. json.encode_string(f.expected or "")
        parts[#parts + 1] = ',"live":' .. json.encode_string(f.live or "")
        parts[#parts + 1] = ',"notes":' .. json.encode_string(f.notes or "")
        if f.ref and f.ref > 0 then
            parts[#parts + 1] = ',"ref":' .. tostring(f.ref)
        end
        parts[#parts + 1] = "}"
    end
    parts[#parts + 1] = "]}"
    return table.concat(parts)
end

function M.live_from_probe(model_obj, raw_by_table)
    local live = { probed = {}, tables = {} }
    for tname, body in pairs(raw_by_table or {}) do
        local st = model_obj.tables[tname]
        if st then
            local arr = json.parse_json(body or "[]")
            if type(arr) ~= "table" then
                return nil, "probe JSON for " .. tname
            end
            local rows = {}
            for _, obj in ipairs(arr) do
                local parts = {}
                local values = {}
                for _, col in ipairs(st.key_cols) do
                    local v = obj[col]
                    if v == nil then
                        parts = nil
                        break
                    end
                    parts[#parts + 1] = tostring(v)
                    values[col] = tostring(v)
                end
                if parts then
                    for col, v in pairs(obj) do
                        if v == nil then
                            values[col] = nil
                        else
                            values[col] = tostring(v)
                        end
                    end
                    rows[model.display_key(parts)] = values
                end
            end
            live.tables[tname] = rows
            live.probed[tname] = true
        end
    end
    return live
end

local function sqlite_uri(path)
    local esc = tostring(path):gsub(" ", "%%20")
    return "file:" .. esc .. "?mode=ro"
end

function M.probe_sqlite(db_path, model_obj)
    local raw = {}
    for _, tname in ipairs(model_obj.table_order) do
        local st = model_obj.tables[tname]
        local sql = st.probe_sql
        if sql and sql ~= "" then
            local cmd = "sqlite3 -json " .. json.sh_quote(sqlite_uri(db_path))
                .. " " .. json.sh_quote(sql)
            local h = io.popen(cmd)
            if not h then
                return nil, "sqlite3 popen failed"
            end
            local body = h:read("*a") or ""
            local ok = h:close()
            if ok ~= true then
                return nil, "sqlite3 probe failed for " .. tname
            end
            raw[tname] = body
        elseif #(st.probes or {}) == 0 then
            raw[tname] = "[]"
        else
            return nil, "no probe sql for " .. tname
        end
    end
    return M.live_from_probe(model_obj, raw)
end

function M.encode_live(live)
    local parts = {'{"probed":['}
    local names = {}
    for name, _ in pairs(live.probed or {}) do
        names[#names + 1] = name
    end
    table.sort(names)
    for i, name in ipairs(names) do
        if i > 1 then
            parts[#parts + 1] = ","
        end
        parts[#parts + 1] = json.encode_string(name)
    end
    parts[#parts + 1] = '],"tables":{'
    for i, name in ipairs(names) do
        if i > 1 then
            parts[#parts + 1] = ","
        end
        parts[#parts + 1] = json.encode_string(name) .. ":["
        local rows = live.tables[name] or {}
        local keys = {}
        for key, _ in pairs(rows) do
            keys[#keys + 1] = key
        end
        table.sort(keys, model.key_sort)
        for r, key in ipairs(keys) do
            if r > 1 then
                parts[#parts + 1] = ","
            end
            parts[#parts + 1] = '{"key":' .. json.encode_string(key)
            parts[#parts + 1] = ',"values":' .. json.encode_map(rows[key])
            parts[#parts + 1] = "}"
        end
        parts[#parts + 1] = "]"
    end
    parts[#parts + 1] = "}}"
    return table.concat(parts)
end

function M.decode_live(text)
    local obj = json.parse_json(text)
    if type(obj) ~= "table" then
        return nil, "live JSON is not an object"
    end
    local live = { probed = {}, tables = {} }
    for _, name in ipairs(obj.probed or {}) do
        live.probed[name] = true
    end
    for tname, rows in pairs(obj.tables or {}) do
        live.tables[tname] = {}
        for _, row in ipairs(rows or {}) do
            local values = {}
            for col, v in pairs(row.values or {}) do
                if v ~= nil then
                    values[col] = tostring(v)
                end
            end
            live.tables[tname][tostring(row.key)] = values
        end
        live.probed[tname] = true
    end
    return live
end

return M
