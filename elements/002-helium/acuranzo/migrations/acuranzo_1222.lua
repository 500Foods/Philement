-- Migration: acuranzo_1222.lua
-- Creates the mail_routes table for the Hydrogen Mail Relay Subsystem (Phase 4B)

-- luacheck: no max line length
-- luacheck: no unused args

-- CHANGELOG
-- 1.0.0 - 2026-07-07 - Initial creation for MAILRELAY_PLAN Phase 4B (mail_routes table)

return function(engine, design_name, schema_name, cfg)
local queries = {}

cfg.TABLE = "mail_routes"
cfg.MIGRATION = "1222"
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
                route_id            ${INTEGER}          NOT NULL,
                status_a68          ${INTEGER_SMALL}    NOT NULL DEFAULT 1,
                source_network      ${VARCHAR_100}              ,
                sender_domain       ${VARCHAR_100}              ,
                sender_pattern      ${VARCHAR_100}              ,
                recipient_domain    ${VARCHAR_100}              ,
                recipient_pattern   ${VARCHAR_100}              ,
                auth_required       ${INTEGER_SMALL}    NOT NULL DEFAULT 0,
                require_tls         ${INTEGER_SMALL}    NOT NULL DEFAULT 0,
                template_key        ${VARCHAR_100}              ,
                rewrite_from        ${VARCHAR_100}              ,
                rewrite_to          ${VARCHAR_100}              ,
                add_recipients_json ${JSON}                     ,
                priority            ${INTEGER_SMALL}    NOT NULL DEFAULT 0,
                sort_seq            ${INTEGER}          NOT NULL DEFAULT 0,
                ${COMMON_CREATE}
                ${PRIMARY}(route_id)
            );

            ${SUBQUERY_DELIMITER}

            CREATE INDEX ${TABLE}_idx_status_sender
                ON ${SCHEMA}${TABLE}(status_a68, sender_domain, sort_seq);

            ${SUBQUERY_DELIMITER}

            CREATE INDEX ${TABLE}_idx_status_recipient
                ON ${SCHEMA}${TABLE}(status_a68, recipient_domain, sort_seq);

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
            Relay Subsystem. It stores inbound SMTP routing and rewrite rules
            used by the Phase 12 inbound relay to decide whether to accept a
            message and how to transform it before enqueueing for outbound
            delivery.

            ## Schema

            - **route_id**: Surrogate primary key. The application generates
              integer IDs rather than relying on an engine-specific
              autoincrement mechanism.
            - **status_a68**: Route status from Lookup 068: inactive (0),
              active (1). Defaults to active (1).
            - **source_network**: Optional CIDR or network pattern describing
              allowed client source addresses.
            - **sender_domain**: Optional domain to match on the envelope
              sender.
            - **sender_pattern**: Optional wildcard/regex pattern for sender
              matching.
            - **recipient_domain**: Optional domain to match on the envelope
              recipient.
            - **recipient_pattern**: Optional wildcard/regex pattern for
              recipient matching.
            - **auth_required**: 0 = no auth requirement, 1 = authenticated
              clients only.
            - **require_tls**: 0 = allow plaintext, 1 = require TLS before
              accepting mail.
            - **template_key**: Template to use when rewriting the inbound
              message into an outbound queued message.
            - **rewrite_from**: Override From address for the outbound send.
            - **rewrite_to**: Override To address for the outbound send.
            - **add_recipients_json**: JSON array of additional recipients to
              add during rewrite.
            - **priority**: Route evaluation priority (higher evaluated
              first).
            - **sort_seq**: Tie-breaker for stable route ordering within the
              same domain/priority.

            ## Indexes

            - PRIMARY KEY on `route_id`.
            - INDEX on `(status_a68, sender_domain, sort_seq)` for sender-based
              route lookups.
            - INDEX on `(status_a68, recipient_domain, sort_seq)` for
              recipient-based route lookups.
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
            -- DROP INDEX ${TABLE}_idx_status_sender;
            -- DROP INDEX ${TABLE}_idx_status_recipient;

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
                                "name": "route_id",
                                "datatype": "${INTEGER}",
                                "nullable": false,
                                "primary_key": true,
                                "unique": true
                            },
                            {
                                "name": "status_a68",
                                "datatype": "${INTEGER_SMALL}",
                                "nullable": false,
                                "primary_key": false,
                                "unique": false,
                                "lookup": true
                            },
                            {
                                "name": "source_network",
                                "datatype": "${VARCHAR_100}",
                                "nullable": true,
                                "primary_key": false,
                                "unique": false
                            },
                            {
                                "name": "sender_domain",
                                "datatype": "${VARCHAR_100}",
                                "nullable": true,
                                "primary_key": false,
                                "unique": false
                            },
                            {
                                "name": "sender_pattern",
                                "datatype": "${VARCHAR_100}",
                                "nullable": true,
                                "primary_key": false,
                                "unique": false
                            },
                            {
                                "name": "recipient_domain",
                                "datatype": "${VARCHAR_100}",
                                "nullable": true,
                                "primary_key": false,
                                "unique": false
                            },
                            {
                                "name": "recipient_pattern",
                                "datatype": "${VARCHAR_100}",
                                "nullable": true,
                                "primary_key": false,
                                "unique": false
                            },
                            {
                                "name": "auth_required",
                                "datatype": "${INTEGER_SMALL}",
                                "nullable": false,
                                "primary_key": false,
                                "unique": false
                            },
                            {
                                "name": "require_tls",
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
                                "name": "rewrite_from",
                                "datatype": "${VARCHAR_100}",
                                "nullable": true,
                                "primary_key": false,
                                "unique": false
                            },
                            {
                                "name": "rewrite_to",
                                "datatype": "${VARCHAR_100}",
                                "nullable": true,
                                "primary_key": false,
                                "unique": false
                            },
                            {
                                "name": "add_recipients_json",
                                "datatype": "${JSON}",
                                "nullable": true,
                                "primary_key": false,
                                "unique": false
                            },
                            {
                                "name": "priority",
                                "datatype": "${INTEGER_SMALL}",
                                "nullable": false,
                                "primary_key": false,
                                "unique": false
                            },
                            {
                                "name": "sort_seq",
                                "datatype": "${INTEGER}",
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
