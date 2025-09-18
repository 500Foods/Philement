
CREATE TABLE IF NOT EXISTS app.documents
(
    doc_id integer NOT NULL,
    rev_id integer NOT NULL,
    doc_library_lua_49 integer NOT NULL,
    doc_status_lua_50 integer NOT NULL,
    name character varying(100) COLLATE pg_catalog."default" NOT NULL,
    summary text COLLATE pg_catalog."default",
    collection jsonb,
    valid_after timestamp with time zone,
    valid_until timestamp with time zone,
    created_id integer NOT NULL,
    created_at timestamp with time zone NOT NULL,
    updated_id integer NOT NULL,
    updated_at timestamp with time zone NOT NULL,
    doc_type_lua_51 integer,
    file_preview text COLLATE pg_catalog."default",
    file_name text COLLATE pg_catalog."default",
    file_data text COLLATE pg_catalog."default",
    file_text text COLLATE pg_catalog."default",
    CONSTRAINT documents_pkey PRIMARY KEY (doc_id, rev_id),
    CONSTRAINT documents_doc_id_rev_id_name_key UNIQUE (doc_id, rev_id, name)
)
