# SQLite Brotli Decompression Extension

This SQLite loadable extension provides brotli decompression functionality for use in the Helium migration system and other database operations.

## Requirements

- SQLite 3.x
- Brotli library (`libbrotlidec`)
- [sqlean crypto extension](https://github.com/nalgeon/sqlean) (for base64 decoding in tests)

## Building

```bash
make clean
make
```

This will produce `brotli_decompress.so`

## Installation

### System-wide Installation

```bash
sudo make install
```

This installs to `/usr/local/lib/brotli_decompress.so`

### Loading the Extension

From SQLite command line:

```sql
.load /usr/local/lib/brotli_decompress
-- or if installed in a standard location:
.load brotli_decompress
```

From application code, use `sqlite3_load_extension()`.

## Usage

### Function Signature

```sql
BROTLI_DECOMPRESS(compressed_blob) -> text
```

**Parameters:**

- `compressed_blob`: Binary blob containing brotli-compressed data

**Returns:**

- Decompressed text string
- `NULL` if input is `NULL`
- Empty string if input is empty

### Examples

#### Basic Usage

```sql
-- Decompress a brotli-compressed blob
SELECT BROTLI_DECOMPRESS(my_compressed_column) FROM my_table;
```

#### With Base64 Encoded Data

If your compressed data is base64-encoded (common for text storage), use the sqlean crypto extension:

```sql
-- Load both extensions
.load brotli_decompress
-- sqlean crypto is typically auto-loaded from ~/.sqliterc

-- Decode base64 then decompress
SELECT BROTLI_DECOMPRESS(crypto_decode('jwWASGVsbG8gV29ybGQhAw==', 'base64')) AS result;
-- Result: "Hello World!"
```

#### Extract JSON Fields from Compressed Data

```sql
-- Decompress and parse JSON in one query
SELECT 
    json_extract(
        BROTLI_DECOMPRESS(compressed_config),
        '$.server.port'
    ) AS port
FROM configurations;
```

## Testing

The test suite uses the same sample data as the MySQL UDF for consistency.

### Prerequisites

Install the sqlean crypto extension for base64 decoding:

```bash
# Download from https://github.com/nalgeon/sqlean/releases
# Or add to ~/.sqliterc:
.load /usr/local/lib/crypto
```

### Run Tests

```bash
make test
```

Or use the test SQL file directly:

```bash
sqlite3 < test_brotli.sql
```

### Test Data

The tests use three sample strings that match the MySQL UDF tests:

1. **Short string** (12 bytes): "Hello World!"
2. **Medium string** (369 bytes): JSON configuration data
3. **Large string** (~50KB): Extended configuration data

## Common Issues

### Error: undefined symbol: sqlite3_brotlidecompress_init

**Cause:** The init function name must match SQLite's naming convention.

**Solution:** The function is named `sqlite3_brotlidecompress_init` (no underscores between "brotli" and "decompress") to match SQLite's requirements.

### Error: no such function: base64_decode

**Cause:** The sqlean crypto extension is not loaded.

**Solution:** Install and load the sqlean crypto extension, or use `crypto_decode()` instead of `base64_decode()`.

### Extension Won't Load

**Cause:** Library path or permissions issue.

**Solution:**

```bash
# Check file exists and has correct permissions
ls -la /usr/local/lib/brotli_decompress.so

# Try loading with full path
sqlite3 -cmd ".load /usr/local/lib/brotli_decompress"
```

## Performance

The extension efficiently handles compressed data:

- **Buffer Management:** Dynamic buffer allocation with automatic resizing
- **Memory Limits:** Maximum decompressed size of 256MB to prevent memory exhaustion
- **Streaming:** Uses Brotli's streaming API for optimal performance

## Comparison with MySQL UDF

Both implementations use identical:

- Decompression logic
- Buffer management strategy
- Error handling approach
- Test data samples

Key differences:

- **API:** SQLite uses `sqlite3_context` instead of MySQL's `UDF_INIT`/`UDF_ARGS`
- **Memory:** SQLite uses `sqlite3_malloc()`/`sqlite3_free()` instead of standard `malloc()`/`free()`
- **Init Function:** SQLite requires specific naming: `sqlite3_<filename>_init()`

## License

See [LICENSE.md](../../LICENSE.md) in the project root.

## See Also

- [MySQL Brotli UDF](../brotli_udf_mysql/) - MySQL version of this extension
- [Helium Migration System](../../../002-helium/) - Uses this extension for data migration
- [Brotli Library Documentation](https://github.com/google/brotli)
- [SQLite Loadable Extensions](https://www.sqlite.org/loadext.html)