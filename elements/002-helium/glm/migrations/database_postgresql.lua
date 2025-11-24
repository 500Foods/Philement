-- database_postgresql.lua

-- luacheck: no max line length

-- CHANGLOG
-- 2.1.0 - 2025-11-23 - Added DROP_CHECK to test for non-empty tables prior to drop
-- 2.0.0 - 2025-11-16 - Added BASE64_START and BASE64_END macros

-- NOTES
-- Base64 support provided natively by database

return {
    CHAR_2 = "char(2)",
    CHAR_20 = "char(20)",
    CHAR_50 = "char(50)",
    CHAR_128 = "char(128)",
    INTEGER = "integer",
    NOW = "CURRENT_TIMESTAMP",
    PRIMARY = "PRIMARY KEY",
    SERIAL = "serial",
    TEXT = "text",
    TEXT_BIG = "text",
    TIMESTAMP_TZ = "timestamptz",
    UNIQUE = "UNIQUE",
    VARCHAR_20 = "varchar(20)",
    VARCHAR_50 = "varchar(50)",
    VARCHAR_100 = "varchar(100)",
    VARCHAR_128 = "varchar(128)",
    VARCHAR_500 = "varchar(500)",

    BASE64_START = "CONVERT_FROM(DECODE(",
    BASE64_END = ", 'base64'), 'UTF8')",

    DROP_CHECK = "SELECT pg_catalog.pg_terminate_backend(pg_backend_pid()) FROM ${SCHEMA}${TABLE} LIMIT 1",

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