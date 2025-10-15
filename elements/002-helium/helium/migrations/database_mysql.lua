-- database_mysql.lua
-- MySQL-specific configuration for Helium schema

return {
    SERIAL = "INT AUTO_INCREMENT",
    INTEGER = "INT",
    VARCHAR_20 = "VARCHAR(20)",
    VARCHAR_50 = "VARCHAR(50)",
    VARCHAR_100 = "VARCHAR(100)",
    VARCHAR_128 = "VARCHAR(128)",
    VARCHAR_500 = "VARCHAR(500)",
    TEXT = "TEXT",
    BIGTEXT = "TEXT",
    JSONB = "LONGTEXT CHARACTER SET utf8mb4 COLLATE utf8mb4_bin", -- because JSON_VALID doesn't pass our JSON properly
    TIMESTAMP_TZ = "TIMESTAMP",
    NOW = "CURRENT_TIMESTAMP",
    CHECK_CONSTRAINT = "ENUM('Pending', 'Applied', 'Utility')",
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