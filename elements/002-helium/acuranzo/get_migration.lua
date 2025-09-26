-- Command line usage: lua get_migration.lua <database> <design> <migration>
-- Example: lua get_migration.lua postgresql acuranzo app 1000

local database = require 'database'

-- Get command line arguments
local db_engine = arg[1] or 'postgresql'
local design_name = arg[2] or 'acuranzo'
local schema_name = arg[3] or 'app'
local migration_num = arg[4] or '1000'

-- Load the migration file dynamically
local migration_file = design_name .. '_' .. migration_num
local migration = require(migration_file)

-- Generate the SQL
local sql = database:run_migration(migration.queries, db_engine, design_name, schema_name)

print(sql)
