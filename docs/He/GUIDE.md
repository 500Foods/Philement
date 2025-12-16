# Migration Creation Guide

This guide explains how to create new database migrations for the Helium system. Migrations are written in Lua and support embedding SQL, JSON, and Markdown content directly without escaping.

## Lua Basics for Migrations

Before diving into migration creation, let's cover the essential Lua concepts you'll need:

### Basic Syntax

```lua
-- Variables
local name = "example"
local version = 1.0

-- Functions
local function create_table()
    -- code here
end

-- Tables (Lua's primary data structure, similar to arrays/objects)
local queries = {}
local config = {
    table_name = "users",
    version = 1000
}
```

### Multiline Strings

Lua's `[=[...]=]` syntax is perfect for embedding SQL without escaping:

```lua
local sql = [=[
    CREATE TABLE users (
        id INTEGER PRIMARY KEY,
        name TEXT NOT NULL,
        email TEXT UNIQUE
    );
]=]

-- You can nest them for complex content
local complex_sql = [=[
    INSERT INTO users VALUES (
        1,
        'John Doe',
        [=[
            SELECT email FROM contacts
            WHERE active = 1
        ]=]
    );
]=]
```

### Key Concepts for Migrations

- **Functions**: Migrations are Lua functions that return a table of queries
- **Tables**: Used to store query objects and configuration
- **Strings**: Handle SQL, JSON, and other content
- **Local variables**: Keep your code clean and avoid global pollution

For a more comprehensive introduction to Lua, see our [Lua Introduction](/docs/He/LUA_INTRO.md).

## Migration Structure

Each migration file follows this pattern:

```lua
-- Migration: schema_migration_number.lua
-- Brief description

-- CHANGELOG
-- version - date - description

return function(engine, design_name, schema_name, cfg)
    local queries = {}

    cfg.TABLE = "target_table_name"
    cfg.MIGRATION = "migration_number"

    -- Migration content here

    return queries
end
```

## Migration Types

### Forward Migration

Creates or modifies database objects. Always required.

```lua
table.insert(queries,{sql=[[
    INSERT INTO ${SCHEMA}${QUERIES} (
        ${QUERIES_INSERT}
    )
    WITH next_query_id AS (
        SELECT COALESCE(MAX(query_id), 0) + 1 AS new_query_id
        FROM ${SCHEMA}${QUERIES}
    )
    SELECT
        new_query_id,
        ${MIGRATION},
        ${STATUS_ACTIVE},
        ${TYPE_FORWARD_MIGRATION},
        ${DIALECT},
        ${QTC_SLOW},
        ${TIMEOUT},
        [=[
            -- SQL content here
            CREATE TABLE ${SCHEMA}${TABLE} (
                id ${INTEGER} PRIMARY KEY,
                name ${VARCHAR_100}
            );
        ]=],
        'Create ${TABLE} table',
        [=[
            # Forward Migration ${MIGRATION}

            Creates the ${TABLE} table for storing data.
        ]=],
        '{}',
        ${COMMON_INSERT}
    FROM next_query_id;
]]})
```

### Reverse Migration

Undoes the forward migration. Used for testing and rollback.

```lua
table.insert(queries,{sql=[[
    -- Similar structure but with TYPE_REVERSE_MIGRATION
    -- Contains DROP TABLE or DELETE statements
]]})
```

### Diagram Migration

Provides JSON metadata for Entity-Relationship Diagrams (ERD).

```lua
table.insert(queries,{sql=[[
    -- Uses TYPE_DIAGRAM_MIGRATION
    -- Contains JSON table definitions for ERD generation
]]})
```

## Embedding Content

### SQL

SQL statements can be embedded directly using multiline strings:

```lua
[=[
    CREATE TABLE ${SCHEMA}${TABLE} (
        id ${INTEGER} PRIMARY KEY,
        name ${VARCHAR_100} NOT NULL,
        created_at ${TIMESTAMP_TZ} DEFAULT ${NOW}
    );

    CREATE INDEX idx_name ON ${SCHEMA}${TABLE} (name);
]=]
```

### JSON

JSON content is embedded unmodified:

```lua
[=[
    {
        "diagram": [
            {
                "object_type": "table",
                "object_id": "table.${TABLE}",
                "table": [
                    {
                        "name": "id",
                        "datatype": "${INTEGER}",
                        "nullable": false,
                        "primary_key": true
                    }
                ]
            }
        ]
    }
]=]
```

### Markdown

Markdown documentation can be included directly:

```lua
[=[
    # Migration ${MIGRATION}: Create ${TABLE} Table

    This migration creates the ${TABLE} table with the following columns:

    - `id`: Primary key
    - `name`: Table name
    - `created_at`: Creation timestamp

    ## Notes

    - This table stores configuration data
    - Indexes are created for performance
]=]
```

