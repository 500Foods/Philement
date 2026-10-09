-- schematool_rows_model.lua
-- In-memory row model for schematool_rows.
--
-- Builds a keyed default-row model from classified SQL statements:
--   - CREATE TABLE notes fold primary keys and unique keys.
--   - INSERT/UPDATE/DELETE against a known table fold into keyed rows.
-- Statements that do not bind a single key row are unkeyed (AutoMigration's
-- domain); the caller stays refused. A live key no migration names is not a
-- finding and is not deleted.
--
-- The model also emits probe SQL (a targeted SELECT to fetch the live
-- rows the migration owns) and JSON round-trips (encode_model/decode_model).
--
-- Usage: required by schematool_rows.lua
--
-- CHANGELOG
-- 1.0.0 - 2026-10-09 - Split from schematool_rows.lua
--
-- luacheck: globals arg package

local json = require("schematool_rows_json")
local parse = require("schematool_rows_parse")
local apply = require("schemahelper_apply")

local M = {}

function M.display_key(parts)
    if #parts == 1 then
        return parts[1]
    end
    return table.concat(parts, "|")
end

local function same_list(a, b)
    if #a ~= #b then
        return false
    end
    for i = 1, #a do
        if a[i] ~= b[i] then
            return false
        end
    end
    return true
end

local function take_literal(v)
    if not v or v.kind == "expr" then
        return nil
    end
    if v.kind == "null" then
        return { null = true }
    end
    if v.kind == "num" then
        return { text = v.text, num = true }
    end
    if v.kind == "bool" or v.kind == "str" then
        return { text = v.text }
    end
    return nil
end

local function blank_row(key_cols, parts, ref)
    return {
        key = M.display_key(parts),
        parts = parts,
        key_cols = key_cols,
        ref = ref,
        values = {},
        nums = {},
        nulls = {},
        order = {},
    }
end

