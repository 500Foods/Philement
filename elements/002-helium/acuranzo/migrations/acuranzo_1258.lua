-- Migration: acuranzo_1258.lua
-- Create Mail Relay mailadmin user with mail_send role

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.0.0 - 2026-07-08 - Initial creation for MAILRELAY_PLAN Phase 7.5b

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "accounts"
cfg.MIGRATION = "1258"
-- ----------------------------------------------------------------------------
-- Forward: Insert mailadmin account, contacts, and role assignment
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
            -- Insert the mailadmin account. Credentials are supplied through
            -- HYDROGEN_MAILADMIN_NAME / HYDROGEN_MAILADMIN_PASS env vars so the
            -- test harness can authenticate without hardcoding production secrets.
            INSERT INTO ${SCHEMA}accounts (
                account_id,
                status_a16,
                iana_timezone_a17,
                name,
                first_name,
                middle_name,
                last_name,
                password_hash,
                summary,
                collection,
                ${COMMON_FIELDS}
            )
            VALUES (
                5,                              -- account_id
                1,                              -- Enabled (Account Status)
                1,                              -- America/Vancouver-ish default
                '${HYDROGEN_MAILADMIN_NAME}',   -- login username
                'Mail',                         -- first_name
                '',                             -- middle_name
                'Admin',                        -- last_name
                ${SHA256_HASH_START}'5'${SHA256_HASH_MID}'${HYDROGEN_MAILADMIN_PASS}'${SHA256_HASH_END},
                'Mail Relay administrative test account',
                '{}',
                ${COMMON_VALUES}
            );

            ${SUBQUERY_DELIMITER}

            -- Username contact for login
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
                7,
                5,
                0,
                0,                              -- Username
                1,                              -- Can authenticate
                1,                              -- Active
                '${HYDROGEN_MAILADMIN_NAME}',
                'mailadmin primary username',
                '{}',
                ${COMMON_VALUES}
            );

            ${SUBQUERY_DELIMITER}

            -- Email contact
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
                8,
                5,
                1,
                1,                              -- Email
                1,
                1,
                '${HYDROGEN_MAILADMIN_EMAIL}',
                'mailadmin primary email',
                '{}',
                ${COMMON_VALUES}
            );

            ${SUBQUERY_DELIMITER}

            -- Assign the mail_send role (role_id 1 from migration 1257)
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
            VALUES (
                5,
                1,
                0,
                1,                              -- Active
                'mail_send',
                'mailadmin mail_send assignment',
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
        'Create mailadmin Account and Role Assignment'                      AS name,
        [=[
            # Forward Migration ${MIGRATION}: Create mailadmin Account and Role Assignment

            Creates a dedicated `mailadmin` test account (account_id 5), adds
            username/email contacts, and assigns the `mail_send` role so the
            blackbox API test can authenticate and send/preview mail.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})

-- ----------------------------------------------------------------------------
-- Reverse: Remove mailadmin account, contacts, and role assignment
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
            DELETE FROM ${SCHEMA}account_roles
            WHERE account_id = 5;

            ${SUBQUERY_DELIMITER}

            DELETE FROM ${SCHEMA}account_contacts
            WHERE account_id = 5;

            ${SUBQUERY_DELIMITER}

            DELETE FROM ${SCHEMA}accounts
            WHERE account_id = 5;

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Remove mailadmin Account and Role Assignment'                      AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Remove mailadmin Account and Role Assignment

            Deletes the `mailadmin` account, contacts, and role assignment.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})

return queries end
