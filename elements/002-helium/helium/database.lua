-- database.lua
-- Defines the Helium schema, supported database engines, and SQL defaults for migrations used in Hydrogen

--[[
    CHANGELOG
    2025-09-13 | 1.0.0      | Andrew Simard     | Initial creation with support for SQLite, PostgreSQL, MySQL, DB2
]]

local database = {

    schema = "helium",

    engines = {
        sqlite = true,
        postgresql = true,
        mysql = true,
        db2 = true
    },

    defaults = {

        sqlite = {
            INTEGER = "INTEGER",
            TEXT = "TEXT",
            TIMESTAMP = "CURRENT_TIMESTAMP",
            CHECK_CONSTRAINT = "CHECK(status IN ('Pending', 'Applied', 'Utility'))",
            SCHEMA_PREFIX = "" -- No schema prefix
        },

        postgresql = {
            INTEGER = "SERIAL",
            TEXT = "TEXT",
            TIMESTAMP = "CURRENT_TIMESTAMP",
            CHECK_CONSTRAINT = "CHECK(status IN ('Pending', 'Applied', 'Utility'))",
            SCHEMA_PREFIX = "helium." -- e.g., helium.queries
        },

        mysql = {
            INTEGER = "INT",
            TEXT = "VARCHAR(255)",
            TIMESTAMP = "CURRENT_TIMESTAMP",
            CHECK_CONSTRAINT = "ENUM('Pending', 'Applied', 'Utility')",
            SCHEMA_PREFIX = "helium." -- e.g., helium.queries
        },

        db2 = {
            INTEGER = "INTEGER",
            TEXT = "VARCHAR(255)",
            TIMESTAMP = "CURRENT TIMESTAMP",
            CHECK_CONSTRAINT = "CHECK(status IN ('Pending', 'Applied', 'Utility'))",
            SCHEMA_PREFIX = "HELIUM." -- Uppercase for DB2 convention
        }
    },

    replace_query = function(template, engine, prefix)
        if not database.engines[engine] then
            error("Unsupported engine: " .. engine)
        end
        local cfg = database.defaults[engine]
        local schema = prefix or cfg.SCHEMA_PREFIX
        -- Strip leading 32 or 24 spaces, for [=[ and [[ respectively for SQL-within-SQL and then Markdown and SQL indentation
        local sql = template:gsub("\n%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s", "\n") -- 32 spaces
                            :gsub("\n%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s", "\n") -- 24 spaces
        -- Apply placeholders
        for key, value in pairs(cfg) do
            sql = sql:gsub("%%" .. key .. "%%", value)
        end
        sql = sql:gsub("%%SCHEMA%%", schema)
        return sql
    end,

    run_migration = function(queries, engine, prefix)
        local sql_parts = {}
        for _, q in ipairs(queries) do
            local formatted = string.format(q.sql, engine)
            table.insert(sql_parts, database.replace_query(formatted, engine, prefix) .. ";")
        end
        return table.concat(sql_parts, "\n")
    end

}

return database