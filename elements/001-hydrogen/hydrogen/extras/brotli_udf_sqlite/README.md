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

## Usage in Migration Scripts

The Brotli decompression extension is automatically used by the Helium migration system when processing migrations with compressed strings larger than 1KB. Unlike other database engines, SQLite does not require explicit function creation - the extension is loaded dynamically.

### Automatic Extension Loading

Migration 1000 (`acuranzo_1000.lua`) includes a comment indicating how to load the extension:

```sql
-- SQLite loadable extension for Brotli decompression
-- Requires: brotli_decompress.so in SQLite extension directory
-- Installation handled via extras/brotli_udf_sqlite/
-- Extension will be loaded via: SELECT load_extension('brotli_decompress');
```

The extension must be available in SQLite's extension directory or loaded explicitly before running migrations.

### Compression Pipeline

The migration system processes large strings through:

1. **Detection**: Identifies strings > 1KB in `[=[multiline blocks]=]`
2. **Compression**: Brotli compression (quality 11)
3. **Encoding**: Base64 encoding using sqlean crypto extension
4. **Storage**: Stored with UDF wrapper for automatic decompression

### Generated SQL Examples

#### Compressed Migration Code

```sql
INSERT INTO queries (code) VALUES (
    BROTLI_DECOMPRESS(CRYPTO_DECODE('H4sIAAAAAAAA/0XOwQ2AIBAE0F8h...', 'base64'))
);
```

#### Full Pipeline Example

```sql
-- Automatic generation from multiline SQL blocks
BROTLI_DECOMPRESS(CRYPTO_DECODE('encoded_compressed_data', 'base64'))
```

### Migration Author Experience

Authors write standard SQL with multiline markers:

```lua
-- In migration file
sql=[[
    [=[
        CREATE TABLE settings (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            key TEXT NOT NULL,
            value TEXT,
            -- ... extensive schema
        );
    ]=]
]]
```

The system transparently converts this to compressed storage using the sqlean crypto extension for base64 operations.

### Prerequisites for Migration System

- **Extension Installation**: `brotli_decompress.so` must be in SQLite's extension directory
- **Crypto Extension**: sqlean crypto extension for `CRYPTO_DECODE()` function
- **Loading**: Extensions loaded via `.load` command or `sqlite3_load_extension()`

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

## See Also

- [MySQL Brotli UDF](/elements/001-hydrogen/hydrogen/extras/brotli_udf_mysql/) - MySQL version of this extension
- [Brotli Library Documentation](https://github.com/google/brotli)
- [SQLite Loadable Extensions](https://www.sqlite.org/loadext.html)

## Using Extensions in C Code

The SQLite extensions are dynamically loaded at connection time in the Hydrogen C codebase, specifically in `src/database/sqlite/connection.c`. This required significant implementation work to properly integrate both base64 decoding (via sqlean crypto extension) and Brotli decompression.

### Dynamic Loading Process

The connection establishment process includes:

1. **Library Loading**: Both `crypto.so` and `brotli_decompress.so` are loaded using `dlopen()` with `RTLD_LAZY`
2. **Extension Enabling**: SQLite extension loading is enabled using `sqlite3_db_config(SQLITE_DBCONFIG_ENABLE_LOAD_EXTENSION, 1)`
3. **Extension Registration**: Extensions are loaded into SQLite using `sqlite3_load_extension()`
4. **Error Handling**: Comprehensive error handling with graceful degradation if extensions fail to load

### Key Implementation Details

#### Library Path Configuration

```c
const char* crypto_path = "/usr/local/lib/crypto.so";
const char* brotli_path = "/usr/local/lib/brotli_decompress.so";
```

#### Extension Loading Sequence

```c
// Enable extension loading (must be done immediately after sqlite3_open)
sqlite3_db_config_ptr(sqlite_db, SQLITE_DBCONFIG_ENABLE_LOAD_EXTENSION, 1, NULL);

// Load crypto extension for base64 functions
sqlite3_load_extension_ptr(sqlite_db, crypto_path, 0, &error_msg);

// Load brotli extension for decompression
sqlite3_load_extension_ptr(sqlite_db, brotli_path, 0, &error_msg);
```

#### Connection State Management

The `SQLiteConnection` struct stores library handles for proper cleanup:

```c
typedef struct {
    void* db;
    char* db_path;
    void* crypto_handle;      // dlopen handle for crypto.so
    void* brotli_handle;      // dlopen handle for brotli_decompress.so
    PreparedStatementCache* prepared_statements;
} SQLiteConnection;
```

### Error Handling and Logging

The implementation includes extensive logging and error handling:

- **Library Load Failures**: Logged as errors but connection continues
- **Extension Load Failures**: Detailed error messages with SQLite result codes
- **Graceful Degradation**: System continues if extensions aren't available
- **Memory Management**: Proper cleanup of error messages and library handles

### Integration with Migration System

Unlike other database engines where UDFs are registered via SQL scripts, SQLite extensions are loaded dynamically:

- **Connection-Time Loading**: Extensions loaded when database connection is established
- **No Migration Registration**: Migration 1000 only includes a comment about extension loading
- **Runtime Availability**: Functions available immediately after connection
- **Per-Connection Basis**: Each connection loads its own extension instances

### Performance Considerations

- **Lazy Loading**: Libraries loaded with `RTLD_LAZY` for faster startup
- **Connection Overhead**: Extension loading adds ~50-100ms to connection time
- **Memory Usage**: Each connection maintains its own library handles
- **Thread Safety**: Extensions are loaded per-connection to avoid threading issues

### Troubleshooting Extension Loading

Common issues and solutions:

- **Permission Denied**: Ensure `.so` files have execute permissions
- **Library Not Found**: Check `/usr/local/lib/` path and library dependencies
- **Extension Loading Disabled**: Verify `SQLITE_DBCONFIG_ENABLE_LOAD_EXTENSION` succeeds
- **Function Not Available**: Confirm extensions loaded successfully via SQLite pragma

This dynamic loading approach provides flexibility but requires careful management of library lifecycles and error conditions.