-- get_migration_wrapper.lua
-- Command line wrapper for database.lua from design folders
-- Usage: lua get_migration.lua <engine> <design> <schema> <migration>

-- Get command line arguments
local engine = arg[1] or 'postgresql'
local design_name = arg[2] or 'helium'
local schema_name = arg[3] or nil
local migration_num = arg[4] or '0000'

-- Load the design's database module
local database = require('database')

-- Load the migration file
local migration_file = design_name .. '_' .. migration_num
local migration = require(migration_file)

-- Generate SQL using the design's database
local sql = database:run_migration(migration.queries, engine, design_name, schema_name)

-- Output the SQL
print(sql)