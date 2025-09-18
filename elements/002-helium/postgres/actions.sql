
CREATE TABLE IF NOT EXISTS app.actions
(
    action_id integer NOT NULL DEFAULT nextval('app.actions_action_id_seq'::regclass),
    action_type_lua_24 integer NOT NULL,
    system_id integer,
    application_id integer,
    application_version character varying(20) COLLATE pg_catalog."default",
    account_id integer,
    feature_lua_21 integer NOT NULL,
    action character varying(500) COLLATE pg_catalog."default",
    action_msecs integer NOT NULL,
    ip_address character varying(50) COLLATE pg_catalog."default",
    created_id integer NOT NULL,
    created_at timestamp with time zone NOT NULL
)
