-- schemahelper_apply_ddl.lua
-- Catalog DDL statement generators for SchemaHelper apply: column type,
-- nullability, add/drop column, create/drop table, and build_catalog_sql.
-- Depends on schemahelper_apply_sqlutil, schemahelper_apply_engine,
-- schemahelper_apply_decode, and schemahelper_apply_logic.
--
-- CHANGELOG
-- 0.6.3 - 2026-10-09 - Extracted from schemahelper_apply.lua (DDL generators)

local U = require("schemahelper_apply_sqlutil")
local E = require("schemahelper_apply_engine")
local DC = require("schemahelper_apply_decode")
local Logic = require("schemahelper_apply_logic")

local M = {}

function M.column_type_text(out_dir, table_name, column, prefer)
    local typ = U.safe_type(prefer)
    if typ then
        return typ
    end
    local col, err = DC.lookup_column(out_dir, table_name, column)
    if not col then
        return nil, err
    end
    typ = U.safe_type(col.data_type)
    if not typ then
        return nil, "unsafe column type"
    end
    return typ
end

function M.fold_null_word(out_dir, table_name, column)
    local col, err = DC.lookup_column(out_dir, table_name, column)
    if not col then
        return nil, err
    end
    if col.nullable then
        return "NULL"
    end
    return "NOT NULL"
end

function M.add_column_sql(engine, qualified, column, out_dir, table_name)
    local typ, err = M.column_type_text(out_dir, table_name, column, nil)
    if not typ then
        return nil, err
    end
    if engine == "firebird" or engine == "mssql" then
        return string.format(
            "ALTER TABLE %s ADD %s %s;", qualified, column, typ)
    end
    return string.format(
        "ALTER TABLE %s ADD COLUMN %s %s;", qualified, column, typ)
end

function M.nullability_sql(engine, qualified, column, finding, out_dir, table_name)
    local action
    if E.want_null(finding) then
        action = "DROP NOT NULL"
    else
        action = "SET NOT NULL"
    end
    if E.is_pg(engine) or engine == "db2" then
        return string.format(
            "ALTER TABLE %s ALTER COLUMN %s %s;",
            qualified, column, action)
    end
    if engine == "firebird" then
        return string.format(
            "ALTER TABLE %s ALTER %s %s;",
            qualified, column, action)
    end
    local typ, err = M.column_type_text(out_dir, table_name, column, nil)
    if not typ then
        return nil, err
    end
    local verb
    if E.want_null(finding) then
        verb = "NULL"
    else
        verb = "NOT NULL"
    end
    if E.is_mysql(engine) then
        return string.format(
            "ALTER TABLE %s MODIFY COLUMN %s %s %s;",
            qualified, column, typ, verb)
    end
    if engine == "mssql" then
        return string.format(
            "ALTER TABLE %s ALTER COLUMN %s %s %s;",
            qualified, column, typ, verb)
    end
    return nil, "unsupported engine"
end

function M.type_sql(engine, qualified, column, finding, out_dir, table_name)
    local typ, err = M.column_type_text(
        out_dir, table_name, column, finding.expected)
    if not typ then
        return nil, err
    end
    if E.is_pg(engine) then
        return string.format(
            "ALTER TABLE %s ALTER COLUMN %s TYPE %s;",
            qualified, column, typ)
    end
    if engine == "db2" then
        return string.format(
            "ALTER TABLE %s ALTER COLUMN %s SET DATA TYPE %s;",
            qualified, column, typ)
    end
    if engine == "firebird" then
        return string.format(
            "ALTER TABLE %s ALTER %s TYPE %s;",
            qualified, column, typ)
    end
    local verb, verb_err = M.fold_null_word(out_dir, table_name, column)
    if not verb then
        return nil, verb_err
    end
    if E.is_mysql(engine) then
        return string.format(
            "ALTER TABLE %s MODIFY COLUMN %s %s %s;",
            qualified, column, typ, verb)
    end
    if engine == "mssql" then
        return string.format(
            "ALTER TABLE %s ALTER COLUMN %s %s %s;",
            qualified, column, typ, verb)
    end
    return nil, "unsupported engine"
end

