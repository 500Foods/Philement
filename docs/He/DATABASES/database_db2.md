# database_db2.lua

## Overview

`database_db2.lua` defines IBM DB2-specific database configuration and macros for the Helium migration system. It provides data type mappings, SQL functions, and engine-specific optimizations for DB2 databases.

## Data Type Mappings

| Macro | DB2 Type | Description |
|-------|----------|-------------|
| `CHAR_2` | `CHAR(2)` | Fixed 2-character string |
| `CHAR_20` | `CHAR(20)` | Fixed 20-character string |
| `CHAR_50` | `CHAR(50)` | Fixed 50-character string |
| `CHAR_128` | `CHAR(128)` | Fixed 128-character string |
| `INTEGER` | `INTEGER` | Standard integer |
| `NOW` | `CURRENT TIMESTAMP` | Current timestamp function |
| `PRIMARY` | `PRIMARY KEY` | Primary key constraint |
| `SERIAL` | `INTEGER GENERATED ALWAYS AS IDENTITY` | Auto-incrementing integer |
| `TEXT` | `VARCHAR(250)` | Variable-length text (small) |
| `TEXT_BIG` | `CLOB(1M)` | Large text (1MB CLOB) |
| `TIMESTAMP_TZ` | `TIMESTAMP` | Timestamp (timezone handling varies) |
| `UNIQUE` | `UNIQUE` | Unique constraint |
| `VARCHAR_20` | `VARCHAR(20)` | Variable 20-char string |
| `VARCHAR_50` | `VARCHAR(50)` | Variable 50-char string |
| `VARCHAR_100` | `VARCHAR(100)` | Variable 100-char string |
| `VARCHAR_128` | `VARCHAR(128)` | Variable 128-char string |
| `VARCHAR_500` | `VARCHAR(500)` | Variable 500-char string |

## Base64 Support

DB2 uses custom UDFs for Base64 support:

```lua
BASE64_START = "${SCHEMA}BASE64DECODE("
BASE64_END = ")"
```

This creates SQL like: `${SCHEMA}BASE64DECODE('encoded_data')`

Requires the BASE64_DECODE_CHUNK UDF to be installed.

## Compression Support

For Brotli-compressed data:

```lua
COMPRESS_START = "${SCHEMA}BROTLI_DECOMPRESS(${SCHEMA}BASE64DECODEBINARY("
COMPRESS_END = "))"
```

Requires both Base64 decode and Brotli decompress UDFs.

## JSON Support

```lua
JSON = "CLOB(1M)"  -- DB2 stores JSON as CLOB
```

JSON ingestion functions:

```lua
JSON_INGEST_START = "${SCHEMA}JSON_INGEST("
JSON_INGEST_END = ")"
```

Includes a custom `JSON_INGEST` function that validates JSON using SYSTOOLS.JSON2BSON and handles malformed JSON by escaping control characters.

## Additional Features

### DROP_CHECK

```lua
DROP_CHECK = "BEGIN IF EXISTS(SELECT 1 FROM ${SCHEMA}${TABLE}) THEN SIGNAL SQLSTATE '75001' SET MESSAGE_TEXT='Refusing to drop table ${SCHEMA}${TABLE} – it contains data'; END IF; END"
```

Used to prevent dropping tables that contain data, raising an exception if the table is not empty.

### Brotli Decompression UDF

Defines the SQL to create the `BROTLI_DECOMPRESS` function using DB2's C extension interface.

## Version Information

- **Script**: database_db2.lua
- **Version**: 2.1.0
- **Release**: 2025-11-23

## Requirements

- IBM DB2 10.0+
- Base64 UDFs installed (extras/base64_udf_db2/)
- Brotli UDF installed (extras/brotli_udf_db2/)
- SYSTOOLS schema available for JSON validation

## Notes

- DB2 uses uppercase for data types and keywords
- Large objects handled with CLOB/BLOB types
- Identity columns for auto-increment
- Comprehensive JSON validation using built-in functions
- Custom UDFs required for advanced features