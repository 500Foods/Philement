# database_mysql.lua

## Overview

`database_mysql.lua` defines MySQL/MariaDB-specific database configuration and macros for the Helium migration system. It provides data type mappings, SQL functions, and engine-specific optimizations for MySQL and MariaDB databases.

## Data Type Mappings

| Macro | MySQL Type | Description |
| ------- | ------------ | ------------- |
| `CHAR_2` | `char(2)` | Fixed 2-character string |
| `CHAR_20` | `char(20)` | Fixed 20-character string |
| `CHAR_50` | `char(50)` | Fixed 50-character string |
| `CHAR_128` | `char(128)` | Fixed 128-character string |
| `INTEGER` | `int` | Standard integer |
| `NOW` | `CURRENT_TIMESTAMP` | Current timestamp function |
| `PRIMARY` | `PRIMARY KEY` | Primary key constraint |
| `SERIAL` | `int auto increment` | Auto-incrementing integer |
| `TEXT` | `text` | Variable-length text |
| `TEXT_BIG` | `text` | Large text (same as TEXT in MySQL) |
| `TIMESTAMP_TZ` | `timestamp` | Timestamp (timezone handling varies) |
| `UNIQUE` | `UNIQUE` | Unique constraint |
| `VARCHAR_20` | `varchar(20)` | Variable 20-char string |
| `VARCHAR_50` | `varchar(50)` | Variable 50-char string |
| `VARCHAR_100` | `varchar(100)` | Variable 100-char string |
| `VARCHAR_128` | `varchar(128)` | Variable 128-char string |
| `VARCHAR_500` | `varchar(500)` | Variable 500-char string |

## Base64 Support

MySQL has native Base64 support through built-in functions:

```lua
BASE64_START = "cast(FROM_BASE64("
BASE64_END = ") as char character set utf8mb4)"
```

This creates SQL like: `cast(FROM_BASE64('encoded_data') as char character set utf8mb4)`

## Compression Support

For Brotli-compressed data:

```lua
COMPRESS_START = "brotli_decompress(FROM_BASE64("
COMPRESS_END = "))"
```

Requires the `brotli_decompress` MySQL UDF.

## JSON Support

```lua
JSON = "longtext"  -- MySQL stores JSON as text
```

JSON ingestion functions:

```lua
JSON_INGEST_START = "${SCHEMA}json_ingest("
JSON_INGEST_END = ")"
```

Includes a custom `json_ingest` function that handles malformed JSON by escaping control characters, including comprehensive Unicode escape sequences for control characters.

## Additional Features

### DROP_CHECK

```lua
DROP_CHECK = " DO IF(EXISTS(SELECT 1 FROM ${SCHEMA}${TABLE}), cast('Refusing to drop table ${SCHEMA}${TABLE} – it contains data' AS CHAR(0)), NULL)"
```

Used to prevent dropping tables that contain data, providing a safety check for schema modifications.

### Brotli Decompression UDF

Defines the SQL to create the `brotli_decompress` function using MySQL's UDF interface.

## Version Information

- **Script**: database_mysql.lua
- **Version**: 2.1.0
- **Release**: 2025-11-23

## Requirements

- MySQL 5.7+ or MariaDB 10.0+
- `brotli_decompress` UDF installed (extras/brotli_udf_mysql/)
- `json_ingest` function created by migrations

## Timezones

In order to support queries using IANA timezone names like "America/New_York", an extra step is required to load the timezone tables. Note that PostgreSQL and DB2 support using IANA timezone names natively, and SQLite requires a UDF to do this at all.  Running this command enables CONVERT_TZ(timestamp, source_tz, target_tz) to work properly. Failing to populate these tables will result in CONVERT_TZ returning NULL.

MySQL: `mysql_tzinfo_to_sql /usr/share/zoneinfo | mysql -u root -p mysql`
MariaDB: `mariadb-tzinfo-to-sql /usr/share/zoneinfo | mariadb -u root -p mysql`

## Notes

- MySQL's `text` type handles both small and large text
- `timestamp` type behavior depends on timezone settings
- Native Base64 functions provide good performance
- JSON stored as text, validation handled by custom functions