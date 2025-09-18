
CREATE TABLE IF NOT EXISTS app.queries
(
    query_id integer NOT NULL,
    query_ref integer NOT NULL,
    query_type_lua_28 integer NOT NULL,
    query_dialect_lua_30 integer NOT NULL,
    name character varying(100) COLLATE pg_catalog."default" NOT NULL,
    summary text COLLATE pg_catalog."default",
    query_code text COLLATE pg_catalog."default" NOT NULL,
    query_status_lua_27 integer NOT NULL,
    collection jsonb,
    valid_after timestamp with time zone,
    valid_until timestamp with time zone,
    created_id integer NOT NULL,
    created_at timestamp with time zone NOT NULL,
    updated_id integer NOT NULL,
    updated_at timestamp with time zone NOT NULL,
    CONSTRAINT queries_pkey PRIMARY KEY (query_id)
)

