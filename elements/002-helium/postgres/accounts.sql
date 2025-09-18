
CREATE TABLE IF NOT EXISTS app.accounts
(
    account_id integer NOT NULL,
    name character varying(100) COLLATE pg_catalog."default" NOT NULL,
    summary character varying(500) COLLATE pg_catalog."default",
    first_name character varying(50) COLLATE pg_catalog."default" NOT NULL,
    middle_name character varying(50) COLLATE pg_catalog."default" NOT NULL,
    last_name character varying(50) COLLATE pg_catalog."default" NOT NULL,
    password_hash character varying(128) COLLATE pg_catalog."default" NOT NULL,
    iana_timezone_lua_17 integer NOT NULL,
    status_lua_16 integer NOT NULL,
    collection jsonb,
    valid_after timestamp with time zone,
    valid_until timestamp with time zone,
    created_id integer NOT NULL,
    created_at timestamp with time zone NOT NULL,
    updated_id integer NOT NULL,
    updated_at timestamp with time zone NOT NULL,
    CONSTRAINT accounts_pkey PRIMARY KEY (account_id)
)
