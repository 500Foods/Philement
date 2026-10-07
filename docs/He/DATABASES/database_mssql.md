# database_mssql.lua

## Overview

`database_mssql.lua` defines SQL Server macros for the Helium migration system. Lookup 030 key 5 is the dialect id (`${DIALECT}` = `5`).

SQL Server 2022 runs as the official Linux container under Podman (`philement-mssql`, TDS port 1433). Developer edition is for development and test. It is not a production license. The Hydrogen client is unixODBC. Test 39 defaults `MSSQL_ODBC_DRIVER` to FreeTDS. Microsoft ODBC Driver 18 is the driver name the engine describes when that driver is installed.

`${SCHEMA}` is a dot-prefixed schema name (`testms.` on Test 39, `demoms.` on the demo matrix). Both schemas live in database `hydrotst`.

Container scripts: [extras/mssql_server/README.md](/elements/001-hydrogen/hydrogen/extras/mssql_server/README.md).

## Data Type Mappings

| Macro | SQL Server type | Description |
| --- | --- | --- |
| `INTEGER` | `INT` | 32-bit integer |
| `INTEGER_BIG` | `BIGINT` | 64-bit integer |
| `INTEGER_SMALL` | `SMALLINT` | 16-bit integer |
| `FLOAT` | `REAL` | Binary32 |
| `FLOAT_BIG` | `FLOAT` | Binary64 (`FLOAT` in T-SQL is 53-bit) |
| `TEXT` | `NVARCHAR(255)` | Short Unicode text |
| `TEXT_BIG` | `NVARCHAR(MAX)` | Large Unicode text |
| `JSON` | `NVARCHAR(MAX)` | JSON stored as Unicode text |
| `TIMESTAMP_TZ` | `DATETIMEOFFSET` | Timestamp with offset |
| `DATETIME` | `DATETIME2` | Timestamp without a zone |
| `SERIAL` | `INT IDENTITY(1,1)` | Identity column |
| `VARCHAR_*` | `NVARCHAR(n)` | Sized Unicode text |
| `CHAR_*` | `NCHAR(n)` | Sized Unicode char |
| `NOW` | `SYSUTCDATETIME()` | Current UTC timestamp |
| `PRIMARY` | `PRIMARY KEY` | Primary key constraint |
| `UNIQUE` | `UNIQUE` | Unique constraint |

`${INSERT_KEY_RETURN}` is the word `RETURNING` followed by a space. `database.lua` rewrites that to `OUTPUT INSERTED.col` and moves a trailing `WITH` list in front of `INSERT`. `LENGTH(` is rewritten to `LEN(`. `CHAR_LENGTH`, `DATALENGTH`, `OCTET_LENGTH`, and `CHARACTER_LENGTH` stay.

## Base64 and SHA-256

Both are T-SQL functions created by the migration, not CLR. `base64_decode` and `base64_encode` use XML `xs:base64Binary` and read the bytes as UTF-8. `sha256_b64` uses `HASHBYTES('SHA2_256')`.

```lua
BASE64_START = "${SCHEMA}base64_decode("
BASE64_END = ")"
SHA256_HASH_START = "${SCHEMA}sha256_b64("
SHA256_HASH_MID = ", "
SHA256_HASH_END = ")"
```

## Brotli

`COMPRESS_START` and `COMPRESS_END` are `nil`. `BROTLI_DECOMPRESS_FUNCTION` is a comment until the CLR assembly in [extras/brotli_udf_mssql/README.md](/elements/001-hydrogen/hydrogen/extras/brotli_udf_mssql/README.md) is deployed. A migration that expands `${COMPRESS_START}` on SQL Server does not decompress until that assembly exists.

## JSON

`JSON_VALUE` and `ISJSON` are native (SQL Server 2016+). `json_ingest` is a T-SQL function. Schema documents (`$ref`, `$id`, `$schema`) use the same function. No separate JSON UDR is required.

```lua
JSON_INGEST_START = "${SCHEMA}json_ingest("
JSON_INGEST_END = ")"
JRS = "JSON_VALUE("
JRE = ")"
```

## Other macros

`${DUMMY_TABLE}` is empty. A bare `SELECT` does not need `FROM`.

`${DROP_CHECK}` raises error 51000 when the table has rows (`IF EXISTS … THROW`).

`${REORG}` is a SQL comment. SQL Server does not need DB2's reorg-after-alter.

`${SESSION_SECS}` is `DATEDIFF(SECOND, :SESSION_START, SYSUTCDATETIME())`.

Date arithmetic uses `DATEADD(MINUTE, 0 +/- n, SYSUTCDATETIME())`.
