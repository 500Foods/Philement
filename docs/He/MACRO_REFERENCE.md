# Macro Reference

This document lists all available macros in the Helium migration system. Macros are placeholders that get automatically replaced with database-specific values, enabling single migrations to work across PostgreSQL, MySQL, SQLite, and IBM DB2.

## How Macros Work

Macros use the `${MACRO_NAME}` syntax and are expanded at runtime by the `database.lua` framework. They ensure database portability by adapting SQL to each engine's syntax.

## Categories

### Schema and Table References

| Macro | Description | PostgreSQL | MySQL | SQLite | DB2 | Source |
|-------|-------------|------------|-------|--------|-----|--------|
| `${SCHEMA}` | Schema prefix with proper separator | `public.` | `schema.` | `` (empty) | `SCHEMA.` | `database.lua` |
| `${TABLE}` | Configured table name from cfg.TABLE | User-defined | User-defined | User-defined | User-defined | Migration cfg |
| `${QUERIES}` | Queries table name | `queries` | `queries` | `queries` | `queries` | `database.lua` |

### Data Types

| Macro | Description | PostgreSQL | MySQL | SQLite | DB2 | Source |
|-------|-------------|------------|-------|--------|-----|--------|
| `${INTEGER}` | Standard integer type | `integer` | `int` | `integer` | `INTEGER` | `database_*.lua` |
| `${TEXT}` | Variable-length text | `text` | `text` | `text` | `VARCHAR(250)` | `database_*.lua` |
| `${TEXT_BIG}` | Large text content | `text` | `text` | `text` | `CLOB(1M)` | `database_*.lua` |
| `${JSON}` | JSON storage type | `jsonb` | `longtext` | `text` | `CLOB(1M)` | `database_*.lua` |
| `${TIMESTAMP_TZ}` | Timestamp with timezone | `timestamptz` | `timestamp` | `text` | `TIMESTAMP` | `database_*.lua` |
| `${VARCHAR_20}` | VARCHAR(20) | `varchar(20)` | `varchar(20)` | `varchar(20)` | `VARCHAR(20)` | `database_*.lua` |
| `${VARCHAR_50}` | VARCHAR(50) | `varchar(50)` | `varchar(50)` | `varchar(50)` | `VARCHAR(50)` | `database_*.lua` |
| `${VARCHAR_100}` | VARCHAR(100) | `varchar(100)` | `varchar(100)` | `varchar(100)` | `VARCHAR(100)` | `database_*.lua` |
| `${VARCHAR_128}` | VARCHAR(128) | `varchar(128)` | `varchar(128)` | `varchar(128)` | `VARCHAR(128)` | `database_*.lua` |
| `${VARCHAR_500}` | VARCHAR(500) | `varchar(500)` | `varchar(500)` | `varchar(500)` | `VARCHAR(500)` | `database_*.lua` |
| `${CHAR_2}` | CHAR(2) | `char(2)` | `char(2)` | `char(2)` | `CHAR(2)` | `database_*.lua` |
| `${CHAR_20}` | CHAR(20) | `char(20)` | `char(20)` | `char(20)` | `CHAR(20)` | `database_*.lua` |
| `${CHAR_50}` | CHAR(50) | `char(50)` | `char(50)` | `char(50)` | `CHAR(50)` | `database_*.lua` |
| `${CHAR_128}` | CHAR(128) | `char(128)` | `char(128)` | `char(128)` | `CHAR(128)` | `database_*.lua` |

### Constraints and Keys

| Macro | Description | PostgreSQL | MySQL | SQLite | DB2 | Source |
|-------|-------------|------------|-------|--------|-----|--------|
| `${PRIMARY}` | PRIMARY KEY constraint | `PRIMARY KEY` | `PRIMARY KEY` | `PRIMARY KEY` | `PRIMARY KEY` | `database_*.lua` |
| `${UNIQUE}` | UNIQUE constraint | `UNIQUE` | `UNIQUE` | `UNIQUE` | `UNIQUE` | `database_*.lua` |
| `${SERIAL}` | Auto-incrementing integer | `serial` | `int auto increment` | `integer AUTOINCREMENT` | `INTEGER GENERATED ALWAYS AS IDENTITY` | `database_*.lua` |

### Functions

| Macro | Description | PostgreSQL | MySQL | SQLite | DB2 | Source |
|-------|-------------|------------|-------|--------|-----|--------|
| `${NOW}` | Current timestamp | `CURRENT_TIMESTAMP` | `CURRENT_TIMESTAMP` | `CURRENT_TIMESTAMP` | `CURRENT TIMESTAMP` | `database_*.lua` |

