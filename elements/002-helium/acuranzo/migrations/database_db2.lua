-- database_db2.lua
-- DB2-specific configuration for Helium schema

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

    JSON = "CLOB(1M)",
    JSON_INGEST_START = "${SCHEMA}json_ingest(",
    JSON_INGEST_END = ")",
    JSON_INGEST_FUNCTION = [[
        -- json.ingest for db2 attempt 3
        CREATE OR REPLACE FUNCTION ${SCHEMA}json_ingest(s CLOB)
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