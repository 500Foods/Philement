
CREATE TABLE IF NOT EXISTS app.convos
(
    convos_id integer NOT NULL,
    convos_ref character(50) COLLATE pg_catalog."default" NOT NULL,
    convos_keywords text COLLATE pg_catalog."default",
    convos_icon text COLLATE pg_catalog."default",
    prompt text COLLATE pg_catalog."default",
    response text COLLATE pg_catalog."default",
    context text COLLATE pg_catalog."default",
    history text COLLATE pg_catalog."default",
    collection jsonb,
    valid_after timestamp with time zone,
    valid_until timestamp with time zone,
    created_id integer NOT NULL,
    created_at timestamp with time zone NOT NULL,
    updated_id integer NOT NULL,
    updated_at timestamp with time zone NOT NULL,
    CONSTRAINT convos_pkey PRIMARY KEY (convos_id)
)

