-- database_db2.lua

-- luacheck: no max line length

-- CHANGLOG
-- 2.1.0 - 2025-11-23 - Added DROP_CHECK to test for non-empty tables prior to drop
-- 2.0.0 - 2025-11-16 - Added BASE64_START and BASE64_END macros

-- NOTES
-- Bse64 support provided via a C UDF found in extras

return {
    CHAR_2 = "CHAR(2)",
    CHAR_20 = "CHAR(20)",
    CHAR_50 = "CHAR(50)",
    CHAR_128 = "CHAR(128)",
    INTEGER = "INTEGER",
    NOW = "CURRENT TIMESTAMP",
    PRIMARY = "PRIMARY KEY",
    SERIAL = "INTEGER GENERATED ALWAYS AS IDENTITY",
    TEXT = "VARCHAR(250)",
    TEXT_BIG = "CLOB(1M)",
    TIMESTAMP_TZ = "TIMESTAMP",
    UNIQUE = "UNIQUE",
    VARCHAR_20 = "VARCHAR(20)",
    VARCHAR_50 = "VARCHAR(50)",
    VARCHAR_100 = "VARCHAR(100)",
    VARCHAR_128 = "VARCHAR(128)",
    VARCHAR_500 = "VARCHAR(500)",

    BASE64_START = "${SCHEMA}BASE64DECODE(",
    BASE64_END = ")",

    COMPRESS_START = "${SCHEMA}BROTLI_DECOMPRESS(${SCHEMA}BASE64DECODEBINARY(",
    COMPRESS_END = "))",

    DROP_CHECK = "BEGIN IF EXISTS(SELECT 1 FROM ${SCHEMA}${TABLE}) THEN SIGNAL SQLSTATE '75001' SET MESSAGE_TEXT='Refusing to drop table ${SCHEMA}${TABLE} – it contains data'; END IF; END",

    BROTLI_DECOMPRESS_FUNCTION = [[
        CREATE OR REPLACE FUNCTION ${SCHEMA}BROTLI_DECOMPRESS(compressed BLOB(2G))
        RETURNS CLOB(2G)
        LANGUAGE C
        PARAMETER STYLE DB2SQL
        NO SQL
        DETERMINISTIC
        NOT FENCED
        THREADSAFE
        RETURNS NULL ON NULL INPUT
        EXTERNAL NAME 'brotli_decompress.so!BROTLI_DECOMPRESS'
   ]],

    JSON = "CLOB(1M)",
    JIS = "${SCHEMA}JSON_INGEST(",
    JIE = ")",
    JSON_INGEST_START = "${SCHEMA}JSON_INGEST(",
    JSON_INGEST_END = ")",
    JSON_INGEST_FUNCTION = [[
        -- json.ingest for db2 attempt 4
        CREATE OR REPLACE FUNCTION ${SCHEMA}JSON_INGEST(s CLOB)
        RETURNS CLOB
        LANGUAGE SQL
        DETERMINISTIC
        BEGIN
            DECLARE i INTEGER DEFAULT 1;
            DECLARE L INTEGER;
            DECLARE ch CHAR(1);
            DECLARE out CLOB(10M) DEFAULT '';
            DECLARE in_str SMALLINT DEFAULT 0;
            DECLARE esc SMALLINT DEFAULT 0;

            SET L = LENGTH(s);

            -- fast path: Validate input JSON
            IF SYSTOOLS.JSON2BSON(s) IS NOT NULL THEN
                RETURN s;
            END IF;

            WHILE i <= L DO
                SET ch = SUBSTR(s, i, 1);

                IF esc = 1 THEN
                    SET out = out || ch;
                    SET esc = 0;

                ELSEIF ch = '\' THEN
                    SET out = out || ch;
                    SET esc = 1;

                ELSEIF ch = '"' THEN
                    SET out = out || ch;
                    SET in_str = 1 - in_str;

                ELSEIF in_str = 1 AND ch = X'0A' THEN -- \n
                    SET out = out || '\n';

                ELSEIF in_str = 1 AND ch = X'0D' THEN -- \r
                    SET out = out || '\r';

                ELSEIF in_str = 1 AND ch = X'09' THEN -- \t
                    SET out = out || '\t';

                ELSE
                    SET out = out || ch;
                END IF;

                SET i = i + 1;
            END WHILE;

            -- ensure result is JSON
            IF SYSTOOLS.JSON2BSON(out) IS NULL THEN
                SIGNAL SQLSTATE '22032' SET MESSAGE_TEXT = 'Invalid JSON after normalization';
            END IF;

            RETURN out;
        END
    ]]
}