
CREATE TABLE IF NOT EXISTS app.tokens
(
    token_hash character(128) COLLATE pg_catalog."default" NOT NULL,
    account_id integer NOT NULL,
    system_id integer NOT NULL,
    application_id integer NOT NULL,
    application_version character varying(20) COLLATE pg_catalog."default" NOT NULL,
    ip_address character varying(50) COLLATE pg_catalog."default" NOT NULL,
    valid_after timestamp with time zone NOT NULL,
    valid_until timestamp with time zone NOT NULL,
    CONSTRAINT tokens_pkey PRIMARY KEY (token_hash)
)

