-- Migration: courses_4132.lua
-- Seed: grant admin role (role_id 3) to adminuser (account_id 1)
--
-- Course Builder / Course Manager are gated on JWT role names staff/admin
-- (roles seeded in acuranzo_1380). Nothing assigned them to any account, so
-- a fresh database had no user able to see those modules. Idempotent:
-- skipped when account 1 already holds role 3.
--
-- luacheck: no max line length
-- luacheck: no unused args
--
-- CHANGELOG
-- 1.0.0 - 2026-10-31 - Grant admin role to adminuser

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "account_roles"
cfg.MIGRATION = "4132"
if engine == "db2" then
    cfg.FROM_ONE = "FROM SYSIBM.SYSDUMMY1"
elseif engine == "firebird" then
    cfg.FROM_ONE = "FROM RDB$DATABASE"
elseif engine == "mysql" or engine == "mariadb" then
    cfg.FROM_ONE = "FROM DUAL"
else
    cfg.FROM_ONE = ""
end
-- ----------------------------------------------------------------------------
-- Forward: grant admin to adminuser
-- ----------------------------------------------------------------------------
table.insert(queries,{sql=[====[

    INSERT INTO ${SCHEMA}${QUERIES} (
        ${QUERIES_INSERT}
    )
    WITH next_query_id AS (
        SELECT COALESCE(MAX(query_id), 0) + 1 AS new_query_id
        FROM ${SCHEMA}${QUERIES}
    )
    SELECT
        new_query_id                                                        AS query_id,
        ${MIGRATION}                                                        AS query_ref,
        ${STATUS_ACTIVE}                                                    AS query_status_a27,
        ${TYPE_FORWARD_MIGRATION}                                           AS query_type_a28,
        ${DIALECT}                                                          AS query_dialect_a30,
        ${QTC_SLOW}                                                         AS query_queue_a58,
        ${TIMEOUT}                                                          AS query_timeout,
        [=[
            INSERT INTO ${SCHEMA}account_roles (
                account_id,
                role_id,
                system_id,
                status_a37,
                name,
                summary,
                collection,
                ${COMMON_FIELDS}
            )
            SELECT
                1,
                3,
                0,
                1,
                'admin',
                'adminuser admin',
                '{}',
                ${COMMON_VALUES}
            ${FROM_ONE}
            WHERE NOT EXISTS (
                SELECT 1 FROM ${SCHEMA}account_roles
                WHERE account_id = 1
                  AND role_id = 3
            );

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Grant admin role to adminuser'                                     AS name,
        [=[
            # Forward Migration ${MIGRATION}: Grant admin role to adminuser

            Inserts account_roles (account_id 1, role_id 3 'admin') so adminuser can
            reach Course Builder and Course Manager. Skipped if the row already exists.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]====]})
-- ----------------------------------------------------------------------------
-- Reverse
-- ----------------------------------------------------------------------------
table.insert(queries,{sql=[====[

    INSERT INTO ${SCHEMA}${QUERIES} (
        ${QUERIES_INSERT}
    )
    WITH next_query_id AS (
        SELECT COALESCE(MAX(query_id), 0) + 1 AS new_query_id
        FROM ${SCHEMA}${QUERIES}
    )
    SELECT
        new_query_id                                                        AS query_id,
        ${MIGRATION}                                                        AS query_ref,
        ${STATUS_ACTIVE}                                                    AS query_status_a27,
        ${TYPE_REVERSE_MIGRATION}                                           AS query_type_a28,
        ${DIALECT}                                                          AS query_dialect_a30,
        ${QTC_SLOW}                                                         AS query_queue_a58,
        ${TIMEOUT}                                                          AS query_timeout,
        [=[
            DELETE FROM ${SCHEMA}account_roles
            WHERE account_id = 1
              AND role_id = 3;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Revoke admin role from adminuser'                                  AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Revoke admin role from adminuser
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]====]})

return queries end
