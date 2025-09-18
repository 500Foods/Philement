
CREATE TABLE IF NOT EXISTS app.languages
(
    language_id integer NOT NULL,
    name character varying(100) COLLATE pg_catalog."default" NOT NULL,
    summary character varying(500) COLLATE pg_catalog."default",
    iso639 character(2) COLLATE pg_catalog."default" NOT NULL,
    status_lua_2 integer NOT NULL,
    collection jsonb,
    valid_after timestamp with time zone,
    valid_until timestamp with time zone,
    created_id integer NOT NULL,
    created_at timestamp with time zone NOT NULL,
    updated_id integer NOT NULL,
    updated_at timestamp with time zone NOT NULL,
    CONSTRAINT languages_pkey PRIMARY KEY (language_id)
)

