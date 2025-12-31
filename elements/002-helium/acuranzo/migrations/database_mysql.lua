-- database_mysql.lua

-- luacheck: no max line length

-- CHANGELOG
-- 2.5.0 - 2025-12-31 - Added SIZE_ macros
-- 2.4.0 - 2025-12-30 - Added JRS, JRM, JRE macros for JSON value retrieval
-- 2.3.0 - 2025-12-29 - Added SESSION_SECS macro for session duration calculation
-- 2.2.0 - 2025-12-28 - Added TRMS and TRME macros for time calculations (Time Range Minutes Start/End)
-- 2.1.0 - 2025-11-23 - Added DROP_CHECK to test for non-empty tables prior to drop
-- 2.0.0 - 2025-11-16 - Added BASE64_START and BASE64_END macros

-- NOTES
-- Base64 support provided natively by database

return {
    CHAR_2 = "char(2)",
    CHAR_20 = "char(20)",
    CHAR_50 = "char(50)",
    CHAR_128 = "char(128)",
    FLOAT = "float",
    FLOAT_BIG = "double",
    INTEGER = "int",
    INTEGER_BIG = "bigint",
    INTEGER_SMALL = "smallint",
    JRS = "",
    JRM = " ->> ",
    JRE = "",
    NOW = "CURRENT_TIMESTAMP(3)",
    PRIMARY = "PRIMARY KEY",
    SERIAL = "int auto increment",
    SESSION_SECS = "UNIX_TIMESTAMP(CURRENT_TIMESTAMP) - UNIX_TIMESTAMP(:SESSION_START)",
    SIZE_COLLECTION = "LENGTH(collection)",
    SIZE_FLOAT = "4",
    SIZE_FLOAT_BIG = "8",
    SIZE_INTEGER = "4",
    SIZE_INTEGER_BIG = "8",
    SIZE_INTEGER_SMALL = "2",
    SIZE_TIMESTAMP = "7",
    TEXT = "text",
    TEXT_BIG = "text",
    TIMESTAMP_TZ = "datetime(3)",
    TRMS = "DATE_SUB(${NOW}, INTERVAL ",
    TRME = " MINUTE)",
    UNIQUE = "UNIQUE",
    VARCHAR_20 = "varchar(20)",
    VARCHAR_50 = "varchar(50)",
    VARCHAR_100 = "varchar(100)",
    VARCHAR_128 = "varchar(128)",
    VARCHAR_500 = "varchar(500)",

    BASE64_START = "cast(FROM_BASE64(",
    BASE64_END = ") as char character set utf8mb4)",

    COMPRESS_START = "brotli_decompress(FROM_BASE64(",
    COMPRESS_END = "))",

    DROP_CHECK = " DO IF(EXISTS(SELECT 1 FROM ${SCHEMA}${TABLE}), cast('Refusing to drop table ${SCHEMA}${TABLE} – it contains data' AS CHAR(0)), NULL)",

    -- MySQL UDF for Brotli decompression
    -- Requires: libbrotli-dev and brotli_decompress.so in plugin directory
    -- Installation handled via extras/brotli_udf_mysql/
    -- DROP FUNCTION IF EXISTS brotli_decompress;
    -- NOTE: Drop function added to migration 1000 so that this is a single statement for the preparation phase
    BROTLI_DECOMPRESS_FUNCTION = [[
        CREATE FUNCTION brotli_decompress RETURNS STRING SONAME 'brotli_decompress.so';
    ]],

    JSON = "longtext",
    JIS = "${SCHEMA}json_ingest(",
    JIE = ")",
    JSON_INGEST_START = "${SCHEMA}json_ingest(",
    JSON_INGEST_END = ")",
    JSON_INGEST_FUNCTION = [[
        CREATE OR REPLACE FUNCTION json_ingest(s longtext)
        RETURNS longtext
        DETERMINISTIC
        BEGIN
          DECLARE fixed longtext DEFAULT '';
          DECLARE i INT DEFAULT 1;
          DECLARE L INT DEFAULT CHAR_LENGTH(s);
          DECLARE ch CHAR(1);
          DECLARE in_str BOOL DEFAULT FALSE;
          DECLARE esc BOOL DEFAULT FALSE;
          -- fast path: check validity without exception
          IF JSON_VALID(s) THEN
            RETURN s;
          END IF;
          -- fallback: escape control chars in strings
          WHILE i <= L DO
            SET ch = SUBSTRING(s, i, 1);
            IF esc THEN
              SET fixed = CONCAT(fixed, ch);
              SET esc = FALSE;
            ELSEIF ch = '\\' THEN
              SET fixed = CONCAT(fixed, ch);
              SET esc = TRUE;
            ELSEIF ch = '''' THEN
              SET fixed = CONCAT(fixed, ch);
              SET in_str = NOT in_str;
            ELSEIF in_str AND ch = '\n' THEN
              SET fixed = CONCAT(fixed, '\\n');
            ELSEIF in_str AND ch = '\r' THEN
              SET fixed = CONCAT(fixed, '\\r');
            ELSEIF in_str AND ch = '\t' THEN
              SET fixed = CONCAT(fixed, '\\t');
            ELSEIF in_str AND ORD(ch) < 32 THEN
              -- General escape for other control chars (U+0000-U+001F) as \u00xx
              SET fixed = CONCAT(fixed, CONCAT('\\u00', LPAD(HEX(ORD(ch)), 2, '0')));
            ELSE
              SET fixed = CONCAT(fixed, ch);
            END IF;
            SET i = i + 1;
          END WHILE;
          RETURN fixed;
        END;
    ]]
}