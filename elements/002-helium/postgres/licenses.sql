
CREATE TABLE IF NOT EXISTS app.licenses
(
    license_id integer NOT NULL,
    name character varying(100) COLLATE pg_catalog."default" NOT NULL,
    summary character varying(500) COLLATE pg_catalog."default",
    application_key character varying(100) COLLATE pg_catalog."default",
    system_id integer NOT NULL,
    status_lua_13 integer NOT NULL,
    collection jsonb,
    valid_after timestamp with time zone,
    valid_until timestamp with time zone,
    created_id integer NOT NULL,
    created_at timestamp with time zone NOT NULL,
    updated_id integer NOT NULL,
    updated_at timestamp with time zone NOT NULL,
    CONSTRAINT licenses_pkey PRIMARY KEY (license_id)
)

