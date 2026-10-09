-- schemahelper_apply.lua
-- Main module for SchemaHelper apply: builds per-finding SQL statements
-- (whole-row metadata UPDATE/INSERT/DELETE, orphan DELETE, or catalog DDL)
-- and writes apply log files. Delegates pure helpers to sibling submodules
-- under lua/ (schemahelper_apply_sqlutil, _engine, _decode, _json,
-- _logic, _ddl).
--
-- CHANGELOG
-- 0.6.3 - 2026-10-09 - Split into lua/ submodules for the 1000-line cap;
--   this file is now the orchestrator + write_log
-- 0.6.2 - 2026-10-07 - Default-row SQL sees guard_names
-- 0.6.1 - 2026-10-07 - One default row: INSERT, UPDATE, or DELETE; token table.key
-- 0.6.0 - 2026-10-07 - Dialect DDL, whole-row metadata, UTF-8 JSON literals
-- 0.5.9 - 2026-10-07 - Type and dropped catalog findings stay review-only
-- 0.5.8 - 2026-10-07 - MariaDB qualifies with the same backticks as MySQL
-- 0.5.7 - 2026-10-07 - Dollar-quote literals no longer treat cockroachdb as postgresql
-- 0.5.6 - 2026-10-07 - MSSQL bracket qualify and N'' field literals
-- 0.5.5 - 2026-08-24 - Phase 7: catalog DDL apply (nullable / add column), louder confirm (object.column)
-- 0.5.4 - 2026-08-24 - Phase 5 slice: confirmed orphan DELETE (true orphans only)
-- 0.5.0 - 2026-08-23 - Phase 5: per-field UPDATE, confirm REF.field

local U = require("schemahelper_apply_sqlutil")
local E = require("schemahelper_apply_engine")
local DC = require("schemahelper_apply_decode")
local J = require("schemahelper_apply_json")
local Logic = require("schemahelper_apply_logic")
local DDL = require("schemahelper_apply_ddl")

local M = {}

-- Re-export public API from submodules so callers see no difference.
M.file_exists = U.file_exists
M.write_all = U.write_all
M.sh_quote = U.sh_quote
M.utf8_from_code = U.utf8_from_code
M.json_string_field = U.json_string_field
M.json_has_string = U.json_has_string
M.dollar_quote = U.dollar_quote
M.sql_string_literal = U.sql_string_literal
M.backtick_name = U.backtick_name
M.safe_ident = U.safe_ident
M.safe_type = U.safe_type
M.APPLY_FIELDS = U.APPLY_FIELDS
M.ROW_FIELDS = U.ROW_FIELDS

M.is_pg = E.is_pg
M.is_mysql = E.is_mysql
M.supported_engine = E.supported_engine
M.want_null = E.want_null
M.column_is_table = E.column_is_table
M.qualify_queries = E.qualify_queries
M.qualify_table = E.qualify_table
M.guard_names = E.guard_names
M.is_default_kind = E.is_default_kind

M.jq_capture = DC.jq_capture
M.nonempty_lines = DC.nonempty_lines
M.lookup_column = DC.lookup_column
M.lookup_table = DC.lookup_table

M.csv_names = J.csv_names
M.decode_flat = J.decode_flat
M.row_literal = J.row_literal
M.field_literal = J.field_literal
M.build_row_sql = J.build_row_sql

M.confirm_token = Logic.confirm_token
M.refuse_reason = Logic.refuse_reason
M.can_apply = Logic.can_apply

M.column_type_text = DDL.column_type_text
M.fold_null_word = DDL.fold_null_word
M.add_column_sql = DDL.add_column_sql
M.nullability_sql = DDL.nullability_sql
M.type_sql = DDL.type_sql
M.mssql_drop_column = DDL.mssql_drop_column
M.drop_column_sql = DDL.drop_column_sql
M.create_table_sql = DDL.create_table_sql
M.build_catalog_sql = DDL.build_catalog_sql

