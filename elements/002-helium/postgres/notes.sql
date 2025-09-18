
CREATE TABLE IF NOT EXISTS app.notes
(
    note_id integer NOT NULL,
    name character varying(100) COLLATE pg_catalog."default" NOT NULL,
    summary character varying(500) COLLATE pg_catalog."default",
    entity_id integer NOT NULL,
    entity_type_lua_9 integer NOT NULL,
    status_lua_8 integer NOT NULL,
    collection jsonb,
    valid_after timestamp with time zone,
    valid_until timestamp with time zone,
    created_id integer NOT NULL,
    created_at timestamp with time zone NOT NULL,
    updated_id integer NOT NULL,
    updated_at timestamp with time zone NOT NULL,
    CONSTRAINT notes_pkey PRIMARY KEY (note_id)
)

