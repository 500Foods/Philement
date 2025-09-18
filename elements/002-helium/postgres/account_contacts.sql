
CREATE TABLE IF NOT EXISTS app.account_contacts
(
    account_id integer NOT NULL,
    contact_id integer NOT NULL DEFAULT nextval('app.account_contacts_contact_id_seq'::regclass),
    contact_type_lua_18 integer NOT NULL,
    contact_seq integer NOT NULL,
    contact character varying(100) COLLATE pg_catalog."default" NOT NULL,
    summary character varying(500) COLLATE pg_catalog."default",
    authenticate_lua_19 integer NOT NULL,
    status_lua_20 integer NOT NULL,
    collection jsonb,
    valid_after timestamp with time zone,
    valid_until timestamp with time zone,
    created_id integer NOT NULL,
    created_at timestamp with time zone NOT NULL,
    updated_id integer NOT NULL,
    updated_at timestamp with time zone NOT NULL,
    CONSTRAINT account_contacts_pkey PRIMARY KEY (contact_id)
)

