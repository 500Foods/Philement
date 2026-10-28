-- Migration: acuranzo_1407.lua
-- QueryRef #160 - Get Login History Summary
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
cfg.MIGRATION = "1407"
cfg.QUERY_REF = "160"
cfg.QUERY_NAME = "Get Login History Summary"

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
                    SELECT
                        (SELECT COUNT(*) FROM ${SCHEMA}actions
                          WHERE feature_a21 = 100 AND action_type_a24 IN (3, 4)
                            AND account_id = :ACCOUNTID)
                      + (SELECT COUNT(*) FROM ${SCHEMA}tokens
                          WHERE account_id = :ACCOUNTID)                 AS total_logins,
                        (SELECT MAX(x.ts) FROM (
                            SELECT MAX(created_at) AS ts FROM ${SCHEMA}actions
                             WHERE feature_a21 = 100 AND action_type_a24 IN (3, 4)
                               AND account_id = :ACCOUNTID
                            UNION ALL
                            SELECT MAX(valid_after) AS ts FROM ${SCHEMA}tokens
                             WHERE account_id = :ACCOUNTID
                        ) x)                                             AS last_login,
                        (SELECT COUNT(*) FROM ${SCHEMA}actions
                          WHERE feature_a21 = 100 AND action_type_a24 = 2
                            AND LOWER(action) IN (
                                SELECT LOWER(ac.contact) FROM ${SCHEMA}account_contacts ac
                                 WHERE ac.account_id = :ACCOUNTID
                                UNION
                                SELECT LOWER(acc.name) FROM ${SCHEMA}accounts acc
                                 WHERE acc.account_id = :ACCOUNTID
                            ))                                           AS failed_attempts,
                        (SELECT COUNT(*) FROM ${SCHEMA}tokens
                          WHERE account_id = :ACCOUNTID
                            AND valid_until > CURRENT_TIMESTAMP)         AS active_sessions
                      FROM ${SCHEMA}queries
                     WHERE query_ref = 160
                ]==]                                                                AS code,
                '${QUERY_NAME}'                                                     AS name,
                [==[
                    # QueryRef #${QUERY_REF} - ${QUERY_NAME}

                    total_logins/last_login include tokens (JWT issuances);
                    active_sessions = tokens with valid_until > now (sessions
                    table has no writers).

                    One row of login counters for the current user:
                    total_logins, last_login, failed_attempts (type 2 rows
                    for LOGINID; includes attempts that later succeeded),
                    active_sessions (sessions with status_a25 = 0).

                    ## Parameters

                    - ACCOUNTID (INTEGER) - JWT user_id
                    - LOGINID (STRING) - unused (kept for compatibility); type-2 rows match any account_contacts.contact or accounts.name for ACCOUNTID, case-insensitive, since Hydrogen logs the login_id exactly as typed (QR 004)

                    ## Tables

                    - `${SCHEMA}actions`
                    - `${SCHEMA}sessions`
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
