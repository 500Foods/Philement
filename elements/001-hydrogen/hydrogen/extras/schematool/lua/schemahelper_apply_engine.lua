-- schemahelper_apply_engine.lua
-- Engine classification and name-qualification helpers for SchemaHelper
-- apply operations. Depends on schemahelper_apply_sqlutil.
--
-- CHANGELOG
-- 0.6.3 - 2026-10-09 - Extracted from schemahelper_apply.lua (engine/qualify helpers)

local U = require("schemahelper_apply_sqlutil")

local M = {}

function M.is_pg(engine)
    return engine == "postgresql" or engine == "yugabytedb"
end

function M.is_mysql(engine)
    return engine == "mysql" or engine == "mariadb"
end

function M.supported_engine(engine)
    return M.is_pg(engine) or M.is_mysql(engine) or engine == "sqlite"
        or engine == "db2" or engine == "firebird" or engine == "mssql"
end

function M.want_null(finding)
    local e = tostring(finding and finding.expected or "")
    return e == "true" or e == "YES" or e == "1"
end

function M.column_is_table(column)
    return column == nil or column == "" or column == "-"
end

function M.qualify_queries(engine, schema)
    if not schema or schema == "" or schema == "." or engine == "sqlite" then
        return "queries"
    end
    if engine == "db2" then
        return string.upper(schema) .. ".QUERIES"
    end
    if engine == "mssql" and schema:match("^[%w_]+$") then
        return "[" .. schema .. "].[queries]"
    end
    if engine == "mysql" or engine == "mariadb" then
        return U.backtick_name(schema, "queries")
    end
    return schema .. ".queries"
end

function M.qualify_table(engine, schema, table_name)
    if not schema or schema == "" or schema == "." or engine == "sqlite" then
        return table_name
    end
    if engine == "db2" then
        return string.upper(schema) .. "." .. string.upper(table_name)
    end
    if engine == "mssql"
        and schema:match("^[%w_]+$")
        and tostring(table_name):match("^[%w_]+$") then
        return "[" .. schema .. "].[" .. table_name .. "]"
    end
    if engine == "mysql" or engine == "mariadb" then
        return U.backtick_name(schema, table_name)
    end
    return schema .. "." .. table_name
end

function M.guard_names(engine, schema, table_name)
    if not M.supported_engine(engine) then
        return nil, "unsupported engine"
    end
    if schema and schema ~= "" and schema ~= "." and not U.safe_ident(schema) then
        return nil, "unsafe schema"
    end
    if not U.safe_ident(table_name) then
        return nil, "unsafe table"
    end
    return M.qualify_table(engine, schema, table_name)
end

function M.is_default_kind(kind)
    return kind == "row_missing" or kind == "row_diff"
        or kind == "row_present"
end

return M