### CSS (Advanced Example)

Complex content like CSS can also be embedded:

```lua
[=[
    .Theme-Custom {
        --ACZ-color-1: rgba(0, 0, 0, 0.8);
        --ACZ-font-family: "Arial";
    }

    .Theme-Custom body {
        background: white;
    }
]=]
```

## Macro System

Macros are placeholders in your SQL that get automatically replaced with database-specific values. Think of them as variables that adapt your migration to work on different database engines.

### Simple Example

```lua
-- Without macros (not recommended)
local sql = [[
    CREATE TABLE users (
        id INTEGER PRIMARY KEY,
        name VARCHAR(100) NOT NULL
    );
]]

-- With macros (works on all databases)
local sql = [[
    CREATE TABLE ${SCHEMA}${TABLE} (
        id ${INTEGER} PRIMARY KEY,
        name ${VARCHAR_100} NOT NULL
    );
]]
```

The `${SCHEMA}` macro becomes the correct schema name for each database, `${TABLE}` uses your configured table name, and `${INTEGER}` becomes the right integer type for that database.

### Built-in Macros

- **Data Types**: `${INTEGER}`, `${VARCHAR_100}`, `${TIMESTAMP_TZ}`
- **Functions**: `${NOW}`, `${SCHEMA}`, `${TABLE}`
- **Query Types**: `${TYPE_FORWARD_MIGRATION}`, `${TYPE_REVERSE_MIGRATION}`
- **Status**: `${STATUS_ACTIVE}`, `${STATUS_INACTIVE}`
- **Queues**: `${QTC_SLOW}`, `${QTC_MEDIUM}`, `${QTC_FAST}`

### Custom Configuration

Set migration-specific variables:

```lua
cfg.TABLE = "my_table"
cfg.MIGRATION = "1234"
cfg.LOOKUP_ID = "CUSTOM"
cfg.CUSTOM_VALUE = "example"
```

## Beginner Example Migration

Here's a complete, simple migration to create a `users` table. This example is simpler than the complex production migrations and focuses on the basic pattern.

### Complete Migration File: acuranzo_9998.lua

```lua
-- Migration: acuranzo_9998.lua
-- Creates a simple users table for beginners

-- luacheck: no max line length
-- luacheck: no unused args

return function(engine, design_name, schema_name, cfg)
    local queries = {}

    cfg.TABLE = "users"
    cfg.MIGRATION = "9998"

    -- Forward Migration
    table.insert(queries,{sql=[=[

        INSERT INTO ${SCHEMA}${QUERIES} (
            ${QUERIES_INSERT}
        )
        WITH next_query_id AS (
            SELECT COALESCE(MAX(query_id), 0) + 1 AS new_query_id
            FROM ${SCHEMA}${QUERIES}
        )
        SELECT
            new_query_id                                                        AS query_id,
            ${MIGRATION}                                                        AS query_ref,
            ${STATUS_ACTIVE}                                                    AS query_status_a27,
            ${TYPE_FORWARD_MIGRATION}                                           AS query_type_a28,
            ${DIALECT}                                                          AS query_dialect_a30,
            ${QTC_SLOW}                                                         AS query_queue_a58,
            ${TIMEOUT}                                                          AS query_timeout,
            [=[
                CREATE TABLE ${SCHEMA}${TABLE} (
                    user_id     ${INTEGER}      PRIMARY KEY,
                    username    ${VARCHAR_50}   NOT NULL UNIQUE,
                    email       ${VARCHAR_100}  NOT NULL,
                    ${COMMON_CREATE}
                );

                ${SUBQUERY_DELIMITER}

                UPDATE ${SCHEMA}${QUERIES}
                  SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
                WHERE query_ref = ${MIGRATION}
                  AND query_type_a28 = ${TYPE_FORWARD_MIGRATION};
            ]=]
                                                                                AS code,
            'Create ${TABLE} table'                                            AS name,
            [=[
                # Migration ${MIGRATION}: Create Users Table

                Creates a basic users table with id, username, and email fields.
                This is a beginner-friendly example migration.
            ]=]
                                                                                AS summary,
            '{}'                                                                AS collection,
            ${COMMON_INSERT}
        FROM next_query_id;

    ]=]}})

    -- Reverse Migration
    table.insert(queries,{sql=[=[

        INSERT INTO ${SCHEMA}${QUERIES} (
            ${QUERIES_INSERT}
        )
        WITH next_query_id AS (
            SELECT COALESCE(MAX(query_id), 0) + 1 AS new_query_id
            FROM ${SCHEMA}${QUERIES}
        )
        SELECT
            new_query_id                                                        AS query_id,
            ${MIGRATION}                                                        AS query_ref,
            ${STATUS_ACTIVE}                                                    AS query_status_a27,
            ${TYPE_REVERSE_MIGRATION}                                           AS query_type_a28,
            ${DIALECT}                                                          AS query_dialect_a30,
            ${QTC_SLOW}                                                         AS query_queue_a58,
            ${TIMEOUT}                                                          AS query_timeout,
            [=[
                DROP TABLE ${SCHEMA}${TABLE};

                ${SUBQUERY_DELIMITER}

                UPDATE ${SCHEMA}${QUERIES}
                  SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
                WHERE query_ref = ${MIGRATION}
                  AND query_type_a28 = ${TYPE_APPLIED_MIGRATION};
            ]=]
                                                                                AS code,
            'Drop ${TABLE} table'                                               AS name,
            [=[
                # Reverse Migration ${MIGRATION}: Drop Users Table

                Removes the users table created by the forward migration.
            ]=]
                                                                                AS summary,
            '{}'                                                                AS collection,
            ${COMMON_INSERT}
        FROM next_query_id;

    ]=]}})

    return queries
end
```

