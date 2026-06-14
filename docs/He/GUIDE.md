# Migration Creation Guide

This guide explains how to create new database migrations for the Helium system. Migrations are written in Lua and support embedding SQL, JSON, and Markdown content directly without escaping.

> **IMPORTANT FOR AI ASSISTANTS / LLMs**: If you are generating or editing migrations, read the section **"For AI / LLM Migration Generation"** below first, then study the recommended files in order. Do not rely on generic migration patterns from other frameworks. Helium uses an indirect `INSERT INTO queries` pattern with a strict state machine.

## For AI / LLM Migration Generation

**Goal**: Produce correct, complete, reversible, multi-engine Acuranzo (or other design) migrations that pass Hydrogen's test suite (especially `test_31_migrations.sh`, `test_32_postgres_migrations.sh`, `test_38_yugabytedb_migrations.sh`, and `test_71_database_diagrams.sh`).

### Mandatory Study Order (follow this sequence)

1. `docs/He/MIGRATION_ANATOMY.md` — Understand the `queries` table, the three migration types, and the state machine (1000 forward → 1003 applied → 1001 reverse). This is the most important conceptual document.
2. `docs/He/MACRO_REFERENCE.md` — Know every `${MACRO}` you are allowed to use and where they come from (database.lua + per-engine files).
3. A recent simple new-table migration, e.g. `elements/002-helium/acuranzo/migrations/acuranzo_1189.lua` (table + indexes + forward/reverse/diagram, uses `${DROP_CHECK}`, `${SUBQUERY_DELIMITER}`, `${JSON_INGEST_START/END}`, `object_ref`, `${COMMON_DIAGRAM}`).
4. A recent content-heavy or Brotli-related migration, e.g. `elements/002-helium/acuranzo/migrations/acuranzo_1159.lua` (convo_segs).
5. A per-engine special-case migration, e.g. `elements/002-helium/acuranzo/migrations/acuranzo_1190.lua` (ALTER COLUMN nullability — shows conditional blocks per engine + diagram update).
6. The bootstrap: `elements/002-helium/acuranzo/migrations/acuranzo_1000.lua` (UDFs, Base64, Brotli, timezone functions).
7. `docs/He/GUIDE.md` (this file) — templates and patterns, now that you understand the why.
8. `docs/He/QUICKSTART.md` and `docs/He/TESTING_GUIDE.md` — how to manually validate what you generate.
9. Hydrogen test scripts (especially `test_31_migrations.sh`) to understand what will be checked (sqruff, unsubstituted `${...}`, cross-engine generation).

Only after the above should you write or modify a migration.

### Non-Negotiable Rules for Every Migration You Generate

- The migration file **must** be a Lua function with exact signature: `return function(engine, design_name, schema_name, cfg) ... return queries end`
- Top of file **must** have:
  ```lua
  -- luacheck: no max line length
  -- luacheck: no unused args
  ```
- Header **must** contain a `-- CHANGELOG` with version/date/description.
- **Always** produce three query records for schema changes:
  1. Forward (`${TYPE_FORWARD_MIGRATION}` = 1000) — the real DDL/DML + an `UPDATE ... SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}` inside the embedded `code` block.
  2. Reverse (`${TYPE_REVERSE_MIGRATION}` = 1001) — undoes it + resets state back to 1000.
  3. Diagram (`${TYPE_DIAGRAM_MIGRATION}` = 1002) — JSON in `collection` using `${JSON_INGEST_START}` / `${JSON_INGEST_END}`. Include `"object_ref": "${MIGRATION}"`. For tables use `${COMMON_DIAGRAM}` for the audit columns.
- The **actual executable SQL** lives inside the `code` column as a `[=[ ... ]=]` multiline string. The outer structure is always an `INSERT INTO ${SCHEMA}${QUERIES} ...`.
- Use `${SUBQUERY_DELIMITER}` between statements inside the embedded code.
- For table drops in reverse, use `${DROP_CHECK};` before the `DROP TABLE`.
- For new tables or schema changes, set `cfg.TABLE` and `cfg.MIGRATION`.
- Use only documented macros from `MACRO_REFERENCE.md`. Never invent `${SOMETHING}`.
- Every migration must be self-contained and reversible. The reverse must be able to run cleanly after the forward (for testing).
- Rare engine differences are allowed with `if engine == 'xxx' then ... end` blocks (see acuranzo_1190.lua). Document why in the summary.
- After changes, the model (or human) must be able to run the Hydrogen migration validation tests without failures.

If you cannot follow all of the above from the source material, ask for clarification or re-read the anatomy document instead of guessing.

### Quick Canonical Checklist (use before emitting any migration)

- [ ] Correct function signature and luacheck directives
- [ ] CHANGELOG entry
- [ ] cfg.TABLE + cfg.MIGRATION set
- [ ] Forward query with state UPDATE inside the code block to TYPE_APPLIED_MIGRATION
- [ ] Reverse query that resets state to TYPE_FORWARD_MIGRATION (and uses DROP_CHECK for tables)
- [ ] Diagram query with proper JSON_INGEST wrappers, object_ref, and COMMON_DIAGRAM where appropriate
- [ ] All SQL uses only allowed macros + ${SUBQUERY_DELIMITER}
- [ ] Summary Markdown is clear and explains purpose, columns, indexes, and any engine quirks
- [ ] Tested mentally against PostgreSQL 15 / YugabyteDB semantics first (primary target)

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

