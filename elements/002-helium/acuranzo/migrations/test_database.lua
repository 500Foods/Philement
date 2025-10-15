#!/usr/bin/env lua
-- test_database.lua
-- Simple test to verify the refactored database module loads correctly

print("Testing database module...")

-- Adjust package path to find modules from project root
package.path = package.path .. ";../../../../?.lua"
package.path = package.path .. ";./?.lua"

-- Load the database module
local success, database = pcall(require, "database")

if not success then
    print("ERROR: Failed to load database module")
    print(database)
    os.exit(1)
end

print("✓ Database module loaded successfully")

-- Test that all engines are present
local engines = {"sqlite", "postgresql", "mysql", "db2"}
for _, engine in ipairs(engines) do
    if not database.defaults[engine] then
        print("ERROR: Missing defaults for engine: " .. engine)
        os.exit(1)
    end
    print("✓ " .. engine .. " configuration loaded")
    
    -- Verify some key fields exist
    local config = database.defaults[engine]
    if not config.SERIAL or not config.JSONB or not config.JSON_INGEST_FUNCTION then
        print("ERROR: Missing required fields in " .. engine .. " configuration")
        os.exit(1)
    end
end

print("\n✓ All tests passed!")
print("Database module refactoring successful!")