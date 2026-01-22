-- database_sqlite.lua

-- luacheck: no max line length

-- CHANGELOG
-- 2.6.0 - 2025-12-31 - Added fancy INSERT_ macros to get our new key value returned
-- 2.5.0 - 2025-12-31 - Added SIZE_ macros
-- 2.4.0 - 2025-12-30 - Added JRS, JRM, JRE macros for JSON value retrieval
-- 2.3.0 - 2025-12-29 - Added SESSION_SECS macro for session duration calculation
-- 2.2.0 - 2025-12-28 - Added TRMS and TRME macros for time calculations (Time Range Minutes Start/End)
-- 2.1.0 - 2025-11-23 - Added DROP_CHECK to test for non-empty tables prior to drop
-- 2.0.0 - 2025-11-16 - Added BASE64_START and BASE64_END macros

-- NOTES
-- SQLite stores timestamps as TEXT in ISO format
-- SQLite doesn't have native JSON, so no need to deal with invalid JSON at this stage
-- Base64 support provided by way of sqlean's crypto.so extension in /usr/local/lib/crypto.so

return {
    CHAR_2 = "char(2)",
    CHAR_20 = "char(20)",
    CHAR_50 = "char(50)",
    CHAR_128 = "char(128)",
    DATE = "text",
    DATETIME = "text",
    FLOAT = "real",
    FLOAT_BIG = "real",
    INSERT_KEY_START = "-- ",
    INSERT_KEY_END = "",
    INSERT_KEY_RETURN = "RETURNING ",
    INTEGER = "integer",
    INTEGER_BIG = "integer",
    INTEGER_SMALL = "integer",
    JRS = "json_extract(",
    JRM = ", ",
    JRE = ")",
    NOW = "CURRENT_TIMESTAMP",
    PRIMARY = "PRIMARY KEY",
    SERIAL = "integer AUTOINCREMENT",
    SESSION_SECS ="unixepoch(CURRENT_TIMESTAMP, 'subsec') - unixepoch(:SESSION_START, 'subsec')",
    SIZE_COLLECTION = "LENGTH(collection)",
    SIZE_INTEGER = "8",
    SIZE_INTEGER_BIG = "8",
    SIZE_INTEGER_SMALL = "8",
    SIZE_FLOAT = "8",
    SIZE_FLOAT_BIG = "8",
    SIZE_TIMESTAMP = "20",
    TEXT = "text",
    TEXT_BIG = "text",
    TIME = "text",
    TIMESTAMP = "text",
    TIMESTAMP_TZ = "text",
    TRMS = "datetime(${NOW}, '-' || ",
    TRME = " || ' minutes')",
    UNIQUE = "UNIQUE",
    VARCHAR_20 = "varchar(20)",
    VARCHAR_50 = "varchar(50)",
    VARCHAR_100 = "varchar(100)",
    VARCHAR_128 = "varchar(128)",
    VARCHAR_500 = "varchar(500)",

    -- SQLite doesn't require FROM clause for SELECT without tables
    DUMMY_TABLE = "",

    BASE64_START = "CRYPTO_DECODE(",
    BASE64_END = ",'base64')",

    -- Password hash: crypto_encode(crypto_sha256(account_id || password), 'base64')
    -- Uses sqlean crypto.so extension with crypto_sha256() direct hash function
    -- crypto_sha256() returns binary blob directly, which crypto_encode converts to base64
    -- Returns base64-encoded SHA256 hash matching DB2's CHAR(128) format
    -- Usage: ${SHA256_HASH_START}'0'${SHA256_HASH_MID}'${HYDROGEN_DEMO_ADMIN_PASS}'${SHA256_HASH_END}
    -- Note: SQLite version uses || for concatenation (not comma like DB2)
    SHA256_HASH_START = "crypto_encode(crypto_sha256(",
    SHA256_HASH_MID = " || ",
    SHA256_HASH_END = "), 'base64')",

    COMPRESS_START = "BROTLI_DECOMPRESS(CRYPTO_DECODE(",
    COMPRESS_END = ",'base64'))",

    DROP_CHECK = "SELECT 'Refusing to drop table ${SCHEMA}${TABLE} – it contains data' WHERE EXISTS (SELECT 1 FROM ${SCHEMA}${TABLE})",

    BROTLI_DECOMPRESS_FUNCTION = [[
        -- SQLite loadable extension for Brotli decompression
        -- Requires: brotli_decompress.so in SQLite extension directory
        -- Installation handled via extras/brotli_udf_sqlite/
        -- Extension will be loaded via: SELECT load_extension('brotli_decompress');
    ]],

    CONVERT_TZ_FUNCTION = [[
        -- SQLite loadable extension for timezone conversion
        -- Requires: convert_tz.so in SQLite extension directory
        -- Installation handled via extras/converttz_udf_sqlite/
        -- Extension will be loaded via: SELECT load_extension('convert_tz');
    ]],

    JSON = "text",
    JIS = "(",
    JIE = ")",
    JSON_INGEST_START = "(",
    JSON_INGEST_END = ")",
    JSON_INGEST_FUNCTION = ""
}