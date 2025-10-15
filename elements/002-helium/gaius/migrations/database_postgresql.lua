-- database_postgresql.lua
-- PostgreSQL-specific configuration for Helium schema

return {
    PRIMARY = "PRIMARY KEY",
    UNIQUE = "UNIQUE",
    SERIAL = "SERIAL",
    INTEGER = "INTEGER",
    VARCHAR_20 = "VARCHAR(20)",
    VARCHAR_50 = "VARCHAR(50)",
    VARCHAR_100 = "VARCHAR(100)",
    VARCHAR_128 = "VARCHAR(128)",
    VARCHAR_500 = "VARCHAR(500)",
    TEXT = "TEXT",
    BIGTEXT = "TEXT",
    JSONB = "JSONB",
    TIMESTAMP_TZ = "TIMESTAMPTZ",
    NOW = "CURRENT_TIMESTAMP",
    CHECK_CONSTRAINT = "CHECK(status IN ('Pending', 'Applied', 'Utility'))",
    JSON_INGEST_START = "${SCHEMA}json_ingest (",
    JSON_INGEST_END = ")",
    JSON_INGEST_FUNCTION = [[
        CREATE OR REPLACE FUNCTION ${SCHEMA}json_ingest(s TEXT)
        RETURNS JSONB
        LANGUAGE plpgsql
        STRICT
        STABLE
        AS $fn$
        DECLARE
          i      int := 1;
          L      int := length(s);
          ch     text;
          out    text := '';
          in_str boolean := false;  -- are we inside a "..." JSON string?
          esc    boolean := false;  -- previous char was a backslash
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
              -- we were in an escape sequence: keep this char verbatim
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
        $fn$;
    ]]
}