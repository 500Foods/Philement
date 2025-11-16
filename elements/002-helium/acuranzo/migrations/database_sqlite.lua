-- database_sqlite.lua
-- SQLite-specific configuration for Helium schema

-- CHANGLOG
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
    NOW = "CURRENT_TIMESTAMP",
    PRIMARY = "PRIMARY KEY",
    SERIAL = "integer AUTOINCREMENT",
    TEXT = "text",
    TEXT_BIG = "text",
    TIMESTAMP_TZ = "text",
    UNIQUE = "UNIQUE",
    VARCHAR_20 = "varchar(20)",
    VARCHAR_50 = "varchar(50)",
    VARCHAR_100 = "varchar(100)",
    VARCHAR_128 = "varchar(128)",
    VARCHAR_500 = "varchar(500)",

    BASE64_START = "CRYPTO_DECODE(",
    BASE64_END = ",'base64')",

    JSON = "text",
    JSON_INGEST_START = "(",
    JSON_INGEST_END = ")",
    JSON_INGEST_FUNCTION = ""
}