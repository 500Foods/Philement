-- Migration: acuranzo_1223.lua
-- QueryRef #093 - Insert Pending Mail Queue Row

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.0.0 - 2026-07-07 - Initial creation for MAILRELAY_PLAN Phase 4C.1

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "queries"
cfg.MIGRATION = "1223"
cfg.QUERY_REF = "093"
cfg.QUERY_NAME = "Insert Pending Mail Queue Row"
-- ----------------------------------------------------------------------------
-- Forward: Populate QueryRef #093 - Insert Pending Mail Queue Row
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
                    ${INSERT_KEY_START} queue_id ${INSERT_KEY_END}
                        INSERT INTO ${SCHEMA}mail_queue (
                            queue_id,
                            message_uuid,
                            status_a63,
                            priority,
                            template_key,
                            from_addr,
                            reply_to,
                            recipients_json,
                            subject,
                            body_text,
                            body_html,
                            headers_json,
                            idempotency_key,
                            attempts,
                            next_attempt_at,
                            server_index,
                            instance_id,
                            claim_token,
                            ${COMMON_FIELDS}
                        )
                        WITH next_queue_id AS (
                            SELECT COALESCE(MAX(queue_id), 0) + 1 AS new_queue_id
                            FROM ${SCHEMA}mail_queue
                        )
                        SELECT
                            new_queue_id,
                            :MESSAGE_UUID,
                            0,
                            :PRIORITY,
                            :TEMPLATE_KEY,
                            :FROM_ADDR,
                            :REPLY_TO,
                            :RECIPIENTS_JSON,
                            :SUBJECT,
                            :BODY_TEXT,
                            :BODY_HTML,
                            :HEADERS_JSON,
                            :IDEMPOTENCY_KEY,
                            0,
                            :NEXT_ATTEMPT_AT,
                            0,
                            NULL,
                            NULL,
                            ${COMMON_VALUES}
                        FROM next_queue_id
                    ${INSERT_KEY_RETURN} queue_id
                    ;
                ]==]                                                                AS code,
                '${QUERY_NAME}'                                                     AS name,
                [==[
                    # QueryRef #${QUERY_REF} - ${QUERY_NAME}

                    Inserts a new pending row into `${SCHEMA}mail_queue`.
                    The application generates `queue_id` rather than relying on
                    an engine-specific autoincrement.

                    ## Parameters

                    - `MESSAGE_UUID` (string, required): Stable external message ID.
                    - `PRIORITY` (smallint, required): Send priority (higher first).
                    - `TEMPLATE_KEY` (string, optional): Template used to render the message.
                    - `FROM_ADDR` (string, optional): Envelope/message From address.
                    - `REPLY_TO` (string, optional): Reply-To address.
                    - `RECIPIENTS_JSON` (json, required): JSON array of recipient objects.
                    - `SUBJECT` (string, optional): Rendered or raw subject.
                    - `BODY_TEXT` (string, optional): Plain text body.
                    - `BODY_HTML` (string, optional): HTML body.
                    - `HEADERS_JSON` (json, optional): Additional MIME headers.
                    - `IDEMPOTENCY_KEY` (string, optional): Caller-supplied duplicate key.
                    - `NEXT_ATTEMPT_AT` (timestamp, optional): Scheduled first attempt time.

                    ## Returns

                    - `queue_id`: The newly inserted queue primary key.

                    ## Tables

                    - `${SCHEMA}mail_queue` (migration 1218).

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

            This migration creates the internal INSERT query for the outbound
            mail queue (QueryRef #${QUERY_REF}).
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})

-- ----------------------------------------------------------------------------
-- Reverse: Remove QueryRef #093 - Insert Pending Mail Queue Row
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
