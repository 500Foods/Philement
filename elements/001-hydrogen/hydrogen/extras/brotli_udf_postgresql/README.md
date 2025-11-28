# Brotli Decompression Extension for PostgreSQL

This directory contains the PostgreSQL extension for Brotli decompression, used by the Helium migration system to decompress large base64-encoded strings.

## Files

- `brotli_decompress.c` - C implementation of Brotli decompression function
- `brotli_decompress.control` - Extension metadata
- `brotli_decompress--1.0.sql` - SQL function definition
- `Makefile` - Build and installation automation using PGXS
- `README.md` - This file

## Dependencies

- PostgreSQL 9.1 or later
- libbrotlidec (Brotli decompression library)
- postgresql-server-dev package (for postgres.h headers)
- gcc compiler

### Installing Dependencies

```bash
# Ubuntu/Debian
sudo apt-get install postgresql-server-dev-all libbrotli-dev

# RHEL/CentOS
sudo yum install postgresql-devel brotli-devel
```

## Building and Installing

```bash
# Build the extension
make

# Install to PostgreSQL (requires superuser)
sudo make install

# Enable in your database
psql -d yourdb -c "CREATE EXTENSION brotli_decompress;"
```

## Usage in Migrations

The extension is automatically used by migration 1000 when processing migrations with compressed strings larger than 1KB:

```sql
-- Example usage (generated automatically)
INSERT INTO queries (code) VALUES (
    brotli_decompress(CONVERT_FROM(DECODE('base64_encoded_compressed_data', 'base64'), 'UTF8'))
);
```

## Function Signature

```sql
CREATE FUNCTION brotli_decompress(compressed bytea)
RETURNS text
LANGUAGE C STRICT IMMUTABLE;
```

## Troubleshooting

### Error: extension "brotli_decompress" is not available

Solution:

```bash
# Reinstall the extension
cd extras/brotli_udf_postgresql
sudo make install

# Verify installation
psql -c "SELECT name, default_version FROM pg_available_extensions WHERE name = 'brotli_decompress';"
```

### Error: could not load library

Solution:

```bash
# Check if libbrotlidec is installed
ldconfig -p | grep brotli

# If not found, install
sudo apt-get install libbrotli-dev
sudo ldconfig
```

### Error: permission denied

Solution:

```bash
# Ensure you have superuser privileges
psql -c "CREATE EXTENSION brotli_decompress;" -U postgres
```

## Integration with Migration System

This extension is declared in migration 1000 (`acuranzo_1000.lua`) using the function declaration SQL. The compression is transparent to migration authors - strings larger than 1KB are automatically compressed by the Lua migration system.

## Performance

- Native C implementation
- < 1ms decompression for most strings
- 70-80% size reduction for text content
- Minimal CPU overhead
- Example: 21KB CSS → 5.9KB compressed+base64

## Version History

- 1.0.0 (2025-11-27) - Initial creation for Brotli decompression support