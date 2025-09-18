
CREATE TABLE IF NOT EXISTS app.dictionaries
(
    dictionary_id integer NOT NULL,
    language_id integer NOT NULL,
    name character varying(100) COLLATE pg_catalog."default" NOT NULL,
    summary character varying(500) COLLATE pg_catalog."default",
    status_lua_3 integer NOT NULL,
    collection jsonb,
    valid_after timestamp with time zone,
    valid_until timestamp with time zone,
    created_id integer NOT NULL,
    created_at timestamp with time zone NOT NULL,
    updated_id integer NOT NULL,
    updated_at timestamp with time zone NOT NULL,
    CONSTRAINT dictionaries_pkey PRIMARY KEY (dictionary_id, language_id)
)

