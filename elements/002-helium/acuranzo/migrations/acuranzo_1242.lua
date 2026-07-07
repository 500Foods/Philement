-- Migration: acuranzo_1242.lua
-- QueryRef #112 - Insert OTP Code

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.0.0 - 2026-07-07 - Initial creation for MAILRELAY_PLAN Phase 4C.3

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "queries"
cfg.MIGRATION = "1242"
cfg.QUERY_REF = "112"
cfg.QUERY_NAME = "Insert OTP Code"
-- ----------------------------------------------------------------------------
-- Forward: Populate QueryRef #112 - Insert OTP Code
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
                ${TYPE_INTERNAL_SQL}                                                AS query_type_a28,
                ${DIALECT}                                                          AS query_dialect_a30,
                ${QTC_SLOW}                                                         AS query_queue_a58,
                ${TIMEOUT}                                                          AS query_timeout,
                [==[
                    ${INSERT_KEY_START} otp_id ${INSERT_KEY_END}
                        INSERT INTO ${SCHEMA}mail_otp_codes (
                            otp_id,
                            code_hash,
                            email,
                            account_id,
                            purpose_a66,
                            status_a67,
                            expiry_at,
                            attempts,
                            max_attempts,
                            consumed_at,
                            ${COMMON_FIELDS}
                        )
                        WITH next_otp_id AS (
                            SELECT COALESCE(MAX(otp_id), 0) + 1 AS new_otp_id
                            FROM ${SCHEMA}mail_otp_codes
                        )
                        SELECT
                            new_otp_id,
                            :CODE_HASH,
                            :EMAIL,
                            :ACCOUNT_ID,
                            :PURPOSE_A66,
                            0,
                            :EXPIRY_AT,
                            0,
                            :MAX_ATTEMPTS,
                            NULL,
                            ${COMMON_VALUES}
                        FROM next_otp_id
                    ${INSERT_KEY_RETURN} otp_id
                    ;
                ]==]                                                                AS code,
                '${QUERY_NAME}'                                                     AS name,
                [==[
                    # QueryRef #${QUERY_REF} - ${QUERY_NAME}

                    Inserts a new one-time password code into
                    `${SCHEMA}mail_otp_codes`. Only the hash of the code is
                    stored; the plaintext code must never be persisted.

                    ## Parameters

                    - `CODE_HASH` (string, required): Hash of the issued OTP.
                    - `EMAIL` (string, required): Address the OTP was sent to.
                    - `ACCOUNT_ID` (integer, optional): Account reference when
                      the OTP is tied to a known account.
                    - `PURPOSE_A66` (smallint, required): OTP purpose from
                      Lookup 066 (0=login_mfa, 1=email_verify, 2=password_reset).
                    - `EXPIRY_AT` (timestamp, required): Time after which the
                      OTP is no longer valid.
                    - `MAX_ATTEMPTS` (smallint, required): Maximum allowed
                      verification attempts.

                    ## Returns

                    - `otp_id`: The newly inserted OTP primary key.

                    ## Tables

                    - `${SCHEMA}mail_otp_codes` (migration 1221).

                    ## Security Notes

                    - `query_type_a28` is `${TYPE_INTERNAL_SQL}` (0) so this query
                      is not reachable via the REST API.
                    - The plaintext OTP is never stored; only `code_hash` is
                      persisted.
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

            This migration creates the internal INSERT query for OTP codes
            (QueryRef #${QUERY_REF}).
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})

-- ----------------------------------------------------------------------------
-- Reverse: Remove QueryRef #112 - Insert OTP Code
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

            This is provided for completeness when testing the migration system.
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})

return queries end
