-- database_firebase.lua

-- luacheck: no max line length

-- CHANGELOG
-- 1.0.0 - 2026-09-16 - Initial Firebase / Cloud Firestore dialect (complete key set)

-- NOTES
-- Helium still emits SQL. Hydrogen's firebase engine interprets it against
-- Cloud Firestore. UDF-class functions (Base64, Brotli, SHA-256, JSON ingest,
-- NOW, CONVERT_TZ, JSON extract) run in-process in Hydrogen, not as Google
-- Cloud Functions.
-- ${SCHEMA} is a collection prefix with underscore (testfb_queries), not schema.

return {
    CHAR_2 = "text",
    CHAR_20 = "text",
    CHAR_50 = "text",
    CHAR_128 = "text",
    DATE = "timestamp",
    DATETIME = "timestamp",
    FLOAT = "real",
    FLOAT_BIG = "real",
    INSERT_KEY_START = "-- ",
    INSERT_KEY_END = "",
    INSERT_KEY_RETURN = "RETURNING ",
    INTEGER = "integer",
    INTEGER_BIG = "bigint",
    INTEGER_SMALL = "integer",
    JRS = "FB_JSON_VALUE(",
    JRM = ", ",
    JRE = ")",
    NOW = "FB_NOW()",
    PRIMARY = "PRIMARY KEY",
    REORG = "-- REORG TABLE",
    SERIAL = "integer",
    SESSION_SECS = "FB_SESSION_SECS(:SESSION_START)",
    SIZE_COLLECTION = "LENGTH(collection)",
    SIZE_INTEGER = "8",
    SIZE_INTEGER_BIG = "8",
    SIZE_INTEGER_SMALL = "8",
    SIZE_FLOAT = "8",
    SIZE_FLOAT_BIG = "8",
    SIZE_TIMESTAMP = "20",
    TEXT = "text",
    TEXT_BIG = "text",
    TIME = "timestamp",
    TIMESTAMP = "timestamp",
    TIMESTAMP_TZ = "timestamp",
    TRMS = "FB_TIME_ADD(${NOW}, -(",
    TRME = "), 'minutes')",
    TRFS = "FB_TIME_ADD(${NOW}, (",
    TRFE = "), 'seconds')",
    TRFMS = "FB_TIME_ADD(${NOW}, (",
    TRFME = "), 'minutes')",
    UNIQUE = "UNIQUE",
    VARCHAR_20 = "text",
    VARCHAR_50 = "text",
    VARCHAR_64 = "text",
    VARCHAR_100 = "text",
    VARCHAR_128 = "text",
    VARCHAR_500 = "text",

    -- Firebase interpreter does not require FROM for SELECT without tables
    DUMMY_TABLE = "",

    BASE64_START = "FB_BASE64_DECODE(",
    BASE64_END = ")",

    -- DB2-only encode macros must still exist so replace_query never leaves
    -- ${UNSUBSTITUTED}. Firebase has a single in-process encoder.
    BASE64ENCODE_START = "FB_BASE64_ENCODE(",
    BASE64ENCODE_END = ")",
    BASE64ENCODEBINARY_START = "FB_BASE64_ENCODE(",
    BASE64ENCODEBINARY_END = ")",

    -- Date/Time formatting (DB2-only keys; unused by Acuranzo QueryRefs today)
    DATETIME_FORMAT = "FB_NOW()",
    TIMESTAMP_FORMAT = "FB_NOW()",

    -- Password hash: FB_SHA256_B64(account_id, password)
    -- Concat as UTF-8, SHA-256, standard base64. Must match other engines.
    -- Usage: ${SHA256_HASH_START}'0'${SHA256_HASH_MID}'${HYDROGEN_DEMO_ADMIN_PASS}'${SHA256_HASH_END}
    SHA256_HASH_START = "FB_SHA256_B64(",
    SHA256_HASH_MID = ", ",
    SHA256_HASH_END = ")",

    COMPRESS_START = "FB_BROTLI_DECOMPRESS(FB_BASE64_DECODE(",
    COMPRESS_END = "))",

    DROP_CHECK = "SELECT FB_REFUSE_DROP('${SCHEMA}${TABLE}') WHERE EXISTS (SELECT 1 FROM ${SCHEMA}${TABLE})",

    BROTLI_DECOMPRESS_FUNCTION = [[
        -- firebase: FB_BROTLI_DECOMPRESS is in-process
    ]],

    CONVERT_TZ_FUNCTION = [[
        -- firebase: FB_CONVERT_TZ is in-process
    ]],

    JSON = "json",
    JIS = "FB_JSON_INGEST(",
    JIE = ")",
    JSON_INGEST_START = "FB_JSON_INGEST(",
    JSON_INGEST_END = ")",
    JSON_INGEST_FUNCTION = [[
        -- firebase: FB_JSON_INGEST is in-process
    ]],

    -- Schema ingest aliases ingest (PG/SQLite/MySQL, not DB2 JSON2BSON).
    -- $ref/$id/$schema are accepted; stored as JSON text (stringValue).
    JSON_INGEST_SCHEMA_START = "FB_JSON_INGEST(",
    JSON_INGEST_SCHEMA_END = ")",
    JSON_INGEST_SCHEMA_FUNCTION = ""
}
