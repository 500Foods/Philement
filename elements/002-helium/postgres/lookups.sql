
CREATE TABLE IF NOT EXISTS app.lookups
(
    lookup_id integer NOT NULL,
    key_idx integer NOT NULL,
    value_txt character varying(100) COLLATE pg_catalog."default",
    value_int integer,
    sort_seq integer NOT NULL,
    status_lua_1 integer NOT NULL,
    collection jsonb,
    valid_after timestamp with time zone,
    valid_until timestamp with time zone,
    created_id integer NOT NULL,
    created_at timestamp with time zone NOT NULL,
    updated_id integer NOT NULL,
    updated_at timestamp with time zone NOT NULL,
    summary text COLLATE pg_catalog."default",
    code text COLLATE pg_catalog."default",
    CONSTRAINT lookups_pkey PRIMARY KEY (lookup_id, key_idx)
)

