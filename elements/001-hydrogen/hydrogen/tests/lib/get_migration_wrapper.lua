-- get_migration_wrapper.lua
-- Command line wrapper for migrations.lua
-- Usage: lua get_migration_wrapper.lua <design> <engine> <migration>

local migrations = require 'migrations'

-- Get command line arguments
local design_name = arg[1] or 'helium'
local engine = arg[2] or 'postgresql'
local migration_num = arg[3] or '0000'

-- Get migration SQL
local sql = migrations.get_migration(design_name, engine, migration_num)

-- Output the SQL
print(sql)