-- migrations.lua
-- Combined database and migration functionality for Hydrogen test suite
-- Provides SQL generation for database migrations with support for multiple engines

-- CHANGELOG
-- 1.0.0 - 2025-09-14 - Initial creation combining database.lua and get_migration.lua

local migrations = {}

-- Database schema and engine definitions
migrations.schema = "helium"

-- Currently supported database engines
migrations.engines = {
    sqlite = true,
    postgresql = true,
    mysql = true,
    db2 = true
}

-- Lookup for dialects
migrations.dialects = {
    sql = 0,
    postgresql = 1,
    sqlite = 2,
    mysql = 3,
    db2 = 4
}

-- Status codes
migrations.status_codes = {
    Pending = 1,
    Applied = 2,
    Utility = 3
}

-- SQL defaults for different database engines
migrations.defaults = {
    sqlite = {
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
}

-- Replace query placeholders with engine-specific values
function migrations.replace_query(template, engine, design_name)
    if not migrations.engines[engine] then
        error("Unsupported engine: " .. engine)
    end

    local cfg = migrations.defaults[engine]

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
    cfg.DIALECT = migrations.dialects[engine]
    cfg.STATUS_UTILITY = migrations.status_codes.Utility
    cfg.STATUS_PENDING = migrations.status_codes.Pending
    cfg.STATUS_APPLIED = migrations.status_codes.Applied

    local sql = template

    -- Strip indentation due to Lua formatting only
    sql = sql:gsub("%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s", "\n")
    sql = sql:gsub("\n%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s", "\n")
    sql = sql:gsub("%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s", "\n")
    sql = sql:gsub("%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s", "\n")

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
end

-- Run migration queries for a specific engine and design
function migrations.run_migration(queries, engine, design_name)
    local sql_parts = {}
    for _, q in ipairs(queries) do
        local formatted = string.format(q.sql, engine)
        table.insert(sql_parts, migrations:replace_query(formatted, engine, design_name))
    end
    return table.concat(sql_parts, "")
end

-- Get migration SQL for specific design, engine, and migration number
function migrations.get_migration(design_name, engine, migration_num)
    -- Load the migration file dynamically
    local migration_file = design_name .. '_' .. migration_num
    local migration = require(migration_file)

    -- Generate SQL for this engine
    local sql = migrations:run_migration(migration.queries, engine, design_name)

    return sql
end

return migrations