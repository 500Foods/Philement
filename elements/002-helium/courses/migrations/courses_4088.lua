-- Migration: courses_4088.lua
-- QueryRef #4508 - Staff: Search Accounts
--
-- 4508 is not a file number. TYPE_SQL. Exact match only.
-- A positive ACCOUNTID wins. Otherwise a non-blank SUBJECT
-- matches account_oidc_identities.subject. Otherwise a
-- non-blank EMAIL matches that table's email, case-insensitive.
-- All three binds are required. An empty search returns no rows.
-- 4001 stays the Canvas link read Lua calls.
--
-- luacheck: no max line length
-- luacheck: no unused args
--
-- CHANGELOG
-- 1.0.0 - 2026-10-23 - Staff account search by id, subject, or email

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "queries"
cfg.MIGRATION = "4088"
cfg.QUERY_REF = "4508"
cfg.QUERY_NAME = "Staff: Search Accounts"
if engine == "mysql" then
    cfg.CAST_INTEGER = "signed"
else
    cfg.CAST_INTEGER = cfg.INTEGER
end

local sql_new = [[
                    SELECT a.account_id,
                           a.name,
                           oi.email,
                           oi.subject
                      FROM ${SCHEMA}account_oidc_identities oi
                      JOIN ${SCHEMA}accounts a
                        ON a.account_id = oi.account_id
                     WHERE (
                            CAST(:ACCOUNTID AS ${CAST_INTEGER}) > 0
                            AND a.account_id = CAST(:ACCOUNTID AS ${CAST_INTEGER})
                           )
                        OR (
                            CAST(:ACCOUNTID AS ${CAST_INTEGER}) = 0
                            AND NULLIF(:SUBJECT, '') IS NOT NULL
                            AND oi.subject = :SUBJECT
                           )
                        OR (
                            CAST(:ACCOUNTID AS ${CAST_INTEGER}) = 0
                            AND NULLIF(:SUBJECT, '') IS NULL
                            AND NULLIF(:EMAIL, '') IS NOT NULL
                            AND LOWER(oi.email) = LOWER(:EMAIL)
                           )
                     ORDER BY a.account_id, oi.identity_id]]

-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
table.insert(queries,{sql=[[

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
        ${QTC_FAST}                                                         AS query_queue_a58,
        ${TIMEOUT}                                                          AS query_timeout,
        [=[
            INSERT INTO ${SCHEMA}${QUERIES} (
                ${QUERIES_INSERT}
            )
            WITH next_query_id AS (
                SELECT COALESCE(MAX(query_id), 0) + 1 AS new_query_id
                FROM ${SCHEMA}${QUERIES}
            )
            SELECT
                new_query_id                                                        AS query_id,
                ${QUERY_REF}                                                        AS query_ref,
                ${STATUS_ACTIVE}                                                    AS query_status_a27,
                ${TYPE_SQL}                                                         AS query_type_a28,
                ${DIALECT}                                                          AS query_dialect_a30,
                ${QTC_FAST}                                                         AS query_queue_a58,
                ${TIMEOUT}                                                          AS query_timeout,
                [==[
]] .. sql_new .. [[
                ]==]                                                                AS code,
                '${QUERY_NAME}'                                                     AS name,
                [==[
                    # QueryRef #${QUERY_REF} - ${QUERY_NAME}

                    Not public (TYPE_SQL). INTEGER ACCOUNTID, STRING SUBJECT,
and STRING EMAIL are all required. ACCOUNTID 0 with blank
SUBJECT and EMAIL returns nothing. MySQL casts the id as signed.
password_hash is not selected.
                ]==]
                                                                                     AS summary,
                '{}'                                                                AS collection,
                ${COMMON_INSERT}
            FROM next_query_id;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Populate QueryRef #${QUERY_REF} - ${QUERY_NAME}'                   AS name,
        [=[
            # Forward Migration ${MIGRATION}: Populate QueryRef #${QUERY_REF}

            Inserts the staff account search. Account rows stay.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
table.insert(queries,{sql=[[

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
        ${QTC_FAST}                                                         AS query_queue_a58,
        ${TIMEOUT}                                                          AS query_timeout,
        [=[
            DELETE FROM ${SCHEMA}${TABLE}
            WHERE query_ref = ${QUERY_REF}
              AND query_type_a28 = ${TYPE_SQL};

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Remove QueryRef #${QUERY_REF}'                                     AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Remove QueryRef #${QUERY_REF}

            Deletes the TYPE_SQL row only. Account rows stay.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
return queries end
