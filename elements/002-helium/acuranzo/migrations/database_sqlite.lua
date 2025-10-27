-- database_sqlite.lua
-- SQLite-specific configuration for Helium schema

-- NOTES
-- SQLite stores timestamps as TEXT in ISO format
-- SQLite doesn't have native JSON, so no need to deal with invalid JSON at this stage

return {
    CHAR_2 = "char(2)",
    INTEGER = "integer",
    NOW = "CURRENT_TIMESTAMP",
    PRIMARY = "PRIMARY KEY",
    SERIAL = "integer AUTOINCREMENT",
    TEXT = "text",
    TEXTBIG = "text",
    TIMESTAMP_TZ = "text",
    UNIQUE = "UNIQUE",
    VARCHAR_20 = "varchar(20)",
    VARCHAR_50 = "varchar(50)",
    VARCHAR_100 = "varchar(100)",
    VARCHAR_128 = "varchar(128)",
    VARCHAR_500 = "varchar(500)",

    JSON = "text",
    JSON_INGEST_START = "(",
    JSON_INGEST_END = ")",
    JSON_INGEST_FUNCTION = ""
}