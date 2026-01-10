-- Migration: acuranzo_1144.lua
-- Test Data - Auth Test Accounts

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.0.0 - 2026-01-08 - Initial creation

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "queries"
cfg.MIGRATION = "1144"
cfg.QUERY_REF = "1144"
cfg.QUERY_NAME = "Test Data - Auth Test Accounts"
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
            -- Insert test accounts for authentication testing
            -- Uses CTE to compute base ID (MySQL doesn't allow subselects from same table in INSERT)
            INSERT INTO ${SCHEMA}accounts (account_id, name, first_name, last_name, password_hash, status_a16, iana_timezone_a17, summary, collection, ${COMMON_FIELDS})
            WITH next_account_id AS (
                SELECT COALESCE(MAX(account_id), 0) AS base_id
                FROM ${SCHEMA}accounts
            )
            SELECT
                base_id + 1,
                'testuser',
                'Test',
                'User',
                'hashed_test_password',
                1,
                1,
                'Test account for auth testing',
                '{}',
                ${COMMON_VALUES}
            FROM next_account_id
            UNION ALL
            SELECT
                base_id + 2,
                'adminuser',
                'Admin',
                'User',
                'hashed_admin_password',
                1,
                1,
                'Admin test account',
                '{}',
                ${COMMON_VALUES}
            FROM next_account_id
            UNION ALL
            SELECT
                base_id + 3,
                'disableduser',
                'Disabled',
                'User',
                'hashed_disabled_password',
                0,
                1,
                'Disabled test account',
                '{}',
                ${COMMON_VALUES}
            FROM next_account_id
            UNION ALL
            SELECT
                base_id + 4,
                'unauthorizeduser',
                'Unauth',
                'User',
                'hashed_unauth_password',
                2,
                1,
                'Unauthorized test account',
                '{}',
                ${COMMON_VALUES}
            FROM next_account_id;

            ${SUBQUERY_DELIMITER}

            -- Insert email contacts for each test account
            -- Uses CTE to compute base contact_id (MySQL doesn't allow subselects from same table in INSERT)
            INSERT INTO ${SCHEMA}account_contacts (contact_id, account_id, contact_seq, contact_type_a18, authenticate_a19, status_a20, contact, summary, collection, ${COMMON_FIELDS})
            WITH next_contact_id AS (
                SELECT COALESCE(MAX(contact_id), 0) AS base_id
                FROM ${SCHEMA}account_contacts
            )
            SELECT
                base_id + 1,
                (SELECT account_id FROM ${SCHEMA}accounts WHERE name = 'testuser'),
                1,
                1,
                1,
                1,
                'test@example.com',
                'Primary email',
                '{}',
                ${COMMON_VALUES}
            FROM next_contact_id
            UNION ALL
            SELECT
                base_id + 2,
                (SELECT account_id FROM ${SCHEMA}accounts WHERE name = 'adminuser'),
                1,
                1,
                1,
                1,
                'admin@example.com',
                'Primary email',
                '{}',
                ${COMMON_VALUES}
            FROM next_contact_id
            UNION ALL
            SELECT
                base_id + 3,
                (SELECT account_id FROM ${SCHEMA}accounts WHERE name = 'disableduser'),
                1,
                1,
                1,
                1,
                'disabled@example.com',
                'Primary email',
                '{}',
                ${COMMON_VALUES}
            FROM next_contact_id
            UNION ALL
            SELECT
                base_id + 4,
                (SELECT account_id FROM ${SCHEMA}accounts WHERE name = 'unauthorizeduser'),
                1,
                1,
                1,
                1,
                'unauth@example.com',
                'Primary email',
                '{}',
                ${COMMON_VALUES}
            FROM next_contact_id;

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
            -- Remove test account contacts first (foreign key constraint)
            DELETE FROM ${SCHEMA}account_contacts
            WHERE account_id IN (
                SELECT account_id FROM ${SCHEMA}accounts
                WHERE name IN ('testuser', 'adminuser', 'disableduser', 'unauthorizeduser')
            );

            ${SUBQUERY_DELIMITER}

            -- Remove test accounts
            DELETE FROM ${SCHEMA}accounts
            WHERE name IN ('testuser', 'adminuser', 'disableduser', 'unauthorizeduser');

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