### Query Metadata

| Macro | Description | PostgreSQL | MySQL | SQLite | DB2 | Source |
|-------|-------------|------------|-------|--------|-----|--------|
| `${QUERIES_INSERT}` | Column list for queries table INSERT | `query_id, query_ref, ...` | `query_id, query_ref, ...` | `query_id, query_ref, ...` | `query_id, query_ref, ...` | `database.lua` |
| `${TIMEOUT}` | Default query timeout (ms) | `5000` | `5000` | `5000` | `5000` | `database.lua` |
| `${DIALECT}` | Database dialect ID | `1` | `3` | `2` | `4` | `database.lua` |

### Status and Types

| Macro | Description | Value | Source |
|-------|-------------|-------|--------|
| `${STATUS_ACTIVE}` | Active query status | `1` | `database.lua` |
| `${STATUS_INACTIVE}` | Inactive query status | `0` | `database.lua` |
| `${TYPE_FORWARD_MIGRATION}` | Forward migration type | `1000` | `database.lua` |
| `${TYPE_REVERSE_MIGRATION}` | Reverse migration type | `1001` | `database.lua` |
| `${TYPE_DIAGRAM_MIGRATION}` | Diagram migration type | `1002` | `database.lua` |
| `${TYPE_APPLIED_MIGRATION}` | Applied migration type | `1003` | `database.lua` |

### Queue Types

| Macro | Description | Value | Source |
|-------|-------------|-------|--------|
| `${QTC_SLOW}` | Slow queue priority | `0` | `database.lua` |
| `${QTC_MEDIUM}` | Medium queue priority | `1` | `database.lua` |
| `${QTC_FAST}` | Fast queue priority | `2` | `database.lua` |
| `${QTC_CACHED}` | Cached queue priority | `3` | `database.lua` |

### Common Field Groups

| Macro | Description | Expansion | Source |
|-------|-------------|-----------|--------|
| `${COMMON_CREATE}` | Standard audit fields for CREATE TABLE | `valid_after timestamptz, valid_until timestamptz, created_id integer NOT NULL, created_at timestamptz NOT NULL, updated_id integer NOT NULL, updated_at timestamptz NOT NULL` | `database.lua` |
| `${COMMON_FIELDS}` | Field names for INSERT | `valid_after, valid_until, created_id, created_at, updated_id, updated_at` | `database.lua` |
| `${COMMON_VALUES}` | Default values for INSERT | `NULL, NULL, 0, CURRENT_TIMESTAMP, 0, CURRENT_TIMESTAMP` | `database.lua` |
| `${COMMON_INSERT}` | Complete INSERT values clause | `NULL AS valid_after, NULL AS valid_until, 0 AS created_id, CURRENT_TIMESTAMP AS created_at, 0 AS updated_id, CURRENT_TIMESTAMP AS updated_at` | `database.lua` |

### Diagram Metadata

| Macro | Description | Expansion | Source |
|-------|-------------|-----------|--------|
| `${COMMON_DIAGRAM}` | JSON fields for ERD diagrams | Complex JSON array with standard fields | `database.lua` |

### Content Encoding

| Macro | Description | PostgreSQL | MySQL | SQLite | DB2 | Source |
|-------|-------------|------------|-------|--------|-----|--------|
| `${BASE64_START}` | Base64 decode function start | `CONVERT_FROM(DECODE(` | `cast(FROM_BASE64(` | `CRYPTO_DECODE(` | `${SCHEMA}BASE64DECODE(` | `database_*.lua` |
| `${BASE64_END}` | Base64 decode function end | `, 'base64'), 'UTF8')` | `) as char character set utf8mb4)` | `,'base64')` | `)` | `database_*.lua` |
| `${COMPRESS_START}` | Brotli decompress start | `${SCHEMA}brotli_decompress(DECODE(` | `brotli_decompress(FROM_BASE64(` | `BROTLI_DECOMPRESS(CRYPTO_DECODE(` | `${SCHEMA}BROTLI_DECOMPRESS(${SCHEMA}BASE64DECODEBINARY(` | `database_*.lua` |
| `${COMPRESS_END}` | Brotli decompress end | `, 'base64'))` | `))` | `,'base64'))` | `))` | `database_*.lua` |

### JSON Processing

| Macro | Description | PostgreSQL | MySQL | SQLite | DB2 | Source |
|-------|-------------|------------|-------|--------|-----|--------|
| `${JSON_INGEST_START}` | JSON ingest function start | `${SCHEMA}json_ingest (` | `${SCHEMA}json_ingest(` | `(` | `${SCHEMA}JSON_INGEST(` | `database_*.lua` |
| `${JSON_INGEST_END}` | JSON ingest function end | `)` | `)` | `)` | `)` | `database_*.lua` |

