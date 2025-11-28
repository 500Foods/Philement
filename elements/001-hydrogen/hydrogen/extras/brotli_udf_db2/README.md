# Brotli Decompression UDF for DB2

This directory contains the DB2 User Defined Function (UDF) for Brotli decompression, used by the Helium migration system to decompress large base64-encoded strings.

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

## Usage in Migrations

The UDF is automatically used by migration 1000 when processing migrations with compressed strings larger than 1KB:

```sql
-- Example usage (generated automatically)
INSERT INTO queries (code) VALUES (
    BROTLI_DECOMPRESS(BASE64DECODEBINARY('base64_encoded_compressed_data'))
);
```

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
CREATE FUNCTION BROTLI_DECOMPRESS(compressed CLOB(2G))
RETURNS CLOB(2G)
LANGUAGE SQL
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

- 1.0.0 (2025-11-27) - Initial creation for Brotli decompression support