function M.mssql_drop_column(schema, table_name, column, qualified)
    local lines = {
        "DECLARE @schemahelper_dc sysname;",
        "DECLARE @schemahelper_sql nvarchar(1000);",
        "SELECT @schemahelper_dc = dc.name",
        "  FROM sys.default_constraints AS dc",
        "  INNER JOIN sys.columns AS c",
        "    ON c.object_id = dc.parent_object_id",
        "   AND c.column_id = dc.parent_column_id",
        "  INNER JOIN sys.tables AS t ON t.object_id = c.object_id",
        "  INNER JOIN sys.schemas AS s ON s.schema_id = t.schema_id",
        " WHERE t.name = N'" .. table_name .. "'",
    }
    if schema and schema ~= "" and schema ~= "." then
        lines[#lines + 1] = "   AND s.name = N'" .. schema .. "'"
    end
    lines[#lines + 1] = "   AND c.name = N'" .. column .. "';"
    lines[#lines + 1] = "IF @schemahelper_dc IS NOT NULL"
    lines[#lines + 1] = "BEGIN"
    lines[#lines + 1] = "    SET @schemahelper_sql = N'ALTER TABLE "
        .. qualified .. " DROP CONSTRAINT ' +"
    lines[#lines + 1] = "        QUOTENAME(@schemahelper_dc);"
    lines[#lines + 1] = "    EXEC sp_executesql @schemahelper_sql;"
    lines[#lines + 1] = "END"
    lines[#lines + 1] = "ALTER TABLE " .. qualified
        .. " DROP COLUMN " .. column .. ";"
    return table.concat(lines, "\n")
end

function M.drop_column_sql(engine, schema, table_name, column, qualified)
    if engine == "firebird" then
        return string.format("ALTER TABLE %s DROP %s;", qualified, column)
    end
    if engine == "mssql" then
        return M.mssql_drop_column(schema, table_name, column, qualified)
    end
    if engine == "db2" then
        return string.format(
            "ALTER TABLE %s DROP COLUMN %s;\nCOMMIT;\nREORG TABLE %s;",
            qualified, column, qualified)
    end
    return string.format(
        "ALTER TABLE %s DROP COLUMN %s;", qualified, column)
end

function M.create_table_sql(engine, qualified, out_dir, table_name)
    local cols, pk, err = DC.lookup_table(out_dir, table_name)
    if not cols then
        return nil, err
    end
    local lines = {}
    for i = 1, #cols do
        local col = cols[i]
        local ident = U.safe_ident(col.name)
        local typ = U.safe_type(col.data_type)
        if not ident or not typ then
            return nil, "unsafe column in " .. table_name
        end
        local nullsql = ""
        if not col.nullable then
            nullsql = " NOT NULL"
        end
        lines[#lines + 1] = "    " .. ident .. " " .. typ .. nullsql
    end
    if pk and #pk > 0 then
        local pk_ok = {}
        for i = 1, #pk do
            local ident = U.safe_ident(pk[i])
            if not ident then
                return nil, "unsafe primary key"
            end
            pk_ok[#pk_ok + 1] = ident
        end
        lines[#lines + 1] = "    PRIMARY KEY ("
            .. table.concat(pk_ok, ", ") .. ")"
    end
    return "CREATE TABLE " .. qualified .. " (\n"
        .. table.concat(lines, ",\n") .. "\n);"
end

function M.build_catalog_sql(finding, conn, out_dir)
    local engine = conn and conn.engine or ""
    if engine == "" then
        return nil, "unresolved engine"
    end
    local why = Logic.refuse_reason(finding, true, engine)
    if why then
        return nil, why
    end
    local schema = conn and conn.schema or ""
    local table_name = finding.object or ""
    local qualified, name_err = E.guard_names(engine, schema, table_name)
    if not qualified then
        return nil, name_err
    end
    local kind = finding.kind or ""
    local column = finding.column or ""
    if kind == "table" then
        return M.create_table_sql(engine, qualified, out_dir, table_name)
    end
    if kind == "dropped" and E.column_is_table(column) then
        return string.format("DROP TABLE %s;", qualified)
    end
    if not U.safe_ident(column) then
        return nil, "unsafe column"
    end
    if kind == "column" then
        return M.add_column_sql(engine, qualified, column, out_dir, table_name)
    end
    if kind == "nullable" then
        return M.nullability_sql(
            engine, qualified, column, finding, out_dir, table_name)
    end
    if kind == "type" then
        return M.type_sql(
            engine, qualified, column, finding, out_dir, table_name)
    end
    if kind == "dropped" then
        return M.drop_column_sql(engine, schema, table_name, column, qualified)
    end
    return nil, "unsupported catalog check: " .. tostring(kind)
end

return M
