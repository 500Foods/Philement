
CREATE TABLE IF NOT EXISTS app.reports
(
    report_id integer NOT NULL,
    rev_id integer NOT NULL,
    name text COLLATE pg_catalog."default" NOT NULL,
    summary text COLLATE pg_catalog."default",
    collection jsonb,
    thumbnail text COLLATE pg_catalog."default",
    valid_after timestamp with time zone,
    valid_until timestamp with time zone,
    created_id integer NOT NULL,
    created_at timestamp with time zone NOT NULL,
    updated_id integer NOT NULL,
    updated_at timestamp with time zone NOT NULL,
    CONSTRAINT report_pkey PRIMARY KEY (report_id, rev_id)
)
