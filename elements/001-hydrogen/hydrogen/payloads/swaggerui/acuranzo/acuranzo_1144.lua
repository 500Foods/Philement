-- Migration: acuranzo_1144.lua
-- Create Auth Test Accounts

-- NOTE: Also see acuranzo_10244.lua

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.1.0 - 2026-01-10 - Use fixed account IDs (0-3) and SHA256_HASH_* macros for password hashing
-- 1.0.0 - 2026-01-08 - Initial creation

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "queries"
cfg.MIGRATION = "1144"
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
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
            -- Insert demo accounts with fixed account IDs (0-3) for predictable password hashing
            -- Password hash = BASE64(SHA256(account_id || password)) computed in migration
            INSERT INTO ${SCHEMA}accounts (
                account_id,
                name,
                first_name,
                last_name,
                password_hash,
                status_a16,
                iana_timezone_a17,
                summary,
                collection,
                ${COMMON_FIELDS}
            )
            VALUES
            (
                1,
                '${HYDROGEN_DEMO_ADMIN_NAME}',
                'Demo',
                'Admin',
                ${SHA256_HASH_START}'1'${SHA256_HASH_MID}'${HYDROGEN_DEMO_ADMIN_PASS}'${SHA256_HASH_END},
                1,
                1,
                'Demo Admin Account',
                '{}',
                ${COMMON_VALUES}
            ),
            (
                2,
                '${HYDROGEN_DEMO_USER_NAME}',
                'Demo',
                'User',
                ${SHA256_HASH_START}'2'${SHA256_HASH_MID}'${HYDROGEN_DEMO_USER_PASS}'${SHA256_HASH_END},
                1,
                1,
                'Demo User Account',
                '{}',
                ${COMMON_VALUES}
            ),
            (
                3,
                'disableduser',
                'Disabled',
                'User',
                ${SHA256_HASH_START}'2'${SHA256_HASH_MID}'disabled_pass'${SHA256_HASH_END},
                0,
                1,
                'Demo Disabled Account',
                '{}',
                ${COMMON_VALUES}
            ),
            (
                4,
                'unauthorizeduser',
                'Unauthorized',
                'User',
                ${SHA256_HASH_START}'3'${SHA256_HASH_MID}'unauth_pass'${SHA256_HASH_END},
                2,
                1,
                'Demo unauthorized account',
                '{}',
                ${COMMON_VALUES}
            );

            ${SUBQUERY_DELIMITER}

            -- Insert email contacts for each test account, as well as username contacts
            -- Uses fixed account_ids (0=admin, 1=user, 2=disabled, 3=unauthorized)
            INSERT INTO ${SCHEMA}account_contacts (
                contact_id,
                account_id,
                contact_seq,
                contact_type_a18,
                authenticate_a19,
                status_a20,
                contact,
                summary,
                collection,
                ${COMMON_FIELDS}
            )
            VALUES (
                1,
                1,
                0,
                0,
                1,
                1,
                '${HYDROGEN_DEMO_ADMIN_NAME}',
                'Admin primary username',
                '{}',
                ${COMMON_VALUES}
            ),
            (
                2,
                1,
                1,
                1,
                1,
                1,
                '${HYDROGEN_DEMO_EMAIL}',
                'Admin primary email',
                '{}',
                ${COMMON_VALUES}
            ),
            (
                3,
                2,
                0,
                0,
                1,
                1,
                '${HYDROGEN_DEMO_USER_NAME}',
                'User primary username',
                '{}',
                ${COMMON_VALUES}
            ),
            (
                4,
                2,
                1,
                1,
                1,
                1,
                '${HYDROGEN_DEMO_EMAIL}',
                'User primary email',
                '{}',
                ${COMMON_VALUES}
            ),
            (
                5,
                3,
                1,
                1,
                1,
                1,
                '${HYDROGEN_DEMO_EMAIL}',
                'Disabled account email',
                '{}',
                ${COMMON_VALUES}
            ),
            (
                6,
                4,
                1,
                1,
                1,
                1,
                '${HYDROGEN_DEMO_EMAIL}',
                'Unauthorized account email',
                '{}',
                ${COMMON_VALUES}
            );

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Populate Test Data - Auth Test Accounts'                          AS name,
        [=[
            # Forward Migration ${MIGRATION}: Populate Test Data - Auth Test Accounts

            This migration creates test user accounts for authentication endpoint testing.
            Includes various account states: enabled/disabled, authorized/unauthorized.
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
        ${QTC_SLOW}                                                         AS query_queue_a58,
        ${TIMEOUT}                                                          AS query_timeout,
        [=[
            -- Remove contacts first
            -- Uses fixed account_ids (1=admin, 2=user, 3=disabled, 4=unauthorized)
            DELETE FROM ${SCHEMA}account_contacts
            WHERE account_id IN (1, 2, 3, 4);

            ${SUBQUERY_DELIMITER}

            -- Then remove accounts
            DELETE FROM ${SCHEMA}accounts
            WHERE account_id IN (1, 2, 3, 4);

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Remove Test Data - Auth Test Accounts'                             AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Remove Test Data - Auth Test Accounts

            This migration removes the test user accounts created for authentication testing.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
return queries end