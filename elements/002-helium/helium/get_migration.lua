-- Command line usage: lua get_migration.lua <database> <design> <migration>
-- Example: lua get_migration.lua postgresql helium 0000

local database = require 'database'

-- Get command line arguments
local db_engine = arg[1] or 'postgresql'
local design_name = arg[2] or 'helium'
local migration_num = arg[3] or '0000'

-- Load the migration file dynamically
local migration_file = design_name .. '_' .. migration_num
local migration = require(migration_file)

-- print("Generated SQL for " .. db_engine .. " (" .. design_name .. " design, migration " .. migration_num .. "):")
local sql = database:run_migration(migration.queries, db_engine, design_name)

-- Clean up the output - only remove the first character if it's a space
-- if sql:sub(1, 1) == " " then
--    sql = sql:sub(3)  -- Remove only the first character if it's a space
--end

-- Remove blank lines only at the very beginning and end
-- sql = sql:gsub("^\n+", "")  -- Remove leading blank lines
-- sql = sql:gsub("\n+$", "")  -- Remove trailing blank lines

print(sql)