### Basic Table Creation Template (Modern Canonical Form)

This is the current style used in recent Acuranzo migrations (e.g. 1189). It includes `${DROP_CHECK}` in reverse, `object_ref`, `${COMMON_DIAGRAM}`, and the full state machine.

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
            CREATE TABLE ${SCHEMA}${TABLE}
            (
                id                      ${INTEGER}          NOT NULL,
                name                    ${VARCHAR_100}      NOT NULL,
                ${COMMON_CREATE}
                ${PRIMARY}(id)
            );

            ${SUBQUERY_DELIMITER}

            CREATE INDEX ${TABLE}_idx_name
                ON ${SCHEMA}${TABLE}(name);

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                                AS code,
        'Create ${TABLE} Table'                                             AS name,
        [=[
            # Forward Migration ${MIGRATION}: Create ${TABLE} Table

            Brief description of purpose, key columns, and indexes.
        ]=]
                                                                                AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]=]}})
```

Reverse (note `${DROP_CHECK}` and state reset):

```lua
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
            ${DROP_CHECK};

            ${SUBQUERY_DELIMITER}

            DROP TABLE ${SCHEMA}${TABLE};

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                                AS code,
        'Drop ${TABLE} Table'                                               AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Drop ${TABLE} Table

            Drops the table created by the forward migration.
        ]=]
                                                                                AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]=]}})
```

Diagram (note `object_ref` and `${COMMON_DIAGRAM}`):

```lua
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
        ]=]
                                                                                AS summary,
                                                                                -- DIAGRAM_START
        ${JSON_INGEST_START}
        [=[
            {
                "diagram": [
                    {
                        "object_type": "table",
                        "object_id": "table.${TABLE}",
                        "object_ref": "${MIGRATION}",
                        "table": [
                            {
                                "name": "id",
                                "datatype": "${INTEGER}",
                                "nullable": false,
                                "primary_key": true,
                                "unique": true
                            },
                            {
                                "name": "name",
                                "datatype": "${VARCHAR_100}",
                                "nullable": false,
                                "primary_key": false,
                                "unique": false
                            },
                            ${COMMON_DIAGRAM}
                        ]
                    }
                ]
            }
        ]=]
        ${JSON_INGEST_END}
                                                                                -- DIAGRAM_END
                                                                                AS collection,
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

1. **Always include reverse migrations** for testing (and use `${DROP_CHECK}` for table drops).
2. **Use diagram migrations** for every schema change (table, significant column change). Include `object_ref` and `${COMMON_DIAGRAM}`.
3. **Test migrations** on all supported databases, with primary focus on PostgreSQL 15 / YugabyteDB (see Hydrogen `test_38_yugabytedb_migrations.sh` and `test_32_postgres_migrations.sh`).
4. **Use descriptive names** and summaries. Summaries should explain purpose, columns, indexes, and any engine-specific behavior.
5. **Include CHANGELOG** entries at the top of every migration file.
6. **Leverage macros** for database portability — never hard-code engine-specific syntax except in rare guarded `if engine == 'xxx'` blocks.
7. **Embed content directly** - no escaping needed. Use `[=[ ... ]=]` for the `code` and `summary` blocks.
8. **Put the state transition UPDATE inside the embedded `code`** for forward and reverse migrations.
9. **Use `${SUBQUERY_DELIMITER}`** between statements inside the embedded code.
10. **Keep reverse migrations safe** — for data-changing reverses, document manual prerequisites (e.g. "delete or assign passwords before reversing").

## Migration Workflow

1. Create migration file: `acuranzo_XXXX.lua` (or appropriate design prefix).
2. Implement forward migration (with state UPDATE to APPLIED inside the code block).
3. Implement reverse migration (with `${DROP_CHECK}` for tables + state reset to FORWARD).
4. Add diagram migration (if schema changes) using `object_ref` + `${COMMON_DIAGRAM}`.
5. Validate locally with `lua database.lua postgresql ...` and the Hydrogen test suite (`test_31_migrations.sh`, `test_32...`, `test_38...`).
6. Run `helium_update.sh` (or `migration_index.sh`) to update documentation.
7. Commit changes.

## File Naming

- Format: `{schema}_{number}.lua`
- Numbers: 1000+ for schema migrations
- Examples: `acuranzo_1000.lua`, `helium_4000.lua`

## Testing

Migrations are tested by:

1. Running forward migration (state becomes 1003 / applied).
2. Running reverse migration (state returns to 1000 / forward).
3. Verifying data integrity and that reverse truly undoes the change.
4. Checking on all supported databases, with primary emphasis on PostgreSQL 15 / YugabyteDB.

The system (via the `queries` table state machine and Hydrogen tests) ensures forward and reverse migrations are complete and reversible. See `docs/He/TESTING_GUIDE.md` and the Hydrogen tests `test_31_migrations.sh`, `test_32_postgres_migrations.sh`, `test_38_yugabytedb_migrations.sh`, and `test_71_database_diagrams.sh`.

### Primary Target Reminder

All active development targets **PostgreSQL 15 semantics** (exercised primarily through YugabyteDB). Do not rely on PostgreSQL 16+ features. Validate that your generated SQL is compatible with PG15 / YugabyteDB first.