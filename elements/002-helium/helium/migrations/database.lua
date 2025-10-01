-- database.lua
-- Defines the Helium schema, supported database engines, and SQL defaults for migrations used in Hydrogen

--[[
    CHANGELOG
    2025-09-13 | 1.0.0      | Andrew Simard     | Initial creation with support for SQLite, PostgreSQL, MySQL, DB2
]]

local database = {

    schema = "helium",

    -- Currently supported database engines
    engines = {
        sqlite = true,
        postgresql = true,
        mysql = true,
        db2 = true
    },

    -- Lookup #30 - Need this before it exists
    dialects = {
        sql = 0,
        postgresql = 1,
        sqlite = 2,
        mysql = 3,
        db2 = 4
    },

    status_codes = {
        Pending = 1,
        Applied = 2,
        Utility = 3
    },

    defaults = {

        sqlite = {
	    CREATE_TABLE="CREATE TABLE IF NOT EXISTS",
            SERIAL = "INTEGER PRIMARY KEY AUTOINCREMENT",
            INTEGER = "INTEGER",
            VARCHAR_100 = "VARCHAR(100)",
            TEXT = "TEXT",
            JSONB = "TEXT", -- SQLite doesn't have native JSON, store as TEXT
            TIMESTAMP_TZ = "TEXT", -- SQLite stores as TEXT in ISO format
            TIMESTAMP = "CURRENT_TIMESTAMP",
            CHECK_CONSTRAINT = "CHECK(status IN ('Pending', 'Applied', 'Utility'))",
            SCHEMA_PREFIX = "helium." 
        },

        postgresql = {
	    CREATE_TABLE="CREATE TABLE IF NOT EXISTS",
            SERIAL = "SERIAL",
            INTEGER = "INTEGER",
            VARCHAR_100 = "VARCHAR(100)",
            TEXT = "TEXT",
            JSONB = "JSONB",
            TIMESTAMP_TZ = "TIMESTAMP WITH TIME ZONE",
            TIMESTAMP = "CURRENT_TIMESTAMP",
            CHECK_CONSTRAINT = "CHECK(status IN ('Pending', 'Applied', 'Utility'))",
            SCHEMA_PREFIX = "app." -- Using app schema as per provided statement
        },

        mysql = {
	    CREATE_TABLE="CREATE TABLE IF NOT EXISTS",
            SERIAL = "INT AUTO_INCREMENT",
            INTEGER = "INT",
            VARCHAR_100 = "VARCHAR(100)",
            TEXT = "TEXT",
            JSONB = "JSON",
            TIMESTAMP_TZ = "TIMESTAMP",
            TIMESTAMP = "CURRENT_TIMESTAMP",
            CHECK_CONSTRAINT = "ENUM('Pending', 'Applied', 'Utility')",
            SCHEMA_PREFIX = "helium." -- e.g., helium.queries
        },
        db2 = {
	    CREATE_TABLE="CREATE TABLE",
            SERIAL = "INTEGER GENERATED ALWAYS AS IDENTITY",
            INTEGER = "INTEGER",
            VARCHAR_100 = "VARCHAR(100)",
            TEXT = "VARCHAR(255)", -- DB2 TEXT equivalent
            JSONB = "VARCHAR(4000)", -- DB2 JSON stored as VARCHAR
            TIMESTAMP_TZ = "TIMESTAMP",
            TIMESTAMP = "CURRENT TIMESTAMP",
            CHECK_CONSTRAINT = "CHECK(status IN ('Pending', 'Applied', 'Utility'))",
            SCHEMA_PREFIX = "HELIUM." -- Uppercase for DB2 convention
        }
    },

    replace_query = function(self, template, engine, design_name)

        if not self.engines[engine] then
            error("Unsupported engine: " .. engine)
        end

        local cfg = self.defaults[engine]

        -- Generate schema prefix based on design name and database conventions
        local schema_prefix
        if design_name then
            if engine == "db2" then
                schema_prefix = design_name:upper() .. "."
            elseif engine == "postgresql" then
                schema_prefix = "app."  -- Keep original PostgreSQL schema
            else
                schema_prefix = design_name .. "."
            end
        else
            schema_prefix = cfg.SCHEMA_PREFIX or ""
        end

        local schema = schema_prefix

        -- Add additional placeholders to cfg for unified processing
        cfg.SCHEMA = schema
        cfg.DIALECT = self.dialects[engine]
        cfg.STATUS_UTILITY = self.status_codes.Utility
        cfg.STATUS_PENDING = self.status_codes.Pending
        cfg.STATUS_APPLIED = self.status_codes.Applied

        local sql = template

        -- Strip indentation due to Lua formatting only
        sql = sql:gsub("%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s", "\n")
        sql = sql:gsub("\n%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s", "\n")
        sql = sql:gsub("%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s", "\n")
        sql = sql:gsub("%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s", "\n")

        -- Replace Lua string literals with SQL quotes (preserve content)
        sql = sql:gsub("%[%[(.-)%]%]", "'%1'")
        sql = sql:gsub("%[=%[(.-)%]=%]", "'%1'")
        
        sql = sql:gsub("^\n+", "")  -- Remove leading blank lines
        -- sql = sql:gsub("\n+$", "")  -- Remove trailing blank lines

        -- Apply all placeholders in unified loop
        for key, value in pairs(cfg) do
            sql = sql:gsub("%%" .. key .. "%%", value)
        end

        return sql
    end,

    run_migration = function(self, queries, engine, design_name)
        local sql_parts = {}
        for _, q in ipairs(queries) do
            local formatted = string.format(q.sql, engine)
            table.insert(sql_parts, self:replace_query(formatted, engine, design_name))
        end
        return table.concat(sql_parts, "")
    end

}

return database
