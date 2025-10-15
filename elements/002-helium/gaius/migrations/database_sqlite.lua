-- database_sqlite.lua
-- SQLite-specific configuration for Helium schema

return {
    SERIAL = "INTEGER PRIMARY KEY AUTOINCREMENT",
    INTEGER = "INTEGER",
    VARCHAR_20 = "VARCHAR(20)",
    VARCHAR_50 = "VARCHAR(50)",
    VARCHAR_100 = "VARCHAR(100)",
    VARCHAR_128 = "VARCHAR(128)",
    VARCHAR_500 = "VARCHAR(500)",
    TEXT = "TEXT",
    BIGTEXT = "TEXT",
    JSONB = "TEXT", -- SQLite doesn't have native JSON, store as TEXT
    TIMESTAMP_TZ = "TEXT", -- SQLite stores as TEXT in ISO format
    NOW = "CURRENT_TIMESTAMP",
    CHECK_CONSTRAINT = "CHECK(status IN ('Pending', 'Applied', 'Utility'))",
    JSON_INGEST_START = "(",
    JSON_INGEST_END = ")",
    JSON_INGEST_FUNCTION = ""
}