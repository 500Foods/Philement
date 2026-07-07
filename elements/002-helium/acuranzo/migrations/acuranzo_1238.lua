-- Migration: acuranzo_1238.lua
-- QueryRef #108 - Insert Mail Event

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.0.0 - 2026-07-07 - Initial creation for MAILRELAY_PLAN Phase 4C.2

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "queries"
cfg.MIGRATION = "1238"
cfg.QUERY_REF = "108"
cfg.QUERY_NAME = "Insert Mail Event"
-- ----------------------------------------------------------------------------
-- Forward: Populate QueryRef #108 - Insert Mail Event
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
                    ${INSERT_KEY_START} event_id ${INSERT_KEY_END}
                        INSERT INTO ${SCHEMA}mail_events (
                            event_id,
                            event_key,
                            status_a65,
                            template_key,
                            from_addr,
                            reply_to,
                            recipients_json,
                            subject,
                            body_text,
                            body_html,
                            headers_json,
                            params_json,
                            debounce_key,
                            idempotency_key,
                            priority,
                            queue_id,
                            processed_at,
                            ${COMMON_FIELDS}
                        )
                        WITH next_event_id AS (
                            SELECT COALESCE(MAX(event_id), 0) + 1 AS new_event_id
                            FROM ${SCHEMA}mail_events
                        )
                        SELECT
                            new_event_id,
                            :EVENT_KEY,
                            0,
                            :TEMPLATE_KEY,
                            :FROM_ADDR,
                            :REPLY_TO,
                            :RECIPIENTS_JSON,
                            :SUBJECT,
                            :BODY_TEXT,
                            :BODY_HTML,
                            :HEADERS_JSON,
                            :PARAMS_JSON,
                            :DEBOUNCE_KEY,
                            :IDEMPOTENCY_KEY,
                            :PRIORITY,
                            NULL,
                            NULL,
                            ${COMMON_VALUES}
                        FROM next_event_id
                    ${INSERT_KEY_RETURN} event_id
                    ;
                ]==]                                                                AS code,
                '${QUERY_NAME}'                                                     AS name,
                [==[
                    # QueryRef #${QUERY_REF} - ${QUERY_NAME}

                    Inserts a new pending system-generated mail event into
                    `${SCHEMA}mail_events`. The event starts with
                    `status_a65 = 0` (pending) and `queue_id = NULL` until it
                    is converted to a queued message or suppressed.

                    ## Parameters

                    - `EVENT_KEY` (string, required): System event identifier
                      (e.g. `system.server_started`).
                    - `TEMPLATE_KEY` (string, optional): Template used when the
                      event becomes a queued message.
                    - `FROM_ADDR` (string, optional): From address for the
                      resulting message.
                    - `REPLY_TO` (string, optional): Reply-To address.
                    - `RECIPIENTS_JSON` (json, required): JSON array of
                      recipient objects.
                    - `SUBJECT` (string, optional): Rendered or raw subject.
                    - `BODY_TEXT` (string, optional): Plain text body override.
                    - `BODY_HTML` (string, optional): HTML body override.
                    - `HEADERS_JSON` (json, optional): Additional MIME headers.
                    - `PARAMS_JSON` (json, optional): Template macro values.
                    - `DEBOUNCE_KEY` (string, optional): Coalescing key.
                    - `IDEMPOTENCY_KEY` (string, optional): Duplicate-suppression
                      key.
                    - `PRIORITY` (smallint, required): Send priority when queued.

                    ## Returns

                    - `event_id`: The newly inserted event primary key.

                    ## Tables

                    - `${SCHEMA}mail_events` (migration 1220).

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

            This migration creates the internal INSERT query for mail events
            (QueryRef #${QUERY_REF}).
        ]=]
                                                                            AS summary,
        '{}'                                                                AS collection,
        ${COMMON_INSERT}
    FROM next_query_id;

]]})

-- ----------------------------------------------------------------------------
-- Reverse: Remove QueryRef #108 - Insert Mail Event
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
