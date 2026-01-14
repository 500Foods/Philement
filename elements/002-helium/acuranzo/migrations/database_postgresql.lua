-- database_postgresql.lua

-- luacheck: no max line length

-- CHANGELOG
-- 2.6.0 - 2025-12-31 - Added fancy INSERT_ macros to get our new key value returned
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
    INSERT_KEY_START = "-- ",
    INSERT_KEY_END = "",
    INSERT_KEY_RETURN = "RETURNING ",
    INTEGER = "integer",
    INTEGER_BIG = "bigint",
    INTEGER_SMALL = "smallint",
    FLOAT = "real",
    FLOAT_BIG = "double precision",
    JRS = "",
    JRM = "::json ->> ",
    JRE = "",
    NOW = "CURRENT_TIMESTAMP",
    PRIMARY = "PRIMARY KEY",
    SERIAL = "serial",
    SESSION_SECS = "EXTRACT(EPOCH FROM (CURRENT_TIMESTAMP - :SESSION_START))",
    SIZE_COLLECTION = "LENGTH(collection::text)",
    SIZE_FLOAT = "4",
    SIZE_FLOAT_BIG = "8",
    SIZE_INTEGER = "4",
    SIZE_INTEGER_BIG = "8",
    SIZE_INTEGER_SMALL = "2",
    SIZE_TIMESTAMP = "8",
    TEXT = "text",
    TEXT_BIG = "text",
    TIMESTAMP_TZ = "timestamptz",
    TRMS = "(${NOW} - ( ",
    TRME = " || ' minutes')::interval)",
    UNIQUE = "UNIQUE",
    VARCHAR_20 = "varchar(20)",
    VARCHAR_50 = "varchar(50)",
    VARCHAR_100 = "varchar(100)",
    VARCHAR_128 = "varchar(128)",
    VARCHAR_500 = "varchar(500)",

    -- PostgreSQL doesn't require FROM clause for SELECT without tables
    DUMMY_TABLE = "",

    BASE64_START = "CONVERT_FROM(DECODE(",
    BASE64_END = ", 'base64'), 'UTF8')",

    -- Password hash: ENCODE(SHA256(CONCAT(account_id, password)::bytea), 'base64')
    -- Returns base64-encoded SHA256 hash
    -- Usage: ${SHA256_HASH_START}'0'${SHA256_HASH_MID}'${HYDROGEN_DEMO_ADMIN_PASS}'${SHA256_HASH_END}
    SHA256_HASH_START = "ENCODE(SHA256(CONCAT(",
    SHA256_HASH_MID = ", ",
    SHA256_HASH_END = ")::bytea), 'base64')",

    COMPRESS_START = "${SCHEMA}brotli_decompress(DECODE(",
    COMPRESS_END = ", 'base64'))",

    DROP_CHECK = "SELECT pg_catalog.pg_terminate_backend(pg_backend_pid()) FROM ${SCHEMA}${TABLE} LIMIT 1",

    BROTLI_DECOMPRESS_FUNCTION = [[
        -- PostgreSQL C extension for Brotli decompression
        -- Requires: libbrotlidec and brotli_decompress.so in PostgreSQL lib directory
        -- Installation handled via extras/brotli_udf_postgresql/
        CREATE OR REPLACE FUNCTION ${SCHEMA}brotli_decompress(compressed bytea)
        RETURNS text
        AS 'brotli_decompress', 'brotli_decompress'
        LANGUAGE c STRICT IMMUTABLE;
    ]],

    JSON = "jsonb",
    JIS = "${SCHEMA}json_ingest (",
    JIE = ")",
    JSON_INGEST_START = "${SCHEMA}json_ingest (",
    JSON_INGEST_END = ")",
    JSON_INGEST_FUNCTION = [[
      CREATE OR REPLACE FUNCTION ${SCHEMA}json_ingest(s text)
      RETURNS jsonb
      LANGUAGE plpgsql
      STRICT
      STABLE
      AS $json_ingest_fn$
      DECLARE
        i      int := 1;
        L      int := length(s);
        ch     text;
        out    text := '';
        in_str boolean := false;
        esc    boolean := false;
      BEGIN
        -- Fast path: already valid JSON
        BEGIN
          RETURN s::jsonb;
        EXCEPTION WHEN others THEN
          -- fall through to fix-up pass
        END;

        -- Fix-up pass: escape only control chars *inside* JSON strings
        WHILE i <= L LOOP
          ch := substr(s, i, 1);

          IF esc THEN
            out := out || ch;
            esc := false;

          ELSIF ch = E'\\' THEN
            out := out || ch;
            esc := true;

          ELSIF ch = '"' THEN
            out := out || ch;
            in_str := NOT in_str;

          ELSIF in_str AND ch = E'\n' THEN
            out := out || E'\\n';

          ELSIF in_str AND ch = E'\r' THEN
            out := out || E'\\r';

          ELSIF in_str AND ch = E'\t' THEN
            out := out || E'\\t';

          ELSE
            out := out || ch;
          END IF;

          i := i + 1;
        END LOOP;

        -- Parse after fix-ups
        RETURN out::jsonb;
      END
      $json_ingest_fn$;  -- ← Tag matches exactly, semicolon attached
    ]]
}