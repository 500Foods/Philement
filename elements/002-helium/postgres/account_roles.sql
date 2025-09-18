
CREATE TABLE IF NOT EXISTS app.account_roles
(
    system_id integer NOT NULL,
    account_id integer NOT NULL,
    role_id integer NOT NULL,
    name character varying(100) COLLATE pg_catalog."default",
    summary character varying(500) COLLATE pg_catalog."default",
    status_lua_37 integer NOT NULL,
    collection jsonb,
    valid_after timestamp with time zone,
    valid_until timestamp with time zone,
    created_id integer NOT NULL,
    created_at timestamp with time zone NOT NULL,
    updated_id integer NOT NULL,
    updated_at timestamp with time zone NOT NULL
)