This migration creates a simple users table and demonstrates the complete forward/reverse pattern. Copy this structure and modify the table definition for your needs.

## Examples from Existing Migrations

### Table Creation (Migration 1000)

Creates the core `queries` table with all standard columns.

### Data Insertion (Migration 1024)

Inserts administrator account data using environment variables:

```lua
INSERT INTO ${SCHEMA}${TABLE} VALUES (
    0, 1, 0, '${HYDROGEN_USERNAME}', '', '', '${HYDROGEN_USERNAME}',
    '${HYDROGEN_PASSHASH}', 'Administrative Account', '{}',
    ${COMMON_VALUES}
);
```

### Lookup Data (Migration 1069)

Inserts theme CSS data into the lookups table with complex multiline CSS content.

## Common Patterns

### Table Creation Pattern

Most migrations create new tables. The pattern includes:

1. Define table structure with macros for data types
2. Include `${COMMON_CREATE}` for audit fields
3. Use appropriate constraints (PRIMARY KEY, UNIQUE, etc.)
4. Update migration state to applied

### Data Insertion Pattern

For inserting reference or initial data:

1. Use INSERT with explicit column lists
2. Include `${COMMON_FIELDS}` and `${COMMON_VALUES}` for audit data
3. Consider foreign key relationships
4. Ensure reverse migration deletes the same data

### Schema Modification Pattern

When altering existing tables:

1. Use ALTER TABLE statements
2. Consider data migration if changing column types
3. Ensure reverse migration undoes the change
4. Update diagram migration for schema changes

### Lookup Table Pattern

For status and reference tables:

1. Include status fields with lookup references
2. Add appropriate indexes
3. Insert initial lookup values
4. Use diagram migration with `"lookup": true` for status fields

## Copy/Paste Templates

### Basic Table Creation Template

```lua
-- Forward Migration
table.insert(queries,{sql=[=[

    INSERT INTO ${SCHEMA}${QUERIES} (
        ${QUERIES_INSERT}
    )
    WITH next_query_id AS (
        SELECT COALESCE(MAX(query_id), 0) + 1 AS new_query_id
        FROM ${SCHEMA}${QUERIES}
    )
    SELECT
        new_query_id                                                        AS query_id,
        ${MIGRATION}                                                        AS query_ref,
        ${STATUS_ACTIVE}                                                    AS query_status_a27,
        ${TYPE_FORWARD_MIGRATION}                                           AS query_type_a28,
        ${DIALECT}                                                          AS query_dialect_a30,
        ${QTC_SLOW}                                                         AS query_queue_a58,
        ${TIMEOUT}                                                          AS query_timeout,
        [=[
            CREATE TABLE ${SCHEMA}${TABLE} (
                id          ${INTEGER}      PRIMARY KEY,
                name        ${VARCHAR_100}  NOT NULL,
                ${COMMON_CREATE}
            );

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              AND query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                                AS code,
        'Create ${TABLE} table'                                            AS name,
        [=[
            # Migration ${MIGRATION}: Create ${TABLE} Table

            Description of what this table stores and why it was created.
        ]=]
                                                                                AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]=]}})
```

### Reverse Migration Template