### Safety Checks

| Macro | Description | PostgreSQL | MySQL | SQLite | DB2 | Source |
|-------|-------------|------------|-------|--------|-----|--------|
| `${DROP_CHECK}` | Check for data before DROP | `SELECT pg_catalog.pg_terminate_backend...` | `DO IF(EXISTS(SELECT 1 FROM...` | `SELECT 'Refusing to drop...` | `BEGIN IF EXISTS(SELECT 1...` | `database_*.lua` |

### Subquery Delimiters

| Macro | Description | Value | Source |
|-------|-------------|-------|--------|
| `${SUBQUERY_DELIMITER}` | Separates SQL statements | `-- SUBQUERY DELIMITER` | `database.lua` |

## Environment Variables and Runtime Resolution

Macros can reference environment variables that are resolved at runtime when the migration is executed. This allows sensitive data like passwords and configuration-specific values to be injected without hardcoding them in the migration files.

### How Runtime Resolution Works

1. **Macro Processing**: The `database.lua` framework first expands all known macros from the configuration
2. **Environment Lookup**: For any remaining `${VARIABLE_NAME}` patterns, it calls `os.getenv(VARIABLE_NAME)` to get the value from environment variables
3. **Runtime Injection**: This happens when the migration is actually executed by the Hydrogen system, not during the Lua script processing

### Example: Dynamic User Data

Migration `acuranzo_1024.lua` demonstrates this pattern:

```lua
INSERT INTO ${SCHEMA}${TABLE} (
    account_id, status_a16, iana_timezone_a17,
    name, first_name, middle_name, last_name,
    password_hash, summary, collection, ${COMMON_FIELDS}
) VALUES (
    0, 1, 0,
    '${HYDROGEN_USERNAME}', '', '', '${HYDROGEN_USERNAME}',
    '${HYDROGEN_PASSHASH}', 'Administrative Account', '{}',
    ${COMMON_VALUES}
);
```

Here, `${HYDROGEN_USERNAME}` and `${HYDROGEN_PASSHASH}` are not defined in the macro configuration. Instead, they are resolved at runtime from environment variables set in the Hydrogen server environment.

### Benefits

- **Security**: Sensitive data like password hashes aren't stored in code
- **Flexibility**: Same migration can work with different configurations
- **Deployment Safety**: Environment-specific values are set per deployment

### Known Environment Variables

| Variable | Description | Used In |
|----------|-------------|---------|
| `HYDROGEN_USERNAME` | Admin account username | `acuranzo_1024.lua` |
| `HYDROGEN_PASSHASH` | SHA256 password hash for admin | `acuranzo_1024.lua` |

### Custom Environment Variables

You can use any environment variable in your migrations:

```lua
-- In migration
INSERT INTO config VALUES ('${MY_CUSTOM_VAR}');

-- Set at runtime
export MY_CUSTOM_VAR="production_value"
```

**Note**: Undefined environment variables will cause migration failures, so ensure all required variables are set in your deployment environment.

## Usage Examples

### Basic Table Creation

```lua
[=[
    CREATE TABLE ${SCHEMA}${TABLE} (
        id ${INTEGER} PRIMARY KEY,
        name ${VARCHAR_100} NOT NULL,
        created_at ${TIMESTAMP_TZ} DEFAULT ${NOW}
    );
]=]
```

### With Common Fields

```lua
[=[
    CREATE TABLE ${SCHEMA}${TABLE} (
        id ${INTEGER} PRIMARY KEY,
        name ${VARCHAR_100} NOT NULL,
        ${COMMON_CREATE}
    );
]=]
```

### Insert with Metadata

```lua
INSERT INTO ${SCHEMA}${TABLE} (id, name, ${COMMON_FIELDS})
VALUES (1, 'Example', ${COMMON_VALUES});
```

## Notes

- **Case Sensitivity**: DB2 macros are uppercase, others are lowercase
- **Extensions Required**: Some features require database extensions (Brotli, crypto functions)
- **Schema Handling**: SQLite typically uses no schema prefix
- **JSON Support**: Varies significantly between databases
- **Auto-Increment**: Each database has different syntax for auto-incrementing fields

## Source Files

- `database.lua` - Core macro definitions and processing
- `database_postgresql.lua` - PostgreSQL-specific expansions
- `database_mysql.lua` - MySQL/MariaDB-specific expansions
- `database_sqlite.lua` - SQLite-specific expansions
- `database_db2.lua` - IBM DB2-specific expansions