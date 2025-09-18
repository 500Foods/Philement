
CREATE TABLE IF NOT EXISTS app.workflow_steps
(
    workflow_id integer NOT NULL,
    step_id integer NOT NULL,
    name character varying(100) COLLATE pg_catalog."default" NOT NULL,
    summary character varying(500) COLLATE pg_catalog."default",
    sort_seq integer NOT NULL,
    status_lua_11 integer NOT NULL,
    collection jsonb,
    valid_after timestamp with time zone,
    valid_until timestamp with time zone,
    created_id integer NOT NULL,
    created_at timestamp with time zone NOT NULL,
    updated_id integer NOT NULL,
    updated_at timestamp with time zone NOT NULL,
    CONSTRAINT workflow_steps_pkey PRIMARY KEY (step_id)
)

