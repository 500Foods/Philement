# DB2 Base64 Encode UDF

This directory contains DB2 User-Defined Functions (UDFs) for encoding data to Base64 format, with support for chunked processing to handle large inputs efficiently.

## Memory Efficiency

The underlying C UDFs use DB2's parameter style for efficient handling of data:

- **Memory Usage**: C UDFs only allocate memory for actual data size
- **Performance**: Critical for SQL statements that call these functions hundreds of times
- **No SQL Changes**: Existing SQL code continues to work without modifications

## Files Overview

- `base64_encode_udf.c`: C source code implementing the core Base64 encoding UDF
- `base64_encode_udf.so`: Compiled shared library containing the UDF implementation
- `install_base64encode.clp`: DB2 CLP script for registering the UDFs in the database
- `Makefile`: Build script for compiling and installing the UDF

## Detailed Descriptions

### base64_encode_udf.c

This C file implements DB2 UDFs for Base64 encoding. Key features:

- **Function Signature**: `BASE64_ENCODE_CHUNK(VARCHAR(32672)) RETURNS VARCHAR(32672)`
- **Parameter Style**: DB2SQL
- **Encoding Logic**: Implements standard Base64 encoding with proper padding
- **Error Handling**: Sets SQLSTATE for input too large errors
- **Null Handling**: Properly handles NULL inputs by setting output to NULL

The implementation includes:

- Base64 encoding table
- Block-by-block encoding function
- Main UDF entry point with DB2-specific parameter handling
- Binary version: `BASE64_ENCODE_CHUNK_BINARY(BLOB(32672)) RETURNS VARCHAR(32672)` for binary data

### install_base64encode.clp

This DB2 Command Line Processor script creates and registers the UDFs in the database:

- **BASE64_ENCODE_CHUNK**: Core C UDF for encoding text data to base64, returns VARCHAR(32672)
- **BASE64ENCODE**: SQL wrapper function for encoding large CLOB(100K) inputs, returns CLOB(100K)
- **BASE64_ENCODE_CHUNK_BINARY**: Core C UDF for encoding binary data (BLOB) to base64
- **BASE64ENCODEBINARY**: SQL wrapper function for encoding large BLOB(100K) inputs

The script includes smoke tests at the end to verify functionality.

### Makefile

Build automation script with the following targets:

- `make` or `make all`: Compiles the C source into the shared library
- `make install`: Copies the .so file to DB2's function directory
- `make register`: Runs the CLP script to register UDFs in the database
- `make test`: Executes smoke tests on the registered functions (including round-trip with decode)
- `make clean`: Removes build artifacts

Requires DB2DIR environment variable pointing to DB2 installation directory.

## Usage

1. Set DB2DIR: `export DB2DIR=/opt/ibm/db2/V11.5`
2. Build: `make`
3. Install: `make install`
4. Register: `make register DB=your_db SCHEMA=your_schema`
5. Test: `make test DB=your_db`

## Usage in Migration Scripts

The Base64 encode UDFs are automatically created and used by the Helium migration system during migration 1000 (`acuranzo_1000.lua`). They enable encoding of binary data (such as password hashes) to base64 format for storage and display.

### Automatic Function Creation

Migration 1000 creates the following functions in your database schema:

- `BASE64_ENCODE_CHUNK`: C UDF for encoding text data to base64, returns VARCHAR(32672)
- `BASE64ENCODE`: SQL wrapper function for encoding large CLOB(100K) inputs, returns CLOB(100K)
- `BASE64_ENCODE_CHUNK_BINARY`: C UDF for encoding binary/BLOB data to base64
- `BASE64ENCODEBINARY`: SQL wrapper function for encoding large BLOB(100K) inputs

### How Migrations Use These Functions

The migration system uses BASE64ENCODE for:

1. Encoding password hashes for storage
2. Creating base64-encoded representations of binary data
3. Data export/import operations

#### Example Generated SQL (Password Hashing)

```sql
-- Create base64-encoded SHA256 hash of password
INSERT INTO accounts (password_hash) VALUES (
    SCHEMA.BASE64ENCODE(HASH('SHA256', CAST(CONCAT(account_id, password) AS VARCHAR(256) FOR BIT DATA)))
);
```

### Migration System Integration

- **Transparent to Authors**: Migration writers use macros like `${SHA256_HASH_START}`, `${SHA256_HASH_MID}`, `${SHA256_HASH_END}`
- **Automatic Processing**: The Lua migration engine handles function registration automatically
- **Schema Prefixing**: Functions are created with your database schema prefix for proper namespacing

## Dependencies

- GCC compiler
- IBM DB2 headers and libraries
- DB2 instance with appropriate permissions

## Related

- [Base64 Decode UDF](/elements/001-hydrogen/hydrogen/extras/base64decode_udf_db2/README.md) - Companion decode function
- [Brotli UDF](/elements/001-hydrogen/hydrogen/extras/brotli_udf_db2/README.md) - Compression functions