local function metadata_assign(engine, finding)
    if finding.field == "row" then
        local sets = {}
        for i = 1, #M.ROW_FIELDS do
            local name = M.ROW_FIELDS[i]
            if U.json_has_string(finding.expected, name) then
                local value = U.json_string_field(finding.expected, name)
                sets[#sets + 1] = name .. " = "
                    .. M.field_literal(engine, value)
            end
        end
        if #sets == 0 then
            return nil, "expected row has no code, name, or summary"
        end
        return "SET " .. table.concat(sets, ",\n       ")
    end
    local value = U.json_string_field(finding.expected, finding.field)
    return "SET " .. finding.field .. " = " .. M.field_literal(engine, value)
end

function M.build_sql(finding, conn, out_dir)
    conn = conn or {}
    local why = M.refuse_reason(finding, true, conn.engine)
    if why then
        return nil, why
    end
    local engine = conn.engine or ""
    if engine == "" then
        return nil, "unresolved engine"
    end
    local kind = finding.kind or ""
    if E.is_default_kind(kind) then
        return J.build_row_sql(finding, engine, conn.schema)
    end
    local class = finding.class or ""
    if class:find("^catalog") then
        return M.build_catalog_sql(finding, conn, out_dir)
    end
    local qtable = M.qualify_queries(engine, conn.schema)
    if finding.kind == "orphan" then
        if type(finding.ref) ~= "number" or finding.ref < 1 then
            return nil, "missing ref"
        end
        return string.format(
            "DELETE FROM %s\n WHERE query_ref = %d\n   AND query_type_a28 BETWEEN 1000 AND 1003;",
            qtable, finding.ref)
    end
    local assign, assign_err = metadata_assign(engine, finding)
    if not assign then
        return nil, assign_err
    end
    return string.format(
        "UPDATE %s\n   %s\n WHERE query_ref = %d\n   AND query_type_a28 = %d;",
        qtable, assign, finding.ref, finding.db_type)
end

function M.write_log(out_dir, finding, sql)
    if not out_dir or out_dir == "" then
        return nil, "no out-dir"
    end
    local stamp = os.date("!%Y%m%dT%H%M%SZ")
    local name
    local header
    local caveat
    local class = finding.class or ""
    if finding.kind == "orphan" then
        name = string.format("schemahelper_apply_%d_delete_%s.sql",
            finding.ref, stamp)
        header = "-- SchemaHelper apply (orphan DELETE; not executed by SchemaTool)"
        caveat = "-- Deleting orphan rows does NOT author a migration."
    elseif E.is_default_kind(finding.kind) then
        local obj = (finding.object or "row"):gsub("[^%w_]", "_")
        local col = (finding.column or "key"):gsub("[^%w_]", "_")
        name = string.format("schemahelper_apply_row_%s_%s_%s.sql",
            obj, col, stamp)
        header = "-- SchemaHelper apply (one default row; not executed by SchemaTool)"
        caveat = "-- This statement changes only the named key."
    elseif class:find("^catalog") then
        local obj = finding.object or "unknown"
        local col = finding.column or "-"
        col = col:gsub("[^%w_]", "_")
        obj = obj:gsub("[^%w_]", "_")
        name = string.format("schemahelper_apply_ddl_%s_%s_%s.sql",
            obj, col, stamp)
        header = "-- SchemaHelper apply (catalog DDL; not executed by SchemaTool)"
        if finding.kind == "dropped" then
            caveat = "-- This DROP removes an object the fold dropped."
        elseif finding.kind == "table" then
            caveat = "-- This CREATE adds a table from the fold."
        else
            caveat = "-- This statement mutates live DDL shape."
        end
    else
        name = string.format("schemahelper_apply_%d_%s_%s.sql",
            finding.ref, finding.field, stamp)
        header = "-- SchemaHelper apply (metadata only; not executed by SchemaTool)"
        caveat = "-- Updating queries metadata does NOT replay DDL."
    end
    local lines = {
        header,
        "-- finding " .. tostring(finding.id or ""),
        "-- confirm " .. tostring(M.confirm_token(finding) or ""),
        caveat,
        sql,
        "",
    }
    local path = out_dir .. "/" .. name
    local f, err = io.open(path, "wb")
    if not f then
        return nil, err
    end
    f:write(table.concat(lines, "\n"))
    f:close()
    return path
end

return M
