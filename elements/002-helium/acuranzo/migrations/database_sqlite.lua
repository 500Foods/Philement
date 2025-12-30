-- database_sqlite.lua

-- luacheck: no max line length

-- CHANGELOG
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
    INTEGER = "integer",
    JRS = "json_extract(",
    JRM = ", ",
    JRE = ")",
    NOW = "CURRENT_TIMESTAMP",
    PRIMARY = "PRIMARY KEY",
    SERIAL = "integer AUTOINCREMENT",
    SESSION_SECS ="unixepoch(CURRENT_TIMESTAMP, 'subsec') - unixepoch(:SESSION_START, 'subsec')",
    TEXT = "text",
    TEXT_BIG = "text",
    TIMESTAMP_TZ = "text",
    TRMS = "datetime(${NOW}, '-' || ",
    TRME = " || ' minutes')",
    UNIQUE = "UNIQUE",
    VARCHAR_20 = "varchar(20)",
    VARCHAR_50 = "varchar(50)",
    VARCHAR_100 = "varchar(100)",
    VARCHAR_128 = "varchar(128)",
    VARCHAR_500 = "varchar(500)",

    BASE64_START = "CRYPTO_DECODE(",
    BASE64_END = ",'base64')",

    COMPRESS_START = "BROTLI_DECOMPRESS(CRYPTO_DECODE(",
    COMPRESS_END = ",'base64'))",

    DROP_CHECK = "SELECT 'Refusing to drop table ${SCHEMA}${TABLE} – it contains data' WHERE EXISTS (SELECT 1 FROM ${SCHEMA}${TABLE})",

    BROTLI_DECOMPRESS_FUNCTION = [[
        -- SQLite loadable extension for Brotli decompression
        -- Requires: brotli_decompress.so in SQLite extension directory
        -- Installation handled via extras/brotli_udf_sqlite/
        -- Extension will be loaded via: SELECT load_extension('brotli_decompress');
    ]],

    JSON = "text",
    JIS = "(",
    JIE = ")",
    JSON_INGEST_START = "(",
    JSON_INGEST_END = ")",
    JSON_INGEST_FUNCTION = ""
}