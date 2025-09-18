
CREATE TABLE IF NOT EXISTS app.lists
(
    list_type_lua_31 integer NOT NULL,
    list_key integer,
    list_value character varying(100) COLLATE pg_catalog."default",
    list_note character varying(100) COLLATE pg_catalog."default",
    status_lua_32 integer NOT NULL,
    collection jsonb,
    valid_after timestamp with time zone,
    valid_until timestamp with time zone,
    created_id integer NOT NULL,
    created_at timestamp with time zone NOT NULL,
    updated_id integer NOT NULL,
    updated_at timestamp with time zone NOT NULL
)
