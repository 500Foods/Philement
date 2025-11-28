# Brotli Compression for Database Migrations

## Overview

This document describes the Brotli compression implementation for large base64-encoded strings in the Helium migration system. The implementation automatically compresses multiline strings larger than 1KB before encoding them in base64, reducing storage size by approximately 70-80% for text-heavy content like CSS, JSON, and HTML.

## Architecture

### Data Flow

```flow
Migration String (>1KB)
  ↓
Lua Brotli Compress (quality 11)
  ↓
Base64 Encode
  ↓
Wrap: BROTLI_DECOMPRESS(BASE64_DECODE('...'))
  ↓
Database stores nested function call
  ↓
On retrieval: Database executes nested functions
  ↓
Original string restored
```

### Compression Results

Example data from migration 1067 (CSS Theme):

- Original CSS: 21,000 bytes
- Base64 only: 28,000 bytes (+33% bloat)
- **Brotli + Base64: 5,900 bytes (-72% vs base64, -79% vs original)**

## Implementation Details

### 1. Lua-Side Compression (database.lua)

#### Compression Function

Location: `elements/002-helium/acuranzo/migrations/database.lua:256-272`

```lua
local function brotli_compress(data)
    local success, brotli = pcall(require, "brotli")
    if not success then
        error("Brotli compression library not available")
    end
    
    -- Compress with maximum quality (11) for best compression ratio
    local compressed = brotli.compress(data, 11)
    return compressed
end
```

**Requirements:**

- Lua Brotli library: `luarocks install lua-brotli`

#### Size Threshold

```lua
local COMPRESSION_THRESHOLD = 1024  -- 1KB
```

Strings smaller than 1KB are not compressed to avoid overhead on small content.

#### Encoding Logic

Location: `elements/002-helium/acuranzo/migrations/database.lua:328-358`

The encoding logic checks content size and conditionally applies compression before base64 encoding:

```lua
-- Check if content exceeds compression threshold
local content_size = #stripped_content
local should_compress = content_size > COMPRESSION_THRESHOLD

-- Compress if needed, then encode
local data_to_encode = stripped_content
if should_compress then
    data_to_encode = brotli_compress(stripped_content)
end

local encoded = "'" .. base64_encode(data_to_encode) .. "'"
```

### 2. Database-Specific Implementations

#### MySQL

**Functions:**

- `BROTLI_DECOMPRESS()` - C UDF for decompression

**Macros:** (database_mysql.lua)

```lua
COMPRESS_START = "${SCHEMA}BROTLI_DECOMPRESS(",
COMPRESS_END = ")",
BROTLI_DECOMPRESS_FUNCTION = [[
    CREATE FUNCTION IF NOT EXISTS ${SCHEMA}BROTLI_DECOMPRESS
    RETURNS STRING
    SONAME 'brotli_decompress.so';
]]
```

**Requirements:**

- libbrotli-dev package
- brotli_decompress.so in MySQL plugin directory
- See: `extras/brotli_udf_mysql/README.md`

#### PostgreSQL

**Functions:**

- `brotli_decompress()` - C extension function

**Macros:** (database_postgresql.lua)

```lua
COMPRESS_START = "${SCHEMA}brotli_decompress(",
COMPRESS_END = ")",
BROTLI_DECOMPRESS_FUNCTION = [[
    CREATE OR REPLACE FUNCTION ${SCHEMA}brotli_decompress(compressed bytea)
    RETURNS text
    AS 'brotli_decompress', 'brotli_decompress'
    LANGUAGE C STRICT IMMUTABLE;
]]
```

**Requirements:**

- libbrotlidec library
- brotli_decompress.so in PostgreSQL lib directory
- See: `extras/brotli_udf_postgresql/README.md`

#### SQLite

**Functions:**

- `BROTLI_DECOMPRESS()` - Loadable C extension

**Macros:** (database_sqlite.lua)

```lua
COMPRESS_START = "BROTLI_DECOMPRESS(",
COMPRESS_END = ")",
```

**Requirements:**

- brotli_decompress.so loadable extension
- Load with: `SELECT load_extension('brotli_decompress');`
- See: `extras/brotli_udf_sqlite/README.md`

#### DB2

**Functions:**

- `BROTLI_DECOMPRESS_CHUNK()` - C UDF for chunked decompression
- `BROTLI_DECOMPRESS()` - SQL wrapper for large CLOBs

**Macros:** (database_db2.lua)

```lua
COMPRESS_START = "${SCHEMA}BROTLI_DECOMPRESS(",
COMPRESS_END = ")",
```

**Architecture:**
DB2 follows the same pattern as BASE64DECODE with chunked processing:

1. `BROTLI_DECOMPRESS_CHUNK()` - Processes 32KB chunks
2. `BROTLI_DECOMPRESS()` - Wrapper that chunks large CLOBs

**Requirements:**

- libbrotlidec library
- brotli_decompress.so in sqllib/function
- See: `extras/brotli_udf_db2/README.md`

### 3. Migration Integration

#### Function Declaration (acuranzo_1000.lua)

The Brotli decompression functions are declared in migration 1000, following the same pattern as JSON_INGEST:

```lua
-- Brotli decompression functions for all engines (except SQLite loadable extension)
if engine ~= 'sqlite' then table.insert(queries,{sql=[[
    ${BROTLI_DECOMPRESS_FUNCTION}
]]}) end

-- DB2 requires additional wrapper function
if engine == 'db2' then table.insert(queries,{sql=[[
    ${BROTLI_DECOMPRESS_WRAPPER}
]]}) end
```

