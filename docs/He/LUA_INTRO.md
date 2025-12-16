# Lua Introduction for Helium Migrations

This guide introduces Lua programming concepts specifically needed for creating database migrations in the Helium system. Lua is a lightweight scripting language that's perfect for this use case.

## Why Lua for Migrations?

- **Simple syntax**: Easy to learn and read
- **Excellent string handling**: Perfect for SQL templating
- **Lightweight**: Minimal runtime overhead
- **Embeddable**: Integrates well with larger systems
- **No complex types**: Focus on the essentials

## Basic Concepts

### Variables

```lua
-- Local variables (recommended)
local name = "users_table"
local version = 1000
local is_active = true

-- Numbers
local migration_number = 1024
local priority = 1.5

-- Don't use global variables
-- WRONG: global_var = "avoid this"
```

### Functions

```lua
-- Define a function
local function create_migration_table()
    -- function body
    return "table created"
end

-- Call a function
local result = create_migration_table()

-- Anonymous functions (used in migrations)
return function(engine, design_name, schema_name, cfg)
    -- migration code here
    local queries = {}
    return queries
end
```

### Tables

Tables are Lua's primary data structure (like arrays/objects in other languages):

```lua
-- Empty table
local queries = {}

-- Array-like table
local colors = {"red", "green", "blue"}

-- Object-like table
local config = {
    table_name = "users",
    version = 1000,
    active = true
}

-- Mixed table
local migration = {
    id = 1024,
    queries = {},
    metadata = {
        author = "developer",
        date = "2025-01-01"
    }
}

-- Adding to tables
table.insert(queries, {sql = "SELECT 1"})
config.new_field = "value"
```

## Strings

### Basic Strings

```lua
local name = "users"
local sql = "SELECT * FROM " .. name  -- Concatenation
```

### Multiline Strings (Crucial for Migrations)

```lua
-- Basic multiline
local sql = [[
    CREATE TABLE users (
        id INTEGER PRIMARY KEY,
        name TEXT
    );
]]

-- Nested multiline strings (for complex SQL)
local complex_sql = [[
    INSERT INTO users VALUES (
        1,
        'John',
        [[
            SELECT email FROM contacts
            WHERE user_id = 1
        ]]
    );
]]
```

### Long String Syntax `[=[...]=]`

For migrations, we use `[=[...]=]` to avoid conflicts:

```lua
local migration_sql = [=[
    CREATE TABLE ${SCHEMA}${TABLE} (
        id ${INTEGER} PRIMARY KEY,
        name ${VARCHAR_100} NOT NULL,
        created_at ${TIMESTAMP_TZ} DEFAULT ${NOW}
    );

    CREATE INDEX idx_name ON ${SCHEMA}${TABLE} (name);
]=]
```

## Control Structures

### Conditionals

```lua
if engine == "postgresql" then
    -- PostgreSQL specific code
elseif engine == "mysql" then
    -- MySQL specific code
else
    -- Default case
end
```

### Loops

```lua
-- For loop
for i = 1, 10 do
    table.insert(queries, {sql = "SELECT " .. i})
end

-- While loop (less common)
local i = 1
while i <= 10 do
    -- do something
    i = i + 1
end

-- Iterate over table
for index, value in ipairs(colors) do
    print(index, value)
end
```

## Migration-Specific Patterns

### Standard Migration Structure

```lua
-- Migration: schema_migration_number.lua
-- Brief description

return function(engine, design_name, schema_name, cfg)
    local queries = {}

    -- Set configuration
    cfg.TABLE = "target_table_name"
    cfg.MIGRATION = "migration_number"

    -- Add queries
    table.insert(queries, {
        sql = [=[
            CREATE TABLE ${SCHEMA}${TABLE} (
                id ${INTEGER} PRIMARY KEY
            );
        ]=],
        description = "Create table"
    })

    return queries
end
```

### Common Table Operations

```lua
-- Adding queries
table.insert(queries, {sql = sql_string, description = "What this does"})

-- Building complex SQL
local sql_parts = {}
table.insert(sql_parts, "CREATE TABLE users (")
table.insert(sql_parts, "id INTEGER PRIMARY KEY,")
table.insert(sql_parts, "name TEXT")
table.insert(sql_parts, ")")
local full_sql = table.concat(sql_parts, "\n")
```

## Error Handling

```lua
-- Basic error checking
if not cfg.TABLE then
    error("TABLE configuration is required")
end

-- Safe function calls
local success, result = pcall(some_function)
if not success then
    print("Error:", result)
end
```

## Best Practices

1. **Use local variables**: Avoid global pollution
2. **Consistent naming**: Use snake_case for variables
3. **Comments**: Document complex logic
4. **Table.insert**: Use for building query arrays
5. **Multiline strings**: Use `[=[...]=]` for SQL content
6. **Configuration**: Set cfg variables at the start
7. **Return queries**: Always return the queries table

## Common Pitfalls

- **Global variables**: Don't create them accidentally
- **String concatenation**: Use `..` for simple strings, `table.concat` for arrays
- **Table indexing**: Lua tables start at 1, not 0
- **Nil values**: Check for nil before using variables
- **String escaping**: Use multiline strings to avoid escaping quotes

## Learning Resources

- [Lua in 15 Minutes](https://tylerneylon.com/a/learn-lua/) - Quick start guide
- [Official Lua Manual](https://www.lua.org/manual/5.1/) - Complete reference
- [Programming in Lua](https://www.lua.org/pil/) - Free online book

## Testing Your Lua Code

```bash
# Run a Lua file
lua your_migration.lua

# Interactive Lua (for testing)
lua -i
> local t = {1, 2, 3}
> print(t[1])  -- prints 1
```

Remember: Lua is designed to be simple. Most migration code follows the same patterns, so studying existing migrations is the best way to learn!