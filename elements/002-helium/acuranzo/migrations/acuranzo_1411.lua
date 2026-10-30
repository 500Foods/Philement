-- Migration: acuranzo_1411.lua
-- Login History: soft-expire JWT (QR 019), logout audit (QR 161 type 5),
-- one Signed-in row per token in 159, failed = unpaired attempts, fix 160.
--
-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.0.1 - 2026-10-29 - DB2: QR 161 seed drops WHERE NOT EXISTS (SQL1585N)
-- 1.0.0 - 2026-10-28 - Soft revoke; logout audit; Login History merge/summary

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "queries"
cfg.MIGRATION = "1411"

-- ----------------------------------------------------------------------------
-- Forward
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
        ${QTC_SLOW}                                                         AS query_queue_a58,
        ${TIMEOUT}                                                          AS query_timeout,
        [=[
            UPDATE ${SCHEMA}${QUERIES}
               SET code = [==[
                    UPDATE ${SCHEMA}tokens
                       SET valid_until = ${NOW}
                     WHERE token_hash = :TOKENHASH
                       AND (valid_until IS NULL OR valid_until > ${NOW})
                ]==],
                   name = 'End JWT Session',
                   summary = [==[
                    # QueryRef #019 - End JWT Session

                    Soft-expires a JWT by setting valid_until to now so the
                    row remains for Login History. Validate JWT (018) already
                    requires valid_until >= now, so ended sessions are rejected.
                ]==]
             WHERE query_ref = 19;

            ${SUBQUERY_DELIMITER}

            -- No WHERE NOT EXISTS. That anti-join temps a queries row, and
            -- code/summary/collection are CLOB(100K), which is wider than a
            -- 32K page (SQL1585N). The id lookup below projects one integer.
            -- UNIQUE(query_ref, query_type_a28) rejects a second QR 161.
            INSERT INTO ${SCHEMA}${QUERIES} (
                ${QUERIES_INSERT}
            )
            WITH next_query_id AS (
                SELECT COALESCE(MAX(query_id), 0) + 1 AS new_query_id
                FROM ${SCHEMA}${QUERIES}
            )
            SELECT
                new_query_id                                                        AS query_id,
                161                                                                 AS query_ref,
                ${STATUS_ACTIVE}                                                    AS query_status_a27,
                ${TYPE_SQL}                                                         AS query_type_a28,
                ${DIALECT}                                                          AS query_dialect_a30,
                ${QTC_MEDIUM}                                                       AS query_queue_a58,
                ${TIMEOUT}                                                          AS query_timeout,
                [==[
                    INSERT INTO ${SCHEMA}actions (
                        action_id,
                        action_type_a24,
                        app_version,
                        feature_a21,
                        action,
                        action_msecs,
                        ip_address,
                        geo_city,
                        geo_region,
                        geo_country,
                        created_id,
                        created_at
                    )
                    WITH next_action_id AS (
                        SELECT COALESCE(MAX(action_id), 0) + 1 AS new_action_id
                        FROM ${SCHEMA}actions
                    )
                    SELECT
                        new_action_id,
                        5,
                        :APPVERSION,
                        100,
                        :LOGINID,
                        :LOGINTIMER,
                        :IPADDRESS,
                        NULLIF(:GEOCITY, ''),
                        NULLIF(:GEOREGION, ''),
                        NULLIF(:GEOCOUNTRY, ''),
                        :LOGINLOGID,
                        ${NOW}
                    FROM next_action_id
                ]==]                                                                AS code,
                'Log Logout'                                                        AS name,
                [==[
                    # QueryRef #161 - Log Logout

                    Inserts actions type 5 (logout / session end) for Login History.
                ]==]                                                                AS summary,
                '{}'                                                                AS collection,
                ${COMMON_INSERT}
            FROM next_query_id;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
               SET code = [==[
                    SELECT * FROM (
                        SELECT
                            NULL AS action_id,
                            0 AS action_type_a24,
                            'Signed in' AS action_type_name,
                            acc.name AS login_id,
                            TRIM(t.ip_address) AS ip_address,
                            t.geo_city,
                            t.geo_region,
                            t.geo_country,
                            CASE
                                WHEN t.valid_until IS NOT NULL
                                     AND t.valid_until <= CURRENT_TIMESTAMP
                                THEN (EXTRACT(EPOCH FROM (t.valid_until - t.valid_after)) * 1000)::bigint
                                ELSE NULL
                            END AS action_msecs,
                            TRIM(t.app_version) AS app_version,
                            t.valid_after AS created_at
                          FROM ${SCHEMA}tokens t
                          LEFT JOIN ${SCHEMA}accounts acc ON acc.account_id = t.account_id
                         WHERE t.account_id = :ACCOUNTID
                        UNION ALL
                        SELECT
                            a.action_id,
                            a.action_type_a24,
                            'Failed' AS action_type_name,
                            a.action AS login_id,
                            a.ip_address,
                            a.geo_city,
                            a.geo_region,
                            a.geo_country,
                            a.action_msecs,
                            a.app_version,
                            a.created_at
                          FROM ${SCHEMA}actions a
                         WHERE a.feature_a21 = 100
                           AND a.action_type_a24 = 2
                           AND LOWER(a.action) IN (
                                SELECT LOWER(ac.contact) FROM ${SCHEMA}account_contacts ac
                                 WHERE ac.account_id = :ACCOUNTID
                                UNION
                                SELECT LOWER(acc.name) FROM ${SCHEMA}accounts acc
                                 WHERE acc.account_id = :ACCOUNTID
                           )
                           AND NOT EXISTS (
                                SELECT 1 FROM ${SCHEMA}tokens t
                                 WHERE t.account_id = :ACCOUNTID
                                   AND t.valid_after >= a.created_at - INTERVAL '30 seconds'
                                   AND t.valid_after <= a.created_at + INTERVAL '120 seconds'
                           )
                        UNION ALL
                        SELECT
                            a.action_id,
                            a.action_type_a24,
                            'Signed out' AS action_type_name,
                            a.action AS login_id,
                            a.ip_address,
                            a.geo_city,
                            a.geo_region,
                            a.geo_country,
                            a.action_msecs,
                            a.app_version,
                            a.created_at
                          FROM ${SCHEMA}actions a
                         WHERE a.feature_a21 = 100
                           AND a.action_type_a24 = 5
                           AND LOWER(a.action) IN (
                                SELECT LOWER(ac.contact) FROM ${SCHEMA}account_contacts ac
                                 WHERE ac.account_id = :ACCOUNTID
                                UNION
                                SELECT LOWER(acc.name) FROM ${SCHEMA}accounts acc
                                 WHERE acc.account_id = :ACCOUNTID
                           )
                    ) h
                     ORDER BY h.created_at DESC
                     LIMIT 200
                ]==]
             WHERE query_ref = 159;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
               SET code = [==[
                    SELECT
                        (SELECT COUNT(*) FROM ${SCHEMA}tokens
                          WHERE account_id = :ACCOUNTID)                 AS total_logins,
                        (SELECT MAX(valid_after) FROM ${SCHEMA}tokens
                          WHERE account_id = :ACCOUNTID)                 AS last_login,
                        (SELECT COUNT(*) FROM ${SCHEMA}actions a
                          WHERE a.feature_a21 = 100 AND a.action_type_a24 = 2
                            AND LOWER(a.action) IN (
                                SELECT LOWER(ac.contact) FROM ${SCHEMA}account_contacts ac
                                 WHERE ac.account_id = :ACCOUNTID
                                UNION
                                SELECT LOWER(acc.name) FROM ${SCHEMA}accounts acc
                                 WHERE acc.account_id = :ACCOUNTID
                            )
                            AND NOT EXISTS (
                                SELECT 1 FROM ${SCHEMA}tokens t
                                 WHERE t.account_id = :ACCOUNTID
                                   AND t.valid_after >= a.created_at - INTERVAL '30 seconds'
                                   AND t.valid_after <= a.created_at + INTERVAL '120 seconds'
                            ))                                           AS failed_attempts,
                        (SELECT COUNT(*) FROM ${SCHEMA}tokens
                          WHERE account_id = :ACCOUNTID
                            AND valid_until > CURRENT_TIMESTAMP)         AS active_sessions
                      FROM ${SCHEMA}queries
                     WHERE query_ref = 160
                ]==],
                   summary = [==[
                    # QueryRef #160 - Get Login History Summary

                    total_logins = token rows (one per sign-in).
                    failed_attempts = type-2 actions with no nearby token.
                    active_sessions = tokens with valid_until > now.
                ]==]
             WHERE query_ref = 160;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Login History soft-expire + logout audit + 159/160'                AS name,
        [=[
            # Forward Migration 1411
            Soft-expire QR 019; add QR 161 logout; update 159/160.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})

-- ----------------------------------------------------------------------------
-- Reverse
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
            UPDATE ${SCHEMA}${QUERIES}
               SET code = [==[
                    DELETE FROM
                        ${SCHEMA}tokens
                    WHERE
                        (token_hash = :TOKENHASH)
                ]==],
                   name = 'Delete JWT'
             WHERE query_ref = 19;

            ${SUBQUERY_DELIMITER}

            DELETE FROM ${SCHEMA}${QUERIES} WHERE query_ref = 161;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Reverse 1411'                                                      AS name,
        'Restores QR 019 DELETE; drops QR 161.'                             AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})

return queries
end