```lua
-- Reverse Migration
table.insert(queries,{sql=[=[

    INSERT INTO ${SCHEMA}${QUERIES} (
        ${QUERIES_INSERT}
    )
    WITH next_query_id AS (
        SELECT COALESCE(MAX(query_id), 0) + 1 AS new_query_id
        FROM ${SCHEMA}${QUERIES}
    )
    SELECT
        new_query_id                                                        AS query_id,
        ${MIGRATION}                                                        AS query_ref,
        ${STATUS_ACTIVE}                                                    AS query_status_a27,
        ${TYPE_REVERSE_MIGRATION}                                           AS query_type_a28,
        ${DIALECT}                                                          AS query_dialect_a30,
        ${QTC_SLOW}                                                         AS query_queue_a58,
        ${TIMEOUT}                                                          AS query_timeout,
        [=[
            DROP TABLE ${SCHEMA}${TABLE};

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              AND query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                                AS code,
        'Drop ${TABLE} table'                                               AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Drop ${TABLE} Table

            Undoes the forward migration by removing the table.
        ]=]
                                                                                AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]=]}})
```

### Data Insertion Template

```lua
-- Forward Migration with Data
table.insert(queries,{sql=[=[

    INSERT INTO ${SCHEMA}${QUERIES} (
        ${QUERIES_INSERT}
    )
    WITH next_query_id AS (
        SELECT COALESCE(MAX(query_id), 0) + 1 AS new_query_id
        FROM ${SCHEMA}${QUERIES}
    )
    SELECT
        new_query_id                                                        AS query_id,
        ${MIGRATION}                                                        AS query_ref,
        ${STATUS_ACTIVE}                                                    AS query_status_a27,
        ${TYPE_FORWARD_MIGRATION}                                           AS query_type_a28,
        ${DIALECT}                                                          AS query_dialect_a30,
        ${QTC_SLOW}                                                         AS query_queue_a58,
        ${TIMEOUT}                                                          AS query_timeout,
        [=[
            INSERT INTO ${SCHEMA}${TABLE} (
                id, name, ${COMMON_FIELDS}
            ) VALUES (
                1, 'Example Item', ${COMMON_VALUES}
            );

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              AND query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                                AS code,
        'Insert initial ${TABLE} data'                                      AS name,
        [=[
            # Migration ${MIGRATION}: Insert Initial Data

            Adds initial data to the ${TABLE} table.
        ]=]
                                                                                AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]=]}})
```

### Diagram Migration Template

```lua
-- Diagram Migration
table.insert(queries,{sql=[=[

    INSERT INTO ${SCHEMA}${QUERIES} (
        ${QUERIES_INSERT}
    )
    WITH next_query_id AS (
        SELECT COALESCE(MAX(query_id), 0) + 1 AS new_query_id
        FROM ${SCHEMA}${QUERIES}
    )
    SELECT
        new_query_id                                                        AS query_id,
        ${MIGRATION}                                                        AS query_ref,
        ${STATUS_ACTIVE}                                                    AS query_status_a27,
        ${TYPE_DIAGRAM_MIGRATION}                                           AS query_type_a28,
        ${DIALECT}                                                          AS query_dialect_a30,
        ${QTC_SLOW}                                                         AS query_queue_a58,
        ${TIMEOUT}                                                          AS query_timeout,
        'JSON Table Definition in collection'                               AS code,
        'Diagram Tables: ${SCHEMA}${TABLE}'                                 AS name,
        [=[
            # Diagram Migration ${MIGRATION}

            ## Diagram Tables: ${SCHEMA}${TABLE}

            Provides ERD metadata for the ${TABLE} table.
        ]=]
                                                                                AS summary,
        ${JSON_INGEST_START}
        [=[
            {
                "diagram": [
                    {
                        "object_type": "table",
                        "object_id": "table.${TABLE}",
                        "table": [
                            {
                                "name": "id",
                                "datatype": "${INTEGER}",
                                "nullable": false,
                                "primary_key": true
                            }
                        ]
                    }
                ]
            }
        ]=]
        ${JSON_INGEST_END}
                                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]=]}})
```

These templates provide a starting point for common migration patterns. Copy, modify, and test thoroughly before using in production.

## Best Practices

1. **Always include reverse migrations** for testing
2. **Use diagram migrations** for schema changes
3. **Test migrations** on all supported databases
4. **Use descriptive names** and summaries
5. **Include CHANGELOG** entries
6. **Leverage macros** for database portability
7. **Embed content directly** - no escaping needed

## Migration Workflow

1. Create migration file: `schema_XXXX.lua`
2. Implement forward migration
3. Implement reverse migration
4. Add diagram migration (if schema changes)
5. Test on all database engines
6. Run `migration_index.sh` to update documentation
7. Commit changes

## File Naming

- Format: `{schema}_{number}.lua`
- Numbers: 1000+ for schema migrations
- Examples: `acuranzo_1000.lua`, `helium_4000.lua`

## Testing

Migrations are tested by:

1. Running forward migration
2. Running reverse migration
3. Verifying data integrity
4. Checking on all supported databases

The system ensures forward and reverse migrations are complete and reversible.