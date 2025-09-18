
CREATE TABLE IF NOT EXISTS app.sessions
(
    session_id text COLLATE pg_catalog."default" NOT NULL,
    account_id integer NOT NULL,
    session text COLLATE pg_catalog."default" NOT NULL,
    session_length integer NOT NULL,
    session_issues integer NOT NULL,
    session_secs integer NOT NULL,
    status_lua_25 integer NOT NULL,
    flag_lua_26 integer NOT NULL,
    created_at timestamp with time zone NOT NULL,
    updated_at timestamp with time zone NOT NULL,
    session_changes integer
)
