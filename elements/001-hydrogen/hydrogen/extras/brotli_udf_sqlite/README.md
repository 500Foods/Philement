# Brotli Decompression Extension for SQLite

This directory contains the SQLite loadable extension for Brotli decompression, used by the Helium migration system to decompress large base64-encoded strings.

## Files

- `brotli_decompress.c` - C implementation of Brotli decompression extension
- `Makefile` - Build and installation automation
- `README.md` - This file

## Dependencies

- SQLite 3.0 or later
- libbrotlidec (Brotli decompression library)
- gcc compiler

### Installing Dependencies

```bash
# Ubuntu/Debian
sudo apt-get install libsqlite3-dev libbrotli-dev

# RHEL/CentOS
sudo yum install sqlite-devel brotli-devel
```

## Building and Installing

```bash
# Build the extension
make

# Install to system location
sudo make install
```

## Loading the Extension

There are several ways to load the extension:

### Method 1: Runtime Loading

```sql
-- In SQLite CLI or connection
.load /usr/local/lib/brotli_decompress
```

### Method 2: Automatic Loading

Add to SQLite initialization:

```c
sqlite3_enable_load_extension(db, 1);
sqlite3_load_extension(db, "brotli_decompress", NULL, NULL);
```

### Method 3: Connection String (if supported)

```bash
sqlite3 database.db -cmd ".load brotli_decompress"
```

## Usage in Migrations

The extension is used by the Helium migration system when processing migrations with compressed strings larger than 1KB:

```sql
-- Example usage (generated automatically)
INSERT INTO queries (code) VALUES (
    BROTLI_DECOMPRESS(CRYPTO_DECODE('base64_encoded_compressed_data','base64'))
);
```

## Function Signature

```sql
BROTLI_DECOMPRESS(compressed_blob) -> text
```

**Parameters:**

- `compressed_blob`: BLOB containing Brotli-compressed data

**Returns:**

- Decompressed text as UTF-8 string

## Testing

```bash
# Load and test the extension
sqlite3 :memory: <<EOF
.load ./brotli_decompress
SELECT 'Extension loaded successfully' AS status;
EOF
```

## Troubleshooting

### Error: cannot open shared object file

Solution:

```bash
# Check library path
ldd brotli_decompress.so

# If libbrotlidec.so not found, install and update cache
sudo apt-get install libbrotli-dev
sudo ldconfig
```

### Error: not authorized to load extension

Solution:

```bash
# Extensions must be enabled before loading
sqlite3_enable_load_extension(db, 1);
```

### Error: no such function: BROTLI_DECOMPRESS

Solution:

```bash
# Ensure extension is loaded
.load /path/to/brotli_decompress
```

## Integration with Migration System

SQLite uses the `CRYPTO_DECODE` function from the sqlean crypto extension for base64 decoding, and this `BROTLI_DECOMPRESS` extension for decompression. The functions work together:

```sql
BROTLI_DECOMPRESS(CRYPTO_DECODE('...', 'base64'))
```

## Performance

- Minimal overhead
- < 1ms decompression for most strings
- 70-80% size reduction for text content
- Thread-safe implementation
- Example: 21KB CSS → 5.9KB compressed+base64

## Version History

- 1.0.0 (2025-11-27) - Initial creation for Brotli decompression support

## See Also

- sqlean crypto extension (for CRYPTO_DECODE)
- Migration system documentation: `BROTLI_COMPRESSION.md`