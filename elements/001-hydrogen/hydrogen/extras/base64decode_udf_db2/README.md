# DB2 Base64 Decode UDF

This directory contains DB2 User-Defined Functions (UDFs) for decoding Base64 encoded data, with support for chunked processing to handle large inputs efficiently.

## Files Overview

- `base64_chunk_udf.c`: C source code implementing the core Base64 decoding UDF
- `base64_chunk_udf.so`: Compiled shared library containing the UDF implementation
- `install_base64decode.clp`: DB2 CLP script for registering the UDFs in the database
- `Makefile`: Build script for compiling and installing the UDF

## Detailed Descriptions

### base64_chunk_udf.c

This C file implements a DB2 UDF called `BASE64_DECODE_CHUNK` that decodes Base64 encoded strings. Key features:

- **Function Signature**: `BASE64_DECODE_CHUNK(VARCHAR(32672)) RETURNS VARCHAR(32672)`
- **Parameter Style**: DB2SQL
- **Decoding Logic**: Implements standard Base64 decoding with proper handling of padding characters (=)
- **Error Handling**: Sets SQLSTATE for invalid input or memory allocation failures
- **Memory Management**: Dynamically allocates working memory for input filtering
- **Input Validation**: Filters input to only valid Base64 characters before decoding
- **Null Handling**: Properly handles NULL inputs by setting output to NULL

The implementation includes:

- Base64 index mapping initialization
- Block-by-block decoding function
- Main UDF entry point with DB2-specific parameter handling
- Additional test UDFs: `HYDROGEN_CHECK` (returns "Hydrogen") and `ScalarUDF` (official DB2 sample)

### install_base64decode.clp

This DB2 Command Line Processor script creates and registers the UDFs in the database:

- **BASE64_DECODE_CHUNK**: Core C UDF for decoding Base64 chunks (text output)
- **BASE64DECODE**: SQL wrapper function that handles large CLOB inputs by processing them in 32,672-character chunks (text output)
- **BASE64_DECODE_CHUNK_BINARY**: Core C UDF for decoding Base64 chunks (binary output, used with Brotli decompression)
- **BASE64DECODEBINARY**: SQL wrapper function for decoding large CLOB inputs (binary output, used with Brotli decompression)
- **HYDROGEN_CHECK**: Test UDF returning "Hydrogen"
- **ScalarUDF**: Official DB2 sample UDF for salary calculations

The script includes smoke tests at the end to verify functionality.

### Makefile

Build automation script with the following targets:

- `make` or `make all`: Compiles the C source into the shared library
- `make install`: Copies the .so file to DB2's function directory
- `make register`: Runs the CLP script to register UDFs in the database
- `make test`: Executes smoke tests on the registered functions
- `make clean`: Removes build artifacts

Requires DB2DIR environment variable pointing to DB2 installation directory.

## Usage

1. Set DB2DIR: `export DB2DIR=/opt/ibm/db2/V11.5`
2. Build: `make`
3. Install: `make install`
4. Register: `make register DB=your_db SCHEMA=your_schema`
5. Test: `make test DB=your_db`

## Usage in Migration Scripts

The Base64 decode UDFs are automatically created and used by the Helium migration system during migration 1000 (`acuranzo_1000.lua`). They enable the storage of large SQL strings (such as DDL statements, JSON data, or configuration) in a compressed and base64-encoded format to reduce database size and improve migration performance.

### Automatic Function Creation

Migration 1000 creates the following functions in your database schema:

- `BASE64_DECODE_CHUNK`: C UDF for decoding individual 32KB chunks
- `BASE64DECODE`: SQL wrapper function for decoding large CLOB inputs (text data)
- `BASE64DECODEBINARY`: SQL wrapper function for decoding large CLOB inputs (binary data, used with Brotli decompression)

### How Migrations Use These Functions

The migration system automatically detects SQL strings larger than 1KB and:

1. Compresses them using Brotli (if >1KB)
2. Base64-encodes the result
3. Wraps the encoded data with the appropriate UDF calls

#### Example Generated SQL (Text Data)

```sql
INSERT INTO queries (code) VALUES (
    SCHEMA.BASE64DECODE('SGVsbG8gV29ybGQ=')
);
```

#### Example Generated SQL (Compressed Data)

```sql
INSERT INTO queries (code) VALUES (
    SCHEMA.BROTLI_DECOMPRESS(SCHEMA.BASE64DECODEBINARY('H4sIAAAAAAAA/0XOwQ2AIBAE0F8h')))
);
```

### Migration System Integration

- **Transparent to Authors**: Migration writers use normal SQL syntax with `[=[multiline strings]=]` markers
- **Automatic Processing**: The Lua migration engine handles compression and encoding automatically
- **Performance Benefits**: 70-80% size reduction for text content, faster migration execution
- **Schema Prefixing**: Functions are created with your database schema prefix for proper namespacing

## Dependencies

- GCC compiler
- IBM DB2 headers and libraries
- DB2 instance with appropriate permissions