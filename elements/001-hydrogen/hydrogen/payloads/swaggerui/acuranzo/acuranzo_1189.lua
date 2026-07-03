-- Migration: acuranzo_1189.lua
-- Creates the account_oidc_identities table (OIDC RP Phase 15)

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.0.0 - 2026-05-09 - Initial creation for OIDC Phase 15

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "account_oidc_identities"
cfg.MIGRATION = "1189"
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
            CREATE TABLE ${SCHEMA}${TABLE}
            (
                identity_id             ${INTEGER}          NOT NULL,
                account_id              ${INTEGER}          NOT NULL,
                issuer                  ${VARCHAR_500}      NOT NULL,
                subject                 ${VARCHAR_500}      NOT NULL,
                email                   ${VARCHAR_500}              ,
                email_verified          ${INTEGER_SMALL}            ,
                last_seen_at            ${TIMESTAMP_TZ}     NOT NULL,
                ${COMMON_CREATE}
                ${PRIMARY}(identity_id),
                ${UNIQUE}(issuer, subject)
            );

            ${SUBQUERY_DELIMITER}

            CREATE INDEX ${TABLE}_idx_account
                ON ${SCHEMA}${TABLE}(account_id);

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_APPLIED_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_FORWARD_MIGRATION};
        ]=]
                                                                            AS code,
        'Create ${TABLE} Table'                                             AS name,
        [=[
            # Forward Migration ${MIGRATION}: Create ${TABLE} Table

            This migration creates the ${TABLE} table for the OIDC Relying
            Party. Each row links a Hydrogen ${SCHEMA}accounts row to an
            external OIDC identity (identified by `(issuer, subject)` per
            OIDC Core 1.0 §2). One account can have multiple identities (one
            per IdP); each `(issuer, subject)` maps to exactly one account.

            ## Schema

            - **identity_id**: Surrogate primary key.
            - **account_id**: Foreign reference to ${SCHEMA}accounts (no SQL
              FK constraint — same convention as account_roles).
            - **issuer**: OIDC `iss` claim verbatim. Compared byte-for-byte
              per OIDC Core §3.1.3.7.
            - **subject**: OIDC `sub` claim verbatim. Opaque per IdP; stable
              across an account's lifetime.
            - **email**: Optional cached email at link time. Updated by
              QueryRef #084 on each successful login (Phase 17).
            - **email_verified**: 0/1; whether the IdP marked the email as
              verified at link time.
            - **last_seen_at**: Updated on each successful sign-in via
              QueryRef #084.

            ## Indexes

            - PRIMARY KEY on `identity_id`.
            - UNIQUE on `(issuer, subject)` — one row per IdP-side identity.
            - INDEX on `account_id` — supports reverse lookup (all
              identities for an account, used at admin-side merge).

            ## Notes

            This migration is additive. Existing `accounts` rows are not
            touched. No code reads from this table yet (Phase 17 adds the
            QueryRefs; Phase 18 adds the linker that calls them).
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
            DROP INDEX ${TABLE}_idx_account;

            ${SUBQUERY_DELIMITER}

            ${DROP_CHECK};

            ${SUBQUERY_DELIMITER}

            DROP TABLE ${SCHEMA}${TABLE};

            ${SUBQUERY_DELIMITER}

            UPDATE ${SCHEMA}${QUERIES}
              SET query_type_a28 = ${TYPE_FORWARD_MIGRATION}
            WHERE query_ref = ${MIGRATION}
              and query_type_a28 = ${TYPE_APPLIED_MIGRATION};
        ]=]
                                                                            AS code,
        'Drop ${TABLE} Table'                                               AS name,
        [=[
            # Reverse Migration ${MIGRATION}: Drop ${TABLE} Table

            This is provided for completeness when testing the migration
            system to ensure that forward and reverse migrations are
            complete. Production rollback is documented in the OIDC plan
            but not automated.
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
        ${TYPE_DIAGRAM_MIGRATION}                                           AS query_type_a28,
        ${DIALECT}                                                          AS query_dialect_a30,
        ${QTC_SLOW}                                                         AS query_queue_a58,
        ${TIMEOUT}                                                          AS query_timeout,
        'JSON Table Definition in collection'                               AS code,
        'Diagram Tables: ${SCHEMA}${TABLE}'                                 AS name,
        [=[
            # Diagram Migration ${MIGRATION}

            ## Diagram Tables: ${SCHEMA}${TABLE}

            This is the JSON Diagram code for the ${TABLE} table.
        ]=]
                                                                            AS summary,
                                                                            -- DIAGRAM_START
        ${JSON_INGEST_START}
        [=[
            {
                "diagram": [
                    {
                        "object_type": "table",
                        "object_id": "table.${TABLE}",
                        "object_ref": "${MIGRATION}",
                        "table": [
                            {
                                "name": "identity_id",
                                "datatype": "${INTEGER}",
                                "nullable": false,
                                "primary_key": true,
                                "unique": true
                            },
                            {
                                "name": "account_id",
                                "datatype": "${INTEGER}",
                                "nullable": false,
                                "primary_key": false,
                                "unique": false,
                                "lookup": false
                            },
                            {
                                "name": "issuer",
                                "datatype": "${VARCHAR_500}",
                                "nullable": false,
                                "primary_key": false,
                                "unique": false
                            },
                            {
                                "name": "subject",
                                "datatype": "${VARCHAR_500}",
                                "nullable": false,
                                "primary_key": false,
                                "unique": false
                            },
                            {
                                "name": "email",
                                "datatype": "${VARCHAR_500}",
                                "nullable": true,
                                "primary_key": false,
                                "unique": false
                            },
                            {
                                "name": "email_verified",
                                "datatype": "${INTEGER_SMALL}",
                                "nullable": true,
                                "primary_key": false,
                                "unique": false
                            },
                            {
                                "name": "last_seen_at",
                                "datatype": "${TIMESTAMP_TZ}",
                                "nullable": false,
                                "primary_key": false,
                                "unique": false
                            },
                            ${COMMON_DIAGRAM}
                        ]
                    }
                ]
            }
        ]=]
        ${JSON_INGEST_END}
                                                                            -- DIAGRAM_END
                                                                            AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})
-- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
return queries end
