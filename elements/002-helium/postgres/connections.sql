
CREATE TABLE IF NOT EXISTS app.connections
(
    connection_id integer NOT NULL,
    connection_type_lua_4 integer NOT NULL,
    name character varying(100) COLLATE pg_catalog."default" NOT NULL,
    summary character varying(500) COLLATE pg_catalog."default",
    connected_lua_5 integer NOT NULL,
    status_lua_6 integer NOT NULL,
    collection jsonb,
    valid_after timestamp with time zone,
    valid_until timestamp with time zone,
    created_id integer NOT NULL,
    created_at timestamp with time zone NOT NULL,
    updated_id integer NOT NULL,
    updated_at timestamp with time zone NOT NULL,
    CONSTRAINT connections_pkey PRIMARY KEY (connection_id)
)

