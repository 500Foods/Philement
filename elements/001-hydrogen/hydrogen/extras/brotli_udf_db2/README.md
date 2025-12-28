# Brotli Decompression UDF for DB2

This directory contains the DB2 User Defined Function (UDF) for Brotli decompression, used by the Helium migration system to decompress large base64-encoded strings.

## Motivation

One key motivation for using Brotli compression in DB2 is that the DB2 SQL parser restricts individual string literals to 32 KB. By using Brotli compression with maximum quality settings and base64 encoding, we can effectively handle text files up to potentially 200 KB if they compress well (such as the CSS, HTML, and JSON content commonly found in migration files). This provides a significant expansion of the effective input size limit while maintaining excellent compression ratios.

### Memory Efficiency with CLOB Locators

The UDFs now use DB2's CLOB locator mechanism (`CLOB(2G) AS LOCATOR`) for true memory efficiency. Locators prevent DB2 from pre-allocating the full 2GB capacity for each function call, allocating only the memory actually needed for the data:

- **Memory Usage**: Only allocates memory for actual data size, not the full CLOB(2G) capacity
- **Performance**: Critical for SQL statements that call these functions hundreds of times
- **No SQL Changes**: Existing SQL code continues to work without modifications
- **Increased Capacity**: Support for up to 2GB of decompressed data

## Architecture

Following the same chunked pattern as `BASE64DECODE`:

- **BROTLI_DECOMPRESS_CHUNK**: C function that processes 32KB chunks
- **BROTLI_DECOMPRESS**: SQL wrapper that handles large CLOBs by chunking

## Files

- `brotli_decompress.c` - C implementation of Brotli decompression UDF
- `Makefile` - Build and installation automation
- `install_brotli_decompress.clp` - DB2 CLP script for function registration
- `README.md` - This file

## Dependencies

- DB2 11.5 or later
- libbrotlidec (Brotli decompression library)
- gcc compiler

### Installing libbrotlidec

```bash
# Ubuntu/Debian
sudo apt-get install libbrotli-dev

# RHEL/CentOS
sudo yum install brotli-devel

# From source
git clone https://github.com/google/brotli.git
cd brotli
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make
sudo make install
```

## Building

```bash
# Set DB2 installation directory
export DB2DIR=/opt/ibm/db2/V12.1

# Build the shared object
make

# Install to DB2 function directory
make install
```

## Registration

Register the UDF in your database schema:

```bash
# Register in TEST schema of HYDROTST database
make register DB=HYDROTST SCHEMA=TEST

# Or register in a different schema
make register DB=MYDB SCHEMA=MYSCHEMA
```

## Testing

```bash
# Run basic test
make test DB=HYDROTST

# Manual testing in DB2 CLP
db2 "CONNECT TO HYDROTST"
db2 "VALUES TEST.HELIUM_BROTLI_CHECK()"
```

Expected output: `Helium-Brotli`

## Manual Testing of Encoded Strings

To verify that the base64 and Brotli encoding/decoding pipeline is working correctly, you can manually test encoded strings using command-line tools:

### Testing Base64 + Brotli Decompression

```bash
# Extract a base64-encoded compressed string from your migration data
# Then decode and decompress it manually:

echo "H4sIAAAAAAAA/0XOwQ2AIBAE0F8h..." | base64 -d | brotli -d

# Or using a file:
cat encoded_string.txt | base64 -d | brotli -d
```

### Testing Individual Components

```bash
# Test just base64 decoding:
echo "SGVsbG8gV29ybGQ=" | base64 -d
# Output: "Hello World"

# Test just brotli decompression:
echo "compressed_data" | brotli -d

# Test full pipeline (base64 decode then brotli decompress):
echo "base64_encoded_brotli_data" | base64 -d | brotli -d
```

### Verifying Migration Data

When debugging migration issues, you can extract the encoded strings from the database and verify them:

```sql
-- Get the encoded data from queries table
SELECT code FROM queries WHERE query_ref = 1000;
```

Then copy the base64 string and test it manually:

```bash
# Replace 'encoded_string_here' with actual data from database
echo "encoded_string_here" | base64 -d | brotli -d
```

This manual testing helps verify that:

- Base64 encoding/decoding is working correctly
- Brotli compression/decompression is functioning
- The data integrity is maintained through the full pipeline
- Migration scripts will produce the expected decompressed SQL

## Usage in Migrations

The UDF is automatically used by migration 1000 when processing migrations with compressed strings larger than 1KB.

## Detailed Migration System Integration

