# Get Migration Wrapper Lua Script Documentation

## Overview

The `get_migration_wrapper.lua` script provides a command-line interface for generating database migration SQL from design-specific migration files. It serves as a bridge between the Hydrogen project's database abstraction layer and the migration system.

## Script Information

- **Script**: `../lib/get_migration_wrapper.lua`
- **Language**: Lua
- **Purpose**: Command-line wrapper for database migration SQL generation
- **Dependencies**: Lua runtime, database.lua module

## Usage

```bash
lua get_migration_wrapper.lua <engine> <design> <schema> <migration>
```

### Parameters

- `engine`: Database engine (default: 'postgresql')
  - Supported: 'postgresql', 'mysql', 'sqlite', 'db2'
- `design`: Design name (default: 'helium')
- `schema`: Schema name (optional)
- `migration`: Migration number (default: '0000')

## Functionality

### Process Flow

1. **Argument Parsing**: Reads command-line arguments with defaults
2. **Module Loading**: Requires the `database.lua` module for the specified design
3. **Migration Loading**: Loads the specific migration file based on design and migration number
4. **SQL Generation**: Calls `database:run_migration()` to generate engine-specific SQL
5. **Output**: Prints the generated SQL to stdout

### Migration File Naming

Migration files are loaded using the pattern: `{design_name}_{migration_num}.lua`

Example: `helium_0001.lua` for design "helium" migration "0001"

## Integration with Test Suite

This script is used by database migration tests (test_31_migrations.sh, test_32_postgres_migrations.sh, etc.) to:

- Generate migration SQL for different database engines
- Validate migration syntax and structure
- Test migration performance across databases
- Ensure cross-engine compatibility

## Dependencies

- **Lua Runtime**: Required for script execution
- **database.lua**: Design-specific database abstraction module
- **Migration Files**: Individual migration Lua files following naming convention

## Error Handling

- Uses default values for missing arguments
- Relies on Lua's error handling for module loading failures
- SQL generation errors are handled by the database module

## Usage Examples

### PostgreSQL Migration

```bash
lua get_migration_wrapper.lua postgresql helium public 0001
```

### MySQL Migration with Schema

```bash
lua get_migration_wrapper.lua mysql lithium myapp 0005
```

### Default Parameters

```bash
lua get_migration_wrapper.lua
# Uses: postgresql, helium, nil, 0000
```

## Related Documentation

- [Database Migration Tests](/docs/H/tests/test_31_migrations.md) - How this script is used in testing
- [PostgreSQL Migration Tests](/docs/H/tests/test_32_postgres_migrations.md) - PostgreSQL-specific usage
- [MySQL Migration Tests](/docs/H/tests/test_33_mysql_migrations.md) - MySQL-specific usage
- [SQLite Migration Tests](/docs/H/tests/test_34_sqlite_migrations.md) - SQLite-specific usage
- [DB2 Migration Tests](/docs/H/tests/test_35_db2_migrations.md) - DB2-specific usage