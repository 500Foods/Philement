-- Migration: acuranzo_1194.lua
-- QueryRef #083 - OIDC RP: Provision OIDC Account (Phase 17)

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.0.0 - 2026-05-09 - Initial creation for OIDC Phase 17

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "queries"
cfg.MIGRATION = "1194"
cfg.QUERY_REF = "083"
cfg.QUERY_NAME = "OIDC RP: Provision OIDC Account"
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
        ${QTC_SLOW}                                                         AS query_queue_a58,
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
                ${QTC_MEDIUM}                                                       AS query_queue_a58,
                ${TIMEOUT}                                                          AS query_timeout,
                [==[
                    ${INSERT_KEY_START} account_id ${INSERT_KEY_END}
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
                        WITH next_account_id AS (
                            SELECT COALESCE(MAX(account_id), 0) + 1 AS new_account_id
                            FROM ${SCHEMA}accounts
                        )
                        SELECT
                            new_account_id,
                            :USERNAME,
                            :FIRST_NAME,
                            :LAST_NAME,
                            NULL,
                            1,
                            1,
                            :SUMMARY,
                            '{}',
                            '2025-01-01 00:00:00',
                            '2035-01-01 00:00:00',
                            0,
                            ${NOW},
                            0,
                            ${NOW}
                        FROM next_account_id
                    ${INSERT_KEY_RETURN} account_id
                    ;
                ]==]                                                                AS code,
                '${QUERY_NAME}'                                                     AS name,
                [==[
                    #  QueryRef #${QUERY_REF} - ${QUERY_NAME}

                    Creates a new Hydrogen `accounts` row for a first-time OIDC user who
                    has no existing account (the `provision_only` and
                    `match_email_then_provision` strategies). The account is created without
                    a password hash (`password_hash = NULL`) because authentication is
                    performed by the external IdP; Phase 16's migration relaxed the NOT NULL
                    constraint on `password_hash` for exactly this purpose.

                    After this query returns, the caller (Phase 20 linker) MUST immediately
                    call QueryRef #081 to insert the `account_oidc_identities` row that links
                    the new account to the OIDC identity. Without that follow-up call the
                    account exists but is unlinked, and a subsequent sign-in would attempt
                    to provision a second account.

                    The caller is responsible for domain-allow-list validation
                    (`AllowedEmailDomains` config), `RequireEmailVerified`, and default role
                    assignment before or after calling this query.

                    ## Parameters

                    - `USERNAME` (string): The username for the new account (stored in
                      `accounts.name`). Typically derived from the IdP's
                      `preferred_username` claim, falling back to the email local-part.
                      Must be unique; the caller should check availability first.
                    - `FIRST_NAME` (string): From IdP `given_name` claim; empty string
                      if not provided.
                    - `LAST_NAME` (string): From IdP `family_name` claim; empty string
                      if not provided.
                    - `SUMMARY` (string): Brief description; defaults to empty string.

                    ## Returns

                    - `account_id` (integer): The PK of the newly created account row.

                    ## Tables

                    - `${SCHEMA}accounts`: User accounts table. `password_hash` is nullable
                      since Phase 16 (acuranzo_1190).

                    ## Notes

                    - `password_hash` is explicitly inserted as NULL — do not rely on a
                      column default. (Phase 17 plan note from setup section.)
                    - `status_a16 = 1` (active) and `iana_timezone_a17 = 1` (UTC default)
                      are the same defaults used by QueryRef #051 (Create New Account).
                    - `ProvisionDefaults.Authorized = true` from config maps to
                      `status_a16 = 1`. If the config says `Authorized = false`, the caller
                      should pass status 0 — future versions may parameterise this.
                    - Role assignment after provisioning is handled by the Phase 20/22 C
                      code, not by this query.

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

            This migration creates the query for QueryRef #${QUERY_REF} - ${QUERY_NAME}
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

            This is provided for completeness when testing the migration system
            to ensure that forward and reverse migrations are complete.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
return queries end