local function put_col(row, col, lit)
    local seen = row.values[col] ~= nil or row.nulls[col]
    if not seen then
        row.order[#row.order + 1] = col
    end
    if lit.null then
        row.values[col] = nil
        row.nums[col] = nil
        row.nulls[col] = true
        return
    end
    row.nulls[col] = nil
    row.values[col] = lit.text
    if lit.num then
        row.nums[col] = true
    else
        row.nums[col] = nil
    end
end

local function bind_columns(info, present)
    local function try(cols)
        if not cols or #cols == 0 then
            return nil
        end
        local parts = {}
        for _, col in ipairs(cols) do
            local lit = present[col]
            if not lit or not lit.text then
                return nil
            end
            parts[#parts + 1] = lit.text
        end
        return cols, parts
    end
    local cols, parts = try(info.pk)
    if cols then
        return cols, parts
    end
    for _, uniq in ipairs(info.unique or {}) do
        cols, parts = try(uniq)
        if cols then
            return cols, parts
        end
    end
    return nil
end

local function ensure_state(model, tname)
    local st = model.tables[tname]
    if st then
        return st
    end
    st = { key_cols = {}, rows = {}, tombs = {} }
    model.tables[tname] = st
    model.table_order[#model.table_order + 1] = tname
    return st
end

local function remember_unkeyed(model, ref, dml)
    model.unkeyed[#model.unkeyed + 1] = {
        ref = ref,
        table = dml.table or "",
        op = dml.op or "",
        note = dml.note or "WHERE does not bind one row",
    }
end

local function preds_present(preds)
    local present = {}
    for _, pred in ipairs(preds) do
        if present[pred.col] then
            return nil
        end
        present[pred.col] = take_literal(pred.val)
    end
    return present
end

local function accept_key(st, key_cols)
    if #st.key_cols == 0 then
        st.key_cols = key_cols
        return true
    end
    return same_list(st.key_cols, key_cols)
end

local function apply_insert(model, keys, dml, ref)
    local info = parse.ensure_key(keys, dml.table)
    local st = ensure_state(model, dml.table)
    for _, vals in ipairs(dml.rows or {}) do
        if #vals ~= #dml.columns then
            remember_unkeyed(model, ref, {
                op = "insert",
                table = dml.table,
                unkeyed = true,
                note = "INSERT column and value counts differ",
            })
        else
            local present = {}
            local lits = {}
            for n, col in ipairs(dml.columns) do
                local lit = take_literal(vals[n])
                if lit then
                    lits[col] = lit
                    if lit.text then
                        present[col] = lit
                    end
                end
            end
            local key_cols, parts = bind_columns(info, present)
            if not key_cols or not accept_key(st, key_cols) then
                remember_unkeyed(model, ref, {
                    op = "insert",
                    table = dml.table,
                    unkeyed = true,
                    note = "INSERT does not supply a key",
                })
            else
                local row = blank_row(key_cols, parts, ref)
                for _, col in ipairs(key_cols) do
                    put_col(row, col, lits[col])
                end
                for _, col in ipairs(dml.columns) do
                    if lits[col] and not row.values[col] and not row.nulls[col] then
                        put_col(row, col, lits[col])
                    end
                end
                st.rows[row.key] = row
                st.tombs[row.key] = nil
            end
        end
    end
end

local function apply_update(model, keys, dml, ref)
    local info = parse.ensure_key(keys, dml.table)
    local present = preds_present(dml.preds or {})
    if not present then
        remember_unkeyed(model, ref, dml)
        return
    end
    local key_cols, parts = bind_columns(info, present)
    if not key_cols then
        remember_unkeyed(model, ref, {
            op = "update",
            table = dml.table,
            unkeyed = true,
            note = "WHERE does not match a key",
        })
        return
    end
    local st = ensure_state(model, dml.table)
    if not accept_key(st, key_cols) then
        remember_unkeyed(model, ref, {
            op = "update",
            table = dml.table,
            unkeyed = true,
            note = "WHERE does not match the table key",
        })
        return
    end
    local row = st.rows[M.display_key(parts)]
    if not row then
        row = blank_row(key_cols, parts, ref)
        for i, col in ipairs(key_cols) do
            local lit = present[col]
            put_col(row, col, lit or { text = parts[i] })
        end
        st.rows[row.key] = row
        st.tombs[row.key] = nil
    end
    row.ref = ref
    for _, set in ipairs(dml.sets or {}) do
        local lit = take_literal(set.val)
        if lit then
            put_col(row, set.col, lit)
        end
    end
end

local function apply_delete(model, keys, dml, ref)
    local info = parse.ensure_key(keys, dml.table)
    local present = preds_present(dml.preds or {})
    if not present then
        remember_unkeyed(model, ref, dml)
        return
    end
    local key_cols, parts = bind_columns(info, present)
    if not key_cols then
        remember_unkeyed(model, ref, {
            op = "delete",
            table = dml.table,
            unkeyed = true,
            note = "WHERE does not match a key",
        })
        return
    end
    local st = ensure_state(model, dml.table)
    if not accept_key(st, key_cols) then
        remember_unkeyed(model, ref, {
            op = "delete",
            table = dml.table,
            unkeyed = true,
            note = "WHERE does not match the table key",
        })
        return
    end
    local key = M.display_key(parts)
    local tomb = blank_row(key_cols, parts, ref)
    for _, col in ipairs(key_cols) do
        put_col(tomb, col, present[col])
    end
    st.rows[key] = nil
    st.tombs[key] = tomb
end

M.display_key = M.display_key
M.same_list = same_list
M.take_literal = take_literal

local function apply_dml(model, keys, dml, ref, only)
    if not dml or dml.skip then
        return
    end
    local tname = dml.table or ""
    if only and tname ~= "" and not only[tname] then
        return
    end
    if only and tname == "" then
        return
    end
    if dml.unkeyed then
        remember_unkeyed(model, ref, dml)
        return
    end
    if dml.op == "insert" then
        apply_insert(model, keys, dml, ref)
    elseif dml.op == "update" then
        apply_update(model, keys, dml, ref)
    elseif dml.op == "delete" then
        apply_delete(model, keys, dml, ref)
    end
end

function M.key_sort(a, b)
    local na = tonumber(a)
    local nb = tonumber(b)
    if na and nb and na ~= nb then
        return na < nb
    end
    return tostring(a) < tostring(b)
end

function M.sql_lit(engine, text, is_num, is_null)
    if is_null then
        return "NULL"
    end
    if is_num then
        if not tostring(text):match("^%-?%d+%.?%d*$") then
            return nil
        end
        return text
    end
    return apply.field_literal(engine, text)
end

function M.probe_sql(engine, schema, table_name, key_cols, data_cols, probes)
    if not probes or #probes == 0 then
        return nil
    end
    if not apply.qualify_table then
        return nil
    end
    local qualified = apply.qualify_table(engine, schema, table_name)
    local cols = {}
    for _, col in ipairs(key_cols) do
        if not col:match("^[%w_]+$") then
            return nil
        end
        cols[#cols + 1] = col
    end
    for _, col in ipairs(data_cols) do
        if not col:match("^[%w_]+$") then
            return nil
        end
        cols[#cols + 1] = col
    end
    local wheres = {}
    for _, probe in ipairs(probes) do
        local preds = {}
        for i, col in ipairs(key_cols) do
            local lit = M.sql_lit(engine, probe.parts[i], probe.nums[col], false)
            if not lit then
                return nil
            end
            preds[#preds + 1] = col .. " = " .. lit
        end
        wheres[#wheres + 1] = "(" .. table.concat(preds, " AND ") .. ")"
    end
    return "SELECT " .. table.concat(cols, ", ")
        .. " FROM " .. qualified
        .. " WHERE " .. table.concat(wheres, " OR ") .. ";"
end

local function data_columns(st)
    local seen = {}
    for _, col in ipairs(st.key_cols) do
        seen[col] = true
    end
    local cols = {}
    local function add(col)
        if not seen[col] then
            seen[col] = true
            cols[#cols + 1] = col
        end
    end
    local keys = {}
    for key, _ in pairs(st.rows) do
        keys[#keys + 1] = key
    end
    table.sort(keys, M.key_sort)
    for _, key in ipairs(keys) do
        local row = st.rows[key]
        for _, col in ipairs(row.order) do
            add(col)
        end
    end
    return cols
end

local function probe_list(st)
    local list = {}
    local function add(row)
        local nums = {}
        for _, col in ipairs(st.key_cols) do
            if row.nums[col] then
                nums[col] = true
            end
        end
        list[#list + 1] = { parts = row.parts, nums = nums, key = row.key }
    end
    local keys = {}
    for key, _ in pairs(st.rows) do
        keys[#keys + 1] = key
    end
    table.sort(keys, M.key_sort)
    for _, key in ipairs(keys) do
        add(st.rows[key])
    end
    local tombs = {}
    for key, _ in pairs(st.tombs) do
        if not st.rows[key] then
            tombs[#tombs + 1] = key
        end
    end
    table.sort(tombs, M.key_sort)
    for _, key in ipairs(tombs) do
        add(st.tombs[key])
    end
    return list
end

local function finish_probes(model, opts)
    opts = opts or {}
    local engine = opts.engine or "sqlite"
    local schema = opts.schema or ""
    for _, tname in ipairs(model.table_order) do
        local st = model.tables[tname]
        local probes = probe_list(st)
        st.probes = probes
        st.data_cols = data_columns(st)
        st.probe_sql = M.probe_sql(
            engine, schema, tname, st.key_cols, st.data_cols, probes)
    end
end

function M.extract(items, catalog_keys, opts)
    opts = opts or {}
    local keys = {}
    for tname, info in pairs(catalog_keys or {}) do
        local copy = { pk = {}, unique = {} }
        for _, col in ipairs(info.pk or {}) do
            copy.pk[#copy.pk + 1] = tostring(col):lower()
        end
        for _, uniq in ipairs(info.unique or {}) do
            local one = {}
            for _, col in ipairs(uniq) do
                one[#one + 1] = tostring(col):lower()
            end
            copy.unique[#copy.unique + 1] = one
        end
        keys[tostring(tname):lower()] = copy
    end
    local only
    if opts.only then
        only = {}
        for name, _ in pairs(opts.only) do
            only[tostring(name):lower()] = true
        end
    end
    local model = {
        engine = opts.engine or "",
        schema = opts.schema or "",
        tables = {},
        table_order = {},
        unkeyed = {},
    }
    for _, item in ipairs(items or {}) do
        local ref = item.ref or 0
        local in_range = true
        if opts.from_ref and ref < opts.from_ref then
            in_range = false
        end
        if opts.to_ref and ref > opts.to_ref then
            in_range = false
        end
        if in_range then
            for _, stmt in ipairs(parse.split_code(item.code or "")) do
                local dml = parse.classify(stmt, keys)
                apply_dml(model, keys, dml, ref, only)
            end
        end
    end
    finish_probes(model, opts)
    return model
end

function M.encode_model(model)
    local tables = {}
    for _, tname in ipairs(model.table_order) do
        local st = model.tables[tname]
        local rows = {}
        local keys = {}
        for key, _ in pairs(st.rows) do
            keys[#keys + 1] = key
        end
        table.sort(keys, M.key_sort)
        for _, key in ipairs(keys) do
            rows[#rows + 1] = json.encode_row(st.rows[key])
        end
        local tombs = {}
        local tkeys = {}
        for key, _ in pairs(st.tombs) do
            tkeys[#tkeys + 1] = key
        end
        table.sort(tkeys, M.key_sort)
        for _, key in ipairs(tkeys) do
            tombs[#tkeys + 1] = json.encode_row(st.tombs[key])
        end
        tables[#tables + 1] = {
            table = tname,
            key = st.key_cols,
            columns = st.data_cols or {},
            probe_sql = st.probe_sql or "",
            rows = rows,
            tombstones = tombs,
        }
    end
    local loose = model.unkeyed or {}
    local parts = {
        '{"engine":', json.encode_string(model.engine or ""),
        ',"schema":', json.encode_string(model.schema or ""),
        ',"tables":[',
    }
    for i, t in ipairs(tables) do
        if i > 1 then
            parts[#parts + 1] = ","
        end
        parts[#parts + 1] = '{"table":' .. json.encode_string(t.table)
        parts[#parts + 1] = ',"key":' .. json.encode_array(t.key)
        parts[#parts + 1] = ',"columns":' .. json.encode_array(t.columns)
        parts[#parts + 1] = ',"probe_sql":' .. json.encode_string(t.probe_sql)
        parts[#parts + 1] = ',"rows":['
        for r, row in ipairs(t.rows) do
            if r > 1 then
                parts[#parts + 1] = ","
            end
            parts[#parts + 1] = '{"key":' .. json.encode_string(row.key)
            parts[#parts + 1] = ',"ref":' .. tostring(row.ref or 0)
            parts[#parts + 1] = ',"order":' .. json.encode_array(row.order)
            parts[#parts + 1] = ',"parts":' .. json.encode_array(row.parts)
            parts[#parts + 1] = ',"values":' .. json.encode_map(row.values)
            parts[#parts + 1] = ',"nums":' .. json.encode_array(row.nums)
            parts[#parts + 1] = ',"nulls":' .. json.encode_array(row.nulls)
            parts[#parts + 1] = "}"
        end
        parts[#parts + 1] = '],"tombstones":['
        for r, row in ipairs(t.tombstones) do
            if r > 1 then
                parts[#parts + 1] = ","
            end
            parts[#parts + 1] = '{"key":' .. json.encode_string(row.key)
            parts[#parts + 1] = ',"ref":' .. tostring(row.ref or 0)
            parts[#parts + 1] = ',"order":' .. json.encode_array(row.order)
            parts[#parts + 1] = ',"parts":' .. json.encode_array(row.parts)
            parts[#parts + 1] = ',"values":' .. json.encode_map(row.values)
            parts[#parts + 1] = ',"nums":' .. json.encode_array(row.nums)
            parts[#parts + 1] = ',"nulls":' .. json.encode_array(row.nulls)
            parts[#parts + 1] = "}"
        end
        parts[#parts + 1] = "]}"
    end
    parts[#parts + 1] = '],"unkeyed":['
    for i, u in ipairs(loose) do
        if i > 1 then
            parts[#parts + 1] = ","
        end
        parts[#parts + 1] = '{"ref":' .. tostring(u.ref or 0)
        parts[#parts + 1] = ',"table":' .. json.encode_string(u.table or "")
        parts[#parts + 1] = ',"op":' .. json.encode_string(u.op or "")
        parts[#parts + 1] = ',"note":' .. json.encode_string(u.note or "")
        parts[#parts + 1] = "}"
    end
    parts[#parts + 1] = "]}"
    return table.concat(parts)
end

local function list_flags(arr)
    local set = {}
    for _, name in ipairs(arr or {}) do
        set[name] = true
    end
    return set
end

local function row_from_json(obj, key_cols)
    local nums = list_flags(obj.nums)
    local nulls = list_flags(obj.nulls)
    local values = {}
    for col, text in pairs(obj.values or {}) do
        if text ~= nil then
            values[col] = tostring(text)
        end
    end
    return {
        key = tostring(obj.key),
        ref = tonumber(obj.ref) or 0,
        parts = obj.parts or {},
        key_cols = key_cols,
        order = obj.order or {},
        values = values,
        nums = nums,
        nulls = nulls,
    }
end

function M.decode_model(text)
    local obj = json.parse_json(text)
    if type(obj) ~= "table" then
        return nil, "rows JSON is not an object"
    end
    local model = {
        engine = obj.engine or "",
        schema = obj.schema or "",
        tables = {},
        table_order = {},
        unkeyed = obj.unkeyed or {},
    }
    for _, t in ipairs(obj.tables or {}) do
        local key_cols = t.key or {}
        local st = {
            key_cols = key_cols,
            data_cols = t.columns or {},
            probe_sql = t.probe_sql or "",
            rows = {},
            tombs = {},
        }
        for _, row in ipairs(t.rows or {}) do
            local got = row_from_json(row, key_cols)
            st.rows[got.key] = got
        end
        for _, row in ipairs(t.tombstones or {}) do
            local got = row_from_json(row, key_cols)
            st.tombs[got.key] = got
        end
        model.tables[t.table] = st
        model.table_order[#model.table_order + 1] = t.table
    end
    return model
end

return M
