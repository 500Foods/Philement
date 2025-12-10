# Migrations Lua Module Documentation

## Overview

The `migrations.lua` module provides comprehensive database migration functionality for the Hydrogen test suite. It handles SQL generation for multiple database engines, schema management, and migration execution across different database platforms.

## Module Information

- **Script**: `../lib/migrations.lua`
- **Language**: Lua
- **Version**: 1.0.0
- **Created**: 2025-09-14
- **Purpose**: Database migration SQL generation and engine abstraction

## Key Features

- **Multi-Engine Support**: Generates SQL for PostgreSQL, MySQL, SQLite, and DB2
- **Schema Abstraction**: Handles schema prefixes and naming conventions per engine
- **Template Processing**: Replaces placeholders with engine-specific SQL syntax
- **Migration Execution**: Processes migration query arrays into executable SQL
- **Design Pattern Support**: Works with design-specific migration files

## Supported Database Engines

### Engine Capabilities

- **PostgreSQL**: Full JSONB support, schema-based organization
- **MySQL**: JSON support, auto-increment handling
- **SQLite**: TEXT-based JSON storage, AUTOINCREMENT syntax
- **DB2**: VARCHAR-based JSON storage, uppercase schema conventions

### Engine-Specific Defaults

Each engine has customized SQL defaults for:

- **SERIAL**: Auto-increment primary keys
- **JSONB**: JSON data storage (varies by engine capabilities)
- **TIMESTAMP**: Date/time handling with/without time zones
- **SCHEMA_PREFIX**: Naming conventions for schema qualification

## Core Functions

### `migrations.replace_query(template, engine, design_name, schema_name)`

Replaces placeholders in SQL templates with engine-specific values.

**Parameters:**

- `template`: SQL template string with placeholders (e.g., `%SERIAL%`)
- `engine`: Target database engine
- `design_name`: Design identifier for schema generation
- `schema_name`: Optional custom schema name

**Process:**

1. Validates engine support
2. Generates appropriate schema prefix
3. Applies engine-specific SQL syntax replacements
4. Handles indentation and formatting cleanup

**Supported Placeholders:**

- `%SERIAL%`: Auto-increment column type
- `%INTEGER%`: Integer column type
- `%VARCHAR_100%`: VARCHAR(100) column type
- `%TEXT%`: Text column type
- `%JSONB%`: JSON data column type
- `%TIMESTAMP%`: Timestamp column type
- `%TIMESTAMP_TZ%`: Timestamp with time zone
- `%SCHEMA%`: Schema prefix for table names
- Status code constants (`%STATUS_PENDING%`, etc.)

### `migrations.run_migration(queries, engine, design_name, schema_name)`

Processes an array of migration queries into executable SQL.

**Parameters:**

- `queries`: Array of query objects with `sql` property
- `engine`: Target database engine
- `design_name`: Design identifier
- `schema_name`: Optional schema name

**Returns:** Concatenated SQL string for all queries

**Process:**

1. Iterates through query array
2. Applies `replace_query` to each SQL template
3. Concatenates results with proper separation

### `migrations.get_migration(design_name, engine, migration_num, schema_name)`

Loads and processes a specific migration file.

**Parameters:**

- `design_name`: Design name (e.g., "helium")
- `migration_num`: Migration number (e.g., "0001")
- `engine`: Target database engine
- `schema_name`: Optional schema name

**Process:**

1. Dynamically loads migration file (`{design_name}_{migration_num}.lua`)
2. Extracts queries from migration module
3. Generates engine-specific SQL using `run_migration`

## Migration File Structure

Migration files should export a `queries` array:

```lua
return {
    queries = {
        {
            sql = [[
                CREATE TABLE %SCHEMA%migrations (
                    id %SERIAL% PRIMARY KEY,
                    name VARCHAR(255) NOT NULL,
                    status %CHECK_CONSTRAINT%
                );
            ]]
        },
        {
            sql = [[
                CREATE INDEX idx_migrations_status ON %SCHEMA%migrations(status);
            ]]
        }
    }
}
```

## Integration with Test Suite

Used by database migration performance tests:

- **test_31_migrations.sh**: General migration validation
- **test_32_postgres_migrations.sh**: PostgreSQL-specific testing
- **test_33_mysql_migrations.sh**: MySQL-specific testing
- **test_34_sqlite_migrations.sh**: SQLite-specific testing
- **test_35_db2_migrations.sh**: DB2-specific testing

## Dependencies

- **Lua Runtime**: Required for script execution
- **Migration Files**: Individual migration Lua files following naming convention
- **Database Engines**: Target databases must be available for testing

## Error Handling

- Validates engine support before processing
- Handles missing migration files gracefully
- Provides meaningful error messages for invalid configurations

## Usage Examples

### Direct Module Usage

```lua
local migrations = require('migrations')

-- Generate SQL for PostgreSQL
local sql = migrations:get_migration('helium', 'postgresql', '0001', 'app')
print(sql)
```

### Template Processing

```lua
local template = "CREATE TABLE %SCHEMA%users (id %SERIAL%, name %VARCHAR_100%);"
local sql = migrations:replace_query(template, 'postgresql', 'helium', 'app')
-- Result: CREATE TABLE app.users (id SERIAL, name VARCHAR(100));
```

## Related Documentation

- [Get Migration Wrapper](/docs/H/tests/get_migration_wrapper.md) - Command-line interface for this module
- [Database Migration Tests](/docs/H/tests/test_31_migrations.md) - General migration testing
- [PostgreSQL Migration Tests](/docs/H/tests/test_32_postgres_migrations.md) - PostgreSQL-specific tests
- [MySQL Migration Tests](/docs/H/tests/test_33_mysql_migrations.md) - MySQL-specific tests
- [SQLite Migration Tests](/docs/H/tests/test_34_sqlite_migrations.md) - SQLite-specific tests
- [DB2 Migration Tests](/docs/H/tests/test_35_db2_migrations.md) - DB2-specific tests