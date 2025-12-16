# database_sqlite.lua

## Overview

`database_sqlite.lua` defines SQLite-specific database configuration and macros for the Helium migration system. It provides data type mappings and functions optimized for SQLite's lightweight, file-based architecture.

## Data Type Mappings

| Macro | SQLite Type | Description |
|-------|-------------|-------------|
| `CHAR_2` | `char(2)` | Fixed 2-character string |
| `CHAR_20` | `char(20)` | Fixed 20-character string |
| `CHAR_50` | `char(50)` | Fixed 50-character string |
| `CHAR_128` | `char(128)` | Fixed 128-character string |
| `INTEGER` | `integer` | Standard integer |
| `NOW` | `CURRENT_TIMESTAMP` | Current timestamp function |
| `PRIMARY` | `PRIMARY KEY` | Primary key constraint |
| `SERIAL` | `integer AUTOINCREMENT` | Auto-incrementing integer |
| `TEXT` | `text` | Variable-length text |
| `TEXT_BIG` | `text` | Large text (same as TEXT in SQLite) |
| `TIMESTAMP_TZ` | `text` | Timestamps stored as ISO format text |
| `UNIQUE` | `UNIQUE` | Unique constraint |
| `VARCHAR_20` | `varchar(20)` | Variable 20-char string |
| `VARCHAR_50` | `varchar(50)` | Variable 50-char string |
| `VARCHAR_100` | `varchar(100)` | Variable 100-char string |
| `VARCHAR_128` | `varchar(128)` | Variable 128-char string |
| `VARCHAR_500` | `varchar(500)` | Variable 500-char string |

## Base64 Support

SQLite uses the sqlean crypto extension for Base64 support:

```lua
BASE64_START = "CRYPTO_DECODE("
BASE64_END = ",'base64')"
```

This creates SQL like: `CRYPTO_DECODE('encoded_data','base64')`

Requires the sqlean crypto extension to be loaded.

## Compression Support

For Brotli-compressed data:

```lua
COMPRESS_START = "BROTLI_DECOMPRESS(CRYPTO_DECODE("
COMPRESS_END = ",'base64'))"
```

Requires the `brotli_decompress` SQLite extension.

## JSON Support

```lua
JSON = "text"  -- SQLite stores JSON as text
```

JSON ingestion functions are minimal since SQLite doesn't validate JSON:

```lua
JSON_INGEST_START = "("
JSON_INGEST_END = ")"
```

No custom JSON ingestion function needed.

## Additional Features

### DROP_CHECK

```lua
DROP_CHECK = "SELECT 'Refusing to drop table ${SCHEMA}${TABLE} – it contains data' WHERE EXISTS (SELECT 1 FROM ${SCHEMA}${TABLE})"
```

Used to prevent dropping tables that contain data, providing a safety check for schema modifications.

### Brotli Decompression Extension

References the SQLite loadable extension for Brotli decompression. The extension must be loaded separately.

## Version Information

- **Script**: database_sqlite.lua
- **Version**: 2.1.0
- **Release**: 2025-11-23

## Requirements

- SQLite 3.0+
- sqlean crypto extension (for Base64 support)
- `brotli_decompress` extension (extras/brotli_udf_sqlite/)

## Notes

- SQLite uses dynamic typing, so data types are more for documentation
- Timestamps stored as text in ISO 8601 format
- No native JSON type, stored as text
- Extensions must be loaded at runtime
- Excellent for embedded/single-user applications