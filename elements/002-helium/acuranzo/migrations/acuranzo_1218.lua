-- Migration: acuranzo_1218.lua
-- Creates the mail_queue table for the Hydrogen Mail Relay Subsystem (Phase 4B)

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.0.0 - 2026-07-07 - Initial creation for MAILRELAY_PLAN Phase 4B (mail_queue table)

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "mail_queue"
cfg.MIGRATION = "1218"
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
                queue_id            ${INTEGER}          NOT NULL,
                message_uuid        ${VARCHAR_64}       NOT NULL,
                status_a63          ${INTEGER_SMALL}    NOT NULL DEFAULT 0,
                priority            ${INTEGER_SMALL}    NOT NULL DEFAULT 0,
                template_key        ${VARCHAR_100}              ,
                from_addr           ${VARCHAR_100}              ,
                reply_to            ${VARCHAR_100}              ,
                recipients_json     ${JSON}             NOT NULL,
                subject             ${VARCHAR_500}              ,
                body_text           ${TEXT_BIG}                 ,
                body_html           ${TEXT_BIG}                 ,
                headers_json        ${JSON}                     ,
                idempotency_key     ${VARCHAR_128}              ,
                attempts            ${INTEGER_SMALL}    NOT NULL DEFAULT 0,
                next_attempt_at     ${TIMESTAMP_TZ}             ,
                last_attempt_at     ${TIMESTAMP_TZ}             ,
                smtp_code           ${INTEGER}                  ,
                smtp_text           ${VARCHAR_500}              ,
                server_index        ${INTEGER_SMALL}    NOT NULL DEFAULT 0,
                instance_id         ${VARCHAR_64}               ,
                claim_token         ${VARCHAR_64}               ,
                ${COMMON_CREATE}
                ${PRIMARY}(queue_id),
                ${UNIQUE}(message_uuid)
            );

            ${SUBQUERY_DELIMITER}

            CREATE INDEX ${TABLE}_idx_status_next_attempt
                ON ${SCHEMA}${TABLE}(status_a63, next_attempt_at);

            ${SUBQUERY_DELIMITER}

            CREATE INDEX ${TABLE}_idx_idempotency
                ON ${SCHEMA}${TABLE}(idempotency_key);

            ${SUBQUERY_DELIMITER}

            CREATE INDEX ${TABLE}_idx_instance_claim
                ON ${SCHEMA}${TABLE}(instance_id, claim_token);

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
            Relay Subsystem. It is the durable outbound queue for mail
            messages, separate from the in-memory queue used for transient
            throughput. Queue bodies are stored as structured fields only;
            no pre-rendered RFC 5322 payload is kept here.

            ## Schema

            - **queue_id**: Surrogate primary key. The application generates
              integer IDs rather than relying on an engine-specific
              autoincrement mechanism.
            - **message_uuid**: Stable external identifier for the message,
              returned to callers and used for idempotency lookups.
            - **status_a63**: Queue status from Lookup 063: pending (0),
              sending (1), sent (2), failed (3), retrying (4). Defaults to
              pending (0).
            - **priority**: Send priority (higher values sent first).
            - **template_key**: Optional template used to render the message;
              NULL for raw sends.
            - **from_addr**: Envelope/message From address.
            - **reply_to**: Reply-To address.
            - **recipients_json**: JSON array of recipient objects (to/cc/bcc).
            - **subject**: Rendered or raw subject line.
            - **body_text**: Plain text body.
            - **body_html**: HTML body.
            - **headers_json**: JSON object of additional MIME headers.
            - **idempotency_key**: Caller-supplied key for duplicate
              suppression.
            - **attempts**: Number of delivery attempts made so far.
            - **next_attempt_at**: Scheduled time for the next attempt;
              NULL when no retry is pending.
            - **last_attempt_at**: Time of the most recent attempt.
            - **smtp_code**: Last SMTP reply code received.
            - **smtp_text**: Last SMTP reply text received.
            - **server_index**: Index of the outbound server used/attempted.
            - **instance_id**: Owning Hydrogen instance for HA claim
              coordination (Phase 11).
            - **claim_token**: Random token held while a worker is sending
              this row (Phase 11).

            ## Indexes

            - PRIMARY KEY on `queue_id`.
            - UNIQUE on `message_uuid`.
            - INDEX on `(status_a63, next_attempt_at)` for claim-next-pending
              queries.
            - INDEX on `idempotency_key` for duplicate suppression.
            - INDEX on `(instance_id, claim_token)` for stale-claim recovery.
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
            -- DROP INDEX ${TABLE}_idx_status_next_attempt;
            -- DROP INDEX ${TABLE}_idx_idempotency;
            -- DROP INDEX ${TABLE}_idx_instance_claim;

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
                                "name": "queue_id",
                                "datatype": "${INTEGER}",
                                "nullable": false,
                                "primary_key": true,
                                "unique": true
                            },
                            {
                                "name": "message_uuid",
                                "datatype": "${VARCHAR_64}",
                                "nullable": false,
                                "primary_key": false,
                                "unique": true
                            },
                            {
                                "name": "status_a63",
                                "datatype": "${INTEGER_SMALL}",
                                "nullable": false,
                                "primary_key": false,
                                "unique": false,
                                "lookup": true
                            },
                            {
                                "name": "priority",
                                "datatype": "${INTEGER_SMALL}",
                                "nullable": false,
                                "primary_key": false,
                                "unique": false
                            },
                            {
                                "name": "template_key",
                                "datatype": "${VARCHAR_100}",
                                "nullable": true,
                                "primary_key": false,
                                "unique": false
                            },
                            {
                                "name": "from_addr",
                                "datatype": "${VARCHAR_100}",
                                "nullable": true,
                                "primary_key": false,
                                "unique": false
                            },
                            {
                                "name": "reply_to",
                                "datatype": "${VARCHAR_100}",
                                "nullable": true,
                                "primary_key": false,
                                "unique": false
                            },
                            {
                                "name": "recipients_json",
                                "datatype": "${JSON}",
                                "nullable": false,
                                "primary_key": false,
                                "unique": false
                            },
                            {
                                "name": "subject",
                                "datatype": "${VARCHAR_500}",
                                "nullable": true,
                                "primary_key": false,
                                "unique": false
                            },
                            {
                                "name": "body_text",
                                "datatype": "${TEXT_BIG}",
                                "nullable": true,
                                "primary_key": false,
                                "unique": false
                            },
                            {
                                "name": "body_html",
                                "datatype": "${TEXT_BIG}",
                                "nullable": true,
                                "primary_key": false,
                                "unique": false
                            },
                            {
                                "name": "headers_json",
                                "datatype": "${JSON}",
                                "nullable": true,
                                "primary_key": false,
                                "unique": false
                            },
                            {
                                "name": "idempotency_key",
                                "datatype": "${VARCHAR_128}",
                                "nullable": true,
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
                                "name": "next_attempt_at",
                                "datatype": "${TIMESTAMP_TZ}",
                                "nullable": true,
                                "primary_key": false,
                                "unique": false
                            },
                            {
                                "name": "last_attempt_at",
                                "datatype": "${TIMESTAMP_TZ}",
                                "nullable": true,
                                "primary_key": false,
                                "unique": false
                            },
                            {
                                "name": "smtp_code",
                                "datatype": "${INTEGER}",
                                "nullable": true,
                                "primary_key": false,
                                "unique": false
                            },
                            {
                                "name": "smtp_text",
                                "datatype": "${VARCHAR_500}",
                                "nullable": true,
                                "primary_key": false,
                                "unique": false
                            },
                            {
                                "name": "server_index",
                                "datatype": "${INTEGER_SMALL}",
                                "nullable": false,
                                "primary_key": false,
                                "unique": false
                            },
                            {
                                "name": "instance_id",
                                "datatype": "${VARCHAR_64}",
                                "nullable": true,
                                "primary_key": false,
                                "unique": false
                            },
                            {
                                "name": "claim_token",
                                "datatype": "${VARCHAR_64}",
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
