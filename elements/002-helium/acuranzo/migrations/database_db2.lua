-- database_db2.lua

-- luacheck: no max line length

-- CHANGELOG
-- 2.6.0 - 2025-12-31 - Added fancy INSERT_ macros to get our new key value returned
-- 2.5.0 - 2025-12-31 - Added SIZE_ macros
-- 2.4.0 - 2025-12-30 - Added JRS, JRM, JRE macros (JSON Retrieval Start, Middle, End)
-- 2.3.0 - 2025-12-29 - Added SESSION_SECS macro for session duration calculation
-- 2.2.0 - 2025-12-28 - Added TRMS and TRME macros for time calculations (Time Range Minutes Start/End)
-- 2.1.0 - 2025-11-23 - Added DROP_CHECK to test for non-empty tables prior to drop
-- 2.0.0 - 2025-11-16 - Added BASE64_START and BASE64_END macros

-- NOTES
-- Base64 decode support provided via a C UDF in extras/base64decode_udf_db2
-- Base64 encode support provided via a C UDF in extras/base64encode_udf_db2
-- Brotli decompression provided via a C UDF in extras/brotli_udf_db2
-- Source for all UDFs can be found in the elements/001-hydrogen/hydrogen/extras directory
-- Unlike other databases, DB2 requires explicit sizes for JSON/VARCHAR/CLOB types
-- This currently uses 100K for this kind of thing - COLLECTION_SIZE, TEXT_BIG, etc.

return {
    CHAR_2 = "CHAR(2)",
    CHAR_20 = "CHAR(20)",
    CHAR_50 = "CHAR(50)",
    CHAR_128 = "CHAR(128)",
    FLOAT = "REAL",
    FLOAT_BIG = "DOUBLE",
    INSERT_KEY_START = "SELECT ",
    INSERT_KEY_END = " FROM FINAL TABLE (",
    INSERT_KEY_RETURN = "); -- ",
    INTEGER = "INTEGER",
    INTEGER_BIG = "BIGINT",
    INTEGER_SMALL = "SMALLINT",
    JRS = "JSON_VALUE(",
    JRM = ", ",
    JRE = " DEFAULT NULL ON ERROR)",
    NOW = "CURRENT TIMESTAMP",
    PRIMARY = "PRIMARY KEY",
    SERIAL = "INTEGER GENERATED ALWAYS AS IDENTITY",
    SESSION_SECS ="(TIMESTAMPDIFF(2, TIMESTAMP(:SESSION_START) - CURRENT TIMESTAMP) * -1) / 1000000",
    SIZE_COLLECTION = "COALESCE(LENGTH(collection), 0)",
    SIZE_FLOAT = "4",
    SIZE_FLOAT_BIG = "4",
    SIZE_INTEGER = "4",
    SIZE_INTEGER_BIG = "8",
    SIZE_INTEGER_SMALL = "2",
    SIZE_TIMESTAMP = "10",
    TEXT = "VARCHAR(250)",
    TEXT_BIG = "CLOB(100K)",
    TIMESTAMP_TZ = "TIMESTAMP",
    TRMS = "${NOW} - ( ",
    TRME = " ) MINUTES",
    UNIQUE = "UNIQUE",
    VARCHAR_20 = "VARCHAR(20)",
    VARCHAR_50 = "VARCHAR(50)",
    VARCHAR_100 = "VARCHAR(100)",
    VARCHAR_128 = "VARCHAR(128)",
    VARCHAR_500 = "VARCHAR(500)",

    -- DB2 requires FROM clause for SELECT without tables
    DUMMY_TABLE = "FROM SYSIBM.SYSDUMMY1",

    BASE64_START = "${SCHEMA}BASE64DECODE(",
    BASE64_END = ")",

    BASE64ENCODE_START = "${SCHEMA}BASE64ENCODE(",
    BASE64ENCODE_END = ")",

    BASE64ENCODEBINARY_START = "${SCHEMA}BASE64ENCODEBINARY(",
    BASE64ENCODEBINARY_END = ")",

    -- Password hash: CAST(BASE64ENCODEBINARY(HASH(CONCAT(account_id, password), 2)) AS CHAR(128))
    -- Returns base64-encoded SHA256 hash as CHAR(128) for password_hash column
    -- HASH(data, 2) where 2 = SHA256 algorithm; CAST to CHAR(128) for column compatibility
    -- Usage: ${SHA256_HASH_START}'0'${SHA256_HASH_MID}'${HYDROGEN_DEMO_ADMIN_PASS}'${SHA256_HASH_END}
    SHA256_HASH_START = "CAST(${SCHEMA}BASE64ENCODEBINARY(HASH(CAST(CONCAT(",
    SHA256_HASH_MID = ", ",
    SHA256_HASH_END = ") AS VARCHAR(256) FOR BIT DATA), 2)) AS CHAR(128))",

    COMPRESS_START = "${SCHEMA}BROTLI_DECOMPRESS(${SCHEMA}BASE64DECODEBINARY(",
    COMPRESS_END = "))",

    DROP_CHECK = "BEGIN IF EXISTS(SELECT 1 FROM ${SCHEMA}${TABLE}) THEN SIGNAL SQLSTATE '75001' SET MESSAGE_TEXT='Refusing to drop table ${SCHEMA}${TABLE} – it contains data'; END IF; END",

    BROTLI_DECOMPRESS_FUNCTION = [[
        CREATE OR REPLACE FUNCTION ${SCHEMA}BROTLI_DECOMPRESS(compressed BLOB(100K))
        RETURNS CLOB(100K)
        LANGUAGE C
        PARAMETER STYLE DB2SQL
        NO SQL
        DETERMINISTIC
        NOT FENCED
        THREADSAFE
        RETURNS NULL ON NULL INPUT
        EXTERNAL NAME 'brotli_decompress.so!BROTLI_DECOMPRESS'
   ]],

    JSON = "CLOB(100K)",
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
            DECLARE out CLOB(100K) DEFAULT '';
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