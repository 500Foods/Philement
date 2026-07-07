-- Migration: acuranzo_1221.lua
-- Creates the mail_otp_codes table for the Hydrogen Mail Relay Subsystem (Phase 4B)

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.0.0 - 2026-07-07 - Initial creation for MAILRELAY_PLAN Phase 4B (mail_otp_codes table)

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "mail_otp_codes"
cfg.MIGRATION = "1221"
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
                otp_id              ${INTEGER}          NOT NULL,
                code_hash           ${VARCHAR_128}      NOT NULL,
                email               ${VARCHAR_100}      NOT NULL,
                account_id          ${INTEGER}                  ,
                purpose_a66         ${INTEGER_SMALL}    NOT NULL,
                status_a67          ${INTEGER_SMALL}    NOT NULL DEFAULT 0,
                expiry_at           ${TIMESTAMP_TZ}     NOT NULL,
                attempts            ${INTEGER_SMALL}    NOT NULL DEFAULT 0,
                max_attempts        ${INTEGER_SMALL}    NOT NULL,
                consumed_at         ${TIMESTAMP_TZ}             ,
                ${COMMON_CREATE}
                ${PRIMARY}(otp_id)
            );

            ${SUBQUERY_DELIMITER}

            CREATE INDEX ${TABLE}_idx_email_purpose_status
                ON ${SCHEMA}${TABLE}(email, purpose_a66, status_a67);

            ${SUBQUERY_DELIMITER}

            CREATE INDEX ${TABLE}_idx_status_expiry
                ON ${SCHEMA}${TABLE}(status_a67, expiry_at);

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

            This migration creates the ${TABLE} table for the Hydrogen Mail
            Relay Subsystem. It stores one-time password codes that have been
            generated and sent via email. Only the hash of the code is stored;
            the plaintext code is never persisted after the send completes.

            ## Schema

            - **otp_id**: Surrogate primary key. The application generates
              integer IDs rather than relying on an engine-specific
              autoincrement mechanism.
            - **code_hash**: Hash of the issued OTP, used for verification.
              The plaintext code is never stored here.
            - **email**: Address the OTP was sent to.
            - **account_id**: Optional account reference when the OTP is tied
              to a known account.
            - **purpose_a66**: OTP purpose from Lookup 066: login_mfa (0),
              email_verify (1), password_reset (2).
            - **status_a67**: OTP status from Lookup 067: active (0),
              consumed (1), expired (2), max_attempts_exceeded (3). Defaults
              to active (0).
            - **expiry_at**: Time after which the OTP is no longer valid.
            - **attempts**: Number of verification attempts made so far.
            - **max_attempts**: Maximum allowed verification attempts.
            - **consumed_at**: Timestamp when the OTP was successfully used;
              NULL until consumed.

            ## Indexes

            - PRIMARY KEY on `otp_id`.
            - INDEX on `(email, purpose_a66, status_a67)` for retrieving the
              active OTP for an email/purpose.
            - INDEX on `(status_a67, expiry_at)` for cleanup and expiry
              operations.
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
            -- DROP INDEX ${TABLE}_idx_email_purpose_status;
            -- DROP INDEX ${TABLE}_idx_status_expiry;

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
            system to ensure that forward and reverse migrations are complete.
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
                                "name": "otp_id",
                                "datatype": "${INTEGER}",
                                "nullable": false,
                                "primary_key": true,
                                "unique": true
                            },
                            {
                                "name": "code_hash",
                                "datatype": "${VARCHAR_128}",
                                "nullable": false,
                                "primary_key": false,
                                "unique": false
                            },
                            {
                                "name": "email",
                                "datatype": "${VARCHAR_100}",
                                "nullable": false,
                                "primary_key": false,
                                "unique": false
                            },
                            {
                                "name": "account_id",
                                "datatype": "${INTEGER}",
                                "nullable": true,
                                "primary_key": false,
                                "unique": false
                            },
                            {
                                "name": "purpose_a66",
                                "datatype": "${INTEGER_SMALL}",
                                "nullable": false,
                                "primary_key": false,
                                "unique": false,
                                "lookup": true
                            },
                            {
                                "name": "status_a67",
                                "datatype": "${INTEGER_SMALL}",
                                "nullable": false,
                                "primary_key": false,
                                "unique": false,
                                "lookup": true
                            },
                            {
                                "name": "expiry_at",
                                "datatype": "${TIMESTAMP_TZ}",
                                "nullable": false,
                                "primary_key": false,
                                "unique": false
                            },
                            {
                                "name": "attempts",
                                "datatype": "${INTEGER_SMALL}",
                                "nullable": false,
                                "primary_key": false,
                                "unique": false
                            },
                            {
                                "name": "max_attempts",
                                "datatype": "${INTEGER_SMALL}",
                                "nullable": false,
                                "primary_key": false,
                                "unique": false
                            },
                            {
                                "name": "consumed_at",
                                "datatype": "${TIMESTAMP_TZ}",
                                "nullable": true,
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
