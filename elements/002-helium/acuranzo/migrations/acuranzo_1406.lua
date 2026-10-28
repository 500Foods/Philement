-- Migration: acuranzo_1406.lua
-- QueryRef #159 - Get Login History
--
-- Read-only queries for the Lithium Profile Manager Login History page
-- (index -17). Login rows come from actions (QueryRef 004 / 014).

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.0.0 - 2026-10-27 - Initial creation

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "queries"
cfg.MIGRATION = "1406"
cfg.QUERY_REF = "159"
cfg.QUERY_NAME = "Get Login History"

-- ----------------------------------------------------------------------------
-- Forward: Populate QueryRef #${QUERY_REF} - ${QUERY_NAME}
-- ----------------------------------------------------------------------------
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
                ${QTC_SLOW}                                                         AS query_queue_a58,
                ${TIMEOUT}                                                          AS query_timeout,
                [==[
                    SELECT * FROM (
                        SELECT
                            a.action_id,
                            a.action_type_a24,
                            CASE a.action_type_a24
                                WHEN 2 THEN 'Login Attempt'
                                WHEN 3 THEN 'Login Successful'
                                WHEN 4 THEN 'Login Passed'
                                ELSE 'Other'
                            END AS action_type_name,
                            a.action AS login_id,
                            a.ip_address,
                            a.action_msecs,
                            a.app_version,
                            a.created_at
                          FROM ${SCHEMA}actions a
                         WHERE a.feature_a21 = 100
                           AND (
                                (a.action_type_a24 IN (3, 4) AND a.account_id = :ACCOUNTID)
                             OR (a.action_type_a24 = 2 AND LOWER(a.action) IN (
                                    SELECT LOWER(ac.contact) FROM ${SCHEMA}account_contacts ac
                                     WHERE ac.account_id = :ACCOUNTID
                                    UNION
                                    SELECT LOWER(acc.name) FROM ${SCHEMA}accounts acc
                                     WHERE acc.account_id = :ACCOUNTID
                                ))
                           )
                        UNION ALL
                        SELECT
                            NULL AS action_id,
                            0 AS action_type_a24,
                            'Token Issued' AS action_type_name,
                            acc.name AS login_id,
                            TRIM(t.ip_address) AS ip_address,
                            NULL AS action_msecs,
                            TRIM(t.app_version) AS app_version,
                            t.valid_after AS created_at
                          FROM ${SCHEMA}tokens t
                          LEFT JOIN ${SCHEMA}accounts acc ON acc.account_id = t.account_id
                         WHERE t.account_id = :ACCOUNTID
                    ) h
                     ORDER BY h.created_at DESC
                     LIMIT 200
                ]==]                                                                AS code,
                '${QUERY_NAME}'                                                     AS name,
                [==[
                    # QueryRef #${QUERY_REF} - ${QUERY_NAME}

                    Also includes JWT issuances from tokens (type 0, 'Token Issued'),
                    so OIDC/Keycloak sign-ins appear even without actions rows.

                    Recent login activity (max 200 rows, newest first) for
                    the current user. Successful logins (types 3, 4) match
                    by account_id; attempts (type 2) carry no account_id and
                    match by login id.

                    ## Parameters

                    - ACCOUNTID (INTEGER) - JWT user_id
                    - LOGINID (STRING) - unused (kept for compatibility); type-2 rows match any account_contacts.contact or accounts.name for ACCOUNTID, case-insensitive, since Hydrogen logs the login_id exactly as typed (QR 004)

                    ## Tables

                    - `${SCHEMA}actions`
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
            # Forward Migration ${MIGRATION}: Populate QueryRef #${QUERY_REF} - ${QUERY_NAME}

            Inserts QueryRef #${QUERY_REF} for the Lithium Profile Manager
            Login History page.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})

-- ----------------------------------------------------------------------------
-- Reverse: Remove QueryRef #${QUERY_REF} - ${QUERY_NAME}
-- ----------------------------------------------------------------------------
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
        ${QTC_SLOW}                                                         AS query_queue_a58,
        ${TIMEOUT}                                                          AS query_timeout,
        [=[
            DELETE FROM ${SCHEMA}${TABLE}
            WHERE query_ref = ${QUERY_REF};

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Remove QueryRef #${QUERY_REF} - ${QUERY_NAME}'                     AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Remove QueryRef #${QUERY_REF} - ${QUERY_NAME}

            Deletes the runtime QueryRef.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})


return queries
end
