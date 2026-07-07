-- Migration: acuranzo_1232.lua
-- QueryRef #102 - Insert Mail Attempt

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.0.0 - 2026-07-07 - Initial creation for MAILRELAY_PLAN Phase 4C.1b

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "queries"
cfg.MIGRATION = "1232"
cfg.QUERY_REF = "102"
cfg.QUERY_NAME = "Insert Mail Attempt"
-- ----------------------------------------------------------------------------
-- Forward: Populate QueryRef #102 - Insert Mail Attempt
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
                    ${INSERT_KEY_START} attempt_id ${INSERT_KEY_END}
                        INSERT INTO ${SCHEMA}mail_attempts (
                            attempt_id,
                            queue_id,
                            attempt_number,
                            server_index,
                            success,
                            smtp_code,
                            smtp_text,
                            duration_ms,
                            error_class,
                            ${COMMON_FIELDS}
                        )
                        WITH next_attempt_id AS (
                            SELECT COALESCE(MAX(attempt_id), 0) + 1 AS new_attempt_id
                            FROM ${SCHEMA}mail_attempts
                        )
                        SELECT
                            new_attempt_id,
                            :QUEUE_ID,
                            :ATTEMPT_NUMBER,
                            :SERVER_INDEX,
                            :SUCCESS,
                            :SMTP_CODE,
                            :SMTP_TEXT,
                            :DURATION_MS,
                            :ERROR_CLASS,
                            ${COMMON_VALUES}
                        FROM next_attempt_id
                    ${INSERT_KEY_RETURN} attempt_id
                    ;
                ]==]                                                                AS code,
                '${QUERY_NAME}'                                                     AS name,
                [==[
                    # QueryRef #${QUERY_REF} - ${QUERY_NAME}

                    Inserts a single delivery attempt record for an outbound
                    mail queue row.

                    ## Parameters

                    - `QUEUE_ID` (integer, required): Logical foreign key to
                      `mail_queue.queue_id`.
                    - `ATTEMPT_NUMBER` (smallint, required): 1-based attempt
                      number for this message.
                    - `SERVER_INDEX` (smallint, required): Index of the outbound
                      server used.
                    - `SUCCESS` (smallint, required): 0 = failed, 1 = succeeded.
                    - `SMTP_CODE` (integer, optional): SMTP reply code, if any.
                    - `SMTP_TEXT` (string, optional): SMTP reply text, if any.
                    - `DURATION_MS` (integer, optional): Elapsed attempt time
                      in milliseconds.
                    - `ERROR_CLASS` (string, optional): Failure classification
                      (e.g. `transient`, `permanent`, `timeout`, `tls`).

                    ## Returns

                    - `attempt_id`: The newly inserted attempt primary key.

                    ## Tables

                    - `${SCHEMA}mail_attempts` (migration 1219).

                    ## Security Notes

                    - `query_type_a28` is `${TYPE_INTERNAL_SQL}` (0) so this query
                      is not reachable via the REST API.
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

            This migration creates the internal INSERT query for mail delivery
            attempts (QueryRef #${QUERY_REF}).
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})

-- ----------------------------------------------------------------------------
-- Reverse: Remove QueryRef #102 - Insert Mail Attempt
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
