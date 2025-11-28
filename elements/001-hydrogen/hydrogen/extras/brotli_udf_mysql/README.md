# Brotli Decompression UDF for MySQL

This directory contains the MySQL User Defined Function (UDF) for Brotli decompression, used by the Helium migration system to decompress large base64-encoded strings.

## Files

- `brotli_decompress.c` - C implementation of Brotli decompression UDF
- `Makefile` - Build and installation automation
- `README.md` - This file

## Dependencies

- MySQL 5.7+ or MariaDB 10.2+
- libbrotlidec (Brotli decompression library)
- mysql-dev package (for mysql.h headers)
- gcc compiler

### Installing Dependencies

```bash
# Ubuntu/Debian
sudo apt-get install libmysqlclient-dev libbrotli-dev

# RHEL/CentOS
sudo yum install mysql-devel brotli-devel
```

## Building

```bash
# Build the shared object
make

# Install to MySQL plugin directory
sudo make install
```

## Registration

Register the UDF in MySQL:

```bash
# Register the function
make register

# Or manually in MySQL:
mysql -e "CREATE FUNCTION BROTLI_DECOMPRESS RETURNS STRING SONAME 'brotli_decompress.so';"
```

## Testing

```bash
# Basic test
make test

# Manual testing
mysql -e "SELECT 'UDF installed' AS status;"
```

## Usage in Migrations

The UDF is automatically used by migration 1000 when processing migrations with compressed strings larger than 1KB:

```sql
-- Example usage (generated automatically)
INSERT INTO queries (code) VALUES (
    BROTLI_DECOMPRESS(cast(from_base64('base64_encoded_compressed_data') as char character set utf8mb4))
);
```

## Function Signature

```sql
CREATE FUNCTION BROTLI_DECOMPRESS(compressed_data BLOB)
RETURNS STRING
SONAME 'brotli_decompress.so';
```

## Troubleshooting

### Error: Can't open shared library 'brotli_decompress.so'

Solution:

```bash
# Check plugin directory
mysql -e "SHOW VARIABLES LIKE 'plugin_dir';"

# Manually install
sudo cp brotli_decompress.so /path/to/plugin_dir/
sudo chmod 755 /path/to/plugin_dir/brotli_decompress.so
```

### Error: Cannot load library libbrotlidec.so

Solution:

```bash
# Install Brotli development package
sudo apt-get install libbrotli-dev

# Update library cache
sudo ldconfig
```

### Error: Function already exists

Solution:

```bash
# Drop and recreate
mysql -e "DROP FUNCTION IF EXISTS BROTLI_DECOMPRESS;"
make register
```

## Performance

- Simple wrapper around libbrotlidec
- < 1ms decompression for most strings
- Minimal memory overhead
- Thread-safe

## Version History

- 1.0.0 (2025-11-27) - Initial creation for Brotli decompression support