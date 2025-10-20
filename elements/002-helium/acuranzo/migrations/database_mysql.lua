-- database_mysql.lua
-- MySQL-specific configuration for Helium schema

return {
    INTEGER = "int",
    NOW = "CURRENT_TIMESTAMP",
    PRIMARY = "primary key",
    SERIAL = "int auto increment",
    TEXT = "text",
    TEXTBIG = "text",
    TIMESTAMP_TZ = "timestamp",
    UNIQUE = "unique key",
    VARCHAR_20 = "varchar(20)",
    VARCHAR_50 = "varchar(50)",
    VARCHAR_100 = "varchar(100)",
    VARCHAR_128 = "varchar(128)",
    VARCHAR_500 = "varchar(500)",

    JSON = "longtest characterset utf8mb4 collate utf8mb4_bin", -- because JSON_VALID doesn't pass our JSON properly
    JSON_INGEST_START = "${SCHEMA}json_ingest(",
    JSON_INGEST_END = ")",
    JSON_INGEST_FUNCTION = [[
        CREATE OR REPLACE FUNCTION json_ingest(s LONGTEXT)
        RETURNS LONGTEXT
        DETERMINISTIC
        BEGIN
          DECLARE fixed LONGTEXT DEFAULT '';
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