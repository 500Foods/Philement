# database_postgresql.lua

## Overview

`database_postgresql.lua` defines PostgreSQL-specific database configuration and macros for the Helium migration system. It provides data type mappings, SQL functions, and engine-specific optimizations for PostgreSQL databases.

## Data Type Mappings

| Macro | PostgreSQL Type | Description |
|-------|----------------|-------------|
| `CHAR_2` | `char(2)` | Fixed 2-character string |
| `CHAR_20` | `char(20)` | Fixed 20-character string |
| `CHAR_50` | `char(50)` | Fixed 50-character string |
| `CHAR_128` | `char(128)` | Fixed 128-character string |
| `INTEGER` | `integer` | Standard integer |
| `NOW` | `CURRENT_TIMESTAMP` | Current timestamp function |
| `PRIMARY` | `PRIMARY KEY` | Primary key constraint |
| `SERIAL` | `serial` | Auto-incrementing integer |
| `TEXT` | `text` | Variable-length text |
| `TEXT_BIG` | `text` | Large text (same as TEXT in PostgreSQL) |
| `TIMESTAMP_TZ` | `timestamptz` | Timestamp with timezone |
| `UNIQUE` | `UNIQUE` | Unique constraint |
| `VARCHAR_20` | `varchar(20)` | Variable 20-char string |
| `VARCHAR_50` | `varchar(50)` | Variable 50-char string |
| `VARCHAR_100` | `varchar(100)` | Variable 100-char string |
| `VARCHAR_128` | `varchar(128)` | Variable 128-char string |
| `VARCHAR_500` | `varchar(500)` | Variable 500-char string |

## Base64 Support

PostgreSQL has native Base64 support through built-in functions:

```lua
BASE64_START = "CONVERT_FROM(DECODE("
BASE64_END = ", 'base64'), 'UTF8')"
```

This creates SQL like: `CONVERT_FROM(DECODE('encoded_data', 'base64'), 'UTF8')`

## Compression Support

For Brotli-compressed data:

```lua
COMPRESS_START = "${SCHEMA}brotli_decompress(DECODE("
COMPRESS_END = ", 'base64'))"
```

Requires the `brotli_decompress` PostgreSQL extension function.

## JSON Support

```lua
JSON = "jsonb"  -- PostgreSQL's binary JSON type
```

JSON ingestion functions:

```lua
JSON_INGEST_START = "${SCHEMA}json_ingest ("
JSON_INGEST_END = ")"
```

Includes a custom `json_ingest` function that handles malformed JSON by escaping control characters.

## Additional Features

### DROP_CHECK

```lua
DROP_CHECK = "SELECT pg_catalog.pg_terminate_backend(pg_backend_pid()) FROM ${SCHEMA}${TABLE} LIMIT 1"
```

Used to terminate connections before dropping tables, ensuring clean schema modifications.

### Brotli Decompression UDF

Defines the SQL to create the `brotli_decompress` function using PostgreSQL's C extension interface.

## Version Information

- **Script**: database_postgresql.lua
- **Version**: 2.1.0
- **Release**: 2025-11-23

## Requirements

- PostgreSQL 13+ (for optimal jsonb support)
- `brotli_decompress` UDF installed (extras/brotli_udf_postgresql/)
- `json_ingest` function created by migrations

## Notes

- PostgreSQL's `text` type handles both small and large text efficiently
- `timestamptz` ensures timezone-aware timestamps
- Native Base64 functions provide excellent performance
- `jsonb` offers indexed JSON operations