-- database_mysql.lua
-- MySQL-specific configuration for Helium schema

return {
    CHAR_2 = "char(2)",
    CHAR_20 = "char(20)",
    CHAR_50 = "char(50)",
    CHAR_128 = "char(128)",
    INTEGER = "int",
    NOW = "CURRENT_TIMESTAMP",
    PRIMARY = "PRIMARY KEY",
    SERIAL = "int auto increment",
    TEXT = "text",
    TEXT_BIG = "text",
    TIMESTAMP_TZ = "timestamp",
    UNIQUE = "UNIQUE",
    VARCHAR_20 = "varchar(20)",
    VARCHAR_50 = "varchar(50)",
    VARCHAR_100 = "varchar(100)",
    VARCHAR_128 = "varchar(128)",
    VARCHAR_500 = "varchar(500)",

    BASE64_START = "CAST(FROM_BASE64(",
    BASE64_END = ") AS CHAR CHARACTER SET utf8mb4)",

    -- JSON = "longtext characterset utf8mb4 collate utf8mb4_bin", -- because JSON_VALID doesn't pass our JSON properly
    JSON = "longtext",
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