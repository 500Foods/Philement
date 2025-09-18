
CREATE TABLE IF NOT EXISTS app.account_access
(
    account_id integer NOT NULL,
    access_id integer NOT NULL,
    feature_lua_21 integer NOT NULL,
    access_type_lua_22 integer NOT NULL,
    status_lua_23 integer NOT NULL,
    collection jsonb,
    valid_after timestamp with time zone,
    valid_until timestamp with time zone,
    created_id integer NOT NULL,
    created_at timestamp with time zone NOT NULL,
    updated_id integer NOT NULL,
    updated_at timestamp with time zone NOT NULL,
    CONSTRAINT account_access_pkey PRIMARY KEY (access_id)
)
