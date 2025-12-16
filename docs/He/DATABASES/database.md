# database.lua

## Overview

`database.lua` is the core Lua module that defines the Helium database schema framework, supported database engines, and provides comprehensive SQL processing capabilities. It serves as the foundation for all migration operations in the Helium system.

## Architecture

The module is structured as a Lua table with the following key components:

### Engine Support

Defines supported database engines and their configurations:

- PostgreSQL
- MySQL/MariaDB  
- SQLite
- IBM DB2

### Query Types & Status

Lookup tables for query classification:

- Query statuses (active/inactive)
- Query types (forward migration, reverse migration, diagram migration, etc.)
- Query queues (slow, medium, fast, cached)

### SQL Templates

Predefined SQL fragments for common operations:

- `queries_insert`: Standard INSERT statement columns
- `common_insert`: Default values for auditing fields
- `common_create`: DDL for standard table columns
- `common_diagram`: JSON structure for ERD generation

## Core Functions

### replace_query()

The primary function that transforms template SQL into engine-specific SQL:

**Parameters:**

- `template`: SQL template string with macros
- `engine`: Target database engine ('postgresql', 'mysql', 'sqlite', 'db2')
- `design_name`: Schema design identifier
- `schema_name`: Database schema name

**Functionality:**

1. **Macro Expansion**: Replaces `${MACRO}` placeholders with engine-specific values
2. **Nested Macros**: Supports up to 5 levels of macro nesting
3. **Environment Variables**: Substitutes runtime environment variables
4. **Multiline String Processing**: Handles `[=[...]=]` blocks for large content
5. **Base64 Encoding**: Converts multiline content to Base64 with optional compression
6. **Engine-Specific Wrappers**: Applies database-specific functions (CONVERT_FROM for PostgreSQL, etc.)

### indent_sql()

Formats and indents SQL statements for readability:

- Preserves multiline string markers
- Handles nested brackets and parentheses
- Maintains consistent indentation
- Supports comment alignment

### run_migration()

Processes an array of query objects into formatted SQL:

- Applies `replace_query()` to each query
- Applies `indent_sql()` for formatting
- Joins with query delimiters

## Macro System

### Built-in Macros

- **Data Types**: `${INTEGER}`, `${TEXT}`, `${TIMESTAMP_TZ}`, etc.
- **Functions**: `${NOW}`, `${SCHEMA}`, `${TABLE}`
- **Query Elements**: `${QUERIES_INSERT}`, `${COMMON_CREATE}`
- **Lookup Values**: `${STATUS_ACTIVE}`, `${TYPE_FORWARD_MIGRATION}`
- **Delimiters**: `${SUBQUERY_DELIMITER}`, `${QUERY_DELIMITER}`

### Engine-Specific Macros

Loaded from separate engine files:

- PostgreSQL: `CONVERT_FROM(DECODE(...))` for Base64
- MySQL: Custom UDF calls
- SQLite: Limited functionality (no native Base64)
- DB2: Custom UDF calls with chunking

## Multiline String Processing

### Base64 Encoding

Large content blocks within `[=[...]=]` are:

1. Indentation-stripped
2. Base64-encoded
3. Optionally Brotli-compressed (>1KB threshold)
4. Wrapped in database-specific decode functions

### Nested Strings

Supports multiple nesting levels:

- `[=[level 1]=]`
- `[==[level 2]==]`
- `[===[level 3]===]`

## Compression

### Brotli Integration

- Requires `lua-brotli` library
- Applied to strings >1024 bytes
- Uses maximum compression (level 11)
- Database-specific decompression UDFs required

See [BROTLI_COMPRESSION.md](/docs/He/BROTLI_COMPRESSION.md) for detailed information about Brotli compression in migrations.

## Error Handling

- Validates engine support
- Checks for required libraries
- Provides descriptive error messages

## Dependencies

- Lua 5.1+
- `lua-brotli` (optional, for compression)
- Engine-specific UDFs for advanced features

## Integration

This module is required by all migration files and is automatically loaded by the Hydrogen migration system. It provides the bridge between platform-agnostic migration definitions and database-specific SQL execution.