#### Nested Function Calls

When compression is used, the resulting SQL includes nested function calls:

```sql
-- For compressed content:
BROTLI_DECOMPRESS(BASE64_DECODE('base64_encoded_compressed_data'))

-- For non-compressed content (< 1KB):
BASE64_DECODE('base64_encoded_data')
```

## Installation Guide

### 1. Lua Environment

```bash
# Install lua-brotli library
luarocks install lua-brotli
```

### 2. MySQL

```bash
cd elements/001-hydrogen/hydrogen/extras/brotli_udf_mysql
make
sudo make install
# Restart MySQL or load UDF dynamically
```

### 3. PostgreSQL

```bash
cd elements/001-hydrogen/hydrogen/extras/brotli_udf_postgresql
make
sudo make install
# Function created via migration 1000
```

### 4. SQLite

```bash
cd elements/001-hydrogen/hydrogen/extras/brotli_udf_sqlite
make
sudo make install
# Extension loaded automatically via connection initialization
```

### 5. DB2

```bash
cd elements/001-hydrogen/hydrogen/extras/brotli_udf_db2
export DB2DIR=/opt/ibm/db2/V12.1  # Adjust for your installation
make
make install DB=YOURDB SCHEMA=YOURSCHEMA
```

## Testing

### Test with Migration 1067

Migration 1067 contains a large CSS theme (~21KB) that provides an excellent test case:

```lua
-- Location: elements/002-helium/acuranzo/migrations/acuranzo_1067.lua
-- Contains ~21KB CSS that compresses to ~5.9KB (with base64)
```

### Test Commands

```bash
# Run migration 1067
./run_migration.sh 1067 forward mysql

# Verify compression in queries table
mysql -e "SELECT LENGTH(code) FROM queries WHERE query_ref = 1067;"
# Should show ~5900 bytes instead of ~28000

# Verify decompression works
mysql -e "SELECT code FROM queries WHERE query_ref = 1067 LIMIT 1;"
# Should return original CSS
```

### Integration Tests

Each database engine should be tested:

```bash
# SQLite
./test_migration.sh sqlite 1067

# PostgreSQL
./test_migration.sh postgresql 1067

# MySQL
./test_migration.sh mysql 1067

# DB2
./test_migration.sh db2 1067
```

## Troubleshooting

### Lua Errors

**Error:** `Brotli compression library not available`
**Solution:** Install lua-brotli: `luarocks install lua-brotli`

### MySQL Errors

**Error:** `Can't open shared library 'brotli_decompress.so'`
**Solution:**

1. Check plugin directory: `SHOW VARIABLES LIKE 'plugin_dir';`
2. Copy .so file to plugin directory
3. Verify file permissions (755)

### PostgreSQL Errors

**Error:** `function brotli_decompress does not exist`
**Solution:**

1. Verify brotli_decompress.so in PostgreSQL lib directory
2. Check: `SELECT setting FROM pg_settings WHERE name = 'dynamic_library_path';`
3. Copy .so to appropriate directory
4. Re-run migration 1000

**Error:** `could not load library "brotli_decompress.so"`
**Solution:**

1. Install libbrotlidec: `apt-get install libbrotli-dev` or `yum install brotli-devel`
2. Rebuild and reinstall the extension
3. Verify library path: `ldd /path/to/brotli_decompress.so`

### SQLite Errors

**Error:** `no such function: BROTLI_DECOMPRESS`
**Solution:** Load extension: `SELECT load_extension('brotli_decompress');`

### DB2 Errors

**Error:** `SQLCODE=-204, SQLSTATE=42704, SQLERRMC=BROTLI_DECOMPRESS`
**Solution:**

1. Verify .so in sqllib/function directory
2. Check function registration in schema
3. Verify libbrotlidec installed

## Performance Considerations

### Compression Trade-offs

- **CPU**: More CPU time for compression/decompression
- **Storage**: 70-80% reduction in database size
- **Network**: Reduced data transfer during migration
- **Memory**: Temporary buffer for compression

### When Compression is Applied

- **Applied**: String > 1024 bytes
- **Skipped**: String ≤ 1024 bytes
- **Rationale**: Avoid overhead on small strings

### Compression Quality

- **Quality Level**: 11 (maximum)
- **Rationale**: Maximum compression for archival data
- **Trade-off**: Slower compression, faster decompression

## Future Enhancements

### Potential Improvements

1. **Configurable Threshold**: Allow adjustment of 1KB threshold
2. **Compression Algorithm**: Support multiple algorithms (zstd, lz4)
3. **Selective Compression**: Per-migration compression control
4. **Compression Metrics**: Track compression ratios and performance
5. **Decompression Caching**: Cache decompressed results

### Compatibility

- **Backward Compatible**: Non-compressed migrations continue working
- **Forward Compatible**: Compressed data readable by older systems (after UDF installation)
- **Migration Order**: Install UDFs before running compressed migrations

## Version History

- **3.0.0** (2025-11-27) - Initial Brotli compression implementation
  - Add compression for strings > 1KB
  - Implement UDF declarations for all 4 database engines
  - Add nested function wrapper support
  - Update migration 1000 for UDF registration

## References

- Brotli Algorithm: <https://github.com/google/brotli>
- lua-brotli: <https://github.com/sjnam/lua-brotli>
- Database Migration System: `database.lua`
- Example Migration: `acuranzo_1067.lua` (CSS Theme)