The Brotli decompression UDF is part of the Helium migration system's automatic compression pipeline. Here's how it works:

### Automatic Function Creation

Migration 1000 (`acuranzo_1000.lua`) creates the `BROTLI_DECOMPRESS` function in your database schema using this SQL:

```sql
CREATE OR REPLACE FUNCTION ${SCHEMA}BROTLI_DECOMPRESS(compressed BLOB(2G))
RETURNS CLOB(2G)
LANGUAGE C
PARAMETER STYLE DB2SQL
NO SQL
DETERMINISTIC
NOT FENCED
THREADSAFE
RETURNS NULL ON NULL INPUT
EXTERNAL NAME 'brotli_decompress.so!BROTLI_DECOMPRESS'
```

### Compression Pipeline

The migration system processes large SQL strings through this pipeline:

1. **Detection**: Identifies strings > 1KB in `[=[multiline blocks]=]`
2. **Compression**: Uses Brotli compression (quality level 11 for maximum compression)
3. **Encoding**: Base64-encodes the compressed binary data
4. **Storage**: Stores as text in the database with UDF wrapper for decompression

### Generated SQL Examples

#### Compressed Migration Code

```sql
INSERT INTO queries (code) VALUES (
    ${SCHEMA}BROTLI_DECOMPRESS(${SCHEMA}BASE64DECODEBINARY('H4sIAAAAAAAA/0XOwQ2AIBAE0F8h...'))
);
```

#### Combined with Base64 Decoding

The UDF is always used in combination with `BASE64DECODEBINARY` for proper handling of the base64-encoded compressed data:

```sql
-- The migration system generates this pattern automatically
${SCHEMA}BROTLI_DECOMPRESS(${SCHEMA}BASE64DECODEBINARY(encoded_data))
```

### Migration Author Experience

Migration authors write normal SQL with multiline string markers:

```lua
-- In migration file
sql=[[
    [=[
        CREATE TABLE example (
            id INTEGER PRIMARY KEY,
            data TEXT
        );
        -- Lots more SQL...
    ]=]
]]
```

The system automatically converts this to compressed storage, transparent to the author.

## Function Signatures

### BROTLI_DECOMPRESS_CHUNK

```sql
CREATE FUNCTION BROTLI_DECOMPRESS_CHUNK(compressed VARCHAR(32672))
RETURNS VARCHAR(32672)
LANGUAGE C
EXTERNAL NAME 'brotli_decompress.so!BROTLI_DECOMPRESS_CHUNK'
```

### BROTLI_DECOMPRESS

```sql
CREATE FUNCTION BROTLI_DECOMPRESS(compressed BLOB(1M))
RETURNS CLOB(2G) AS LOCATOR
LANGUAGE C
```

### BASE64_BROTLI_DECOMPRESS

```sql
CREATE FUNCTION BASE64_BROTLI_DECOMPRESS(encoded CLOB(2G) AS LOCATOR)
RETURNS CLOB(2G) AS LOCATOR
LANGUAGE C
```

## Troubleshooting

### Error: Cannot find brotli_decompress.so

Solution:

```bash
# Verify the file exists
ls -la /home/db2inst1/sqllib/function/brotli_decompress.so

# Check permissions
chmod 755 /home/db2inst1/sqllib/function/brotli_decompress.so
```

### Error: Cannot load library libbrotlidec.so

Solution:

```bash
# Verify Brotli library is installed
ldconfig -p | grep brotli

# If not found, install libbrotlidec package

# Update library cache
sudo ldconfig
```

### Error: SQLCODE=-204, Function not found

Solution:

```bash
# Re-run registration
make register DB=YOURDB SCHEMA=YOURSCHEMA

# Verify registration
db2 "SELECT ROUTINENAME FROM SYSCAT.ROUTINES WHERE ROUTINENAME LIKE 'BROTLI%'"
```

## Integration with Migration System

This UDF is declared in migration 1000 (`acuranzo_1000.lua`) and is automatically created when running migrations on a DB2 database. The compression is transparent to migration authors - strings larger than 1KB are automatically compressed by the Lua migration system.

## Performance

- Decompression is very fast (< 1ms for most strings)
- 70-80% size reduction for text content
- Minimal CPU overhead
- Example: 21KB CSS → 5.9KB compressed+base64

## Version History

- 1.2.0 (2025-12-28) - Implemented CLOB locators for memory efficiency with hundreds of function calls
- 1.1.0 (2025-12-28) - Increased capacity to CLOB(2G) for better performance and larger data support
- 1.0.0 (2025-11-27) - Initial creation for Brotli decompression support