CREATE TABLE app.queries (
    query_id SERIAL,
    query_ref INTEGER NOT NULL,
    query_type_lua_28 INTEGER NOT NULL,
    query_dialect_lua_30 INTEGER NOT NULL,
    name VARCHAR(100) NOT NULL,
    summary TEXT,
    query_code TEXT NOT NULL,
    query_status_lua_27 INTEGER NOT NULL,
    collection JSONB,
    valid_after TIMESTAMP WITH TIME ZONE,
    valid_until TIMESTAMP WITH TIME ZONE,
    created_id INTEGER NOT NULL,
    created_at TIMESTAMP WITH TIME ZONE NOT NULL,
    updated_id INTEGER NOT NULL,
    updated_at TIMESTAMP WITH TIME ZONE NOT NULL
);
CREATE SCHEMA IF NOT EXISTS app;
CREATE OR REPLACE FUNCTION app.json_ingest(s text)
RETURNS jsonb
LANGUAGE plpgsql
STRICT
STABLE
AS $fn$
DECLARE
i      int := 1;
L      int := length(s);
ch     text;
out    text := '';
in_str boolean := false;                -- are we inside a "..." JSON string?
esc    boolean := false;                -- previous char was a backslash
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
INSERT INTO app.queries (
    query_id,
    query_ref,
    query_type_lua_28,
    query_dialect_lua_30,
    name,
    summary,
    query_code,
    query_status_lua_27,
    collection,
    valid_after,
    valid_until,
    created_id,
    created_at,
    updated_id,
    updated_at
)
VALUES (
    1,                                      -- query_id
    1000,                                   -- query_ref
    1000,                                   -- query_type_lua_28
    1,                                      -- query_dialect_lua_30
    'Create Tables Query',                  -- name, summary, query_code
'# Forward Migration 1000: Create Tables Query

This is the first migration that, technically, is run automatically
when connecting to an empty database and kicks off the migration,
so long as the database has been configured with AutoMigration: true
in its config (this is the default if not supplied).'
    ,
'CREATE TABLE app.queries (
    query_id SERIAL,
    query_ref INTEGER NOT NULL,
    query_type_lua_28 INTEGER NOT NULL,
    query_dialect_lua_30 INTEGER NOT NULL,
    name VARCHAR(100) NOT NULL,
    summary TEXT,
    query_code TEXT NOT NULL,
    query_status_lua_27 INTEGER NOT NULL,
    collection JSONB,
    valid_after TIMESTAMP WITH TIME ZONE,
    valid_until TIMESTAMP WITH TIME ZONE,
    created_id INTEGER NOT NULL,
    created_at TIMESTAMP WITH TIME ZONE NOT NULL,
    updated_id INTEGER NOT NULL,
    updated_at TIMESTAMP WITH TIME ZONE NOT NULL
);'
    ,
    1,                                      -- query_status_lua_27
    NULL,                                   -- collection
    NULL,                                   -- valid_after
    NULL,                                   -- valid_until
    0,                                      -- created_id
    CURRENT_TIMESTAMP,                      -- created_at
    0,                                      -- updated_id
    CURRENT_TIMESTAMP                       -- updated_at
);
INSERT INTO app.queries (
    query_id,
    query_ref,
    query_type_lua_28,
    query_dialect_lua_30,
    name,
    summary,
    query_code,
    query_status_lua_27,
    collection,
    valid_after,
    valid_until,
    created_id,
    created_at,
    updated_id,
    updated_at
)
VALUES (
    2,                                      -- query_id
    1000,                                   -- query_ref
    1001,                                   -- query_type_lua_28
    1,                                      -- query_dialect_lua_30
    'Delete Tables Query',                  -- name, summary, query_code
'# Reverse Migration 1000: Delete Tables Query

This is provided for completeness when testing the migration system
to ensure that forward and reverse migrations are complete.'
    ,
'DROP TABLE app.queries;'
    ,
    1,                                      -- query_status_lua_27
    NULL,                                   -- collection
    NULL,                                   -- valid_after
    NULL,                                   -- valid_until
    0,                                      -- created_id
    CURRENT_TIMESTAMP,                      -- created_at
    0,                                      -- updated_id
    CURRENT_TIMESTAMP                       -- updated_at
);
INSERT INTO app.queries (
    query_id,
    query_ref,
    query_type_lua_28,
    query_dialect_lua_30,
    name,
    summary,
    query_code,
    query_status_lua_27,
    collection,
    valid_after,
    valid_until,
    created_id,
    created_at,
    updated_id,
    updated_at
)
VALUES (
    3,                                      -- query_id
    1000,                                   -- query_ref
    1002,                                   -- query_type_lua_28
    1,                                      -- query_dialect_lua_30
    'Diagram Tables Query',                 -- name, summary
'# Diagram Migration 1000: Diagram Tables Query

This is the first PlantUML code for the query table.'
    ,
    NULL,                                   -- query_code, collection
    app.json_ingest(
'{
    "object_type": "table",
    "object_id": "table.queries",
    "plant_uml":
"@startuml
    entity queries {
        * query_id : SERIAL
        query_ref : INTEGER [NOT NULL]
        query_type_lua_28 : INTEGER [NOT NULL]
        query_dialect_lua_30 : INTEGER [NOT NULL]
        name : VARCHAR_100 [NOT NULL]
        summary : TEXT
        query_code : TEXT [NOT NULL]
        query_status_lua_27 : INTEGER [NOT NULL]
        collection : JSONB
        valid_after : TIMESTAMP_TZ
        valid_until : TIMESTAMP_TZ
        created_id : INTEGER [NOT NULL]
        created_at : TIMESTAMP_TZ [NOT NULL]
        updated_id : INTEGER [NOT NULL]
        updated_at : TIMESTAMP_TZ [NOT NULL]
    }
@enduml"
}'
    )
    ,
    1,                                      -- query_status_lua_27
    NULL,                                   -- collection
    NULL,                                   -- valid_after
    NULL,                                   -- valid_until
    0,                                      -- created_id
    CURRENT_TIMESTAMP,                      -- created_at
    0,                                      -- updated_id
    CURRENT_TIMESTAMP                       -- updated_at
);
